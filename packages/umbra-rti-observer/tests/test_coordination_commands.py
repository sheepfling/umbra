"""Internal observer control evidence, not normative RTI service conformance."""
from __future__ import annotations

import time
from typing import get_args

import pytest
from pydantic import TypeAdapter, ValidationError
from umbra_rti_observer.command_models import (
    COORDINATION_ACTIONS,
    CommandAction,
    CommandOutcome,
    CommandRequest,
    CoordinationCommandReceipt,
    CoordinationCommandRequest,
    JoinParameters,
    ResignParameters,
)
from umbra_rti_observer.commands import CommandDispatcher, CommandUnavailable
from umbra_rti_observer.core import ObserverStore
from umbra_rti_observer.schemas import StateResponse
from umbra_rti_observer.session_commands import SessionController
from umbra_rti_observer.showcase_commands import ShowcaseSessionDriver
from umbra_rti_observer.showcase_scenarios import (
    coordination_scenario,
    topology_scenario,
)

VERSION = {"instance_id": "owned", "revision": 0}


def command(action: str, parameters: dict[str, object] | None = None, request_id: str = "operation") -> CoordinationCommandRequest:
    return TypeAdapter(CoordinationCommandRequest).validate_python({"action": action,
        "target_id": "owned", "request_id": request_id, "confirmed": True,
        "expected_version": VERSION, "parameters": parameters or {}})


@pytest.mark.parametrize("changes", [
    {"expected_version": None}, {"parameters": {"label": True}},
    {"parameters": {"label": " "}}, {"parameters": {"label": "x", "provider_path": "/tmp"}},
])
def test_boundary_rejects_missing_version_and_opaque_parameters(changes: dict[str, object]) -> None:
    with pytest.raises(ValidationError):
        TypeAdapter(CommandRequest).validate_python({"action": "save.request", "target_id": "owned",
            "request_id": "operation", "expected_version": VERSION, "parameters": {"label": "x"}, **changes})


def test_optional_controls_are_explicit_and_unsupported_never_calls_driver() -> None:
    controller = SessionController(target_id="owned", label="Owned", adapter="test",
        driver_factory=lambda: pytest.fail("unsupported command constructed a driver"), federations=("F",))
    assert not any(item.action in COORDINATION_ACTIONS for item in controller.describe().commands)
    with pytest.raises(CommandUnavailable, match="does not offer"):
        controller.execute(command("save.request", {"label": "x"}))


def test_demo_sequence_preserves_local_vs_collective_and_exact_checkpoint_labels() -> None:
    clock = [1.0]
    store = ObserverStore()
    driver = ShowcaseSessionDriver(sink=store.publish, clock=lambda: clock[0])
    driver.connect()
    driver.join(JoinParameters(federation="F", federate_name="Owned"))
    def flush() -> None:
        clock[0] += 1
        driver.poll()
    def phases(kind: str, scope: str) -> list[str]:
        data = StateResponse.model_validate(store.state()).inspectors.coordination
        assert data is not None
        return [row.phase for row in data.transitions if row.kind == kind and row.scope == scope]

    receipt = driver.coordinate(command("sync.register", {"label": " barrier ", "tag_text": "opaque tag"}))
    assert receipt.coordination is not None and receipt.coordination.label == " barrier "
    assert phases("synchronization", "federation") == []
    with pytest.raises(CommandUnavailable, match="already pending"):
        driver.coordinate(command("sync.register", {"label": " barrier "}))
    with pytest.raises(CommandUnavailable, match="no announced"):
        driver.coordinate(command("sync.achieve", {"label": " barrier "}))
    flush()
    driver.coordinate(command("sync.achieve", {"label": " barrier "}))
    assert phases("synchronization", "local")[-1] == "achieved"
    assert phases("synchronization", "federation") == ["announced"]
    flush()
    assert phases("synchronization", "federation")[-1] == "completed"

    with pytest.raises(CommandUnavailable, match="no begun"):
        driver.coordinate(command("save.complete"))
    driver.coordinate(command("save.request", {"label": " Checkpoint "}, "save-1"))
    with pytest.raises(CommandUnavailable, match="Wait for"):
        driver.coordinate(command("save.begun"))
    flush()
    driver.coordinate(command("save.begun"))
    driver.coordinate(command("save.complete", {"result": "completed"}))
    assert phases("save", "local")[-1] == "completed"
    assert phases("save", "federation") == ["requested"]
    flush()
    with pytest.raises(CommandUnavailable, match="exact label"):
        driver.coordinate(command("restore.request", {"label": "Checkpoint"}))
    driver.coordinate(command("restore.request", {"label": " Checkpoint "}, "restore-1"))
    with pytest.raises(CommandUnavailable, match="no begun"):
        driver.coordinate(command("restore.complete"))
    flush()
    driver.coordinate(command("restore.complete", {"result": "failed"}))
    assert phases("restore", "local")[-1] == "failed"
    assert phases("restore", "federation") == ["begun"]
    flush()
    assert phases("restore", "federation")[-1] == "failed"


def test_dispatcher_guards_confirmation_version_and_dedup_before_owned_service() -> None:
    dispatcher = CommandDispatcher()
    controller = SessionController(target_id="owned", label="Owned", adapter="test",
        driver_factory=ShowcaseSessionDriver, federations=("F",), coordination_actions=COORDINATION_ACTIONS)
    dispatcher.register(controller)
    def submit(action: str, parameters: dict[str, object], *, confirmed: bool = True, request_id: str | None = None):
        request = TypeAdapter(CommandRequest).validate_python({"action": action, "target_id": "owned",
            "request_id": request_id or f"request-{len(dispatcher.history().records)}", "confirmed": confirmed,
            "expected_version": dispatcher.catalog().targets[0].version, "parameters": parameters})
        record = dispatcher.submit(request)
        deadline = time.monotonic() + 2
        while record.status in {"accepted", "running"} and time.monotonic() < deadline:
            time.sleep(0.005)
            record = dispatcher.get(record.command_id)
        assert record.status not in {"accepted", "running"}
        return record
    try:
        assert submit("save.request", {"label": "x"}).status == "unavailable"
        assert submit("session.connect", {}).status == "succeeded"
        assert submit("federate.join", {"federation": "F", "federate_name": "Owned"}).status == "succeeded"
        assert submit("save.request", {"label": "x"}, confirmed=False).status == "unavailable"
        record = submit("save.request", {"label": "x"}, request_id="save-once")
        assert record.status == "succeeded"
        assert record.outcome is not None and record.outcome.coordination is not None
        assert dispatcher.submit(record.request).command_id == record.command_id
        assert submit("save.request", {"label": "other"}).status == "unavailable"
        assert dispatcher.catalog().targets[0].state == "joined"
        stale = record.request.model_copy(update={"request_id": "stale"})
        assert dispatcher.submit(stale).status == "unavailable"
    finally:
        dispatcher.close()


def test_demo_rejoin_has_fresh_identity_and_checkpoints_do_not_cross_federations() -> None:
    clock = [1.0]
    store = ObserverStore()
    driver = ShowcaseSessionDriver(sink=store.publish, clock=lambda: clock[0])
    driver.connect()
    driver.join(JoinParameters(federation="First", federate_name="Same"))
    driver.coordinate(command("save.request", {"label": "shared"}, "first-save"))
    clock[0] += 1
    driver.poll()
    driver.coordinate(command("save.begun"))
    driver.coordinate(command("save.complete"))
    clock[0] += 1
    driver.poll()
    driver.resign(ResignParameters())
    driver.join(JoinParameters(federation="Second", federate_name="Same"))
    with pytest.raises(CommandUnavailable, match="exact label"):
        driver.coordinate(command("restore.request", {"label": "shared"}))
    driver.coordinate(command("sync.register", {"label": "new join"}, "second-sync"))
    data = StateResponse.model_validate(store.state()).inspectors.coordination
    assert data is not None
    subjects = [row.subject for row in data.transitions if row.subject is not None]
    assert subjects[0].participant_id == subjects[-1].participant_id
    assert subjects[0].membership_id != subjects[-1].membership_id


def test_demo_scenarios_cover_graph_shapes_and_coordination_modalities_with_valid_models() -> None:
    store = ObserverStore()
    relationships = topology_scenario("demo")
    store.publish_many([{"event_type": "relationship.observed", "relationship": row} for row in relationships])
    for tick in range(80):
        store.publish_many([{"event_type": "coordination.observed", "coordination": row} for row in coordination_scenario("demo", tick)])
    state = StateResponse.model_validate(store.state())
    assert len(relationships) == 13
    assert any(row["producer"] == row["recipient"] for row in relationships)
    coordination = state.inspectors.coordination
    assert coordination is not None
    for kind in ("save", "restore"):
        phases = {row.phase for row in coordination.transitions if row.kind == kind and row.scope == "federation"}
        assert {"requested", "completed", "failed"} <= phases
    assert any(row.phase == "registration-failed" for row in coordination.transitions)
    assert any(participant.status == "unknown" for row in coordination.transitions for participant in row.participants or ())
    assert set(COORDINATION_ACTIONS) <= set(get_args(CommandAction))


def test_named_receipt_cannot_claim_a_different_operation_kind() -> None:
    from umbra_rti_observer.command_models import validate_command_outcome
    opaque = "  provider / opération α  "
    receipt = CoordinationCommandReceipt(kind="save", label="x", operation_id=opaque)
    validate_command_outcome(command("save.request", {"label": "x"}), CommandOutcome(summary="returned", coordination=receipt))
    assert receipt.operation_id == opaque
    with pytest.raises(ValueError, match="matching named"):
        validate_command_outcome(command("save.request", {"label": "x"}), CommandOutcome(summary="returned",
            coordination=CoordinationCommandReceipt(kind="restore", label="x")))
