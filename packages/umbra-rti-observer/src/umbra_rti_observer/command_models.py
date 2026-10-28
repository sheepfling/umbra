# pyright: strict
"""Language-neutral models for optional adapter-controlled session commands."""

from __future__ import annotations

from collections.abc import Sequence
from typing import Annotated, Literal, TypeAlias

from pydantic import Field, model_validator

from .schemas import CapabilityState, DeclarationId, JsonObject, JsonText, ObserverModel

CommandAction: TypeAlias = Literal[
    "session.connect",
    "federate.join",
    "federate.resign",
    "session.disconnect",
    "session.inspect",
    "session.recover",
    "sync.register",
    "sync.achieve",
    "save.request",
    "save.begun",
    "save.complete",
    "restore.request",
    "restore.complete",
]
CoordinationAction: TypeAlias = Literal[
    "sync.register",
    "sync.achieve",
    "save.request",
    "save.begun",
    "save.complete",
    "restore.request",
    "restore.complete",
]
COORDINATION_ACTIONS: tuple[CoordinationAction, ...] = (
    "sync.register",
    "sync.achieve",
    "save.request",
    "save.begun",
    "save.complete",
    "restore.request",
    "restore.complete",
)
CommandStatus: TypeAlias = Literal[
    "accepted", "running", "succeeded", "failed", "unavailable"
]
ResignPolicy: TypeAlias = Literal[
    "no_action",
    "unconditionally_divest_attributes",
    "delete_objects",
    "cancel_pending_ownership_acquisitions",
    "delete_objects_then_divest",
    "cancel_then_delete_then_divest",
]
Identifier = Annotated[
    JsonText, Field(min_length=1, max_length=128, pattern=r"^[A-Za-z0-9_.:-]+$")
]
SessionText = Annotated[str, Field(min_length=1, max_length=256)]
ReportedSessionName = Annotated[
    JsonText, Field(min_length=1, max_length=256, pattern=r"\S")
]


class EmptyParameters(ObserverModel):
    """Connection configuration and credentials belong to the server."""


class JoinParameters(ObserverModel):
    federation: SessionText
    federate_name: SessionText
    federate_type: SessionText = "Observer"


class ResignParameters(ObserverModel):
    policy: ResignPolicy = "no_action"


class CoordinationLabelParameters(ObserverModel):
    label: ReportedSessionName


class SynchronizationRegisterParameters(CoordinationLabelParameters):
    tag_text: Annotated[JsonText, Field(max_length=256)] = ""


class SynchronizationAchieveParameters(CoordinationLabelParameters):
    result: Literal["achieved", "failed"] = "achieved"


class CoordinationCompletionParameters(ObserverModel):
    result: Literal["completed", "failed"] = "completed"


class CommandTargetVersion(ObserverModel):
    """Owned dispatcher registration and revision, never observation identity."""

    instance_id: Identifier
    revision: Annotated[int, Field(ge=0)]


class SessionStateObservation(ObserverModel):
    """Explicit provider facts about this driver's own connection/membership."""

    connection: Literal["connected", "disconnected", "unknown"]
    membership: Literal["joined", "not-joined", "unknown"]
    federation: ReportedSessionName | None = None
    federate: ReportedSessionName | None = None
    federate_handle: Annotated[JsonText, Field(min_length=1, max_length=256)] | None = (
        None
    )
    reason: Annotated[str, Field(max_length=2048)] | None = None

    @model_validator(mode="after")
    def consistent_facts(self) -> SessionStateObservation:
        if self.connection == "disconnected" and self.membership == "joined":
            raise ValueError("a disconnected driver cannot report joined membership")
        if self.membership == "not-joined" and any(
            value is not None
            for value in (self.federation, self.federate, self.federate_handle)
        ):
            raise ValueError(
                "not-joined membership cannot retain stale member metadata"
            )
        return self


class SessionInspectionResult(ObserverModel):
    basis: Literal["owned-driver-query"] = "owned-driver-query"
    checked_at: Annotated[float, Field(ge=0, allow_inf_nan=False)]
    observation: SessionStateObservation
    reconciled: bool
    resolution: Annotated[str, Field(min_length=1, max_length=2048)]

    @model_validator(mode="after")
    def complete_reconciliation(self) -> SessionInspectionResult:
        observation = self.observation
        if self.reconciled and not (
            observation.connection in {"connected", "disconnected"}
            and observation.membership == "not-joined"
            or observation.connection == "connected"
            and observation.membership == "joined"
            and observation.federation is not None
        ):
            raise ValueError(
                "incomplete provider facts cannot claim reconciled authority"
            )
        return self


class CommandBase(ObserverModel):
    request_id: Identifier
    target_id: Identifier
    confirmed: bool = False


class LifecycleCommandBase(CommandBase):
    expected_version: CommandTargetVersion | None = None


class InspectionCommandBase(CommandBase):
    expected_version: CommandTargetVersion


class ConnectCommand(LifecycleCommandBase):
    action: Literal["session.connect"]
    parameters: EmptyParameters


class JoinCommand(LifecycleCommandBase):
    action: Literal["federate.join"]
    parameters: JoinParameters


class ResignCommand(LifecycleCommandBase):
    action: Literal["federate.resign"]
    parameters: ResignParameters


class DisconnectCommand(LifecycleCommandBase):
    action: Literal["session.disconnect"]
    parameters: EmptyParameters


class InspectSessionCommand(InspectionCommandBase):
    action: Literal["session.inspect"]
    parameters: EmptyParameters


class RecoverSessionCommand(InspectionCommandBase):
    action: Literal["session.recover"]
    parameters: EmptyParameters


class RegisterSynchronizationCommand(InspectionCommandBase):
    action: Literal["sync.register"]
    parameters: SynchronizationRegisterParameters


class AchieveSynchronizationCommand(InspectionCommandBase):
    action: Literal["sync.achieve"]
    parameters: SynchronizationAchieveParameters


class RequestSaveCommand(InspectionCommandBase):
    action: Literal["save.request"]
    parameters: CoordinationLabelParameters


class SaveBegunCommand(InspectionCommandBase):
    action: Literal["save.begun"]
    parameters: EmptyParameters


class CompleteSaveCommand(InspectionCommandBase):
    action: Literal["save.complete"]
    parameters: CoordinationCompletionParameters


class RequestRestoreCommand(InspectionCommandBase):
    action: Literal["restore.request"]
    parameters: CoordinationLabelParameters


class CompleteRestoreCommand(InspectionCommandBase):
    action: Literal["restore.complete"]
    parameters: CoordinationCompletionParameters


CoordinationCommandRequest: TypeAlias = (
    RegisterSynchronizationCommand
    | AchieveSynchronizationCommand
    | RequestSaveCommand
    | SaveBegunCommand
    | CompleteSaveCommand
    | RequestRestoreCommand
    | CompleteRestoreCommand
)
COORDINATION_REQUEST_TYPES = (
    RegisterSynchronizationCommand,
    AchieveSynchronizationCommand,
    RequestSaveCommand,
    SaveBegunCommand,
    CompleteSaveCommand,
    RequestRestoreCommand,
    CompleteRestoreCommand,
)


CommandRequest: TypeAlias = Annotated[
    ConnectCommand
    | JoinCommand
    | ResignCommand
    | DisconnectCommand
    | InspectSessionCommand
    | RecoverSessionCommand
    | CoordinationCommandRequest,
    Field(discriminator="action"),
]


class CommandField(ObserverModel):
    name: str
    label: str
    input: Literal["text", "select"]
    required: bool
    choices: Sequence[str] = ()
    default_value: str = ""


class CommandDefinition(ObserverModel):
    action: CommandAction
    title: str
    description: str
    state: CapabilityState
    reason: str | None = None
    scope: Literal["controlled-session"] = "controlled-session"
    requires_confirmation: bool = False
    completion: Literal["service-return", "inspection-result"] = "service-return"
    parameter_model: str
    parameter_schema: JsonObject
    fields: Sequence[CommandField] = ()

    @model_validator(mode="after")
    def inspection_contract(self) -> CommandDefinition:
        if self.action in COORDINATION_ACTIONS and (
            not self.requires_confirmation or self.completion != "service-return"
        ):
            raise ValueError(
                "coordination requires confirmation and local service-return completion"
            )
        if self.action in {"session.inspect", "session.recover"} and (
            self.parameter_model != "EmptyParameters"
            or self.fields
            or self.completion != "inspection-result"
            or self.requires_confirmation != (self.action == "session.recover")
        ):
            raise ValueError(
                "inspection/recovery uses empty parameters, named inspection completion and explicit recovery confirmation"
            )
        return self


class CommandTarget(ObserverModel):
    version: CommandTargetVersion | None = None
    inspection: SessionInspectionResult | None = None
    target_id: Identifier
    label: str
    adapter: str
    state: Literal["disconnected", "connected", "joined", "unknown"]
    federation: JsonText | None = None
    federate: JsonText | None = None
    synthetic: bool = False
    busy: bool = False
    fault: str | None = None
    commands: Sequence[CommandDefinition]


class CommandError(ObserverModel):
    code: str
    message: str
    effect: Literal["none", "unknown"]


class CoordinationCommandReceipt(ObserverModel):
    """Local service return, never a collective synchronization/save result."""

    kind: Literal["synchronization", "save", "restore"]
    label: ReportedSessionName
    operation_id: DeclarationId | None = None
    basis: Literal["service-return"] = "service-return"


class CommandOutcome(ObserverModel):
    summary: str
    federate_handle: str | None = None
    inspection: SessionInspectionResult | None = None
    coordination: CoordinationCommandReceipt | None = None


class CommandRecord(ObserverModel):
    command_id: str
    request: CommandRequest
    status: CommandStatus
    accepted_at: float
    started_at: float | None = None
    finished_at: float | None = None
    outcome: CommandOutcome | None = None
    error: CommandError | None = None
    late_outcome: CommandOutcome | None = None
    late_error: CommandError | None = None
    late_finished_at: float | None = None

    @model_validator(mode="after")
    def inspection_outcomes(self) -> CommandRecord:
        if (
            self.status == "succeeded"
            and self.request.action
            in {"session.inspect", "session.recover", *COORDINATION_ACTIONS}
            and self.outcome is None
        ):
            raise ValueError(
                "successful versioned inspection/coordination must retain its named outcome"
            )
        for outcome in (self.outcome, self.late_outcome):
            if outcome is not None:
                validate_command_outcome(self.request, outcome)
        return self


def validate_command_outcome(request: CommandRequest, outcome: CommandOutcome) -> None:
    if request.action in COORDINATION_ACTIONS:
        expected = (
            "synchronization"
            if request.action.startswith("sync.")
            else request.action.partition(".")[0]
        )
        if outcome.coordination is None or outcome.coordination.kind != expected:
            raise ValueError(
                "coordination commands require a matching named service-return receipt"
            )
        if (
            isinstance(
                request,
                (
                    RegisterSynchronizationCommand,
                    AchieveSynchronizationCommand,
                    RequestSaveCommand,
                    RequestRestoreCommand,
                ),
            )
            and outcome.coordination.label != request.parameters.label
        ):
            raise ValueError(
                "coordination receipt must preserve the exact requested label"
            )
    elif outcome.coordination is not None:
        raise ValueError("only coordination commands return coordination receipts")
    if request.action in {"session.inspect", "session.recover"}:
        if outcome.inspection is None:
            raise ValueError(
                "inspection/recovery returns its named inspection evidence"
            )
        if request.action == "session.inspect" and outcome.inspection.reconciled:
            raise ValueError("inspection-only commands cannot restore authority")


class CommandCatalog(ObserverModel):
    contract_version: str
    enabled: bool
    timeout_seconds: float
    history_limit: int
    targets: Sequence[CommandTarget]

    @model_validator(mode="after")
    def unique_versioned_targets(self) -> CommandCatalog:
        if len({target.target_id for target in self.targets}) != len(self.targets):
            raise ValueError("owned command targets must be unique")
        if any(
            target.version is None
            and any(
                definition.action
                in {"session.inspect", "session.recover", *COORDINATION_ACTIONS}
                and definition.state != "unsupported"
                for definition in target.commands
            )
            for target in self.targets
        ):
            raise ValueError(
                "offered inspection/recovery or coordination requires an owned target version"
            )
        return self


class CommandHistory(ObserverModel):
    contract_version: str
    records: Sequence[CommandRecord]
    limit: int
    evicted_records: int
    persistence: Literal["server-process"] = "server-process"
