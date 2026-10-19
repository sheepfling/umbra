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
