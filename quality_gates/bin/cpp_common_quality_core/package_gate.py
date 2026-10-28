"""Installed package binding and consumer-smoke gate."""

from __future__ import annotations

import time
from pathlib import Path
from typing import cast

from .common import _cache_values, _finding, _read_json, _report, _sha256_file, _under
from .models import (
    ConsumerMetrics,
    GateReport,
    JsonObject,
    JsonValue,
    PackageBindingReport,
    PackagePolicy,
)
from .test_gate import _execution_cases


def check_package(root: Path, policy: PackagePolicy, run_id: str | None) -> GateReport:
    root = root.resolve()
    prefix_value = policy.get("prefix")
    if not isinstance(prefix_value, str):
        raise TypeError("package requires a prefix path")
    prefix = _under(root, prefix_value)
    if not prefix.is_dir():
        raise ValueError("package prefix is missing")
    if ("cache" in policy) == ("binding_report" in policy):
        raise ValueError("package needs exactly one of cache or binding_report")
    if "binding_keys" in policy and "cache_path_keys" in policy:
        raise ValueError("choose binding_keys or cache_path_keys")
    if "cache" in policy:
        cache = _under(root, policy["cache"])
        if not cache.is_file():
            raise ValueError("package CMake cache is missing")
        binding_values = _cache_values(cache)
        binding_base = cache.parent
        binding_source = str(cache)
        binding_document = None
    else:
        binding_report_value = policy.get("binding_report")
        if not isinstance(binding_report_value, str):
            raise ValueError("package requires a binding report path")
        binding_report = _under(root, binding_report_value)
        bindings = cast(PackageBindingReport, _read_json(binding_report))
        if bindings.get("schema") != 1 or not isinstance(
            bindings.get("bindings"), dict
        ):
            raise ValueError("binding report needs schema 1 and a bindings object")
        binding_values = cast(dict[str, JsonValue], bindings.get("bindings"))
        binding_base = binding_report.parent
        binding_source = str(binding_report)
        binding_document = bindings
    if "require_consumer_smoke" in policy and not isinstance(
        policy["require_consumer_smoke"], bool
    ):
        raise TypeError("package.require_consumer_smoke must be boolean")
    keys = policy.get("binding_keys", policy.get("cache_path_keys"))
    if (
        not isinstance(keys, list)
        or not keys
        or any(not isinstance(key, str) or not key for key in keys)
    ):
        raise ValueError("package.binding_keys must be a nonempty array")
    required_globs = policy.get("required_globs", [])
    if not isinstance(required_globs, list) or any(
        not isinstance(pattern, str) or not pattern for pattern in required_globs
    ):
        raise ValueError("package.required_globs must be a string array")
    findings: list[dict[str, str]] = []
    if (
        binding_document is not None
        and run_id is not None
        and binding_document.get("run_id") != run_id
    ):
        findings.append(_finding("wrong_binding_run_id", binding_source))
    if "max_evidence_age_seconds" in policy:
        age_limit = int(policy["max_evidence_age_seconds"])
        if age_limit < 0:
            raise ValueError("package.max_evidence_age_seconds must be nonnegative")
        if time.time() - Path(binding_source).stat().st_mtime > age_limit:
            findings.append(_finding("stale_package_binding", binding_source))
    resolved_paths: dict[str, str] = {}
    for key in keys:
        value = binding_values.get(key, "")
        if not isinstance(value, str) or not value or value.endswith("-NOTFOUND"):
            findings.append(_finding("missing_package_binding", f"{key} is absent"))
            continue
        actual = _under(binding_base, value)
        resolved_paths[key] = str(actual)
        if not actual.exists() or not actual.is_relative_to(prefix):
            findings.append(
                _finding(
                    "wrong_package_binding",
                    f"{key} resolves to {actual}, outside {prefix} or absent",
                )
            )
    for pattern in required_globs:
        if not any(path.is_file() for path in prefix.glob(pattern)):
            findings.append(_finding("missing_installed_artifact", pattern))
    consumer = policy.get("consumer")
    consumer_metrics: ConsumerMetrics = {"checked": False}
    if consumer is None:
        if policy.get("require_consumer_smoke", False):
            findings.append(
                _finding("missing_consumer_smoke", "consumer evidence is required")
            )
    elif isinstance(consumer, dict):
        if set(consumer) - {
            "executable",
            "junit",
            "result_json",
            "expected_sha256",
            "min_cases",
            "max_junit_age_seconds",
            "max_result_age_seconds",
        }:
            raise ValueError("unknown package.consumer key")
        if "executable" not in consumer or (
            ("junit" in consumer) == ("result_json" in consumer)
        ):
            raise ValueError("package.consumer needs executable and one result path")
        executable_value = consumer.get("executable")
        if not isinstance(executable_value, str):
            raise ValueError("package.consumer needs an executable path")
        result_value = consumer.get("result_json")
        if not isinstance(result_value, str):
            result_value = consumer.get("junit")
        if not isinstance(result_value, str):
            raise ValueError("package.consumer needs one result path")
        executable = _under(root, executable_value)
        result_path = _under(root, result_value)
        expected_digest = consumer.get("expected_sha256")
        if not executable.is_file() or not result_path.is_file() or not expected_digest:
            findings.append(
                _finding(
                    "incomplete_consumer_smoke",
                    "need executable, result report, and expected SHA-256",
                )
            )
        else:
            digest = _sha256_file(executable)
            if digest.casefold() != str(expected_digest).casefold():
                findings.append(
                    _finding(
                        "consumer_binary_changed",
                        "consumer SHA-256 differs; it may have been rebuilt",
                    )
                )
            age_limit = int(
                consumer.get(
                    "max_result_age_seconds",
                    consumer.get("max_junit_age_seconds", 3600),
                )
            )
            if age_limit < 0:
                raise ValueError("consumer result age must be nonnegative")
            if time.time() - result_path.stat().st_mtime > age_limit:
                findings.append(
                    _finding(
                        "stale_consumer_result",
                        f"result is older than {age_limit} seconds",
                    )
                )
            cases = _execution_cases(
                result_path,
                normalized_json="result_json" in consumer,
                run_id=run_id,
            )
            for name, occurrences in cases.items():
                if len(occurrences) != 1:
                    findings.append(_finding("duplicate_consumer_case", name))
            minimum = int(consumer.get("min_cases", 1))
            if minimum < 1:
                raise ValueError("consumer.min_cases must be at least 1")
            if len(cases) < minimum:
                findings.append(
                    _finding(
                        "too_few_consumer_tests",
                        f"{len(cases)} < {minimum}",
                    )
                )
            for name, occurrences in cases.items():
                for status in occurrences:
                    if status != "passed":
                        findings.append(_finding("consumer_test_not_passed", name))
            consumer_metrics = {
                "checked": True,
                "case_count": sum(map(len, cases.values())),
                "sha256": digest,
            }
    else:
        raise ValueError("package.consumer must be an object")
    return _report(
        "package",
        findings,
        cast(
            JsonObject,
            {
                "prefix": str(prefix),
                "binding_source": binding_source,
                "bindings": resolved_paths,
                "consumer": consumer_metrics,
            },
        ),
        run_id,
    )
