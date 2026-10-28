"""Build input and source-state manifest records."""

from __future__ import annotations

import hashlib
import json
import os
import re
from datetime import datetime, timezone
from pathlib import Path
from typing import cast

from .common import _finding, _git_bytes, _sha256_file, _under
from .models import (
    BuildManifest,
    JsonObject,
    SourceState,
)


def _source_state(root: Path) -> SourceState:
    root = root.resolve()
    revision = _git_bytes(root, "rev-parse", "HEAD").decode().strip()
    status = _git_bytes(root, "status", "--porcelain=v1", "-z", "--untracked-files=all")
    diff = _git_bytes(root, "diff", "--binary", "HEAD", "--")
    untracked = _git_bytes(root, "ls-files", "--others", "--exclude-standard", "-z")
    digest = hashlib.sha256()

    def add(data: bytes) -> None:
        digest.update(len(data).to_bytes(8, "big"))
        digest.update(data)

    add(status)
    add(diff)
    for raw in sorted(item for item in untracked.split(b"\0") if item):
        path = root / os.fsdecode(raw)
        add(raw)
        if path.is_symlink():
            add(os.fsencode(os.readlink(path)))
        elif path.is_file():
            add(bytes.fromhex(_sha256_file(path)))
        else:
            raise ValueError(f"untracked input disappeared: {path}")
    return {
        "root": str(root),
        "revision": revision,
        "dirty": bool(status),
        "state_sha256": digest.hexdigest(),
    }


def create_manifest(
    root: Path,
    run_id: str,
    configuration: str,
    toolchain: str,
    targets: list[str],
    expected_executables: list[str],
    inputs: list[str],
    require_clean: bool,
    artifact_root: Path | None = None,
) -> BuildManifest:
    if not all((run_id, configuration, toolchain)):
        raise ValueError("manifest needs run ID, configuration, and toolchain")
    if not targets or not inputs or any(not item for item in [*targets, *inputs]):
        raise ValueError("manifest needs selected targets and input files")
    if len(set(targets)) != len(targets) or len(set(expected_executables)) != len(
        expected_executables
    ):
        raise ValueError("manifest target names must be unique")
    root = root.resolve()
    source = _source_state(root)
    if require_clean and source["dirty"]:
        raise ValueError("manifest requires a clean source checkout")
    input_rows = []
    seen: set[Path] = set()
    for value in inputs:
        path = _under(root, value)
        if path in seen or not path.is_file():
            raise ValueError(f"manifest input is absent or duplicated: {path}")
        seen.add(path)
        input_rows.append({"path": str(path), "sha256": _sha256_file(path)})
    return {
        "schema": 1,
        "kind": "cpp-quality-build-manifest",
        "run_id": run_id,
        "created_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "source": source,
        "build": {
            "configuration": configuration,
            "toolchain": toolchain,
            "selected_targets": sorted(targets),
            "expected_executables": sorted(expected_executables),
            "artifact_root": str(artifact_root.resolve()) if artifact_root else None,
        },
        "inputs": input_rows,
    }


def _manifest_binding(path: Path, run_id: str | None) -> tuple[BuildManifest, str]:
    raw = path.read_bytes()
    manifest = cast(JsonObject, json.loads(raw))
    if (
        not isinstance(manifest, dict)
        or type(manifest.get("schema")) is not int
        or manifest["schema"] != 1
        or manifest.get("kind") != "cpp-quality-build-manifest"
    ):
        raise ValueError("manifest needs schema 1 and cpp-quality-build-manifest kind")
    if not isinstance(run_id, str) or not run_id or manifest.get("run_id") != run_id:
        raise ValueError("manifest run_id differs from --run-id")
    return cast(BuildManifest, manifest), hashlib.sha256(raw).hexdigest()


def _manifest_findings(manifest: BuildManifest) -> list[dict[str, str]]:
    source = manifest.get("source")
    build = manifest.get("build")
    inputs = manifest.get("inputs")
    if (
        not isinstance(source, dict)
        or not isinstance(source.get("root"), str)
        or not isinstance(source.get("revision"), str)
        or type(source.get("dirty")) is not bool
        or not isinstance(source.get("state_sha256"), str)
        or not isinstance(build, dict)
        or not all(
            isinstance(build.get(key), str) and build[key]
            for key in ("configuration", "toolchain")
        )
        or not isinstance(build.get("selected_targets"), list)
        or not build["selected_targets"]
        or not isinstance(build.get("expected_executables"), list)
        or not (
            build.get("artifact_root") is None
            or isinstance(build.get("artifact_root"), str)
        )
        or not isinstance(inputs, list)
        or not inputs
    ):
        raise ValueError("manifest has incomplete source, build, or input metadata")
    for key in ("selected_targets", "expected_executables"):
        names = build[key]
        if any(not isinstance(name, str) or not name for name in names) or len(
            names
        ) != len(set(names)):
            raise ValueError(f"manifest {key} needs unique nonempty names")
    findings: list[dict[str, str]] = []
    current = _source_state(Path(source["root"]))
    for key in ("revision", "dirty", "state_sha256"):
        if current[key] != source[key]:
            findings.append(_finding("manifest_source_changed", key))
    seen: set[Path] = set()
    for item in inputs:
        if (
            not isinstance(item, dict)
            or not isinstance(item.get("path"), str)
            or not isinstance(item.get("sha256"), str)
            or not re.fullmatch(r"[0-9a-fA-F]{64}", item["sha256"])
        ):
            raise ValueError("manifest input needs path and SHA-256")
        path = Path(item["path"]).resolve()
        if path in seen:
            raise ValueError(f"manifest repeats input: {path}")
        seen.add(path)
        if not path.is_file() or _sha256_file(path) != item["sha256"].lower():
            findings.append(_finding("manifest_input_changed", str(path)))
    return findings
