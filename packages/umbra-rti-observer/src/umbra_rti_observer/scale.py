"""Repeatable internal observer scale measurements, not RTI conformance or an SLA."""

# pyright: strict
from __future__ import annotations

import argparse
import gc
import hashlib
import math
import os
import platform
import statistics
import time
import tracemalloc
from collections.abc import Callable
from datetime import datetime, timezone
from pathlib import Path
from typing import Annotated

from pydantic import VERSION, Field

from .analytics import build_analytics
from .core import ObserverStore
from .recording_codec import StorageFormat
from .recording_models import CompactRecordingDocument, RecordingDocument
from .scale_workload import PROFILES, Workload, observation, seed_inspectors
from .schemas import (
    AnalyticsWindow,
    EventEnvelope,
    ObserverModel,
    RuntimeSummaryResponse,
    StateResponse,
    StorageCoverageResponse,
)

Finite = Annotated[float, Field(ge=0, allow_inf_nan=False)]


class Timing(ObserverModel):
    samples: int
    minimum_ms: Finite
    median_ms: Finite
    p95_ms: Finite
    maximum_ms: Finite


def timing(samples: list[float]) -> Timing:
    values = sorted(samples)
    return Timing(samples=len(values), minimum_ms=values[0], median_ms=statistics.median(values),
                  p95_ms=values[math.ceil(.95 * len(values)) - 1], maximum_ms=values[-1])


def measured(operation: Callable[[], object], repeats: int) -> Timing:
    operation()  # Warmup is excluded. Automatic GC remains enabled as in the service.
    samples: list[float] = []
    for _ in range(repeats):
        started = time.perf_counter()
        operation()
        samples.append((time.perf_counter() - started) * 1000)
    return timing(samples)


class SnapshotProfile(ObserverModel):
    retained_events: int
    dropped_events: int
    roster_rows: int
    objects: int
    object_perspectives: int
    object_perspective_evictions: int | None
    omitted_attribute_changes: int | None
    attribute_changes: int
    full_bytes: int
    compact_bytes: int
    analytics_bytes: int
    analytics_window_bytes: int
    full_within_http_checker_limit: bool
    full_snapshot: Timing
    compact_snapshot: Timing
    validation: Timing
    serialization: Timing
    full_response_pipeline: Timing
    analytics_only: Timing
    analytics_with_full_snapshot: Timing
    analytics_with_projection: Timing
    runtime_summary_pipeline: Timing
    python_allocation_peak_bytes: int | None
    projection_allocation_peak_bytes: int | None
    storage: StorageCoverageResponse | None = None


class CaptureProfile(ObserverModel):
    storage_format: StorageFormat = "full-v1"
    started: bool
    unavailable_reason: str | None = None
    baseline_publish_commit: Timing
    start_ms: Finite
    recorded_publish_commit: Timing | None = None
    frames: int = 0
    accounted_bytes: int = 0
    export_bytes: int = 0
    export_validation_ms: Finite | None = None
    stop_reason: str | None = None
    decoded_state_bytes: int | None = None
    edit_count: int | None = None


class ScaleReport(ObserverModel):
    report_version: str = "umbra-observer-scale-v1"
    generated_at: str
    python: str
    python_implementation: str
    pydantic: str
    platform: str
    processor: str
    cpu_count: int | None
    source_fingerprint: str | None = None
    source_changed_during_run: bool = False
    instrumentation_active_during_timings: bool
    workload: Workload
    seed_events: int
    ingress_seconds: Finite
    ingress_events_per_second: Finite
    snapshot: SnapshotProfile
    recording: CaptureProfile | None
    retained_count_and_attribution_verified: bool
    clear_verified: bool
    measurement_scope: str = "In-process synthetic Python pipeline; no network, HTTP framework, browser or provider timing."


class ProfileOptions(ObserverModel):
    repeats: Annotated[int, Field(ge=1, le=50)] = 7
    memory_probe: bool = True
    recording_format: StorageFormat = "compact-v2"


def source_fingerprint() -> str:
    digest = hashlib.sha256()
    root = Path(__file__).parent
    for path in sorted(root.rglob("*.py")):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes().replace(b"\r\n", b"\n"))
        digest.update(b"\0")
    return digest.hexdigest()


def allocation_peak(operation: Callable[[], object]) -> int:
    gc.collect()
    tracemalloc.start()
    try:
        operation()
        return tracemalloc.get_traced_memory()[1]
    finally:
        tracemalloc.stop()


def profile(workload: Workload, *, repeats: int = 7, memory_probe: bool = True,
            recording_format: StorageFormat = "compact-v2") -> ScaleReport:
    options = ProfileOptions(repeats=repeats, memory_probe=memory_probe, recording_format=recording_format)
    repeats, memory_probe = options.repeats, options.memory_probe
    started_fingerprint = source_fingerprint()
    store = ObserverStore(max_events=workload.retained_events, max_object_perspectives=workload.perspective_capacity)
    try:
        seed_count = seed_inspectors(store, workload.inspector_rows, workload.participants)
        seed_tail = {row["sequence"]: EventEnvelope.model_validate(row) for row in store.events()}
        started = time.perf_counter()
        for first in range(0, workload.events, workload.batch_size):
            store.publish_many(observation(index, workload) for index in range(first, min(workload.events, first + workload.batch_size)))
        ingress = time.perf_counter() - started
        raw = store.state()
        snapshot = StateResponse.model_validate(raw)
        compact = StateResponse.model_validate(store.state(include_events=False))
        now = max((row.observed_at for row in snapshot.normalized_events or ()), default=time.time()) + .001
        analytics = build_analytics(snapshot, now=now)
        full_bytes = len(snapshot.model_dump_json(exclude_none=True).encode("utf-8"))
        total = workload.events + seed_count
        retained = tuple(snapshot.normalized_events or ())
        count = len(retained)
        exact_input = True
        for row in retained:
            if row.sequence <= seed_count:
                exact_input = exact_input and row == seed_tail.get(row.sequence)
            else:
                original = observation(row.sequence - seed_count - 1, workload)
                wire = row.model_dump(mode="python", exclude_unset=True)
                exact_input = exact_input and row.family == "generic" and all(wire.get(key) == value for key, value in original.items())
        verified = (snapshot.retention.retained_events == count <= workload.retained_events
                    and snapshot.retention.next_sequence == total + 1
                    and snapshot.retention.dropped_events + count == total
                    and [row.sequence for row in retained] == list(range(total - count + 1, total + 1))
                    and exact_input
                    and sum(row.events for row in analytics.activity) == analytics.events_in_window
                    and sum(row.event_count for row in analytics.participants) + analytics.unattributed_events == analytics.events_in_window)
        if not verified:
            raise ValueError("synthetic workload violated retained count/attribution invariants")
        instrumented = tracemalloc.is_tracing()
        snapshot_profile = SnapshotProfile(
            retained_events=count, dropped_events=snapshot.retention.dropped_events, roster_rows=len(snapshot.federate_roster),
            objects=len(snapshot.inspectors.objects), object_perspectives=len(snapshot.inspectors.object_observations.snapshots) if snapshot.inspectors.object_observations else 0,
            object_perspective_evictions=snapshot.inspectors.object_observations.dropped_snapshots if snapshot.inspectors.object_observations else None,
            omitted_attribute_changes=snapshot.inspectors.object_observations.omitted_changes if snapshot.inspectors.object_observations else None,
            attribute_changes=len(snapshot.inspectors.object_observations.changes) if snapshot.inspectors.object_observations else 0,
            full_bytes=full_bytes, compact_bytes=len(compact.model_dump_json(exclude_none=True).encode("utf-8")),
            analytics_bytes=len(analytics.model_dump_json().encode("utf-8")), full_within_http_checker_limit=full_bytes <= 8 * 1024 * 1024,
            analytics_window_bytes=len(AnalyticsWindow.model_validate(store.analytics_window()).model_dump_json(exclude_none=True).encode("utf-8")),
            full_snapshot=measured(store.state, repeats), compact_snapshot=measured(lambda: store.state(include_events=False), repeats),
            validation=measured(lambda: StateResponse.model_validate(raw), repeats),
            serialization=measured(lambda: snapshot.model_dump_json(exclude_none=True), repeats),
            full_response_pipeline=measured(lambda: StateResponse.model_validate(store.state()).model_dump_json(exclude_none=True), repeats),
            analytics_only=measured(lambda: build_analytics(snapshot, now=now), repeats),
            analytics_with_full_snapshot=measured(lambda: build_analytics(StateResponse.model_validate(store.state()), now=now), repeats),
            analytics_with_projection=measured(lambda: build_analytics(AnalyticsWindow.model_validate(store.analytics_window()), now=now), repeats),
            runtime_summary_pipeline=measured(lambda: RuntimeSummaryResponse.model_validate(store.runtime_summary()).model_dump_json(), repeats),
            python_allocation_peak_bytes=None,
            projection_allocation_peak_bytes=None,
            storage=snapshot.storage,
        )
        full_peak, projection_peak = None, None
        if memory_probe and not instrumented:
            full_peak = allocation_peak(lambda: build_analytics(StateResponse.model_validate(store.state()), now=now).model_dump_json())
            projection_peak = allocation_peak(lambda: build_analytics(AnalyticsWindow.model_validate(store.analytics_window()), now=now).model_dump_json())
        snapshot_profile = SnapshotProfile.model_validate({**snapshot_profile.model_dump(), "python_allocation_peak_bytes": full_peak,
                                                          "projection_allocation_peak_bytes": projection_peak})
        capture_profile = None
        if workload.recording_commits:
            next_index = workload.events

            def publish_commit() -> None:
                nonlocal next_index
                store.publish_many(observation(index, workload) for index in range(next_index, next_index + workload.batch_size))
                next_index += workload.batch_size

            baseline = measured(publish_commit, repeats)
            started = time.perf_counter()
            try:
                capture = store.start_capture("Synthetic scale capture", max_frames=workload.recording_commits + 2,
                                              storage_format=options.recording_format)
            except ValueError as error:
                capture_profile = CaptureProfile(storage_format=recording_format, started=False, unavailable_reason=str(error), baseline_publish_commit=baseline,
                                                 start_ms=(time.perf_counter() - started) * 1000)
            else:
                start_ms = (time.perf_counter() - started) * 1000
                samples: list[float] = []
                for _ in range(workload.recording_commits):
                    current_capture = store.capture_info()
                    if current_capture is None or not current_capture.active:
                        break
                    started = time.perf_counter()
                    publish_commit()
                    samples.append((time.perf_counter() - started) * 1000)
                info = store.stop_capture(capture.recording_id)
                document = store.capture_document(capture.recording_id)
                started = time.perf_counter()
                validated = CompactRecordingDocument.model_validate(document) if recording_format == "compact-v2" else RecordingDocument.model_validate(document)
                validation_ms = (time.perf_counter() - started) * 1000
                capture_profile = CaptureProfile(storage_format=recording_format, started=True, baseline_publish_commit=baseline, start_ms=start_ms,
                                                 recorded_publish_commit=timing(samples) if samples else None,
                                                 frames=info.frame_count, accounted_bytes=info.captured_bytes,
                                                 export_bytes=len(validated.model_dump_json(exclude_unset=True).encode("utf-8")),
                                                 export_validation_ms=validation_ms, stop_reason=info.stop_reason,
                                                 decoded_state_bytes=info.decoded_state_bytes, edit_count=info.edit_count)
        store.clear()
        cleared = StateResponse.model_validate(store.state())
        clear_verified = not cleared.normalized_events and not cleared.federate_roster and not cleared.inspectors.objects
        if not clear_verified:
            raise ValueError("clear failed to reset the synthetic view")
        return ScaleReport(generated_at=datetime.now(timezone.utc).isoformat(), python=platform.python_version(),
                           python_implementation=platform.python_implementation(), pydantic=VERSION,
                           platform=platform.platform(), processor=platform.processor(), cpu_count=os.cpu_count(),
                           source_fingerprint=started_fingerprint,
                           source_changed_during_run=source_fingerprint() != started_fingerprint,
                           instrumentation_active_during_timings=instrumented, workload=workload, seed_events=seed_count,
                           ingress_seconds=ingress, ingress_events_per_second=workload.events / ingress,
                           snapshot=snapshot_profile, recording=capture_profile,
                           retained_count_and_attribution_verified=verified, clear_verified=clear_verified)
    finally:
        store.close()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=tuple(PROFILES), default="standard")
    parser.add_argument("--repeats", type=int, default=7)
    parser.add_argument("--no-memory-probe", action="store_true", help="skip the separate allocation probe; timings never include its tracing")
    parser.add_argument("--recording-format", choices=("full-v1", "compact-v2"), default="compact-v2")
    parser.add_argument("--output", type=Path, help="save the named JSON measurement report")
    args = parser.parse_args(argv)
    report = profile(PROFILES[args.profile], repeats=args.repeats, memory_probe=not args.no_memory_probe,
                     recording_format=args.recording_format)
    text = report.model_dump_json(indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
        print(f"Wrote synthetic scale measurements to {args.output}")
    else:
        print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
