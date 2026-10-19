"""Versioned, strict portable recording boundary; never executes captured data."""

# pyright: strict
from __future__ import annotations

import json
from collections.abc import Sequence
from typing import Annotated, Literal, TypeAlias

from pydantic import Field, model_validator

from .capture import (
    MAX_RECORDING_BYTES,
    MAX_RECORDING_FRAMES,
    CaptureChange,
    StopReason,
)
from .contract import OBSERVER_CONTRACT_VERSION, OBSERVER_JSON_MEDIA_TYPE
from .models import to_json_object
from .recording_codec import (
    MAX_DECODED_STATE_BYTES,
    MAX_FRAME_EDITS,
    MAX_PATH_DEPTH,
    MAX_STATE_BYTES,
    MAX_TOTAL_EDITS,
    DeleteKey,
    Edit,
    SetValue,
    SpliceList,
    StorageFormat,
    apply_edits,
    state_size,
)
from .schemas import (
    WIRE_REQUIRED_FIELDS,
    AnalyticsWindow,
    ContractResponse,
    EventEnvelope,
    JsonText,
    JsonValue,
    NonNegativeInt,
    ObserverModel,
    StateResponse,
)

RecordingId = Annotated[JsonText, Field(min_length=1, max_length=128)]
RecordingLabel = Annotated[JsonText, Field(min_length=1, max_length=120)]
FiniteTime = Annotated[float, Field(allow_inf_nan=False)]
Offset = Annotated[float, Field(ge=0, allow_inf_nan=False)]
FrameLimit = Annotated[int, Field(ge=1, le=MAX_RECORDING_FRAMES)]
ByteLimit = Annotated[int, Field(ge=1, le=MAX_RECORDING_BYTES)]


class RecordingStartRequest(ObserverModel):
    label: RecordingLabel
    replace_id: RecordingId | None = None
    max_frames: FrameLimit = 600
    max_bytes: Annotated[int, Field(ge=65536, le=MAX_RECORDING_BYTES)] = MAX_RECORDING_BYTES
    storage_format: StorageFormat = "full-v1"

    @model_validator(mode="after")
    def meaningful_label(self) -> RecordingStartRequest:
        if not self.label.strip():
            raise ValueError("recording label cannot be blank")
        return self


class RecordingTargetRequest(ObserverModel):
    recording_id: RecordingId


class CaptureStatus(ObserverModel):
    recording_id: RecordingId
    label: RecordingLabel
    started_at: FiniteTime
    active: bool
    frame_count: NonNegativeInt
    captured_bytes: NonNegativeInt
    max_frames: FrameLimit
    max_bytes: ByteLimit
    duration_seconds: Offset
    stop_reason: StopReason | None
    storage_format: StorageFormat | None = None
    decoded_state_bytes: NonNegativeInt | None = None
    max_decoded_state_bytes: Literal[1073741824] | None = None
    max_state_bytes: Literal[8388608] | None = None
    edit_count: Annotated[int, Field(ge=0, le=MAX_TOTAL_EDITS)] | None = None

    @model_validator(mode="after")
    def bounded_status(self) -> CaptureStatus:
        if self.frame_count > self.max_frames or self.captured_bytes > self.max_bytes or self.active != (self.stop_reason is None):
            raise ValueError("recording status must respect limits and explicit active/stop state")
        details = (self.decoded_state_bytes, self.max_decoded_state_bytes, self.max_state_bytes, self.edit_count)
        if self.storage_format == "compact-v2":
            if any(value is None for value in details) or self.decoded_state_bytes is None or self.decoded_state_bytes > MAX_DECODED_STATE_BYTES:
                raise ValueError("compact status requires bounded reconstruction metadata")
            if self.decoded_state_bytes < self.frame_count or self.edit_count is None or self.edit_count > self.frame_count * MAX_FRAME_EDITS:
                raise ValueError("compact status work must cover its retained frames")
        elif any(value is not None for value in details):
            raise ValueError("reconstruction metadata applies only to compact captures")
        return self


class RecordingStatusResponse(ObserverModel):
    contract_version: Literal["umbra-observer-contract-v1"] = "umbra-observer-contract-v1"
    recording: CaptureStatus | None
    scope: Literal["backend-process"] = "backend-process"


def require_recorded_collections(state: StateResponse) -> None:
    for model, required in ((state, WIRE_REQUIRED_FIELDS["StateResponse"]),
                            (state.inspectors, WIRE_REQUIRED_FIELDS["InspectorsResponse"])):
        if not required.issubset(model.model_fields_set):
            raise ValueError("recorded states require explicit wire collections, not constructor defaults")


class RecordingFrame(ObserverModel):
    index: NonNegativeInt
    offset_seconds: Offset
    captured_at: FiniteTime
    change: CaptureChange
    arrivals: Sequence[EventEnvelope]
    state: StateResponse

    @model_validator(mode="after")
    def complete_retained_view(self) -> RecordingFrame:
        state = self.state
        require_recorded_collections(state)
        if "normalized_events" not in state.model_fields_set or state.normalized_events is None:
            raise ValueError("recorded frames require their complete retained envelope window")
        events = state.normalized_events
        sequences = [row.sequence for row in events]
        retention = state.retention
        if sequences != sorted(set(sequences)) or len(sequences) != retention.retained_events:
            raise ValueError("recorded event windows must be unique, ordered and agree with retained counts")
        if sequences and (sequences[0] != retention.first_retained_sequence or sequences[-1] != retention.last_sequence or sequences[-1] != retention.next_sequence - 1):
            raise ValueError("recorded envelope boundaries must match retention")
        if sequences and len(sequences) != sequences[-1] - sequences[0] + 1:
            raise ValueError("a retained event window cannot contain internal cursor holes")
        arrivals = [row.sequence for row in self.arrivals]
        if arrivals != sorted(set(arrivals)) or arrivals and arrivals[-1] != retention.next_sequence - 1:
            raise ValueError("recorded arrivals must be ordered, unique and end at the committed cursor")
        if self.change != "events" and arrivals:
            raise ValueError("only event-commit frames contain new arrivals")
        if self.change == "clear" and events:
            raise ValueError("a clear boundary cannot retain cleared envelopes")
        by_sequence = {row.sequence: row for row in self.arrivals}
        if any(row.sequence in by_sequence and row != by_sequence[row.sequence] for row in events):
            raise ValueError("arrivals and retained envelopes must agree")
        # Check Python values before JSON-mode serializers can turn NaN into null.
        json.dumps(self.model_dump(mode="python", exclude_unset=True), allow_nan=False)
        return self


class RecordingDocument(ObserverModel):
    recording_version: Literal["umbra-observer-recording-v1"]
    recording_id: RecordingId
    label: RecordingLabel
    contract: ContractResponse
    started_at: FiniteTime
    stopped_at: FiniteTime
    duration_seconds: Offset
    stop_reason: StopReason
    max_frames: FrameLimit
    max_bytes: ByteLimit
    captured_bytes: NonNegativeInt
    frames: Annotated[Sequence[RecordingFrame], Field(min_length=1, max_length=MAX_RECORDING_FRAMES)]

    @model_validator(mode="after")
    def deterministic_timeline(self) -> RecordingDocument:
        frames = self.frames
        if not WIRE_REQUIRED_FIELDS["ContractResponse"].issubset(self.contract.model_fields_set):
            raise ValueError("recording requires an explicit capability manifest")
        if self.contract.contract_version != OBSERVER_CONTRACT_VERSION or self.contract.media_type != OBSERVER_JSON_MEDIA_TYPE:
            raise ValueError("recording uses an incompatible observer contract")
        if self.contract.event_schema_version != "umbra-runtime-observer-v1":
            raise ValueError("recording uses an incompatible event schema")
        if len(frames) > self.max_frames or self.captured_bytes > self.max_bytes:
            raise ValueError("recording exceeds its declared bounds")
        if self.stop_reason == "frame-limit" and len(frames) != self.max_frames:
            raise ValueError("a frame-limit stop must exhaust the declared frame capacity")
        if self.stop_reason == "user" and frames[-1].change != "stop":
            raise ValueError("a user-stopped recording requires its explicit final state")
        if frames[0].change != "start" or frames[0].offset_seconds != 0 or frames[0].arrivals:
            raise ValueError("recording begins with an explicit zero-offset baseline")
        if self.contract.contract_version != frames[0].state.contract_version or self.contract.capabilities != frames[0].state.capabilities:
            raise ValueError("initial contract and capability manifest must match the baseline")
        if self.duration_seconds != frames[-1].offset_seconds:
            raise ValueError("replay duration ends at the last retained state, not an uncaptured tail")
        for index, frame in enumerate(frames):
            if frame.index != index or frame.state.contract_version != self.contract.contract_version or frame.state.schema_version != self.contract.event_schema_version:
                raise ValueError("recorded frames must be indexed and use the advertised contract/schema")
            if index == 0:
                continue
            validate_transition(frames[index - 1], frame)
        # Includes extension values, which must remain finite JSON, and enforces
        # the declared byte bound even for files constructed by another backend.
        encoded = json.dumps(self.model_dump(mode="python", exclude_unset=True), ensure_ascii=False,
                             allow_nan=False, separators=(",", ":"))
        if len(encoded.encode("utf-8")) > self.captured_bytes:
            raise ValueError("recording document exceeds its reported capture budget")
        return self


def validate_transition(previous: RecordingFrame, frame: RecordingFrame) -> None:
    """Both storage versions preserve the same event/cursor/clock semantics."""
    previous_events, current_events = previous.state.normalized_events, frame.state.normalized_events
    if previous_events is None or current_events is None:
        raise ValueError("recorded frames require retained envelopes")
    if frame.change == "start" or frame.offset_seconds < previous.offset_seconds:
        raise ValueError("replay offsets must be monotonic; only the first frame is a baseline")
    before, after = previous.state.retention.next_sequence - 1, frame.state.retention.next_sequence - 1
    if after < before:
        raise ValueError("a capture cannot reset its observer cursor")
    arrivals = [row.sequence for row in frame.arrivals]
    if frame.change == "events":
        if not arrivals or arrivals[0] != before + 1 or len(arrivals) != after - before:
            raise ValueError("a committed batch must retain every new envelope, including immediate history expiry")
        count = frame.state.retention.retained_events
        expected = (*previous_events, *frame.arrivals)[-count:] if count else ()
        if tuple(current_events) != expected:
            raise ValueError("event commits cannot rewrite or resurrect retained envelopes")
        expired = len(previous_events) + len(frame.arrivals) - len(current_events)
        if frame.state.retention.dropped_events != previous.state.retention.dropped_events + expired:
            raise ValueError("event-commit expiry must account for every missing retained envelope")
    elif after != before:
        raise ValueError("only event commits advance the observer cursor")
    elif frame.change != "clear" and (frame.state.normalized_events != previous.state.normalized_events or frame.state.retention != previous.state.retention):
        raise ValueError("metadata/clock checkpoints cannot change retained history")


EditPath = Annotated[Sequence[JsonText | NonNegativeInt], Field(max_length=MAX_PATH_DEPTH)]


class RecordingSetEdit(ObserverModel):
    op: Literal["set"]
    path: EditPath
    value: JsonValue

    def to_edit(self) -> SetValue:
        return SetValue(tuple(self.path), self.value)


class RecordingDeleteEdit(ObserverModel):
    op: Literal["delete"]
    path: Annotated[Sequence[JsonText | NonNegativeInt], Field(min_length=1, max_length=MAX_PATH_DEPTH)]

    def to_edit(self) -> DeleteKey:
        return DeleteKey(tuple(self.path))


class RecordingSpliceEdit(ObserverModel):
    op: Literal["splice"]
    path: EditPath
    index: NonNegativeInt
    delete_count: NonNegativeInt
    items: Sequence[JsonValue]

    def to_edit(self) -> SpliceList:
        return SpliceList(tuple(self.path), self.index, self.delete_count, list(self.items))


RecordingEdit = Annotated[RecordingSetEdit | RecordingDeleteEdit | RecordingSpliceEdit, Field(discriminator="op")]


class RecordingDeltaFrame(ObserverModel):
    index: NonNegativeInt
    offset_seconds: Offset
    captured_at: FiniteTime
    change: CaptureChange
    arrivals: Sequence[EventEnvelope]
    edits: Annotated[Sequence[RecordingEdit], Field(max_length=MAX_FRAME_EDITS)]
    state_bytes: Annotated[int, Field(ge=1, le=MAX_STATE_BYTES)]


class CompactRecordingDocument(ObserverModel):
    recording_version: Literal["umbra-observer-recording-v2"]
    codec: Literal["observer-json-edits-v1"]
    recording_id: RecordingId
    label: RecordingLabel
    contract: ContractResponse
    started_at: FiniteTime
    stopped_at: FiniteTime
    duration_seconds: Offset
    stop_reason: StopReason
    max_frames: FrameLimit
    max_bytes: ByteLimit
    captured_bytes: NonNegativeInt
    max_state_bytes: Literal[8388608]
    max_decoded_state_bytes: Literal[1073741824]
    decoded_state_bytes: Annotated[int, Field(ge=1, le=MAX_DECODED_STATE_BYTES)]
    edit_count: Annotated[int, Field(ge=0, le=MAX_TOTAL_EDITS)]
    baseline: StateResponse
    frames: Annotated[Sequence[RecordingDeltaFrame], Field(min_length=1, max_length=MAX_RECORDING_FRAMES)]

    @model_validator(mode="after")
    def bounded_validated_timeline(self) -> CompactRecordingDocument:
        frames = self.frames
        require_recorded_collections(self.baseline)
        if not WIRE_REQUIRED_FIELDS["ContractResponse"].issubset(self.contract.model_fields_set):
            raise ValueError("compact recording requires an explicit capability manifest")
        if len(frames) > self.max_frames or self.captured_bytes > self.max_bytes:
            raise ValueError("compact recording exceeds declared bounds")
        if self.contract.contract_version != OBSERVER_CONTRACT_VERSION or self.contract.media_type != OBSERVER_JSON_MEDIA_TYPE or self.contract.event_schema_version != "umbra-runtime-observer-v1":
            raise ValueError("compact recording uses an incompatible contract/schema")
        if frames[0].change != "start" or frames[0].offset_seconds != 0 or frames[0].arrivals or frames[0].edits:
            raise ValueError("compact recording requires a zero-offset unedited baseline")
        if self.contract.capabilities != self.baseline.capabilities or self.baseline.contract_version != self.contract.contract_version:
            raise ValueError("compact baseline and capability manifest must match")
        if self.stop_reason == "frame-limit" and len(frames) != self.max_frames:
            raise ValueError("frame-limit stop must exhaust capacity")
        if self.stop_reason == "user" and frames[-1].change != "stop":
            raise ValueError("user stop requires its final committed state")
        if self.duration_seconds != frames[-1].offset_seconds:
            raise ValueError("duration must end at the last accepted state")
        if sum(row.state_bytes for row in frames) != self.decoded_state_bytes or sum(len(row.edits) for row in frames) != self.edit_count:
            raise ValueError("compact work accounting must include every frame/edit")
        # Only two validated states and one owned working JSON tree are live;
        # never expand the entire compact file into an unbounded response.
        working = to_json_object(self.baseline.model_dump(mode="python", exclude_unset=True))
        previous: RecordingFrame | None = None
        for index, row in enumerate(frames):
            if row.index != index:
                raise ValueError("compact frames must be sequentially indexed")
            edits: list[Edit] = [edit.to_edit() for edit in row.edits]
            working = apply_edits(working, edits)
            if state_size(working) > row.state_bytes:
                raise ValueError("a compact frame underreports its reconstructed state size")
            current = RecordingFrame.model_validate({"index": row.index, "offset_seconds": row.offset_seconds,
                "captured_at": row.captured_at, "change": row.change, "arrivals": row.arrivals, "state": working})
            if current.state.contract_version != self.contract.contract_version or current.state.schema_version != self.contract.event_schema_version:
                raise ValueError("every reconstructed state must use the advertised contract/schema")
            if previous is not None:
                validate_transition(previous, current)
            previous = current
        encoded = json.dumps(self.model_dump(mode="python", exclude_unset=True), ensure_ascii=False,
                             allow_nan=False, separators=(",", ":"))
        if len(encoded.encode("utf-8")) > self.captured_bytes:
            raise ValueError("compact document exceeds its encoded budget")
        return self


class RecordingAnalysisRequest(ObserverModel):
    frame: RecordingFrame
    window_seconds: Annotated[int, Field(ge=30, le=3600)] = 300
    bucket_count: Annotated[int, Field(ge=10, le=120)] = 30


class RecordedWindowAnalysisRequest(ObserverModel):
    window: AnalyticsWindow
    captured_at: FiniteTime
    window_seconds: Annotated[int, Field(ge=30, le=3600)] = 300
    bucket_count: Annotated[int, Field(ge=10, le=120)] = 30


PortableRecordingDocument: TypeAlias = Annotated[
    RecordingDocument | CompactRecordingDocument, Field(discriminator="recording_version")
]
