"""Internal observer identity/provenance evidence, not IEEE RTI conformance."""
from __future__ import annotations

import copy
import json

import pytest
from fastapi.testclient import TestClient
from pydantic import ValidationError
from umbra_rti_observer.analytics import build_analytics
from umbra_rti_observer.attribution import event_perspective
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import (
    AnalyticsResponse,
    AnalyticsWindow,
    EventInput,
    ParticipantPerspectiveResponse,
    StateResponse,
)
from umbra_rti_observer.server import create_app


def attribution(*, namespace="ns", federation="fed", participant="p", membership="join", session="session", name="Twin", handle="7"):
    return {"session_id": session, "identity_namespace": namespace, "federation_id": federation,
            "participant": {"participant_id": participant, "membership_id": membership, "display_name": name, "handle": handle}}


def event(*, source="producer", identity=None, **values):
    return {"event_type": "federate.joined", "source": source, "participant_attribution": identity or attribution(), **values}


@pytest.mark.parametrize("fields", [
    {"session_id": " session "},
    {"time_observation": {"kind": "grant", "session_id": " session ", "logical_time": 1}},
    {"declaration": {"kind": "interaction", "direction": "publication", "status": "declared", "class_key": "Signal", "session_id": " session "}},
])
def test_compact_analytics_metadata_does_not_invent_a_second_unreported_session(fields):
    store = ObserverStore()
    store.publish({"event_type": "callback.invoked", "source": "reporter", "federate": "Twin", "observed_at": 100, **fields})
    state = StateResponse.model_validate(store.state(include_events=True))
    now = state.normalized_events[0].observed_at
    expected = state.federate_roster[0].perspective.key
    for snapshot in (state, AnalyticsWindow.model_validate(store.analytics_window())):
        report = build_analytics(snapshot, now=now)
        assert len(report.participants) == 1
        assert report.participants[0].perspective.key == expected
        assert report.participants[0].event_count == 1
        assert build_analytics(snapshot, now=now, perspective=expected).events_in_window == 1


def test_legacy_name_filter_keeps_event_counts_when_the_same_explicit_join_has_been_renamed():
    store = ObserverStore()
    store.publish(event(event_type="callback.invoked", identity=attribution(name="Old name")))
    store.publish(event(event_type="callback.invoked", identity=attribution(name="New name")))
    snapshot = StateResponse.model_validate(store.state(include_events=True))
    for name in ("Old name", "New name"):
        report = build_analytics(snapshot, federate=name)
        assert report.events_in_window == 1
        assert len(report.participants) == 1 and report.participants[0].event_count == 1
        assert report.participants[0].federate == "New name"


def test_a_legacy_raw_handle_that_looks_like_a_scoped_object_key_cannot_alias_it():
    store = ObserverStore()
    scoped = json.dumps(["federation-object", "ns", "fed", "raw"], separators=(",", ":"))
    store.publish_many([
        event(event_type="object.updated", object_key="raw", object_context={"identity_namespace": "ns", "federation_id": "fed"}, attributes={"value": 1}),
        event(event_type="object.updated", object_key=scoped, attributes={"value": 2}),
    ])
    snapshot = StateResponse.model_validate(store.state())
    assert len(snapshot.inspectors.objects) == 2
    assert len(snapshot.inspectors.object_observations.snapshots) == 4
    assert len({row.cache_key for row in snapshot.inspectors.object_observations.snapshots}) == 4


def test_wire_models_reject_duplicate_identified_roster_objects_and_analytics_even_without_storage_metadata():
    store = ObserverStore()
    store.publish(event(event_type="object.updated", object_key="one"))
    snapshot = StateResponse.model_validate(store.state(include_events=True))
    for collection in ("roster", "objects"):
        body = snapshot.model_dump(mode="json")
        body["storage"] = None
        body["capabilities"].pop("storage_inspection", None)
        rows = body["federate_roster"] if collection == "roster" else body["inspectors"]["objects"]
        rows.append(copy.deepcopy(rows[0]))
        with pytest.raises(ValidationError, match="must be unique"):
            StateResponse.model_validate(body)
    analytics = build_analytics(snapshot).model_dump(mode="json")
    analytics["participants"].append(copy.deepcopy(analytics["participants"][0]))
    with pytest.raises(ValidationError, match="must be unique"):
        AnalyticsResponse.model_validate(analytics)


@pytest.mark.parametrize("invalid", [{"federate": 7}, {"participant": True}, {"federation": 17},
    {"federate_name": ["not a label"]}, {"participant_name": {"name": "not a string"}}, {"object_key": 7}])
def test_core_and_http_reject_invalid_known_actor_shapes_atomically_before_bad_wire_state(invalid):
    store = ObserverStore()
    with pytest.raises(TypeError):
        store.publish_many([event(), {"event_type": "callback.invoked", **invalid}])
    assert store.state()["retention"]["next_sequence"] == 1
    assert store.state()["federate_roster"] == []
    with TestClient(create_app(store)) as client:
        assert client.post("/api/events", json={"events": [event(), {"event_type": "callback.invoked", **invalid}]}).status_code == 422
    assert store.state()["retention"]["next_sequence"] == 1


def test_opaque_object_keys_are_not_trimmed_into_another_objects_baseline():
    store = ObserverStore()
    for key, value in ((" raw ", 1), ("raw", 2)):
        store.publish(event(event_type="object.updated", object_key=key,
            object_context={"identity_namespace": "ns", "federation_id": "fed"}, attributes={"value": value}))
    snapshot = StateResponse.model_validate(store.state())
    assert {row.object_key for row in snapshot.inspectors.objects} == {" raw ", "raw"}
    assert len(snapshot.inspectors.object_observations.snapshots) == 4
    assert all(not change.previous_known for change in snapshot.inspectors.object_observations.changes)


def test_http_preserves_opaque_object_keys_and_explicit_reported_aliases_through_all_consumers():
    store = ObserverStore()
    subject = attribution(name=" Twin ")
    events = [
        event(identity=subject, event_type="object.updated", object_key=" raw ",
            object_context={"identity_namespace": "ns", "federation_id": "fed"}, attributes={"value": 1}),
        event(identity=subject, event_type="time.observed", time_observation={"kind": "grant", "logical_time": 1}),
        event(identity=subject, event_type="declaration.observed", declaration={"kind": "interaction", "direction": "publication", "status": "declared", "class_key": "Signal"}),
        event(identity=subject, event_type="interaction.received", interaction_class="Signal"),
    ]
    with TestClient(create_app(store)) as client:
        assert client.post("/api/events", json={"events": events}).status_code == 200
        response = client.get("/api/state?include_events=true")
        assert response.status_code == 200
        snapshot = StateResponse.model_validate(response.json())
        assert snapshot.federate_roster[0].federate == " Twin "
        assert snapshot.inspectors.objects[0].object_key == " raw "
        assert snapshot.inspectors.time.snapshots[0].scope.federate == " Twin "
        assert snapshot.inspectors.declarations.snapshots[0].scope.federate == " Twin "
        assert snapshot.inspectors.interactions[0].federate == " Twin "
        analytics = AnalyticsResponse.model_validate(client.get("/api/analytics").json())
        assert analytics.participants[0].federate == " Twin "
        assert analytics.participants[0].object_keys == [" raw "]
        assert analytics.logical_times[0].federate == " Twin "


@pytest.mark.parametrize("identified", [False, True])
def test_explicit_root_reporting_sessions_are_shared_by_objects_time_and_declarations_without_join_id_inference(identified):
    store = ObserverStore()
    actor = {"source": "reporter", "federate": "Twin", "session_id": " session "}
    if identified:
        actor["participant_attribution"] = attribution(session=" session ")
    store.publish_many([
        {**actor, "event_type": "object.updated", "object_key": "one"},
        {**actor, "event_type": "time.observed", "time_observation": {"kind": "request", "request_id": "r1", "logical_time": 1}},
        {**actor, "event_type": "time.observed", "time_observation": {"kind": "grant", "request_id": "r1", "logical_time": 1}},
        {**actor, "event_type": "declaration.observed", "declaration": {"kind": "interaction", "direction": "publication", "status": "declared", "class_key": "Signal"}},
    ])
    snapshot = StateResponse.model_validate(store.state(include_events=True))
    assert len(snapshot.federate_roster) == 1
    key = snapshot.federate_roster[0].perspective.key
    assert snapshot.inspectors.time.requests[0].status == "granted"
    for scope in (snapshot.inspectors.time.snapshots[0].scope, snapshot.inspectors.declarations.snapshots[0].scope):
        assert scope.session_id == " session " and scope.perspective.key == key
        assert (scope.perspective.attribution is not None) is identified
    assert len(build_analytics(snapshot).participants) == 1


def test_key_uses_opaque_join_ids_and_reporting_provenance_not_names_handles_or_federation_labels():
    first = event_perspective(event())
    renamed = event_perspective(event(identity=attribution(name="Renamed", handle="88"), federation="Changed display label"))
    assert first is not None and renamed is not None
    assert first == renamed and hash(first) == hash(renamed) and first.label != renamed.label
    assert json.loads(first.key) == ["explicit-join", "producer", "session", "ns", "fed", "p", "join"]
    for identity in (attribution(namespace="other"), attribution(federation="other"), attribution(participant="other"),
                     attribution(membership="other"), attribution(session="other")):
        assert event_perspective(event(identity=identity)) != first
    assert event_perspective(event(source="other")) != first


def test_ordinary_roster_keeps_same_names_and_handles_separate_and_updates_aliases_in_one_join():
    store = ObserverStore()
    identities = [attribution(), attribution(namespace="other"), attribution(federation="other"),
                  attribution(participant="other"), attribution(membership="other"), attribution(session="other")]
    store.publish_many([event(identity=identity) for identity in identities] + [event(source="other")])
    assert len(store.state()["federate_roster"]) == 7
    store.publish(event(identity=attribution(name="Renamed", handle="99"), event_type="federate.updated"))
    roster = StateResponse.model_validate(store.state()).federate_roster
    assert len(roster) == 7
    renamed = [row for row in roster if row.federate == "Renamed"]
    assert len(renamed) == 1 and renamed[0].handle == "99"
    assert len({row.perspective.key for row in roster}) == 7


def test_explicit_join_end_is_not_reopened_by_late_observations_or_another_same_named_membership():
    store = ObserverStore()
    store.publish(event())
    store.publish(event(event_type="federate.resigned"))
    store.publish(event(event_type="object.updated", object_key="late", attributes={"x": 1}))
    store.publish(event(identity=attribution(membership="fresh")))
    rows = StateResponse.model_validate(store.state()).federate_roster
    assert {row.perspective.attribution.participant.membership_id: row.status for row in rows} == {"join": "resigned", "fresh": "active"}
    store.publish({"event_type": "session.disconnected", "source": "unrelated", "session_id": "session"})
    assert any(row.status == "active" for row in StateResponse.model_validate(store.state()).federate_roster)
    store.publish({"event_type": "session.disconnected", "source": "producer", "session_id": "session"})
    assert any(row.status == "active" for row in StateResponse.model_validate(store.state()).federate_roster)
    store.publish(event(event_type="session.disconnected", session_id="session"))
    assert {row.status for row in StateResponse.model_validate(store.state()).federate_roster} == {"resigned", "reporting-ended"}


def test_legacy_metadata_is_explicitly_unresolved_and_does_not_merge_different_reporters_or_sessions():
    store = ObserverStore()
    store.publish_many([
        {"event_type": "object.updated", "federate": "Twin", "source": "a", "session_id": "s1"},
        {"event_type": "object.updated", "federate": "Twin", "source": "a", "session_id": "s2"},
        {"event_type": "object.updated", "federate": "Twin", "source": "b", "session_id": "s1"},
    ])
    rows = StateResponse.model_validate(store.state()).federate_roster
    assert len(rows) == 3 and all(row.perspective.basis == "legacy-provenance" and row.perspective.attribution is None for row in rows)
    assert len({row.perspective.key for row in rows}) == 3
    assert event_perspective({"source": "Twin", "participant_handle": "7"}) is None
    assert event_perspective({"source": "reporter", "mom_observation": {"subject": attribution()},
        "relationship": {"producer": attribution()}, "federation_id": "fed", "identity_namespace": "ns"}) is None


@pytest.mark.parametrize("changes", [
    {"identity_namespace": ""}, {"federation_id": 7}, {"session_id": " "}, {"extra": "not-a-field"},
    {"participant": {"display_name": "Twin", "handle": "7"}},
    {"participant": {"participant_id": "p", "membership_id": False}},
])
def test_invalid_explicit_identity_rejects_the_whole_batch_before_mutation(changes):
    store = ObserverStore()
    bad = {**attribution(), **changes}
    with pytest.raises((TypeError, ValueError)):
        store.publish_many([event(), event(identity=bad)])
    assert store.state()["retention"]["next_sequence"] == 1
    assert store.state()["federate_roster"] == []
    with pytest.raises(ValidationError):
        EventInput.model_validate(event(identity=bad))


def test_perspective_metadata_and_key_are_semantically_validated_not_opaque_tags():
    perspective = event_perspective(event())
    valid = perspective.to_json()
    assert ParticipantPerspectiveResponse.model_validate(valid).key == perspective.key
    for field, value in (("key", "made-up"), ("basis", "legacy-provenance"), ("session_id", "other")):
        with pytest.raises(ValidationError):
            ParticipantPerspectiveResponse.model_validate({**valid, field: value})


def test_analytics_and_exact_http_filter_keep_same_names_independent_even_after_roster_expiry():
    store = ObserverStore(max_federates=1)
    a, b = attribution(membership="a"), attribution(membership="b")
    store.publish_many([event(identity=a, event_type="object.updated", object_key="a"),
        event(identity=b, event_type="object.updated", object_key="b"),
        event(identity=a, event_type="runtime.error")])
    snapshot = StateResponse.model_validate(store.state())
    result = build_analytics(snapshot)
    assert result.events_in_window == 3 and sum(row.event_count for row in result.participants) == 3
    assert sorted(row.event_count for row in result.participants) == [1, 2]
    assert len({row.perspective.key for row in result.participants}) == 2
    assert sum(row.roster_retained for row in result.participants) == 1
    first = event_perspective(event(identity=a)).key
    with TestClient(create_app(store)) as client:
        reply = client.get("/api/analytics", params={"perspective": first})
        assert reply.status_code == 200
        body = reply.json()
        assert body["scoped_perspective"] == first and body["events_in_window"] == 2
        assert len(body["participants"]) == 1 and body["participants"][0]["object_keys"] == ["a"]
        assert client.get("/api/analytics", params={"perspective": "missing"}).json()["events_in_window"] == 0


def test_ingress_rejects_conflicting_explicit_and_reported_sessions_without_provider_side_effects():
    store = ObserverStore()
    original = copy.deepcopy(event())
    with TestClient(create_app(store)) as client:
        for extra in ({"session_id": "other"}, {"time_observation": {"kind": "grant", "session_id": "other", "logical_time": 0}}):
            assert client.post("/api/events", json={**original, **extra}).status_code == 422
        assert store.state()["retention"]["retained_events"] == 0
        assert client.post("/api/events", json=original).status_code == 200
    assert original == event()


def test_object_comparisons_isolate_joins_and_explicit_federation_contexts_without_relabeling_raw_keys():
    store = ObserverStore()
    context = {"identity_namespace": "ns", "federation_id": "fed"}
    def update(identity, value, object_context=context):
        store.publish(event(identity=identity, event_type="object.updated", object_key="same", object_context=object_context, attributes={"x": value}))
    update(attribution(membership="a"), 1)
    update(attribution(membership="b"), 20)
    update(attribution(membership="a", name="Renamed"), 2)
    update(attribution(membership="a", namespace="other"), 50, {**context, "identity_namespace": "other"})
    snapshot = StateResponse.model_validate(store.state())
    assert len(snapshot.inspectors.objects) == 2
    assert {row.object_key for row in snapshot.inspectors.objects} == {"same"}
    assert len({row.observation_key for row in snapshot.inspectors.objects}) == 2
    changes = snapshot.inspectors.object_observations.changes
    first = next(row for row in changes if row.sequence == 2 and row.perspective is not None)
    assert not first.previous_known
    rename = next(row for row in changes if row.sequence == 3 and row.perspective is not None)
    assert rename.previous_known and rename.previous_value == 1
    assert not next(row for row in changes if row.sequence == 4 and row.federate is None).previous_known
    scopes = [row for row in snapshot.inspectors.object_observations.snapshots if row.perspective is not None]
    assert len(scopes) == 3 and any(row.federate == "Renamed" for row in scopes)


def test_legacy_unknown_reporter_does_not_supply_another_reporters_discovery_or_comparison_baseline():
    store = ObserverStore()
    store.publish({"event_type": "object.discovered", "federate": "Twin", "object_key": "one", "object_name": "Unknown reporter metadata", "attributes": {"x": 1}})
    store.publish({"event_type": "object.updated", "source": "known", "federate": "Twin", "object_key": "one", "attributes": {"x": 2}})
    data = StateResponse.model_validate(store.state()).inspectors.object_observations
    known = next(row for row in data.snapshots if row.perspective and row.perspective.source == "known")
    assert known.object.object_name == "one" and known.object.attributes == {"x": 2}
    assert not next(row for row in data.changes if row.sequence == 2 and row.perspective).previous_known


def test_time_requests_use_exact_joins_and_keep_original_request_metadata_through_alias_changes():
    store = ObserverStore()
    def observe(identity, kind, **values):
        store.publish(event(identity=identity, event_type="time.observed", time_observation={"kind": kind, "request_id": "same-id", **values}))
    observe(attribution(), "request", logical_time=10)
    observe(attribution(namespace="other"), "grant", logical_time=99)
    observe(attribution(name="Renamed"), "grant", logical_time=10)
    observe(attribution(membership="fresh"), "request", logical_time=20)
    data = StateResponse.model_validate(store.state()).inspectors.time
    assert len(data.snapshots) == 3 and len(data.requests) == 2
    original, fresh = data.requests
    assert original.status == "granted" and original.granted_time == 10 and original.latency_seconds is not None
    assert original.scope.perspective.attribution.participant.display_name == "Twin"
    assert fresh.status == "pending"
    assert any(row.scope.federate == "Renamed" for row in data.snapshots)
    store.publish(event(event_type="federate.resigned", identity=attribution(membership="fresh")))
    assert StateResponse.model_validate(store.state()).inspectors.time.requests[1].status == "session-ended"


def test_reporting_end_needs_the_correct_explicit_domain_not_only_same_source_and_session_labels():
    store = ObserverStore()
    for namespace in ("a", "b"):
        store.publish(event(identity=attribution(namespace=namespace), event_type="time.observed", time_observation={"kind": "request", "request_id": "same"}))
    store.publish({"event_type": "session.disconnected", "source": "producer", "session_id": "session"})
    assert all(row.status == "pending" for row in StateResponse.model_validate(store.state()).inspectors.time.requests)
    store.publish(event(identity=attribution(namespace="a"), event_type="session.disconnected"))
    requests = StateResponse.model_validate(store.state()).inspectors.time.requests
    assert {row.scope.perspective.attribution.identity_namespace: row.status for row in requests} == {"a": "session-ended", "b": "pending"}


def test_session_and_namespace_ids_preserve_whitespace_as_opaque_text_not_display_labels():
    left = event_perspective(event(identity=attribution(session=" session ", namespace=" ns ")))
    right = event_perspective(event())
    assert left != right
    legacy_left = event_perspective({"source": "s", "session_id": " session ", "federate": "Twin"})
    legacy_right = event_perspective({"source": "s", "session_id": "session", "federate": "Twin"})
    assert legacy_left != legacy_right
