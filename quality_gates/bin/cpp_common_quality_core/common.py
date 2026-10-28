"""Shared JSON, policy, filesystem, and report helpers."""

from __future__ import annotations

import fnmatch
import hashlib
import json
import os
import subprocess
from collections.abc import Mapping
from pathlib import Path
from typing import cast

from .models import (
    SECTION_KEYS,
    GateReport,
    JsonObject,
    JsonValue,
)


def _read_json(path: Path) -> JsonObject:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise TypeError(f"expected a JSON object: {path}")
    return cast(JsonObject, payload)


def _config(path: Path, section: str) -> JsonObject:
    payload = _read_json(path)
    if payload.get("schema") != 1:
        raise ValueError(f"expected schema 1 config: {path}")
    value = payload.get(section)
    if not isinstance(value, dict):
        raise TypeError(f"config requires a {section} object: {path}")
    unknown = set(value) - SECTION_KEYS[section]
    if unknown:
        raise ValueError(
            f"unknown {section} config key(s): {', '.join(sorted(unknown))}"
        )
    return cast(JsonObject, value)


def _matches(path: str, patterns: list[str]) -> bool:
    folded = path.casefold()
    return any(fnmatch.fnmatchcase(folded, pattern.casefold()) for pattern in patterns)


def _finding(
    code: str, detail: str, *, path: str | None = None, severity: str = "error"
) -> dict[str, str]:
    finding = {"code": code, "severity": severity, "detail": detail}
    if path is not None:
        finding["path"] = path
    return finding


def _report(
    gate: str,
    findings: list[dict[str, str]],
    metrics: Mapping[str, JsonValue],
    run_id: str | None = None,
) -> GateReport:
    return {
        "schema": 1,
        "gate": gate,
        "run_id": run_id,
        "passed": not any(item["severity"] == "error" for item in findings),
        "findings": findings,
        "metrics": dict(metrics),
    }


def _emit(report: GateReport, output: Path | None) -> int:
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if output is None:
        print(encoded, end="")
    else:
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(encoded, encoding="utf-8")
        print(
            f"{report['gate']}: {'PASS' if report['passed'] else 'FAIL'}; "
            f"findings={len(report['findings'])}; report={output}"
        )
        for item in report["findings"][:20]:
            print(
                f"  {item['severity'].upper()} {item['code']}: "
                f"{item.get('path', '')} {item['detail']}"
            )
    return 0 if report["passed"] else 1


def _files(root: Path) -> list[Path]:
    try:
        result = subprocess.run(
            [
                "git",
                "-C",
                str(root),
                "ls-files",
                "-z",
                "--cached",
                "--others",
                "--exclude-standard",
            ],
            capture_output=True,
            check=False,
        )
        if result.returncode == 0:
            return [
                root / os.fsdecode(raw) for raw in result.stdout.split(b"\0") if raw
            ]
    except FileNotFoundError:
        pass
    # A Git checkout is not required when the checker is copied into another build.
    return [path for path in root.rglob("*") if path.is_file()]


def _under(root: Path, value: str) -> Path:
    candidate = Path(value)
    return (candidate if candidate.is_absolute() else root / candidate).resolve()


def _cache_values(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        key_type, separator, value = line.partition("=")
        if separator and not key_type.startswith(("#", "//")):
            values[key_type.partition(":")[0]] = value
    return values


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as binary:
        for chunk in iter(lambda: binary.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _git_bytes(root: Path, *arguments: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(root), *arguments], capture_output=True, check=False
    )
    if result.returncode:
        raise ValueError(result.stderr.decode(errors="replace").strip())
    return result.stdout
