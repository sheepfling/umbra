"""Small deterministic functional checks; measurements are not CI timing claims."""
from __future__ import annotations

from unittest.mock import patch

import pytest
from pydantic import ValidationError
from umbra_rti_observer.scale import ScaleReport, profile
from umbra_rti_observer.scale_reference import ReferenceBudget, assess
from umbra_rti_observer.scale_workload import Workload


@pytest.mark.parametrize("rich", [False, True])
@pytest.mark.parametrize("explicit_identity", [False, True])
def test_scale_profile_checks_retention_projection_and_capture_without_an_rti(rich: bool, explicit_identity: bool) -> None:
    result = profile(Workload(events=25, retained_events=10, participants=3, objects=4, payload_bytes=4,
                              batch_size=5, inspector_rows=1 if rich else 0, recording_commits=2, explicit_identity=explicit_identity), repeats=1, memory_probe=False)
    assert result.retained_count_and_attribution_verified and result.clear_verified
    assert result.snapshot.retained_events == 10
    assert result.snapshot.dropped_events == 15 + result.seed_events
    assert result.snapshot.analytics_window_bytes < result.snapshot.full_bytes
    assert result.snapshot.analytics_with_projection.samples == 1
    assert result.snapshot.python_allocation_peak_bytes is None
    assert result.recording.started and result.recording.frames == 4 and result.recording.stop_reason == "user"
    assert ScaleReport.model_validate_json(result.model_dump_json()) == result
    assert result.snapshot.roster_rows <= 3


@pytest.mark.parametrize("fields", [{"events": True}, {"events": 200001}, {"retained_events": 0}, {"participants": -1}, {"payload_bytes": 4097}, {"recording_commits": 101}, {"objects": "5"}, {"perspective_capacity": 50001}])
def test_workloads_are_strict_and_bounded(fields: dict[str, object]) -> None:
    with pytest.raises(ValidationError): Workload(**fields)


def test_optional_allocation_probe_is_separate_and_recording_can_be_omitted() -> None:
    result = profile(Workload(events=3, retained_events=2, participants=1, objects=1, recording_commits=0), repeats=1)
    assert result.recording is None
    assert result.snapshot.python_allocation_peak_bytes > 0
    assert result.snapshot.projection_allocation_peak_bytes > 0


def test_reference_budgets_compare_measurements_without_hiding_outside_shapes() -> None:
    report = profile(Workload(events=3, retained_events=2, participants=1, objects=1, batch_size=100, recording_commits=0), repeats=1, memory_probe=False)
    assessment = assess(report, "synthetic.json", ReferenceBudget())
    assert assessment.workload_within_reference and assessment.semantic_checks_passed
    assert not assessment.measurement_coverage_sufficient and not assessment.passed
    assert assessment.full_pipeline_budget_met == (report.snapshot.full_response_pipeline.p95_ms <= 500)
    strict_size = assess(report, "synthetic.json", ReferenceBudget(maximum_full_response_bytes=1))
    assert not strict_size.response_size_budget_met and not strict_size.passed


def test_wider_payloads_are_diagnostic_not_promoted_into_the_reference_target() -> None:
    report = profile(Workload(events=3, retained_events=2, participants=1, objects=1, batch_size=100, payload_bytes=1024, recording_commits=0), repeats=1, memory_probe=False)
    assessment = assess(report, "wide.json", ReferenceBudget())
    assert not assessment.workload_within_reference and not assessment.passed


def test_source_changes_during_a_run_invalidate_reference_coverage() -> None:
    with patch("umbra_rti_observer.scale.source_fingerprint", side_effect=["a" * 64, "b" * 64]):
        report = profile(Workload(events=3, retained_events=2, participants=1, objects=1, batch_size=100, recording_commits=0), repeats=1, memory_probe=False)
    assert report.source_changed_during_run
    budget = ReferenceBudget(minimum_published_events=1, minimum_pipeline_samples=1)
    assert not assess(report, "changed.json", budget).measurement_coverage_sufficient
    stable = ScaleReport.model_validate({**report.model_dump(), "source_changed_during_run": False,
                                       "instrumentation_active_during_timings": False})
    assert assess(stable, "stable.json", budget).measurement_coverage_sufficient
