"""Pydantic schemas for the optional observer HTTP transport.

The core store intentionally has no web-framework dependency.  This module is
the opposite boundary: every request body and every finite JSON response that
FastAPI exposes has a named, inspectable model.  Provider-specific fields are
allowed only where the wire contract explicitly calls them extensions.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from typing import TYPE_CHECKING, Annotated, ClassVar, Literal, TypeAlias, cast

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    RootModel,
    SerializerFunctionWrapHandler,
    StringConstraints,
    TypeAdapter,
    model_serializer,
    model_validator,
)
from typing_extensions import TypeAliasType

JsonText = Annotated[str, StringConstraints(strip_whitespace=False)]

if TYPE_CHECKING:
    JsonValue: TypeAlias = None | bool | int | float | str | list["JsonValue"] | dict[str, "JsonValue"]
else:
    # A named runtime alias gives OpenAPI a recursive schema; the static alias
    # above also works with checkers that cannot resolve TypeAliasType recursion.
    JsonValue = TypeAliasType(
        "JsonValue",
        None | bool | int | float | JsonText | list["JsonValue"] | dict[JsonText, "JsonValue"],
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
    extensions: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]

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


TimeId = Annotated[str, StringConstraints(strip_whitespace=False, min_length=1, max_length=256, pattern=r"\S")]
TimeNumber = Annotated[int | float, Field(allow_inf_nan=False)]


class TimeRequestObservation(ObserverModel):
    kind: Literal["request"]
    logical_time: TimeNumber | None = None
    session_id: TimeId | None = None
    request_id: TimeId | None = None


class TimeGrantObservation(ObserverModel):
    kind: Literal["grant"]
    logical_time: TimeNumber | None = None
    session_id: TimeId | None = None
    request_id: TimeId | None = None


class TimeStateObservation(ObserverModel):
    kind: Literal["state"]
    session_id: TimeId | None = None
    logical_time: TimeNumber | None = None
    lookahead: TimeNumber | None = None
    regulating: bool | None = None
    constrained: bool | None = None
    galt: TimeNumber | None = None
    lbts: TimeNumber | None = None


class TimeOutcomeObservation(ObserverModel):
    kind: Literal["failed", "cancelled"]
    session_id: TimeId | None = None
    request_id: TimeId | None = None
    reason: JsonText | None = None


TimeObservationInput = Annotated[
    TimeRequestObservation | TimeGrantObservation | TimeStateObservation | TimeOutcomeObservation,
    Field(discriminator="kind"),
]


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
    attributes: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    interaction_key: str | None = None
    interaction_class: str | None = None
    interaction_handle: str | None = None
    parameters: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    tag: Annotated[JsonValue, Field(default=None)]
    details: Annotated[Mapping[JsonText, JsonValue] | None, Field(default=None)]
    logical_time: int | float | None = None
    operation: str | None = None
    time_observation: TimeObservationInput | None = None

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
    attributes: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]
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
    parameters: Annotated[Mapping[JsonText, JsonValue], Field(default_factory=dict)]
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
    federate: str | dict[str, JsonValue] | None = None
    participant: str | dict[str, JsonValue] | None = None
    federate_name: str | None = None
    participant_name: str | None = None
    logical_time: Annotated[int | float | None, Field(default=None, allow_inf_nan=False)]
    operation: str | None = None
    details: Mapping[str, JsonValue] | None = None
    object_key: str | None = None
    time_observation: TimeObservationInput | None = None


class ObjectPerspectiveResponse(ObserverModel):
    """Null federate is global; a name is explicit adapter attribution only."""

    federate: NonEmptyText | None
    generation: SequenceNumber
    last_sequence: SequenceNumber
    object: ObjectInspectionResponse

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        # No return annotation: preserve the named model's serialization schema.
        # State/stream transports omit optional nulls, but this null is required
        # and means global scope, not missing attribution support.
        serialized = cast(dict[str, object], handler(self))
        serialized["federate"] = self.federate
        return serialized


class ObjectAttributeChangeResponse(ObserverModel):
    sequence: SequenceNumber
    object_key: NonEmptyText
    federate: NonEmptyText | None
    generation: SequenceNumber
    attribute: JsonText
    value: JsonValue
    previous_known: bool
    previous_value: JsonValue
    source: NonEmptyText
    observed_at: Annotated[float, Field(allow_inf_nan=False)]

    @model_serializer(mode="wrap")
    def preserve_null_values(self, handler: SerializerFunctionWrapHandler):
        # Keep required nullable fields even under parent exclude_none=True.
        # Leave the return unannotated so JSON Schema still describes this DTO.
        serialized = cast(dict[str, object], handler(self))
        serialized["federate"] = self.federate
        if self.value is None:
            serialized["value"] = None
        if self.previous_value is None:
            serialized["previous_value"] = None
        return serialized


class ObjectObservationsResponse(ObserverModel):
    snapshots: Sequence[ObjectPerspectiveResponse]
    changes: Sequence[ObjectAttributeChangeResponse]
    history_truncated: bool
    history_first_sequence: NonNegativeInt
    history_last_sequence: NonNegativeInt


class TimeScopeResponse(ObserverModel):
    source: NonEmptyText
    session_id: TimeId | None
    federation: str | None
    federate: str | None

    @model_serializer(mode="wrap")
    def preserve_scope(self, handler: SerializerFunctionWrapHandler):
        serialized = cast(dict[str, object], handler(self))
        serialized.update(session_id=self.session_id, federation=self.federation, federate=self.federate)
        return serialized


class TimeSampleResponse(ObserverModel):
    sequence: SequenceNumber
    logical_time: TimeNumber | None = None
    observed_at: Annotated[float, Field(allow_inf_nan=False)]
    request_id: TimeId | None = None


class ReportedTimeStateResponse(ObserverModel):
    logical_time: TimeNumber | None = None
    lookahead: TimeNumber | None = None
    regulating: bool | None = None
    constrained: bool | None = None
    galt: TimeNumber | None = None
    lbts: TimeNumber | None = None


class TimeSnapshotResponse(ObserverModel):
    scope: TimeScopeResponse
    last_sequence: SequenceNumber
    observed_at: Annotated[float, Field(allow_inf_nan=False)]
    status: Literal["observed", "ended"]
    last_grant: TimeSampleResponse | None = None
    last_request: TimeSampleResponse | None = None
    reported_state: ReportedTimeStateResponse | None = None
    state_sequence: SequenceNumber | None = None
    uncorrelated_requests: NonNegativeInt
    uncorrelated_grants: NonNegativeInt


class TimeRequestResponse(ObserverModel):
    scope: TimeScopeResponse
    request_id: TimeId
    request_sequence: SequenceNumber
    requested_time: TimeNumber | None = None
    requested_at: Annotated[float, Field(allow_inf_nan=False)]
    status: Literal["pending", "granted", "failed", "cancelled", "ambiguous", "session-ended"]
    outcome_sequence: SequenceNumber | None = None
    outcome_at: Annotated[float | None, Field(default=None, allow_inf_nan=False)]
    granted_time: TimeNumber | None = None
    latency_seconds: Annotated[float | None, Field(default=None, ge=0, allow_inf_nan=False)]
    pending_seconds: Annotated[float | None, Field(default=None, ge=0, allow_inf_nan=False)]
    reason: JsonText | None = None


class TimeInspectionResponse(ObserverModel):
    snapshots: Sequence[TimeSnapshotResponse]
    requests: Sequence[TimeRequestResponse]
    request_capacity: PositiveInt
    dropped_requests: NonNegativeInt
    duration_basis: Literal["observer-ingress-monotonic"]
    scope_basis: Literal["explicit-observations"]

    @model_validator(mode="after")
    def consistent_inspection(self) -> TimeInspectionResponse:
        def key(scope: TimeScopeResponse) -> tuple[str, str | None, str | None, str | None]:
            return scope.source, scope.session_id, scope.federation, scope.federate

        snapshots = {key(row.scope): row for row in self.snapshots}
        if len(snapshots) != len(self.snapshots) or len(self.requests) > self.request_capacity:
            raise ValueError("time scopes must be unique and request capacity must be respected")
        requests = {(key(row.scope), row.request_id) for row in self.requests}
        if len(requests) != len(self.requests):
            raise ValueError("duplicate scoped time request ID")
        for snapshot in self.snapshots:
            if any(sample.sequence > snapshot.last_sequence for sample in (snapshot.last_grant, snapshot.last_request) if sample):
                raise ValueError("time samples cannot be newer than their snapshot")
            if snapshot.state_sequence is not None and snapshot.state_sequence > snapshot.last_sequence:
                raise ValueError("reported time state cannot be newer than its snapshot")
        for row in self.requests:
            snapshot = snapshots.get(key(row.scope))
            if snapshot is None or row.scope.source == "unknown" or row.scope.session_id is None or row.scope.federate is None:
                raise ValueError("correlated requests require a known source, session and federate snapshot")
            if row.request_sequence > snapshot.last_sequence:
                raise ValueError("request is newer than its snapshot")
            if row.status == "pending":
                if any(value is not None for value in (row.outcome_sequence, row.outcome_at, row.granted_time, row.latency_seconds)):
                    raise ValueError("pending requests cannot claim an outcome")
            else:
                if (row.outcome_sequence is None or not row.request_sequence < row.outcome_sequence <= snapshot.last_sequence
                        or row.outcome_at is None or row.pending_seconds is not None):
                    raise ValueError("finished requests require ordered outcomes and no pending duration")
                if row.status != "granted" and (row.granted_time is not None or row.latency_seconds is not None):
                    raise ValueError("only matched grants can claim granted time or latency")
        return self


class InspectorsResponse(ObserverModel):
    objects: Annotated[Sequence[ObjectInspectionResponse], Field(default_factory=tuple)]
    interactions: Annotated[Sequence[InteractionInspectionResponse], Field(default_factory=tuple)]
    object_observations: ObjectObservationsResponse | None = None
    time: TimeInspectionResponse | None = None


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


class ActivityBucket(ObserverModel):
    started_at: float
    events: NonNegativeInt
    object_updates: NonNegativeInt
    interactions: NonNegativeInt
    callbacks: NonNegativeInt
    errors: NonNegativeInt


class FederateActivity(ObserverModel):
    federate: NonEmptyText
    role: str | None
    handle: str | None
    status: NonEmptyText
    source: NonEmptyText
    last_seen: float | None
    event_count: NonNegativeInt
    object_updates: NonNegativeInt
    interaction_count: NonNegativeInt
    error_count: NonNegativeInt
    object_keys: Sequence[str]
    latest_logical_time: int | float | None


class TimeObservation(ObserverModel):
    sequence: SequenceNumber
    federate: str | None
    logical_time: Annotated[int | float, Field(allow_inf_nan=False)]
    observed_at: float
    event_type: NonEmptyText
    operation: str | None
    observation_kind: Literal["timestamp", "request", "grant", "state"] = "timestamp"


class AnalyticsResponse(ObserverModel):
    contract_version: NonEmptyText
    basis: Literal["retained-observations"]
    generated_at: float
    window_start: float
    window_seconds: PositiveInt
    bucket_seconds: Annotated[float, Field(gt=0)]
    scoped_federate: str | None
    events_in_window: NonNegativeInt
    unattributed_events: NonNegativeInt
    history_truncated: bool
    retention: RetentionResponse
    participants: Sequence[FederateActivity]
    activity: Sequence[ActivityBucket]
    logical_times: Sequence[TimeObservation]
    event_types: Mapping[str, NonNegativeInt]
    sources: Mapping[str, NonNegativeInt]
    families: Mapping[str, NonNegativeInt]


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
