"""C++ source ownership and file-size gate."""

from __future__ import annotations

from collections import defaultdict
from pathlib import Path
from typing import cast

from .common import _files, _finding, _matches, _report
from .models import (
    DEFAULT_SOURCE_EXCLUDE,
    DEFAULT_SOURCE_INCLUDE,
    FRAGMENT_SUFFIXES,
    HEADER_SUFFIXES,
    IMPLEMENTATION_SUFFIXES,
    INCLUDE_RE,
    OWNER_SUFFIXES,
    SOURCE_SUFFIXES,
    GateReport,
    JsonObject,
    SourcePolicy,
    SourceSizeRecord,
)


def check_source(root: Path, policy: SourcePolicy, run_id: str | None) -> GateReport:
    root = root.resolve()
    if not root.is_dir():
        raise ValueError(f"source root is not a directory: {root}")
    includes = policy.get("include", DEFAULT_SOURCE_INCLUDE)
    excludes = policy.get("exclude", DEFAULT_SOURCE_EXCLUDE)
    if (
        not isinstance(includes, list)
        or not includes
        or not isinstance(excludes, list)
        or any(not isinstance(item, str) or not item for item in [*includes, *excludes])
    ):
        raise ValueError(
            "source include/exclude must be lists, with a nonempty include"
        )
    soft_lines = int(policy.get("soft_lines", 3000))
    hard_lines = int(policy.get("hard_lines", 10000))
    hard_bytes = policy.get("hard_bytes")
    hard_bytes = int(hard_bytes) if hard_bytes is not None else None
    if (
        soft_lines < 0
        or hard_lines < soft_lines
        or (hard_bytes is not None and hard_bytes < 0)
    ):
        raise ValueError("source size limits must be nonnegative and hard >= soft")
    legacy = policy.get("legacy", {})
    fragment_exceptions = policy.get("fragment_exceptions", {})
    include_roots = [root / item for item in policy.get("include_roots", [])]
    if not isinstance(legacy, dict) or not isinstance(fragment_exceptions, dict):
        raise TypeError("source legacy and fragment_exceptions must be objects")
    for relative, exemption in legacy.items():
        if (
            not isinstance(exemption, dict)
            or not exemption.get("reason")
            or "max_physical_lines" not in exemption
        ):
            raise ValueError(
                f"legacy exception needs reason and max_physical_lines: {relative}"
            )
        if set(exemption) - {
            "reason",
            "max_physical_lines",
            "max_expanded_lines",
            "max_expanded_bytes",
        }:
            raise ValueError(f"unknown legacy exception key: {relative}")
    for relative, exemption in fragment_exceptions.items():
        if (
            not isinstance(exemption, dict)
            or not exemption.get("reason")
            or "max_lines" not in exemption
        ):
            raise ValueError(
                f"fragment exception needs reason and max_lines: {relative}"
            )
        if set(exemption) - {"reason", "max_lines"}:
            raise ValueError(f"unknown fragment exception key: {relative}")
    files = {
        path.resolve(): path.relative_to(root).as_posix()
        for path in _files(root)
        if path.is_file()
        and not path.is_symlink()
        and path.suffix.lower() in SOURCE_SUFFIXES
        and _matches(path.relative_to(root).as_posix(), includes)
        and not _matches(path.relative_to(root).as_posix(), excludes)
    }
    findings: list[dict[str, str]] = []
    minimum_files = int(policy.get("min_files", 1))
    if minimum_files < 1:
        raise ValueError("source.min_files must be at least 1")
    if len(files) < minimum_files:
        findings.append(
            _finding("too_few_source_files", f"{len(files)} < {minimum_files}")
        )
    lines: dict[Path, int] = {}
    bytes_by_file: dict[Path, int] = {}
    includes_by_file: dict[Path, list[Path]] = {}
    consumers: dict[Path, set[Path]] = defaultdict(set)
    for path, relative in files.items():
        data = path.read_bytes()
        lines[path] = len(data.splitlines())
        bytes_by_file[path] = len(data)
        included: list[Path] = []
        for line in data.decode("utf-8", errors="replace").splitlines():
            match = INCLUDE_RE.match(line)
            if match is None:
                continue
            token = Path(match.group(1))
            if token.suffix.lower() in IMPLEMENTATION_SUFFIXES:
                findings.append(
                    _finding(
                        "included_source_file",
                        f"includes {match.group(1)} as source",
                        path=relative,
                    )
                )
            candidate = next(
                (
                    item.resolve()
                    for base in [path.parent, root, *include_roots]
                    if (item := base / token).resolve() in files
                ),
                None,
            )
            if candidate is None:
                if token.suffix.lower() in FRAGMENT_SUFFIXES:
                    findings.append(
                        _finding(
                            "unresolved_fragment",
                            match.group(1),
                            path=relative,
                        )
                    )
                continue
            consumers[candidate].add(path)
            if candidate.suffix.lower() in FRAGMENT_SUFFIXES | IMPLEMENTATION_SUFFIXES:
                included.append(candidate)
        includes_by_file[path] = included

    expanded_cache: dict[Path, tuple[int, int]] = {}

    def expanded(path: Path, active: frozenset[Path] = frozenset()) -> tuple[int, int]:
        if path in expanded_cache:
            return expanded_cache[path]
        if path in active:
            findings.append(
                _finding(
                    "include_cycle", "local source include cycle", path=files[path]
                )
            )
            return 0, 0
        total_lines = lines[path]
        total_bytes = bytes_by_file[path]
        for child in includes_by_file[path]:
            child_lines, child_bytes = expanded(child, active | {path})
            total_lines += child_lines
            total_bytes += child_bytes
        expanded_cache[path] = total_lines, total_bytes
        return total_lines, total_bytes

    largest: list[SourceSizeRecord] = []
    for path, relative in files.items():
        physical = lines[path]
        effective, effective_bytes = (
            expanded(path)
            if path.suffix.lower() in OWNER_SUFFIXES
            else (physical, bytes_by_file[path])
        )
        largest.append(
            {
                "path": relative,
                "physical_lines": physical,
                "expanded_lines": effective,
                "bytes": bytes_by_file[path],
                "expanded_bytes": effective_bytes,
            }
        )
        exemption = legacy.get(relative)
        if exemption is not None:
            if not isinstance(exemption, dict) or not exemption.get("reason"):
                raise ValueError(f"legacy exception needs a reason: {relative}")
            if physical > int(exemption["max_physical_lines"]):
                findings.append(
                    _finding(
                        "legacy_physical_growth",
                        f"{physical} > {exemption['max_physical_lines']}",
                        path=relative,
                    )
                )
            if effective > int(
                exemption.get("max_expanded_lines", exemption["max_physical_lines"])
            ):
                findings.append(
                    _finding(
                        "legacy_expanded_growth",
                        f"{effective} expanded lines exceed exception",
                        path=relative,
                    )
                )
            if "max_expanded_bytes" in exemption and effective_bytes > int(
                exemption["max_expanded_bytes"]
            ):
                findings.append(
                    _finding(
                        "legacy_expanded_byte_growth",
                        f"{effective_bytes} expanded bytes exceed exception",
                        path=relative,
                    )
                )
        elif effective > hard_lines:
            findings.append(
                _finding(
                    "source_hard_lines",
                    f"{effective} expanded lines > {hard_lines}",
                    path=relative,
                )
            )
        elif effective > soft_lines:
            findings.append(
                _finding(
                    "source_soft_lines",
                    f"{effective} expanded lines > {soft_lines}",
                    path=relative,
                    severity="warning",
                )
            )
        byte_exception = (
            exemption is not None
            and "max_expanded_bytes" in exemption
            and effective_bytes <= int(exemption["max_expanded_bytes"])
        )
        if (
            hard_bytes is not None
            and effective_bytes > hard_bytes
            and not byte_exception
        ):
            findings.append(
                _finding(
                    "source_hard_bytes",
                    f"{effective_bytes} expanded bytes > {hard_bytes}",
                    path=relative,
                )
            )
        if path.suffix.lower() in FRAGMENT_SUFFIXES:
            count = len(consumers[path])
            if count < 2:
                waiver = fragment_exceptions.get(relative)
                if waiver is not None:
                    if (
                        not isinstance(waiver, dict)
                        or not waiver.get("reason")
                        or physical > int(waiver["max_lines"])
                    ):
                        findings.append(
                            _finding(
                                "invalid_fragment_exception",
                                "exception needs reason and current growth guard",
                                path=relative,
                            )
                        )
                else:
                    findings.append(
                        _finding(
                            "single_consumer_fragment",
                            f"{count} direct consumers; fold into owner or justify shared use",
                            path=relative,
                        )
                    )
        if (
            path.suffix.lower() in HEADER_SUFFIXES
            and physical >= int(policy.get("single_consumer_header_lines", 500))
            and len(consumers[path]) == 1
        ):
            findings.append(
                _finding(
                    "large_single_consumer_header",
                    f"{physical} lines and one direct consumer; review ownership",
                    path=relative,
                    severity="warning",
                )
            )
    for relative in set(legacy) - set(files.values()):
        findings.append(
            _finding("missing_legacy_source", "exception path is absent", path=relative)
        )
    for relative in set(fragment_exceptions) - set(files.values()):
        findings.append(
            _finding(
                "missing_fragment_exception_source",
                "exception path is absent",
                path=relative,
            )
        )
    largest.sort(key=lambda item: item["expanded_lines"], reverse=True)
    return _report(
        "source",
        findings,
        cast(
            JsonObject,
            {
                "file_count": len(files),
                "fragment_count": sum(
                    p.suffix.lower() in FRAGMENT_SUFFIXES for p in files
                ),
                "largest": largest[:10],
            },
        ),
        run_id,
    )
