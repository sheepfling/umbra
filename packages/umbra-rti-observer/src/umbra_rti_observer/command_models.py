# pyright: strict
"""Language-neutral models for optional adapter-controlled session commands."""

from __future__ import annotations

from collections.abc import Sequence
from typing import Annotated, Literal, TypeAlias

from pydantic import Field

from .schemas import CapabilityState, JsonObject, ObserverModel

CommandAction: TypeAlias = Literal[
    "session.connect", "federate.join", "federate.resign", "session.disconnect"
]
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
    str, Field(min_length=1, max_length=128, pattern=r"^[A-Za-z0-9_.:-]+$")
]
SessionText = Annotated[str, Field(min_length=1, max_length=256)]


class EmptyParameters(ObserverModel):
    """Connection configuration and credentials belong to the server."""


class JoinParameters(ObserverModel):
    federation: SessionText
    federate_name: SessionText
    federate_type: SessionText = "Observer"


class ResignParameters(ObserverModel):
    policy: ResignPolicy = "no_action"


class CommandBase(ObserverModel):
    request_id: Identifier
    target_id: Identifier
    confirmed: bool = False


class ConnectCommand(CommandBase):
    action: Literal["session.connect"]
    parameters: EmptyParameters


class JoinCommand(CommandBase):
    action: Literal["federate.join"]
    parameters: JoinParameters


class ResignCommand(CommandBase):
    action: Literal["federate.resign"]
    parameters: ResignParameters


class DisconnectCommand(CommandBase):
    action: Literal["session.disconnect"]
    parameters: EmptyParameters


CommandRequest: TypeAlias = Annotated[
    ConnectCommand | JoinCommand | ResignCommand | DisconnectCommand,
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
    completion: Literal["service-return"] = "service-return"
    parameter_model: str
    parameter_schema: JsonObject
    fields: Sequence[CommandField] = ()


class CommandTarget(ObserverModel):
    target_id: Identifier
    label: str
    adapter: str
    state: Literal["disconnected", "connected", "joined", "unknown"]
    federation: str | None = None
    federate: str | None = None
    synthetic: bool = False
    busy: bool = False
    fault: str | None = None
    commands: Sequence[CommandDefinition]


class CommandError(ObserverModel):
    code: str
    message: str
    effect: Literal["none", "unknown"]


class CommandOutcome(ObserverModel):
    summary: str
    federate_handle: str | None = None


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


class CommandCatalog(ObserverModel):
    contract_version: str
    enabled: bool
    timeout_seconds: float
    history_limit: int
    targets: Sequence[CommandTarget]


class CommandHistory(ObserverModel):
    contract_version: str
    records: Sequence[CommandRecord]
    limit: int
    evicted_records: int
    persistence: Literal["server-process"] = "server-process"
