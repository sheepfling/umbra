"""Bounded immutable observer-state journal; no web, provider or filesystem I/O."""

# pyright: strict
from __future__ import annotations

import json
import time
import uuid
from dataclasses import dataclass
from typing import Literal

from .models import JsonObject, to_json_object
from .recording_codec import (
    COMPACT_CODEC,
    COMPACT_RECORDING_VERSION,
    MAX_DECODED_STATE_BYTES,
    MAX_STATE_BYTES,
    MAX_TOTAL_EDITS,
    StateLimit,
    StorageFormat,
    difference,
    encode,
    state_size,
)

RECORDING_VERSION = "umbra-observer-recording-v1"
MAX_RECORDING_BYTES = 16 * 1024 * 1024
MAX_RECORDING_FRAMES = 2000
CaptureChange = Literal["start", "events", "metadata", "clear", "checkpoint", "stop"]
StopReason = Literal["user", "frame-limit", "byte-limit", "invalid-state", "backend-stopped", "state-limit", "decode-limit", "edit-limit"]


class CaptureConflict(ValueError):
    """The requested recording is not the retained/active recording."""


def _label(value: object) -> str:
    if not isinstance(value, str) or not value.strip() or len(value) > 120:
        raise ValueError("recording label must contain 1..120 characters")
    return value


def _limit(name: str, value: object, upper: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= upper:
        raise ValueError(f"{name} must be an integer in 1..{upper}")
    return value


@dataclass(frozen=True, slots=True)
class CapturedFrame:
    offset_seconds: float
    encoded: str
    byte_count: int


@dataclass(frozen=True, slots=True)
class CaptureInfo:
    recording_id: str
    label: str
    started_at: float
    active: bool
    frame_count: int
    captured_bytes: int
    max_frames: int
    max_bytes: int
    duration_seconds: float
    stop_reason: StopReason | None
    storage_format: StorageFormat = "full-v1"
    decoded_state_bytes: int | None = None
    max_decoded_state_bytes: int | None = None
    max_state_bytes: int | None = None
    edit_count: int | None = None

    def to_json(self) -> JsonObject:
        return {"recording_id": self.recording_id, "label": self.label, "started_at": self.started_at,
                "active": self.active, "frame_count": self.frame_count, "captured_bytes": self.captured_bytes,
                "max_frames": self.max_frames, "max_bytes": self.max_bytes, "duration_seconds": self.duration_seconds,
                "stop_reason": self.stop_reason, "storage_format": self.storage_format,
                "decoded_state_bytes": self.decoded_state_bytes, "max_decoded_state_bytes": self.max_decoded_state_bytes,
                "max_state_bytes": self.max_state_bytes, "edit_count": self.edit_count}


class CaptureBuffer:
    """Owned by the store's lock. Every accepted state is encoded once, never mutated.

    Frames represent committed batches/metadata/clear boundaries, plus explicit
    clock checkpoints. Arrivals include the entire committed batch even when
    some envelopes already expired from that frame's event-history window.
    Limits stop capture, never evict earlier frames or interrupt collection.
    """

    def __init__(self, label: str, contract: JsonObject, *, max_frames: int = 600,
                 max_bytes: int = MAX_RECORDING_BYTES) -> None:
        label = _label(label)
        max_frames = _limit("max_frames", max_frames, MAX_RECORDING_FRAMES)
        max_bytes = _limit("max_bytes", max_bytes, MAX_RECORDING_BYTES)
        self.recording_id = str(uuid.uuid4())
        self.label = label
        self.started_at = time.time()
        self._started = time.monotonic()
        self._contract = json.dumps(contract, ensure_ascii=False, allow_nan=False, separators=(",", ":"))
        self._frames: list[CapturedFrame] = []
        # Includes the encoded manifest and conservative space for document/frame
        # wrappers and stop metadata. Export additionally enforces the exact limit.
        self._bytes: int = len(self._contract.encode("utf-8")) + len(label.encode("utf-8")) + 4096
        self.max_frames, self.max_bytes = max_frames, max_bytes
        self.stop_reason: StopReason | None = None
        self.stopped_at: float | None = None

    def append(self, state: JsonObject, change: CaptureChange, arrivals: list[JsonObject] | None = None) -> None:
        if self.stop_reason is not None:
            return
        if len(self._frames) >= self.max_frames:
            self.stop("frame-limit")
            return
        offset = max(self._frames[-1].offset_seconds if self._frames else 0.0, time.monotonic() - self._started) if self._frames else 0.0
        try:
            encoded = json.dumps({"index": len(self._frames), "offset_seconds": offset, "captured_at": time.time(),
                                  "change": change, "arrivals": arrivals or [], "state": state},
                                 ensure_ascii=False, allow_nan=False, separators=(",", ":"))
            size = len(encoded.encode("utf-8")) + 1
        except (ValueError, TypeError, UnicodeError):
            self.stop("invalid-state")
            return
        if self._bytes + size > self.max_bytes:
            self.stop("byte-limit")
            return
        self._frames.append(CapturedFrame(offset, encoded, size))
        self._bytes += size

    def stop(self, reason: StopReason = "user") -> None:
        if self.stop_reason is None:
            self.stop_reason, self.stopped_at = reason, time.time()

    def info(self) -> CaptureInfo:
        return CaptureInfo(self.recording_id, self.label, self.started_at, self.stop_reason is None,
                           len(self._frames), self._bytes, self.max_frames, self.max_bytes,
                           self._frames[-1].offset_seconds if self._frames else 0.0, self.stop_reason)

    def document(self) -> JsonObject:
        if self.stop_reason is None:
            raise CaptureConflict("Stop the recording before exporting its fixed timeline.")
        if not self._frames:
            raise CaptureConflict("No frame fitted the recording limit; start a new capture with a smaller retained history.")
        return to_json_object({"recording_version": RECORDING_VERSION, "recording_id": self.recording_id,
                               "label": self.label, "contract": json.loads(self._contract),
                               "started_at": self.started_at, "stopped_at": self.stopped_at,
                               "duration_seconds": self._frames[-1].offset_seconds,
                               "stop_reason": self.stop_reason, "max_frames": self.max_frames,
                               "max_bytes": self.max_bytes, "captured_bytes": self._bytes,
                               "frames": [json.loads(row.encoded) for row in self._frames]})


class CompactCaptureBuffer(CaptureBuffer):
    """One immutable baseline and lossless edits, not sampled full snapshots.

    The previous state is an owned working value solely for producing edits.
    Accepted frames are encoded/detached. Byte, reconstruction and operation
    work budgets stop before accepting a frame; collection remains independent.
    """

    def __init__(self, label: str, contract: JsonObject, *, max_frames: int = 600,
                 max_bytes: int = MAX_RECORDING_BYTES) -> None:
        super().__init__(label, contract, max_frames=max_frames, max_bytes=max_bytes)
        self._baseline: str | None = None
        self._previous: JsonObject | None = None
        self._decoded_bytes = 0
        self._edit_count = 0

    def append(self, state: JsonObject, change: CaptureChange, arrivals: list[JsonObject] | None = None) -> None:
        if self.stop_reason is not None:
            return
        if len(self._frames) >= self.max_frames:
            self.stop("frame-limit")
            return
        offset = max(self._frames[-1].offset_seconds, time.monotonic() - self._started) if self._frames else 0.0
        try:
            size = state_size(state)
            if self._decoded_bytes + size > MAX_DECODED_STATE_BYTES:
                self.stop("decode-limit")
                return
            edits = difference(self._previous, state) if self._previous is not None else []
            if self._edit_count + len(edits) > MAX_TOTAL_EDITS:
                self.stop("edit-limit")
                return
            encoded = encode(to_json_object({"index": len(self._frames), "offset_seconds": offset, "captured_at": time.time(),
                              "change": change, "arrivals": arrivals or [], "edits": [edit.to_json() for edit in edits],
                              "state_bytes": size}))
            baseline = encode(state) if self._baseline is None else None
            frame_bytes = len(encoded.encode("utf-8")) + 1
            extra = len(baseline.encode("utf-8")) if baseline is not None else 0
        except StateLimit:
            self.stop("state-limit")
            return
        except (ValueError, TypeError, UnicodeError, RecursionError):
            self.stop("invalid-state")
            return
        if self._bytes + frame_bytes + extra > self.max_bytes:
            self.stop("byte-limit")
            return
        self._frames.append(CapturedFrame(offset, encoded, frame_bytes))
        self._baseline = baseline if baseline is not None else self._baseline
        self._previous = state
        self._bytes += frame_bytes + extra
        self._decoded_bytes += size
        self._edit_count += len(edits)

    def info(self) -> CaptureInfo:
        base = super().info()
        return CaptureInfo(base.recording_id, base.label, base.started_at, base.active, base.frame_count,
                           base.captured_bytes, base.max_frames, base.max_bytes, base.duration_seconds, base.stop_reason,
                           "compact-v2", self._decoded_bytes, MAX_DECODED_STATE_BYTES, MAX_STATE_BYTES, self._edit_count)

    def stop(self, reason: StopReason = "user") -> None:
        super().stop(reason)
        self._previous = None  # Export needs encoded evidence, not a live diff baseline.

    def document(self) -> JsonObject:
        if self.stop_reason is None:
            raise CaptureConflict("Stop the recording before exporting its fixed timeline.")
        if self._baseline is None or not self._frames:
            raise CaptureConflict("No compact baseline fitted the recording limits.")
        return to_json_object({"recording_version": COMPACT_RECORDING_VERSION, "codec": COMPACT_CODEC,
                               "recording_id": self.recording_id, "label": self.label, "contract": json.loads(self._contract),
                               "started_at": self.started_at, "stopped_at": self.stopped_at,
                               "duration_seconds": self._frames[-1].offset_seconds, "stop_reason": self.stop_reason,
                               "max_frames": self.max_frames, "max_bytes": self.max_bytes, "captured_bytes": self._bytes,
                               "max_state_bytes": MAX_STATE_BYTES, "max_decoded_state_bytes": MAX_DECODED_STATE_BYTES,
                               "decoded_state_bytes": self._decoded_bytes, "edit_count": self._edit_count,
                               "baseline": json.loads(self._baseline), "frames": [json.loads(row.encoded) for row in self._frames]})
