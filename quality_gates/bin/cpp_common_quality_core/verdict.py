"""Aggregate gate verdict and evidence comparisons."""

from __future__ import annotations

import hashlib
import json
import time
from collections.abc import Mapping
from pathlib import Path
from typing import cast

from .common import _finding, _report, _under
from .manifest import _manifest_binding, _manifest_findings
from .models import (
    GateReport,
    JsonObject,
    JsonValue,
    SetComparison,
    ValueComparison,
    VerdictEvidence,
    VerdictPolicy,
)


def _field(payload: Mapping[str, JsonValue], dotted: str) -> JsonValue:
    value: JsonValue = payload
    for part in dotted.split("."):
        if not isinstance(value, dict) or part not in value:
            raise KeyError(dotted)
        value = value[part]
    return value


def _string_array(value: JsonValue, context: str) -> list[str]:
    if not isinstance(value, list) or any(
        not isinstance(item, str) or not item for item in value
    ):
        raise ValueError(f"{context} needs a nonempty string array")
    return cast(list[str], value)


def check_verdict(
    config_path: Path,
    policy: VerdictPolicy,
    run_id: str | None,
    manifest_path: Path | None = None,
) -> GateReport:
    rows = policy.get("reports")
    minimum_reports = int(policy.get("min_reports", 1))
    if minimum_reports < 1:
        raise ValueError("verdict.min_reports must be at least 1")
    if not isinstance(rows, list) or len(rows) < minimum_reports:
        raise ValueError("verdict.reports must meet min_reports")
    if (
        sum(isinstance(row, dict) and row.get("required", True) is True for row in rows)
        < minimum_reports
    ):
        raise ValueError("verdict needs at least min_reports required entries")
    findings: list[dict[str, str]] = []
    evidence: list[VerdictEvidence] = []
    reports_by_name: dict[str, Mapping[str, JsonValue]] = {}
    manifest_digest = None
    for key in ("require_manifest", "require_clean_source"):
        if key in policy and not isinstance(policy.get(key), bool):
            raise TypeError(f"verdict.{key} must be boolean")
    if "manifest_source_root" in policy and not isinstance(
        policy["manifest_source_root"], str
    ):
        raise TypeError("verdict.manifest_source_root must be a path")
    if policy.get("require_manifest", False) and manifest_path is None:
        findings.append(_finding("missing_build_manifest", "pass --manifest"))
    if manifest_path is not None:
        manifest, manifest_digest = _manifest_binding(manifest_path, run_id)
        reports_by_name["@manifest"] = cast(Mapping[str, JsonValue], manifest)
        findings.extend(_manifest_findings(manifest))
        expected_root = policy.get("manifest_source_root")
        if expected_root is not None and Path(
            manifest["source"]["root"]
        ).resolve() != _under(config_path.parent, expected_root):
            findings.append(_finding("wrong_manifest_source_root", str(expected_root)))
        if policy.get("require_clean_source", False) and manifest["source"]["dirty"]:
            findings.append(
                _finding("dirty_manifest_source", "source checkout is dirty")
            )
    max_age = policy.get("max_age_seconds")
    if run_id is None and max_age is None:
        raise ValueError(
            "verdict requires --run-id or max_age_seconds to reject stale reports"
        )
    if max_age is not None and int(max_age) < 0:
        raise ValueError("max_age_seconds must be nonnegative")
    now = time.time()
    names: set[str] = set()
    paths: set[Path] = set()
    for row in rows:
        if not isinstance(row, dict) or not row.get("name") or not row.get("path"):
            raise ValueError("every verdict report needs name and path")
        if set(row) - {
            "name",
            "path",
            "required",
            "pass_field",
            "expected",
            "require_run_id",
            "expected_schema",
            "expected_gate",
            "require_manifest",
        }:
            raise ValueError(f"unknown verdict report key: {row['name']}")
        name = row["name"]
        if name == "@manifest":
            raise ValueError("@manifest is reserved for set checks")
        if (
            not isinstance(row.get("required", True), bool)
            or not isinstance(row.get("require_run_id", True), bool)
            or not isinstance(row.get("require_manifest", True), bool)
        ):
            raise TypeError(f"verdict report flags must be boolean: {name}")
        expected_schema = row.get("expected_schema")
        if expected_schema is not None and (
            type(expected_schema) is not int or expected_schema < 1
        ):
            raise ValueError(f"expected_schema must be a positive integer: {name}")
        expected_gate = row.get("expected_gate")
        if expected_gate is not None and (
            not isinstance(expected_gate, str) or not expected_gate
        ):
            raise ValueError(f"expected_gate must be nonempty text: {name}")
        if name in names:
            findings.append(_finding("duplicate_gate_name", name))
        names.add(name)
        path = _under(config_path.parent, row["path"])
        if path in paths:
            findings.append(
                _finding("duplicate_gate_report_path", str(path), path=name)
            )
        paths.add(path)
        required = row.get("required", True)
        if not path.is_file():
            if required:
                findings.append(_finding("missing_gate_report", str(path), path=name))
            evidence.append({"name": name, "status": "missing"})
            continue
        if max_age is not None and now - path.stat().st_mtime > int(max_age):
            findings.append(_finding("stale_gate_report", str(path), path=name))
        report_bytes = path.read_bytes()
        report = json.loads(report_bytes)
        if not isinstance(report, dict):
            raise TypeError(f"expected a JSON object: {path}")
        report_digest = hashlib.sha256(report_bytes).hexdigest()
        reports_by_name[name] = report
        if (
            manifest_digest is not None
            and row.get("require_manifest", True)
            and report.get("manifest_sha256") != manifest_digest
        ):
            findings.append(_finding("wrong_report_manifest", str(path), path=name))
        if expected_schema is not None and (
            type(report.get("schema")) is not int or report["schema"] != expected_schema
        ):
            findings.append(_finding("wrong_report_schema", str(path), path=name))
        if expected_gate is not None and report.get("gate") != expected_gate:
            findings.append(_finding("wrong_report_gate", str(path), path=name))
        if (
            run_id is not None
            and row.get("require_run_id", True)
            and report.get("run_id") != run_id
        ):
            findings.append(_finding("wrong_run_id", str(path), path=name))
        field = row.get("pass_field", "passed")
        try:
            observed = _field(report, field)
        except KeyError:
            findings.append(_finding("missing_pass_field", field, path=name))
            evidence.append(
                {"name": name, "status": "invalid", "sha256": report_digest}
            )
            continue
        expected = row.get("expected", True)
        passed = type(observed) is type(expected) and observed == expected
        evidence.append(
            {
                "name": name,
                "status": "passed" if passed else "failed",
                "observed": observed,
                "expected": expected,
                "report": str(path),
                "sha256": report_digest,
            }
        )
        if required and not passed:
            findings.append(
                _finding(
                    "gate_failed",
                    f"{field}={observed!r}; expected {expected!r}",
                    path=name,
                )
            )
    set_results: list[SetComparison] = []
    checks = policy.get("set_checks", [])
    if not isinstance(checks, list):
        raise TypeError("verdict.set_checks must be an array")
    check_names: set[str] = set()
    for check in checks:
        keys = {"name", "left_report", "left_field", "right_report", "right_field"}
        if (
            not isinstance(check, dict)
            or set(check) != keys
            or any(not isinstance(check[key], str) or not check[key] for key in keys)
        ):
            raise ValueError("set check needs name, report names, and field paths")
        if check["name"] in check_names:
            raise ValueError(f"duplicate set check: {check['name']}")
        check_names.add(check["name"])
        try:
            left = _field(reports_by_name[check["left_report"]], check["left_field"])
            right = _field(reports_by_name[check["right_report"]], check["right_field"])
        except KeyError:
            findings.append(_finding("missing_set_check_evidence", check["name"]))
            continue
        left_items = _string_array(left, f"set check {check['name']}")
        right_items = _string_array(right, f"set check {check['name']}")
        if len(left_items) != len(set(left_items)) or len(right_items) != len(
            set(right_items)
        ):
            findings.append(_finding("duplicate_set_check_item", check["name"]))
        left_set, right_set = set(left_items), set(right_items)
        missing, unexpected = sorted(left_set - right_set), sorted(right_set - left_set)
        set_results.append(
            {
                "name": check["name"],
                "left_count": len(left_items),
                "right_count": len(right_items),
                "missing_from_right": missing[:20],
                "unexpected_on_right": unexpected[:20],
            }
        )
        if missing or unexpected:
            findings.append(
                _finding(
                    "set_check_mismatch",
                    f"{check['name']}: missing={missing[:8]}, unexpected={unexpected[:8]}",
                )
            )
    value_results: list[ValueComparison] = []
    value_checks = policy.get("value_checks", [])
    if not isinstance(value_checks, list):
        raise TypeError("verdict.value_checks must be an array")
    for check in value_checks:
        keys = {"name", "left_report", "left_field", "right_report", "right_field"}
        if (
            not isinstance(check, dict)
            or set(check) != keys
            or any(not isinstance(check[key], str) or not check[key] for key in keys)
        ):
            raise ValueError("value check needs name, report names, and field paths")
        if check["name"] in check_names:
            raise ValueError(f"duplicate comparison name: {check['name']}")
        check_names.add(check["name"])
        try:
            left = _field(reports_by_name[check["left_report"]], check["left_field"])
            right = _field(reports_by_name[check["right_report"]], check["right_field"])
        except KeyError:
            findings.append(_finding("missing_value_check_evidence", check["name"]))
            continue
        if type(left) not in (str, int, float, bool) or type(right) not in (
            str,
            int,
            float,
            bool,
        ):
            raise ValueError(f"value check fields need scalar values: {check['name']}")
        equal = type(left) is type(right) and left == right
        value_results.append({"name": check["name"], "left": left, "right": right})
        if not equal:
            findings.append(
                _finding(
                    "value_check_mismatch", f"{check['name']}: {left!r} != {right!r}"
                )
            )
    report = _report(
        "verdict",
        findings,
        cast(
            JsonObject,
            {
                "reports": evidence,
                "set_checks": set_results,
                "value_checks": value_results,
            },
        ),
        run_id,
    )
    if manifest_digest is not None:
        report["manifest_sha256"] = manifest_digest
    return report
