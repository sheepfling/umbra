"""Assess saved synthetic measurements against an explicit, narrow reference load.

This is an opt-in developer budget check, not a CI timing assertion or an RTI,
browser, network, recording-duration or hardware-independent service guarantee.
"""

# pyright: strict
from __future__ import annotations

import argparse
from pathlib import Path

from .scale import ScaleReport
from .schemas import ObserverModel


class ReferenceBudget(ObserverModel):
    minimum_published_events: int = 20000
    minimum_pipeline_samples: int = 7
    retained_events: int = 2000
    participants: int = 100
    objects: int = 1000
    perspectives: int = 5000
    payload_bytes: int = 128
    inspector_rows: int = 64
    minimum_ingress_events_per_second: float = 1000
    maximum_full_pipeline_p95_ms: float = 500
    maximum_analytics_pipeline_p95_ms: float = 100
    maximum_full_response_bytes: int = 8 * 1024 * 1024


class ReferenceCase(ObserverModel):
    report_file: str
    source_fingerprint: str | None
    workload_within_reference: bool
    measurement_coverage_sufficient: bool
    ingress_budget_met: bool
    full_pipeline_budget_met: bool
    analytics_pipeline_budget_met: bool
    response_size_budget_met: bool
    semantic_checks_passed: bool
    passed: bool


class ReferenceAssessment(ObserverModel):
    assessment_version: str = "umbra-observer-scale-reference-v1"
    scope: str = "Saved in-process synthetic snapshot/analytics measurements only; not recording, HTTP/browser, provider or portable SLA coverage."
    budget: ReferenceBudget
    cases: tuple[ReferenceCase, ...]
    passed: bool


def assess(report: ScaleReport, name: str, budget: ReferenceBudget) -> ReferenceCase:
    load, snapshot = report.workload, report.snapshot
    shape = (load.retained_events <= budget.retained_events and load.participants <= budget.participants
             and load.objects <= budget.objects and snapshot.object_perspectives <= budget.perspectives
             and load.perspective_capacity <= budget.perspectives
             and load.payload_bytes <= budget.payload_bytes and load.inspector_rows <= budget.inspector_rows
             and snapshot.roster_rows <= budget.participants and snapshot.objects <= budget.objects
             and load.batch_size == 100)
    ingress = report.ingress_events_per_second >= budget.minimum_ingress_events_per_second
    full = snapshot.full_response_pipeline.p95_ms <= budget.maximum_full_pipeline_p95_ms
    analytics = snapshot.analytics_with_projection.p95_ms <= budget.maximum_analytics_pipeline_p95_ms
    size = snapshot.full_bytes <= budget.maximum_full_response_bytes
    semantics = report.retained_count_and_attribution_verified and report.clear_verified
    coverage = (load.events >= budget.minimum_published_events and not report.instrumentation_active_during_timings
                and not report.source_changed_during_run
                and bool(report.source_fingerprint)
                and snapshot.full_response_pipeline.samples >= budget.minimum_pipeline_samples
                and snapshot.analytics_with_projection.samples >= budget.minimum_pipeline_samples)
    return ReferenceCase(report_file=name, source_fingerprint=report.source_fingerprint, workload_within_reference=shape,
                         measurement_coverage_sufficient=coverage,
                         ingress_budget_met=ingress, full_pipeline_budget_met=full, analytics_pipeline_budget_met=analytics,
                         response_size_budget_met=size, semantic_checks_passed=semantics,
                         passed=shape and coverage and ingress and full and analytics and size and semantics)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reports", type=Path, nargs="+")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--enforce", action="store_true", help="return nonzero when any supplied report misses the reference envelope/budgets")
    args = parser.parse_args(argv)
    budget = ReferenceBudget()
    cases: list[ReferenceCase] = []
    for path in args.reports:
        if path.stat().st_size > 1024 * 1024:
            raise ValueError("scale reports are bounded to 1 MiB; no recording/federation data belongs here")
        cases.append(assess(ScaleReport.model_validate_json(path.read_text(encoding="utf-8")), path.name, budget))
    assessment = ReferenceAssessment(budget=budget, cases=tuple(cases), passed=all(case.passed for case in cases))
    text = assessment.model_dump_json(indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
        print(f"Wrote reference assessment to {args.output}: passed={assessment.passed}")
    else:
        print(text, end="")
    return 1 if args.enforce and not assessment.passed else 0


if __name__ == "__main__":
    raise SystemExit(main())
