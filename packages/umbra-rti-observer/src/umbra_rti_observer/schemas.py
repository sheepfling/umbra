"""Pydantic schemas for the optional observer HTTP transport.

The core store intentionally has no web-framework dependency.  This module is
the opposite boundary: every request body and every finite JSON response that
FastAPI exposes has a named, inspectable model.  Provider-specific fields are
allowed only where the wire contract explicitly calls them extensions.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from typing import Annotated, ClassVar, Literal, cast

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    RootModel,
    SerializerFunctionWrapHandler,
    TypeAdapter,
    model_serializer,
    model_validator,
)
from typing_extensions import TypeAliasType

JsonValue = TypeAliasType(
    "JsonValue",
    None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"],
)
JsonObject = dict[str, JsonValue]
# Pyright cannot express the recursive TypeForm in the TypeAdapter constructor.
_JSON_VALUE_ADAPTER = TypeAdapter(JsonValue)  # pyright: ignore[reportArgumentType]
CapabilityState = Literal[
    "available",
    "partial",
    "disabled",
    "unsupported",
    "unknown",
]
NonEmptyText = Annotated[str, Field(min_length=1)]
EventTypeText = Annotated[str, Field(min_length=1, max_length=256)]
SequenceNumber = Annotated[int, Field(ge=1)]
NonNegativeInt = Annotated[int, Field(ge=0)]
PositiveInt = Annotated[int, Field(ge=1)]
NonNegativeFloat = Annotated[float, Field(ge=0)]


class ObserverModel(BaseModel):
    """Strict, immutable Pydantic boundary model."""

    model_config = ConfigDict(
        extra="forbid",
        frozen=True,
        populate_by_name=True,
        strict=True,
        str_strip_whitespace=True,
    )


class WireObserverModel(ObserverModel):
    """Response model that preserves unknown wire fields at the top level."""

    model_config = ConfigDict(extra="allow")


class ExtensibleObserverModel(ObserverModel):
    """Strict model with one explicit provider-extension escape hatch."""

    _reject_batch_shape: ClassVar[bool] = False
    extensions: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]

    @model_validator(mode="before")
    @classmethod
    def collect_extensions(cls, value: object) -> object:
        """Collect unknown wire fields into the named extension mapping."""

        if not isinstance(value, Mapping):
            return value
        if cls._reject_batch_shape and "events" in value:
            raise ValueError("an object containing 'events' must be an event batch")
        fields = set(cls.model_fields) - {"extensions"}
        raw_extensions = value.get("extensions", {})
        if not isinstance(raw_extensions, Mapping):
            raise TypeError("extensions must be a JSON object")
        extensions = {str(key): _JSON_VALUE_ADAPTER.validate_python(item) for key, item in raw_extensions.items()}
        fields_in_extensions = {
            str(key): _JSON_VALUE_ADAPTER.validate_python(item)
            for key, item in value.items()
            if key not in fields and key != "extensions"
        }
        extensions.update(fields_in_extensions)
        return {
            **{key: item for key, item in value.items() if key in fields},
            "extensions": extensions,
        }

    @model_serializer(mode="wrap")
    def flatten_extensions(self, handler: SerializerFunctionWrapHandler) -> object:
        """Keep the existing top-level wire shape while storing extensions explicitly."""

        serialized = cast(dict[str, object], handler(self))
        extensions = serialized.pop("extensions", {})
        if not isinstance(extensions, Mapping):  # pragma: no cover - validated above
            raise TypeError("extensions must serialize as a JSON object")
        return {**dict(extensions), **serialized}


class EventInput(ExtensibleObserverModel):
    """Normalized event input plus JSON-compatible provider extensions."""

    sequence: Annotated[int | None, Field(default=None, ge=1)]
    event_type: Annotated[EventTypeText | None, Field(default=None)]
    kind: Annotated[EventTypeText | None, Field(default=None)]
    family: Annotated[NonEmptyText | None, Field(default=None)]
    source: Annotated[str | None, Field(default=None, min_length=1, max_length=512)]
    observed_at: float | None = None
    connection: str | None = None
    federation: str | None = None
    federate: str | dict[str, JsonValue] | None = None
    participant: str | dict[str, JsonValue] | None = None
    federate_name: str | None = None
    participant_name: str | None = None
    federate_handle: str | None = None
    participant_handle: str | None = None
    role: str | None = None
    object_key: str | None = None
    object_name: str | None = None
    object_handle: str | None = None
    class_name: str | None = None
    class_handle: str | None = None
    attributes: Annotated[Mapping[str, JsonValue] | None, Field(default=None)]
    interaction_key: str | None = None
    interaction_class: str | None = None
    interaction_handle: str | None = None
    parameters: Annotated[Mapping[str, JsonValue] | None, Field(default=None)]
    tag: Annotated[JsonValue, Field(default=None)]
    details: Annotated[Mapping[str, JsonValue] | None, Field(default=None)]
    logical_time: int | float | None = None
    operation: str | None = None

    _reject_batch_shape: ClassVar[bool] = True


class EventBatchRequest(BaseModel):
    """Atomic event batch request.

    Extra keys are forbidden so an event object containing an ``events`` field
    cannot be silently interpreted as a batch.  Single events remain
    extensible through :class:`EventInput`.
    """

    model_config = ConfigDict(extra="forbid", frozen=True, strict=True)

    events: Annotated[Sequence[EventInput], Field(min_length=1, max_length=10_000)]


EventIngestRequest = Annotated[
    EventBatchRequest | EventInput,
    Field(description="One event object or an atomic events batch."),
]


class ContextRequest(RootModel[dict[str, JsonValue]]):
    """JSON object merged into the observer context."""

    model_config = ConfigDict(frozen=True, strict=True)


class StatusRequest(ExtensibleObserverModel):
    status: Annotated[str, Field(min_length=1, max_length=128)]
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]


class CapabilityResponse(WireObserverModel):
    state: CapabilityState
    label: NonEmptyText
    description: NonEmptyText
    owner: NonEmptyText
    source: str | None = None
    owners: Annotated[Sequence[str], Field(default_factory=tuple)]
    sources: Annotated[Sequence[str], Field(default_factory=tuple)]
    observations: Annotated[Sequence[Mapping[str, JsonValue]], Field(default_factory=tuple)]
    reason: str | None = None
    reasons: Annotated[Sequence[str], Field(default_factory=tuple)]
    event_types: Annotated[Sequence[str], Field(default_factory=tuple)]


class ObjectInspectionResponse(ObserverModel):
    object_key: NonEmptyText
    object_name: NonEmptyText
    object_handle: str | None = None
    class_name: str | None = None
    class_handle: str | None = None
    family: NonEmptyText
    attributes: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]
    update_count: NonNegativeInt
    discovery_count: NonNegativeInt
    sources: Annotated[Sequence[str], Field(default_factory=tuple)]
    status: NonEmptyText
    last_seen: float | None = None


class InteractionInspectionResponse(ObserverModel):
    sequence: SequenceNumber
    interaction_key: NonEmptyText
    interaction_class: NonEmptyText
    class_handle: str | None = None
    family: NonEmptyText
    source: NonEmptyText
    federate: str | None = None
    parameters: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]
    tag: Annotated[JsonValue, Field(default=None)]
    observed_at: float


class FederateInspectionResponse(ObserverModel):
    federate: NonEmptyText
    role: str | None = None
    handle: str | None = None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None = None


class LiveMetricsResponse(ObserverModel):
    event_count: NonNegativeInt
    event_rate_per_second: NonNegativeFloat
    error_count: NonNegativeInt
    event_types: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    families: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    sources: Annotated[Mapping[str, NonNegativeInt], Field(default_factory=dict)]
    first_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    object_count: Annotated[NonNegativeInt, Field(default=0)]
    interaction_count: Annotated[NonNegativeInt, Field(default=0)]
    federate_count: Annotated[NonNegativeInt, Field(default=0)]


class RetentionResponse(ObserverModel):
    max_events: PositiveInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt
    first_retained_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    next_sequence: PositiveInt


class RetentionGapResponse(ObserverModel):
    first_retained_sequence: PositiveInt
    last_sequence: NonNegativeInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt


class AdapterResponse(ObserverModel):
    name: NonEmptyText
    status: NonEmptyText
    capabilities: Annotated[Sequence[str], Field(default_factory=tuple)]


class EventEnvelope(WireObserverModel):
    sequence: SequenceNumber
    event_type: NonEmptyText
    family: NonEmptyText
    source: NonEmptyText
    observed_at: float


class InspectorsResponse(ObserverModel):
    objects: Annotated[Sequence[ObjectInspectionResponse], Field(default_factory=tuple)]
    interactions: Annotated[Sequence[InteractionInspectionResponse], Field(default_factory=tuple)]


class StateResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    federate_roster: Annotated[Sequence[FederateInspectionResponse], Field(default_factory=tuple)]
    inspectors: InspectorsResponse
    normalized_events: Annotated[Sequence[EventEnvelope] | None, Field(default=None)]


class HealthResponse(ObserverModel):
    service: NonEmptyText
    status: NonEmptyText
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    event_count: NonNegativeInt
    retention: RetentionResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]


class MetricsResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    status: NonEmptyText
    live_metrics: LiveMetricsResponse
    retention: RetentionResponse
    adapters: Annotated[Sequence[AdapterResponse], Field(default_factory=tuple)]


class EventPageResponse(ObserverModel):
    contract_version: NonEmptyText
    events: Annotated[Sequence[EventEnvelope], Field(default_factory=tuple)]
    next_after: NonNegativeInt
    has_more: bool
    gap: bool
    first_retained_sequence: NonNegativeInt
    last_sequence: NonNegativeInt
    retained_events: NonNegativeInt
    dropped_events: NonNegativeInt
    cleared_events: NonNegativeInt


class IngestResponse(ObserverModel):
    accepted: NonNegativeInt
    state: StateResponse


class ContextResponse(ObserverModel):
    context: Annotated[Mapping[str, JsonValue], Field(default_factory=dict)]


class InspectorResponse(ObserverModel):
    contract_version: NonEmptyText
    kind: Literal["objects", "interactions", "federates"]
    items: Annotated[
        Sequence[
            ObjectInspectionResponse
            | InteractionInspectionResponse
            | FederateInspectionResponse
        ],
        Field(default_factory=tuple),
    ]


class CapabilitySchemaResponse(ObserverModel):
    states: Sequence[CapabilityState]
    fields: Sequence[str]
    known: Sequence[str]
    state_meanings: Mapping[CapabilityState, str]


class ResourceDescription(ObserverModel):
    method: Literal["GET", "POST", "PUT", "DELETE"]
    path: NonEmptyText
    query: Annotated[Mapping[str, str], Field(default_factory=dict)]
    required_fields: Annotated[Sequence[str], Field(default_factory=tuple)]


class EventEnvelopeDescription(ObserverModel):
    required_fields: Sequence[str]
    extension_rule: NonEmptyText


class StreamDescription(ObserverModel):
    websocket_path: NonEmptyText
    sse_path: NonEmptyText
    message_types: Sequence[Literal["snapshot", "events", "heartbeat", "gap", "error"]]
    messages: Mapping[str, str]


class ContractResponse(ObserverModel):
    contract_version: NonEmptyText
    media_type: NonEmptyText
    event_schema_version: NonEmptyText
    api_paths: Mapping[str, str]
    capability_schema: CapabilitySchemaResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    snapshot: ResourceDescription
    events: ResourceDescription
    event_envelope: EventEnvelopeDescription
    stream: StreamDescription
    event_types: Sequence[str]
    backend_rule: NonEmptyText


class CatalogResponse(ObserverModel):
    contract_version: NonEmptyText
    schema_version: NonEmptyText
    event_schema: Mapping[str, JsonValue]
    contract: ContractResponse
    capabilities: Annotated[Mapping[str, CapabilityResponse], Field(default_factory=dict)]
    features: Annotated[Sequence[str], Field(default_factory=tuple)]


class JsonObjectResponse(RootModel[JsonObject]):
    """Validated object response for the JSON schema endpoint."""


class SnapshotMessage(ObserverModel):
    type: Literal["snapshot"]
    state: StateResponse


class EventsMessage(ObserverModel):
    type: Literal["events"]
    events: Annotated[Sequence[EventEnvelope], Field(default_factory=tuple)]
    state: StateResponse


class HeartbeatMessage(ObserverModel):
    type: Literal["heartbeat"]


class GapMessage(ObserverModel):
    type: Literal["gap"]
    retention: RetentionGapResponse


class ErrorMessage(ObserverModel):
    type: Literal["error"]
    message: str


StreamMessage = (
    SnapshotMessage
    | EventsMessage
    | HeartbeatMessage
    | GapMessage
    | ErrorMessage
)


def event_input_mapping(event: EventInput) -> dict[str, object]:
    """Convert a validated request model to the core adapter seam."""

    return event.model_dump(mode="python", exclude_none=True)


def json_model(model: BaseModel | RootModel[JsonObject]) -> JsonObject:
    """Serialize a validated model as a JSON-compatible object."""

    value = model.model_dump(mode="json", exclude_none=True)
    if not isinstance(value, dict):  # pragma: no cover - guarded by model type
        raise TypeError("observer transport model must serialize to an object")
    return value


__all__ = [
    "AdapterResponse",
    "CapabilityResponse",
    "CapabilitySchemaResponse",
    "CatalogResponse",
    "ContextRequest",
    "ContextResponse",
    "ContractResponse",
    "ErrorMessage",
    "EventBatchRequest",
    "EventEnvelope",
    "EventEnvelopeDescription",
    "EventIngestRequest",
    "EventInput",
    "EventPageResponse",
    "EventsMessage",
    "FederateInspectionResponse",
    "GapMessage",
    "HealthResponse",
    "HeartbeatMessage",
    "IngestResponse",
    "InspectorResponse",
    "InspectorsResponse",
    "InteractionInspectionResponse",
    "JsonObjectResponse",
    "LiveMetricsResponse",
    "MetricsResponse",
    "ObjectInspectionResponse",
    "ResourceDescription",
    "RetentionGapResponse",
    "RetentionResponse",
    "SnapshotMessage",
    "StateResponse",
    "StatusRequest",
    "StreamDescription",
    "StreamMessage",
    "event_input_mapping",
    "json_model",
]
