#!/usr/bin/env python3
"""Query the Umbra roadmap and native C++ test-to-standard mappings.

The Requirements Lab export and the Catch2 plan remain the sources of truth.
This tool only joins them for fast, read-only work selection; ``work`` resolves
one bounded implementation slice, ``focus`` resolves one exact lane's
candidate/status card, ``source`` resolves all planned cases under one known
C++ translation unit, and ``trace`` resolves one direct
test-to-requirement-to-2025-subsection mapping.  ``ready`` resolves exactly
one next implementation handoff from the indexed planned-row queue; source-only
reconciliation is opt-in with ``--include-source-only``.  When the indexed
queues are exhausted it returns a bounded open-family shortlist; it never edits
the Lab, the plan, or generated evidence.  ``unplanned`` is a
one-way source-to-plan reconciliation view; it never invents a requirement or
implementation status.
``matrix`` is the bounded reverse view: one exact test, C++ API surface (full
id or method shorthand),
requirement, standard subsection, or lane becomes one row per mapped test with
source, status, and requirement/section counts.
``gaps`` is the bounded forward requirement view: it lists pinned 2025
requirements that have no mapped C++ Catch2 row, grouped by canonical
document:clause subsection, so a new case can be selected without reopening
the full Requirements Lab export.  ``requirement`` uses the same corpus as a
bounded fallback when a reverse lookup has no mapped Catch2 row, returning the
normative statement and source needed to plan that case.
``case`` is the one-case handoff: it accepts an exact plan id or Catch2 title
and emits the source, direct requirement-to-2025-subsection pairs, roadmap
owner, API surfaces, and focused execution handles in one bounded card. It
never falls back to fuzzy search; use ``test`` or ``search`` for discovery.
``contract-drift`` is the bounded local contract-selector guard: it compares
native ``cpp/tests/...::Title`` references with current C++ declarations and
counts external portable-TCK symbols without treating them as native drift.
``lanes`` is the bounded forward view: an optional roadmap-family scope becomes
one row per exact Catch2 tag with mapping state and a deterministic next-case
handle, so lane selection does not require a broad plan search.  Its optional
disposition filter separates intentional no-standalone-surface decisions from
rows that still need a mapping decision.
It also accepts an exact indexed roadmap family id as an aggregate handle.
``roadmap`` is the bounded family search: an optional title/tag/action,
    Requirements-Lab id, canonical 2025 subsection, or official C++ API query
returns live family counts and copyable work/focus/matrix handles without
printing roadmap prose. An exact Catch2 lane-tag query additionally embeds the
lane's representative test/source/trace and focused CTest handles, so lane
ownership and mapping can be resolved in one command.
``dashboard`` composes the live roadmap/checklist and mapping counts with one
ready implementation handoff and a small open-family queue preview.  Use
``resume`` for the smaller first-read projection when resuming work.
``resume`` is the deliberately smaller first-read card: it keeps only the
current counts, one exact next handoff (or one bounded family choice), and the
copyable case/trace/matrix/focus/check handles.  It is intended for normal
work-loop resumes when the detailed dashboard would be unnecessary context.
When a family is deliberately queued as ``new-case-needed``, its choice row
also carries one bounded pinned-2025 coverage head and exact requirement/
section lookup commands; the full gap inventory remains opt-in.
``lab-issues`` reads the separate, machine-readable Requirements Lab issue
ledger without loading the Lab export or scanning C++ sources, so extraction
defects and recurrences stay visible without becoming implementation work.
Queue rows also expose an ``action_state`` so evidence-complete families and
diagnostic-only source drift cannot be mistaken for runnable work.
``plan`` indexes implementation-plan headings and can filter that heading
index without loading the plan prose.
Summary rows expose direct requirement-to-subsection pairs so callers do not
have to join independent arrays.
For mapped ``requirement`` and ``section`` reverse lookups, ``--summary`` uses
the compact matrix-row renderer; this keeps source/assertion counts and direct
pairs visible without repeating full contract provenance.
Roadmap items may carry an explicit ``planned`` source-slice pointer; that
pointer is a bounded future test contract and is not counted as executable
evidence until its Catch2 declaration is added.
``check`` validates the live roadmap, plan, requirements, and source index by
default; the append-only completion ledger is a separate opt-in audit via
``check --historical`` so historical source splits do not block current work.
For every mapped Catch2 row it also verifies that each selected requirement's
canonical 2025 ``document:clause`` subsection is present in the derived
standard-section list, so requirement-to-subsection drift fails locally.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Iterable


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INDEX = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP-INDEX.json"
DEFAULT_PLAN = (
    REPOSITORY_ROOT
    / "compliance"
    / "requirements-lab"
    / "catch2-test-plan.json"
)
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_CONTRACT_DIRECTORY = REPOSITORY_ROOT / "compliance" / "requirements-lab"
DEFAULT_LAB_ISSUES = DEFAULT_CONTRACT_DIRECTORY / "known-issues.json"
DEFAULT_ROADMAP = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP.md"
DEFAULT_IMPLEMENTATION_PLAN = REPOSITORY_ROOT / "docs" / "planning" / "IMPLEMENTATION-PLAN.md"
DEFAULT_TEST_ROOT = REPOSITORY_ROOT / "cpp" / "tests"


_CATCH2_TEST_CASE = re.compile(
    r'\bTEST_CASE(?:_[A-Za-z0-9_]+)?\s*\(\s*"((?:\\.|[^"\\])*)"',
    re.MULTILINE,
)
_CPP_CONDITIONAL_DIRECTIVE = re.compile(
    r"^\s*#\s*(if|ifdef|ifndef|endif)\b"
)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON file {path}: {error}") from error
    if not isinstance(value, dict):
        raise ValueError(f"JSON root must be an object: {path}")
    return value


def strings(value: Any) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, Iterable) and not isinstance(value, (bytes, dict)):
        return [item for item in value if isinstance(item, str)]
    return []


def compact_prose(value: Any, limit: int = 240) -> str:
    """Collapse roadmap prose so compact reports stay bounded."""

    if not isinstance(value, str):
        return ""
    collapsed = " ".join(value.split())
    if len(collapsed) <= limit:
        return collapsed
    return f"{collapsed[: max(limit - 3, 1)].rstrip()}..."


def text_gap_preview(option: dict[str, Any], prefix: str = "  ") -> list[str]:
    """Render the bounded uncovered-requirement handoff for one family row.

    ``ready``/``next``/``resume`` all use the same family-choice projection.
    Keep the human-readable form as small as the JSON form: one coverage-head
    requirement in canonical corpus order, its 2025 subsection, and copyable
    reverse-lookups.  This is a bounded inventory pointer, not semantic
    prioritization; the complete gap inventory remains behind ``gaps``.
    """

    preview = option.get("gap_preview")
    if not isinstance(preview, dict):
        return []
    requirement = preview.get("requirement")
    if not isinstance(requirement, dict):
        return []
    requirement_id = requirement.get("id") or "<unidentified>"
    standard_section = requirement.get("standard_section") or "<unresolved>"
    covered = preview.get("covered_requirement_count")
    total = preview.get("total_requirement_count")
    uncovered = preview.get("uncovered_requirement_count")
    coverage_percent = preview.get("coverage_percent")
    if isinstance(covered, int) and isinstance(total, int):
        coverage_text = (
            f"{covered}/{total} covered"
            + (
                f" ({coverage_percent:.2f}%)"
                if isinstance(coverage_percent, (int, float))
                else ""
            )
        )
        if isinstance(uncovered, int):
            coverage_text += f"; {uncovered} uncovered"
        lines = [f"{prefix}gap_scope={coverage_text}"]
    else:
        lines = []
    lines.append(
        f"{prefix}gap_head={requirement_id} -> {standard_section} "
        "(canonical corpus order)"
    )
    requirement_query = requirement.get("requirement_query")
    if isinstance(requirement_query, str) and requirement_query:
        lines.append(f"{prefix}gap_requirement={requirement_query}")
    section_query = requirement.get("section_query")
    if isinstance(section_query, str) and section_query:
        lines.append(f"{prefix}gap_section={section_query}")
    statement = compact_prose(requirement.get("statement"), 240)
    if statement:
        lines.append(f"{prefix}gap_statement={statement}")
    return lines


def unlocated_display(row: dict[str, Any]) -> str:
    """Render diagnostic and actionable source-unlocated counts together."""

    total = row.get("unlocated_case_count", 0)
    actionable = row.get("unlocated_actionable_case_count", total)
    if isinstance(total, int) and isinstance(actionable, int) and actionable != total:
        return f"{total} (actionable={actionable})"
    return str(total)


NON_EXECUTABLE_SOURCE_STATUSES = frozenset(
    {
        "disabled-source-artifact",
        "source-missing-needs-reconciliation",
    }
)


def is_planned_test(test: dict[str, Any]) -> bool:
    """Return whether a plan row is an intentional future implementation slice.

    Planned rows are allowed to precede their C++ ``TEST_CASE`` declaration so
    the roadmap can carry the exact title, lane, and mapping before source
    work begins.  They are not source-reconciliation defects and must remain
    visible to the planned-row queue.
    """

    return str(test.get("status") or "").casefold() == "planned"


def is_non_executable_source_status(test: dict[str, Any]) -> bool:
    """Return whether a source-unlocated row is diagnostic-only.

    Disabled historical declarations and rows explicitly awaiting source
    reconciliation remain visible in counts, but neither should become the
    next runnable handoff or an implementation candidate.
    """

    return str(test.get("status") or "") in NON_EXECUTABLE_SOURCE_STATUSES


def lane_ctest_command(handles: Any) -> str | None:
    """Return a copyable bounded CTest command for a configured lane."""

    if not isinstance(handles, dict):
        return None
    ctest_filter = handles.get("ctest_filter")
    if isinstance(ctest_filter, str) and ctest_filter.strip():
        return (
            'ctest --test-dir <build-dir> -C Debug -R '
            f'"{ctest_filter}" --output-on-failure'
        )
    ctest_label = handles.get("ctest_label")
    if isinstance(ctest_label, str) and ctest_label.strip():
        return (
            'ctest --test-dir <build-dir> -C Debug -L '
            f'"^{ctest_label}$" --output-on-failure'
        )
    return None


def indexed_lane_assertion_count(index: dict[str, Any], lane: str) -> int | None:
    """Return the verified aggregate assertion total for one exact lane.

    Focused JUnit lanes may cover more source declarations than the individual
    plan rows can record (for example, legacy rows with no per-case assertion
    count).  The roadmap index is the authoritative aggregate in that case.
    Keep the lookup case-insensitive so ``lanes`` and ``focus`` report the same
    total for an exact tag regardless of its spelling.
    """

    mapping = index.get("mapping", {})
    if not isinstance(mapping, dict):
        return None
    counts = mapping.get("lane_assertion_counts", {})
    if not isinstance(counts, dict):
        return None
    value = counts.get(lane)
    if isinstance(value, int):
        return value
    folded = lane.casefold()
    for name, candidate in counts.items():
        if str(name).casefold() == folded and isinstance(candidate, int):
            return candidate
    return None


def item_handles(item: dict[str, Any], compact: bool = False) -> list[str]:
    """Return explicit next-step handles recorded in the roadmap index.

    Compact reports keep the actionable lane/test/ctest handles but replace
    potentially large standard-section and plan-id lists with bounded counts.
    The full lists remain available through the default text and JSON views.
    """

    handles: list[str] = []
    if item.get("next_work_id"):
        handles.append(f"work={item['next_work_id']}")
    if item.get("next_work_status"):
        handles.append(f"work_status={item['next_work_status']}")
    if item.get("next_work_query"):
        handles.append(
            f"work_query={compact_prose(item['next_work_query'], 180)}"
        )
    if item.get("next_task"):
        handles.append(f"next_task={compact_prose(item['next_task'], 180)}")
    if item.get("next_lane"):
        handles.append(f"lane={item['next_lane']}")
        handles.append(
            "focus_query="
            f"python tools/query_rti_work.py focus {item['next_lane']} --summary --compact"
        )
    if item.get("next_test_query"):
        role = item.get("next_test_role", "planned")
        label = "baseline_test" if role == "baseline" else "test"
        handles.append(f"{label}={item['next_test_query']}")
    if item.get("next_source_state"):
        handles.append(f"source_state={item['next_source_state']}")
    if item.get("next_source_test_query"):
        handles.append(f"source_test={item['next_source_test_query']}")
    if item.get("next_source_location"):
        handles.append(f"source_location={item['next_source_location']}")
    if item.get("next_source_lane"):
        handles.append(f"source_lane={item['next_source_lane']}")
    source_requirement_ids = strings(item.get("next_source_requirement_ids"))
    if source_requirement_ids:
        handles.append(
            f"source_requirement_count={len(source_requirement_ids)}"
            if compact
            else f"source_requirements={','.join(source_requirement_ids)}"
        )
    source_standard_sections = strings(item.get("next_source_standard_sections"))
    if source_standard_sections:
        handles.append(
            f"source_standard_count={len(source_standard_sections)}"
            if compact
            else f"source_standard={','.join(source_standard_sections)}"
        )
    source_api_surfaces = strings(item.get("next_source_api_surfaces"))
    if source_api_surfaces:
        handles.append(
            f"source_api_count={len(source_api_surfaces)}"
            if compact
            else f"source_api={','.join(source_api_surfaces)}"
        )
    if item.get("next_source_ctest_filter"):
        handles.append(f"source_ctest={item['next_source_ctest_filter']}")
    if item.get("next_ctest_filter"):
        handles.append(f"ctest={item['next_ctest_filter']}")
    if item.get("next_package_target"):
        handles.append(f"package_target={item['next_package_target']}")
    if item.get("next_package_ctest_filter"):
        handles.append(f"package_ctest={item['next_package_ctest_filter']}")
    if item.get("next_package_manifest"):
        handles.append(f"package_manifest={item['next_package_manifest']}")
    if item.get("next_junit_target"):
        handles.append(f"junit_target={item['next_junit_target']}")
    if item.get("next_junit_artifact"):
        handles.append(f"junit_artifact={item['next_junit_artifact']}")
    if item.get("next_process_probe_target"):
        handles.append(f"process_probe_target={item['next_process_probe_target']}")
    if item.get("next_process_package_target"):
        handles.append(f"process_package_target={item['next_process_package_target']}")
    if item.get("next_process_package_test"):
        handles.append(f"process_package_test={item['next_process_package_test']}")
    if item.get("next_process_package_ctest_filter"):
        handles.append(
            f"process_package_ctest={item['next_process_package_ctest_filter']}"
        )
    if item.get("next_process_package_timestamped_test"):
        handles.append(
            "process_package_timestamped_test="
            f"{item['next_process_package_timestamped_test']}"
        )
    if item.get("next_process_package_timestamped_ctest_filter"):
        handles.append(
            "process_package_timestamped_ctest="
            f"{item['next_process_package_timestamped_ctest_filter']}"
        )
    if item.get("next_process_package_parameterized_test"):
        handles.append(
            "process_package_parameterized_test="
            f"{item['next_process_package_parameterized_test']}"
        )
    if item.get("next_process_package_parameterized_ctest_filter"):
        handles.append(
            "process_package_parameterized_ctest="
            f"{item['next_process_package_parameterized_ctest_filter']}"
        )
    if item.get("next_process_package_connection_loss_test"):
        handles.append(
            "process_package_connection_loss_test="
            f"{item['next_process_package_connection_loss_test']}"
        )
    if item.get("next_process_package_connection_loss_ctest_filter"):
        handles.append(
            "process_package_connection_loss_ctest="
            f"{item['next_process_package_connection_loss_ctest_filter']}"
        )
    if item.get("next_process_package_object_registration_test"):
        handles.append(
            "process_package_object_registration_test="
            f"{item['next_process_package_object_registration_test']}"
        )
    if item.get("next_process_package_object_registration_ctest_filter"):
        handles.append(
            "process_package_object_registration_ctest="
            f"{item['next_process_package_object_registration_ctest_filter']}"
        )
    if item.get("next_process_package_named_registration_test"):
        handles.append(
            "process_package_named_registration_test="
            f"{item['next_process_package_named_registration_test']}"
        )
    if item.get("next_process_package_named_registration_ctest_filter"):
        handles.append(
            "process_package_named_registration_ctest="
            f"{item['next_process_package_named_registration_ctest_filter']}"
        )
    if item.get("next_process_package_attribute_update_test"):
        handles.append(
            "process_package_attribute_update_test="
            f"{item['next_process_package_attribute_update_test']}"
        )
    if item.get("next_process_package_attribute_update_ctest_filter"):
        handles.append(
            "process_package_attribute_update_ctest="
            f"{item['next_process_package_attribute_update_ctest_filter']}"
        )
    if item.get("next_process_package_directed_retraction_test"):
        handles.append(
            "process_package_directed_retraction_test="
            f"{item['next_process_package_directed_retraction_test']}"
        )
    if item.get("next_process_package_directed_retraction_ctest_filter"):
        handles.append(
            "process_package_directed_retraction_ctest="
            f"{item['next_process_package_directed_retraction_ctest_filter']}"
        )
    if item.get("next_process_package_catalog_verifier"):
        handles.append(
            "process_package_catalog_verifier="
            f"{item['next_process_package_catalog_verifier']}"
        )
    sections = strings(item.get("next_standard_sections"))
    if sections:
        handles.append(
            f"standard_count={len(sections)}"
            if compact
            else f"standard={','.join(sections)}"
        )
    plan_ids = strings(item.get("next_plan_ids"))
    if plan_ids:
        handles.append(
            f"plan_id_count={len(plan_ids)}"
            if compact
            else f"plan={','.join(plan_ids)}"
        )
    catalog_gap_ids = strings(item.get("catalog_gap_plan_ids"))
    if catalog_gap_ids:
        handles.append(
            f"catalog_gap_count={len(catalog_gap_ids)}"
            if compact
            else f"catalog_gaps={','.join(catalog_gap_ids)}"
        )
    return handles


def search_key(value: Any) -> str:
    """Fold section punctuation and separators for forgiving human queries."""

    return re.sub(r"[^a-z0-9]+", "", str(value).casefold())


def _issue_search_values(value: Any) -> Iterable[str]:
    """Yield scalar values from one issue record for bounded lookup."""

    if isinstance(value, dict):
        for key, nested in value.items():
            yield str(key)
            yield from _issue_search_values(nested)
    elif isinstance(value, (list, tuple, set, frozenset)):
        for nested in value:
            yield from _issue_search_values(nested)
    elif value is not None:
        yield str(value)


def lab_issues_snapshot(
    path: Path = DEFAULT_LAB_ISSUES,
    *,
    query: str | None = None,
    status: str = "all",
    limit: int = 20,
) -> dict[str, Any]:
    """Return a small, read-only Requirements Lab issue ledger view.

    The ledger is intentionally separate from the pinned Lab export.  This
    command gives contributors a cheap way to find known extraction/usability
    defects without reopening or rescanning the sibling Requirements Lab.
    """

    ledger = load_json(path)
    raw_issues = ledger.get("issues", [])
    if not isinstance(raw_issues, list) or not all(
        isinstance(issue, dict) for issue in raw_issues
    ):
        raise ValueError(f"Requirements Lab issue ledger has invalid issues: {path}")
    normalized_query = search_key(query) if query else ""
    selected: list[dict[str, Any]] = []
    for issue in raw_issues:
        issue_status = str(issue.get("status") or "unknown")
        if status != "all" and issue_status.casefold() != status.casefold():
            continue
        if normalized_query:
            haystack = " ".join(_issue_search_values(issue))
            if normalized_query not in search_key(haystack):
                continue
        selected.append(issue)
    selected.sort(key=lambda issue: str(issue.get("id") or ""))
    shown = selected if limit == 0 else selected[: max(limit, 0)]
    return {
        "path": relative_path(path),
        "schema_version": ledger.get("schema_version"),
        "export_policy": ledger.get("export_policy"),
        "query": query,
        "status": status,
        "count": len(selected),
        "shown_count": len(shown),
        "issues": shown,
    }


def relative_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPOSITORY_ROOT.resolve()).as_posix()
    except ValueError:
        return str(path)


def decode_cpp_string(value: str) -> str:
    """Decode the small escape subset used in Catch2 test names."""

    return re.sub(r'\\(["\\])', r"\1", value)


def load_test_source_locations(
    root: Path,
    *,
    include_health: bool = False,
) -> dict[str, list[dict[str, Any]]] | tuple[
    dict[str, list[dict[str, Any]]], dict[str, Any]
]:
    """Index Catch2 test names to their C++ source locations.

    The test plan remains the traceability source of truth.  This bounded
    derived index only makes the plan actionable by locating the corresponding
    ``TEST_CASE`` declaration, and is intentionally rebuilt per query so it
    cannot become a second, stale plan.
    """

    if not root.is_dir():
        raise ValueError(f"test source root is not a directory: {root}")
    locations: dict[str, list[dict[str, Any]]] = defaultdict(list)
    health: dict[str, Any] = {
        "status": "ok",
        "files_scanned": 0,
        "files_with_control_characters": 0,
        "control_character_count": 0,
        "control_character_samples": [],
        "files_with_unbalanced_conditionals": 0,
        "unbalanced_conditional_count": 0,
        "unbalanced_conditional_samples": [],
    }
    for path in sorted(root.rglob("*.cpp")):
        try:
            source = path.read_text(encoding="utf-8")
        except OSError as error:
            raise ValueError(f"cannot read test source {path}: {error}") from error
        health["files_scanned"] += 1
        conditional_depth = 0
        unbalanced_conditionals: list[int] = []
        for line_number, line in enumerate(source.splitlines(), 1):
            directive = _CPP_CONDITIONAL_DIRECTIVE.match(line)
            if not directive:
                continue
            if directive.group(1) == "endif":
                if conditional_depth == 0:
                    unbalanced_conditionals.append(line_number)
                else:
                    conditional_depth -= 1
            else:
                conditional_depth += 1
        if conditional_depth or unbalanced_conditionals:
            health["status"] = "attention"
            health["files_with_unbalanced_conditionals"] += 1
            imbalance_count = conditional_depth + len(unbalanced_conditionals)
            health["unbalanced_conditional_count"] += imbalance_count
            samples = health["unbalanced_conditional_samples"]
            if len(samples) < 8:
                samples.append(
                    {
                        "path": relative_path(path),
                        "open_conditionals": conditional_depth,
                        "unmatched_endif_lines": unbalanced_conditionals[:8],
                    }
                )
        controls = [
            (offset, character)
            for offset, character in enumerate(source)
            if ord(character) < 0x20 and character not in "\t\r\n"
        ]
        if controls:
            health["status"] = "attention"
            health["files_with_control_characters"] += 1
            health["control_character_count"] += len(controls)
            samples = health["control_character_samples"]
            for offset, character in controls:
                if len(samples) >= 8:
                    break
                samples.append(
                    {
                        "path": relative_path(path),
                        "line": source.count("\n", 0, offset) + 1,
                        "codepoint": f"U+{ord(character):04X}",
                    }
                )
        for match in _CATCH2_TEST_CASE.finditer(source):
            name = decode_cpp_string(match.group(1))
            line = source.count("\n", 0, match.start()) + 1
            locations[name].append(
                {
                    "path": relative_path(path),
                    "line": line,
                }
            )
    if health["status"] == "attention":
        hints: list[str] = []
        if health["files_with_control_characters"]:
            hints.append(
                "inspect git diff --check for control-byte damage"
            )
        if health["files_with_unbalanced_conditionals"]:
            hints.append(
                "inspect unmatched C++ preprocessor conditionals before trusting "
                "aggregate TEST_CASE locations"
            )
        health["hint"] = (
            "Treat source-unlocated rows as source reconciliation, not Lab drift; "
            + "; ".join(hints)
            + "."
        )
    else:
        health["hint"] = "The C++ source index contains no control-byte damage."
    result = dict(locations)
    if include_health:
        return result, health
    return result


def source_location_text(test: dict[str, Any]) -> str:
    locations = test.get("source_locations")
    if not isinstance(locations, list) or not locations:
        return "<not found>"
    values = []
    for location in locations:
        if not isinstance(location, dict):
            continue
        path = location.get("path")
        line = location.get("line")
        if isinstance(path, str) and line is not None:
            values.append(f"{path}:{line}")
        elif isinstance(path, str):
            values.append(path)
    return ", ".join(values) or "<not found>"


def preferred_source_locations(
    test: dict[str, Any],
    locations: list[dict[str, Any]],
) -> list[dict[str, Any]]:
    """Prefer the plan's recorded source when duplicate TEST_CASE titles exist.

    Focused extractions may temporarily retain the same TEST_CASE title in a
    historical aggregate translation unit.  The explicit plan pointer is the
    authoritative executable source in that situation; fall back to all
    derived declarations when the pointer is absent or stale so integrity
    checks can still report the mismatch.
    """

    recorded = test.get("source_location")
    if not isinstance(recorded, str) or not recorded.strip():
        return locations
    folded_recorded = recorded.replace("\\", "/").casefold()
    preferred = [
        location
        for location in locations
        if isinstance(location, dict)
        and f"{location.get('path', '')}:{location.get('line', '')}".replace(
            "\\", "/"
        ).casefold()
        == folded_recorded
    ]
    return preferred or locations


def source_location_sort_key(test: dict[str, Any]) -> tuple[str, int, str]:
    """Sort source-backed records by path and numeric declaration line.

    Source locations are rendered as ``path:line`` strings for humans, but
    sorting those strings makes line 51392 appear before line 5461.  Queue
    selection must follow the actual declaration order so the roadmap pointer
    can advance one source case at a time without another repository scan.
    """

    locations = [
        location
        for location in test.get("source_locations", [])
        if isinstance(location, dict)
    ]
    first = locations[0] if locations else {}
    path = str(first.get("path", ""))
    line = first.get("line")
    return (
        path.casefold(),
        line if isinstance(line, int) else 0,
        str(test.get("test_case") or test.get("id") or ""),
    )


def source_pointer_state(item: dict[str, Any]) -> str:
    """Normalize an indexed source pointer for bounded status queries.

    Older roadmap rows represented an exhausted queue with null pointer fields.
    Keep that representation valid in the JSON index, but expose a useful
    state to callers instead of printing Python's ``None``.
    """

    state = item.get("next_source_state")
    if isinstance(state, str) and state.strip():
        return state
    if item.get("next_source_test_query"):
        return "unplanned-source"
    if item.get("next_source_lane"):
        return "exhausted"
    return "none"


def live_next_action(item: dict[str, Any]) -> str | None:
    """Return an actionable next-step note without stale source prose."""

    pointer_state = source_pointer_state(item)
    if pointer_state == "unplanned-source":
        query = item.get("next_source_test_query")
        location = item.get("next_source_location") or "<unlocated>"
        if isinstance(query, str) and query.strip():
            return (
                "Add an explicit Catch2 plan row for the indexed source declaration "
                f"{query} ({location}), then run ready/unplanned and its focused "
                "mapping check before treating it as evidence."
            )
    if pointer_state == "exhausted":
        lane = item.get("next_source_lane") or "<unspecified>"
        return (
            f"The indexed source queue for lane {lane} is exhausted. "
            "Select the next bounded service family with queue, work, or focus; "
            "do not rescan the unchanged Requirements Lab."
        )
    value = item.get("next_action")
    return value if isinstance(value, str) and value.strip() else None


def queue_action_state(
    item: dict[str, Any],
    *,
    candidate_count: int,
    unclassified_count: int,
    actionable_source_drift_count: int,
    lane_state: str | None = None,
) -> str:
    """Classify the next *kind* of work a roadmap family can accept.

    ``indexed_work_queue`` historically exposed only a coarse state such as
    ``new-case-needed``.  That made a fully implemented family with no queued
    source work look like the next implementation target.  Keep the coarse
    state for compatibility, but publish this separate, deterministic action
    classification so resume callers can distinguish runnable work from an
    evidence-complete family and from an external review handoff.

    The classification is derived from the already indexed plan/source join;
    it never searches the Requirements Lab or infers a new requirement.
    """

    # A completed lane remains evidence-complete even when its roadmap family
    # has a different planned companion queued next.  The lane state is the
    # narrower scope; only a planned lane should inherit the planned action.
    if lane_state == "complete-pointer":
        return "evidence-complete"
    if item.get("next_test_role") == "planned" and lane_state in {
        None,
        "planned",
    }:
        return "planned"
    if candidate_count:
        return "implementation"
    if unclassified_count:
        return "mapping"
    if actionable_source_drift_count:
        return "source-reconciliation"

    action = str(item.get("next_action") or "").casefold()
    if action.lstrip().startswith("submit ") or "submit the existing raw" in action[:240]:
        return "external-review"
    if lane_state == "complete-pointer":
        return "evidence-complete"
    if "keep this item open for newly identified evidence only" in action:
        return "evidence-complete"
    if "queues are exhausted" in action or "queue is exhausted" in action:
        return "evidence-complete"
    return "new-case-needed"


def source_locations_for_test(
    test_case: Any,
    source_locations: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    """Resolve a plan entry, including entries grouping cases with semicolons."""

    if not isinstance(test_case, str):
        return []
    candidates = [test_case]
    candidates.extend(part.strip() for part in test_case.split(";") if part.strip())
    resolved: list[dict[str, Any]] = []
    seen: set[tuple[Any, Any]] = set()
    for candidate in candidates:
        for location in source_locations.get(candidate, []):
            if not isinstance(location, dict):
                continue
            key = (location.get("path"), location.get("line"))
            if key in seen:
                continue
            seen.add(key)
            resolved.append(location)
    return resolved


def tests_for_source_path(
    tests: list[dict[str, Any]],
    path_query: str,
) -> list[dict[str, Any]]:
    """Return planned tests whose derived C++ declaration is under a path.

    This is intentionally narrower than ``search``: it only inspects derived
    source locations and keeps the result ordered by file/line so a known test
    translation unit becomes a bounded traceability view.
    """

    folded_query = path_query.replace("\\", "/").casefold()
    selected = [
        test
        for test in tests
        if any(
            folded_query
            in str(location.get("path", "")).replace("\\", "/").casefold()
            for location in test.get("source_locations", [])
            if isinstance(location, dict)
        )
    ]

    def sort_key(test: dict[str, Any]) -> tuple[str, int, str]:
        locations = [
            location
            for location in test.get("source_locations", [])
            if isinstance(location, dict)
        ]
        first = locations[0] if locations else {}
        path = str(first.get("path", ""))
        line = first.get("line")
        return (
            path,
            line if isinstance(line, int) else 0,
            str(test.get("id") or ""),
        )

    return sorted(selected, key=sort_key)


def unplanned_source_cases(
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]],
    source_path_query: str | None = None,
) -> list[dict[str, Any]]:
    """Return source TEST_CASEs that have no exact Catch2 plan row.

    The Catch2 plan remains the traceability source of truth.  This derived
    view is intentionally one-way: it does not infer requirements or status
    for a source declaration.  It only exposes a bounded reconciliation queue
    so a new case can be mapped deliberately instead of found by a repository-
    wide text search.
    """

    planned_names: set[str] = set()
    for test in tests:
        test_case = test.get("test_case")
        if not isinstance(test_case, str):
            continue
        planned_names.add(test_case)
        planned_names.update(
            part.strip() for part in test_case.split(";") if part.strip()
        )
    folded_path = source_path_query.casefold() if source_path_query else None
    records: list[dict[str, Any]] = []
    for test_case, locations in source_locations.items():
        if test_case in planned_names:
            continue
        if folded_path is not None and not any(
            folded_path in str(location.get("path", "")).casefold()
            for location in locations
            if isinstance(location, dict)
        ):
            continue
        records.append(
            {
                "test_case": test_case,
                "source_locations": locations,
                "source_state": "unplanned-source",
            }
        )
    records.sort(key=source_location_sort_key)
    return records


def unplanned_source_summary(
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]],
) -> dict[str, Any]:
    """Return only the global source-queue count and deterministic head.

    Work-selection output should expose the next unplanned declaration when a
    narrower indexed queue is exhausted, but it must not print or retain the
    whole source catalog.  The full bounded queue remains available through
    ``unplanned``.
    """

    queue = unplanned_source_cases(tests, source_locations)
    result: dict[str, Any] = {"count": len(queue)}
    if queue:
        head = queue[0]
        result.update(
            {
                "test_query": head.get("test_case"),
                "source_location": source_location_text(head),
                "command": (
                    "python tools/query_rti_work.py unplanned "
                    "--summary --compact --limit 1"
                ),
            }
        )
    return result


def load_standard_requirements(bundle: dict[str, Any]) -> dict[str, dict[str, Any]]:
    requirements: dict[str, dict[str, Any]] = {}
    for document in bundle.get("documents", []):
        if not isinstance(document, dict):
            continue
        document_id = document.get("document_id")
        if not isinstance(document_id, str) or not document_id.endswith("-2025"):
            continue
        for requirement in document.get("requirements", []):
            if not isinstance(requirement, dict):
                continue
            requirement_id = requirement.get("id")
            if not isinstance(requirement_id, str):
                continue
            requirements[requirement_id] = {
                "id": requirement_id,
                "document_id": document_id,
                "ordinal": requirement.get("ordinal"),
                "clause": requirement.get("clause"),
                "clause_id": requirement.get("clause_id"),
                "title": requirement.get("title"),
                "statement": requirement.get("statement"),
                "machine_statement": requirement.get("machine_statement"),
                "kind": requirement.get("kind"),
                "coverage_kind": requirement.get("coverage_kind"),
                "semantic_group_id": requirement.get("semantic_group_id"),
                "transition_ids": strings(requirement.get("transition_ids")),
                "source": requirement.get("source"),
            }
    return requirements


def requirement_gap_inventory(
    standard_requirements: dict[str, dict[str, Any]],
    tests: list[dict[str, Any]],
    *,
    index: dict[str, Any] | None = None,
    query: str | None = None,
    document: str | None = None,
    clause: str | None = None,
    coverage_kind: str | None = None,
    family: str | None = None,
    limit: int = 20,
) -> dict[str, Any]:
    """Return a bounded inventory of uncovered pinned 2025 requirements.

    The checked-in Catch2 plan is the only source of coverage here.  A
    requirement is considered covered when at least one current plan row
    selects its exact Lab id, regardless of whether that row belongs to an
    embedded or process target.  When ``family`` is supplied, only rows
    linked to that exact roadmap family participate in the coverage count.
    This is deliberately a planning diagnostic: it never infers a new mapping
    or edits the Requirements Lab.
    """

    scoped_tests = tests
    if family:
        family_folded = family.casefold()
        scoped_tests = [
            test
            for test in tests
            if any(
                isinstance(item, dict)
                and str(item.get("id") or "").casefold() == family_folded
                for item in roadmap_links_for_test(index or {}, test, limit=0)
            )
        ]

    covered_ids = {
        requirement_id
        for test in scoped_tests
        for requirement_id in strings(test.get("lab_requirement_ids"))
        if requirement_id in standard_requirements
    }

    def matches(requirement: dict[str, Any]) -> bool:
        document_id = str(requirement.get("document_id") or "")
        clause_id = str(requirement.get("clause_id") or "")
        if document and document.casefold() not in document_id.casefold():
            return False
        if clause and clause.casefold() not in clause_id.casefold() and clause.casefold() not in str(
            requirement.get("clause") or ""
        ).casefold():
            return False
        if coverage_kind and str(requirement.get("coverage_kind") or "").casefold() != coverage_kind.casefold():
            return False
        if not query:
            return True
        values = (
            requirement.get("id"),
            requirement.get("title"),
            requirement.get("statement"),
            requirement.get("machine_statement"),
            requirement.get("document_id"),
            requirement.get("clause"),
            requirement.get("clause_id"),
            f"{document_id}:{clause_id}" if document_id and clause_id else None,
            requirement.get("kind"),
            requirement.get("coverage_kind"),
            requirement.get("semantic_group_id"),
            *strings(requirement.get("transition_ids")),
            requirement.get("source", {}).get("path")
            if isinstance(requirement.get("source"), dict)
            else None,
        )
        folded = query.casefold()
        return any(
            isinstance(value, str) and folded in value.casefold()
            for value in values
        )

    filtered = [
        requirement
        for requirement in standard_requirements.values()
        if matches(requirement)
    ]
    uncovered = [
        requirement
        for requirement in filtered
        if requirement.get("id") not in covered_ids
    ]
    uncovered.sort(
        key=lambda requirement: (
            str(requirement.get("document_id") or ""),
            str(requirement.get("clause_id") or ""),
            int(requirement.get("ordinal") or 0),
            str(requirement.get("id") or ""),
        )
    )

    grouped: defaultdict[str, dict[str, Any]] = defaultdict(
        lambda: {
            "standard_section": None,
            "document_id": None,
            "clause_id": None,
            "total_requirement_count": 0,
            "covered_requirement_count": 0,
            "uncovered_requirement_count": 0,
            "uncovered_requirement_ids": [],
        }
    )
    for requirement in filtered:
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        if not isinstance(document_id, str) or not isinstance(clause_id, str):
            continue
        key = f"{document_id}:{clause_id}"
        row = grouped[key]
        row["standard_section"] = key
        row["document_id"] = document_id
        row["clause_id"] = clause_id
        row["total_requirement_count"] += 1
        if requirement.get("id") in covered_ids:
            row["covered_requirement_count"] += 1
        else:
            row["uncovered_requirement_count"] += 1
            if len(row["uncovered_requirement_ids"]) < 8:
                row["uncovered_requirement_ids"].append(requirement.get("id"))
    groups = sorted(
        (
            row
            for row in grouped.values()
            if row["uncovered_requirement_count"]
        ),
        key=lambda row: (
            -row["uncovered_requirement_count"],
            str(row.get("standard_section") or ""),
        ),
    )
    requested_limit = max(limit, 0)
    shown_requirements = (
        uncovered if requested_limit == 0 else uncovered[:requested_limit]
    )
    group_limit = 20 if requested_limit == 0 else max(1, min(20, requested_limit))
    shown_groups = groups[:group_limit]

    def requirement_record(requirement: dict[str, Any]) -> dict[str, Any]:
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        identifier = requirement.get("id")
        source = requirement.get("source")
        return {
            "id": identifier,
            "coverage": "uncovered",
            "document_id": document_id,
            "clause": requirement.get("clause"),
            "clause_id": clause_id,
            "standard_section": (
                f"{document_id}:{clause_id}"
                if document_id and clause_id
                else None
            ),
            "title": requirement.get("title"),
            "statement": requirement.get("statement"),
            "kind": requirement.get("kind"),
            "coverage_kind": requirement.get("coverage_kind"),
            "semantic_group_id": requirement.get("semantic_group_id"),
            "transition_ids": strings(requirement.get("transition_ids")),
            "source": source,
            "requirement_query": (
                f"python tools/query_rti_work.py requirement {identifier} "
                "--summary --compact"
                if isinstance(identifier, str)
                else None
            ),
        }

    total_count = len(filtered)
    covered_count = sum(
        requirement.get("id") in covered_ids for requirement in filtered
    )
    return {
        "query": query,
        "document": document,
        "clause": clause,
        "coverage_kind": coverage_kind,
        "family": family,
        "total_requirement_count": total_count,
        "covered_requirement_count": covered_count,
        "uncovered_requirement_count": len(uncovered),
        "coverage_percent": (
            round((covered_count / total_count) * 100, 2) if total_count else 0.0
        ),
        "group_count": len(groups),
        "shown_group_count": len(shown_groups),
        "shown_requirement_count": len(shown_requirements),
        "groups": shown_groups,
        "requirements": [requirement_record(requirement) for requirement in shown_requirements],
    }


def requirement_gap_summary_data(result: dict[str, Any]) -> dict[str, Any]:
    """Return the bounded gap-card view without full requirement records.

    Subsection aggregates are enough to select the next exact query.  Keeping
    the requirement records opt-in is important for JSON consumers too:
    ``gaps --summary --limit 0`` must not expand into a context-sized dump.
    """

    summary = dict(result)
    summary["requirements"] = []
    summary["shown_requirement_count"] = 0
    return summary


def requirement_gap_lookup_summary_data(
    result: dict[str, Any],
    *,
    preview_limit: int = 8,
) -> dict[str, Any]:
    """Return a bounded requirement-lookup preview for an uncovered query.

    ``gaps --summary`` intentionally stays at subsection aggregates.  An
    exact ``requirement`` lookup is different: when no Catch2 row is mapped,
    the caller needs one small normative record to decide what C++ case to
    add.  Keep that bridge bounded even when the caller used ``--limit 0``.
    """

    summary = requirement_gap_summary_data(result)
    records = result.get("requirements", [])
    if not isinstance(records, list):
        records = []
    bounded_limit = max(preview_limit, 0)
    selected = records if bounded_limit == 0 else records[:bounded_limit]
    preview: list[dict[str, Any]] = []
    for record in selected:
        if not isinstance(record, dict):
            continue
        bounded_record = dict(record)
        bounded_record["title"] = compact_prose(record.get("title"), 160)
        bounded_record["statement"] = compact_prose(record.get("statement"), 360)
        preview.append(bounded_record)
    summary["requirements"] = preview
    summary["shown_requirement_count"] = len(preview)
    summary["requirement_preview_limit"] = bounded_limit
    summary["requirement_preview_truncated"] = len(records) > len(selected)
    return summary


def family_gap_preview(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    standard_requirements: dict[str, dict[str, Any]] | None,
    family: str | None,
) -> dict[str, Any] | None:
    """Return one bounded uncovered-requirement handoff for a roadmap family.

    Family queue rows deliberately avoid printing the full 2025 gap inventory.
    When a family is explicitly marked ``new-case-needed``, however, a
    contributor should not have to issue a second broad search just to obtain
    a bounded coverage head.  This helper joins the already-loaded plan and
    pinned corpus only; it never reads or mutates the Requirements Lab.
    """

    if not isinstance(standard_requirements, dict) or not isinstance(family, str):
        return None
    if not family.strip():
        return None
    gaps = requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family=family,
        limit=1,
    )
    records = gaps.get("requirements")
    if not isinstance(records, list) or not records:
        return {
            "family": family,
            "total_requirement_count": gaps.get("total_requirement_count", 0),
            "covered_requirement_count": gaps.get("covered_requirement_count", 0),
            "uncovered_requirement_count": gaps.get("uncovered_requirement_count", 0),
            "coverage_percent": gaps.get("coverage_percent", 0.0),
            "requirement": None,
        }
    record = records[0]
    if not isinstance(record, dict):
        return None
    requirement_id = record.get("id")
    standard_section = record.get("standard_section")
    clause_id = record.get("clause_id")
    return {
        "family": family,
        "total_requirement_count": gaps.get("total_requirement_count", 0),
        "covered_requirement_count": gaps.get("covered_requirement_count", 0),
        "uncovered_requirement_count": gaps.get("uncovered_requirement_count", 0),
        "coverage_percent": gaps.get("coverage_percent", 0.0),
        "requirement": {
            "id": requirement_id,
            "standard_section": standard_section,
            "clause_id": clause_id,
            "title": compact_prose(record.get("title"), 160),
            "statement": compact_prose(record.get("statement"), 360),
            "requirement_query": record.get("requirement_query"),
            "section_query": (
                f"python tools/query_rti_work.py section {standard_section} "
                "--summary --compact"
                if isinstance(standard_section, str) and standard_section
                else None
            ),
        },
    }


def load_contract_links(directory: Path) -> dict[str, list[dict[str, Any]]]:
    links: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for path in sorted(directory.glob("*-requirements-contract.json")):
        try:
            contract = load_json(path)
        except ValueError:
            continue
        for requirement in contract.get("requirements", []):
            if not isinstance(requirement, dict):
                continue
            lab_id = requirement.get("requirements_lab_requirement_id")
            if not isinstance(lab_id, str):
                continue
            links[lab_id].append(
                {
                    "contract": relative_path(path),
                    "contract_requirement_id": requirement.get("id"),
                    "source": requirement.get("source"),
                    "source_symbol": requirement.get("source_symbol"),
                }
            )
    return dict(links)


def load_contract_test_candidates(directory: Path) -> dict[str, list[dict[str, Any]]]:
    """Index exact test references exported by local Requirements-Lab contracts.

    The normal plan remains authoritative: this reverse index never assigns a
    requirement or changes a Catch2 row.  It only lets the bounded ``unmapped``
    query show a contributor which existing contract records already name the
    exact test, including the candidate Lab ID and clause.
    """

    candidates: defaultdict[str, list[dict[str, Any]]] = defaultdict(list)
    seen: defaultdict[str, set[tuple[Any, ...]]] = defaultdict(set)
    for path in sorted(directory.glob("*-requirements-contract.json")):
        try:
            contract = load_json(path)
        except ValueError:
            continue
        contract_path = relative_path(path)
        requirements = contract.get("requirements", [])
        if not isinstance(requirements, list):
            continue
        for requirement in requirements:
            if not isinstance(requirement, dict):
                continue
            lab_id = requirement.get("requirements_lab_requirement_id")
            if not isinstance(lab_id, str) or not lab_id.strip():
                continue
            contract_requirement_id = requirement.get("id")
            clause_id = requirement.get("clause_id")
            tests = requirement.get("tests", [])
            if not isinstance(tests, list):
                continue
            for test_reference in tests:
                if not isinstance(test_reference, str) or not test_reference.strip():
                    continue
                if "::" in test_reference:
                    source_path, test_case = test_reference.split("::", 1)
                else:
                    source_path, test_case = "", test_reference
                source_path = source_path.replace("\\", "/").strip()
                test_case = test_case.strip()
                if not test_case:
                    continue
                record = {
                    "contract": contract_path,
                    "contract_requirement_id": contract_requirement_id,
                    "lab_requirement_id": lab_id,
                    "clause_id": clause_id,
                    "source_path": source_path,
                }
                identity = (
                    record["contract"],
                    record["contract_requirement_id"],
                    record["lab_requirement_id"],
                    record["clause_id"],
                    record["source_path"],
                )
                # Keep both indexes: the path-qualified key lets the caller
                # identify an exact source match, while the title key keeps a
                # stale/relocated contract discoverable for review.  The
                # returned record carries ``source_path_match`` so a caller
                # never has to mistake a relocated reference for a clean map.
                keys = [test_case]
                if source_path:
                    keys.append(f"{source_path}::{test_case}")
                for key in keys:
                    if identity in seen[key]:
                        continue
                    seen[key].add(identity)
                    candidates[key].append(record)
    for key in candidates:
        candidates[key].sort(
            key=lambda record: (
                str(record.get("contract") or ""),
                str(record.get("lab_requirement_id") or ""),
                str(record.get("clause_id") or ""),
            )
        )
    return dict(candidates)


def contract_candidates_for_test(
    test: dict[str, Any],
    candidates: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    """Return exact contract references for one current plan/source row."""

    test_case = test.get("test_case")
    if not isinstance(test_case, str) or not test_case.strip():
        return []
    source_paths = {
        str(location.get("path") or "").replace("\\", "/").strip()
        for location in test.get("source_locations", [])
        if isinstance(location, dict) and location.get("path")
    }
    keys = [test_case]
    keys.extend(f"{path}::{test_case}" for path in sorted(source_paths))
    result: list[dict[str, Any]] = []
    seen: set[tuple[Any, ...]] = set()
    for key in keys:
        for candidate in candidates.get(key, []):
            identity = (
                candidate.get("contract"),
                candidate.get("contract_requirement_id"),
                candidate.get("lab_requirement_id"),
                candidate.get("clause_id"),
                candidate.get("source_path"),
            )
            if identity in seen:
                continue
            seen.add(identity)
            candidate_copy = dict(candidate)
            candidate_path = str(candidate_copy.get("source_path") or "")
            candidate_copy["source_path_match"] = (
                not candidate_path or candidate_path in source_paths
            )
            result.append(candidate_copy)
    result.sort(
        key=lambda candidate: (
            not bool(candidate.get("source_path_match")),
            str(candidate.get("lab_requirement_id") or ""),
            str(candidate.get("clause_id") or ""),
            str(candidate.get("contract") or ""),
        )
    )
    return result


def _iter_contract_test_references(
    value: Any,
    *,
    owner_id: str | None = None,
) -> Iterable[tuple[str | None, str]]:
    """Yield every local-contract ``tests`` reference without resyncing Lab data.

    Requirements and API contracts both carry nested ``tests`` arrays.  This
    small structural walk keeps the drift query independent of either contract
    kind and preserves the nearest record id for a useful repair handle.
    """

    if isinstance(value, dict):
        next_owner = value.get("id") if isinstance(value.get("id"), str) else owner_id
        tests = value.get("tests")
        if isinstance(tests, list):
            for reference in tests:
                if isinstance(reference, str) and reference.strip():
                    yield next_owner, reference.strip()
        for child in value.values():
            yield from _iter_contract_test_references(child, owner_id=next_owner)
    elif isinstance(value, list):
        for child in value:
            yield from _iter_contract_test_references(child, owner_id=owner_id)


def contract_drift_report(
    directory: Path,
    source_locations: dict[str, list[dict[str, Any]]],
    *,
    query: str | None = None,
    limit: int = 20,
) -> dict[str, Any]:
    """Report stale local contract-to-Catch2 selectors in one bounded query.

    The native C++ source index is the only source of current selectors.  A
    ``cpp/tests/...::Title`` reference is clean only when both path and title
    still match.  External symbolic references (for example a portable TCK
    ``main.cpp#scenario``) are counted but never treated as native drift.
    This is intentionally read-only and does not load or resynchronize the
    Requirements Lab export.
    """

    folded_query = query.casefold().strip() if isinstance(query, str) and query.strip() else None
    source_keys = {
        (str(location.get("path") or "").replace("\\", "/"), location.get("line"))
        for locations in source_locations.values()
        for location in locations
        if isinstance(location, dict) and location.get("path")
    }
    findings: list[dict[str, Any]] = []
    contract_count = 0
    reference_count = 0
    native_reference_count = 0
    external_reference_count = 0
    clean_reference_count = 0

    for path in sorted(directory.glob("*-contract.json")):
        contract_name = relative_path(path)
        if folded_query and folded_query not in contract_name.casefold():
            continue
        contract_count += 1
        try:
            contract = load_json(path)
        except ValueError as error:
            findings.append(
                {
                    "kind": "invalid-contract-json",
                    "contract": contract_name,
                    "owner_id": None,
                    "test_reference": None,
                    "detail": str(error),
                }
            )
            continue
        for owner_id, reference in _iter_contract_test_references(contract):
            reference_count += 1
            if "#" in reference and "::" not in reference:
                external_reference_count += 1
                continue

            if "::" in reference:
                source_path, test_case = reference.split("::", 1)
                source_path = source_path.replace("\\", "/").strip()
                test_case = test_case.strip()
                if not source_path.startswith("cpp/tests/"):
                    external_reference_count += 1
                    continue
                native_reference_count += 1
                locations = source_locations.get(test_case, [])
                exact = any(
                    isinstance(location, dict)
                    and str(location.get("path") or "").replace("\\", "/")
                    == source_path
                    for location in locations
                )
                if exact:
                    clean_reference_count += 1
                    continue
                kind = "relocated-selector" if locations else "stale-selector"
                if not (REPOSITORY_ROOT / source_path).is_file():
                    kind = "missing-native-source"
                findings.append(
                    {
                        "kind": kind,
                        "contract": contract_name,
                        "owner_id": owner_id,
                        "test_reference": reference,
                        "source_path": source_path,
                        "test_case": test_case,
                        "current_source_locations": [
                            {
                                "path": str(location.get("path")),
                                "line": location.get("line"),
                            }
                            for location in locations
                            if isinstance(location, dict)
                        ][:8],
                    }
                )
                continue

            native_match = re.fullmatch(r"(.+):(\d+)", reference)
            if native_match and native_match.group(1).replace("\\", "/").startswith(
                "cpp/tests/"
            ):
                native_reference_count += 1
                native_key = (
                    native_match.group(1).replace("\\", "/"),
                    int(native_match.group(2)),
                )
                if native_key in source_keys:
                    clean_reference_count += 1
                    continue
                findings.append(
                    {
                        "kind": "invalid-native-line-anchor",
                        "contract": contract_name,
                        "owner_id": owner_id,
                        "test_reference": reference,
                        "source_path": native_key[0],
                        "line": native_key[1],
                    }
                )
                continue

            if reference in source_locations:
                clean_reference_count += 1
                continue
            native_reference_count += 1
            findings.append(
                {
                    "kind": "stale-selector",
                    "contract": contract_name,
                    "owner_id": owner_id,
                    "test_reference": reference,
                    "test_case": reference,
                    "current_source_locations": [],
                }
            )

    findings.sort(
        key=lambda finding: (
            str(finding.get("contract") or ""),
            str(finding.get("owner_id") or ""),
            str(finding.get("test_reference") or ""),
        )
    )
    kind_counts = Counter(str(finding.get("kind") or "unknown") for finding in findings)
    shown_limit = max(limit, 0)
    shown_findings = findings if shown_limit == 0 else findings[:shown_limit]
    return {
        "status": "clean" if not findings else "drift",
        "contract_directory": relative_path(directory),
        "contract_count": contract_count,
        "test_reference_count": reference_count,
        "native_reference_count": native_reference_count,
        "external_reference_count": external_reference_count,
        "clean_reference_count": clean_reference_count,
        "finding_count": len(findings),
        "finding_counts": dict(sorted(kind_counts.items())),
        "shown_finding_count": len(shown_findings),
        "findings": shown_findings,
    }


def requirement_view(
    lab_id: str,
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
) -> dict[str, Any]:
    standard = standard_requirements.get(lab_id)
    value: dict[str, Any] = {
        "lab_requirement_id": lab_id,
        "standard": standard,
        "contracts": contract_links.get(lab_id, []),
    }
    if standard is None:
        value["unresolved"] = True
    return value


def mapped_test(
    test: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
    source_locations: dict[str, list[dict[str, Any]]],
) -> dict[str, Any]:
    requirement_ids = strings(test.get("selected_requirements_lab_requirement_ids"))
    requirements = [
        requirement_view(identifier, standard_requirements, contract_links)
        for identifier in requirement_ids
    ]
    standards = [
        requirement["standard"]
        for requirement in requirements
        if requirement.get("standard") is not None
    ]
    clauses = {
        f"{standard.get('document_id')}:{standard.get('clause_id')}"
        for standard in standards
        if standard.get("document_id") and standard.get("clause_id")
    }
    # A development-profile case can have no row-level Requirements-Lab
    # candidate while still being explicitly anchored to canonical standard
    # sections. Preserve that deliberate disposition in every query view
    # instead of making the case appear section-less merely because its Lab
    # requirement list is empty.
    if not requirement_ids:
        clauses.update(
            section
            for section in strings(test.get("standard_sections"))
            if ":" in section
        )
    clauses = sorted(clauses)
    locations = preferred_source_locations(
        test,
        source_locations_for_test(test.get("test_case"), source_locations),
    )
    # Some plan rows deliberately name the same executable evidence under a
    # narrower standards/lane label. Keep that alias explicit in the plan,
    # but resolve its checked-in source pointer without requiring a duplicate
    # TEST_CASE declaration or a second runtime lane.
    if not locations:
        alias_location = test.get("source_alias_location")
        if isinstance(alias_location, str) and ":" in alias_location:
            alias_path, alias_line = alias_location.rsplit(":", 1)
            try:
                parsed_line = int(alias_line)
            except ValueError:
                parsed_line = 0
            if alias_path.strip() and parsed_line > 0:
                locations = [{"path": alias_path.strip(), "line": parsed_line}]
    return {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "primary_lane": test.get("primary_lane"),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "requirements_lab_api_surface_status": test.get(
            "requirements_lab_api_surface_status"
        ),
        "traceability_state": traceability_state(test),
        "tags": strings(test.get("tags")),
        "selected_cpp_api_surface_ids": strings(
            test.get("selected_cpp_api_surface_ids")
        ),
        "assertions": test.get("assertions"),
        "passed_assertions": test.get("passed_assertions"),
        "failed_assertions": test.get("failed_assertions"),
        "expected_assertions_if_harness_fixed": test.get(
            "expected_assertions_if_harness_fixed"
        ),
        "failure_location": test.get("failure_location"),
        "failure_summary": test.get("failure_summary"),
        "callback_models": strings(test.get("callback_models")),
        "delivery_modes": strings(test.get("delivery_modes")),
        "callback_gate_modes": strings(test.get("callback_gate_modes")),
        "status": test.get("status"),
        "source_alias_of": test.get("source_alias_of"),
        "source_alias_location": test.get("source_alias_location"),
        "source_missing_reason": test.get("source_missing_reason"),
        "next_action": test.get("next_action"),
        "requirements": requirements,
        "lab_requirement_ids": requirement_ids,
        "standard_clauses": clauses,
        "standard_sections": clauses,
        "source_locations": locations,
        "source_state": "located" if locations else "unlocated",
        "unresolved_requirement_ids": [
            requirement["lab_requirement_id"]
            for requirement in requirements
            if requirement.get("unresolved")
        ],
    }


def all_mapped_tests(
    plan: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    contract_links: dict[str, list[dict[str, Any]]],
    source_locations: dict[str, list[dict[str, Any]]],
) -> list[dict[str, Any]]:
    return [
        mapped_test(test, standard_requirements, contract_links, source_locations)
        for test in plan.get("tests", [])
        if isinstance(test, dict)
    ]


def roadmap_checklist(path: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    section = ""
    heading = re.compile(r"^#{1,6}\s+(.+?)\s*$")
    checkbox = re.compile(r"^-\s+\[([ xX])\]\s+(.+?)\s*$")
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        heading_match = heading.match(line)
        if heading_match:
            section = heading_match.group(1)
            continue
        checkbox_match = checkbox.match(line)
        if checkbox_match:
            entries.append(
                {
                    "line": line_number,
                    "section": section,
                    "status": "complete" if checkbox_match.group(1).lower() == "x" else "open",
                    "title": checkbox_match.group(2),
                }
            )
    return entries


def implementation_plan_sections(path: Path) -> list[dict[str, Any]]:
    """Return the stable section outline without loading the plan prose.

    The breadcrumb is derived from headings only.  It gives a filtered plan
    query enough context to open the exact section without printing the full
    implementation-plan prose into a work-selection context.
    """

    sections: list[dict[str, Any]] = []
    heading = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
    parents: list[tuple[int, str]] = []
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = heading.match(line)
        if match is None:
            continue
        level = len(match.group(1))
        title = match.group(2)
        while parents and parents[-1][0] >= level:
            parents.pop()
        parents.append((level, title))
        sections.append(
            {
                "line": line_number,
                "level": level,
                "title": title,
                "path": " > ".join(title for _, title in parents),
            }
        )
    return sections


def filtered_plan_sections(
    sections: list[dict[str, Any]],
    query: str | None,
    limit: int,
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Filter the heading-only plan index and apply an explicit output bound."""

    if query is None or not query.strip():
        matches = sections
    else:
        folded = search_key(query)
        matches = [
            section
            for section in sections
            if folded
            and (
                folded in search_key(section.get("title"))
                or folded in search_key(section.get("path"))
                or folded == search_key(section.get("line"))
            )
        ]
    shown = matches if limit == 0 else matches[: max(limit, 0)]
    return matches, shown


def index_status(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    roadmap_entries: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
) -> dict[str, Any]:
    items = [item for item in index.get("items", []) if isinstance(item, dict)]
    tag_counts = Counter(tag for test in tests for tag in test.get("tags", []))
    enriched_items = []
    for item in items:
        tags = strings(item.get("query_tags"))
        tags.extend(strings(item.get("focused_lane_tags")))
        pointer = next_test_pointer(item, tests)
        matching_tests = item_tests(item, tests)
        enriched_items.append(
            {
                **item,
                "matching_test_counts": {tag: tag_counts.get(tag, 0) for tag in tags},
                # A roadmap item usually carries several overlapping tags.
                # Keep the per-tag counts for discovery, but also publish the
                # number of distinct Catch2 cases so compact status output
                # cannot look larger merely because a case has many tags.
                "matching_test_count": len(matching_tests),
                "next_test_pointer": pointer,
                "live_current_focus": live_item_focus(item, tests),
                "live_mapping_counts": live_item_mapping_counts(item, tests),
            }
        )
    mapping = index.get("mapping", {})
    current_counts = (
        mapping.get("current_counts", {})
        if isinstance(mapping, dict)
        else {}
    )
    derived_counts = {
        "catch2_plan_cases": len(tests),
        "catch2_planned_cases": sum(is_planned_test(test) for test in tests),
        "catch2_cases_without_lab_requirement_mapping": sum(
            not test.get("requirements") for test in tests
        ),
        "catch2_unmapped_explicit_disposition": sum(
            not test.get("requirements")
            and traceability_state(test) == "explicit-disposition"
            for test in tests
        ),
        "catch2_unmapped_unclassified": sum(
            not test.get("requirements")
            and traceability_state(test) == "unclassified"
            for test in tests
        ),
        "catch2_cases_without_cpp_source_location": sum(
            not test.get("source_locations") for test in tests
        ),
        "catch2_actionable_cases_without_cpp_source_location": sum(
            not test.get("source_locations")
            and not is_planned_test(test)
            and not is_non_executable_source_status(test)
            for test in tests
        ),
    }
    if source_locations is not None:
        unplanned_cases = unplanned_source_cases(tests, source_locations)
        derived_counts["cpp_source_cases_without_plan_row"] = len(unplanned_cases)
        derived_counts["cpp_source_cases_without_plan_row_fom_composer"] = sum(
            any(
                "fom_composer" in str(location.get("path", "")).casefold()
                for location in test.get("source_locations", [])
                if isinstance(location, dict)
            )
            for test in unplanned_cases
        )
    # Keep any explicitly published lane counters honest as the Catch2 plan
    # grows.  The index uses snake_case counter keys while Catch2 tags use
    # hyphenated lane names (for example service_report_store_lane_cases), so
    # derive those values from the same tags used by ``focus``/``coverage``.
    if isinstance(current_counts, dict):
        for key in current_counts:
            if not isinstance(key, str) or not key.endswith("_lane_cases"):
                continue
            lane_prefix = key[: -len("_lane_cases")]
            lane_name = lane_prefix.replace("_", "-")
            lane_tests = [
                test
                for test in tests
                if lane_name in strings(test.get("tags"))
            ]
            derived_counts[key] = len(lane_tests)
            mapped_key = f"{lane_prefix}_lane_mapped_cases"
            if mapped_key in current_counts:
                derived_counts[mapped_key] = sum(
                    bool(test.get("requirements")) for test in lane_tests
                )
    global_source_queue = (
        unplanned_source_summary(tests, source_locations)
        if source_locations is not None
        else None
    )
    latest_completed_slice = (
        mapping.get("latest_completed_slice")
        if isinstance(mapping, dict)
        and isinstance(mapping.get("latest_completed_slice"), dict)
        else None
    )
    return {
        "roadmap_items": enriched_items,
        "roadmap_checklist": roadmap_entries,
        "roadmap_counts": dict(Counter(entry["status"] for entry in roadmap_entries)),
        "test_count": len(tests),
        "mapping_counts": current_counts if isinstance(current_counts, dict) else {},
        "derived_mapping_counts": derived_counts,
        "global_source_queue": global_source_queue,
        "latest_completed_slice": latest_completed_slice,
    }


def item_tests(item: dict[str, Any], tests: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Return tests belonging to an indexed roadmap item through its query tags."""

    tags = set(strings(item.get("query_tags")))
    tags.update(strings(item.get("focused_lane_tags")))
    if not tags:
        return []
    return [test for test in tests if tags.intersection(test.get("tags", []))]


def scoped_plan_tests(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    *,
    lane: str | None = None,
    family: str | None = None,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    """Resolve one optional exact lane or roadmap-family scope.

    Lane and family selectors are intentionally separate handles.  Returning
    the family item alongside the rows lets coverage/check render the owner
    without repeating a broad roadmap lookup.
    """

    if family is not None:
        folded_family = family.casefold()
        family_item = next(
            (
                item
                for item in index.get("items", [])
                if isinstance(item, dict)
                and str(item.get("id") or "").casefold() == folded_family
            ),
            None,
        )
        if family_item is None:
            return None, []
        return family_item, item_tests(family_item, tests)
    if lane is not None:
        folded_lane = lane.casefold()
        return None, [
            test
            for test in tests
            if any(
                str(tag).casefold() == folded_lane
                for tag in strings(test.get("tags"))
            )
        ]
    return None, tests


def live_item_focus(item: dict[str, Any], tests: list[dict[str, Any]]) -> str | None:
    """Return a compact, derived focus card instead of stale roadmap prose.

    The long ``current_focus`` field in the roadmap is a human-maintained
    narrative and can lag the indexed plan after a slice is added or repaired.
    Status output should therefore expose counts derived from the same plan it
    is querying, even for families that do not carry narrative focus prose;
    the archival narrative remains available in the JSON index.
    """

    matching = item_tests(item, tests)
    mapped = sum(bool(test.get("requirements")) for test in matching)
    explicit = sum(
        not test.get("requirements")
        and traceability_state(test) == "explicit-disposition"
        for test in matching
    )
    unclassified = sum(
        not test.get("requirements")
        and traceability_state(test) == "unclassified"
        for test in matching
    )
    planned = sum(is_planned_test(test) for test in matching)
    unlocated = sum(
        not test.get("source_locations") and not is_planned_test(test)
        for test in matching
    )
    candidates = sum(
        bool(test.get("source_locations"))
        and not str(test.get("status") or "").startswith(("implemented", "verified"))
        and not is_non_executable_source_status(test)
        for test in matching
    )
    return (
        f"Live index: {len(matching)} tagged Catch2 cases; {mapped} mapped, "
        f"{explicit} explicit dispositions, {unclassified} unclassified, "
        f"{planned} planned, {unlocated} source-unlocated, "
        f"{candidates} executable candidates."
    )


def live_item_mapping_counts(
    item: dict[str, Any],
    tests: list[dict[str, Any]],
) -> dict[str, Any]:
    """Return compact, plan-derived mapping counts for one roadmap family.

    The roadmap's tag sets overlap by design.  These counts therefore use the
    distinct Catch2 rows selected by :func:`item_tests`, and separate an
    intentional no-standalone-surface disposition from a row that still needs
    a requirements decision.  Keeping this structured beside the human focus
    string lets status consumers make a bounded decision without a second
    ``test``/``trace`` query.
    """

    matching = item_tests(item, tests)
    mapped = [test for test in matching if test.get("requirements")]
    explicit = [
        test
        for test in matching
        if not test.get("requirements")
        and traceability_state(test) == "explicit-disposition"
    ]
    unclassified = [
        test
        for test in matching
        if not test.get("requirements")
        and traceability_state(test) == "unclassified"
    ]
    sections = {
        section
        for test in matching
        for section in strings(test.get("standard_sections"))
    }
    api_surfaces = {
        surface
        for test in matching
        for surface in strings(test.get("selected_cpp_api_surface_ids"))
    }
    requirement_section_rows = [
        row
        for test in matching
        for row in requirement_section_mapping_rows(test)
        if isinstance(row, dict)
    ]
    requirement_section_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for row in requirement_section_rows
        if row.get("lab_requirement_id")
    }
    resolved_requirement_section_pairs = {
        pair
        for pair in requirement_section_pairs
        if pair[1]
    }
    requirement_ids = {
        row.get("lab_requirement_id")
        for row in requirement_section_rows
        if row.get("lab_requirement_id")
    }
    planned = sum(is_planned_test(test) for test in matching)
    source_unlocated = sum(
        not test.get("source_locations") and not is_planned_test(test)
        for test in matching
    )
    candidates = sum(
        bool(test.get("source_locations"))
        and not str(test.get("status") or "").startswith(("implemented", "verified"))
        and not is_non_executable_source_status(test)
        for test in matching
    )
    sorted_requirement_ids = sorted(
        value for value in requirement_ids if isinstance(value, str)
    )
    sorted_standard_sections = sorted(
        section for section in sections if isinstance(section, str)
    )
    sorted_api_surfaces = sorted(
        surface for surface in api_surfaces if isinstance(surface, str)
    )
    sorted_requirement_section_pairs = sorted(
        (
            requirement_id,
            standard_section,
        )
        for requirement_id, standard_section in resolved_requirement_section_pairs
        if isinstance(requirement_id, str)
    )
    return {
        "case_count": len(matching),
        "mapped_case_count": len(mapped),
        "explicit_disposition_count": len(explicit),
        "unclassified_count": len(unclassified),
        "planned_count": planned,
        "source_unlocated_count": source_unlocated,
        "executable_candidate_count": candidates,
        "requirement_count": len(requirement_ids),
        "standard_section_count": len(sections),
        "cpp_api_surface_count": len(api_surfaces),
        "requirement_section_pair_count": len(requirement_section_pairs),
        "resolved_requirement_section_pair_count": len(
            resolved_requirement_section_pairs
        ),
        "unresolved_requirement_section_mapping_count": sum(
            bool(row.get("unresolved")) for row in requirement_section_rows
        ),
        # These previews keep a family handoff self-contained without placing
        # the full overlapping mapping arrays in every queue row.  The
        # corresponding counts remain authoritative for callers that need the
        # complete matrix through ``focus``/``matrix``.
        "requirement_ids_preview": sorted_requirement_ids[:8],
        "requirement_ids_remaining": max(len(sorted_requirement_ids) - 8, 0),
        "standard_sections_preview": sorted_standard_sections[:8],
        "standard_sections_remaining": max(len(sorted_standard_sections) - 8, 0),
        "requirement_section_pairs_preview": [
            {
                "lab_requirement_id": requirement_id,
                "standard_section": standard_section,
            }
            for requirement_id, standard_section in sorted_requirement_section_pairs[:8]
        ],
        "requirement_section_pairs_remaining": max(
            len(sorted_requirement_section_pairs) - 8,
            0,
        ),
    }


def item_matches_lane(item: dict[str, Any], lane: str) -> bool:
    """Match a lane against both broad family tags and its exact next pointer.

    A roadmap family may intentionally keep a broad ``query_tags`` set while
    pointing at a narrower ``next_lane``.  Focused-lane aliases are kept in a
    small ``focused_lane_tags`` field so the large family tag list does not
    become another maintenance bottleneck. Treating only the broad tags as a
    lane relationship made ``check --lane`` report zero roadmap owners for
    otherwise valid focused slices.
    """

    folded = lane.casefold()
    query_tags = strings(item.get("query_tags"))
    query_tags.extend(strings(item.get("focused_lane_tags")))
    return (
        any(str(tag).casefold() == folded for tag in query_tags)
        or str(item.get("next_lane") or "").casefold() == folded
    )


def roadmap_links_for_test(
    index: dict[str, Any],
    test: dict[str, Any],
    limit: int = 8,
) -> list[dict[str, Any]]:
    """Return bounded roadmap-family links for one mapped Catch2 case.

    An optional plan-row ``primary_lane`` is ranked first, followed by the
    explicit ``mapping.lane_owners`` relation, exact
    ``next_lane``/``focused_lane_tags`` handles, and finally broad tag overlap.
    This keeps a trace result pointed at the family that owns a focused slice
    while retaining broader roadmap relationships for context.
    """

    test_tags = set(strings(test.get("tags")))
    primary_lane = str(test.get("primary_lane") or "").strip()
    candidates: list[tuple[tuple[int, int, int, str], dict[str, Any]]] = []
    lane_owners = index.get("mapping", {}).get("lane_owners", {})
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    for item in index.get("items", []):
        if not isinstance(item, dict):
            continue
        query_tags = set(strings(item.get("query_tags")))
        focused_lane_tags = set(strings(item.get("focused_lane_tags")))
        query_tags.update(focused_lane_tags)
        next_lane = item.get("next_lane")
        exact_next_lane = isinstance(next_lane, str) and next_lane in test_tags
        exact_focused_lane = bool(focused_lane_tags.intersection(test_tags))
        exact_lane_owner = any(
            owner == item.get("id") and lane in test_tags
            for lane, owner in lane_owners.items()
        )
        exact_primary_lane_owner = bool(
            primary_lane
            and any(
                str(lane).casefold() == primary_lane.casefold()
                and owner == item.get("id")
                for lane, owner in lane_owners.items()
            )
        )
        overlap = len(query_tags.intersection(test_tags))
        if (
            not exact_lane_owner
            and not exact_next_lane
            and not exact_focused_lane
            and overlap == 0
        ):
            continue
        candidates.append(
            (
                (
                    0
                    if exact_primary_lane_owner
                    else 1
                    if exact_lane_owner
                    else 2
                    if exact_next_lane
                    else 3
                    if exact_focused_lane
                    else 4,
                    -overlap,
                    item.get("priority", 999)
                    if isinstance(item.get("priority"), int)
                    else 999,
                    str(item.get("id") or ""),
                ),
                {
                    "id": item.get("id"),
                    "title": item.get("title"),
                    "status": item.get("status"),
                    "priority": item.get("priority"),
                    "next_lane": next_lane,
                    "match": (
                        "lane_owner"
                        if exact_primary_lane_owner or exact_lane_owner
                        else "next_lane"
                        if exact_next_lane
                        else "focused_lane_tag"
                        if exact_focused_lane
                        else "query_tag"
                    ),
                },
            )
        )
    candidates.sort(key=lambda value: value[0])
    values = [value for _, value in candidates]
    return values if limit == 0 else values[: max(limit, 0)]


def primary_roadmap_link(roadmap_items: list[dict[str, Any]]) -> dict[str, Any] | None:
    """Return the strongest roadmap relationship for a mapped test.

    Exact lane ownership is the authoritative family relationship.  The
    fallback order preserves useful context for older rows that only carry a
    family tag, while keeping a machine- and human-readable owner separate
    from broad taxonomy overlaps.
    """

    for match_kind in ("lane_owner", "next_lane", "focused_lane_tag"):
        match = next(
            (
                item
                for item in roadmap_items
                if isinstance(item, dict) and item.get("match") == match_kind
            ),
            None,
        )
        if match is not None:
            return match
    return next(
        (item for item in roadmap_items if isinstance(item, dict)),
        None,
    )


def next_test_pointer(item: dict[str, Any], tests: list[dict[str, Any]]) -> dict[str, Any]:
    """Describe whether an indexed test handle is a TODO or a completed baseline.

    Roadmap items often name an existing regression as the baseline for a new
    slice. Treating that handle as an unqualified ``next`` test made the work
    queue look stale and encouraged rerunning completed work. Keep the handle,
    but derive a small status record from the mapped plan so callers can
    distinguish a planned test from an implemented baseline.
    """

    query = item.get("next_test_query")
    if not isinstance(query, str) or not query.strip():
        return {"state": "not-specified", "match_count": 0, "statuses": {}}

    matches = [test for test in tests if test.get("test_case") == query]
    if not matches:
        return {
            "query": query,
            "state": "missing",
            "match_count": 0,
            "statuses": {},
        }

    status_counts = Counter(
        str(test.get("status") or "<unspecified>") for test in matches
    )
    implemented = all(
        status.startswith(("implemented", "verified"))
        for status in status_counts
    )
    located = all(bool(test.get("source_locations")) for test in matches)
    if implemented and located:
        state = "complete"
    elif implemented:
        # A plan entry may describe an implemented historical expectation
        # whose exact declaration is absent from the current checkout. Keep
        # that distinction visible so ``next`` cannot mistake catalog prose
        # for executable evidence.
        state = "source-missing"
    elif any(status.startswith(("implemented", "verified")) for status in status_counts):
        state = "partial"
    else:
        state = "planned"
    # Keep the pointer useful as a one-screen resume handle.  The exact test
    # query remains authoritative; these aggregate counts let status/queue
    # callers see whether the baseline is mapped to requirements, canonical
    # 2025 sections, and official C++ API surfaces without issuing a second
    # repository-wide query.  Duplicate plan rows are intentionally folded
    # into unique ids/sections so a cross-family pointer cannot inflate the
    # mapping just because the same TEST_CASE is listed twice.
    requirement_ids = sorted(
        {
            requirement_id
            for test in matches
            for requirement_id in strings(test.get("lab_requirement_ids"))
        }
    )
    standard_sections = sorted(
        {
            section
            for test in matches
            for section in strings(test.get("standard_sections"))
        }
    )
    api_surfaces = sorted(
        {
            surface
            for test in matches
            for surface in strings(test.get("selected_cpp_api_surface_ids"))
        }
    )
    source_locations = [
        location
        for test in matches
        for location in test.get("source_locations", [])
        if isinstance(location, dict)
    ]
    pair_rows: list[dict[str, Any]] = []
    seen_pairs: set[tuple[Any, Any]] = set()
    for test in matches:
        for row in requirement_section_mapping_rows(test):
            if not isinstance(row, dict) or not row.get("lab_requirement_id"):
                continue
            pair = (row.get("lab_requirement_id"), row.get("standard_section"))
            if pair in seen_pairs:
                continue
            seen_pairs.add(pair)
            pair_rows.append(
                {
                    "lab_requirement_id": row.get("lab_requirement_id"),
                    "standard_section": row.get("standard_section"),
                    "title": row.get("title"),
                    "unresolved": bool(row.get("unresolved")),
                }
            )
    pair_rows.sort(
        key=lambda row: (
            str(row.get("lab_requirement_id") or ""),
            str(row.get("standard_section") or ""),
        )
    )
    result = {
        "query": query,
        "state": state,
        "match_count": len(matches),
        "statuses": dict(sorted(status_counts.items())),
        "assertion_count": sum(
            value
            for test in matches
            for value in [test.get("assertions")]
            if isinstance(value, int)
        ),
        "requirement_count": len(requirement_ids),
        "requirement_ids": requirement_ids,
        "standard_section_count": len(standard_sections),
        "standard_sections": standard_sections,
        "cpp_api_surface_count": len(api_surfaces),
        "requirement_section_pair_count": len(pair_rows),
        "requirement_section_mappings": pair_rows[:12],
        "traceability_states": sorted(
            {traceability_state(test) for test in matches}
        ),
        "source_locations": source_locations[:8],
    }
    if len(source_locations) > 8:
        result["source_locations_remaining"] = len(source_locations) - 8
    if len(pair_rows) > 12:
        result["requirement_section_mappings_remaining"] = len(pair_rows) - 12
    return result


def index_warnings(
    index_result: dict[str, Any],
    lane: str | None = None,
) -> list[str]:
    """Return bounded warnings for open items whose test handle is complete."""

    warnings: list[str] = []
    for item in index_result.get("roadmap_items", []):
        if not isinstance(item, dict) or item.get("status") != "open":
            continue
        if lane and not item_matches_lane(item, lane):
            continue
        pointer = item.get("next_test_pointer")
        if not isinstance(pointer, dict) or pointer.get("state") != "complete":
            continue
        # A baseline is intentional: the accompanying next_work_query remains
        # the actionable slice. Any unlabelled completed handle is likely a
        # stale pointer and should be corrected in the index.
        if item.get("next_test_role") == "baseline":
            continue
        warnings.append(
            f"{item.get('id', '<unnamed>')} next_test_query is already implemented; "
            "label it as next_test_role=baseline or replace the handle"
        )
    return warnings


def roadmap_item_summary(item: dict[str, Any]) -> dict[str, Any]:
    """Keep the active-work record small while retaining its stable handles."""

    return {
        "id": item.get("id"),
        "title": item.get("title"),
        "status": item.get("status"),
        "kind": item.get("kind"),
        "priority": item.get("priority"),
        "roadmap_anchor": item.get("roadmap_anchor"),
    }


def roadmap_search_values(
    item: dict[str, Any],
    tests: list[dict[str, Any]] | None = None,
) -> list[str]:
    """Return bounded roadmap and mapping fields used by family search.

    The roadmap index remains the discovery surface, but a family search is
    more useful when an exact Requirements-Lab id, canonical 2025 subsection,
    or official C++ API surface can resolve directly to its owning family.  A
    caller may therefore provide the already-loaded Catch2 rows; this joins
    only the rows tagged to the family and never re-reads the Requirements Lab.
    """

    values: list[str] = []
    for field in (
        "id",
        "title",
        "kind",
        "priority",
        "roadmap_anchor",
        "query_tags",
        "focused_lane_tags",
        "current_focus",
        "next_task",
        "next_action",
        "next_work_id",
        "next_work_status",
        "next_lane",
        "next_test_query",
        "next_source_lane",
        "next_source_test_query",
    ):
        values.extend(strings(item.get(field)))
    if tests is not None:
        for test in item_tests(item, tests):
            for field in (
                "id",
                "test_case",
                "tags",
                "lab_requirement_ids",
                "standard_sections",
                "selected_cpp_api_surface_ids",
                "primary_lane",
            ):
                values.extend(strings(test.get(field)))
    return values


def roadmap_inventory(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    *,
    query: str | None = None,
    status: str = "open",
    limit: int = 12,
) -> dict[str, Any]:
    """Return a bounded, live roadmap-family search result.

    The roadmap index is the discovery surface; Catch2 rows remain the source
    of truth for live mapping counts.  Keeping this join in one command means
    a contributor can search family ids, titles, tags, next actions, requirement
    ids, canonical 2025 subsections, or official C++ API surfaces without
    opening the roadmap prose or enumerating every Catch2 lane.
    """

    items = [item for item in index.get("items", []) if isinstance(item, dict)]
    folded_query = search_key(query) if isinstance(query, str) and query.strip() else ""
    exact_index_ids: set[str] = set()
    active_handoff = index.get("active_handoff")
    active_handoff_family: str | None = None
    if isinstance(active_handoff, dict) and folded_query:
        active_values = strings(
            active_handoff.get(field)
            for field in ("id", "family", "lane", "test_case", "objective")
        )
        if any(folded_query in search_key(value) for value in active_values):
            family_value = active_handoff.get("family")
            if isinstance(family_value, str) and family_value:
                active_handoff_family = family_value
    if folded_query:
        exact_index_ids = {
            str(item.get("id"))
            for item in items
            if any(
                folded_query == search_key(value)
                for value in roadmap_search_values(item)
            )
            and item.get("id")
        }
        if active_handoff_family is not None and any(
            str(item.get("id") or "").casefold()
            == active_handoff_family.casefold()
            for item in items
        ):
            exact_index_ids.add(active_handoff_family)
    selected: list[dict[str, Any]] = []
    for item in items:
        item_status = str(item.get("status") or "").casefold()
        if status != "all" and item_status != status.casefold():
            continue
        if folded_query:
            indexed_values = roadmap_search_values(item)
            # Preserve exact lane/family handles as the narrowest selector;
            # broad mapping joins are a fallback for requirement, subsection,
            # and API queries.  This avoids returning every family that merely
            # shares a broad tag with an exact lane while still allowing a
            # canonical mapping id to find its owning family.
            exact_index_match = str(item.get("id")) in exact_index_ids
            if exact_index_ids and not exact_index_match:
                continue
            if not exact_index_ids and not any(
                folded_query in search_key(value)
                for value in roadmap_search_values(item, tests)
            ):
                continue
        selected.append(item)
    selected.sort(
        key=lambda item: (
            item.get("priority") if isinstance(item.get("priority"), int) else 999,
            str(item.get("id") or "").casefold(),
        )
    )

    rows: list[dict[str, Any]] = []
    for item in selected:
        matching_tests = item_tests(item, tests)
        # A lane tag is a much narrower and more useful resume handle than
        # the owning family.  When a roadmap search query names one exact
        # lane (for example ``joined-federate-mom-...``), carry that lane's
        # bounded inventory row in the family result so callers do not have
        # to issue a second family -> lanes -> matrix lookup just to find the
        # test/mapping handle.  Keep this opt-in to exact tag-bearing queries;
        # ordinary requirement/section/API searches remain one row per family.
        lane_query = folded_query
        matching_lane_tags = sorted(
            {
                tag
                for test in matching_tests
                for tag in strings(test.get("tags"))
                if lane_query
                and search_key(tag) == lane_query
            },
            key=str.casefold,
        )
        lane_matches: list[dict[str, Any]] = []
        if matching_lane_tags:
            # ``lane_inventory`` is defined below but is available by the
            # time the CLI invokes this resolver.  Resolve the family once,
            # then retain only the exact lane rows requested by the caller.
            inventory = lane_inventory(
                index,
                tests,
                family=str(item.get("id") or ""),
                limit=0,
            )
            inventory_rows = inventory.get("lanes", []) if isinstance(inventory, dict) else []
            for tag in matching_lane_tags:
                lane_row = next(
                    (
                        candidate
                        for candidate in inventory_rows
                        if isinstance(candidate, dict)
                        and search_key(candidate.get("tag")) == search_key(tag)
                    ),
                    None,
                )
                if not isinstance(lane_row, dict):
                    continue
                lane_matches.append(
                    {
                        "tag": lane_row.get("tag"),
                        "state": lane_row.get("state"),
                        "action_state": lane_row.get("action_state"),
                        "test_count": lane_row.get("test_count", 0),
                        "mapped_count": lane_row.get("mapped_count", 0),
                        "explicit_disposition_count": lane_row.get(
                            "explicit_disposition_count", 0
                        ),
                        "unclassified_count": lane_row.get("unclassified_count", 0),
                        "assertion_count": lane_row.get("assertion_count", 0),
                        "requirement_count": lane_row.get("requirement_count", 0),
                        "standard_section_count": lane_row.get(
                            "standard_section_count", 0
                        ),
                        "requirement_section_pair_count": lane_row.get(
                            "requirement_section_pair_count", 0
                        ),
                        "next_test": lane_row.get("next_test"),
                        "next_test_id": lane_row.get("next_test_id"),
                        "next_source": lane_row.get("next_source"),
                        "next_requirement_ids": lane_row.get("next_requirement_ids", []),
                        "next_standard_sections": lane_row.get(
                            "next_standard_sections", []
                        ),
                        "representative_test": lane_row.get("representative_test"),
                        "representative_test_id": lane_row.get("representative_test_id"),
                        "representative_source": lane_row.get("representative_source"),
                        "representative_requirement_ids": lane_row.get(
                            "representative_requirement_ids", []
                        ),
                        "representative_standard_sections": lane_row.get(
                            "representative_standard_sections", []
                        ),
                        "representative_trace_command": lane_row.get(
                            "representative_trace_command"
                        ),
                        "focus_command": lane_row.get("focus_command"),
                        "ctest_command": lane_row.get("ctest_command"),
                        "next_trace_command": lane_row.get("next_trace_command"),
                    }
                )
        mapping_counts = live_item_mapping_counts(item, tests)
        pointer = next_test_pointer(item, tests)
        pointer_owner = next(
            (
                candidate
                for candidate in items
                if candidate.get("next_work_id") == item.get("id")
                and isinstance(candidate.get("next_lane"), str)
                and candidate.get("next_lane")
            ),
            None,
        )
        lane = item.get("next_lane") or (
            pointer_owner.get("next_lane") if pointer_owner else None
        )
        # A reverse pointer is useful context for display, but it must not
        # make a broad owner inherit the narrower family's completed lane
        # state.  Queue action classification therefore uses only the
        # family's own next_lane when one is recorded.
        action_lane = item.get("next_lane")
        lane_snapshot = (
            focused_lane_result(index, tests, action_lane, limit=1)
            if isinstance(action_lane, str) and action_lane.strip()
            else None
        )
        action_state = queue_action_state(
            item,
            candidate_count=(
                lane_snapshot.get("candidate_count")
                if isinstance(lane_snapshot, dict)
                and lane_snapshot.get("lane_state") != "missing"
                else mapping_counts.get("executable_candidate_count", 0)
            ),
            unclassified_count=(
                lane_snapshot.get("unclassified_count")
                if isinstance(lane_snapshot, dict)
                and lane_snapshot.get("lane_state") != "missing"
                else mapping_counts.get("unclassified_count", 0)
            ),
            actionable_source_drift_count=(
                lane_snapshot.get("actionable_source_drift_count")
                if isinstance(lane_snapshot, dict)
                and lane_snapshot.get("lane_state") != "missing"
                else sum(
                    not test.get("source_locations")
                    and not is_non_executable_source_status(test)
                    for test in matching_tests
                )
            ),
            lane_state=(
                "complete-pointer"
                if isinstance(lane_snapshot, dict)
                and lane_snapshot.get("lane_state") == "complete"
                else None
            ),
        )
        assertion_count = sum(
            value
            for test in matching_tests
            for value in [test.get("assertions")]
            if isinstance(value, int)
        )
        commands = {
            "work": f"python tools/query_rti_work.py work {item.get('id')} --summary --compact",
            "matrix": f"python tools/query_rti_work.py matrix {item.get('id')} --summary --compact",
        }
        if isinstance(lane, str) and lane.strip():
            commands["focus"] = (
                f"python tools/query_rti_work.py focus {lane} --summary --compact"
            )
        row = {
            "id": item.get("id"),
            "title": item.get("title"),
            "status": item.get("status"),
            "kind": item.get("kind"),
            "priority": item.get("priority"),
            "roadmap_anchor": item.get("roadmap_anchor"),
            "query_tags": strings(item.get("query_tags"))
            + strings(item.get("focused_lane_tags")),
            "query_tag_count": len(strings(item.get("query_tags")))
            + len(strings(item.get("focused_lane_tags"))),
            "next_work_id": item.get("next_work_id"),
            "next_work_status": item.get("next_work_status"),
            "next_task": item.get("next_task"),
            "next_work_query": item.get("next_work_query"),
            "next_lane": lane,
            "action_state": action_state,
            "active_pointer_id": pointer_owner.get("id") if pointer_owner else None,
            "active_pointer_lane": pointer_owner.get("next_lane") if pointer_owner else None,
            "next_test_query": item.get("next_test_query"),
            "next_test_pointer": pointer,
            "next_source_state": source_pointer_state(item),
            "next_source_test_query": item.get("next_source_test_query"),
            "next_source_location": item.get("next_source_location"),
            "next_source_lane": item.get("next_source_lane"),
            "next_source_ctest_filter": item.get("next_source_ctest_filter"),
            "next_action": live_next_action(item),
            "matching_test_count": len(matching_tests),
            "assertion_count": assertion_count,
            "live_mapping_counts": mapping_counts,
            "commands": commands,
            "lane_match_count": len(lane_matches),
            "lane_matches": lane_matches,
        }
        if (
            isinstance(active_handoff_family, str)
            and str(item.get("id") or "").casefold()
            == active_handoff_family.casefold()
        ):
            handoff = indexed_active_handoff_record(
                index,
                tests,
                requested_family=str(item.get("id") or ""),
            )
            if isinstance(handoff, dict) and handoff.get("found"):
                row["active_handoff"] = {
                    key: handoff.get(key)
                    for key in (
                        "active_handoff_id",
                        "state",
                        "runnable",
                        "handoff_kind",
                        "test_case",
                        "source_target",
                        "source_lane",
                        "ctest_target",
                        "ctest_filter",
                        "mapping_status",
                        "mapping_seed_plan_id",
                        "mapping_seed_test_case",
                        "requirement_ids",
                        "standard_sections",
                        "api_surfaces",
                        "requirement_section_pair_count",
                        "next_action",
                        "acceptance",
                    )
                    if key in handoff
                }
        rows.append(row)

    shown = rows if limit == 0 else rows[: max(limit, 0)]
    return {
        "query": query,
        "status": status,
        "count": len(rows),
        "shown_count": len(shown),
        "families": shown,
    }


def roadmap_summary_data(row: dict[str, Any], limit: int = 8) -> dict[str, Any]:
    """Bound one roadmap search row for compact JSON consumers."""

    result = dict(row)
    tags = strings(row.get("query_tags"))
    result["query_tags"] = tags[:limit]
    if len(tags) > limit:
        result["query_tags_remaining"] = len(tags) - limit
    action = row.get("next_action")
    if isinstance(action, str):
        result["next_action"] = compact_prose(action, 240)
    for field in ("next_task", "next_work_query"):
        value = row.get(field)
        if isinstance(value, str):
            result[field] = compact_prose(value, 240)
    pointer = row.get("next_test_pointer")
    if isinstance(pointer, dict):
        result["next_test_pointer"] = dict(pointer)
        for field in ("requirement_ids", "standard_sections"):
            values = strings(pointer.get(field))
            result["next_test_pointer"][field] = values[:limit]
            if len(values) > limit:
                result["next_test_pointer"][f"{field}_remaining"] = len(values) - limit
        pair_rows = pointer.get("requirement_section_mappings")
        if isinstance(pair_rows, list):
            result["next_test_pointer"]["requirement_section_mappings"] = pair_rows[:limit]
            if len(pair_rows) > limit:
                result["next_test_pointer"]["requirement_section_mappings_remaining"] = len(pair_rows) - limit
    lane_matches = row.get("lane_matches")
    if isinstance(lane_matches, list):
        bounded_lanes: list[dict[str, Any]] = []
        for lane in lane_matches[:limit]:
            if not isinstance(lane, dict):
                continue
            bounded_lane = dict(lane)
            for field in (
                "next_requirement_ids",
                "next_standard_sections",
                "representative_requirement_ids",
                "representative_standard_sections",
            ):
                values = strings(lane.get(field))
                bounded_lane[field] = values[:limit]
                if len(values) > limit:
                    bounded_lane[f"{field}_remaining"] = len(values) - limit
            bounded_lanes.append(bounded_lane)
        result["lane_matches"] = bounded_lanes
        if len(lane_matches) > limit:
            result["lane_matches_remaining"] = len(lane_matches) - limit
    return result


def bounded_requirement_ids(test: dict[str, Any], limit: int = 12) -> dict[str, Any]:
    """Return exact requirement handles without expanding a cross-cutting case."""

    values = strings(test.get("lab_requirement_ids"))
    result: dict[str, Any] = {
        "requirement_count": len(values),
        "requirement_ids": values[:limit],
    }
    if len(values) > limit:
        result["requirement_ids_remaining"] = len(values) - limit
    return result


def traceability_state(test: dict[str, Any]) -> str:
    """Classify a plan row without inventing a requirement mapping.

    The unmapped queue contains both ordinary rows that still need an explicit
    Lab decision and rows whose plan metadata records why no standalone Lab
    surface exists.  Keep that distinction visible in every bounded query;
    the latter is a disposition, not conformance evidence.
    """

    if (
        test.get("requirements")
        or strings(test.get("selected_requirements_lab_requirement_ids"))
        or strings(test.get("lab_requirement_ids"))
    ):
        return "requirements-mapped"
    disposition = test.get("requirements_lab_api_surface_status")
    if isinstance(disposition, str) and disposition.strip():
        return "explicit-disposition"
    return "unclassified"


def indexed_work_slice(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    requested_id: str | None = None,
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
) -> dict[str, Any]:
    """Resolve one actionable slice without scanning roadmap prose.

    The human roadmap item and the active implementation item are deliberately
    separate in the index: a broad parent can point at a narrower
    ``next_work_id``.  This resolver makes that relationship explicit and joins
    only the exact baseline test, lane counts, and canonical 2025 sections.
    """

    items = [item for item in index.get("items", []) if isinstance(item, dict)]
    by_id = {
        item.get("id"): item
        for item in items
        if isinstance(item.get("id"), str) and item.get("id")
    }
    if requested_id is None:
        open_items = sorted(
            (item for item in items if item.get("status") == "open"),
            key=lambda item: item.get("priority", 999),
        )
        pointer_owner = open_items[0] if open_items else None
    else:
        pointer_owner = by_id.get(requested_id)
        if pointer_owner is None:
            folded_id = requested_id.casefold()
            pointer_owner = next(
                (
                    item
                    for item in items
                    if str(item.get("id") or "").casefold() == folded_id
                ),
                None,
            )

    if pointer_owner is None:
        return {
            "found": False,
            "requested_id": requested_id,
            "reason": "no indexed open roadmap item matched" if requested_id is None
            else "no indexed roadmap item matched",
        }

    # A narrow work item may be referenced by a broader priority owner and
    # intentionally carry no duplicate pointer fields of its own.  Resolve
    # that reverse edge only when the requested item is truly pointer-only.
    # If it has its own current action, an explicit ``work <family-id>`` query
    # must not inherit stale historical counts from the parent.
    if requested_id is not None and not any(
        pointer_owner.get(field)
        for field in ("next_lane", "next_ctest_filter", "next_test_query", "next_work_query")
    ):
        if not str(pointer_owner.get("next_action") or "").strip():
            owners = sorted(
                (
                    item
                    for item in items
                    if item.get("next_work_id") == requested_id
                ),
                key=lambda item: item.get("priority", 999),
            )
            if owners:
                pointer_owner = owners[0]

    target_id = pointer_owner.get("next_work_id") or pointer_owner.get("id")
    target = by_id.get(target_id, pointer_owner)
    lane = pointer_owner.get("next_lane") or target.get("next_lane")
    ctest_filter = pointer_owner.get("next_ctest_filter") or target.get("next_ctest_filter")
    package_target = pointer_owner.get("next_package_target") or target.get("next_package_target")
    package_ctest_filter = pointer_owner.get("next_package_ctest_filter") or target.get("next_package_ctest_filter")
    package_manifest = pointer_owner.get("next_package_manifest") or target.get("next_package_manifest")
    junit_target = pointer_owner.get("next_junit_target") or target.get("next_junit_target")
    junit_artifact = pointer_owner.get("next_junit_artifact") or target.get("next_junit_artifact")
    process_probe_target = pointer_owner.get("next_process_probe_target") or target.get("next_process_probe_target")
    process_package_target = pointer_owner.get("next_process_package_target") or target.get("next_process_package_target")
    process_package_test = pointer_owner.get("next_process_package_test") or target.get("next_process_package_test")
    process_package_ctest_filter = pointer_owner.get("next_process_package_ctest_filter") or target.get("next_process_package_ctest_filter")
    process_package_timestamped_test = pointer_owner.get("next_process_package_timestamped_test") or target.get("next_process_package_timestamped_test")
    process_package_timestamped_ctest_filter = pointer_owner.get("next_process_package_timestamped_ctest_filter") or target.get("next_process_package_timestamped_ctest_filter")
    process_package_parameterized_test = pointer_owner.get("next_process_package_parameterized_test") or target.get("next_process_package_parameterized_test")
    process_package_parameterized_ctest_filter = pointer_owner.get("next_process_package_parameterized_ctest_filter") or target.get("next_process_package_parameterized_ctest_filter")
    process_package_connection_loss_test = pointer_owner.get("next_process_package_connection_loss_test") or target.get("next_process_package_connection_loss_test")
    process_package_connection_loss_ctest_filter = pointer_owner.get("next_process_package_connection_loss_ctest_filter") or target.get("next_process_package_connection_loss_ctest_filter")
    process_package_object_registration_test = pointer_owner.get("next_process_package_object_registration_test") or target.get("next_process_package_object_registration_test")
    process_package_object_registration_ctest_filter = pointer_owner.get("next_process_package_object_registration_ctest_filter") or target.get("next_process_package_object_registration_ctest_filter")
    process_package_named_registration_test = pointer_owner.get("next_process_package_named_registration_test") or target.get("next_process_package_named_registration_test")
    process_package_named_registration_ctest_filter = pointer_owner.get("next_process_package_named_registration_ctest_filter") or target.get("next_process_package_named_registration_ctest_filter")
    process_package_attribute_update_test = pointer_owner.get("next_process_package_attribute_update_test") or target.get("next_process_package_attribute_update_test")
    process_package_attribute_update_ctest_filter = pointer_owner.get("next_process_package_attribute_update_ctest_filter") or target.get("next_process_package_attribute_update_ctest_filter")
    process_package_directed_retraction_test = pointer_owner.get("next_process_package_directed_retraction_test") or target.get("next_process_package_directed_retraction_test")
    process_package_directed_retraction_ctest_filter = pointer_owner.get("next_process_package_directed_retraction_ctest_filter") or target.get("next_process_package_directed_retraction_ctest_filter")
    process_package_catalog_verifier = pointer_owner.get("next_process_package_catalog_verifier") or target.get("next_process_package_catalog_verifier")
    baseline_query = pointer_owner.get("next_test_query") or target.get("next_test_query")
    source_state = source_pointer_state(pointer_owner)
    if source_state == "none" and target is not pointer_owner:
        source_state = source_pointer_state(target)
    source_test_query = pointer_owner.get("next_source_test_query") or target.get("next_source_test_query")
    source_location = pointer_owner.get("next_source_location") or target.get("next_source_location")
    source_lane = pointer_owner.get("next_source_lane") or target.get("next_source_lane")
    source_requirement_ids = strings(
        pointer_owner.get("next_source_requirement_ids")
        or target.get("next_source_requirement_ids")
    )
    source_standard_sections = strings(
        pointer_owner.get("next_source_standard_sections")
        or target.get("next_source_standard_sections")
    )
    source_api_surfaces = strings(
        pointer_owner.get("next_source_api_surfaces")
        or target.get("next_source_api_surfaces")
    )
    source_ctest_filter = pointer_owner.get("next_source_ctest_filter") or target.get("next_source_ctest_filter")
    global_source_queue = (
        unplanned_source_summary(tests, source_locations)
        if source_locations is not None
        else None
    )
    planned_queue = indexed_unlocated_plan_summary(index, tests)
    baseline_matches = [
        test
        for test in tests
        if isinstance(baseline_query, str) and test.get("test_case") == baseline_query
    ]
    baseline = None
    if baseline_matches:
        baseline_test = baseline_matches[0]
        baseline = {
            **test_summary_data(baseline_test),
            **bounded_requirement_ids(baseline_test),
        }
    elif isinstance(baseline_query, str):
        baseline = {
            "test_case": baseline_query,
            "state": "missing",
            "requirement_count": 0,
            "requirement_ids": [],
            "standard_sections": [],
        }

    lane_case_count = None
    lane_snapshot: dict[str, Any] | None = None
    if isinstance(lane, str) and lane:
        lane_case_count = sum(lane in strings(test.get("tags")) for test in tests)
        # Keep the active work card honest when its baseline lane has just
        # become complete.  This is a bounded exact-tag lookup; it does not
        # reopen the whole plan or perform a fuzzy search for replacement work.
        lane_snapshot = focused_lane_result(index, tests, lane, limit=1)

    commands: list[str] = []
    if isinstance(baseline_query, str) and baseline_query:
        commands.append(
            f'python tools/query_rti_work.py trace "{baseline_query}" --summary'
        )
    if (
        isinstance(source_test_query, str)
        and source_test_query
        and source_state != "planned"
    ):
        source_path = None
        if isinstance(source_location, str) and source_location:
            source_path = (
                source_location.rsplit(":", 1)[0]
                if ":" in source_location
                else source_location
            )
        source_command = "python tools/query_rti_work.py unplanned"
        if source_path:
            source_command += f" --path {source_path}"
        commands.append(f"{source_command} --limit 1 --summary")
    if isinstance(lane, str) and lane:
        commands.append(
            f"python tools/query_rti_work.py focus {lane} --summary --compact"
        )
        commands.append(
            f"python tools/query_rti_work.py lane {lane} --summary --compact"
        )
        commands.append(
            f"python tools/query_rti_work.py check --lane {lane} --summary --compact"
        )
    if isinstance(package_target, str) and package_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {package_target}"
        )
    if isinstance(package_ctest_filter, str) and package_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir> -C Debug "
            f"-L {package_ctest_filter} --output-on-failure"
        )
    if isinstance(junit_target, str) and junit_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {junit_target}"
        )
    if isinstance(process_probe_target, str) and process_probe_target:
        commands.append(
            f"cmake --build <build-dir> --config Debug --target {process_probe_target}"
        )
    if isinstance(process_package_target, str) and process_package_target:
        commands.append(
            "cmake --build <build-dir>/package-smoke-consumer --config Debug "
            f"--target {process_package_target}"
        )
    if isinstance(process_package_test, str) and process_package_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_test}$" --output-on-failure'
        )
    if isinstance(process_package_timestamped_test, str) and process_package_timestamped_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_timestamped_test}$" --output-on-failure'
        )
    if isinstance(process_package_timestamped_ctest_filter, str) and process_package_timestamped_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_timestamped_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_parameterized_test, str) and process_package_parameterized_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_parameterized_test}$" --output-on-failure'
        )
    if isinstance(process_package_parameterized_ctest_filter, str) and process_package_parameterized_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_parameterized_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_connection_loss_test, str) and process_package_connection_loss_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_connection_loss_test}$" --output-on-failure'
        )
    if isinstance(process_package_connection_loss_ctest_filter, str) and process_package_connection_loss_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_connection_loss_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_object_registration_test, str) and process_package_object_registration_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_object_registration_test}$" --output-on-failure'
        )
    if isinstance(process_package_object_registration_ctest_filter, str) and process_package_object_registration_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_object_registration_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_named_registration_test, str) and process_package_named_registration_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_named_registration_test}$" --output-on-failure'
        )
    if isinstance(process_package_named_registration_ctest_filter, str) and process_package_named_registration_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_named_registration_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_attribute_update_test, str) and process_package_attribute_update_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_attribute_update_test}$" --output-on-failure'
        )
    if isinstance(process_package_attribute_update_ctest_filter, str) and process_package_attribute_update_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_attribute_update_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_directed_retraction_test, str) and process_package_directed_retraction_test:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_directed_retraction_test}$" --output-on-failure'
        )
    if isinstance(process_package_directed_retraction_ctest_filter, str) and process_package_directed_retraction_ctest_filter:
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_directed_retraction_ctest_filter} --output-on-failure"
        )
    if isinstance(process_package_catalog_verifier, str) and process_package_catalog_verifier:
        commands.append(
            "python tools/verify_process_package_lanes.py --ctest ctest "
            "--test-dir <build-dir>/package-smoke-consumer --config Debug"
        )
    # Every family work card gets the bounded lane inventory handle.  Broad
    # roadmap families can intentionally have no single ``next_lane`` after
    # their indexed source/planned queues are exhausted; the family-scoped
    # ready handle remains useful even when an exact lane is already known,
    # because it reports the same handoff with the family selector applied.
    family_discovery_id = target.get("id") or pointer_owner.get("id")
    family_discovery_command = None
    family_ready_command = None
    if isinstance(family_discovery_id, str) and family_discovery_id:
        family_discovery_command = (
            "python tools/query_rti_work.py lanes "
            f"--family {family_discovery_id} --summary --compact --limit 12"
        )
        commands.append(family_discovery_command)
    if isinstance(family_discovery_id, str) and family_discovery_id:
        family_ready_command = (
            "python tools/query_rti_work.py ready "
            f"--family {family_discovery_id} --summary --compact"
        )
        commands.append(family_ready_command)
    standard_sections = strings(
        pointer_owner.get("next_standard_sections")
        or target.get("next_standard_sections")
    )
    plan_ids = strings(
        pointer_owner.get("next_plan_ids") or target.get("next_plan_ids")
    )
    catalog_gap_plan_ids = strings(
        pointer_owner.get("catalog_gap_plan_ids")
        or target.get("catalog_gap_plan_ids")
    )
    # A priority owner may deliberately point at a narrower work family while
    # retaining an older baseline and historical ``next_task`` prose. Once
    # that target has its own current action, surface the target action as the
    # resumable task so ``resume``/``dashboard`` cannot resurrect stale parent
    # text as the next implementation step.
    target_action = live_next_action(target) if target is not pointer_owner else None
    owner_action = live_next_action(pointer_owner)
    resolved_action = target_action or owner_action
    resolved_task = (
        target_action
        if target_action
        else pointer_owner.get("next_task")
        or pointer_owner.get("next_work_query")
        or target.get("next_action")
    )
    return {
        "found": True,
        "requested_id": requested_id,
        "parent_item": roadmap_item_summary(pointer_owner),
        "work_item": roadmap_item_summary(target),
        "work_id": target.get("id"),
        "work_status": pointer_owner.get("next_work_status") or target.get("status"),
        "task": resolved_task,
        # Prefer the active target-family action when the parent pointer keeps
        # a long historical next_work_query.  This prevents stale snapshot
        # counts from leaking into the one-screen work handoff.
        "work_query": target_action or pointer_owner.get("next_work_query"),
        "next_action": resolved_action,
        "lane": lane,
        "lane_case_count": lane_case_count,
        "lane_state": lane_snapshot.get("lane_state") if lane_snapshot else None,
        "lane_mapped_test_count": (
            lane_snapshot.get("mapped_test_count") if lane_snapshot else None
        ),
        "lane_candidate_count": (
            lane_snapshot.get("candidate_count") if lane_snapshot else None
        ),
        "lane_source_drift_count": (
            lane_snapshot.get("source_drift_count") if lane_snapshot else None
        ),
        "lane_actionable_source_drift_count": (
            lane_snapshot.get("actionable_source_drift_count")
            if lane_snapshot
            else None
        ),
        "lane_assertion_count": (
            lane_snapshot.get("assertion_count") if lane_snapshot else None
        ),
        "ctest_filter": ctest_filter,
        "package_target": package_target,
        "package_ctest_filter": package_ctest_filter,
        "package_manifest": package_manifest,
        "junit_target": junit_target,
        "junit_artifact": junit_artifact,
        "process_probe_target": process_probe_target,
        "process_package_target": process_package_target,
        "process_package_test": process_package_test,
        "process_package_ctest_filter": process_package_ctest_filter,
        "process_package_timestamped_test": process_package_timestamped_test,
        "process_package_timestamped_ctest_filter": process_package_timestamped_ctest_filter,
        "process_package_parameterized_test": process_package_parameterized_test,
        "process_package_parameterized_ctest_filter": process_package_parameterized_ctest_filter,
        "process_package_connection_loss_test": process_package_connection_loss_test,
        "process_package_connection_loss_ctest_filter": process_package_connection_loss_ctest_filter,
        "process_package_object_registration_test": process_package_object_registration_test,
        "process_package_object_registration_ctest_filter": process_package_object_registration_ctest_filter,
        "process_package_named_registration_test": process_package_named_registration_test,
        "process_package_named_registration_ctest_filter": process_package_named_registration_ctest_filter,
        "process_package_attribute_update_test": process_package_attribute_update_test,
        "process_package_attribute_update_ctest_filter": process_package_attribute_update_ctest_filter,
        "process_package_directed_retraction_test": process_package_directed_retraction_test,
        "process_package_directed_retraction_ctest_filter": process_package_directed_retraction_ctest_filter,
        "process_package_catalog_verifier": process_package_catalog_verifier,
        "baseline_test_role": pointer_owner.get("next_test_role") or target.get("next_test_role"),
        "baseline": baseline,
        "next_source_state": source_state,
        "next_source_test_query": source_test_query,
        "next_source_location": source_location,
        "next_source_lane": source_lane,
        "next_source_requirement_ids": source_requirement_ids,
        "next_source_standard_sections": source_standard_sections,
        "next_source_api_surfaces": source_api_surfaces,
        "next_source_ctest_filter": source_ctest_filter,
        "global_source_queue": global_source_queue,
        "planned_queue": planned_queue,
        "standard_sections": standard_sections,
        "plan_ids": plan_ids,
        "catalog_gap_plan_ids": catalog_gap_plan_ids,
        "family_discovery_id": family_discovery_id,
        "family_discovery_command": family_discovery_command,
        "family_ready_command": family_ready_command,
        "commands": commands,
    }


def indexed_item(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    item_id: str,
) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    """Resolve one roadmap item and its tagged tests without scanning prose."""

    folded_id = item_id.casefold()
    for item in index.get("items", []):
        if (
            isinstance(item, dict)
            and str(item.get("id") or "").casefold() == folded_id
        ):
            return item, item_tests(item, tests)
    return None, []


def recent_completed_views(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    lane: str | None = None,
) -> list[dict[str, Any]]:
    """Join the bounded recent-slice ledger to exact Catch2 test records.

    ``ROADMAP-INDEX.json`` keeps a small, human-curated ledger of the latest
    completed slices so work can resume from evidence without reopening the
    full plan.  The test record remains authoritative for source locations and
    requirement/section mappings; this helper only joins the two views.
    """

    views: list[dict[str, Any]] = []
    # The ledger is append-only so that each completion remains easy to audit.
    # A source split can supersede an older historical row without deleting
    # its audit record; those rows stay in the JSON but are omitted from the
    # live view.  Prefer the explicitly designated latest slice, then present
    # the remaining live rows newest-first.  This keeps status/latest and a
    # bounded ``recent --limit`` query aligned even when an older row was
    # repaired after its original append.
    entries = [
        entry
        for entry in index.get("recent_completed_slices", [])
        if isinstance(entry, dict) and not entry.get("superseded_by")
    ]
    if lane:
        entries = [
            entry
            for entry in entries
            if any(
                str(entry_lane).casefold() == lane.casefold()
                for entry_lane in strings(entry.get("lane_queries"))
            )
        ]
    latest = index.get("mapping", {}).get("latest_completed_slice", {})
    latest_plan_id = latest.get("plan_id") if isinstance(latest, dict) else None
    latest_entry = next(
        (entry for entry in entries if entry.get("plan_id") == latest_plan_id),
        None,
    )
    ordered_entries = ([] if latest_entry is None else [latest_entry]) + [
        entry
        for entry in reversed(entries)
        if entry is not latest_entry
    ]
    for entry in ordered_entries:
        if not isinstance(entry, dict):
            continue
        view = dict(entry)
        query = entry.get("test_query")
        matches = [
            test
            for test in tests
            if isinstance(query, str) and test.get("test_case") == query
        ]
        view["matched_test_count"] = len(matches)
        view["tests"] = [test_summary_data(test) for test in matches]
        if matches:
            view["source_locations"] = [
                location
                for test in matches
                for location in test.get("source_locations", [])
            ]
            view["resolved_standard_sections"] = sorted(
                {
                    section
                    for test in matches
                    for section in test.get("standard_sections", [])
                }
            )
            view["resolved_requirement_count"] = sum(
                len(strings(test.get("lab_requirement_ids")))
                for test in matches
            )
        else:
            view.setdefault("source_locations", [])
            view.setdefault("resolved_standard_sections", [])
            view.setdefault("resolved_requirement_count", 0)
        views.append(view)
    return views


def validate_index(
    index: dict[str, Any],
    roadmap_path: Path,
    plan: dict[str, Any],
    standard_requirements: dict[str, dict[str, Any]],
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
    focus_lane: str | None = None,
    focus_family: str | None = None,
    include_historical: bool = False,
) -> list[str]:
    errors: list[str] = []
    if focus_lane is not None and focus_family is not None:
        errors.append("check accepts either focus_lane or focus_family, not both")
    if index.get("schema_version") != 1:
        errors.append("roadmap index schema_version must be 1")
    items = index.get("items")
    if not isinstance(items, list) or not items:
        errors.append("roadmap index must contain at least one item")
        items = []
    try:
        roadmap_text = roadmap_path.read_text(encoding="utf-8")
    except OSError as error:
        errors.append(f"cannot read roadmap {roadmap_path}: {error}")
        roadmap_text = ""
    seen_ids: set[str] = set()
    seen_anchors: set[str] = set()
    known_tags = {
        tag
        for test in plan.get("tests", [])
        if isinstance(test, dict)
        for tag in strings(test.get("tags"))
    }
    # The completion ledger uses ``lane_queries`` for both exact Catch2 tags
    # and roadmap-family handles.  Accept both namespaces so historical
    # entries remain strict-checkable while the query surface can filter by a
    # family id without inventing a duplicate Catch2 tag.
    known_family_ids = {
        str(item.get("id"))
        for item in items
        if isinstance(item, dict) and isinstance(item.get("id"), str)
    }
    known_lane_identifiers = known_tags | known_family_ids
    known_test_ids = {
        test.get("id")
        for test in plan.get("tests", [])
        if isinstance(test, dict) and isinstance(test.get("id"), str)
    }
    known_test_cases = {
        test.get("test_case")
        for test in plan.get("tests", [])
        if isinstance(test, dict) and isinstance(test.get("test_case"), str)
    }
    known_standard_sections = {
        f"{requirement.get('document_id')}:{requirement.get('clause_id')}"
        for requirement in standard_requirements.values()
        if requirement.get("document_id") and requirement.get("clause_id")
    }
    active_handoff = index.get("active_handoff")
    if active_handoff is not None:
        if not isinstance(active_handoff, dict):
            errors.append("roadmap index active_handoff must be an object")
        else:
            for field in (
                "id",
                "state",
                "family",
                "lane",
                "objective",
                "test_case",
                "source_target",
                "mapping_seed_plan_id",
                "ctest_target",
                "ctest_filter",
            ):
                value = active_handoff.get(field)
                if not isinstance(value, str) or not value.strip():
                    errors.append(
                        f"active_handoff {field} must be a non-empty string"
                    )
            family_id = active_handoff.get("family")
            if isinstance(family_id, str) and family_id not in known_family_ids:
                errors.append(
                    "active_handoff family is absent from the roadmap index: "
                    f"{family_id}"
                )
            seed_id = active_handoff.get("mapping_seed_plan_id")
            if isinstance(seed_id, str) and seed_id not in known_test_ids:
                errors.append(
                    "active_handoff mapping_seed_plan_id is absent from the Catch2 plan: "
                    f"{seed_id}"
                )
            commands = active_handoff.get("commands")
            if commands is not None and (
                not isinstance(commands, dict)
                or any(
                    not isinstance(value, str) or not value.strip()
                    for value in commands.values()
                )
            ):
                errors.append(
                    "active_handoff commands must be an object of non-empty strings"
                )
            acceptance = active_handoff.get("acceptance")
            if acceptance is not None and (
                not isinstance(acceptance, list)
                or any(not isinstance(value, str) or not value.strip() for value in acceptance)
            ):
                errors.append(
                    "active_handoff acceptance must contain only non-empty strings"
                )
    if focus_lane is not None and focus_lane.casefold() not in {
        str(tag).casefold() for tag in known_tags
    }:
        errors.append(f"check lane is absent from the Catch2 plan: {focus_lane}")
    family_item, scoped_tests = scoped_plan_tests(
        index,
        tests,
        lane=focus_lane,
        family=focus_family,
    )
    if focus_family is not None and family_item is None:
        errors.append(f"check family is absent from the roadmap index: {focus_family}")
    if focus_lane is not None and focus_family is not None:
        scoped_tests = []
    if focus_family is not None:
        scoped_item_ids = {
            str(family_item.get("id") or "").casefold()
        } if isinstance(family_item, dict) else set()
        items_for_validation = [
            item
            for item in items
            if isinstance(item, dict)
            and str(item.get("id") or "").casefold() in scoped_item_ids
        ]
    elif focus_lane is not None:
        items_for_validation = [
            item
            for item in items
            if isinstance(item, dict) and item_matches_lane(item, focus_lane)
        ]
    else:
        items_for_validation = items
    for item in items_for_validation:
        if not isinstance(item, dict):
            errors.append("roadmap index item must be an object")
            continue
        item_id = item.get("id")
        if not isinstance(item_id, str) or not item_id:
            errors.append("roadmap index item is missing a non-empty id")
        elif item_id in seen_ids:
            errors.append(f"duplicate roadmap index id: {item_id}")
        else:
            seen_ids.add(item_id)
        anchor = item.get("roadmap_anchor")
        if not isinstance(anchor, str) or not anchor:
            errors.append(f"{item_id or '<unnamed>'} is missing roadmap_anchor")
        elif anchor in seen_anchors:
            errors.append(f"duplicate roadmap anchor: {anchor}")
        else:
            seen_anchors.add(anchor)
            if anchor not in roadmap_text:
                errors.append(f"{item_id or '<unnamed>'} anchor is absent from {roadmap_path}")
        if item.get("status") not in {"open", "complete", "blocked"}:
            errors.append(f"{item_id or '<unnamed>'} has an invalid status")
        for field in (
            "next_work_id",
            "next_work_status",
            "next_work_query",
            "next_lane",
            "next_test_query",
            "next_source_state",
            "next_source_test_query",
            "next_source_location",
            "next_source_lane",
            "next_source_ctest_filter",
            "next_ctest_filter",
            "next_package_target",
            "next_package_ctest_filter",
            "next_package_manifest",
            "next_junit_target",
            "next_junit_artifact",
            "next_process_probe_target",
            "next_process_package_target",
            "next_process_package_test",
            "next_process_package_ctest_filter",
            "next_process_package_timestamped_test",
            "next_process_package_timestamped_ctest_filter",
            "next_process_package_parameterized_test",
            "next_process_package_parameterized_ctest_filter",
            "next_process_package_connection_loss_test",
            "next_process_package_connection_loss_ctest_filter",
            "next_process_package_object_registration_test",
            "next_process_package_object_registration_ctest_filter",
            "next_process_package_named_registration_test",
            "next_process_package_named_registration_ctest_filter",
            "next_process_package_attribute_update_test",
            "next_process_package_attribute_update_ctest_filter",
            "next_process_package_directed_retraction_test",
            "next_process_package_directed_retraction_ctest_filter",
            "next_process_package_catalog_verifier",
        ):
            value = item.get(field)
            if value is not None and (not isinstance(value, str) or not value.strip()):
                errors.append(f"{item_id or '<unnamed>'} {field} must be a non-empty string")
        next_work_status = item.get("next_work_status")
        if next_work_status is not None and next_work_status not in {
            "planned",
            "in-progress",
            "blocked",
            "complete",
        }:
            errors.append(
                f"{item_id or '<unnamed>'} next_work_status must be planned, in-progress, blocked, or complete"
            )
        if next_work_status is not None and item.get("next_work_id") is None:
            errors.append(
                f"{item_id or '<unnamed>'} next_work_status requires next_work_id"
            )
        next_task = item.get("next_task")
        if next_task is not None and (not isinstance(next_task, str) or not next_task.strip()):
            errors.append(f"{item_id or '<unnamed>'} next_task must be a non-empty string")
        next_lane = item.get("next_lane")
        if isinstance(next_lane, str) and next_lane not in known_tags:
            errors.append(f"{item_id or '<unnamed>'} next_lane is absent from the Catch2 plan: {next_lane}")
        next_source_lane = item.get("next_source_lane")
        if isinstance(next_source_lane, str) and next_source_lane not in known_tags:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_lane is absent from the Catch2 plan: {next_source_lane}"
            )
        next_test_query = item.get("next_test_query")
        if isinstance(next_test_query, str) and next_test_query not in known_test_cases:
            errors.append(f"{item_id or '<unnamed>'} next_test_query is absent from the Catch2 plan: {next_test_query}")
        next_source_test_query = item.get("next_source_test_query")
        next_source_location = item.get("next_source_location")
        next_source_state = item.get("next_source_state")
        if next_source_test_query is not None and not isinstance(next_source_test_query, str):
            errors.append(f"{item_id or '<unnamed>'} next_source_test_query must be a non-empty string")
        if next_source_location is not None and not isinstance(next_source_location, str):
            errors.append(f"{item_id or '<unnamed>'} next_source_location must be a non-empty string")
        if next_source_state is not None and next_source_state not in {
            "unplanned-source",
            "planned",
            "exhausted",
        }:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_state must be unplanned-source, planned, or exhausted"
            )
        if isinstance(next_source_test_query, str) and next_source_test_query.strip():
            if not isinstance(next_source_location, str) or not next_source_location.strip():
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_test_query requires next_source_location"
                )
            elif source_locations is not None:
                expected_source_locations = {
                    f"{location.get('path')}:{location.get('line')}"
                    for location in source_locations.get(next_source_test_query, [])
                    if isinstance(location, dict)
                    and isinstance(location.get("path"), str)
                    and location.get("line") is not None
                }
                # ``planned`` is an explicit future-test pointer.  Its file
                # location is a source target, not a claim that the TEST_CASE
                # already exists.  Existing unplanned-source pointers retain
                # the strict derived-location check.
                if (
                    next_source_state != "planned"
                    and next_source_location not in expected_source_locations
                ):
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_location does not match "
                        f"the derived TEST_CASE location: {next_source_location}"
                    )
        elif next_source_location is not None:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_location requires next_source_test_query"
            )
        next_test_role = item.get("next_test_role")
        if next_test_role is not None and next_test_role not in {"planned", "baseline"}:
            errors.append(
                f"{item_id or '<unnamed>'} next_test_role must be planned or baseline"
            )
        if next_test_role == "baseline" and not isinstance(next_test_query, str):
            errors.append(
                f"{item_id or '<unnamed>'} next_test_role=baseline requires next_test_query"
            )
        next_ctest_filter = item.get("next_ctest_filter")
        if isinstance(next_ctest_filter, str):
            if not (next_ctest_filter.startswith("[") and next_ctest_filter.endswith("]")):
                errors.append(f"{item_id or '<unnamed>'} next_ctest_filter must be a Catch2 tag filter: {next_ctest_filter}")
            elif next_ctest_filter[1:-1] not in known_tags:
                errors.append(f"{item_id or '<unnamed>'} next_ctest_filter is absent from the Catch2 plan: {next_ctest_filter}")
        next_source_ctest_filter = item.get("next_source_ctest_filter")
        if isinstance(next_source_ctest_filter, str):
            if not (
                next_source_ctest_filter.startswith("[")
                and next_source_ctest_filter.endswith("]")
            ):
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_ctest_filter must be a Catch2 tag filter: {next_source_ctest_filter}"
                )
            elif next_source_ctest_filter[1:-1] not in known_tags:
                errors.append(
                    f"{item_id or '<unnamed>'} next_source_ctest_filter is absent from the Catch2 plan: {next_source_ctest_filter}"
                )
        for field in (
            "next_standard_sections",
            "next_plan_ids",
            "next_source_requirement_ids",
            "next_source_standard_sections",
            "next_source_api_surfaces",
        ):
            value = item.get(field)
            if value is not None and not isinstance(value, list):
                errors.append(f"{item_id or '<unnamed>'} {field} must be an array")
            elif isinstance(value, list) and any(
                not isinstance(entry, str) or not entry.strip() for entry in value
            ):
                errors.append(f"{item_id or '<unnamed>'} {field} must contain only non-empty strings")
        next_standard_sections = item.get("next_standard_sections")
        if isinstance(next_standard_sections, list):
            for section in next_standard_sections:
                if isinstance(section, str) and section not in known_standard_sections:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_standard_sections references unknown 2025 section: {section}"
                    )
        next_plan_ids = item.get("next_plan_ids")
        if isinstance(next_plan_ids, list):
            for plan_id in next_plan_ids:
                if isinstance(plan_id, str) and plan_id not in known_test_ids:
                    errors.append(f"{item_id or '<unnamed>'} next_plan_ids references unknown Catch2 plan id: {plan_id}")
        next_source_requirement_ids = item.get("next_source_requirement_ids")
        if isinstance(next_source_requirement_ids, list):
            for requirement_id in next_source_requirement_ids:
                if isinstance(requirement_id, str) and requirement_id not in standard_requirements:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_requirement_ids references unknown 2025 requirement: {requirement_id}"
                    )
        next_source_standard_sections = item.get("next_source_standard_sections")
        if isinstance(next_source_standard_sections, list):
            for section in next_source_standard_sections:
                if isinstance(section, str) and section not in known_standard_sections:
                    errors.append(
                        f"{item_id or '<unnamed>'} next_source_standard_sections references unknown 2025 section: {section}"
                    )
        catalog_gap_plan_ids = item.get("catalog_gap_plan_ids")
        if catalog_gap_plan_ids is not None and not isinstance(catalog_gap_plan_ids, list):
            errors.append(f"{item_id or '<unnamed>'} catalog_gap_plan_ids must be an array")
        elif isinstance(catalog_gap_plan_ids, list):
            for plan_id in catalog_gap_plan_ids:
                if not isinstance(plan_id, str) or not plan_id.strip():
                    errors.append(
                        f"{item_id or '<unnamed>'} catalog_gap_plan_ids must contain only non-empty strings"
                    )
                elif plan_id not in known_test_ids:
                    errors.append(
                        f"{item_id or '<unnamed>'} catalog_gap_plan_ids references unknown Catch2 plan id: {plan_id}"
                    )
        item_tags = strings(item.get("query_tags")) + strings(item.get("focused_lane_tags"))
        if focus_lane is not None:
            item_tags = [
                tag
                for tag in item_tags
                if tag.casefold() == focus_lane.casefold()
            ]
        for tag in item_tags:
            if tag not in known_tags:
                errors.append(f"{item_id or '<unnamed>'} query tag is absent from the Catch2 plan: {tag}")
    mapped_tests_by_id = {
        test.get("id"): test
        for test in tests
        if isinstance(test, dict) and isinstance(test.get("id"), str)
    }
    for test in plan.get("tests", []):
        if not isinstance(test, dict):
            continue
        legacy_requirement_ids = strings(test.get("requirements_lab_requirement_ids"))
        if legacy_requirement_ids:
            errors.append(
                f"Catch2 test {test.get('id', '<unnamed>')} uses legacy requirements_lab_requirement_ids; "
                "use selected_requirements_lab_requirement_ids"
            )
        test_id = test.get("id", "<unnamed>")
        for field in (
            "selected_requirements_lab_requirement_ids",
            "selected_cpp_api_surface_ids",
            "standard_sections",
            "tags",
        ):
            values = strings(test.get(field))
            duplicates = sorted({value for value in values if values.count(value) > 1})
            if duplicates:
                errors.append(
                    f"Catch2 test {test_id} repeats {field}: "
                    + ", ".join(duplicates)
                )
        for requirement_id in strings(test.get("selected_requirements_lab_requirement_ids")):
            requirement = standard_requirements.get(requirement_id)
            if requirement is None:
                errors.append(
                    f"Catch2 test {test_id} references unknown 2025 requirement {requirement_id}"
                )
                continue
            # Keep the test row's explicit section list synchronized with the
            # canonical requirement-to-subsection join.  Without this guard a
            # row can select a valid Lab requirement while silently omitting
            # its 2025 document:clause key, forcing every consumer to infer
            # the missing relationship again.
            document_id = requirement.get("document_id")
            clause_id = requirement.get("clause_id")
            canonical_section = (
                f"{document_id}:{clause_id}"
                if isinstance(document_id, str)
                and document_id
                and isinstance(clause_id, str)
                and clause_id
                else None
            )
            mapped_test = mapped_tests_by_id.get(test_id)
            mapped_sections = (
                strings(mapped_test.get("standard_sections"))
                if isinstance(mapped_test, dict)
                else []
            )
            if canonical_section is not None and canonical_section not in mapped_sections:
                errors.append(
                    f"Catch2 test {test_id} omits canonical 2025 standard section "
                    f"{canonical_section} for requirement {requirement_id}"
                )
    recent_slices = index.get("recent_completed_slices", [])
    if recent_slices is not None and not isinstance(recent_slices, list):
        errors.append("roadmap index recent_completed_slices must be an array")
        recent_slices = []
    # The completion ledger is append-only audit history.  Source files and
    # plan rows can be deliberately split or replaced after a slice is
    # recorded, so stale historical handles must not block the normal
    # live-plan integrity gate.  ``check --historical`` opts back into the
    # strict ledger validation when an audit needs to inspect every row.
    if not include_historical:
        recent_slices = []
    known_test_ids = {
        test.get("id")
        for test in tests
        if isinstance(test.get("id"), str)
    }
    scoped_test_ids = {
        test.get("id")
        for test in scoped_tests
        if isinstance(test.get("id"), str)
    }
    scoped_test_cases = {
        test.get("test_case")
        for test in scoped_tests
        if isinstance(test.get("test_case"), str)
    }
    seen_recent_plan_ids: set[str] = set()
    seen_recent_queries: set[str] = set()
    for position, slice_value in enumerate(recent_slices):
        label = f"recent_completed_slices[{position}]"
        if not isinstance(slice_value, dict):
            errors.append(f"{label} must be an object")
            continue
        # Retain superseded rows for auditability, but do not require their
        # historical plan ids/source locations to remain live after a source
        # split or plan-row replacement.  The active row carries the current
        # exact test, mapping, and CTest handles.
        if slice_value.get("superseded_by"):
            continue
        if focus_lane is not None and not any(
            str(lane).casefold() == focus_lane.casefold()
            for lane in strings(slice_value.get("lane_queries"))
        ):
            continue
        if focus_family is not None:
            if (
                slice_value.get("plan_id") not in scoped_test_ids
                and slice_value.get("test_query") not in scoped_test_cases
            ):
                continue
        plan_id = slice_value.get("plan_id")
        if not isinstance(plan_id, str) or not plan_id.strip():
            errors.append(f"{label} is missing plan_id")
        elif plan_id not in known_test_ids:
            errors.append(f"{label} references unknown Catch2 plan id: {plan_id}")
        elif plan_id in seen_recent_plan_ids:
            errors.append(f"{label} duplicates recent completion plan_id: {plan_id}")
        else:
            seen_recent_plan_ids.add(plan_id)
        test_query = slice_value.get("test_query")
        if not isinstance(test_query, str) or not test_query.strip():
            errors.append(f"{label} is missing test_query")
            matches: list[dict[str, Any]] = []
        else:
            matches = [
                test
                for test in tests
                if test.get("test_case") == test_query
            ]
            if len(matches) != 1:
                errors.append(
                    f"{label} test_query must resolve to exactly one Catch2 case; "
                    f"found {len(matches)}"
                )
            elif isinstance(plan_id, str) and matches[0].get("id") != plan_id:
                errors.append(
                    f"{label} plan_id does not match its exact test_query: {plan_id}"
                )
        if isinstance(test_query, str) and test_query.strip():
            if test_query in seen_recent_queries:
                errors.append(f"{label} duplicates recent completion test_query: {test_query}")
            else:
                seen_recent_queries.add(test_query)
        source_location = slice_value.get("source_location")
        expected_locations = {
            f"{location.get('path')}:{location.get('line')}"
            for test in matches
            for location in test.get("source_locations", [])
            if isinstance(location, dict)
            and isinstance(location.get("path"), str)
            and location.get("line") is not None
        }
        # ``mapped_test`` intentionally prefers an explicit plan source when
        # duplicate TEST_CASE titles exist in a focused extraction and its
        # historical aggregate.  The append-only recent ledger may still
        # point at the aggregate declaration, so validation accepts every
        # currently derived declaration while trace/matrix output stays on
        # the preferred executable source.
        if source_locations is not None:
            for test in matches:
                expected_locations.update(
                    {
                        f"{location.get('path')}:{location.get('line')}"
                        for location in source_locations_for_test(
                            test.get("test_case"), source_locations
                        )
                        if isinstance(location, dict)
                        and isinstance(location.get("path"), str)
                        and location.get("line") is not None
                    }
                )
        if not isinstance(source_location, str) or not source_location.strip():
            errors.append(f"{label} is missing source_location")
        elif not expected_locations:
            # Historical plan rows whose declaration is absent remain visible
            # in the result's source-drift count, but they must not block
            # validation of source-backed cases.  The source-location check
            # below still rejects a different file; only a missing declaration
            # is retained as historical drift.
            if focus_lane is None and focus_family is None:
                errors.append(f"{label} has no derived C++ source location")
        elif source_location not in expected_locations:
            expected_paths = {
                location.rsplit(":", 1)[0]
                for location in expected_locations
                if ":" in location
            }
            source_path = source_location.rsplit(":", 1)[0]
            if source_path not in expected_paths:
                errors.append(
                    f"{label} source_location does not match the derived TEST_CASE location: "
                    f"{source_location}"
                )
        assertions = slice_value.get("assertions")
        if not isinstance(assertions, int) or assertions <= 0:
            errors.append(f"{label} assertions must be a positive integer")
        status = slice_value.get("status")
        if not isinstance(status, str) or not status.strip():
            errors.append(f"{label} is missing status")
        sections = slice_value.get("standard_sections")
        if not isinstance(sections, list) or any(
            not isinstance(section, str) or not section.strip()
            for section in sections
        ):
            errors.append(f"{label} standard_sections must contain non-empty strings")
        elif any(section not in known_standard_sections for section in sections):
            errors.append(f"{label} standard_sections references an unknown 2025 section")
        lanes = slice_value.get("lane_queries")
        if not isinstance(lanes, list) or any(
            not isinstance(lane, str) or not lane.strip() for lane in lanes
        ):
            errors.append(f"{label} lane_queries must contain non-empty strings")
        else:
            for lane in lanes:
                if lane not in known_lane_identifiers:
                    errors.append(
                        f"{label} lane_queries references an unknown Catch2 tag: {lane}"
                    )
    if (
        include_historical
        and source_locations is not None
        and focus_lane is None
        and focus_family is None
    ):
        source_queue = unplanned_source_cases(tests, source_locations)
        unplanned_pointers = [
            item
            for item in items
            if isinstance(item, dict)
            and item.get("next_source_state") == "unplanned-source"
            and isinstance(item.get("next_source_test_query"), str)
            and item.get("next_source_test_query", "").strip()
        ]
        if len(unplanned_pointers) > 1:
            errors.append(
                "roadmap index must expose at most one unplanned-source queue pointer"
            )
        if source_queue and len(unplanned_pointers) == 1:
            pointer = unplanned_pointers[0]
            queue_head = source_queue[0]
            queue_head_query = queue_head.get("test_case")
            queue_head_location = source_location_text(queue_head)
            pointer_query = pointer.get("next_source_test_query")
            pointer_location = pointer.get("next_source_location")
            if pointer_query != queue_head_query or pointer_location != queue_head_location:
                errors.append(
                    "roadmap index unplanned-source pointer must match the global "
                    f"queue head ({queue_head_query} @ {queue_head_location})"
                )
        elif source_queue and not unplanned_pointers:
            errors.append(
                "roadmap index must expose the global unplanned-source queue head"
            )
        elif not source_queue and unplanned_pointers:
            errors.append(
                "roadmap index has an unplanned-source pointer but the source queue is exhausted"
            )
    for test in scoped_tests:
        source_missing = test.get("status") == "source-missing-needs-reconciliation"
        disabled_artifact = test.get("status") == "disabled-source-artifact"
        planned = is_planned_test(test)
        if source_missing and test.get("source_locations"):
            errors.append(
                f"Catch2 test {test.get('id', '<unnamed>')} is marked source-missing but has a TEST_CASE source location"
            )
        elif (
            not source_missing
            and not disabled_artifact
            and not planned
            and not test.get("source_locations")
        ):
            if focus_lane is None and focus_family is None:
                errors.append(
                    f"Catch2 test {test.get('id', '<unnamed>')} has no matching TEST_CASE source location"
                )
    for document in index.get("documents", []):
        if not isinstance(document, dict):
            errors.append("roadmap index document entry must be an object")
            continue
        document_path = document.get("path")
        if not isinstance(document_path, str) or not (REPOSITORY_ROOT / document_path).is_file():
            errors.append(f"roadmap index document is missing: {document_path}")
    return errors


def text_test(test: dict[str, Any], verbose: bool, compact: bool = False) -> str:
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status: {test.get('status', '<unspecified>')}",
        f"  mapping id: {test.get('requirements_lab_mapping_id') or '<none>'}",
        f"  traceability: {traceability_state(test)}",
        f"  tags: {', '.join(test.get('tags', [])) or '<none>'}",
        f"  source: {source_location_text(test)}",
        f"  standard: {', '.join(test.get('standard_clauses', [])) or '<unmapped>'}",
    ]
    failure_location = test.get("failure_location")
    failure_summary = test.get("failure_summary")
    if isinstance(failure_location, str) and failure_location.strip():
        lines.append(f"  failure_location: {failure_location}")
    if isinstance(failure_summary, str) and failure_summary.strip():
        lines.append(f"  failure_summary: {compact_prose(failure_summary, 240)}")
    if any(
        isinstance(test.get(field), int)
        for field in (
            "passed_assertions",
            "failed_assertions",
            "expected_assertions_if_harness_fixed",
        )
    ):
        lines.append(
            "  assertion_result: "
            f"observed={test.get('assertions', '<unspecified>')}; "
            f"passed={test.get('passed_assertions', '<unspecified>')}; "
            f"failed={test.get('failed_assertions', '<unspecified>')}; "
            f"expected_after_harness_fix={test.get('expected_assertions_if_harness_fixed', '<unspecified>')}"
        )
    api_surface_status = test.get("requirements_lab_api_surface_status")
    if isinstance(api_surface_status, str) and api_surface_status.strip():
        lines.append(f"  Requirements-Lab API surface: {api_surface_status}")
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        lines.append(f"  source-missing reason: {compact_prose(reason, 180)}")
    api_surface_ids = strings(test.get("selected_cpp_api_surface_ids"))
    if api_surface_ids:
        lines.append(f"  cpp api surfaces: {', '.join(api_surface_ids)}")
    if compact:
        if test.get("requirements"):
            lines.append("  requirements:")
            for requirement in test.get("requirements", []):
                standard = requirement.get("standard")
                if standard is None:
                    lines.append(f"    - {requirement['lab_requirement_id']} (unresolved)")
                    continue
                section = f"{standard.get('document_id')} {standard.get('clause_id')}"
                lines.append(f"    - {requirement['lab_requirement_id']} -> {section}")
        else:
            lines.append("  requirements: <unmapped>")
    else:
        for requirement in test.get("requirements", []):
            standard = requirement.get("standard")
            if standard is None:
                lines.append(f"  requirement: {requirement['lab_requirement_id']} (unresolved)")
                continue
            section = f"{standard.get('document_id')} {standard.get('clause_id')}"
            source = standard.get("source") or {}
            source_text = source.get("path") if isinstance(source, dict) else None
            if isinstance(source, dict) and source.get("line") is not None:
                source_text = f"{source_text}:{source['line']}"
            lines.append(
                f"  requirement: {requirement['lab_requirement_id']} -> {section}"
                f"; {standard.get('title', '')}"
            )
            if source_text:
                lines.append(f"    standard source: {source_text}")
            for contract in requirement.get("contracts", []):
                symbol = contract.get("source_symbol")
                if symbol:
                    lines.append(f"    contract: {contract.get('contract')} ({symbol})")
    contract_candidates = test.get("contract_candidates")
    if isinstance(contract_candidates, list):
        lines.append("  contract candidates:")
        if not contract_candidates:
            lines.append("    - <none>")
        else:
            shown_candidates = contract_candidates[:8]
            for candidate in shown_candidates:
                if not isinstance(candidate, dict):
                    continue
                lab_id = candidate.get("lab_requirement_id", "<unnamed requirement>")
                clause = candidate.get("clause_id") or "<unresolved clause>"
                contract = candidate.get("contract") or "<unnamed contract>"
                source_note = ""
                if candidate.get("source_path_match") is False:
                    source_note = (
                        "; source mismatch: "
                        + str(candidate.get("source_path") or "<unnamed source>")
                    )
                lines.append(f"    - {lab_id} -> {clause}; {contract}{source_note}")
            remaining = len(contract_candidates) - len(shown_candidates)
            if remaining > 0:
                lines.append(f"    - ... (+{remaining}); use --json for all candidates")
    if verbose and test.get("next_action"):
        lines.append(f"  next: {test['next_action']}")
    return "\n".join(lines)


def text_test_summary(test: dict[str, Any]) -> str:
    """Render one bounded line-oriented record for fast work selection."""

    requirements = strings(test.get("lab_requirement_ids"))
    sections = strings(test.get("standard_sections"))
    api_surfaces = strings(
        test.get("selected_cpp_api_surface_ids") or test.get("cpp_api_surfaces")
    )
    raw_tags = strings(test.get("tags"))
    service_tags = strings(test.get("service_tags")) or [
        tag for tag in raw_tags if tag.startswith("rti.service.")
    ]
    callback_tags = strings(test.get("callback_tags")) or [
        tag for tag in raw_tags if tag.startswith("federate.callback.")
    ]
    mapping = test.get("requirements_lab_mapping_id") or "<none>"
    # Keep the summary useful for traceability without allowing a highly
    # cross-cutting case to expand into a context-sized report.  The complete
    # values remain available from ``--json`` or the non-summary ``--compact``
    # view.
    preview_limit = 8

    def preview(values: list[str]) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:preview_limit])
        remaining = len(values) - preview_limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    mapping_rows = requirement_section_mapping_rows(test)
    mapping_preview = [
        f"{row.get('lab_requirement_id', '<unnamed requirement>')} -> "
        f"{row.get('standard_section') or '<unresolved>'}"
        for row in mapping_rows
        if isinstance(row, dict)
    ]

    focus_tags = strings(test.get("focus_tags")) or [
        tag
        for tag in raw_tags
        if not tag.startswith("rti.service.")
        and not tag.startswith("federate.callback.")
    ]
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status: {test.get('status', '<unspecified>')}",
        f"  source: {source_location_text(test)}",
        f"  assertions: {test.get('assertions', '<unspecified>')}; "
        f"callback_models={', '.join(strings(test.get('callback_models'))) or '<unspecified>'}; "
        f"delivery_modes={', '.join(strings(test.get('delivery_modes'))) or '<unspecified>'}; "
        f"callback_gate_modes={', '.join(strings(test.get('callback_gate_modes'))) or '<unspecified>'}",
        f"  mapping id: {mapping}; traceability={traceability_state(test)}; "
        f"requirements={len(requirements)}; sections={len(sections)}; "
        f"cpp_api_surfaces={len(api_surfaces)}",
        f"  requirements: {preview(requirements)}",
        f"  standard sections: {preview(sections)}",
        f"  requirement -> standard subsection: {preview(mapping_preview)}",
        f"  cpp api surfaces: {preview(api_surfaces)}",
        f"  service tags: {', '.join(service_tags) or '<none>'}",
        f"  callback tags: {', '.join(callback_tags) or '<none>'}",
        f"  focus tags: {', '.join(focus_tags) or '<none>'}",
    ]
    contract_candidates = test.get("contract_candidates")
    if isinstance(contract_candidates, list):
        if contract_candidates:
            candidate_preview = [
                f"{candidate.get('lab_requirement_id', '<unnamed requirement>')} -> "
                f"{candidate.get('clause_id') or '<unresolved clause>'}"
                + (
                    " [source-mismatch]"
                    if candidate.get("source_path_match") is False
                    else ""
                )
                for candidate in contract_candidates[:8]
                if isinstance(candidate, dict)
            ]
            remaining = len(contract_candidates) - len(candidate_preview)
            suffix = f", ... (+{remaining})" if remaining > 0 else ""
            lines.append(
                "  contract candidates: "
                + (", ".join(candidate_preview) or "<none>")
                + suffix
            )
        else:
            lines.append("  contract candidates: <none>")
    failure_location = test.get("failure_location")
    failure_summary = test.get("failure_summary")
    if isinstance(failure_location, str) and failure_location.strip():
        lines.append(f"  failure_location: {failure_location}")
    if isinstance(failure_summary, str) and failure_summary.strip():
        lines.append(f"  failure_summary: {compact_prose(failure_summary, 240)}")
    if any(
        isinstance(test.get(field), int)
        for field in (
            "passed_assertions",
            "failed_assertions",
            "expected_assertions_if_harness_fixed",
        )
    ):
        lines.append(
            "  assertion_result: "
            f"observed={test.get('assertions', '<unspecified>')}; "
            f"passed={test.get('passed_assertions', '<unspecified>')}; "
            f"failed={test.get('failed_assertions', '<unspecified>')}; "
            f"expected_after_harness_fix={test.get('expected_assertions_if_harness_fixed', '<unspecified>')}"
        )
    api_surface_status = test.get("requirements_lab_api_surface_status")
    if isinstance(api_surface_status, str) and api_surface_status.strip():
        lines.insert(
            8,
            "  Requirements-Lab API surface: "
            + compact_prose(api_surface_status, 360),
        )
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        lines.append(f"  source-missing reason: {compact_prose(reason, 180)}")
    return "\n".join(lines)


def test_summary_data(test: dict[str, Any]) -> dict[str, Any]:
    """Return the bounded machine-readable shape used by ``--summary``."""

    mapping_rows = requirement_section_mapping_rows(test)
    result = {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "status": test.get("status"),
        "source_locations": test.get("source_locations", []),
        "source_state": test.get("source_state"),
        "assertions": test.get("assertions"),
        "passed_assertions": test.get("passed_assertions"),
        "failed_assertions": test.get("failed_assertions"),
        "expected_assertions_if_harness_fixed": test.get(
            "expected_assertions_if_harness_fixed"
        ),
        "failure_location": test.get("failure_location"),
        "failure_summary": test.get("failure_summary"),
        "callback_models": strings(test.get("callback_models")),
        "delivery_modes": strings(test.get("delivery_modes")),
        "callback_gate_modes": strings(test.get("callback_gate_modes")),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "requirements_lab_api_surface_status": test.get(
            "requirements_lab_api_surface_status"
        ),
        "traceability_state": traceability_state(test),
        "lab_requirement_ids": strings(test.get("lab_requirement_ids")),
        "requirement_count": len(strings(test.get("lab_requirement_ids"))),
        "standard_sections": strings(test.get("standard_sections")),
        "requirement_section_mapping_count": len(mapping_rows),
        "requirement_section_mappings": mapping_rows,
        "cpp_api_surface_count": len(
            strings(test.get("selected_cpp_api_surface_ids"))
        ),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "service_tags": [
            tag
            for tag in strings(test.get("tags"))
            if tag.startswith("rti.service.")
        ],
        "callback_tags": [
            tag
            for tag in strings(test.get("tags"))
            if tag.startswith("federate.callback.")
        ],
        "focus_tags": [
            tag
            for tag in strings(test.get("tags"))
            if not tag.startswith("rti.service.")
            and not tag.startswith("federate.callback.")
        ],
    }
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        result["source_missing_reason"] = reason
    if "contract_candidates" in test:
        result["contract_candidates"] = test.get("contract_candidates")
    return result


def focused_lane_result(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    lane: str,
    limit: int = 8,
) -> dict[str, Any]:
    """Resolve one lane into a bounded, actionable work card.

    ``lane`` is deliberately an exact Catch2 tag.  This view is different
    from ``coverage``: it separates executable work from historical rows and
    tells a caller when every source-located case in the lane is already
    implemented.  That prevents a completed baseline from being mistaken for
    the next task and avoids reopening the full plan just to choose a focus
    slice.  Disabled source artifacts remain visible in the result but are not
    executable candidates.
    """

    folded_lane = lane.casefold()
    selected = [
        test
        for test in tests
        if any(
            str(tag).casefold() == folded_lane
            for tag in strings(test.get("tags"))
        )
    ]
    status_counts = Counter(
        str(test.get("status") or "<unspecified>") for test in selected
    )
    implemented = [
        test
        for test in selected
        if str(test.get("status") or "").startswith(("implemented", "verified"))
    ]
    disabled_artifacts = [
        test
        for test in selected
        if str(test.get("status") or "") == "disabled-source-artifact"
    ]
    source_reconciliation = [
        test
        for test in selected
        if str(test.get("status") or "") == "source-missing-needs-reconciliation"
    ]
    planned = [test for test in selected if is_planned_test(test)]
    executable_candidates = [
        test
        for test in selected
        if not str(test.get("status") or "").startswith(("implemented", "verified"))
        and not is_non_executable_source_status(test)
        and test.get("source_locations")
    ]
    # A planned row is intentionally source-unlocated; keep it in the
    # planned queue rather than classifying it as source drift.
    source_drift = [
        test
        for test in selected
        if not test.get("source_locations") and not is_planned_test(test)
    ]
    actionable_source_drift = [
        test
        for test in source_drift
        if not is_non_executable_source_status(test)
    ]
    mapped = [test for test in selected if test.get("requirements")]
    explicit_dispositions = [
        test
        for test in selected
        if not test.get("requirements")
        and traceability_state(test) == "explicit-disposition"
    ]
    unclassified = [
        test
        for test in selected
        if not test.get("requirements")
        and traceability_state(test) == "unclassified"
    ]
    standard_sections = sorted(
        {
            section
            for test in selected
            for section in strings(test.get("standard_sections"))
        }
    )
    recorded_assertion_count = sum(
        value
        for test in selected
        for value in [test.get("assertions")]
        if isinstance(value, int)
    )
    executable_assertion_count = sum(
        value
        for test in executable_candidates
        for value in [test.get("assertions")]
        if isinstance(value, int)
    )
    requirement_ids = sorted(
        {
            requirement_id
            for test in selected
            for requirement_id in strings(test.get("lab_requirement_ids"))
        }
    )
    requirement_section_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for test in selected
        for row in requirement_section_mapping_rows(test)
        if isinstance(row, dict) and row.get("lab_requirement_id")
    }
    mapping_rows: list[dict[str, Any]] = []
    seen_mapping_pairs: set[tuple[Any, Any]] = set()
    for test in selected:
        for row in requirement_section_mapping_rows(test):
            if not isinstance(row, dict) or not row.get("lab_requirement_id"):
                continue
            pair = (row.get("lab_requirement_id"), row.get("standard_section"))
            if pair in seen_mapping_pairs:
                continue
            seen_mapping_pairs.add(pair)
            mapping_rows.append(
                {
                    "lab_requirement_id": row.get("lab_requirement_id"),
                    "standard_section": row.get("standard_section"),
                    "title": row.get("title"),
                    "unresolved": bool(row.get("unresolved")),
                }
            )
    mapping_rows.sort(
        key=lambda row: (
            str(row.get("lab_requirement_id") or ""),
            str(row.get("standard_section") or ""),
        )
    )
    mapping = index.get("mapping", {})
    if not isinstance(mapping, dict):
        mapping = {}
    configured_assertion_count = indexed_lane_assertion_count(index, lane)
    assertion_count = (
        configured_assertion_count
        if isinstance(configured_assertion_count, int)
        else recorded_assertion_count
    )
    assertion_count_source = (
        "indexed-lane-total"
        if isinstance(configured_assertion_count, int)
        else "plan-entry-records"
    )
    lane_owners = mapping.get("lane_owners", {})
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    configured_handles = mapping.get("lane_handles", {})
    if not isinstance(configured_handles, dict):
        configured_handles = {}
    lane_handles = configured_handles.get(lane)
    if not isinstance(lane_handles, dict) or not lane_handles:
        lane_handles = next(
            (
                handles
                for name, handles in configured_handles.items()
                if str(name).casefold() == folded_lane and isinstance(handles, dict)
            ),
            {},
        )
    if not isinstance(lane_handles, dict):
        lane_handles = {}
    execution_gate = str(lane_handles.get("execution_gate") or "").strip()
    roadmap_owner = lane_owners.get(lane)
    if roadmap_owner is None:
        roadmap_owner = next(
            (
                owner
                for name, owner in lane_owners.items()
                if str(name).casefold() == folded_lane
            ),
            None,
        )
    lane_state = (
        "missing"
        if not selected
        else "execution-blocked"
        if execution_gate.casefold().startswith("blocked:")
        else "needs-mapping"
        if unclassified
        else "planned"
        if planned
        else "source-drift"
        if actionable_source_drift
        else "complete"
        if not executable_candidates
        else "has-executable-candidates"
    )
    owner_item = next(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("id") == roadmap_owner
        ),
        {},
    )
    action_state = queue_action_state(
        owner_item,
        candidate_count=len(executable_candidates),
        unclassified_count=len(unclassified),
        actionable_source_drift_count=len(actionable_source_drift),
        lane_state="complete-pointer" if lane_state == "complete" else lane_state,
    )
    candidate_limit = max(limit, 0)
    candidates = executable_candidates if candidate_limit == 0 else executable_candidates[:candidate_limit]
    result = {
        "lane": lane,
        "roadmap_owner": roadmap_owner,
        "lane_handles": dict(lane_handles),
        "lane_state": lane_state,
        "action_state": action_state,
        "test_count": len(selected),
        "mapped_test_count": len(mapped),
        "source_located_test_count": len(selected) - len(source_drift) - len(planned),
        "source_drift_count": len(source_drift),
        "actionable_source_drift_count": len(actionable_source_drift),
        "planned_count": len(planned),
        "disabled_artifact_count": len(disabled_artifacts),
        "source_reconciliation_count": len(source_reconciliation),
        "disabled_artifact_ids": [test.get("id") for test in disabled_artifacts],
        "implemented_test_count": len(implemented),
        "candidate_count": len(executable_candidates),
        "shown_candidate_count": len(candidates),
        "explicit_disposition_count": len(explicit_dispositions),
        "unclassified_count": len(unclassified),
        "assertion_count": assertion_count,
        "recorded_assertion_count": recorded_assertion_count,
        "assertion_count_source": assertion_count_source,
        "executable_assertion_count": executable_assertion_count,
        "status_counts": dict(sorted(status_counts.items())),
        "requirement_count": len(requirement_ids),
        "requirement_ids": requirement_ids,
        "standard_section_count": len(standard_sections),
        "standard_sections": standard_sections,
        "requirement_section_pair_count": len(requirement_section_pairs),
        "requirement_section_mappings": mapping_rows[:12],
        "candidates": [test_summary_data(test) for test in candidates],
        "source_drift_test_ids": [test.get("id") for test in source_drift],
        "actionable_source_drift_test_ids": [
            test.get("id") for test in actionable_source_drift
        ],
    }
    if len(mapping_rows) > 12:
        result["requirement_section_mappings_remaining"] = len(mapping_rows) - 12
    return result


def lane_inventory(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    *,
    family: str | None = None,
    unmapped_only: bool = False,
    disposition: str = "all",
    limit: int = 40,
) -> dict[str, Any]:
    """List exact lane handles inside an optional roadmap-family scope.

    The normal lane inventory is intentionally a work-selection view rather
    than another broad plan dump.  It groups the already-indexed plan rows by
    their exact Catch2 tags and reports only the counts and one deterministic
    next-case handle per tag.  ``--family`` keeps overlapping tags scoped to
    one roadmap owner, while ``--unmapped`` makes the missing traceability
    queue directly queryable without reopening the Requirements Lab.
    ``disposition`` can narrow that queue to intentional ``explicit``
    decisions or rows still needing a mapping decision (``unclassified``).
    """

    if disposition not in {"all", "explicit", "unclassified"}:
        raise ValueError(
            "lane inventory disposition must be all, explicit, or unclassified"
        )

    family_item, scoped_tests = scoped_plan_tests(index, tests, family=family)
    if family is not None and family_item is None:
        return {
            "found": False,
            "family": family,
            "disposition": disposition,
            "error": f"No indexed roadmap family matched: {family}",
            "count": 0,
            "shown_count": 0,
            "lanes": [],
        }

    grouped: defaultdict[str, list[dict[str, Any]]] = defaultdict(list)
    for test in scoped_tests:
        for tag in strings(test.get("tags")):
            grouped[tag].append(test)

    mapping = index.get("mapping", {})
    if not isinstance(mapping, dict):
        mapping = {}
    lane_owners = mapping.get("lane_owners", {})
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    lane_handles = mapping.get("lane_handles", {})
    if not isinstance(lane_handles, dict):
        lane_handles = {}

    def case_insensitive_value(values: dict[str, Any], key: str) -> Any:
        if key in values:
            return values[key]
        folded = key.casefold()
        for name, value in values.items():
            if str(name).casefold() == folded:
                return value
        return None

    rows: list[dict[str, Any]] = []
    for tag, lane_tests in grouped.items():
        mapped_count = sum(bool(test.get("requirements")) for test in lane_tests)
        explicit_count = sum(
            not test.get("requirements")
            and traceability_state(test) == "explicit-disposition"
            for test in lane_tests
        )
        unclassified_count = sum(
            not test.get("requirements")
            and traceability_state(test) == "unclassified"
            for test in lane_tests
        )
        planned_count = sum(is_planned_test(test) for test in lane_tests)
        source_drift_count = sum(
            not test.get("source_locations") and not is_planned_test(test)
            for test in lane_tests
        )
        actionable_source_drift_count = sum(
            not test.get("source_locations")
            and not is_planned_test(test)
            and not is_non_executable_source_status(test)
            for test in lane_tests
        )
        candidate_count = sum(
            bool(test.get("source_locations"))
            and not str(test.get("status") or "").startswith(("implemented", "verified"))
            and not is_non_executable_source_status(test)
            for test in lane_tests
        )
        if unmapped_only or disposition != "all":
            if disposition == "explicit":
                if not explicit_count:
                    continue
            elif disposition == "unclassified":
                if not unclassified_count:
                    continue
            elif not (explicit_count or unclassified_count):
                continue

        requirement_ids = sorted(
            {
                requirement_id
                for test in lane_tests
                for requirement_id in strings(test.get("lab_requirement_ids"))
            }
        )
        standard_sections = sorted(
            {
                section
                for test in lane_tests
                for section in strings(test.get("standard_sections"))
            }
        )
        recorded_assertion_count = sum(
            value
            for test in lane_tests
            for value in [test.get("assertions")]
            if isinstance(value, int)
        )
        indexed_assertion_count = indexed_lane_assertion_count(index, tag)
        assertions = (
            indexed_assertion_count
            if isinstance(indexed_assertion_count, int)
            else recorded_assertion_count
        )
        assertion_count_source = (
            "indexed-lane-total"
            if isinstance(indexed_assertion_count, int)
            else "plan-entry-records"
        )
        requirement_section_rows = [
            row
            for test in lane_tests
            for row in requirement_section_mapping_rows(test)
            if isinstance(row, dict)
        ]
        requirement_section_pairs = {
            (row.get("lab_requirement_id"), row.get("standard_section"))
            for row in requirement_section_rows
            if row.get("lab_requirement_id")
        }
        # ``next_test`` is an actionable handoff.  Explicit no-standalone
        # dispositions are intentionally retained in the inventory, but they
        # are review material rather than implementation work; pointing at
        # one as ``next`` made a completed lane look like it had a runnable
        # case.  Keep a separate review handle below so the disposition can
        # still be inspected without polluting the work queue.
        review_tests: list[dict[str, Any]] = []
        if disposition == "unclassified":
            state = "needs-mapping" if unclassified_count else "missing"
            next_tests = [
                test
                for test in lane_tests
                if not test.get("requirements")
                and traceability_state(test) == "unclassified"
            ]
        elif disposition == "explicit":
            state = "explicit-disposition" if explicit_count else "missing"
            next_tests = [
                test
                for test in lane_tests
                if not test.get("requirements")
                and traceability_state(test) == "explicit-disposition"
            ]
        elif unclassified_count:
            state = "needs-mapping"
            next_tests = [
                test
                for test in lane_tests
                if not test.get("requirements")
                and traceability_state(test) == "unclassified"
            ]
        elif planned_count:
            state = "planned"
            next_tests = [test for test in lane_tests if is_planned_test(test)]
        elif actionable_source_drift_count:
            state = "source-drift"
            next_tests = [
                test
                for test in lane_tests
                if not test.get("source_locations")
                and not is_non_executable_source_status(test)
            ]
        elif candidate_count:
            state = "implementation-candidate"
            next_tests = [
                test
                for test in lane_tests
                if test.get("source_locations")
                and not str(test.get("status") or "").startswith(("implemented", "verified"))
                and not is_non_executable_source_status(test)
            ]
        elif explicit_count:
            state = "explicit-disposition"
            # Deliberately leave ``next_tests`` empty.  These rows have an
            # explicit no-standalone-surface decision, so their trace is a
            # review handle, not an implementation handoff.
            next_tests = []
            review_tests = [
                test
                for test in lane_tests
                if not test.get("requirements")
                and traceability_state(test) == "explicit-disposition"
            ]
        else:
            state = "complete"
            next_tests = []

        next_test = next_tests[0] if next_tests else None
        review_test = review_tests[0] if review_tests else None
        next_case = next_test.get("test_case") if isinstance(next_test, dict) else None
        next_locations = (
            next_test.get("source_locations", [])
            if isinstance(next_test, dict)
            else []
        )
        next_location = (
            next_locations[0]
            if next_locations and isinstance(next_locations[0], dict)
            else None
        )
        review_locations = (
            review_test.get("source_locations", [])
            if isinstance(review_test, dict)
            else []
        )
        review_location = (
            review_locations[0]
            if review_locations and isinstance(review_locations[0], dict)
            else None
        )
        review_case = (
            review_test.get("test_case")
            if isinstance(review_test, dict)
            else None
        )
        # Completed lanes have no ``next`` case, but they still need a
        # deterministic mapping handle.  Keep one representative plan row
        # so a lane search can jump straight to its requirement/section
        # traceability without first dumping every case in the lane.
        representative_test = min(
            lane_tests,
            key=lambda test: (
                str(test.get("id") or "").casefold(),
                str(test.get("test_case") or "").casefold(),
            ),
        ) if lane_tests else None
        representative_locations = (
            representative_test.get("source_locations", [])
            if isinstance(representative_test, dict)
            else []
        )
        representative_location = (
            representative_locations[0]
            if representative_locations and isinstance(representative_locations[0], dict)
            else None
        )
        representative_case = (
            representative_test.get("test_case")
            if isinstance(representative_test, dict)
            else None
        )
        owner = case_insensitive_value(lane_owners, tag)
        handles = case_insensitive_value(lane_handles, tag)
        if not isinstance(handles, dict):
            handles = {}
        if str(handles.get("execution_gate") or "").casefold().startswith("blocked:"):
            state = "execution-blocked"
        owner_item = next(
            (
                item
                for item in index.get("items", [])
                if isinstance(item, dict) and item.get("id") == owner
            ),
            {},
        )
        action_state = queue_action_state(
            owner_item,
            candidate_count=candidate_count,
            unclassified_count=unclassified_count,
            actionable_source_drift_count=actionable_source_drift_count,
            lane_state="complete-pointer" if state == "complete" else state,
        )
        rows.append(
            {
                "tag": tag,
                "state": state,
                "action_state": action_state,
                "test_count": len(lane_tests),
                "mapped_count": mapped_count,
                "explicit_disposition_count": explicit_count,
                "unclassified_count": unclassified_count,
                "source_drift_count": source_drift_count,
                "actionable_source_drift_count": actionable_source_drift_count,
                "planned_count": planned_count,
                "candidate_count": candidate_count,
                "assertion_count": assertions,
                "recorded_assertion_count": recorded_assertion_count,
                "assertion_count_source": assertion_count_source,
                "requirement_count": len(requirement_ids),
                "standard_section_count": len(standard_sections),
                "requirement_section_pair_count": len(requirement_section_pairs),
                "resolved_requirement_section_pair_count": sum(
                    pair[1] is not None for pair in requirement_section_pairs
                ),
                "unresolved_requirement_section_mapping_count": sum(
                    bool(row.get("unresolved")) for row in requirement_section_rows
                ),
                "requirement_ids": requirement_ids,
                "standard_sections": standard_sections,
                "roadmap_owner": owner,
                "handles": dict(handles),
                "next_test": next_case,
                "next_test_id": (
                    next_test.get("id") if isinstance(next_test, dict) else None
                ),
                "next_assertions": (
                    next_test.get("assertions") if isinstance(next_test, dict) else None
                ),
                "next_requirement_ids": (
                    strings(next_test.get("lab_requirement_ids"))
                    if isinstance(next_test, dict)
                    else []
                ),
                "next_standard_sections": (
                    strings(next_test.get("standard_sections"))
                    if isinstance(next_test, dict)
                    else []
                ),
                "next_source": (
                    f"{next_location.get('path')}:{next_location.get('line')}"
                    if next_location and next_location.get("path")
                    else None
                ),
                "representative_test": representative_case,
                "representative_test_id": (
                    representative_test.get("id")
                    if isinstance(representative_test, dict)
                    else None
                ),
                "representative_source": (
                    f"{representative_location.get('path')}:{representative_location.get('line')}"
                    if representative_location and representative_location.get("path")
                    else None
                ),
                "representative_requirement_ids": (
                    strings(representative_test.get("lab_requirement_ids"))
                    if isinstance(representative_test, dict)
                    else []
                ),
                "representative_standard_sections": (
                    strings(representative_test.get("standard_sections"))
                    if isinstance(representative_test, dict)
                    else []
                ),
                "representative_trace_command": (
                    f'python tools/query_rti_work.py trace "{representative_case}" '
                    "--summary --compact"
                    if isinstance(representative_case, str)
                    else None
                ),
                "next_trace_command": (
                    f'python tools/query_rti_work.py trace "{next_case}" '
                    "--summary --compact"
                    if isinstance(next_case, str)
                    else None
                ),
                "review_test": review_case,
                "review_test_id": (
                    review_test.get("id")
                    if isinstance(review_test, dict)
                    else None
                ),
                "review_assertions": (
                    review_test.get("assertions")
                    if isinstance(review_test, dict)
                    else None
                ),
                "review_source": (
                    f"{review_location.get('path')}:{review_location.get('line')}"
                    if review_location and review_location.get("path")
                    else None
                ),
                "review_trace_command": (
                    f'python tools/query_rti_work.py trace "{review_case}" '
                    "--summary --compact"
                    if isinstance(review_case, str)
                    else None
                ),
                "focus_command": (
                    f"python tools/query_rti_work.py focus {tag} --summary --compact"
                ),
                "unmapped_command": (
                    f"python tools/query_rti_work.py unmapped --lane {tag} "
                    "--disposition unclassified --summary --compact"
                    if unclassified_count
                    else None
                ),
                "ctest_command": lane_ctest_command(handles),
            }
        )

    rows.sort(
        key=lambda row: (
            0 if row["unclassified_count"] else 1 if row["explicit_disposition_count"] else 2,
            -row["unclassified_count"],
            -row["explicit_disposition_count"],
            -row["candidate_count"],
            row["tag"],
        )
    )
    requested_limit = max(limit, 0)
    shown = rows if requested_limit == 0 else rows[:requested_limit]
    return {
        "found": True,
        "family": family,
        "family_title": (
            family_item.get("title") if isinstance(family_item, dict) else None
        ),
        "scoped_test_count": len(scoped_tests),
        "count": len(rows),
        "shown_count": len(shown),
        "unmapped_only": unmapped_only,
        "disposition": disposition,
        "lanes": shown,
    }


def indexed_work_queue(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    limit: int = 8,
) -> dict[str, Any]:
    """Return a bounded queue of open roadmap families.

    The roadmap contains broad, overlapping query tags, so printing every tag
    is a poor work-selection interface.  This view keeps one row per open
    indexed family and joins only its exact next lane (when present).  It
    classifies a row as ready, complete-pointer, source-drift-only, or
    new-case-needed without changing the plan or re-reading the Lab.  The
    separate ``action_state`` says whether that row is an implementation,
    mapping, source-reconciliation, external-review, deliberate-new-case, or
    evidence-complete handoff.  It also
    exposes the bounded source-unlocated backlog so a planned next case is
    visible without dumping the full plan; those rows are never executable
    candidates until a matching C++ declaration exists.  Disabled source
    artifacts remain in the diagnostic count, but are excluded from actionable
    queue heads so historical ``#if 0`` declarations cannot block ``ready``.
    """

    items = sorted(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("status") == "open"
        ),
        key=lambda item: item.get("priority", 999),
    )
    mapping = index.get("mapping", {})
    lane_handles = (
        mapping.get("lane_handles", {})
        if isinstance(mapping, dict)
        else {}
    )
    if not isinstance(lane_handles, dict):
        lane_handles = {}
    rows: list[dict[str, Any]] = []
    for item in items:
        selected = item_tests(item, tests)
        planned = [test for test in selected if is_planned_test(test)]
        unlocated = [
            test
            for test in selected
            if not test.get("source_locations") and not is_planned_test(test)
        ]
        actionable_unlocated = [
            test
            for test in unlocated
            if not is_non_executable_source_status(test)
        ]
        mapped_count = sum(bool(test.get("requirements")) for test in selected)
        family_case_count = len(selected)
        family_mapped_case_count = mapped_count
        source_drift_count = len(unlocated)
        actionable_source_drift_count = len(actionable_unlocated)
        planned_count = len(planned)
        family_mapping_counts = live_item_mapping_counts(item, tests)
        unclassified_count = family_mapping_counts.get("unclassified_count", 0)
        candidate_count = sum(
            bool(test.get("source_locations"))
            and not str(test.get("status") or "").startswith(("implemented", "verified"))
            and not is_non_executable_source_status(test)
            for test in selected
        )
        lane = item.get("next_lane")
        lane_snapshot: dict[str, Any] | None = None
        if isinstance(lane, str) and lane:
            lane_snapshot = focused_lane_result(index, tests, lane, limit=1)
        if lane_snapshot and lane_snapshot.get("lane_state") != "missing":
            case_count = lane_snapshot.get("test_count", 0)
            mapped_count = lane_snapshot.get("mapped_test_count", mapped_count)
            unclassified_count = lane_snapshot.get(
                "unclassified_count", unclassified_count
            )
            source_drift_count = lane_snapshot.get("source_drift_count", source_drift_count)
            actionable_source_drift_count = lane_snapshot.get(
                "actionable_source_drift_count", actionable_source_drift_count
            )
            planned_count = lane_snapshot.get("planned_count", planned_count)
            candidate_count = lane_snapshot.get("candidate_count", candidate_count)
            if candidate_count:
                state = "ready"
            elif actionable_source_drift_count:
                state = "source-drift-only"
            elif planned_count:
                state = "planned"
            else:
                state = "complete-pointer"
        elif unclassified_count:
            case_count = len(selected)
            state = "mapping-needed"
        elif candidate_count:
            case_count = len(selected)
            state = "ready"
        elif selected and actionable_source_drift_count:
            case_count = len(selected)
            state = "source-drift-only"
        elif planned_count:
            case_count = len(selected)
            state = "planned"
        else:
            case_count = len(selected)
            state = "new-case-needed"
        action_state = queue_action_state(
            item,
            candidate_count=candidate_count,
            unclassified_count=unclassified_count,
            actionable_source_drift_count=actionable_source_drift_count,
            lane_state=state,
        )
        # Disabled source artifacts remain in the diagnostic backlog, but they
        # are not implementation handoffs. Keep the total visible while using
        # the first actionable row for ready()/dashboard selection.
        unlocated_head = actionable_unlocated[0] if actionable_unlocated else {}
        unlocated_head_query = unlocated_head.get("test_case")
        planned_head = planned[0] if planned else {}
        planned_head_query = planned_head.get("test_case")
        planned_head_lane = planned_head.get("primary_lane")
        planned_head_handles = (
            lane_handles.get(planned_head_lane, {})
            if isinstance(planned_head_lane, str)
            else {}
        )
        if not isinstance(planned_head_handles, dict):
            planned_head_handles = {}
        next_test = next_test_pointer(item, tests)
        family_mapping_counts = live_item_mapping_counts(item, tests)
        rows.append(
            {
                "id": item.get("id"),
                "title": item.get("title"),
                "priority": item.get("priority"),
                "kind": item.get("kind"),
                "next_work_id": item.get("next_work_id"),
                "next_work_status": item.get("next_work_status"),
                "next_lane": lane,
                "state": state,
                "action_state": action_state,
                "case_count": case_count,
                "family_case_count": family_case_count,
                "family_mapped_case_count": family_mapped_case_count,
                "assertion_count": (
                    lane_snapshot.get("assertion_count")
                    if lane_snapshot
                    and lane_snapshot.get("lane_state") != "missing"
                    else None
                ),
                "recorded_assertion_count": (
                    lane_snapshot.get("recorded_assertion_count")
                    if lane_snapshot
                    and lane_snapshot.get("lane_state") != "missing"
                    else None
                ),
                "mapped_case_count": mapped_count,
                "unclassified_count": unclassified_count,
                "explicit_disposition_count": family_mapping_counts.get(
                    "explicit_disposition_count", 0
                ),
                "requirement_section_pair_count": family_mapping_counts.get(
                    "requirement_section_pair_count", 0
                ),
                "resolved_requirement_section_pair_count": family_mapping_counts.get(
                    "resolved_requirement_section_pair_count", 0
                ),
                "unresolved_requirement_section_mapping_count": family_mapping_counts.get(
                    "unresolved_requirement_section_mapping_count", 0
                ),
                "requirement_count": family_mapping_counts.get(
                    "requirement_count", 0
                ),
                "standard_section_count": family_mapping_counts.get(
                    "standard_section_count", 0
                ),
                "cpp_api_surface_count": family_mapping_counts.get(
                    "cpp_api_surface_count", 0
                ),
                "requirement_ids_preview": family_mapping_counts.get(
                    "requirement_ids_preview", []
                ),
                "requirement_ids_remaining": family_mapping_counts.get(
                    "requirement_ids_remaining", 0
                ),
                "standard_sections_preview": family_mapping_counts.get(
                    "standard_sections_preview", []
                ),
                "standard_sections_remaining": family_mapping_counts.get(
                    "standard_sections_remaining", 0
                ),
                "requirement_section_pairs_preview": family_mapping_counts.get(
                    "requirement_section_pairs_preview", []
                ),
                "requirement_section_pairs_remaining": family_mapping_counts.get(
                    "requirement_section_pairs_remaining", 0
                ),
                "source_drift_count": source_drift_count,
                "actionable_source_drift_count": actionable_source_drift_count,
                "planned_count": planned_count,
                "unlocated_case_count": len(unlocated),
                "unlocated_actionable_case_count": len(actionable_unlocated),
                "unlocated_head": (
                    unlocated_head_query
                ),
                "unlocated_head_id": unlocated_head.get("id"),
                "unlocated_head_assertions": unlocated_head.get("assertions"),
                "unlocated_head_requirement_ids": strings(
                    unlocated_head.get("lab_requirement_ids")
                ),
                "unlocated_head_standard_sections": strings(
                    unlocated_head.get("standard_sections")
                ),
                "unlocated_head_api_surfaces": strings(
                    unlocated_head.get("selected_cpp_api_surface_ids")
                ),
                "unlocated_head_trace_command": (
                    f'python tools/query_rti_work.py trace "{unlocated_head_query}" '
                    "--summary --compact"
                    if isinstance(unlocated_head_query, str)
                    else None
                ),
                "planned_head": planned_head_query,
                "planned_head_id": planned_head.get("id"),
                "planned_head_lane": planned_head_lane,
                "planned_head_ctest_filter": planned_head_handles.get(
                    "ctest_filter"
                ),
                "planned_head_assertions": planned_head.get("assertions"),
                "planned_head_requirement_ids": strings(
                    planned_head.get("lab_requirement_ids")
                ),
                "planned_head_standard_sections": strings(
                    planned_head.get("standard_sections")
                ),
                "planned_head_api_surfaces": strings(
                    planned_head.get("selected_cpp_api_surface_ids")
                ),
                "planned_head_trace_command": (
                    f'python tools/query_rti_work.py trace "{planned_head_query}" '
                    "--summary --compact"
                    if isinstance(planned_head_query, str)
                    else None
                ),
                "candidate_count": candidate_count,
                "tag_count": len(strings(item.get("query_tags"))),
                "next_test_query": item.get("next_test_query"),
                "next_test_pointer": next_test,
                "next_test_state": next_test.get("state"),
                "next_action": live_next_action(item),
                "work_query": item.get("next_work_query"),
            }
        )
    requested_limit = max(limit, 0)
    shown = rows if requested_limit == 0 else rows[:requested_limit]
    action_counts = Counter(
        str(row.get("action_state") or "<unspecified>") for row in rows
    )
    return {
        "items": shown,
        "shown_count": len(shown),
        "open_count": len(rows),
        "executable_candidate_count": sum(row["candidate_count"] for row in rows),
        "source_drift_count": sum(row["source_drift_count"] for row in rows),
        "actionable_source_drift_count": sum(
            row["actionable_source_drift_count"] for row in rows
        ),
        "unlocated_case_count": sum(row["unlocated_case_count"] for row in rows),
        "unlocated_actionable_case_count": sum(
            row["unlocated_actionable_case_count"] for row in rows
        ),
        "planned_case_count": sum(row.get("planned_count", 0) for row in rows),
        "mapped_case_count": sum(row["mapped_case_count"] for row in rows),
        "action_counts": dict(sorted(action_counts.items())),
        "queued_action_family_count": sum(
            value
            for name, value in action_counts.items()
            if name != "evidence-complete"
        ),
        "evidence_complete_family_count": action_counts.get(
            "evidence-complete", 0
        ),
    }


def dashboard_snapshot(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    roadmap_entries: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
    source_health: dict[str, Any] | None = None,
    limit: int = 3,
    standard_requirements: dict[str, dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Return one bounded resume card for roadmap and test work selection.

    ``status`` remains the detailed diagnostic view and ``queue`` remains the
    family queue.  This card composes their live counts with one active work
    handoff and a deliberately small queue preview so a contributor can resume
    without choosing between several broad scans.  It is read-only and uses
    the already indexed plan/source joins; it never edits or re-indexes the
    Requirements Lab.
    """

    status = index_status(index, tests, roadmap_entries, source_locations)
    mapping_counts = status.get("derived_mapping_counts", {})
    indexed_counts = status.get("mapping_counts", {})
    if not isinstance(mapping_counts, dict):
        mapping_counts = {}
    if not isinstance(indexed_counts, dict):
        indexed_counts = {}
    comparable_keys = sorted(set(indexed_counts).intersection(mapping_counts))
    live_snapshot = {
        key: mapping_counts.get(key) for key in comparable_keys
    }
    indexed_snapshot = {
        key: indexed_counts.get(key) for key in comparable_keys
    }

    active = indexed_work_slice(
        index,
        tests,
        source_locations=source_locations,
    )
    next_card: dict[str, Any] | None = None
    handles = {
        "ready": "python tools/query_rti_work.py ready --summary --compact",
        "check": "python tools/query_rti_work.py check --summary --compact",
        "lab_issues": "python tools/query_rti_work.py lab-issues --summary --compact",
    }
    if active.get("found"):
        baseline = active.get("baseline")
        next_card = {
            "work_id": active.get("work_id"),
            "work_status": active.get("work_status"),
            "title": (active.get("work_item") or {}).get("title"),
            "task": compact_prose(active.get("task"), 320),
            "lane": active.get("lane"),
            "lane_state": active.get("lane_state"),
            "lane_case_count": active.get("lane_case_count"),
            "lane_mapped_test_count": active.get("lane_mapped_test_count"),
            "lane_assertion_count": active.get("lane_assertion_count"),
            "lane_source_drift_count": active.get("lane_source_drift_count"),
            "lane_actionable_source_drift_count": active.get(
                "lane_actionable_source_drift_count"
            ),
            "baseline": (
                {
                    "test_case": baseline.get("test_case"),
                    "state": baseline.get("status")
                    or baseline.get("traceability_state"),
                    "assertion_count": baseline.get("assertions"),
                    "requirement_count": baseline.get("requirement_count"),
                    "standard_section_count": len(
                        strings(
                            baseline.get("standard_sections")
                            or baseline.get("standard_clauses")
                        )
                    ),
                    "cpp_api_surface_count": len(
                        strings(baseline.get("selected_cpp_api_surface_ids"))
                    ),
                }
                if isinstance(baseline, dict)
                else None
            ),
            "standard_sections": active.get("standard_sections", []),
            "plan_ids": active.get("plan_ids", []),
            "commands": active.get("commands", []),
        }
        work_id = active.get("work_id")
        if isinstance(work_id, str) and work_id:
            handles["work"] = (
                f"python tools/query_rti_work.py work {work_id} "
                "--summary --compact"
            )
        lane = active.get("lane")
        if isinstance(lane, str) and lane:
            handles["focus"] = (
                f"python tools/query_rti_work.py focus {lane} "
                "--summary --compact"
            )
            handles["lane_check"] = (
                f"python tools/query_rti_work.py check --lane {lane} "
                "--summary --compact"
            )
            handles["matrix"] = (
                f"python tools/query_rti_work.py matrix {lane} "
                "--summary --compact"
            )
        if isinstance(baseline, dict) and isinstance(
            baseline.get("test_case"), str
        ):
            handles["trace"] = (
                "python tools/query_rti_work.py trace "
                f'"{baseline["test_case"]}" --summary --compact'
            )
    elif active.get("reason"):
        next_card = {"state": "unavailable", "reason": active.get("reason")}

    # ``work`` intentionally retains the active roadmap pointer even when its
    # baseline lane is complete.  For a resume card, prefer ``ready``'s exact
    # planned/source queue head so "Next" names executable work rather than a
    # completed pointer.  Keep the active pointer separately for orientation.
    active_card = next_card
    ready = ready_slice(
        index,
        tests,
        source_locations,
        standard_requirements=standard_requirements,
    )
    if ready.get("found"):
        owner = ready.get("roadmap_owner") or ready.get("owner") or {}
        requirement_ids = strings(ready.get("requirement_ids"))
        standard_sections = strings(ready.get("standard_sections"))
        api_surfaces = strings(ready.get("api_surfaces"))
        ready_test = next(
            (
                test
                for test in tests
                if isinstance(test, dict)
                and test.get("test_case") == ready.get("test_case")
            ),
            None,
        )
        next_card = {
            "state": ready.get("state"),
            "runnable": ready.get("runnable", True),
            "handoff_kind": ready.get("handoff_kind"),
            "family_id": ready.get("family_id"),
            "work_id": ready.get("work_id") or ready.get("family_id"),
            "work_status": (
                ready.get("status")
                if ready.get("state") in {"planned", "mapping"}
                and ready.get("status")
                else ready.get("work_status") or owner.get("status")
            ),
            "title": owner.get("title"),
            "test_case": ready.get("test_case"),
            "plan_id": ready.get("plan_id"),
            "source_location": ready.get("source_location"),
            "source_target": ready.get("source_target"),
            "source_lane": ready.get("source_lane"),
            "lane_pending": ready.get("lane_pending"),
            "post_mapping_focus_command": ready.get("post_mapping_focus_command"),
            "post_mapping_check_command": ready.get("post_mapping_check_command"),
            "ctest_target": ready.get("ctest_target"),
            "mapping_status": ready.get("mapping_status"),
            "mapping_seed_plan_id": ready.get("mapping_seed_plan_id"),
            "mapping_seed_test_case": ready.get("mapping_seed_test_case"),
            "assertion_count": ready.get("assertions"),
            "requirement_count": len(requirement_ids),
            "standard_section_count": len(standard_sections),
            "cpp_api_surface_count": len(api_surfaces),
            "requirement_section_pair_count": (
                len(requirement_section_mapping_rows(ready_test))
                if isinstance(ready_test, dict)
                else ready.get("requirement_section_pair_count", 0)
            ),
            "requirement_ids": requirement_ids,
            "standard_sections": standard_sections,
            "api_surfaces": api_surfaces,
            "requirement_section_pairs": (
                ready.get("requirement_section_pairs", [])[:8]
                if isinstance(ready.get("requirement_section_pairs"), list)
                else []
            ),
            "requirement_section_pairs_remaining": max(
                0,
                len(ready.get("requirement_section_pairs", [])) - 8,
            )
            if isinstance(ready.get("requirement_section_pairs"), list)
            else 0,
            "next_action": compact_prose(ready.get("next_action"), 320),
            "acceptance": ready.get("acceptance", []),
            "seed_trace_command": ready.get("seed_trace_command"),
            "commands": [
                command
                for command in (
                    ready.get("trace_command"),
                    ready.get("seed_trace_command"),
                    ready.get("implementation_command"),
                    ready.get("focus_command"),
                    ready.get("check_command"),
                    ready.get("post_mapping_focus_command"),
                    ready.get("post_mapping_check_command"),
                )
                if isinstance(command, str) and command
            ],
        }
        ready_work_id = next_card.get("work_id")
        if isinstance(ready_work_id, str) and ready_work_id:
            handles["work"] = (
                f"python tools/query_rti_work.py work {ready_work_id} "
                "--summary --compact"
            )
        for handle_name, field_name in (
            ("trace", "trace_command"),
            ("focus", "focus_command"),
            ("lane_check", "check_command"),
            ("post_mapping_focus", "post_mapping_focus_command"),
            ("post_mapping_check", "post_mapping_check_command"),
        ):
            value = ready.get(field_name)
            if isinstance(value, str) and value:
                handles[handle_name] = value
        if not next_card.get("source_lane"):
            handles.pop("focus", None)
            handles.pop("lane_check", None)
        family_id = next_card.get("family_id")
        if isinstance(family_id, str) and family_id:
            handles["matrix"] = (
                f"python tools/query_rti_work.py matrix {family_id} "
                "--summary --compact"
            )
    elif ready.get("reason"):
        next_card = {
            "state": ready.get("state", "unavailable"),
            "reason": ready.get("reason"),
            "family_options": ready.get("family_options", []),
            "family_queue_count": ready.get("family_queue_count", 0),
            "evidence_complete_families": ready.get(
                "evidence_complete_families", []
            ),
        }
        family_options = next_card.get("family_options")
        if isinstance(family_options, list) and family_options:
            first_family = family_options[0]
            if isinstance(first_family, dict):
                next_card["recommended_family_id"] = first_family.get("id")
        # The active roadmap pointer can legitimately remain on a completed
        # historical lane while the executable/planned queues are exhausted.
        # Do not surface that pointer as though it were the next action. Keep
        # only the bounded family-selection handles on the resume card.
        for stale_handle in ("work", "focus", "trace", "matrix", "lane_check"):
            handles.pop(stale_handle, None)
        handles["queue"] = (
            "python tools/query_rti_work.py queue --summary --compact"
        )
        handles["family_ready"] = (
            "python tools/query_rti_work.py ready --family <family-id> "
            "--summary --compact"
        )

    latest = status.get("latest_completed_slice")
    latest_card = None
    if isinstance(latest, dict):
        latest_lane = latest.get("lane")
        latest_focus_lane = latest.get("focus_lane") or latest_lane
        latest_test_case = latest.get("test_case")
        lane_handles = (
            index.get("mapping", {}).get("lane_handles", {}).get(latest_focus_lane)
            if isinstance(latest_focus_lane, str)
            and isinstance(index.get("mapping"), dict)
            and isinstance(index.get("mapping", {}).get("lane_handles"), dict)
            else None
        )
        if not isinstance(lane_handles, dict) or not lane_handles:
            lane_handles = (
                index.get("mapping", {}).get("lane_handles", {}).get(latest_lane)
                if isinstance(latest_lane, str)
                and isinstance(index.get("mapping"), dict)
                and isinstance(index.get("mapping", {}).get("lane_handles"), dict)
                else None
            )
        latest_card = {
            "lane": latest_lane,
            "focus_lane": latest_focus_lane,
            "plan_id": latest.get("plan_id"),
            "test_case": latest_test_case,
            "assertions": latest.get("assertions"),
            "requirements": latest.get("requirements"),
            "standard_sections": latest.get("standard_sections"),
            "api_surfaces": latest.get("api_surfaces"),
            "source_location": latest.get("source_location"),
            "focus_command": (
                f"python tools/query_rti_work.py focus {latest_focus_lane} --summary --compact"
                if isinstance(latest_focus_lane, str) and latest_focus_lane
                else None
            ),
            "trace_command": (
                f'python tools/query_rti_work.py trace "{latest_test_case}" --summary --compact'
                if isinstance(latest_test_case, str) and latest_test_case
                else None
            ),
            "case_command": (
                "python tools/query_rti_work.py case "
                f"{json.dumps(str(latest.get('plan_id')), ensure_ascii=False)} "
                "--summary --compact"
                if isinstance(latest.get("plan_id"), str) and latest.get("plan_id")
                else None
            ),
            "matrix_command": (
                "python tools/query_rti_work.py matrix "
                f"{json.dumps(str(latest.get('plan_id')), ensure_ascii=False)} --summary --compact"
                if isinstance(latest.get("plan_id"), str) and latest.get("plan_id")
                else None
            ),
            "check_command": (
                f"python tools/query_rti_work.py check --lane {latest_focus_lane} --summary --compact"
                if isinstance(latest_focus_lane, str) and latest_focus_lane
                else None
            ),
            "ctest_command": lane_ctest_command(lane_handles),
        }

    queue = indexed_work_queue(index, tests, limit=max(limit, 0))
    queue_preview = []
    for row in queue.get("items", []):
        if not isinstance(row, dict):
            continue
        preview = {
            key: row.get(key)
            for key in (
                "id",
                "title",
                "priority",
                "state",
                "action_state",
                "case_count",
                "family_case_count",
                "assertion_count",
                "mapped_case_count",
                "family_mapped_case_count",
                "requirement_count",
                "standard_section_count",
                "cpp_api_surface_count",
                "requirement_section_pair_count",
                "candidate_count",
                "source_drift_count",
                "actionable_source_drift_count",
                "unclassified_count",
                "explicit_disposition_count",
                "unlocated_case_count",
                "unlocated_actionable_case_count",
                "next_work_id",
                "next_lane",
                "next_test_query",
                "next_test_state",
            )
        }
        # The dashboard is a resume card, not a prose dump. Keep the family
        # description available for orientation while bounding it just like
        # ``next_action``; exact work/focus/trace handles remain separate.
        preview["work_query"] = compact_prose(row.get("work_query"), 240)
        preview["next_action"] = compact_prose(row.get("next_action"), 240)
        queue_preview.append(preview)
    queue = {**queue, "items": queue_preview}
    source_queue = unplanned_source_summary(
        tests,
        source_locations or {},
    )
    if source_queue.get("count", 0):
        source_queue["ready_command"] = (
            "python tools/query_rti_work.py ready --include-source-only "
            "--summary --compact"
        )
        source_queue["next_command"] = (
            "python tools/query_rti_work.py next --include-source-only "
            "--summary --compact"
        )
    indexed_deferred_slices = index.get("deferred_slices", [])
    if not isinstance(indexed_deferred_slices, list):
        indexed_deferred_slices = []
    deferred_slices = [
        {
            key: slice_record.get(key)
            for key in (
                "id",
                "family",
                "lane",
                "state",
                "reason",
                "next_step",
                "do_not_start_without",
            )
            if key in slice_record
        }
        for slice_record in indexed_deferred_slices[:8]
        if isinstance(slice_record, dict)
    ]
    return {
        "resume_card": index.get("resume_card", {}),
        "roadmap_counts": status.get("roadmap_counts", {}),
        "plan_counts": {
            "catch2_plan_cases": status.get("test_count", 0),
            "planned_cases": mapping_counts.get("catch2_planned_cases", 0),
            "mapped_cases": status.get("test_count", 0)
            - mapping_counts.get("catch2_cases_without_lab_requirement_mapping", 0),
            "unmapped_cases": mapping_counts.get(
                "catch2_cases_without_lab_requirement_mapping", 0
            ),
            "source_unlocated_cases": mapping_counts.get(
                "catch2_cases_without_cpp_source_location", 0
            ),
            "source_unlocated_actionable_cases": mapping_counts.get(
                "catch2_actionable_cases_without_cpp_source_location",
                mapping_counts.get("catch2_cases_without_cpp_source_location", 0),
            ),
            "source_only_cases": mapping_counts.get(
                "cpp_source_cases_without_plan_row", 0
            ),
            "explicit_unmapped_dispositions": mapping_counts.get(
                "catch2_unmapped_explicit_disposition", 0
            ),
            "unclassified_unmapped_cases": mapping_counts.get(
                "catch2_unmapped_unclassified", 0
            ),
        },
        "mapping_counts": mapping_counts,
        "source_health": source_health or {},
        "latest_completed_slice": latest_card,
        "next": next_card,
        "active_handoff": indexed_active_handoff_record(index, tests),
        "deferred_slices": deferred_slices,
        "source_only_reconciliation": source_queue,
        "active_pointer": active_card,
        "queue": queue,
        "index_snapshot": {
            "matches_live": indexed_snapshot == live_snapshot,
            "comparable_keys": comparable_keys,
            "indexed_counts": indexed_snapshot,
            "live_counts": live_snapshot,
        },
        "handles": handles,
    }


def resume_snapshot(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    roadmap_entries: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]] | None = None,
    source_health: dict[str, Any] | None = None,
    limit: int = 1,
    standard_requirements: dict[str, dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Return the smallest useful first-read work card.

    ``dashboard_snapshot`` is intentionally information-rich because it is
    also a diagnostic view.  A normal resume should not need its queue preview
    or historical pointer details, however.  Keep this projection bounded and
    preserve the exact mapping handles needed to move from one test to its Lab
    requirements and canonical 2025 subsections.  The projection consumes the
    same live joins as ``dashboard``; it never rescans or edits the Lab.
    """

    queue_limit = max(1, limit)
    dashboard = dashboard_snapshot(
        index,
        tests,
        roadmap_entries,
        source_locations,
        source_health,
        limit=queue_limit,
        standard_requirements=standard_requirements,
    )

    def bounded_values(
        value: Any,
        preview_limit: int = 8,
    ) -> tuple[list[str], int]:
        values = strings(value)
        return values[:preview_limit], max(0, len(values) - preview_limit)

    def compact_next(value: Any) -> dict[str, Any] | None:
        if not isinstance(value, dict):
            return None
        result = {
            key: value.get(key)
            for key in (
                "state",
                "runnable",
                "handoff_kind",
                "family_id",
                "work_id",
                "work_status",
                "title",
                "test_case",
                "plan_id",
                "source_location",
                "source_target",
                "source_lane",
                "lane_pending",
                "post_mapping_focus_command",
                "post_mapping_check_command",
                "ctest_target",
                "mapping_status",
                "mapping_seed_plan_id",
                "mapping_seed_test_case",
                "assertion_count",
                "requirement_count",
                "standard_section_count",
                "cpp_api_surface_count",
                "requirement_section_pair_count",
                "requirement_section_pairs",
                "next_action",
                "seed_trace_command",
                "acceptance",
                "commands",
                "reason",
                "recommended_family_id",
                "family_queue_count",
                "gap_preview",
            )
            if key in value
        }
        for field in ("requirement_ids", "standard_sections", "api_surfaces"):
            preview, remaining = bounded_values(value.get(field))
            if preview:
                result[field] = preview
            if remaining:
                result[f"{field}_remaining"] = remaining

        options = value.get("family_options")
        if isinstance(options, list):
            compact_options: list[dict[str, Any]] = []
            for option in options[:queue_limit]:
                if not isinstance(option, dict):
                    continue
                compact_options.append(
                    {
                        key: option.get(key)
                        for key in (
                            "id",
                            "title",
                            "priority",
                            "state",
                            "action_state",
                            "case_count",
                            "mapped_case_count",
                            "family_case_count",
                            "family_mapped_case_count",
                            "requirement_count",
                            "standard_section_count",
                            "requirement_section_pair_count",
                            "work_query",
                            "lane_discovery_command",
                            "mapping_lane_discovery_command",
                            "gap_query",
                            "gap_preview",
                            "next_action",
                        )
                        if key in option
                    }
                )
            result["family_options"] = compact_options
            result["family_options_remaining"] = max(
                0, len(options) - len(compact_options)
            )
        return result

    next_card = compact_next(dashboard.get("next"))
    if isinstance(next_card, dict) and next_card.get("test_case"):
        selected_test = next(
            (
                test
                for test in tests
                if isinstance(test, dict)
                and test.get("test_case") == next_card.get("test_case")
            ),
            None,
        )
        if isinstance(selected_test, dict):
            pairs = [
                row
                for row in requirement_section_mapping_rows(selected_test)
                if isinstance(row, dict)
            ]
            next_card["requirement_section_pairs"] = pairs[:8]
            next_card["requirement_section_pairs_remaining"] = max(
                0, len(pairs) - 8
            )
        elif isinstance(next_card.get("requirement_section_pairs"), list):
            pairs = [
                row
                for row in next_card.get("requirement_section_pairs", [])
                if isinstance(row, dict)
            ]
            next_card["requirement_section_pairs"] = pairs[:8]

    active = dashboard.get("active_pointer")
    active_summary = None
    if isinstance(active, dict):
        active_summary = {
            key: active.get(key)
            for key in (
                "work_id",
                "family_id",
                "lane",
                "state",
                "test_case",
                "source_location",
                "next_action",
            )
            if key in active
        }

    latest = dashboard.get("latest_completed_slice")
    latest_summary = None
    if isinstance(latest, dict):
        latest_summary = {
            key: latest.get(key)
            for key in (
                "lane",
                "focus_lane",
                "plan_id",
                "test_case",
                "assertions",
                "requirements",
                "standard_sections",
                "api_surfaces",
                "source_location",
                "case_command",
                "trace_command",
                "matrix_command",
                "focus_command",
                "check_command",
                "ctest_command",
            )
            if key in latest
        }

    queue = dashboard.get("queue")
    queue_summary: dict[str, Any] = {}
    if isinstance(queue, dict):
        queue_summary = {
            key: queue.get(key)
            for key in ("open_count", "action_counts", "queued_action_family_count")
            if key in queue
        }
        rows = queue.get("items")
        if isinstance(rows, list):
            queue_summary["items"] = rows[:queue_limit]
            queue_summary["shown_count"] = min(queue_limit, len(rows))

    source_only = dashboard.get("source_only_reconciliation")
    source_only_summary = None
    if isinstance(source_only, dict):
        source_only_summary = {
            key: source_only.get(key)
            for key in (
                "count",
                "family_count",
                "test_query",
                "source_location",
                "ready_command",
                "next_command",
            )
            if key in source_only
        }

    deferred_slices = dashboard.get("deferred_slices")
    deferred_summary = (
        deferred_slices[:4]
        if isinstance(deferred_slices, list)
        else []
    )

    return {
        "schema_version": 1,
        "bounded": True,
        "resume_card": dashboard.get("resume_card", {}),
        "roadmap_counts": dashboard.get("roadmap_counts", {}),
        "plan_counts": dashboard.get("plan_counts", {}),
        "mapping_counts": dashboard.get("mapping_counts", {}),
        "source_health": dashboard.get("source_health", {}),
        "index_snapshot": dashboard.get("index_snapshot", {}),
        "next": next_card,
        "active_handoff": dashboard.get("active_handoff"),
        "deferred_slices": deferred_summary,
        "active_pointer": active_summary,
        "latest_completed_slice": latest_summary,
        "source_only_reconciliation": source_only_summary,
        "queue": queue_summary,
        "handles": dashboard.get("handles", {}),
    }


def indexed_unlocated_plan_summary(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    family_id: str | None = None,
) -> dict[str, Any]:
    """Return one bounded head for planned rows missing C++ declarations.

    Unlike the source-only queue, these rows already have deliberate plan
    mappings.  Keep the reverse handoff small: report the overlapping count,
    the first family/plan row, and its exact trace command without expanding
    every historical planned case.
    """

    if family_id is not None:
        folded_family = family_id.casefold()
        family_item = next(
            (
                item
                for item in index.get("items", [])
                if isinstance(item, dict)
                and str(item.get("id") or "").casefold() == folded_family
            ),
            None,
        )
        selected = (
            item_tests(family_item, tests)
            if isinstance(family_item, dict)
            else []
        )
        planned = [test for test in selected if is_planned_test(test)]
        diagnostic_unlocated = [
            test
            for test in selected
            if not test.get("source_locations") and not is_planned_test(test)
        ]
        unlocated = [
            test
            for test in selected
            if not test.get("source_locations")
            and not is_planned_test(test)
            and not is_non_executable_source_status(test)
        ]
        head_test = planned[0] if planned else (unlocated[0] if unlocated else None)
        if not isinstance(head_test, dict):
            return {
                "count": 0,
                "planned_count": len(planned),
                "diagnostic_count": len(diagnostic_unlocated),
                "family_count": 0,
                "head": None,
            }
        lane_handles = {}
        mapping = index.get("mapping", {})
        if isinstance(mapping, dict) and isinstance(mapping.get("lane_handles"), dict):
            lane_handles = mapping["lane_handles"]
        head_lane = head_test.get("primary_lane") if isinstance(head_test, dict) else None
        head_lane_handles = (
            lane_handles.get(head_lane, {})
            if isinstance(head_lane, str)
            else {}
        )
        if not isinstance(head_lane_handles, dict):
            head_lane_handles = {}
        return {
            "count": len(unlocated) + len(planned),
            "planned_count": len(planned),
            "diagnostic_count": len(diagnostic_unlocated),
            "family_count": 1,
            "head": {
                "family_id": family_id,
                "plan_id": head_test.get("id"),
                "test_case": head_test.get("test_case"),
                "lane": head_lane,
                "ctest_filter": head_lane_handles.get("ctest_filter"),
                "assertions": head_test.get("assertions"),
                "requirement_ids": strings(head_test.get("lab_requirement_ids")),
                "standard_sections": strings(head_test.get("standard_sections")),
                "api_surfaces": strings(head_test.get("selected_cpp_api_surface_ids")),
                "trace_command": (
                    f'python tools/query_rti_work.py trace "{head_test.get("test_case")}" '
                    "--summary --compact"
                ),
            },
        }

    queue = indexed_work_queue(index, tests, limit=0)
    rows = [
        row
        for row in queue.get("items", [])
        if isinstance(row, dict)
        and (
            row.get(
                "unlocated_actionable_case_count",
                row.get("unlocated_case_count", 0),
            )
            or row.get("planned_count", 0)
        )
    ]
    head = rows[0] if rows else None
    if not isinstance(head, dict):
        return {
            "count": 0,
            "family_count": 0,
            "head": None,
        }
    planned_head = head.get("planned_head")
    planned_head_id = head.get("planned_head_id")
    planned_head_is_present = isinstance(planned_head, str) and bool(planned_head.strip())
    return {
        "count": queue.get("planned_case_count", 0)
        + queue.get(
            "unlocated_actionable_case_count",
            queue.get("unlocated_case_count", 0),
        ),
        "planned_count": queue.get("planned_case_count", 0),
        "diagnostic_count": queue.get("unlocated_case_count", 0),
        "family_count": len(rows),
        "head": {
            "family_id": head.get("id"),
            "plan_id": (
                planned_head_id
                if planned_head_is_present
                else head.get("unlocated_head_id")
            ),
            "test_case": (
                planned_head
                if planned_head_is_present
                else head.get("unlocated_head")
            ),
            "lane": (
                head.get("planned_head_lane")
                if planned_head_is_present
                else None
            ),
            "ctest_filter": (
                head.get("planned_head_ctest_filter")
                if planned_head_is_present
                else None
            ),
            "assertions": (
                head.get("planned_head_assertions")
                if planned_head_is_present
                else head.get("unlocated_head_assertions")
            ),
            "requirement_ids": strings(
                head.get("planned_head_requirement_ids")
                if planned_head_is_present
                else head.get("unlocated_head_requirement_ids")
            ),
            "standard_sections": strings(
                head.get("planned_head_standard_sections")
                if planned_head_is_present
                else head.get("unlocated_head_standard_sections")
            ),
            "api_surfaces": strings(
                head.get("planned_head_api_surfaces")
                if planned_head_is_present
                else head.get("unlocated_head_api_surfaces")
            ),
            "trace_command": (
                head.get("planned_head_trace_command")
                if planned_head_is_present
                else head.get("unlocated_head_trace_command")
            ),
        },
    }


def indexed_active_handoff_record(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    requested_family: str | None = None,
) -> dict[str, Any] | None:
    """Resolve the one deliberately selected next slice from the index.

    The normal source/planned queues are intentionally exhaustive.  A small
    ``active_handoff`` card is therefore allowed to name the next new case
    without pretending that it already exists in the Catch2 plan.  Its
    requirement and canonical-section counts come from an existing mapped
    seed row, so callers get the same direct requirement-to-subsection join
    used by ``case``/``matrix`` without a Requirements-Lab rescan.
    """

    card = index.get("active_handoff")
    if not isinstance(card, dict):
        return None
    family_id = card.get("family")
    if requested_family is not None and str(family_id or "").casefold() != requested_family.casefold():
        return None
    test_case = card.get("test_case")
    if not isinstance(test_case, str) or not test_case.strip():
        return None

    # Once the planned row is added, the regular indexed queue becomes the
    # source of truth and this proposal should no longer shadow it.
    if any(
        isinstance(test, dict) and test.get("test_case") == test_case
        for test in tests
    ):
        return None

    seed_id = card.get("mapping_seed_plan_id")
    seed = next(
        (
            test
            for test in tests
            if isinstance(test, dict) and test.get("id") == seed_id
        ),
        None,
    )
    if not isinstance(seed, dict):
        return {
            "found": False,
            "state": "unavailable",
            "reason": "active_handoff mapping seed is absent from the Catch2 plan",
            "active_handoff_id": card.get("id"),
        }

    requirement_ids = strings(seed.get("lab_requirement_ids"))
    standard_sections = strings(seed.get("standard_sections"))
    api_surfaces = strings(seed.get("selected_cpp_api_surface_ids"))
    pairs = requirement_section_mapping_rows(seed)
    raw_commands = card.get("commands")
    commands = (
        [value for value in raw_commands.values() if isinstance(value, str) and value]
        if isinstance(raw_commands, dict)
        else []
    )
    seed_case = seed.get("test_case")
    seed_trace_command = (
        f'python tools/query_rti_work.py trace "{seed_case}" --summary --compact'
        if isinstance(seed_case, str) and seed_case
        else None
    )
    return {
        "found": True,
        "runnable": False,
        "handoff_kind": "new-case",
        "state": str(card.get("state") or "new-case-needed"),
        "active_handoff_id": card.get("id"),
        "family_id": family_id,
        "test_case": test_case,
        "source_location": None,
        "source_target": card.get("source_target"),
        "source_lane": card.get("lane"),
        "lane_pending": True,
        "ctest_target": card.get("ctest_target"),
        "ctest_filter": card.get("ctest_filter"),
        "mapping_status": "seed-from-existing-case",
        "mapping_seed_plan_id": seed_id,
        "mapping_seed_test_case": seed_case,
        "mapping_seed_policy": card.get("mapping_seed_policy"),
        "requirement_ids": requirement_ids,
        "standard_sections": standard_sections,
        "api_surfaces": api_surfaces,
        "requirement_section_pairs": pairs,
        "requirement_section_pair_count": len(pairs),
        "next_action": card.get("objective"),
        "acceptance": card.get("acceptance", []),
        "commands": commands,
        "seed_trace_command": seed_trace_command,
        "implementation_command": (
            f"python tools/query_rti_work.py case {seed_id} --summary --compact"
            if isinstance(seed_id, str) and seed_id
            else None
        ),
    }


def ready_slice(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    source_locations: dict[str, list[dict[str, Any]]],
    requested_family: str | None = None,
    include_source_only: bool = False,
    standard_requirements: dict[str, dict[str, Any]] | None = None,
) -> dict[str, Any]:
    """Resolve one bounded implementation handoff.

    ``work`` is intentionally allowed to retain a completed baseline and its
    historical owner.  This view answers the narrower question that matters
    when resuming: which exact planned row, unplanned declaration, or
    deliberately indexed new-case handoff should be implemented next, and how
    is that row mapped?  It only consumes the
    already-derived active pointer and the deterministic queue head; it never
    performs fuzzy discovery or reopens the Requirements Lab.  Source-only
    declarations are diagnostic reconciliation work, not implementation work,
    unless the caller explicitly opts in with ``include_source_only``.  This
    prevents an unrelated external or compatibility test from displacing the
    selected 2025 roadmap family.
    """

    active = indexed_work_slice(
        index,
        tests,
        requested_family,
        source_locations,
    )
    if not active.get("found"):
        return {
            "found": False,
            "state": "none",
            "requested_family": requested_family,
            "include_source_only": include_source_only,
            "reason": active.get("reason") or "no indexed work item is open",
        }

    requested_item = next(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict)
            and str(item.get("id") or "").casefold()
            == str(requested_family or "").casefold()
        ),
        None,
    )
    # A family selector is a deliberate scope, not a request to inherit the
    # highest-priority parent's baseline.  Several broad families point back
    # to the same active owner, so keep the selected family as the handoff
    # owner and use its own planned-row queue below.
    parent = (
        roadmap_item_summary(requested_item)
        if isinstance(requested_item, dict)
        else active.get("parent_item") or {}
    )
    source_state = (
        source_pointer_state(requested_item)
        if isinstance(requested_item, dict)
        else active.get("next_source_state")
    )
    if isinstance(requested_item, dict):
        active = {
            **active,
            "lane": requested_item.get("next_lane"),
            "ctest_filter": requested_item.get("next_ctest_filter"),
            "next_source_test_query": requested_item.get("next_source_test_query"),
            "next_source_location": requested_item.get("next_source_location"),
            "next_source_lane": requested_item.get("next_source_lane"),
            "next_source_requirement_ids": requested_item.get(
                "next_source_requirement_ids"
            ),
            "next_source_standard_sections": requested_item.get(
                "next_source_standard_sections"
            ),
            "next_source_api_surfaces": requested_item.get(
                "next_source_api_surfaces"
            ),
            "next_source_ctest_filter": requested_item.get(
                "next_source_ctest_filter"
            ),
            "next_test_query": requested_item.get("next_test_query"),
            "next_test_role": requested_item.get("next_test_role"),
            "next_work_id": requested_item.get("next_work_id"),
            "work_id": requested_item.get("id"),
            "work_status": requested_item.get("status"),
        }
    record: dict[str, Any] | None = None

    # An explicitly indexed source declaration wins over a planned-row queue
    # because it is already present in the checkout and can be run/mapped.
    if source_state == "unplanned-source" and active.get("next_source_test_query"):
        record = {
            "state": "unplanned-source",
            "family_id": parent.get("id"),
            "test_case": active.get("next_source_test_query"),
            "source_location": active.get("next_source_location"),
            "source_lane": active.get("next_source_lane"),
            "requirement_ids": strings(active.get("next_source_requirement_ids")),
            "standard_sections": strings(active.get("next_source_standard_sections")),
            "api_surfaces": strings(active.get("next_source_api_surfaces")),
            "ctest_filter": active.get("next_source_ctest_filter"),
            "trace_command": (
                f'python tools/query_rti_work.py search "{active["next_source_test_query"]}" '
                "--summary --compact"
            ),
            "next_action": (
                "Add an explicit Catch2 plan row for this source declaration, then "
                "run its focused mapping check before treating it as evidence."
            ),
            "implementation_command": (
                "python tools/query_rti_work.py unplanned --summary --compact --limit 1"
            ),
        }
    else:
        global_queue = active.get("global_source_queue")
        if (
            include_source_only
            and source_state == "exhausted"
            and isinstance(global_queue, dict)
            and global_queue.get("count", 0)
        ):
            record = {
                "state": "unplanned-source",
                "family_id": parent.get("id"),
                "test_case": global_queue.get("test_query"),
                "source_location": global_queue.get("source_location"),
                "source_lane": None,
                "requirement_ids": [],
                "standard_sections": [],
                "api_surfaces": [],
                "ctest_filter": None,
                "trace_command": None,
                "next_action": (
                    "Add an explicit Catch2 plan row for this source declaration, then "
                    "run its focused mapping check before treating it as evidence."
                ),
                "implementation_command": global_queue.get("command"),
            }

        if record is None:
            planned_queue = (
                indexed_unlocated_plan_summary(index, tests, requested_family)
                if requested_family is not None
                else active.get("planned_queue")
            )
            head = planned_queue.get("head") if isinstance(planned_queue, dict) else None
            if isinstance(head, dict) and head.get("test_case"):
                exact = next(
                    (
                        test
                        for test in tests
                        if test.get("test_case") == head.get("test_case")
                    ),
                    None,
                )
                record = {
                    "state": "planned",
                    "family_id": head.get("family_id") or parent.get("id"),
                    "plan_id": head.get("plan_id"),
                    "test_case": head.get("test_case"),
                    "source_location": None,
                    "source_lane": head.get("lane"),
                    "requirement_ids": strings(head.get("requirement_ids")),
                    "standard_sections": strings(head.get("standard_sections")),
                    "api_surfaces": strings(head.get("api_surfaces")),
                    "assertions": head.get("assertions"),
                    "ctest_filter": head.get("ctest_filter"),
                    "trace_command": head.get("trace_command"),
                    "next_action": exact.get("next_action") if exact is not None else None,
                    "implementation_command": (
                        f'python tools/query_rti_work.py trace "{head["test_case"]}" '
                        "--summary --compact"
                    ),
                }
                if exact is not None:
                    exact_summary = test_summary_data(exact)
                    record.update(
                        {
                            "status": exact_summary.get("status"),
                            "tags": strings(exact.get("tags")),
                            "source_locations": exact_summary.get("source_locations", []),
                        }
                    )

    # When the implementation/source queues are exhausted, a selected family
    # can still have a real, source-located plan row waiting for an explicit
    # mapping decision.  Make that row a one-command handoff instead of
    # sending the caller back to a 12-row lane inventory.  This is deliberately
    # limited to an explicitly selected family and consumes only the checked-in
    # plan/index; it never searches or resynchronizes the Requirements Lab.
    if record is None and requested_family is not None:
        mapping_queue = lane_inventory(
            index,
            tests,
            family=requested_family,
            unmapped_only=True,
            limit=0,
        )
        for lane_row in mapping_queue.get("lanes", []):
            if not isinstance(lane_row, dict):
                continue
            # Explicit no-standalone-surface dispositions are intentional
            # decisions, not implementation handoffs. Only an unclassified
            # row belongs in this direct mapping queue.
            if not lane_row.get("unclassified_count"):
                continue
            next_test_id = lane_row.get("next_test_id")
            exact = next(
                (
                    test
                    for test in tests
                    if isinstance(test, dict) and test.get("id") == next_test_id
                ),
                None,
            )
            if not isinstance(exact, dict):
                continue
            locations = exact.get("source_locations", [])
            location = locations[0] if locations and isinstance(locations[0], dict) else None
            source_location = (
                f"{location.get('path')}:{location.get('line')}"
                if location and location.get("path")
                else None
            )
            handles = lane_row.get("handles")
            if not isinstance(handles, dict):
                handles = {}
            test_summary = test_summary_data(exact)
            trace_query = exact.get("test_case")
            record = {
                "state": "mapping",
                "family_id": requested_family,
                "plan_id": exact.get("id"),
                "test_case": trace_query,
                "source_location": source_location,
                "source_lane": lane_row.get("tag"),
                "requirement_ids": strings(exact.get("lab_requirement_ids")),
                "standard_sections": strings(exact.get("standard_sections")),
                "api_surfaces": strings(exact.get("selected_cpp_api_surface_ids")),
                "assertions": exact.get("assertions"),
                "status": test_summary.get("status"),
                "ctest_filter": handles.get("ctest_filter"),
                "trace_command": (
                    f'python tools/query_rti_work.py trace "{trace_query}" '
                    "--summary --compact"
                    if isinstance(trace_query, str)
                    else None
                ),
                "next_action": exact.get("next_action")
                or (
                    "Add the explicit Requirements-Lab mapping for this source-located "
                    "case, then run its focused mapping check before implementation "
                    "or conformance claims."
                ),
                "implementation_command": (
                    f'python tools/query_rti_work.py trace "{trace_query}" '
                    "--summary --compact"
                    if isinstance(trace_query, str)
                    else None
                ),
            }
            break

    # When all indexed source and plan-row queues are exhausted, honor the
    # deliberately selected handoff in the roadmap index.  This is a bounded
    # new-case proposal, not a Lab coverage scan; its traceability seed is
    # resolved from the existing Catch2 plan by ``indexed_active_handoff_record``.
    if record is None:
        active_handoff = indexed_active_handoff_record(
            index,
            tests,
            requested_family=requested_family,
        )
        if isinstance(active_handoff, dict) and active_handoff.get("found"):
            record = active_handoff

    if record is None:
        family_queue = indexed_work_queue(index, tests, limit=0)
        family_options = [
            {
                "id": row.get("id"),
                "title": row.get("title"),
                "priority": row.get("priority"),
                "state": row.get("state"),
                "action_state": row.get("action_state"),
                "case_count": row.get("case_count", 0),
                "mapped_case_count": row.get("mapped_case_count", 0),
                "family_case_count": row.get(
                    "family_case_count", row.get("case_count", 0)
                ),
                "family_mapped_case_count": row.get(
                    "family_mapped_case_count", row.get("mapped_case_count", 0)
                ),
                "source_drift_count": row.get("source_drift_count", 0),
                "actionable_source_drift_count": row.get(
                    "actionable_source_drift_count", 0
                ),
                "unlocated_case_count": row.get("unlocated_case_count", 0),
                "requirement_count": row.get("requirement_count", 0),
                "standard_section_count": row.get("standard_section_count", 0),
                "cpp_api_surface_count": row.get("cpp_api_surface_count", 0),
                "requirement_section_pair_count": row.get(
                    "requirement_section_pair_count", 0
                ),
                "requirement_ids_preview": row.get(
                    "requirement_ids_preview", []
                ),
                "requirement_ids_remaining": row.get(
                    "requirement_ids_remaining", 0
                ),
                "standard_sections_preview": row.get(
                    "standard_sections_preview", []
                ),
                "standard_sections_remaining": row.get(
                    "standard_sections_remaining", 0
                ),
                "requirement_section_pairs_preview": row.get(
                    "requirement_section_pairs_preview", []
                ),
                "requirement_section_pairs_remaining": row.get(
                    "requirement_section_pairs_remaining", 0
                ),
                "next_lane": row.get("next_lane"),
                "next_test_query": row.get("next_test_query"),
                "next_test_state": row.get("next_test_state"),
                "unclassified_count": row.get("unclassified_count", 0),
                "explicit_disposition_count": row.get(
                    "explicit_disposition_count", 0
                ),
                "next_test_mapping": (
                    {
                        "assertion_count": row.get("next_test_pointer", {}).get(
                            "assertion_count", 0
                        ),
                        "requirement_count": row.get("next_test_pointer", {}).get(
                            "requirement_count", 0
                        ),
                        "standard_section_count": row.get(
                            "next_test_pointer", {}
                        ).get("standard_section_count", 0),
                        "cpp_api_surface_count": row.get(
                            "next_test_pointer", {}
                        ).get("cpp_api_surface_count", 0),
                    }
                    if isinstance(row.get("next_test_pointer"), dict)
                    and row.get("next_test_pointer", {}).get("query")
                    else None
                ),
                "next_action": row.get("next_action"),
                "work_query": (
                    f"python tools/query_rti_work.py work {row.get('id')} "
                    "--summary --compact"
                    if row.get("id")
                    else None
                ),
                "lane_discovery_command": (
                    f"python tools/query_rti_work.py lanes --family {row.get('id')} "
                    "--unmapped --summary --compact --limit 12"
                    if row.get("id")
                    else None
                ),
                "mapping_lane_discovery_command": (
                    f"python tools/query_rti_work.py lanes --family {row.get('id')} "
                    "--disposition unclassified --summary --compact --limit 12"
                    if row.get("id")
                    else None
                ),
                "gap_query": (
                    f"python tools/query_rti_work.py gaps --family {row.get('id')} "
                    "--summary --compact --limit 12"
                    if row.get("id")
                    else None
                ),
            }
            for row in family_queue.get("items", [])
            if isinstance(row, dict)
            and (
                requested_family is None
                and row.get("action_state") != "evidence-complete"
                and row.get("state") != "complete-pointer"
                or requested_family is not None
                and row.get("id") == requested_family
            )
        ][:3]
        for option in family_options:
            if not isinstance(option, dict):
                continue
            # Only a deliberate new-case queue should surface a coverage head
            # as a bounded inventory pointer. Evidence-complete and
            # external-review families retain their existing handles and are
            # not presented as implementation work.
            if option.get("action_state") != "new-case-needed":
                continue
            family_id = option.get("id")
            gap = family_gap_preview(
                index,
                tests,
                standard_requirements,
                family_id if isinstance(family_id, str) else None,
            )
            if gap is not None:
                option["gap_preview"] = gap
        selected_option = next(
            (
                option
                for option in family_options
                if isinstance(option, dict)
                and option.get("id") == requested_family
            ),
            None,
        )
        if (
            isinstance(selected_option, dict)
            and selected_option.get("action_state") == "evidence-complete"
        ):
            reason = (
                "the selected roadmap family is evidence-complete; "
                "no runnable indexed action is queued"
            )
        elif family_options:
            reason = (
                "the indexed source and planned-row queues are exhausted; "
                "only explicitly queued family actions remain"
            )
        else:
            reason = (
                "the indexed implementation queues are exhausted; "
                "no family has a runnable or explicitly queued action"
            )
        return {
            "found": False,
            "state": "none",
            "requested_family": requested_family,
            "include_source_only": include_source_only,
            "owner": parent,
            "reason": reason,
            "family_options": family_options,
            "family_queue_count": len(family_options),
            "evidence_complete_families": [
                row.get("id")
                for row in family_queue.get("items", [])
                if isinstance(row, dict)
                and row.get("action_state") == "evidence-complete"
            ],
        }

    family_id = record.get("family_id")
    family_item = next(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("id") == family_id
        ),
        None,
    )
    record["found"] = True
    record["requested_family"] = requested_family
    record["include_source_only"] = include_source_only
    record["owner"] = parent
    record["roadmap_owner"] = (
        roadmap_item_summary(family_item)
        if isinstance(family_item, dict)
        else parent
    )
    record["active_pointer"] = parent
    # A planned or mapping handoff is owned by the family that contains the
    # exact row, even when the global queue was reached through a different
    # historical active pointer.  Returning that family id keeps the resume
    # card's ``work`` handle aligned with its ``trace``/``focus`` handles.
    record["work_id"] = (
        family_id
        if record.get("state") in {"planned", "mapping", "new-case-needed"} and family_id
        else parent.get("id")
    )
    record["active_work_id"] = active.get("work_id")
    record["work_status"] = active.get("work_status")
    record["planned_queue_count"] = (
        active.get("planned_queue", {}).get("count", 0)
        if isinstance(active.get("planned_queue"), dict)
        else 0
    )
    record["planned_queue_diagnostic_count"] = (
        active.get("planned_queue", {}).get(
            "diagnostic_count", record["planned_queue_count"]
        )
        if isinstance(active.get("planned_queue"), dict)
        else record["planned_queue_count"]
    )
    lane_command = (
        f"python tools/query_rti_work.py focus {record['source_lane']} --summary --compact"
        if record.get("source_lane")
        else None
    )
    check_command = (
        f"python tools/query_rti_work.py check --lane {record['source_lane']} --summary --compact"
        if record.get("source_lane")
        else None
    )
    if record.get("lane_pending"):
        record["post_mapping_focus_command"] = lane_command
        record["post_mapping_check_command"] = check_command
    else:
        record["focus_command"] = lane_command
        record["check_command"] = check_command
    return record


def trace_requirement_row(requirement: dict[str, Any]) -> dict[str, Any]:
    """Return the direct Lab-requirement to 2025-subsection relationship."""

    standard = requirement.get("standard")
    if not isinstance(standard, dict):
        return {
            "lab_requirement_id": requirement.get("lab_requirement_id"),
            "unresolved": True,
        }
    source = standard.get("source")
    source_value = source if isinstance(source, dict) else None
    return {
        "lab_requirement_id": requirement.get("lab_requirement_id"),
        "document_id": standard.get("document_id"),
        "clause": standard.get("clause"),
        "clause_id": standard.get("clause_id"),
        "title": standard.get("title"),
        "statement": standard.get("statement") or standard.get("machine_statement"),
        "kind": standard.get("kind"),
        "coverage_kind": standard.get("coverage_kind"),
        "source": source_value,
        "unresolved": False,
    }


def requirement_section_mapping_rows(
    test: dict[str, Any],
) -> list[dict[str, Any]]:
    """Return the direct Lab-requirement to canonical subsection pairs.

    The Catch2 plan stores requirement selections while the 2025 subsection is
    resolved from the pinned CorpusBundle.  Keep that join in one helper so
    every bounded query (not only ``matrix``) can expose the same relationship
    without asking a caller to infer it by zipping two independently ordered
    arrays.
    """

    rows: list[dict[str, Any]] = []
    for requirement in test.get("requirements", []):
        if not isinstance(requirement, dict):
            continue
        standard = requirement.get("standard")
        if not isinstance(standard, dict):
            rows.append(
                {
                    "lab_requirement_id": requirement.get("lab_requirement_id"),
                    "standard_section": None,
                    "document_id": None,
                    "clause_id": None,
                    "title": None,
                    "unresolved": True,
                }
            )
            continue
        document_id = standard.get("document_id")
        clause_id = standard.get("clause_id")
        section = (
            f"{document_id}:{clause_id}"
            if isinstance(document_id, str)
            and document_id
            and isinstance(clause_id, str)
            and clause_id
            else None
        )
        rows.append(
            {
                "lab_requirement_id": requirement.get("lab_requirement_id"),
                "standard_section": section,
                "document_id": document_id,
                "clause_id": clause_id,
                "title": standard.get("title"),
                "unresolved": section is None,
            }
        )
    return rows


def exact_requirement_match(requirement: dict[str, Any], query: str) -> bool:
    """Match only stable requirement/contract identifiers for ``trace``."""

    query_folded = query.casefold()
    values = strings(requirement.get("lab_requirement_id"))
    for contract in requirement.get("contracts", []):
        if isinstance(contract, dict):
            values.extend(strings(contract.get("contract_requirement_id")))
    return any(query_folded == value.casefold() for value in values)


def api_surface_match(surface: Any, query: str) -> bool:
    """Match a canonical API id or one unambiguous C++ method shorthand.

    Plan rows carry generated identifiers such as
    ``api.2025.cpp.rtiambassador.nextmessagerequest.<hash>``.  Requiring that
    full identifier makes reverse queries unnecessarily expensive to discover,
    while a free-text substring would make ``trace`` unbounded.  Compare the
    normalized query against one dot/namespace-separated API segment (or a
    contiguous qualified sequence) so inputs such as ``next_message_request``
    and ``RTIambassador::nextMessageRequest`` remain deterministic.
    """

    if not isinstance(surface, str) or not isinstance(query, str):
        return False
    if surface.casefold() == query.casefold():
        return True

    def parts(value: str) -> list[str]:
        return [
            search_key(part)
            for part in re.split(r"[^a-zA-Z0-9]+", value)
            if search_key(part)
        ]

    surface_parts = parts(surface)
    query_parts = parts(query)
    if not surface_parts or not query_parts:
        return False
    query_key = "".join(query_parts)
    if query_key in surface_parts:
        return True
    width = len(query_parts)
    return any(
        "".join(surface_parts[offset : offset + width]) == query_key
        for offset in range(len(surface_parts) - width + 1)
    )


def trace_section_match(section: str, query: str) -> bool:
    """Match one canonical document:clause key or a clause-only shorthand.

    Human-facing reports commonly show ``document_id:clause-6.8.4`` while a
    developer usually asks for the much shorter ``6.8.4``.  Treat the numeric
    clause id as an exact key (not a fuzzy substring) so section and matrix
    lookups remain bounded and deterministic.
    """

    folded = query.casefold()
    normalized = search_key(query)
    values = (section, section.replace(":", " "), section.rsplit(":", 1)[-1])
    if any(
        folded == value.casefold()
        or normalized == search_key(value)
        or (
            normalized
            and search_key(value).endswith(normalized)
            and normalized.startswith("clause")
        )
        for value in values
    ):
        return True

    clause_suffix = values[-1]
    clause_number = re.sub(
        r"^clause(?:[-_ ]*)",
        "",
        clause_suffix,
        flags=re.IGNORECASE,
    )
    query_number = re.sub(
        r"^clause(?:[-_ ]*)",
        "",
        query.strip(),
        flags=re.IGNORECASE,
    )
    return bool(
        re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", query_number)
        and search_key(query_number) == search_key(clause_number)
    )


def resolve_trace_query(
    tests: list[dict[str, Any]],
    query: str,
) -> tuple[str | None, list[dict[str, Any]]]:
    """Resolve one exact trace handle without falling back to broad search.

    The precedence is deliberate: exact plan/test handles, exact C++ API
    surface identifiers, exact requirement identifiers, exact contract mapping
    identifiers, exact standard subsection keys, then exact Catch2 lane tags.
    A fuzzy repository-wide search remains available through ``search`` and is
    never implicit in this bounded trace command.
    """

    folded = query.casefold()
    exact_tests = [
        test
        for test in tests
        if str(test.get("id", "")).casefold() == folded
        or str(test.get("test_case", "")).casefold() == folded
    ]
    if exact_tests:
        return "test", exact_tests

    exact_api_surfaces = [
        test
        for test in tests
        if any(
            str(surface).casefold() == folded
            for surface in strings(test.get("selected_cpp_api_surface_ids"))
        )
    ]
    if exact_api_surfaces:
        return "api", exact_api_surfaces

    shorthand_api_surfaces = [
        test
        for test in tests
        if any(
            api_surface_match(surface, query)
            for surface in strings(test.get("selected_cpp_api_surface_ids"))
        )
    ]
    if shorthand_api_surfaces:
        return "api", shorthand_api_surfaces

    exact_requirements = [
        test
        for test in tests
        if any(
            isinstance(requirement, dict)
            and exact_requirement_match(requirement, query)
            for requirement in test.get("requirements", [])
        )
    ]
    if exact_requirements:
        return "requirement", exact_requirements

    exact_contract_mappings = [
        test
        for test in tests
        if str(test.get("requirements_lab_mapping_id", "")).casefold() == folded
    ]
    if exact_contract_mappings:
        return "contract", exact_contract_mappings

    exact_sections = [
        test
        for test in tests
        if any(
            trace_section_match(section, query)
            for section in strings(test.get("standard_sections"))
        )
    ]
    if exact_sections:
        return "section", exact_sections

    exact_lanes = [
        test
        for test in tests
        if any(str(tag).casefold() == folded for tag in strings(test.get("tags")))
    ]
    if exact_lanes:
        return "lane", exact_lanes
    return None, []


def resolve_matrix_query(
    index: dict[str, Any],
    tests: list[dict[str, Any]],
    query: str,
) -> tuple[str | None, list[dict[str, Any]]]:
    """Resolve a matrix handle, including an indexed roadmap family id.

    ``trace`` intentionally accepts only exact test/requirement/section/lane
    handles.  ``matrix`` is the reverse aggregate view, so it additionally
    accepts one exact roadmap family id after those stable handles have been
    attempted.  Keeping the fallback here makes the CLI and offline regression
    use the same deterministic resolution order.
    """

    match_kind, matches = resolve_trace_query(tests, query)
    if matches:
        return match_kind, matches
    item, family_tests = indexed_item(index, tests, query)
    if item is not None:
        return "family", family_tests
    return None, []


def trace_record(
    test: dict[str, Any],
    match_kind: str,
    query: str,
    index: dict[str, Any] | None = None,
) -> dict[str, Any]:
    """Build a bounded, direct test-to-requirement trace record."""

    requirements = [
        requirement
        for requirement in test.get("requirements", [])
        if isinstance(requirement, dict)
    ]
    if match_kind == "requirement":
        requirements = [
            requirement
            for requirement in requirements
            if exact_requirement_match(requirement, query)
        ]
    elif match_kind == "section":
        requirements = [
            requirement
                for requirement in requirements
            if isinstance(requirement.get("standard"), dict)
            and trace_section_match(
                ":".join(
                    str(value)
                    for value in (
                        requirement["standard"].get("document_id"),
                        requirement["standard"].get("clause_id"),
                    )
                    if value
                ),
                query,
            )
        ]
    roadmap_items = roadmap_links_for_test(index, test) if index is not None else []
    roadmap_owner = primary_roadmap_link(roadmap_items)
    return {
        "id": test.get("id"),
        "test_case": test.get("test_case"),
        "status": test.get("status"),
        "source_locations": test.get("source_locations", []),
        "source_state": test.get("source_state"),
        "assertions": test.get("assertions"),
        "passed_assertions": test.get("passed_assertions"),
        "failed_assertions": test.get("failed_assertions"),
        "expected_assertions_if_harness_fixed": test.get(
            "expected_assertions_if_harness_fixed"
        ),
        "failure_location": test.get("failure_location"),
        "failure_summary": test.get("failure_summary"),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "requirements_lab_api_surface_status": test.get(
            "requirements_lab_api_surface_status"
        ),
        "tags": strings(test.get("tags")),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "standard_sections": strings(test.get("standard_sections")),
        "roadmap_items": roadmap_items,
        "roadmap_owner": roadmap_owner.get("id") if roadmap_owner else None,
        "requirement_mapping_count": len(
            strings(test.get("lab_requirement_ids"))
        ),
        "matched_requirement_count": len(requirements),
        "requirement_mappings": [
            trace_requirement_row(requirement) for requirement in requirements
        ],
    }


def trace_summary_record(record: dict[str, Any], limit: int = 8) -> dict[str, Any]:
    """Bound the mapping rows while retaining exact counts for quick queries."""

    result = dict(record)
    for field in ("tags", "cpp_api_surfaces", "standard_sections"):
        values = strings(record.get(field))
        result[field] = values[:limit]
        if len(values) > limit:
            result[f"{field}_remaining"] = len(values) - limit
    mappings = record.get("requirement_mappings", [])
    if isinstance(mappings, list):
        result["requirement_mappings"] = mappings[:limit]
        if len(mappings) > limit:
            result["requirement_mappings_remaining"] = len(mappings) - limit
    roadmap_items = record.get("roadmap_items", [])
    if isinstance(roadmap_items, list):
        result["roadmap_items"] = roadmap_items[:limit]
        if len(roadmap_items) > limit:
            result["roadmap_items_remaining"] = len(roadmap_items) - limit
    return result


def case_card_record(
    test: dict[str, Any],
    index: dict[str, Any],
    query: str,
) -> dict[str, Any]:
    """Build one bounded handoff card for an exact Catch2 plan case."""

    trace = trace_record(test, "test", query, index)
    roadmap_items = roadmap_links_for_test(index, test)
    owner = primary_roadmap_link(roadmap_items)
    lane_handles_by_tag = (
        index.get("mapping", {}).get("lane_handles", {})
        if isinstance(index.get("mapping"), dict)
        else {}
    )
    lane: str | None = None
    lane_handles: dict[str, Any] = {}
    if isinstance(lane_handles_by_tag, dict):
        lane_tags = strings(test.get("tags"))
        primary_lane = str(test.get("primary_lane") or "").strip()
        if primary_lane:
            lane_tags = [primary_lane] + [
                tag for tag in lane_tags if tag.casefold() != primary_lane.casefold()
            ]
        for tag in lane_tags:
            configured = lane_handles_by_tag.get(tag)
            if isinstance(configured, dict):
                lane = tag
                lane_handles = dict(configured)
                break

    case_id = str(test.get("id") or query)
    case_title = str(test.get("test_case") or case_id)
    quoted_id = json.dumps(case_id, ensure_ascii=False)
    commands: dict[str, str] = {
        "test": (
            "python tools/query_rti_work.py test "
            f"{quoted_id} --summary --compact"
        ),
        "trace": (
            "python tools/query_rti_work.py trace "
            f"{quoted_id} --summary --compact"
        ),
        "matrix": (
            "python tools/query_rti_work.py matrix "
            f"{quoted_id} --summary --compact"
        ),
    }
    if lane:
        commands["focus"] = (
            "python tools/query_rti_work.py focus "
            f"{lane} --summary --compact"
        )
        commands["check"] = (
            "python tools/query_rti_work.py check --lane "
            f"{lane} --summary --compact"
        )
    ctest = lane_ctest_command(lane_handles)
    if ctest:
        commands["ctest"] = ctest

    return {
        "query": query,
        "id": case_id,
        "test_case": case_title,
        "status": test.get("status"),
        "traceability_state": traceability_state(test),
        "source_locations": test.get("source_locations", []),
        "source_state": test.get("source_state"),
        "assertions": test.get("assertions"),
        "callback_models": strings(test.get("callback_models")),
        "requirements": strings(test.get("lab_requirement_ids")),
        "standard_sections": strings(test.get("standard_sections")),
        "requirement_section_mappings": requirement_section_mapping_rows(test),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "requirements_lab_api_surface_status": test.get(
            "requirements_lab_api_surface_status"
        ),
        "next_action": test.get("next_action"),
        "roadmap_owner": owner.get("id") if owner else None,
        "roadmap_items": roadmap_items,
        "lane": lane,
        "lane_handles": lane_handles,
        "trace": trace_summary_record(trace),
        "matrix": matrix_summary_data(test, index),
        "commands": commands,
    }


def text_case_card(card: dict[str, Any], compact: bool = False) -> str:
    """Render one exact case card without expanding unrelated plan rows."""

    requirements = strings(card.get("requirements"))
    sections = strings(card.get("standard_sections"))
    api_surfaces = strings(card.get("cpp_api_surfaces"))
    mappings = card.get("requirement_section_mappings")
    if not isinstance(mappings, list):
        mappings = []

    def preview(values: list[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        suffix = f", ... (+{remaining})" if remaining > 0 else ""
        return shown + suffix

    mapping_preview = [
        f"{row.get('lab_requirement_id', '<unnamed requirement>')} -> "
        f"{row.get('standard_section') or '<unresolved>'}"
        for row in mappings
        if isinstance(row, dict)
    ]
    lines = [
        f"Case card: {card.get('id', '<unnamed>')}: {card.get('test_case', '<unnamed test>')}",
        f"status: {card.get('status', '<unspecified>')}",
        f"traceability: {card.get('traceability_state', '<unspecified>')}",
        f"source: {source_location_text(card)}",
        f"assertions: {card.get('assertions', '<unspecified>')}; "
        f"callback_models={', '.join(strings(card.get('callback_models'))) or '<unspecified>'}",
        f"roadmap_owner: {card.get('roadmap_owner') or '<none>'}",
        f"lane: {card.get('lane') or '<none>'}",
        f"mapping: requirements={len(requirements)} sections={len(sections)} "
        f"direct_pairs={len(mapping_preview)} api_surfaces={len(api_surfaces)}",
        f"requirements: {preview(requirements)}",
        f"standard_sections: {preview(sections)}",
        f"requirement_section_mappings: {preview(mapping_preview)}",
        f"cpp_api_surfaces: {preview(api_surfaces)}",
        "commands:",
    ]
    disposition = card.get("requirements_lab_api_surface_status")
    if isinstance(disposition, str) and disposition.strip():
        lines.insert(
            3,
            "Requirements-Lab note: " + compact_prose(disposition, 360),
        )
    next_action = card.get("next_action")
    if isinstance(next_action, str) and next_action.strip():
        lines.insert(4, "next_action: " + compact_prose(next_action, 360))
    commands = card.get("commands")
    if isinstance(commands, dict):
        for name in ("test", "trace", "matrix", "focus", "check", "ctest"):
            command = commands.get(name)
            if isinstance(command, str) and command:
                lines.append(f"  {name}={command}")
    return "\n".join(lines)


def text_trace(record: dict[str, Any], summary: bool = False) -> str:
    """Render one direct test → requirement → subsection trace."""

    def preview(values: list[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    tags = strings(record.get("tags"))
    api_surfaces = strings(record.get("cpp_api_surfaces"))
    roadmap_items = record.get("roadmap_items")
    if not isinstance(roadmap_items, list):
        roadmap_items = []
    roadmap_owner = primary_roadmap_link(roadmap_items)
    lines = [
        f"- {record.get('id', '<unnamed>')}: {record.get('test_case', '<unnamed test>')}",
        f"  status: {record.get('status', '<unspecified>')}",
        f"  source: {source_location_text(record)}",
        f"  mapping id: {record.get('requirements_lab_mapping_id') or '<none>'}",
        f"  tags: {preview(tags) if summary else ', '.join(tags) or '<none>'}",
        f"  cpp api surfaces: {preview(api_surfaces) if summary else ', '.join(api_surfaces) or '<none>'}",
        "  roadmap owner: "
        + (
            f"{roadmap_owner.get('id')} ({roadmap_owner.get('match')})"
            if roadmap_owner and roadmap_owner.get("id")
            else "<none>"
        ),
        "  roadmap families: "
        + (
            ", ".join(
                f"{item.get('id')} ({item.get('match')})"
                for item in roadmap_items
                if isinstance(item, dict)
            )
            or "<none>"
        ),
        "  requirement mappings:",
    ]
    if isinstance(record.get("failure_location"), str) and record["failure_location"].strip():
        lines.append(f"  failure_location: {record['failure_location']}")
    if isinstance(record.get("failure_summary"), str) and record["failure_summary"].strip():
        lines.append(f"  failure_summary: {compact_prose(record['failure_summary'], 240)}")
    if any(
        isinstance(record.get(field), int)
        for field in (
            "passed_assertions",
            "failed_assertions",
            "expected_assertions_if_harness_fixed",
        )
    ):
        lines.append(
            "  assertion_result: "
            f"observed={record.get('assertions', '<unspecified>')}; "
            f"passed={record.get('passed_assertions', '<unspecified>')}; "
            f"failed={record.get('failed_assertions', '<unspecified>')}; "
            f"expected_after_harness_fix={record.get('expected_assertions_if_harness_fixed', '<unspecified>')}"
        )
    api_surface_status = record.get("requirements_lab_api_surface_status")
    if isinstance(api_surface_status, str) and api_surface_status.strip():
        lines.insert(
            5,
            "  Requirements-Lab API surface: "
            + compact_prose(api_surface_status, 360),
        )
    mappings = record.get("requirement_mappings", [])
    if not isinstance(mappings, list) or not mappings:
        lines.append("    - <none>")
        return "\n".join(lines)
    shown = mappings[:8] if summary else mappings
    for mapping in shown:
        if not isinstance(mapping, dict):
            continue
        requirement_id = mapping.get("lab_requirement_id", "<unnamed requirement>")
        if mapping.get("unresolved"):
            lines.append(f"    - {requirement_id} (unresolved)")
            continue
        section = ":".join(
            str(value)
            for value in (mapping.get("document_id"), mapping.get("clause_id"))
            if value
        ) or "<unmapped section>"
        title = mapping.get("title") or ""
        lines.append(f"    - {requirement_id} -> {section}; {title}")
        statement = mapping.get("statement")
        if isinstance(statement, str) and statement.strip():
            lines.append(f"      statement: {compact_prose(statement, 220)}")
    remaining = len(mappings) - len(shown)
    if remaining > 0:
        lines.append(f"    - ... (+{remaining}); use trace --json for all rows")
    return "\n".join(lines)


def matrix_summary_data(
    test: dict[str, Any],
    index: dict[str, Any],
) -> dict[str, Any]:
    """Return one exact test-to-standard row for the bounded matrix view."""

    result = test_summary_data(test)
    result["tags"] = strings(test.get("tags"))
    roadmap_items = roadmap_links_for_test(index, test)
    result["roadmap_items"] = roadmap_items
    roadmap_owner = primary_roadmap_link(roadmap_items)
    result["roadmap_owner"] = roadmap_owner.get("id") if roadmap_owner else None
    return result


def text_matrix_row(
    test: dict[str, Any],
    index: dict[str, Any],
    compact: bool = False,
    mapping_rows: list[dict[str, Any]] | None = None,
) -> str:
    """Render one compact test → requirement → subsection matrix row."""

    requirements = strings(test.get("lab_requirement_ids"))
    sections = strings(test.get("standard_sections"))
    api_surfaces = strings(
        test.get("selected_cpp_api_surface_ids") or test.get("cpp_api_surfaces")
    )
    roadmap_items = test.get("roadmap_items")
    if not isinstance(roadmap_items, list):
        roadmap_items = roadmap_links_for_test(index, test)

    def preview(values: list[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        if not compact:
            return ", ".join(values)
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status={test.get('status', '<unspecified>')} "
        f"source={source_location_text(test)} assertions={test.get('assertions', '<unspecified>')} "
        f"requirements={len(requirements)} sections={len(sections)} api_surfaces={len(api_surfaces)}",
        f"  requirements: {preview(requirements)}",
        f"  standard_sections: {preview(sections)}",
    ]
    requirement_section_mappings = mapping_rows
    if not isinstance(requirement_section_mappings, list):
        requirement_section_mappings = test.get("requirement_section_mappings")
    if not isinstance(requirement_section_mappings, list):
        requirement_section_mappings = matrix_summary_data(test, index).get(
            "requirement_section_mappings", []
        )
    if isinstance(requirement_section_mappings, list):
        mapping_values = [
            f"{row.get('lab_requirement_id', '<unnamed requirement>')} -> "
            f"{row.get('standard_section') or '<unresolved>'}"
            for row in requirement_section_mappings
            if isinstance(row, dict)
        ]
        if mapping_values:
            lines.append(
                "  requirement_section_mappings: "
                f"{preview(mapping_values)}"
            )
    if roadmap_items:
        owner = primary_roadmap_link(roadmap_items)
        if owner and owner.get("id"):
            lines.append(f"  roadmap_owner: {owner['id']} ({owner.get('match')})")
        owners = [
            str(item.get("id"))
            for item in roadmap_items
            if item.get("id")
        ]
        if owners:
            lines.append(f"  roadmap: {', '.join(owners)}")
    if api_surfaces and not compact:
        lines.append(f"  cpp_api_surfaces: {preview(api_surfaces)}")
    return "\n".join(lines)


def text_query_summary_row(
    command: str,
    test: dict[str, Any],
    index: dict[str, Any],
    query: str | None = None,
) -> str:
    """Render one bounded row for a reverse requirement/section lookup.

    Reverse lookups need the direct mapping pair to be useful, but they do not
    need the full contract-oriented test summary.  Keeping this policy in one
    helper makes the CLI behavior easy to regression-test and keeps ``matrix``
    and the reverse lookup commands visually consistent.
    """

    if command in {"requirement", "section"}:
        mapping_rows = requirement_section_mapping_rows(test)
        if isinstance(query, str) and query:
            if command == "section":
                matched_rows = [
                    row
                    for row in mapping_rows
                    if isinstance(row, dict)
                    and isinstance(row.get("standard_section"), str)
                    and trace_section_match(row["standard_section"], query)
                ]
                # Keep the reverse row exact even when a test matched the
                # section through a legacy/canonical field but has no direct
                # Lab join for that subsection.  Showing every pair in that
                # case makes a bounded lookup look like a fuzzy aggregate.
                mapping_rows = matched_rows
            elif command == "requirement":
                matched_rows = [
                    row
                    for row in mapping_rows
                    if isinstance(row, dict)
                    and str(row.get("lab_requirement_id") or "").casefold()
                    == query.casefold()
                ]
                mapping_rows = matched_rows
        return text_matrix_row(
            test,
            index,
            compact=True,
            mapping_rows=mapping_rows,
        )
    return text_test_summary(test)


def test_search_values(test: dict[str, Any]) -> list[str]:
    """Return all human-queryable identifiers carried by a mapped test."""

    values: list[str] = []
    for field in (
        "id",
        "test_case",
        "requirements_lab_mapping_id",
        "requirements_lab_api_surface_status",
        "status",
        "failure_location",
        "failure_summary",
        "tags",
        "selected_cpp_api_surface_ids",
        "standard_clauses",
        "standard_sections",
        "source_state",
        "lab_requirement_ids",
    ):
        values.extend(strings(test.get(field)))
    for requirement in test.get("requirements", []):
        if not isinstance(requirement, dict):
            continue
        values.extend(strings(requirement.get("lab_requirement_id")))
        standard = requirement.get("standard")
        if isinstance(standard, dict):
            for field in ("document_id", "clause", "clause_id", "title"):
                values.extend(strings(standard.get(field)))
            source = standard.get("source")
            if isinstance(source, dict):
                values.extend(strings(source.get("path")))
                if source.get("line") is not None:
                    values.append(str(source["line"]))
        for contract in requirement.get("contracts", []):
            if isinstance(contract, dict):
                values.extend(strings(contract.get("contract_requirement_id")))
                values.extend(strings(contract.get("source_symbol")))
    for location in test.get("source_locations", []):
        if isinstance(location, dict):
            values.extend(strings(location.get("path")))
            if location.get("line") is not None:
                values.append(str(location["line"]))
    return values


def select_tests(tests: list[dict[str, Any]], query: str, mode: str) -> list[dict[str, Any]]:
    folded = query.casefold()
    if mode == "lane":
        return [
            test
            for test in tests
            if any(str(tag).casefold() == folded for tag in strings(test.get("tags")))
        ]
    if mode == "section":
        # Keep section selection on the same exact matcher used by trace and
        # matrix.  This accepts the canonical document:clause key as well as
        # ``clause-9.13.1`` and the numeric shorthand ``9.13.1`` without
        # turning a section lookup into a broad free-text search.
        selected: list[dict[str, Any]] = []
        for test in tests:
            if any(
                trace_section_match(section, query)
                for section in strings(test.get("standard_sections"))
            ):
                selected.append(test)
        return selected
    if mode == "requirement":
        selected: list[dict[str, Any]] = []
        for test in tests:
            for requirement in test.get("requirements", []):
                standard = requirement.get("standard") or {}
                contract_ids = {
                    str(contract.get("contract_requirement_id"))
                    for contract in requirement.get("contracts", [])
                }
                values = {
                    requirement.get("lab_requirement_id"),
                    standard.get("clause"),
                    standard.get("clause_id"),
                    standard.get("document_id"),
                    standard.get("title"),
                    *contract_ids,
                }
                document_id = standard.get("document_id")
                clause_id = standard.get("clause_id")
                if document_id and clause_id:
                    # The compact test output publishes this canonical
                    # document:clause key. Accept it as a direct query too,
                    # so selecting a standard subsection does not require a
                    # second lookup to discover the Lab requirement id.
                    values.add(f"{document_id}:{clause_id}")
                    values.add(f"{document_id} {clause_id}")
                source = standard.get("source")
                if isinstance(source, dict):
                    values.update(
                        value
                        for value in (source.get("path"), source.get("line"))
                        if value is not None
                    )
                normalized = search_key(query)
                if query in values or any(
                    isinstance(value, str)
                    and (
                        folded in value.casefold()
                        or (normalized and normalized in search_key(value))
                    )
                    for value in values
                ):
                    selected.append(test)
                    break
        return selected
    if mode == "search":
        normalized = search_key(query)
        return [
            test
            for test in tests
            if any(
                folded in value.casefold()
                or (normalized and normalized in search_key(value))
                for value in test_search_values(test)
            )
        ]
    return [
        test
        for test in tests
        if folded in str(test.get("id", "")).casefold()
        or folded in str(test.get("test_case", "")).casefold()
    ]


def build_parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(description=__doc__)
    value.add_argument("--index", type=Path, default=DEFAULT_INDEX)
    value.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    value.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    value.add_argument("--contract-directory", type=Path, default=DEFAULT_CONTRACT_DIRECTORY)
    value.add_argument("--roadmap", type=Path, default=DEFAULT_ROADMAP)
    value.add_argument(
        "--implementation-plan",
        type=Path,
        default=DEFAULT_IMPLEMENTATION_PLAN,
        help="implementation plan to outline (used by the plan command)",
    )
    value.add_argument(
        "--test-root",
        type=Path,
        default=DEFAULT_TEST_ROOT,
        help="C++ Catch2 source root used for derived test locations",
    )
    value.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    value.add_argument("--verbose", action="store_true", help="include per-test next actions")
    value.add_argument(
        "--compact",
        action="store_true",
        help="omit contract provenance while retaining requirement and clause mappings",
    )
    value.add_argument(
        "--summary",
        action="store_true",
        help="emit bounded aggregates without per-record detail",
    )
    subcommands = value.add_subparsers(dest="command", required=True)

    def add_output_flags(command: argparse.ArgumentParser) -> None:
        # Suppress the subparser defaults so flags work both before and after
        # the subcommand without a post-parse argv rewrite.
        command.add_argument(
            "--json",
            action="store_true",
            default=argparse.SUPPRESS,
            help="emit machine-readable JSON",
        )
        command.add_argument(
            "--verbose",
            action="store_true",
            default=argparse.SUPPRESS,
            help="include per-test next actions",
        )
        command.add_argument(
            "--compact",
            action="store_true",
            default=argparse.SUPPRESS,
            help="omit contract provenance while retaining requirement and clause mappings",
        )
        command.add_argument(
            "--summary",
            action="store_true",
            default=argparse.SUPPRESS,
            help="emit bounded aggregates without per-record detail",
        )
        command.add_argument(
            "--test-root",
            type=Path,
            default=argparse.SUPPRESS,
            help="C++ Catch2 source root used for derived test locations",
        )

    dashboard = subcommands.add_parser(
        "dashboard",
        help=(
            "show one bounded roadmap/test resume card with live mapping counts "
            "and the next focused-work handles"
        ),
    )
    dashboard.add_argument(
        "--limit",
        type=int,
        default=3,
        help="maximum open-family queue rows to include (default: 3; use 0 for all)",
    )
    add_output_flags(dashboard)
    resume = subcommands.add_parser(
        "resume",
        help=(
            "show the smallest bounded work card: current counts, one next "
            "slice or family choice, and its mapping/execution handles"
        ),
    )
    resume.add_argument(
        "--limit",
        type=int,
        default=1,
        help=(
            "maximum family/queue choices to retain (default: 1; the card "
            "always keeps the recommended choice)"
        ),
    )
    add_output_flags(resume)
    lab_issues = subcommands.add_parser(
        "lab-issues",
        help=(
            "show the bounded Requirements Lab extraction/usability issue ledger "
            "without reopening the Lab export"
        ),
    )
    lab_issues.add_argument(
        "query",
        nargs="?",
        help="optional issue id, status, clause, requirement id, or text filter",
    )
    lab_issues.add_argument(
        "--status",
        choices=("all", "open", "closed", "deferred"),
        default="all",
        help="limit issues by ledger status (default: all)",
    )
    lab_issues.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum issues to print (default: 20; use 0 for all)",
    )
    add_output_flags(lab_issues)
    status = subcommands.add_parser("status", help="show open roadmap items and checklist counts")
    add_output_flags(status)
    queue = subcommands.add_parser(
        "queue",
        help=(
            "show a bounded queue of open roadmap families and exact lane states"
        ),
    )
    queue.add_argument(
        "--limit",
        type=int,
        default=8,
        help="maximum open roadmap families to print (default: 8; use 0 for all)",
    )
    add_output_flags(queue)
    next_item = subcommands.add_parser(
        "next",
        help=(
            "show the next bounded ready handoff; use --pointer for the "
            "historical roadmap pointer"
        ),
    )
    next_item.add_argument(
        "--all",
        action="store_true",
        help="show every open indexed item instead of only the highest-priority item",
    )
    next_item.add_argument(
        "--pointer",
        action="store_true",
        help="show only the bounded source-pointer fields for the selected item",
    )
    next_item.add_argument(
        "--family",
        help=(
            "resolve one exact indexed roadmap family instead of following the "
            "highest-priority active pointer"
        ),
    )
    next_item.add_argument(
        "--include-source-only",
        action="store_true",
        help=(
            "opt in to the global source-only reconciliation queue when indexed "
            "roadmap queues are exhausted"
        ),
    )
    add_output_flags(next_item)
    roadmap = subcommands.add_parser(
        "roadmap",
        help=(
            "search indexed roadmap families and show live test/mapping counts "
            "with exact work, lane, and matrix handles"
        ),
    )
    roadmap.add_argument(
        "query",
        nargs="?",
        help=(
            "case-insensitive family id, title, tag, anchor, next-action, "
            "Requirements-Lab id, canonical 2025 subsection, or official C++ "
            "API substring; omit to list open families"
        ),
    )
    roadmap.add_argument(
        "--status",
        choices=("open", "complete", "blocked", "all"),
        default="open",
        help="limit families by indexed status (default: open)",
    )
    roadmap.add_argument(
        "--limit",
        type=int,
        default=12,
        help="maximum family rows to print (default: 12; use 0 for all)",
    )
    add_output_flags(roadmap)
    ready = subcommands.add_parser(
        "ready",
        help=(
            "show exactly one next planned or indexed new-case C++ slice with "
            "its mapping handles; use --include-source-only for source reconciliation"
        ),
    )
    ready.add_argument(
        "--family",
        help=(
            "resolve one exact indexed roadmap family instead of following the "
            "highest-priority active pointer"
        ),
    )
    ready.add_argument(
        "--include-source-only",
        action="store_true",
        help=(
            "opt in to the global source-only reconciliation queue when indexed "
            "roadmap queues are exhausted"
        ),
    )
    add_output_flags(ready)
    focus = subcommands.add_parser(
        "focus",
        help=(
            "show one exact lane's executable candidates, mapping counts, "
            "and copyable execution handles"
        ),
    )
    focus.add_argument(
        "lane",
        nargs="?",
        help=(
            "exact Catch2 lane tag; omit to use the active indexed work lane"
        ),
    )
    focus.add_argument(
        "--limit",
        type=int,
        default=8,
        help="maximum candidate tests to print (default: 8; use 0 for all)",
    )
    add_output_flags(focus)
    work = subcommands.add_parser(
        "work",
        help=(
            "resolve one actionable implementation slice, its exact baseline "
            "test, lane, and 2025 mappings"
        ),
    )
    work.add_argument(
        "id",
        nargs="?",
        help=(
            "indexed roadmap/work item id; omit to follow the highest-priority "
            "open item's next_work_id"
        ),
    )
    add_output_flags(work)
    recent = subcommands.add_parser(
        "recent",
        help="show the bounded ledger of recently completed slices",
    )
    recent.add_argument(
        "--limit",
        type=int,
        default=10,
        help="maximum completed slices to print (default: 10; use 0 for all)",
    )
    recent.add_argument(
        "--lane",
        help="show only indexed slices carrying this exact Catch2 lane tag",
    )
    add_output_flags(recent)
    item = subcommands.add_parser(
        "item",
        help="show one indexed roadmap item and its tagged C++ cases",
    )
    item.add_argument("id")
    item.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(item)
    plan_outline = subcommands.add_parser(
        "plan",
        help=(
            "show the implementation-plan section outline or filter its "
            "heading index"
        ),
    )
    plan_outline.add_argument(
        "query",
        nargs="?",
        help=(
            "case-insensitive heading/breadcrumb substring or exact plan line; "
            "omit to show the bounded outline"
        ),
    )
    plan_outline.add_argument(
        "--limit",
        type=int,
        default=20,
        help=(
            "maximum matching headings to print (default: 20; use 0 for all)"
        ),
    )
    add_output_flags(plan_outline)
    coverage = subcommands.add_parser("coverage", help="summarize C++ test and clause mapping coverage")
    coverage.add_argument(
        "--lane",
        help="limit coverage to one exact Catch2 lane tag",
    )
    coverage.add_argument(
        "--family",
        help="limit coverage to one indexed roadmap family id",
    )
    add_output_flags(coverage)
    gaps = subcommands.add_parser(
        "gaps",
        help=(
            "list pinned 2025 requirements without a mapped C++ Catch2 case, "
            "grouped by canonical document:clause subsection"
        ),
    )
    gaps.add_argument(
        "query",
        nargs="?",
        help=(
            "optional case-insensitive requirement id, title, statement, "
            "document, clause, or canonical document:clause filter"
        ),
    )
    gaps.add_argument(
        "--document",
        help="limit gaps to a 2025 document id such as hla-1516.1-2025",
    )
    gaps.add_argument(
        "--clause",
        help="limit gaps to a clause or subsection such as clause-7.2",
    )
    gaps.add_argument(
        "--coverage-kind",
        choices=("cross-cutting", "document-wide", "transition"),
        help="limit gaps to one pinned corpus coverage kind",
    )
    gaps.add_argument(
        "--family",
        help=(
            "limit coverage to requirements selected by one exact indexed "
            "roadmap family; use this to choose a family-local new case"
        ),
    )
    gaps.add_argument(
        "--limit",
        type=int,
        default=20,
        help=(
            "maximum uncovered requirement records to print (default: 20; "
            "use 0 for all records; subsection groups stay capped at 20)"
        ),
    )
    add_output_flags(gaps)
    contract_drift = subcommands.add_parser(
        "contract-drift",
        help=(
            "check local contract-to-Catch2 selectors against current C++ "
            "declarations without reopening the Requirements Lab"
        ),
    )
    contract_drift.add_argument(
        "query",
        nargs="?",
        help="optional contract-path substring filter",
    )
    contract_drift.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum drift findings to print (default: 20; use 0 for all)",
    )
    add_output_flags(contract_drift)
    check = subcommands.add_parser(
        "check",
        help="validate roadmap anchors and requirement references",
    )
    check.add_argument(
        "--lane",
        help=(
            "limit live test/source validation to one exact Catch2 lane; add "
            "--historical to include that lane's completion-ledger rows"
        ),
    )
    check.add_argument(
        "--family",
        help=(
            "limit live test/source validation to one indexed roadmap family; "
            "add --historical to include that family's completion-ledger rows"
        ),
    )
    check.add_argument(
        "--historical",
        action="store_true",
        help=(
            "also validate the append-only recent-completion ledger; by "
            "default check gates the live roadmap, plan, and source index"
        ),
    )
    add_output_flags(check)
    lane = subcommands.add_parser("lane", help="show tests carrying one exact Catch2 tag")
    lane.add_argument("tag")
    lane.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(lane)
    test = subcommands.add_parser("test", help="find tests by id or case-name substring")
    test.add_argument("query")
    test.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(test)
    case = subcommands.add_parser(
        "case",
        help=(
            "show one exact Catch2 case with source, direct requirement-to-2025 "
            "subsection pairs, owner, and execution handles"
        ),
    )
    case.add_argument(
        "query",
        help="exact Catch2 plan id or TEST_CASE title (use test/search for discovery)",
    )
    add_output_flags(case)
    source = subcommands.add_parser(
        "source",
        help="show planned Catch2 cases under one C++ source-path substring",
    )
    source.add_argument(
        "path",
        help="case-insensitive source-path substring (use repository-relative slashes)",
    )
    source.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(source)
    requirement = subcommands.add_parser(
        "requirement",
        help=(
            "find mapped tests by Lab id, contract id, clause, or document; "
            "if none is mapped, show the bounded 2025 requirement gap"
        ),
    )
    requirement.add_argument("query")
    requirement.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(requirement)
    section = subcommands.add_parser(
        "section",
        help="find tests mapped to one exact 2025 standard clause/subsection",
    )
    section.add_argument("query")
    section.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(section)
    trace = subcommands.add_parser(
        "trace",
        help=(
            "show a bounded direct test-to-Requirements-Lab-to-2025-section "
            "mapping for one exact handle"
        ),
    )
    trace.add_argument(
        "query",
        help=(
            "exact Catch2 plan id/title, Lab requirement id, canonical "
            "2025 document:clause key, C++ API surface id or method shorthand, "
            "or exact Catch2 lane tag"
        ),
    )
    trace.add_argument(
        "--limit",
        type=int,
        default=5,
        help="maximum test mappings to print (default: 5; use 0 for all)",
    )
    add_output_flags(trace)
    matrix = subcommands.add_parser(
        "matrix",
        help=(
            "show a bounded test-to-Requirements-Lab-to-2025-subsection "
            "mapping matrix for one exact handle"
        ),
    )
    matrix.add_argument(
        "query",
        nargs="?",
        help=(
            "exact Catch2 title/id, Lab requirement id, canonical 2025 "
            "document:clause key, C++ API surface id or method shorthand, exact Catch2 lane tag, "
            "or indexed roadmap family id; omit to use the active indexed lane"
        ),
    )
    matrix.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum matrix rows to print (default: 20; use 0 for all)",
    )
    add_output_flags(matrix)
    search = subcommands.add_parser(
        "search",
        help="find tests by any id, tag, API surface, requirement, or standard section",
    )
    search.add_argument("query")
    search.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(search)
    unplanned = subcommands.add_parser(
        "unplanned",
        help="show C++ TEST_CASE declarations that have no exact Catch2 plan row",
    )
    unplanned.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum source cases to print (default: 20; use 0 for all)",
    )
    unplanned.add_argument(
        "--path",
        help="limit the queue to source paths containing this case-insensitive substring",
    )
    add_output_flags(unplanned)
    unmapped = subcommands.add_parser(
        "unmapped",
        help="show Catch2 cases that do not yet select a Requirements-Lab requirement",
    )
    unmapped.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    unmapped.add_argument(
        "--lane",
        help="limit the queue to one exact Catch2 lane tag",
    )
    unmapped.add_argument(
        "--family",
        help="limit the queue to one indexed roadmap family id",
    )
    unmapped.add_argument(
        "--disposition",
        choices=("all", "explicit", "unclassified"),
        default="all",
        help=(
            "limit unmapped rows to an explicit no-standalone-surface "
            "disposition or rows still lacking one (default: all)"
        ),
    )
    unmapped.add_argument(
        "--show-contract-candidates",
        action="store_true",
        help=(
            "for each selected unmapped case, show exact local Requirements-Lab "
            "contract references that already name its C++ test"
        ),
    )
    add_output_flags(unmapped)
    unlocated = subcommands.add_parser(
        "unlocated",
        help="show plan cases whose TEST_CASE declaration is absent from the current C++ source",
    )
    unlocated.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    unlocated.add_argument(
        "--lane",
        help="limit the queue to one exact Catch2 lane tag",
    )
    unlocated.add_argument(
        "--family",
        help="limit the queue to one indexed roadmap family id",
    )
    add_output_flags(unlocated)
    lanes = subcommands.add_parser(
        "lanes",
        help="list exact Catch2 tags with their case counts for lane discovery",
    )
    lanes.add_argument(
        "family_query",
        nargs="?",
        help=(
            "exact indexed roadmap family id (shorthand for --family); use the "
            "flag when a script needs an explicit named option"
        ),
    )
    lanes.add_argument(
        "--limit",
        type=int,
        default=None,
        help="maximum lane rows to print (default: 40 with --compact/--summary; 0 for all)",
    )
    lanes.add_argument(
        "--family",
        help=(
            "limit discovery to one exact indexed roadmap family; this keeps "
            "overlapping tags scoped to one work owner"
        ),
    )
    lanes.add_argument(
        "--unmapped",
        action="store_true",
        help=(
            "show only lanes containing plan rows without a Lab requirement "
            "mapping (explicit dispositions are retained for review)"
        ),
    )
    lanes.add_argument(
        "--disposition",
        choices=("all", "explicit", "unclassified"),
        default="all",
        help=(
            "when selecting unmapped lanes, keep all missing rows (all), only "
            "intentional no-standalone-surface dispositions (explicit), or "
            "rows that still need a mapping decision (unclassified)"
        ),
    )
    add_output_flags(lanes)
    return value


def main() -> int:
    arguments = build_parser().parse_args()
    if arguments.command == "lab-issues":
        try:
            result = lab_issues_snapshot(
                query=arguments.query,
                status=arguments.status,
                limit=arguments.limit,
            )
        except (ValueError, OSError) as error:
            print(f"query_rti_work: {error}", file=sys.stderr)
            return 2

        if arguments.summary or arguments.compact:
            result["issues"] = [
                {
                    key: issue.get(key)
                    for key in (
                        "id",
                        "status",
                        "kind",
                        "title",
                        "affected_requirement_ids",
                        "exported_clause",
                        "normative_clause",
                        "document_id",
                        "source",
                    )
                    if key in issue
                }
                for issue in result["issues"]
            ]
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0

        shown_count = result["shown_count"]
        count = result["count"]
        suffix = "" if shown_count == count else f"; showing {shown_count}"
        query_text = f" for {arguments.query!r}" if arguments.query else ""
        print(f"Requirements Lab issue ledger{query_text}: {count} issue(s){suffix}.")
        if result.get("export_policy"):
            print(f"Policy: {result['export_policy']}")
        for issue in result["issues"]:
            print(
                f"- {issue.get('id', '<unknown>')} [{issue.get('status', 'unknown')}]: "
                f"{issue.get('title', '<untitled>')}"
            )
            affected = issue.get("affected_requirement_ids")
            if affected:
                print(f"  affected={','.join(strings(affected))}")
            if issue.get("exported_clause") or issue.get("normative_clause"):
                print(
                    "  clause="
                    f"exported {issue.get('exported_clause', '?')} / "
                    f"normative {issue.get('normative_clause', '?')}"
                )
            if not (arguments.summary or arguments.compact):
                if issue.get("symptom"):
                    print(f"  symptom={compact_prose(issue['symptom'], 320)}")
                if issue.get("workaround"):
                    print(f"  workaround={compact_prose(issue['workaround'], 320)}")
                if issue.get("reproduction"):
                    print(f"  reproduce={issue['reproduction']}")
        return 0
    try:
        index = load_json(arguments.index)
        plan = load_json(arguments.plan)
        bundle = load_json(arguments.bundle)
        standard_requirements = load_standard_requirements(bundle)
        contract_links = load_contract_links(arguments.contract_directory)
        # Keep the normal roadmap/test queries cheap and bounded.  The reverse
        # contract index is built only for the explicit discovery view; it is
        # never an implicit Requirements-Lab rescan for unrelated commands.
        contract_test_candidates = (
            load_contract_test_candidates(arguments.contract_directory)
            if getattr(arguments, "show_contract_candidates", False)
            else {}
        )
        source_locations, source_health = load_test_source_locations(
            arguments.test_root,
            include_health=True,
        )
        tests = all_mapped_tests(
            plan,
            standard_requirements,
            contract_links,
            source_locations,
        )
        roadmap_entries = roadmap_checklist(arguments.roadmap)
    except ValueError as error:
        print(f"query_rti_work: {error}", file=sys.stderr)
        return 2
    except OSError as error:
        print(f"query_rti_work: {error}", file=sys.stderr)
        return 2

    if arguments.command == "contract-drift":
        result = contract_drift_report(
            arguments.contract_directory,
            source_locations,
            query=arguments.query,
            limit=arguments.limit,
        )
        if arguments.json:
            payload = dict(result)
            if arguments.summary:
                payload["findings"] = []
                payload["shown_finding_count"] = 0
            print(json.dumps(payload, indent=2, sort_keys=True))
            return 0 if result["status"] == "clean" else 1
        print(
            "Requirements-Lab contract selectors: "
            f"{result['status']}; "
            f"{result['native_reference_count']} native references, "
            f"{result['clean_reference_count']} exact, "
            f"{result['finding_count']} findings; "
            f"{result['external_reference_count']} external symbolic references ignored."
        )
        for finding in result["findings"]:
            owner = finding.get("owner_id") or "<contract>"
            reference = finding.get("test_reference") or "<invalid>"
            detail = ""
            locations = finding.get("current_source_locations")
            if isinstance(locations, list) and locations:
                detail = " current=" + ", ".join(
                    f"{location.get('path')}:{location.get('line')}"
                    for location in locations
                    if isinstance(location, dict)
                )
            print(
                f"- {finding.get('kind', 'unknown')}: "
                f"{finding.get('contract')}:{owner}: {reference}{detail}"
            )
        if result["shown_finding_count"] < result["finding_count"]:
            print(
                f"- ... {result['finding_count'] - result['shown_finding_count']} more; "
                "use --limit 0 or --json for the complete bounded report"
            )
        return 0 if result["status"] == "clean" else 1

    if arguments.command == "resume":
        if arguments.limit < 0:
            message = "resume --limit must be non-negative"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        result = resume_snapshot(
            index,
            tests,
            roadmap_entries,
            source_locations,
            source_health,
            limit=arguments.limit,
            standard_requirements=standard_requirements,
        )
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0

        roadmap_counts = result.get("roadmap_counts", {})
        plan_counts = result.get("plan_counts", {})
        print(
            "RTI work resume: "
            f"{roadmap_counts.get('complete', 0)} roadmap items complete, "
            f"{roadmap_counts.get('open', 0)} open; "
            f"{plan_counts.get('catch2_plan_cases', 0)} Catch2 cases, "
            f"{plan_counts.get('mapped_cases', 0)} mapped, "
            f"{plan_counts.get('unmapped_cases', 0)} unmapped."
        )
        print(
            "Mapping health: "
            f"{plan_counts.get('source_unlocated_cases', 0)} source-unlocated "
            f"(actionable={plan_counts.get('source_unlocated_actionable_cases', 0)}); "
            f"index={'current' if result.get('index_snapshot', {}).get('matches_live') else 'stale'}"
        )
        next_card = result.get("next")
        if isinstance(next_card, dict) and next_card.get("test_case"):
            print(
                "Next slice: "
                f"{next_card.get('state', '?')} "
                f"family={next_card.get('family_id', '?')} "
                f"lane={next_card.get('source_lane') or '<none>'}"
            )
            print(
                f"  case={compact_prose(next_card.get('test_case'), 220)} "
                f"plan_id={next_card.get('plan_id') or '<none>'} "
                f"source={next_card.get('source_location') or next_card.get('source_target') or '<unlocated>'}"
            )
            if next_card.get("ctest_target"):
                print(f"  ctest_target={next_card['ctest_target']}")
            print(
                "  mapping="
                f"{next_card.get('requirement_count', 0)} requirements; "
                f"{next_card.get('standard_section_count', 0)} sections; "
                f"{next_card.get('requirement_section_pair_count', 0)} direct pairs; "
                f"{next_card.get('cpp_api_surface_count', 0)} APIs"
            )

            def resume_preview(values: Any, remaining_key: str) -> str:
                preview = strings(values)
                if not preview:
                    return "<none>"
                remaining = next_card.get(remaining_key, 0)
                suffix = f", ... (+{remaining})" if remaining else ""
                return ", ".join(preview) + suffix

            if next_card.get("requirement_ids"):
                print(
                    "  requirements="
                    + resume_preview(next_card.get("requirement_ids"), "requirement_ids_remaining")
                )
            if next_card.get("standard_sections"):
                print(
                    "  standard_sections="
                    + resume_preview(next_card.get("standard_sections"), "standard_sections_remaining")
                )
            pairs = next_card.get("requirement_section_pairs")
            if isinstance(pairs, list) and pairs:
                pair_text = ", ".join(
                    f"{pair.get('lab_requirement_id')} -> {pair.get('standard_section') or '<unresolved>'}"
                    for pair in pairs
                    if isinstance(pair, dict)
                )
                remaining = next_card.get("requirement_section_pairs_remaining", 0)
                if remaining:
                    pair_text += f", ... (+{remaining})"
                if pair_text:
                    print(f"  direct_pairs={pair_text}")
            if next_card.get("next_action"):
                print(f"  next={compact_prose(next_card.get('next_action'), 260)}")
            if next_card.get("mapping_seed_plan_id"):
                print(
                    "  mapping_seed="
                    f"{next_card.get('mapping_seed_plan_id')} "
                    f"status={next_card.get('mapping_status', '<unspecified>')}"
                )
            if next_card.get("seed_trace_command"):
                print(f"  seed_trace={next_card['seed_trace_command']}")
        elif isinstance(next_card, dict):
            print(
                "Next: "
                f"{next_card.get('state', 'none')}; "
                f"{next_card.get('reason', 'choose one bounded family action')}"
            )
            options = next_card.get("family_options")
            if isinstance(options, list) and options:
                option = options[0]
                if isinstance(option, dict):
                    print(
                        "  recommended_family="
                        f"{option.get('id', '<unnamed>')} "
                        f"action={option.get('action_state', '<unspecified>')} "
                        f"cases={option.get('family_case_count', option.get('case_count', 0))} "
                        f"mapped={option.get('family_mapped_case_count', option.get('mapped_case_count', 0))} "
                        f"requirements={option.get('requirement_count', 0)} "
                        f"sections={option.get('standard_section_count', 0)}"
                    )
                    for name in (
                        "work_query",
                        "lane_discovery_command",
                        "mapping_lane_discovery_command",
                        "gap_query",
                    ):
                        handle = option.get(name)
                        if isinstance(handle, str) and handle:
                            print(f"  {name.removesuffix('_command')}={handle}")
                    for line in text_gap_preview(option):
                        print(line)
                remaining = next_card.get("family_options_remaining", 0)
                if remaining:
                    print(f"  ... {remaining} more family choice(s); use dashboard for the preview")
        else:
            print("Next: no bounded indexed handoff")

        deferred = result.get("deferred_slices")
        if isinstance(deferred, list) and deferred:
            first_deferred = deferred[0]
            if isinstance(first_deferred, dict):
                print(
                    "Deferred seam: "
                    f"{first_deferred.get('id', '<unnamed>')} "
                    f"[{first_deferred.get('state', '<unspecified>')}] "
                    f"lane={first_deferred.get('lane', '<none>')}"
                )
                if first_deferred.get("next_step"):
                    print(
                        "  next_step="
                        + compact_prose(first_deferred.get("next_step"), 300)
                    )

        latest = result.get("latest_completed_slice")
        if isinstance(latest, dict):
            print(
                "Latest: "
                f"{latest.get('lane', '<unnamed>')} - "
                f"{compact_prose(latest.get('test_case'), 180)} "
                f"({latest.get('assertions', '?')} assertions; "
                f"{latest.get('requirements', '?')} requirements; "
                f"{latest.get('standard_sections', '?')} sections)"
            )
        handles = result.get("handles")
        if isinstance(handles, dict):
            selected_handles = [
                (name, handles.get(name))
                for name in (
                    "work",
                    "trace",
                    "matrix",
                    "focus",
                    "lane_check",
                    "post_mapping_focus",
                    "post_mapping_check",
                    "ready",
                    "check",
                )
                if isinstance(handles.get(name), str) and handles.get(name)
            ]
            if selected_handles:
                print("Handles:")
                for name, handle in selected_handles:
                    print(f"  {name}={handle}")
        return 0

    if arguments.command == "dashboard":
        result = dashboard_snapshot(
            index,
            tests,
            roadmap_entries,
            source_locations,
            source_health,
            limit=arguments.limit,
            standard_requirements=standard_requirements,
        )
        if arguments.json:
            if arguments.summary:
                # Summary mode is the work-selection card: subsection samples
                # are enough to choose the next exact requirement query, and
                # the full requirement records stay opt-in.
                result["requirements"] = []
                result["shown_requirement_count"] = 0
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0

        roadmap_counts = result.get("roadmap_counts", {})
        plan_counts = result.get("plan_counts", {})
        print(
            "Roadmap dashboard: "
            f"{roadmap_counts.get('complete', 0)} complete, "
            f"{roadmap_counts.get('open', 0)} open, "
            f"{roadmap_counts.get('blocked', 0)} blocked"
        )
        print(
            "Plan/mapping: "
            f"{plan_counts.get('catch2_plan_cases', 0)} Catch2 cases; "
            f"{plan_counts.get('mapped_cases', 0)} mapped; "
            f"{plan_counts.get('unmapped_cases', 0)} unmapped "
            f"({plan_counts.get('explicit_unmapped_dispositions', 0)} explicit, "
            f"{plan_counts.get('unclassified_unmapped_cases', 0)} unclassified); "
            f"{plan_counts.get('source_unlocated_cases', 0)} source-unlocated "
            f"(actionable={plan_counts.get('source_unlocated_actionable_cases', plan_counts.get('source_unlocated_cases', 0))}); "
            f"{plan_counts.get('source_only_cases', 0)} source-only"
        )
        health = result.get("source_health", {})
        if isinstance(health, dict):
            health_suffix = ""
            if health.get("files_with_unbalanced_conditionals"):
                health_suffix += (
                    "; "
                    f"{health.get('files_with_unbalanced_conditionals')} file(s) "
                    "with unbalanced preprocessor conditionals"
                )
            print(
                "Source index: "
                f"{health.get('status', 'unknown')} across "
                f"{health.get('files_scanned', 0)} C++ files{health_suffix}"
            )
            if health.get("status") == "attention" and health.get("hint"):
                print(f"Source hint: {health['hint']}")
        snapshot = result.get("index_snapshot", {})
        if isinstance(snapshot, dict):
            print(
                "Traceability index snapshot: "
                f"{'current' if snapshot.get('matches_live') else 'stale'}"
                + (
                    ""
                    if snapshot.get("matches_live")
                    else " (live counts above are authoritative)"
                )
            )
        latest = result.get("latest_completed_slice")
        if isinstance(latest, dict):
            print(
                "Latest: "
                f"{latest.get('lane', '<unnamed>')} - "
                f"{compact_prose(latest.get('test_case'), 180)} "
                f"({latest.get('assertions', '?')} assertions; "
                f"{latest.get('requirements', '?')} requirements; "
                f"{latest.get('standard_sections', '?')} sections; "
                f"{latest.get('api_surfaces', '?')} APIs)"
            )
            for handle_name in (
                "case_command",
                "focus_command",
                "trace_command",
                "matrix_command",
                "check_command",
                "ctest_command",
            ):
                handle = latest.get(handle_name)
                if isinstance(handle, str) and handle:
                    print(f"   latest_{handle_name.removesuffix('_command')}={handle}")
        source_reconciliation = result.get("source_only_reconciliation")
        if isinstance(source_reconciliation, dict) and source_reconciliation.get(
            "count", 0
        ):
            print(
                "Source-only reconciliation head: "
                f"{compact_prose(source_reconciliation.get('test_query'), 180)} "
                f"source={source_reconciliation.get('source_location') or '<unlocated>'}"
            )
            print(
                "   opt_in="
                f"{source_reconciliation.get('ready_command')}"
            )
        next_card = result.get("next")
        if isinstance(next_card, dict) and next_card.get("work_id"):
            if next_card.get("test_case"):
                print(
                    "Next: "
                    f"{next_card.get('state', '?')} "
                    f"family={next_card.get('family_id', '?')} "
                    f"work={next_card.get('work_id', '?')} "
                    f"status={next_card.get('work_status', '?')}"
                )
                print(
                    "   test: "
                    f"{compact_prose(next_card.get('test_case'), 180)} "
                    f"plan_id={next_card.get('plan_id', '<none>')} "
                    f"source={next_card.get('source_location') or next_card.get('source_target') or '<unlocated>'}"
                )
                print(
                    "   mapping: "
                    f"{next_card.get('assertion_count') or '<unrecorded>'} assertions; "
                    f"{next_card.get('requirement_count', 0)} requirements; "
                    f"{next_card.get('standard_section_count', 0)} sections; "
                    f"{next_card.get('cpp_api_surface_count', 0)} APIs; "
                    f"{next_card.get('requirement_section_pair_count', 0)} direct pairs"
                )
                requirement_ids = strings(next_card.get("requirement_ids"))
                standard_sections = strings(next_card.get("standard_sections"))
                if requirement_ids or standard_sections:
                    def preview(values: list[str]) -> str:
                        preview_limit = 4
                        shown = values[:preview_limit]
                        suffix = (
                            f", ... (+{len(values) - preview_limit})"
                            if len(values) > preview_limit
                            else ""
                        )
                        return ",".join(shown) + suffix if shown else "<none>"

                    print(
                        "   mapping_handles: "
                        f"requirements={preview(requirement_ids)}; "
                        f"sections={preview(standard_sections)}"
                    )
                if next_card.get("next_action"):
                    print(f"   next={next_card['next_action']}")
                if next_card.get("mapping_seed_plan_id"):
                    print(
                        "   mapping_seed: "
                        f"{next_card.get('mapping_seed_plan_id')} "
                        f"status={next_card.get('mapping_status', '<unspecified>')}"
                    )
                if next_card.get("seed_trace_command"):
                    print(f"   seed_trace={next_card['seed_trace_command']}")
            else:
                print(
                    "Active pointer: "
                    f"{next_card.get('work_id')} "
                    f"status={next_card.get('work_status', '?')} "
                    f"lane={next_card.get('lane', '<none>')} "
                    f"state={next_card.get('lane_state', '<unknown>')} "
                    f"cases={next_card.get('lane_case_count', '?')} "
                    f"mapped={next_card.get('lane_mapped_test_count', '?')} "
                    f"assertions={next_card.get('lane_assertion_count', '?')} "
                    f"source_drift={next_card.get('lane_source_drift_count', '?')} "
                    f"actionable_source_drift={next_card.get('lane_actionable_source_drift_count', '?')}"
                )
                baseline = next_card.get("baseline")
                if isinstance(baseline, dict):
                    print(
                        "   baseline: "
                        f"{compact_prose(baseline.get('test_case'), 180)} "
                        f"state={baseline.get('state', '?')} "
                        f"{baseline.get('assertion_count', '?')} assertions; "
                        f"{baseline.get('requirement_count', '?')} requirements; "
                        f"{baseline.get('standard_section_count', '?')} sections; "
                        f"{baseline.get('cpp_api_surface_count', '?')} APIs"
                    )
                if next_card.get("task"):
                    print(f"   task={next_card['task']}")
        elif isinstance(next_card, dict):
            family_options = next_card.get("family_options", [])
            if (
                next_card.get("state") == "none"
                and isinstance(family_options, list)
                and family_options
            ):
                print("Next: choose one bounded queued family action")
                print(
                    "   recommended: "
                    f"{family_options[0].get('id', '<unnamed>')}"
                    if isinstance(family_options[0], dict)
                    else "   recommended: <none>"
                )
            elif not (
                next_card.get("state") == "none"
                and next_card.get("evidence_complete_families")
            ):
                print(
                    f"Next: {next_card.get('state', 'unavailable')}: "
                    f"{next_card.get('reason', '')}"
                )
            if isinstance(family_options, list) and family_options:
                print("   bounded_family_options:")
                for option in family_options[:3]:
                    if not isinstance(option, dict):
                        continue
                    print(
                        f"      {option.get('id', '<unnamed>')}: "
                        f"state={option.get('state', '<unspecified>')} "
                        f"action={option.get('action_state', '<unspecified>')} "
                        f"cases={option.get('case_count', 0)} "
                        f"mapped={option.get('mapped_case_count', 0)} "
                        f"family_cases={option.get('family_case_count', option.get('case_count', 0))} "
                        f"family_mapped={option.get('family_mapped_case_count', option.get('mapped_case_count', 0))} "
                        f"requirements={option.get('requirement_count', 0)} "
                        f"sections={option.get('standard_section_count', 0)} "
                        f"direct_pairs={option.get('requirement_section_pair_count', 0)} "
                        f"work={option.get('work_query') or '<none>'}"
                    )
                    if option.get("lane_discovery_command"):
                        print(
                            "      lane_discovery="
                            f"{option['lane_discovery_command']}"
                        )
                    if option.get("mapping_lane_discovery_command"):
                        print(
                            "      mapping_lane_discovery="
                            f"{option['mapping_lane_discovery_command']}"
                        )
                    if option.get("gap_query"):
                        print(f"      gaps={option['gap_query']}")
                    for line in text_gap_preview(option, prefix="      "):
                        print(line)
            elif (
                isinstance(next_card, dict)
                and next_card.get("state") == "none"
                and next_card.get("evidence_complete_families")
            ):
                print(
                    "Next: no runnable indexed work; "
                    "the listed families are evidence-complete and waiting for "
                    "a deliberate new slice"
                )

        handle_values = result.get("handles", {})
        if isinstance(handle_values, dict):
            print("Handles:")
            for name in (
                "ready",
                "work",
                "focus",
                "trace",
                "matrix",
                "lane_check",
                "queue",
                "family_ready",
                "check",
                "lab_issues",
            ):
                handle = handle_values.get(name)
                if handle:
                    print(f"   {name}={handle}")

        queue = result.get("queue", {})
        if isinstance(queue, dict):
            rows = queue.get("items", [])
            print(
                "Open queue: "
                f"showing {len(rows)} of {queue.get('open_count', 0)} families"
            )
            action_counts = queue.get("action_counts")
            if isinstance(action_counts, dict) and action_counts:
                print(
                    "Queue actions: "
                    + ", ".join(
                        f"{name}={count}"
                        for name, count in sorted(action_counts.items())
                    )
                )
            for row in rows:
                if not isinstance(row, dict):
                    continue
                unlocated = unlocated_display(row)
                print(
                    f"   {row.get('priority', '?')}. {row.get('id')}: "
                    f"{row.get('state')} "
                    f"action={row.get('action_state', '<unspecified>')} "
                    f"cases={row.get('case_count', 0)} "
                    f"mapped={row.get('mapped_case_count', 0)} "
                    f"family_cases={row.get('family_case_count', row.get('case_count', 0))} "
                    f"family_mapped={row.get('family_mapped_case_count', row.get('mapped_case_count', 0))} "
                    f"requirements={row.get('requirement_count', 0)} "
                    f"sections={row.get('standard_section_count', 0)} "
                    f"direct_pairs={row.get('requirement_section_pair_count', 0)} "
                    f"candidates={row.get('candidate_count', 0)} "
                    f"source_drift={row.get('source_drift_count', 0)} "
                    f"actionable_source_drift={row.get('actionable_source_drift_count', 0)} "
                    f"unlocated={unlocated}"
                )
        return 0

    if arguments.command == "check":
        family_item, scoped_tests = scoped_plan_tests(
            index,
            tests,
            lane=arguments.lane,
            family=arguments.family,
        )
        errors = validate_index(
            index,
            arguments.roadmap,
            plan,
            standard_requirements,
            tests,
            source_locations=source_locations,
            focus_lane=arguments.lane,
            focus_family=arguments.family,
            include_historical=arguments.historical,
        )
        index_result = index_status(index, tests, roadmap_entries, source_locations)
        index_result["source_health"] = source_health
        warnings = index_warnings(index_result, arguments.lane)
        mapped_test_count = sum(bool(test.get("requirements")) for test in scoped_tests)
        explicit_disposition_count = sum(
            not test.get("requirements")
            and traceability_state(test) == "explicit-disposition"
            for test in scoped_tests
        )
        unclassified_count = sum(
            not test.get("requirements")
            and traceability_state(test) == "unclassified"
            for test in scoped_tests
        )
        requirement_section_rows = [
            row
            for test in scoped_tests
            for row in requirement_section_mapping_rows(test)
            if isinstance(row, dict)
        ]
        requirement_section_pairs = {
            (row.get("lab_requirement_id"), row.get("standard_section"))
            for row in requirement_section_rows
            if row.get("lab_requirement_id")
        }
        scoped_roadmap_item_count = sum(
            (
                arguments.lane is None
                and arguments.family is None
            )
            or (
                arguments.family is not None
                and isinstance(item, dict)
                and str(item.get("id") or "").casefold()
                == arguments.family.casefold()
            )
            or (
                arguments.lane is not None
                and item_matches_lane(item, arguments.lane)
            )
            for item in index.get("items", [])
            if isinstance(item, dict)
        )
        result = {
            "ok": not errors,
            "errors": errors,
            "warnings": warnings,
            "scope_lane": arguments.lane,
            "scope_family": arguments.family,
            "historical_validation": arguments.historical,
            "roadmap_owner": (
                roadmap_item_summary(family_item)
                if isinstance(family_item, dict)
                else None
            ),
            "roadmap_item_count": scoped_roadmap_item_count,
            "test_count": len(scoped_tests),
            "repository_test_count": len(tests),
            "mapped_test_count": mapped_test_count,
            "unmapped_test_count": len(scoped_tests) - mapped_test_count,
            "explicit_disposition_count": explicit_disposition_count,
            "unclassified_count": unclassified_count,
            "requirement_section_pair_count": len(requirement_section_pairs),
            "resolved_requirement_section_pair_count": sum(
                pair[1] is not None for pair in requirement_section_pairs
            ),
            "unresolved_requirement_section_mapping_count": sum(
                bool(row.get("unresolved")) for row in requirement_section_rows
            ),
            "unique_standard_clause_count": len(
                {
                    clause
                    for test in scoped_tests
                    for clause in test.get("standard_clauses", [])
                }
            ),
            "standard_requirement_count": len(standard_requirements),
            "tests_without_source_location": sum(
                not test.get("source_locations") for test in scoped_tests
            ),
            "planned_test_count": sum(is_planned_test(test) for test in scoped_tests),
            "actionable_tests_without_source_location": sum(
                not test.get("source_locations")
                and not is_planned_test(test)
                and not is_non_executable_source_status(test)
                for test in scoped_tests
            ),
            "unlocated_test_ids": [
                test.get("id")
                for test in scoped_tests
                if not test.get("source_locations") and not is_planned_test(test)
            ],
            "planned_test_ids": [
                test.get("id")
                for test in scoped_tests
                if is_planned_test(test)
            ],
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
        elif errors:
            if arguments.compact or arguments.summary:
                # Keep the normal integrity gate bounded even when the dirty
                # checkout contains a large source-drift queue.  The complete
                # diagnostic remains available through ``--json`` (or the
                # unqualified text form when a human needs every row).
                sample_limit = 12
                scope = (
                    f" for roadmap family {arguments.family!r}"
                    if arguments.family
                    else f" for lane {arguments.lane!r}" if arguments.lane else ""
                )
                print(
                    "Roadmap/test mapping check failed: "
                    f"{len(errors)} errors{scope}; {len(scoped_tests)} cases, "
                    f"{mapped_test_count} mapped, "
                    f"{len(scoped_tests) - mapped_test_count} without a requirement mapping, "
                    f"{result['tests_without_source_location']} without a C++ source location"
                    + (
                        f" (actionable={result['actionable_tests_without_source_location']})"
                        if result["actionable_tests_without_source_location"]
                        != result["tests_without_source_location"]
                        else ""
                    )
                    + "."
                )
                print("Sample errors:")
                print("\n".join(f"- {error}" for error in errors[:sample_limit]))
                if len(errors) > sample_limit:
                    print(
                        f"- ... {len(errors) - sample_limit} more; "
                        "use check --json for the complete diagnostic"
                    )
            else:
                print("Roadmap/test mapping check failed:")
                print("\n".join(f"- {error}" for error in errors))
        else:
            scope = (
                f" for roadmap family {arguments.family!r}"
                if arguments.family
                else f" for lane {arguments.lane!r}" if arguments.lane else ""
            )
            print(
                f"Roadmap/test mapping check passed{scope}: "
                f"{result['roadmap_item_count']} roadmap items, "
                f"{len(scoped_tests)} Catch2 cases ({mapped_test_count} mapped, "
                f"{len(scoped_tests) - mapped_test_count} without a requirement mapping; "
                f"{explicit_disposition_count} explicit, {unclassified_count} unclassified), "
                f"{result['unique_standard_clause_count']} standard clauses, "
                f"{result['resolved_requirement_section_pair_count']} direct requirement-section pairs, "
                f"{len(standard_requirements)} 2025 standard requirements; "
                f"{result['tests_without_source_location']} tests without a C++ source location"
                + (
                    f" (actionable={result['actionable_tests_without_source_location']})"
                    if result["actionable_tests_without_source_location"]
                    != result["tests_without_source_location"]
                    else ""
                )
                + "."
            )
            if result.get("planned_test_count"):
                print(
                    f"Planned Catch2 rows: {result['planned_test_count']} "
                    "(not source drift; ready will select them before implementation)."
                )
            if result["unlocated_test_ids"]:
                if arguments.compact or arguments.summary:
                    sample_limit = 12
                    print(
                        f"Unlocated Catch2 plan entries: {len(result['unlocated_test_ids'])}"
                    )
                    print(
                        "\n".join(
                            f"- {identifier}"
                            for identifier in result["unlocated_test_ids"][:sample_limit]
                        )
                    )
                    if len(result["unlocated_test_ids"]) > sample_limit:
                        print(
                            f"- ... {len(result['unlocated_test_ids']) - sample_limit} more; "
                            "use check --json for the complete list"
                        )
                else:
                    print("Unlocated Catch2 plan entries:")
                    print("\n".join(f"- {identifier}" for identifier in result["unlocated_test_ids"]))
            if warnings:
                print("Roadmap index warnings:")
                print("\n".join(f"- {warning}" for warning in warnings))
            if not arguments.historical:
                print(
                    "Historical completion ledger: skipped; use "
                    "check --historical for strict append-only audit."
                )
        return 0 if not errors else 1

    if arguments.command == "queue":
        result = indexed_work_queue(index, tests, arguments.limit)
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(
            "Indexed roadmap queue: "
            f"{result['open_count']} open families; "
            f"{result['executable_candidate_count']} executable candidates; "
            f"{result['source_drift_count']} source-drift rows; "
            f"{result.get('planned_case_count', 0)} planned rows; "
            f"{unlocated_display(result)} other source-unlocated rows."
        )
        action_counts = result.get("action_counts", {})
        if not isinstance(action_counts, dict):
            action_counts = {}
        if action_counts:
            print(
                "Actions: "
                + ", ".join(
                    f"{name}={count}"
                    for name, count in sorted(action_counts.items())
                )
            )
        if not result["items"]:
            print("No open indexed roadmap families.")
            return 0
        if result["shown_count"] < result["open_count"]:
            print(
                f"Showing {result['shown_count']}; use --limit 0 for all indexed families."
            )
        for row in result["items"]:
            assertion_suffix = (
                f"assertions={row.get('assertion_count')} "
                if isinstance(row.get("assertion_count"), int)
                else ""
            )
            unlocated = unlocated_display(row)
            print(
                f"{row.get('priority', '?')}. {row.get('id')}: {row.get('state')} "
                f"action={row.get('action_state', '<unspecified>')} "
                f"cases={row.get('case_count', 0)} "
                f"{assertion_suffix}"
                f"mapped={row.get('mapped_case_count', 0)} "
                f"candidates={row.get('candidate_count', 0)} "
                f"planned={row.get('planned_count', 0)} "
                f"source_drift={row.get('source_drift_count', 0)} "
                f"actionable_source_drift={row.get('actionable_source_drift_count', 0)} "
                f"unlocated={unlocated} "
                f"direct_pairs={row.get('resolved_requirement_section_pair_count', 0)}"
            )
            if row.get("unlocated_head"):
                print(f"   unlocated_head={compact_prose(row['unlocated_head'], 160)}")
                head_assertions = row.get("unlocated_head_assertions")
                head_assertion_text = (
                    str(head_assertions)
                    if isinstance(head_assertions, int)
                    else "<unrecorded>"
                )
                print(
                    "   unlocated_head_mapping: "
                    f"id={row.get('unlocated_head_id') or '<none>'} "
                    f"assertions={head_assertion_text} "
                    f"requirements={len(row.get('unlocated_head_requirement_ids', []))} "
                    f"standard_sections={len(row.get('unlocated_head_standard_sections', []))} "
                    f"api_surfaces={len(row.get('unlocated_head_api_surfaces', []))}"
                )
                if row.get("unlocated_head_trace_command"):
                    print(f"   unlocated_head_trace={row['unlocated_head_trace_command']}")
            if row.get("planned_head"):
                print(
                    f"   planned_head={compact_prose(row['planned_head'], 160)}"
                )
                print(
                    "   planned_head_mapping: "
                    f"id={row.get('planned_head_id') or '<none>'} "
                    f"assertions={row.get('planned_head_assertions', '<unrecorded>')} "
                    f"requirements={len(row.get('planned_head_requirement_ids', []))} "
                    f"standard_sections={len(row.get('planned_head_standard_sections', []))} "
                    f"api_surfaces={len(row.get('planned_head_api_surfaces', []))}"
                )
                if row.get("planned_head_trace_command"):
                    print(f"   planned_head_trace={row['planned_head_trace_command']}")
            if row.get("next_work_id"):
                print(
                    f"   work={row['next_work_id']}"
                    + (
                        f" status={row['next_work_status']}"
                        if row.get("next_work_status")
                        else ""
                    )
                )
            if row.get("next_lane"):
                print(
                    "   focus_query="
                    "python tools/query_rti_work.py focus "
                    f"{row['next_lane']} --summary --compact"
                )
            next_test = row.get("next_test_pointer")
            if isinstance(next_test, dict) and next_test.get("query"):
                print(
                    "   next_test="
                    f"{compact_prose(next_test['query'], 160)} "
                    f"(state={next_test.get('state', '<unspecified>')}; "
                    f"assertions={next_test.get('assertion_count', 0)}; "
                    f"requirements={next_test.get('requirement_count', 0)}; "
                    f"standard_sections={next_test.get('standard_section_count', 0)}; "
                    f"api_surfaces={next_test.get('cpp_api_surface_count', 0)})"
                )
            print(
                "   work_query="
                f"python tools/query_rti_work.py work {row.get('id')} --summary --compact"
            )
            if row.get("next_action"):
                next_action = (
                    compact_prose(row["next_action"], 180)
                    if arguments.compact or arguments.summary
                    else row["next_action"]
                )
                print(f"   next={next_action}")
        return 0

    if arguments.command == "status":
        result = index_status(index, tests, roadmap_entries, source_locations)
        result["source_health"] = source_health
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(
            f"Roadmap checklist: {result['roadmap_counts'].get('complete', 0)} complete, "
            f"{result['roadmap_counts'].get('open', 0)} open"
        )
        print(f"C++ Catch2 plan: {result['test_count']} cases")
        mapping_counts = result.get("derived_mapping_counts", {})
        if isinstance(mapping_counts, dict):
            print(
                "Traceability queues: "
                f"{mapping_counts.get('catch2_cases_without_lab_requirement_mapping', '?')} unmapped, "
                f"{mapping_counts.get('catch2_cases_without_cpp_source_location', '?')} source-unlocated, "
                f"{mapping_counts.get('cpp_source_cases_without_plan_row', '?')} source-only"
            )
            print(
                "Unmapped disposition: "
                f"{mapping_counts.get('catch2_unmapped_explicit_disposition', 0)} explicit, "
                f"{mapping_counts.get('catch2_unmapped_unclassified', 0)} unclassified"
            )
        health = result.get("source_health")
        if isinstance(health, dict):
            print(
                "Source index: "
                f"{health.get('status', 'unknown')} across "
                f"{health.get('files_scanned', 0)} C++ files; "
                f"{health.get('files_with_control_characters', 0)} with control-byte damage."
            )
            if health.get("status") == "attention":
                print(f"Source hint: {health.get('hint', '')}")
        latest = result.get("latest_completed_slice")
        if isinstance(latest, dict):
            print(
                "Latest completed slice: "
                f"{latest.get('lane', '<unnamed>')} - "
                f"{latest.get('test_case', '<unnamed>')} "
                f"({latest.get('assertions', '?')} assertions; "
                f"{latest.get('requirements', '?')} requirements; "
                f"{latest.get('standard_sections', '?')} sections; "
                f"{latest.get('api_surfaces', '?')} APIs)"
            )
            print(
                "   latest_source="
                f"{latest.get('source_location', '<unlocated>')} "
                "latest_trace=python tools/query_rti_work.py trace "
                f"\"{latest.get('test_case', '')}\" --summary --compact"
            )
            latest_lane = latest.get("lane")
            latest_focus_lane = (
                latest.get("focus_lane")
                or latest.get("primary_lane")
                or latest_lane
            )
            latest_handles = (
                index.get("mapping", {}).get("lane_handles", {}).get(latest_focus_lane)
                if isinstance(latest_focus_lane, str)
                and isinstance(index.get("mapping"), dict)
                and isinstance(index.get("mapping", {}).get("lane_handles"), dict)
                else None
            )
            if not isinstance(latest_handles, dict) or not latest_handles:
                latest_handles = (
                    index.get("mapping", {}).get("lane_handles", {}).get(latest_lane)
                    if isinstance(latest_lane, str)
                    and isinstance(index.get("mapping"), dict)
                    and isinstance(index.get("mapping", {}).get("lane_handles"), dict)
                    else None
                )
            latest_commands = {
                "case": (
                    "python tools/query_rti_work.py case "
                    f"{json.dumps(str(latest.get('plan_id')), ensure_ascii=False)} "
                    "--summary --compact"
                    if isinstance(latest.get("plan_id"), str) and latest.get("plan_id")
                    else None
                ),
                "focus": (
                    f"python tools/query_rti_work.py focus {latest_focus_lane} --summary --compact"
                    if isinstance(latest_focus_lane, str) and latest_focus_lane
                    else None
                ),
                "matrix": (
                    f"python tools/query_rti_work.py matrix {latest_focus_lane} --summary --compact"
                    if isinstance(latest_focus_lane, str) and latest_focus_lane
                    else None
                ),
                "check": (
                    f"python tools/query_rti_work.py check --lane {latest_focus_lane} --summary --compact"
                    if isinstance(latest_focus_lane, str) and latest_focus_lane
                    else None
                ),
                "ctest": lane_ctest_command(latest_handles),
            }
            for command_name, command in latest_commands.items():
                if isinstance(command, str) and command:
                    print(f"   latest_{command_name}={command}")
        indexed_counts = result.get("mapping_counts", {})
        comparable_count_keys = (
            set(indexed_counts).intersection(mapping_counts)
            if isinstance(indexed_counts, dict)
            else set()
        )
        indexed_queue_counts = (
            {key: indexed_counts.get(key) for key in comparable_count_keys}
            if isinstance(indexed_counts, dict)
            else {}
        )
        live_queue_counts = {
            key: mapping_counts.get(key) for key in comparable_count_keys
        }
        if isinstance(indexed_counts, dict) and indexed_queue_counts != live_queue_counts:
            print("Traceability index snapshot differs from the live local counts; run the bounded queries before editing the index.")
        print("Open indexed work:")
        if arguments.summary:
            for item in sorted(
                result["roadmap_items"],
                key=lambda value: value.get("priority", 999),
            ):
                if item.get("status") != "open":
                    continue
                print(
                    f"{item.get('priority', '?')}. {item.get('id')}: "
                    f"{item.get('title')}"
                )
                print(
                    f"   kind={item.get('kind')} "
                    f"tagged_cases={item.get('matching_test_count', 0)} "
                    f"tag_count={len(item.get('query_tags', []))}"
                )
                live_counts = item.get("live_mapping_counts", {})
                if isinstance(live_counts, dict):
                    print(
                        "   mapping="
                        f"{live_counts.get('mapped_case_count', 0)} mapped/"
                        f"{live_counts.get('explicit_disposition_count', 0)} explicit/"
                        f"{live_counts.get('unclassified_count', 0)} unclassified; "
                        f"{live_counts.get('standard_section_count', 0)} sections/"
                        f"{live_counts.get('cpp_api_surface_count', 0)} api; "
                        f"{live_counts.get('resolved_requirement_section_pair_count', 0)} "
                        "direct requirement-section pairs; "
                        f"{live_counts.get('source_unlocated_count', 0)} source-unlocated"
                    )
                selectors: list[str] = []
                if item.get("next_work_id"):
                    selectors.append(f"next_work={item.get('next_work_id')}")
                if item.get("next_lane"):
                    selectors.append(f"next_lane={item.get('next_lane')}")
                if item.get("next_source_test_query"):
                    selectors.append(
                        f"next_source={item.get('next_source_test_query')}"
                    )
                if item.get("next_source_location"):
                    selectors.append(
                        f"next_source_location={item.get('next_source_location')}"
                    )
                if item.get("next_source_lane"):
                    selectors.append(
                        f"next_source_lane={item.get('next_source_lane')}"
                    )
                if item.get("next_source_state"):
                    selectors.append(
                        f"next_source_state={item.get('next_source_state')}"
                    )
                if item.get("next_test_query"):
                    pointer = item.get("next_test_pointer", {})
                    selectors.append(
                        f"test={compact_prose(item.get('next_test_query'), 120)}"
                    )
                    if isinstance(pointer, dict):
                        selectors.append(
                            "test_mapping="
                            f"{pointer.get('requirement_count', 0)}req/"
                            f"{pointer.get('standard_section_count', 0)}sec/"
                            f"{pointer.get('cpp_api_surface_count', 0)}api"
                        )
                        if pointer.get("query"):
                            selectors.append(
                                "trace=python tools/query_rti_work.py trace "
                                f"\"{pointer.get('query')}\" --summary --compact"
                            )
                if selectors:
                    print(f"   {' '.join(selectors)}")
                live_action = live_next_action(item)
                if live_action:
                    print(f"   next={compact_prose(live_action, 220)}")
            return 0
        for item in sorted(result["roadmap_items"], key=lambda value: value.get("priority", 999)):
            if item.get("status") != "open":
                continue
            print(f"{item.get('priority', '?')}. {item.get('id')}: {item.get('title')}")
            if arguments.compact:
                print(
                    f"   kind={item.get('kind')} tag_count={len(item.get('query_tags', []))} "
                    f"tagged_cases={item.get('matching_test_count', 0)}"
                )
            else:
                counts = ", ".join(
                    f"{tag}={count}"
                    for tag, count in item["matching_test_counts"].items()
                ) or "no tagged cases"
                print(f"   kind={item.get('kind')} tags=({counts})")
            focus_value = item.get("live_current_focus") or item.get("current_focus")
            if focus_value:
                focus = compact_prose(focus_value) if arguments.compact else focus_value
                print(f"   focus={focus}")
            if item.get("next_task"):
                task = compact_prose(item.get("next_task")) if arguments.compact else item.get("next_task")
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            live_action = live_next_action(item)
            next_action = compact_prose(live_action) if arguments.compact and live_action else live_action
            print(f"   next={next_action}")
        return 0

    if arguments.command == "next":
        # ``ready`` is the authoritative executable/planned handoff.  Keep
        # the older roadmap-pointer view available behind ``--pointer`` (and
        # the explicit ``--all`` diagnostic), but do not let the default
        # ``next`` command present a completed historical lane as new work.
        if not arguments.pointer and not arguments.all:
            ready_result = ready_slice(
                index,
                tests,
                source_locations,
                requested_family=arguments.family,
                include_source_only=arguments.include_source_only,
                standard_requirements=standard_requirements,
            )
            if arguments.json:
                print(json.dumps(ready_result, indent=2, sort_keys=True))
                return 0 if ready_result.get("found") else 1
            if not ready_result.get("found"):
                print(
                    "Next bounded implementation slice: none; "
                    f"{ready_result.get('reason', 'no indexed handoff')}"
                )
                family_options = ready_result.get("family_options") or []
                if family_options:
                    print("Bounded family choices (select one; no Lab rescan):")
                    for option in family_options:
                        if not isinstance(option, dict):
                            continue
                        print(
                            f"- {option.get('id', '<unnamed>')}: "
                            f"state={option.get('state', '<unknown>')} "
                            f"cases={option.get('case_count', 0)} "
                            f"mapped={option.get('mapped_case_count', 0)} "
                            f"family_cases={option.get('family_case_count', option.get('case_count', 0))} "
                            f"family_mapped={option.get('family_mapped_case_count', option.get('mapped_case_count', 0))} "
                            f"requirements={option.get('requirement_count', 0)} "
                            f"sections={option.get('standard_section_count', 0)} "
                            f"direct_pairs={option.get('requirement_section_pair_count', 0)}"
                        )
                        if option.get("work_query"):
                            print(f"  work_query={option['work_query']}")
                        if option.get("lane_discovery_command"):
                            print(f"  lane_discovery={option['lane_discovery_command']}")
                        if option.get("mapping_lane_discovery_command"):
                            print(
                                "  mapping_lane_discovery="
                                f"{option['mapping_lane_discovery_command']}"
                            )
                        if option.get("gap_query"):
                            print(f"  gaps={option['gap_query']}")
                        for line in text_gap_preview(option):
                            print(line)
                        if option.get("next_action"):
                            action = (
                                compact_prose(option["next_action"], 220)
                                if arguments.compact or arguments.summary
                                else option["next_action"]
                            )
                            print(f"  next={action}")
                ready_command = "python tools/query_rti_work.py ready"
                if arguments.family:
                    ready_command += f" --family {arguments.family}"
                ready_command += " --summary --compact"
                print(f"ready_command: {ready_command}")
                return 0 if family_options else 1
            owner = ready_result.get("owner") or {}
            roadmap_owner = ready_result.get("roadmap_owner") or owner
            print("Next bounded implementation slice:")
            print(
                f"roadmap_owner: {roadmap_owner.get('id', '<none>')}: "
                f"{roadmap_owner.get('title', '<unnamed roadmap family>')}"
            )
            if owner.get("id") != roadmap_owner.get("id"):
                print(
                    f"active_pointer: {owner.get('id', '<none>')}: "
                    f"{owner.get('title', '<unnamed roadmap family>')}"
                )
            print(f"state: {ready_result.get('state')}")
            for field, label in (
                ("family_id", "family"),
                ("plan_id", "plan_id"),
                ("test_case", "test"),
                ("source_location", "source"),
                ("source_target", "source_target"),
                ("ctest_target", "ctest_target"),
                ("assertions", "assertions"),
                ("status", "status"),
            ):
                value = ready_result.get(field)
                if value is not None and value != "":
                    print(f"{label}: {value}")
            requirement_ids = strings(ready_result.get("requirement_ids"))
            standard_sections = strings(ready_result.get("standard_sections"))
            api_surfaces = strings(ready_result.get("api_surfaces"))
            print(
                "mapping: "
                f"requirements={len(requirement_ids)} "
                f"standard_sections={len(standard_sections)} "
                f"api_surfaces={len(api_surfaces)}"
            )
            if requirement_ids:
                print("requirements: " + ", ".join(requirement_ids[:8]))
            if standard_sections:
                print("standard_sections: " + ", ".join(standard_sections[:8]))
            if ready_result.get("trace_command"):
                print(f"trace_command: {ready_result['trace_command']}")
            if ready_result.get("seed_trace_command"):
                print(f"seed_trace_command: {ready_result['seed_trace_command']}")
            if ready_result.get("mapping_seed_plan_id"):
                print(
                    "mapping_seed: "
                    f"{ready_result['mapping_seed_plan_id']} "
                    f"status={ready_result.get('mapping_status', '<unspecified>')}"
                )
            if ready_result.get("post_mapping_focus_command"):
                print(
                    f"post_mapping_focus_command: {ready_result['post_mapping_focus_command']}"
                )
            if ready_result.get("post_mapping_check_command"):
                print(
                    f"post_mapping_check_command: {ready_result['post_mapping_check_command']}"
                )
            if ready_result.get("focus_command"):
                print(f"focus_command: {ready_result['focus_command']}")
            if ready_result.get("check_command"):
                print(f"check_command: {ready_result['check_command']}")
            if ready_result.get("next_action"):
                print(f"next: {compact_prose(ready_result['next_action'], 320)}")
            return 0
        result = index_status(index, tests, roadmap_entries, source_locations)
        result["source_health"] = source_health
        open_items = [
            item
            for item in sorted(
                result["roadmap_items"], key=lambda value: value.get("priority", 999)
            )
            if item.get("status") == "open"
        ]
        selected_items = open_items if arguments.all else open_items[:1]
        if not selected_items:
            print("No open indexed roadmap items.")
            return 0
        if arguments.pointer:
            planned_queue = indexed_unlocated_plan_summary(index, tests)
            pointers = [
                {
                    "id": item.get("id"),
                    "title": item.get("title"),
                    "source_state": source_pointer_state(item),
                    "source_test": item.get("next_source_test_query"),
                    "source_location": item.get("next_source_location"),
                    "source_lane": item.get("next_source_lane"),
                    "source_requirement_ids": strings(item.get("next_source_requirement_ids")),
                    "source_standard_sections": strings(item.get("next_source_standard_sections")),
                    "source_api_surfaces": strings(item.get("next_source_api_surfaces")),
                    "source_ctest": item.get("next_source_ctest_filter"),
                    "planned_queue": planned_queue,
                    "family_queue_command": (
                        "python tools/query_rti_work.py queue --summary --compact"
                    ),
                }
                for item in selected_items
            ]
            if arguments.json:
                payload = {"items": pointers}
                if result.get("global_source_queue") is not None:
                    payload["global_source_queue"] = result["global_source_queue"]
                print(json.dumps(payload, indent=2, sort_keys=True))
                return 0
            print("Next source pointer:")
            for pointer in pointers:
                print(f"work: {pointer['id']}: {pointer['title']}")
                print(f"source_state: {pointer['source_state']}")
                print(f"test: {pointer['source_test'] or '<none>'}")
                print(f"location: {pointer['source_location'] or '<unlocated>'}")
                print(f"lane: {pointer['source_lane'] or '<none>'}")
                print(f"ctest: {pointer['source_ctest'] or '<none>'}")
                print(
                    "mapping: "
                    f"requirements={len(pointer['source_requirement_ids'])} "
                    f"standard_sections={len(pointer['source_standard_sections'])} "
                    f"api_surfaces={len(pointer['source_api_surfaces'])}"
                )
                global_queue = result.get("global_source_queue")
                if (
                    pointer["source_state"] == "exhausted"
                    and isinstance(global_queue, dict)
                    and global_queue.get("count", 0)
                ):
                    print(
                        "global_source_queue: "
                        f"{global_queue['count']} unplanned declarations; "
                        f"head={global_queue.get('test_query', '<unnamed>')} "
                        f"({global_queue.get('source_location', '<unlocated>')})"
                    )
                    print(f"global_source_command: {global_queue['command']}")
                planned_queue = pointer.get("planned_queue")
                if (
                    pointer["source_state"] == "exhausted"
                    and isinstance(planned_queue, dict)
                    and planned_queue.get("count", 0)
                    and isinstance(planned_queue.get("head"), dict)
                ):
                    head = planned_queue["head"]
                    print(
                        "planned_queue: "
                        f"{planned_queue['count']} overlapping planned rows; "
                        f"head={head.get('test_case', '<unnamed>')} "
                        f"(plan={head.get('plan_id', '<none>')}; "
                        f"requirements={len(head.get('requirement_ids', []))}; "
                        f"standard_sections={len(head.get('standard_sections', []))}; "
                        f"api_surfaces={len(head.get('api_surfaces', []))})"
                    )
                    if head.get("trace_command"):
                        print(f"planned_trace_command: {head['trace_command']}")
                if (
                    pointer["source_state"] == "exhausted"
                    and not (
                        isinstance(global_queue, dict)
                        and global_queue.get("count", 0)
                    )
                    and not (
                        isinstance(planned_queue, dict)
                        and planned_queue.get("count", 0)
                    )
                ):
                    print(
                        "indexed_queue: source and planned-row queues are exhausted; "
                        "choose the next open family by its bounded work/focus card"
                    )
                    print(
                        "indexed_queue_command: "
                        "python tools/query_rti_work.py queue --summary --compact"
                    )
            return 0
        if arguments.json:
            print(json.dumps({"items": selected_items}, indent=2, sort_keys=True))
            return 0
        print("Next indexed roadmap work:")
        for item in selected_items:
            print(f"{item.get('priority', '?')}. {item.get('id')}: {item.get('title')}")
            if arguments.summary:
                # Prefer plan-derived focus text in the bounded status card.
                # The human-maintained next_task prose can lag after a slice
                # is added; it remains available through the JSON index and
                # the exact work/case queries.
                live_focus = item.get("live_current_focus")
                if live_focus:
                    print(f"   focus={compact_prose(live_focus, 180)}")
                elif item.get("next_task"):
                    print(f"   task={compact_prose(item.get('next_task'), 180)}")
                if item.get("next_work_id"):
                    print(f"   work={item.get('next_work_id')}")
                    if item.get("next_work_id") != item.get("id"):
                        print(f"   work_item={item.get('next_work_id')}")
                if item.get("next_work_status"):
                    print(f"   work_status={item.get('next_work_status')}")
                if item.get("next_work_query"):
                    print(
                        f"   work_query={compact_prose(item.get('next_work_query'), 180)}"
                    )
                if item.get("next_lane"):
                    print(f"   lane={item.get('next_lane')}")
                    lane_snapshot = focused_lane_result(
                        index, tests, item.get("next_lane"), limit=1
                    )
                    print(
                        "   lane_state="
                        f"{lane_snapshot.get('lane_state', 'missing')} "
                        f"cases={lane_snapshot.get('test_count', 0)} "
                        f"assertions={lane_snapshot.get('assertion_count', 0)} "
                        f"mapped={lane_snapshot.get('mapped_test_count', 0)} "
                        f"candidates={lane_snapshot.get('candidate_count', 0)} "
                        f"source_drift={lane_snapshot.get('source_drift_count', 0)}"
                    )
                    print(
                        "   focus_query="
                        "python tools/query_rti_work.py focus "
                        f"{item.get('next_lane')} --summary --compact"
                    )
                if item.get("next_ctest_filter"):
                    ctest_label = (
                        "baseline_ctest"
                        if item.get("next_test_role") == "baseline"
                        else "ctest"
                    )
                    print(f"   {ctest_label}={item.get('next_ctest_filter')}")
                for field, label in (
                    ("next_package_target", "package_target"),
                    ("next_package_ctest_filter", "package_ctest"),
                    ("next_process_probe_target", "process_probe_target"),
                    ("next_process_package_target", "process_package_target"),
                    ("next_process_package_test", "process_package_test"),
                    ("next_process_package_ctest_filter", "process_package_ctest"),
                    ("next_process_package_timestamped_test", "process_package_timestamped_test"),
                    ("next_process_package_timestamped_ctest_filter", "process_package_timestamped_ctest"),
                    ("next_process_package_parameterized_test", "process_package_parameterized_test"),
                    ("next_process_package_parameterized_ctest_filter", "process_package_parameterized_ctest"),
                    ("next_process_package_connection_loss_test", "process_package_connection_loss_test"),
                    ("next_process_package_connection_loss_ctest_filter", "process_package_connection_loss_ctest"),
                    ("next_process_package_object_registration_test", "process_package_object_registration_test"),
                    ("next_process_package_object_registration_ctest_filter", "process_package_object_registration_ctest"),
                    ("next_process_package_named_registration_test", "process_package_named_registration_test"),
                    ("next_process_package_named_registration_ctest_filter", "process_package_named_registration_ctest"),
                    ("next_process_package_attribute_update_test", "process_package_attribute_update_test"),
                    ("next_process_package_attribute_update_ctest_filter", "process_package_attribute_update_ctest"),
                    ("next_process_package_directed_retraction_test", "process_package_directed_retraction_test"),
                    ("next_process_package_directed_retraction_ctest_filter", "process_package_directed_retraction_ctest"),
                    ("next_process_package_catalog_verifier", "process_package_catalog_verifier"),
                ):
                    if item.get(field):
                        print(f"   {label}={item.get(field)}")
                if item.get("next_test_query"):
                    test_label = (
                        "baseline_test"
                        if item.get("next_test_role") == "baseline"
                        else "test"
                    )
                    print(f"   {test_label}={compact_prose(item.get('next_test_query'), 180)}")
                    pointer = item.get("next_test_pointer", {})
                    if isinstance(pointer, dict):
                        print(
                            f"   test_pointer={pointer.get('state', 'unknown')} "
                            f"matches={pointer.get('match_count', 0)} "
                            f"statuses={','.join(pointer.get('statuses', {}).keys()) or '<none>'}"
                        )
                if item.get("next_source_test_query"):
                    print(
                        f"   next_source_state={item.get('next_source_state', 'unplanned-source')}"
                    )
                    print(f"   next_source_test={item.get('next_source_test_query')}")
                    if item.get("next_source_location"):
                        print(f"   next_source_location={item.get('next_source_location')}")
                    if item.get("next_source_lane"):
                        print(f"   next_source_lane={item.get('next_source_lane')}")
                    source_requirement_ids = strings(item.get("next_source_requirement_ids"))
                    source_standard_sections = strings(item.get("next_source_standard_sections"))
                    source_api_surfaces = strings(item.get("next_source_api_surfaces"))
                    print(
                        "   next_source_mapping: "
                        f"requirements={len(source_requirement_ids)} "
                        f"standard_sections={len(source_standard_sections)} "
                        f"api_surfaces={len(source_api_surfaces)}"
                    )
                    if source_requirement_ids:
                        print(
                            "   next_source_requirement_ids="
                            f"{','.join(source_requirement_ids)}"
                        )
                    if source_standard_sections:
                        print(
                            "   next_source_standard_sections="
                            f"{','.join(source_standard_sections)}"
                        )
                    if source_api_surfaces:
                        print(
                            "   next_source_api_surfaces="
                            f"{','.join(source_api_surfaces)}"
                        )
                    if item.get("next_source_ctest_filter"):
                        print(f"   next_source_ctest={item.get('next_source_ctest_filter')}")
                elif source_pointer_state(item) == "exhausted":
                    print(
                        "   next_source_state=exhausted "
                        f"lane={item.get('next_source_lane') or '<unspecified>'}"
                    )
                sections = strings(item.get("next_standard_sections"))
                if sections:
                    if arguments.compact or arguments.summary:
                        preview = ",".join(sections[:8])
                        if len(sections) > 8:
                            preview += f",...(+{len(sections) - 8})"
                    else:
                        preview = ",".join(sections)
                    print(f"   standard_sections={preview}")
                print(
                    f"   mapped_handles: standards={len(strings(item.get('next_standard_sections')))} "
                    f"plan_ids={len(strings(item.get('next_plan_ids')))} "
                    f"catalog_gaps={len(strings(item.get('catalog_gap_plan_ids')))}"
                )
                continue
            tags = strings(item.get("query_tags")) + strings(item.get("focused_lane_tags"))
            if arguments.compact:
                print(f"   tag_count={len(tags)}")
            else:
                print(f"   tags={', '.join(tags) or '<none>'}")
            print(f"   anchor={item.get('roadmap_anchor')}")
            live_focus = item.get("live_current_focus") or item.get("current_focus")
            if live_focus:
                focus = compact_prose(live_focus) if arguments.compact else live_focus
                print(f"   focus={focus}")
            if item.get("next_task"):
                task = compact_prose(item.get("next_task")) if arguments.compact else item.get("next_task")
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            live_action = live_next_action(item)
            next_action = compact_prose(live_action) if arguments.compact and live_action else live_action
            print(f"   next={next_action}")
        return 0

    if arguments.command == "roadmap":
        result = roadmap_inventory(
            index,
            tests,
            query=arguments.query,
            status=arguments.status,
            limit=arguments.limit,
        )
        if arguments.json:
            if arguments.summary or arguments.compact:
                result["families"] = [
                    roadmap_summary_data(row) for row in result["families"]
                ]
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result["families"] or not arguments.query else 1
        if arguments.query and not result["count"]:
            print(
                f"No roadmap family matched: {arguments.query}",
                file=sys.stderr,
            )
            return 1
        scope = f" matching {arguments.query!r}" if arguments.query else ""
        shown = result["shown_count"]
        total = result["count"]
        if shown == total:
            print(f"Roadmap families ({arguments.status}){scope}: {total}.")
        else:
            print(
                f"Roadmap families ({arguments.status}){scope}: {total}; "
                f"showing {shown}. Use --limit 0 for all."
            )
        for row in result["families"]:
            counts = row.get("live_mapping_counts") or {}
            print(
                f"- {row.get('id', '<unnamed>')} [{row.get('kind', '<unspecified>')}] "
                f"priority={row.get('priority', '<unspecified>')} "
                f"status={row.get('status', '<unspecified>')} "
                f"action={row.get('action_state', '<unspecified>')}: "
                f"{row.get('title', '<unnamed family>')}"
            )
            print(
                "  live: "
                f"cases={row.get('matching_test_count', 0)} "
                f"mapped={counts.get('mapped_case_count', 0)} "
                f"explicit={counts.get('explicit_disposition_count', 0)} "
                f"unclassified={counts.get('unclassified_count', 0)} "
                f"candidates={counts.get('executable_candidate_count', 0)} "
                f"assertions={row.get('assertion_count', 0)} "
                f"requirements={counts.get('requirement_count', 0)} "
                f"sections={counts.get('standard_section_count', 0)} "
                f"direct_pairs={counts.get('requirement_section_pair_count', 0)}"
            )
            if row.get("next_lane"):
                print(f"  next_lane: {row['next_lane']}")
            active_handoff = row.get("active_handoff")
            if isinstance(active_handoff, dict):
                print(
                    "  active_handoff: "
                    f"{active_handoff.get('active_handoff_id', '<unnamed>')} "
                    f"state={active_handoff.get('state', '<unspecified>')} "
                    f"runnable={active_handoff.get('runnable', True)}"
                )
                print(
                    "  active_handoff_test: "
                    f"{active_handoff.get('test_case', '<unnamed>')} "
                    f"target={active_handoff.get('source_target') or '<unlocated>'}"
                )
                print(
                    "  active_handoff_mapping: "
                    f"{len(strings(active_handoff.get('requirement_ids')))} requirements; "
                    f"{len(strings(active_handoff.get('standard_sections')))} sections; "
                    f"{active_handoff.get('requirement_section_pair_count', 0)} direct pairs; "
                    f"{len(strings(active_handoff.get('api_surfaces')))} APIs"
                )
                if active_handoff.get("mapping_seed_plan_id"):
                    print(
                        "  active_handoff_seed: "
                        f"{active_handoff['mapping_seed_plan_id']}"
                    )
                if active_handoff.get("next_action"):
                    print(
                        "  active_handoff_next: "
                        + compact_prose(active_handoff["next_action"], 240)
                    )
            lane_matches = row.get("lane_matches") or []
            if isinstance(lane_matches, list):
                for lane_match in lane_matches[:8]:
                    if not isinstance(lane_match, dict):
                        continue
                    print(
                        "  lane_match: "
                        f"{lane_match.get('tag', '<unnamed>')} "
                        f"state={lane_match.get('state', '<unspecified>')} "
                        f"action={lane_match.get('action_state', '<unspecified>')} "
                        f"tests={lane_match.get('test_count', 0)} "
                        f"mapped={lane_match.get('mapped_count', 0)} "
                        f"requirements={lane_match.get('requirement_count', 0)} "
                        f"sections={lane_match.get('standard_section_count', 0)} "
                        f"direct_pairs={lane_match.get('requirement_section_pair_count', 0)}"
                    )
                    if lane_match.get("next_test"):
                        print(
                            "  lane_match_next_test: "
                            f"{lane_match['next_test']} "
                            f"(source={lane_match.get('next_source') or '<unlocated>'}; "
                            f"requirements={len(strings(lane_match.get('next_requirement_ids')))}; "
                            f"sections={len(strings(lane_match.get('next_standard_sections')))})"
                        )
                    if lane_match.get("representative_test"):
                        print(
                            "  lane_match_representative: "
                            f"{lane_match['representative_test']} "
                            f"(source={lane_match.get('representative_source') or '<unlocated>'}; "
                            f"requirements={len(strings(lane_match.get('representative_requirement_ids')))}; "
                            f"sections={len(strings(lane_match.get('representative_standard_sections')))})"
                        )
                    for label in (
                        "focus_command",
                        "representative_trace_command",
                        "next_trace_command",
                        "ctest_command",
                    ):
                        command = lane_match.get(label)
                        if command:
                            label_name = label.removesuffix("_command")
                            print(f"  lane_match_{label_name}: {command}")
            if row.get("active_pointer_id"):
                print(
                    f"  active_pointer: {row['active_pointer_id']}"
                    + (
                        f" (lane={row['active_pointer_lane']})"
                        if row.get("active_pointer_lane")
                        else ""
                    )
                )
            if row.get("next_source_test_query"):
                source_state = row.get("next_source_state") or "<unspecified>"
                source_location = row.get("next_source_location") or "<unlocated>"
                source_lane = row.get("next_source_lane")
                lane_suffix = f"; lane={source_lane}" if source_lane else ""
                print(
                    "  next_source: "
                    f"{row['next_source_test_query']} "
                    f"(state={source_state}; location={source_location}"
                    f"{lane_suffix})"
                )
            pointer = row.get("next_test_pointer") or {}
            if row.get("next_test_query"):
                print(
                    "  next_test: "
                    f"{row['next_test_query']} "
                    f"(state={pointer.get('state', '<unspecified>')}; "
                    f"requirements={pointer.get('requirement_count', 0)}; "
                    f"sections={pointer.get('standard_section_count', 0)}; "
                    f"direct_pairs={pointer.get('requirement_section_pair_count', 0)}; "
                    f"assertions={pointer.get('assertion_count', 0)})"
                )
                if arguments.summary or arguments.compact:
                    requirement_ids = strings(pointer.get("requirement_ids"))
                    sections = strings(pointer.get("standard_sections"))
                    if requirement_ids:
                        print(
                            "  next_test_requirements: "
                            + ", ".join(requirement_ids[:8])
                            + (f", ... (+{len(requirement_ids) - 8})" if len(requirement_ids) > 8 else "")
                        )
                    if sections:
                        print(
                            "  next_test_standard_sections: "
                            + ", ".join(sections[:8])
                            + (f", ... (+{len(sections) - 8})" if len(sections) > 8 else "")
                        )
                    pair_rows = pointer.get("requirement_section_mappings")
                    if isinstance(pair_rows, list) and pair_rows:
                        pair_preview = [
                            f"{row.get('lab_requirement_id')} -> {row.get('standard_section') or '<unresolved>'}"
                            for row in pair_rows[:8]
                            if isinstance(row, dict)
                        ]
                        if pair_preview:
                            remaining = pointer.get("requirement_section_mappings_remaining", 0)
                            suffix = f", ... (+{remaining})" if remaining else ""
                            print(
                                "  next_test_pairs: "
                                + ", ".join(pair_preview)
                                + suffix
                            )
            commands = row.get("commands") or {}
            for label in ("work", "focus", "matrix"):
                if commands.get(label):
                    print(f"  {label}: {commands[label]}")
            action = row.get("next_action")
            if isinstance(action, str) and action.strip():
                print(
                    "  next: "
                    + (compact_prose(action, 240) if arguments.compact or arguments.summary else action)
                )
        return 0

    if arguments.command == "ready":
        result = ready_slice(
            index,
            tests,
            source_locations,
            arguments.family,
            arguments.include_source_only,
            standard_requirements,
        )
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result.get("found") else 1
        if not result.get("found"):
            if result.get("requested_family"):
                print(f"family_selection: {result['requested_family']}")
            print(result.get("reason", "no bounded implementation handoff"))
            family_options = result.get("family_options")
            if isinstance(family_options, list) and family_options:
                print("family_options: bounded open families (choose one; no Lab rescan)")
                for option in family_options:
                    if not isinstance(option, dict):
                        continue
                    unlocated = unlocated_display(option)
                    print(
                        f"- {option.get('id', '<unnamed>')}: "
                        f"state={option.get('state', '<unknown>')} "
                        f"action={option.get('action_state', '<unspecified>')} "
                        f"cases={option.get('case_count', 0)} "
                        f"mapped={option.get('mapped_case_count', 0)} "
                        f"family_cases={option.get('family_case_count', option.get('case_count', 0))} "
                        f"family_mapped={option.get('family_mapped_case_count', option.get('mapped_case_count', 0))} "
                        f"requirements={option.get('requirement_count', 0)} "
                        f"sections={option.get('standard_section_count', 0)} "
                        f"direct_pairs={option.get('requirement_section_pair_count', 0)} "
                        f"unlocated={unlocated}"
                    )
                    requirement_preview = option.get("requirement_ids_preview") or []
                    section_preview = option.get("standard_sections_preview") or []
                    if result.get("requested_family") and requirement_preview:
                        suffix = (
                            f", ... (+{option.get('requirement_ids_remaining', 0)})"
                            if option.get("requirement_ids_remaining")
                            else ""
                        )
                        print(
                            "  requirements="
                            + ",".join(requirement_preview)
                            + suffix
                        )
                    if result.get("requested_family") and section_preview:
                        suffix = (
                            f", ... (+{option.get('standard_sections_remaining', 0)})"
                            if option.get("standard_sections_remaining")
                            else ""
                        )
                        print(
                            "  standard_sections="
                            + ",".join(section_preview)
                            + suffix
                        )
                    pair_preview = option.get("requirement_section_pairs_preview") or []
                    if result.get("requested_family") and pair_preview:
                        pairs = ", ".join(
                            f"{pair.get('lab_requirement_id')} -> {pair.get('standard_section') or '<unresolved>'}"
                            for pair in pair_preview
                            if isinstance(pair, dict)
                        )
                        suffix = (
                            f", ... (+{option.get('requirement_section_pairs_remaining', 0)})"
                            if option.get("requirement_section_pairs_remaining")
                            else ""
                        )
                        if pairs:
                            print(f"  direct_pairs={pairs}{suffix}")
                    if option.get("next_lane"):
                        print(f"  lane={option['next_lane']}")
                    next_test_mapping = option.get("next_test_mapping")
                    if option.get("next_test_query"):
                        print(
                            "  next_test="
                            f"{compact_prose(option['next_test_query'], 160)} "
                            f"(state={option.get('next_test_state', '<unspecified>')}; "
                            f"assertions={next_test_mapping.get('assertion_count', 0) if isinstance(next_test_mapping, dict) else 0}; "
                            f"requirements={next_test_mapping.get('requirement_count', 0) if isinstance(next_test_mapping, dict) else 0}; "
                            f"standard_sections={next_test_mapping.get('standard_section_count', 0) if isinstance(next_test_mapping, dict) else 0}; "
                            f"api_surfaces={next_test_mapping.get('cpp_api_surface_count', 0) if isinstance(next_test_mapping, dict) else 0})"
                        )
                    if option.get("work_query"):
                        print(f"  work_query={option['work_query']}")
                    if option.get("lane_discovery_command"):
                        print(
                            "  lane_discovery="
                            f"{option['lane_discovery_command']}"
                        )
                    if option.get("mapping_lane_discovery_command"):
                        print(
                            "  mapping_lane_discovery="
                            f"{option['mapping_lane_discovery_command']}"
                        )
                    if option.get("gap_query"):
                        print(f"  gaps={option['gap_query']}")
                    for line in text_gap_preview(option):
                        print(line)
                    if option.get("next_action"):
                        # The exhausted-queue response is itself an
                        # implementation handoff. Keep the action visible in
                        # compact mode, but collapse it so selecting a family
                        # never requires reopening the long roadmap prose.
                        action = (
                            compact_prose(option["next_action"], 220)
                            if arguments.compact or arguments.summary
                            else option["next_action"]
                        )
                        print(f"  next={action}")
            print(
                "queue_command: "
                "python tools/query_rti_work.py queue --summary --compact"
            )
            print(
                "next_command: "
                "python tools/query_rti_work.py next --summary --compact"
            )
            return 1
        owner = result.get("owner") or {}
        roadmap_owner = result.get("roadmap_owner") or owner
        print("Next bounded implementation slice:")
        if result.get("requested_family"):
            print(f"family_selection: {result['requested_family']}")
        print(
            f"roadmap_owner: {roadmap_owner.get('id', '<none>')}: "
            f"{roadmap_owner.get('title', '<unnamed roadmap family>')}"
        )
        if owner.get("id") != roadmap_owner.get("id"):
            print(
                f"active_pointer: {owner.get('id', '<none>')}: "
                f"{owner.get('title', '<unnamed roadmap family>')}"
            )
        print(f"state: {result.get('state')}")
        if result.get("handoff_kind"):
            print(
                f"handoff_kind: {result['handoff_kind']} "
                f"runnable={result.get('runnable', True)}"
            )
        if result.get("family_id"):
            print(f"family: {result['family_id']}")
        if result.get("plan_id"):
            print(f"plan_id: {result['plan_id']}")
        print(f"test: {result.get('test_case', '<unnamed>')}")
        if result.get("source_location"):
            print(f"source: {result['source_location']}")
        elif result.get("source_target"):
            print(f"source_target: {result['source_target']} (TEST_CASE not declared yet)")
        elif result.get("state") == "planned":
            print("source: <TEST_CASE not declared yet>")
        if result.get("source_lane"):
            print(f"lane: {result['source_lane']}")
        if result.get("ctest_filter"):
            print(f"ctest: {result['ctest_filter']}")
        if result.get("ctest_target"):
            print(f"ctest_target: {result['ctest_target']}")
        if result.get("assertions") is not None:
            print(f"assertions: {result['assertions']}")
        if result.get("status"):
            print(f"status: {result['status']}")
        print(
            "mapping: "
            f"requirements={len(strings(result.get('requirement_ids')))} "
            f"standard_sections={len(strings(result.get('standard_sections')))} "
            f"api_surfaces={len(strings(result.get('api_surfaces')))}"
        )
        def ready_preview(values: list[str], limit: int = 8) -> str:
            if not values:
                return "<none>"
            if not arguments.compact or len(values) <= limit:
                return ", ".join(values)
            return f"{', '.join(values[:limit])}, ... (+{len(values) - limit})"

        if result.get("requirement_ids"):
            print(
                "requirements: "
                f"{ready_preview(strings(result['requirement_ids']))}"
            )
        if result.get("standard_sections"):
            print(
                "standard_sections: "
                f"{ready_preview(strings(result['standard_sections']))}"
            )
        if result.get("api_surfaces") and not arguments.compact:
            print(f"api_surfaces: {', '.join(result['api_surfaces'])}")
        if result.get("planned_queue_count"):
            planned_queue = result["planned_queue_count"]
            diagnostic_queue = result.get(
                "planned_queue_diagnostic_count", planned_queue
            )
            suffix = (
                f" (diagnostic={diagnostic_queue})"
                if diagnostic_queue != planned_queue
                else ""
            )
            print(f"planned_queue_remaining: {planned_queue}{suffix}")
        if result.get("next_action"):
            print(f"next_action: {compact_prose(result['next_action'], 360)}")
        if result.get("mapping_seed_plan_id"):
            print(
                "mapping_seed: "
                f"{result['mapping_seed_plan_id']} "
                f"status={result.get('mapping_status', '<unspecified>')}"
            )
        if result.get("mapping_seed_policy") and not arguments.compact:
            print(f"mapping_seed_policy: {result['mapping_seed_policy']}")
        if result.get("acceptance") and not arguments.compact:
            for acceptance in result["acceptance"]:
                print(f"acceptance: {acceptance}")
        for label in (
            "trace_command",
            "seed_trace_command",
            "implementation_command",
            "focus_command",
            "check_command",
            "post_mapping_focus_command",
            "post_mapping_check_command",
        ):
            if result.get(label):
                print(f"{label}: {result[label]}")
        return 0

    if arguments.command == "focus":
        active_work = None
        lane = arguments.lane
        if not lane:
            active_work = indexed_work_slice(index, tests, None, source_locations)
            if active_work.get("found"):
                lane = active_work.get("lane")
        if not isinstance(lane, str) or not lane.strip():
            result = {
                "found": False,
                "reason": (
                    "no lane was supplied and the active indexed work item "
                    "has no exact lane handle"
                ),
            }
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            else:
                print(result["reason"], file=sys.stderr)
            return 1

        result = focused_lane_result(index, tests, lane.strip(), arguments.limit)
        result["found"] = result.get("lane_state") != "missing"
        if active_work is not None and active_work.get("found"):
            result["active_work_id"] = active_work.get("work_id")
            result["active_work_status"] = active_work.get("work_status")
            result["active_work_query"] = active_work.get("work_query")
            result["active_work_next_action"] = active_work.get("next_action")
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result["found"] else 1
        if not result["found"]:
            print(f"No indexed Catch2 lane matched: {lane}", file=sys.stderr)
            return 1

        print("Focused Catch2 lane:")
        print(f"lane: {result['lane']} ({result['lane_state']})")
        print(f"action_state: {result.get('action_state', '<unspecified>')}")
        if result.get("roadmap_owner"):
            print(f"roadmap_owner: {result['roadmap_owner']}")
        print(
            "cases: "
            f"{result['test_count']} "
            f"({result['implemented_test_count']} implemented, "
            f"{result['candidate_count']} executable candidates, "
            f"{result.get('planned_count', 0)} planned, "
            f"{result['source_drift_count']} source drift)"
        )
        print(
            "mapping: "
            f"{result['mapped_test_count']}/{result['test_count']} mapped; "
            f"requirements={result['requirement_count']}; "
            f"standard_sections={result['standard_section_count']}; "
            f"direct_pairs={result.get('requirement_section_pair_count', 0)}; "
            f"explicit_dispositions={result['explicit_disposition_count']}; "
            f"unclassified={result['unclassified_count']}"
        )
        if arguments.summary or arguments.compact:
            requirement_ids = strings(result.get("requirement_ids"))
            sections = strings(result.get("standard_sections"))
            if requirement_ids:
                suffix = f", ... (+{len(requirement_ids) - 8})" if len(requirement_ids) > 8 else ""
                print(f"requirements: {', '.join(requirement_ids[:8])}{suffix}")
            if sections:
                suffix = f", ... (+{len(sections) - 8})" if len(sections) > 8 else ""
                print(f"standard_sections: {', '.join(sections[:8])}{suffix}")
            mapping_rows = result.get("requirement_section_mappings")
            if isinstance(mapping_rows, list) and mapping_rows:
                pair_preview = [
                    f"{row.get('lab_requirement_id')} -> {row.get('standard_section') or '<unresolved>'}"
                    for row in mapping_rows[:8]
                    if isinstance(row, dict)
                ]
                if pair_preview:
                    remaining = result.get("requirement_section_mappings_remaining", 0)
                    suffix = f", ... (+{remaining})" if remaining else ""
                    print(f"direct_pairs: {', '.join(pair_preview)}{suffix}")
        print(
            "assertions: "
            f"{result['assertion_count']} total; "
            f"{result['executable_assertion_count']} executable candidates"
            + (
                f" (indexed lane total; plan rows record {result['recorded_assertion_count']})"
                if result.get("assertion_count_source") == "indexed-lane-total"
                else ""
            )
        )
        if result.get("lane_handles"):
            print("handles:")
            for name, value in result["lane_handles"].items():
                print(f"  {name}={value}")
            ctest_command = lane_ctest_command(result["lane_handles"])
            if ctest_command:
                print(f"  ctest_command={ctest_command}")
        if result["candidates"]:
            print(
                f"candidates (showing {result['shown_candidate_count']} "
                f"of {result['candidate_count']}):"
            )
            for candidate in result["candidates"]:
                print(text_test_summary(candidate))
        else:
            print(
                "candidates: none; no source-located unimplemented cases remain."
            )
            if result.get("planned_count"):
                print(
                    "planned: the next TEST_CASE is indexed but not declared yet; "
                    "use ready/trace for its mapping card."
                )
            if result["source_drift_count"]:
                print(
                    "note: source-drift rows remain historical and are not "
                    f"executable candidates (actionable={result.get('actionable_source_drift_count', 0)})."
                )
        if result.get("disabled_artifact_count"):
            print(
                "disabled_artifacts: "
                f"{result['disabled_artifact_count']} retained source artifact(s); "
                "not executable candidates."
            )
        return 0

    if arguments.command == "work":
        result = indexed_work_slice(index, tests, arguments.id, source_locations)
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result.get("found") else 1
        if not result.get("found"):
            print(
                f"No indexed work item matched: {arguments.id or '<next>'}",
                file=sys.stderr,
            )
            return 1
        if arguments.summary:
            parent = result["parent_item"]
            work_item = result["work_item"]
            print("Active indexed work slice (summary):")
            print(
                f"parent: {parent.get('id')}: {parent.get('title')} "
                f"(priority {parent.get('priority')})"
            )
            print(
                f"work: {work_item.get('id')}: {work_item.get('title')} "
                f"(status {result.get('work_status')})"
            )
            if result.get("task"):
                print(f"task: {compact_prose(result['task'], 240)}")
            if result.get("lane"):
                lane_count = result.get("lane_case_count")
                suffix = f" ({lane_count} cases)" if lane_count is not None else ""
                print(f"lane: {result['lane']}{suffix}")
                if result.get("lane_state"):
                    print(
                        "lane_state: "
                        f"{result['lane_state']}; "
                        f"mapped={result.get('lane_mapped_test_count', 0)}/"
                        f"{lane_count if lane_count is not None else '?'}; "
                        f"executable_candidates={result.get('lane_candidate_count', 0)}; "
                        f"source_drift={result.get('lane_source_drift_count', 0)}; "
                        f"assertions={result.get('lane_assertion_count', 0)}"
                    )
            if result.get("ctest_filter"):
                print(f"ctest: {result['ctest_filter']}")
            if result.get("next_source_state") == "exhausted":
                print(
                    "source_queue: exhausted for "
                    f"lane {result.get('next_source_lane') or '<unspecified>'}"
                )
                global_queue = result.get("global_source_queue")
                if isinstance(global_queue, dict) and global_queue.get("count", 0):
                    print(
                        "global_source_queue: "
                        f"{global_queue['count']} unplanned declarations; "
                        f"head={global_queue.get('test_query', '<unnamed>')} "
                        f"({global_queue.get('source_location', '<unlocated>')})"
                    )
                    print(f"global_source_command: {global_queue['command']}")
                planned_queue = result.get("planned_queue")
                if (
                    isinstance(planned_queue, dict)
                    and planned_queue.get("count", 0)
                    and isinstance(planned_queue.get("head"), dict)
                ):
                    head = planned_queue["head"]
                    print(
                        "planned_queue: "
                        f"{planned_queue['count']} overlapping planned rows; "
                        f"head={head.get('test_case', '<unnamed>')} "
                        f"(plan={head.get('plan_id', '<none>')}; "
                        f"requirements={len(head.get('requirement_ids', []))}; "
                        f"standard_sections={len(head.get('standard_sections', []))}; "
                        f"api_surfaces={len(head.get('api_surfaces', []))})"
                    )
                    if head.get("trace_command"):
                        print(f"planned_trace_command: {head['trace_command']}")
            baseline = result.get("baseline")
            if isinstance(baseline, dict):
                baseline_state = baseline.get("state") or baseline.get("status", "<unspecified>")
                print(
                    f"baseline: {baseline.get('test_case', '<unnamed test>')} "
                    f"({baseline_state}; source={source_location_text(baseline)}; "
                    f"requirements={baseline.get('requirement_count', 0)}; "
                    f"sections={len(strings(baseline.get('standard_sections')))}"
                    ")"
                )
            if result.get("next_source_test_query"):
                print(
                    "next_slice: "
                    f"{result['next_source_test_query']} "
                    f"(state={result.get('next_source_state') or 'unplanned-source'}; "
                    f"source={result.get('next_source_location') or '<unlocated>'})"
                )
                if result.get("next_source_lane"):
                    print(f"next_slice_lane: {result['next_source_lane']}")
                print(
                    "next_slice_mapping: "
                    f"requirements={len(strings(result.get('next_source_requirement_ids')))}; "
                    f"standard_sections={len(strings(result.get('next_source_standard_sections')))}; "
                    f"api_surfaces={len(strings(result.get('next_source_api_surfaces')))}"
                )
                source_sections = strings(result.get("next_source_standard_sections"))
                if source_sections:
                    print(f"next_slice_standard_sections: {', '.join(source_sections)}")
                source_requirement_ids = strings(result.get("next_source_requirement_ids"))
                if source_requirement_ids:
                    print(f"next_slice_requirements: {', '.join(source_requirement_ids)}")
                source_api_surfaces = strings(result.get("next_source_api_surfaces"))
                if source_api_surfaces:
                    print(f"next_slice_api_surfaces: {', '.join(source_api_surfaces)}")
                if result.get("next_source_ctest_filter"):
                    print(f"next_slice_ctest: {result['next_source_ctest_filter']}")
            sections = strings(result.get("standard_sections"))
            plan_ids = strings(result.get("plan_ids"))
            print(
                f"mapped work handles: standard_sections={len(sections)}; "
                f"plan_ids={len(plan_ids)}; "
                f"catalog_gaps={len(strings(result.get('catalog_gap_plan_ids')))}"
            )
            if result.get("next_action"):
                print(f"next_action: {compact_prose(result['next_action'], 240)}")
            print("commands:")
            for command in result.get("commands", [])[:5]:
                print(f"  {command}")
            if len(result.get("commands", [])) > 5:
                print("  (additional package/JUnit commands available with work --compact or --json)")
            return 0
        parent = result["parent_item"]
        work_item = result["work_item"]
        print("Active indexed work slice:")
        print(
            f"parent: {parent.get('id')}: {parent.get('title')} "
            f"(priority {parent.get('priority')})"
        )
        print(
            f"work: {work_item.get('id')}: {work_item.get('title')} "
            f"(status {result.get('work_status')})"
        )
        if result.get("task"):
            print(f"task: {compact_prose(result['task'], 240)}")
        if result.get("work_query"):
            print(f"work_query: {compact_prose(result['work_query'], 240)}")
        if result.get("lane"):
            lane_count = result.get("lane_case_count")
            suffix = f" ({lane_count} cases)" if lane_count is not None else ""
            print(f"lane: {result['lane']}{suffix}")
        if result.get("ctest_filter"):
            print(f"ctest: {result['ctest_filter']}")
        if result.get("next_source_state") == "exhausted":
            print(
                "source_queue: exhausted for "
                f"lane {result.get('next_source_lane') or '<unspecified>'}"
            )
            global_queue = result.get("global_source_queue")
            if isinstance(global_queue, dict) and global_queue.get("count", 0):
                print(
                    "global_source_queue: "
                    f"{global_queue['count']} unplanned declarations; "
                    f"head={global_queue.get('test_query', '<unnamed>')} "
                    f"({global_queue.get('source_location', '<unlocated>')})"
                )
                print(f"global_source_command: {global_queue['command']}")
            planned_queue = result.get("planned_queue")
            if (
                isinstance(planned_queue, dict)
                and planned_queue.get("count", 0)
                and isinstance(planned_queue.get("head"), dict)
            ):
                head = planned_queue["head"]
                print(
                    "planned_queue: "
                    f"{planned_queue['count']} overlapping planned rows; "
                    f"head={head.get('test_case', '<unnamed>')} "
                    f"(plan={head.get('plan_id', '<none>')}; "
                    f"requirements={len(head.get('requirement_ids', []))}; "
                    f"standard_sections={len(head.get('standard_sections', []))}; "
                    f"api_surfaces={len(head.get('api_surfaces', []))})"
                )
                if head.get("trace_command"):
                    print(f"planned_trace_command: {head['trace_command']}")
        if result.get("package_target"):
            print(f"package_target: {result['package_target']}")
        if result.get("package_ctest_filter"):
            print(f"package_ctest: {result['package_ctest_filter']}")
        if result.get("package_manifest"):
            print(f"package_manifest: {result['package_manifest']}")
        if result.get("junit_target"):
            print(f"junit_target: {result['junit_target']}")
        if result.get("junit_artifact"):
            print(f"junit_artifact: {result['junit_artifact']}")
        if result.get("process_probe_target"):
            print(f"process_probe_target: {result['process_probe_target']}")
        if result.get("process_package_target"):
            print(f"process_package_target: {result['process_package_target']}")
        if result.get("process_package_test"):
            print(f"process_package_test: {result['process_package_test']}")
        if result.get("process_package_ctest_filter"):
            print(
                "process_package_ctest: "
                f"{result['process_package_ctest_filter']}"
            )
        if result.get("process_package_timestamped_test"):
            print(
                "process_package_timestamped_test: "
                f"{result['process_package_timestamped_test']}"
            )
        if result.get("process_package_timestamped_ctest_filter"):
            print(
                "process_package_timestamped_ctest: "
                f"{result['process_package_timestamped_ctest_filter']}"
            )
        if result.get("process_package_parameterized_test"):
            print(
                "process_package_parameterized_test: "
                f"{result['process_package_parameterized_test']}"
            )
        if result.get("process_package_parameterized_ctest_filter"):
            print(
                "process_package_parameterized_ctest: "
                f"{result['process_package_parameterized_ctest_filter']}"
            )
        if result.get("process_package_connection_loss_test"):
            print(
                "process_package_connection_loss_test: "
                f"{result['process_package_connection_loss_test']}"
            )
        if result.get("process_package_connection_loss_ctest_filter"):
            print(
                "process_package_connection_loss_ctest: "
                f"{result['process_package_connection_loss_ctest_filter']}"
            )
        if result.get("process_package_object_registration_test"):
            print(
                "process_package_object_registration_test: "
                f"{result['process_package_object_registration_test']}"
            )
        if result.get("process_package_object_registration_ctest_filter"):
            print(
                "process_package_object_registration_ctest: "
                f"{result['process_package_object_registration_ctest_filter']}"
            )
        if result.get("process_package_named_registration_test"):
            print(
                "process_package_named_registration_test: "
                f"{result['process_package_named_registration_test']}"
            )
        if result.get("process_package_named_registration_ctest_filter"):
            print(
                "process_package_named_registration_ctest: "
                f"{result['process_package_named_registration_ctest_filter']}"
            )
        if result.get("process_package_attribute_update_test"):
            print(
                "process_package_attribute_update_test: "
                f"{result['process_package_attribute_update_test']}"
            )
        if result.get("process_package_attribute_update_ctest_filter"):
            print(
                "process_package_attribute_update_ctest: "
                f"{result['process_package_attribute_update_ctest_filter']}"
            )
        if result.get("process_package_directed_retraction_test"):
            print(
                "process_package_directed_retraction_test: "
                f"{result['process_package_directed_retraction_test']}"
            )
        if result.get("process_package_directed_retraction_ctest_filter"):
            print(
                "process_package_directed_retraction_ctest: "
                f"{result['process_package_directed_retraction_ctest_filter']}"
            )
        if result.get("process_package_catalog_verifier"):
            print(
                "process_package_catalog_verifier: "
                f"{result['process_package_catalog_verifier']}"
            )
        baseline = result.get("baseline")
        if isinstance(baseline, dict):
            print(f"baseline: {baseline.get('test_case', '<unnamed test>')}")
            if baseline.get("state") == "missing":
                print("  baseline_status: missing from the Catch2 plan")
            else:
                print(
                    f"  baseline_status: {baseline.get('status', '<unspecified>')}; "
                    f"source: {source_location_text(baseline)}"
                )
                print(
                    f"  baseline_mapping: requirements={baseline.get('requirement_count', 0)}; "
                    f"standard_sections={len(strings(baseline.get('standard_sections')))}"
                )
                requirement_ids = strings(baseline.get("requirement_ids"))
                if requirement_ids:
                    print(f"  baseline_requirements: {', '.join(requirement_ids)}")
                remaining = baseline.get("requirement_ids_remaining")
                if remaining:
                    print(f"  baseline_requirements_remaining: {remaining}")
                sections = strings(baseline.get("standard_sections"))
                if sections:
                    print(f"  baseline_standard_sections: {', '.join(sections)}")
        if result.get("next_source_test_query"):
            print(
                "next_slice: "
                f"{result['next_source_test_query']} "
                f"(state={result.get('next_source_state') or 'unplanned-source'}; "
                f"source={result.get('next_source_location') or '<unlocated>'})"
            )
            if result.get("next_source_lane"):
                print(f"next_slice_lane: {result['next_source_lane']}")
            source_requirement_ids = strings(result.get("next_source_requirement_ids"))
            source_sections = strings(result.get("next_source_standard_sections"))
            source_api_surfaces = strings(result.get("next_source_api_surfaces"))
            print(
                "next_slice_mapping: "
                f"requirements={len(source_requirement_ids)}; "
                f"standard_sections={len(source_sections)}; "
                f"api_surfaces={len(source_api_surfaces)}"
            )
            if source_requirement_ids:
                print(f"next_slice_requirements: {', '.join(source_requirement_ids)}")
            if source_sections:
                print(f"next_slice_standard_sections: {', '.join(source_sections)}")
            if source_api_surfaces:
                print(f"next_slice_api_surfaces: {', '.join(source_api_surfaces)}")
            if result.get("next_source_ctest_filter"):
                print(f"next_slice_ctest: {result['next_source_ctest_filter']}")
        sections = strings(result.get("standard_sections"))
        if sections:
            print(f"work_standard_sections: {', '.join(sections)}")
        plan_ids = strings(result.get("plan_ids"))
        if plan_ids:
            print(f"work_plan_ids: {', '.join(plan_ids)}")
        catalog_gaps = strings(result.get("catalog_gap_plan_ids"))
        if catalog_gaps:
            print(f"catalog_gaps: {', '.join(catalog_gaps)}")
        if result.get("next_action"):
            print(f"next_action: {compact_prose(result['next_action'], 240)}")
        if result.get("commands"):
            print("commands:")
            for command in result["commands"]:
                print(f"  {command}")
        return 0

    if arguments.command == "recent":
        all_slices = recent_completed_views(index, tests, arguments.lane)
        shown_slices = (
            all_slices
            if not arguments.limit
            else all_slices[: arguments.limit]
        )
        result = {
            "count": len(all_slices),
            "shown_count": len(shown_slices),
            "slices": shown_slices,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        if not all_slices:
            print("No recent completed slices are indexed.", file=sys.stderr)
            return 1
        lane_suffix = f" for lane {arguments.lane!r}" if arguments.lane else ""
        if len(shown_slices) == len(all_slices):
            print(f"{len(all_slices)} recent completed slices{lane_suffix}.")
        else:
            print(
                f"{len(all_slices)} recent completed slices; showing "
                f"{len(shown_slices)}{lane_suffix}. Use --limit 0 for all."
            )
        for slice_value in shown_slices:
            print(
                f"- {slice_value.get('plan_id', '<unnamed>')}: "
                f"{slice_value.get('test_query', '<unnamed test>')}"
            )
            print(f"  source: {source_location_text({'source_locations': slice_value.get('source_locations', [])})}")
            print(
                f"  assertions: {slice_value.get('assertions', '<unspecified>')}; "
                f"status: {slice_value.get('status', '<unspecified>')}; "
                f"matched_tests={slice_value.get('matched_test_count', 0)}"
            )
            callback_models = strings(slice_value.get("callback_models"))
            if callback_models:
                print(f"  callback_models: {', '.join(callback_models)}")
            lanes = strings(slice_value.get("lane_queries"))
            if lanes:
                print(f"  lanes: {', '.join(lanes)}")
            api_surface_statuses = sorted(
                {
                    status.strip()
                    for test in slice_value.get("tests", [])
                    if isinstance(test, dict)
                    for status in [test.get("requirements_lab_api_surface_status")]
                    if isinstance(status, str) and status.strip()
                }
            )
            for api_surface_status in api_surface_statuses:
                print(
                    "  Requirements-Lab API surface: "
                    + (
                        compact_prose(api_surface_status, 360)
                        if arguments.summary
                        else api_surface_status
                    )
                )
            sections = strings(
                slice_value.get("resolved_standard_sections")
                or slice_value.get("standard_sections")
            )
            if arguments.compact:
                print(
                    f"  mapped: requirements={slice_value.get('resolved_requirement_count', 0)}; "
                    f"standard_sections={len(sections)}"
                )
            elif sections:
                print(f"  standard_sections: {', '.join(sections)}")
        return 0

    if arguments.command == "item":
        item_value, matching_tests = indexed_item(index, tests, arguments.id)
        if item_value is None:
            if arguments.json:
                print(json.dumps({"id": arguments.id, "found": False}, indent=2))
            else:
                print(f"No indexed roadmap item matched: {arguments.id}", file=sys.stderr)
            return 1
        shown_tests = matching_tests if not arguments.limit else matching_tests[: arguments.limit]
        result = {
            "item": item_value,
            "test_count": len(matching_tests),
            "shown_test_count": len(shown_tests),
            "tests": shown_tests,
        }
        if arguments.json:
            if arguments.summary:
                result["tests"] = [test_summary_data(test) for test in shown_tests]
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        print(f"{item_value.get('id')}: {item_value.get('title')}")
        print(f"status: {item_value.get('status')}; priority: {item_value.get('priority')}")
        print(f"anchor: {item_value.get('roadmap_anchor')}")
        query_tags = strings(item_value.get("query_tags"))
        if arguments.compact:
            print(f"query tag count: {len(query_tags)}")
        else:
            print(f"query tags: {', '.join(query_tags) or '<none>'}")
        live_focus = live_item_focus(item_value, matching_tests)
        if live_focus:
            focus = compact_prose(live_focus) if arguments.compact else live_focus
            print(f"current focus: {focus}")
        if item_value.get("next_task") and not arguments.compact:
            task = compact_prose(item_value.get("next_task")) if arguments.compact else item_value.get("next_task")
            print(f"next task: {task}")
        handles = item_handles(item_value, arguments.compact)
        if handles:
            print(f"handles: {' '.join(handles)}")
        live_action = live_next_action(item_value)
        next_action = compact_prose(live_action) if arguments.compact and live_action else live_action
        print(f"next: {next_action}")
        if len(shown_tests) == len(matching_tests):
            print(f"{len(matching_tests)} tagged Catch2 cases.")
        else:
            print(f"{len(matching_tests)} tagged Catch2 cases; showing {len(shown_tests)}.")
        if shown_tests:
            renderer = (
                text_test_summary
                if arguments.summary
                else lambda test: text_test(test, arguments.verbose, arguments.compact)
            )
            print(
                "\n".join(
                    renderer(test)
                    for test in shown_tests
                )
            )
        return 0

    if arguments.command == "plan":
        try:
            sections = implementation_plan_sections(arguments.implementation_plan)
        except OSError as error:
            print(f"query_rti_work: {error}", file=sys.stderr)
            return 2
        matches, shown_sections = filtered_plan_sections(
            sections,
            arguments.query,
            arguments.limit,
        )
        result = {
            "path": relative_path(arguments.implementation_plan),
            "query": arguments.query,
            "section_count": len(matches),
            "shown_section_count": len(shown_sections),
            "sections": shown_sections,
        }
        if arguments.json:
            if arguments.query and not matches:
                result["found"] = False
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if matches or not arguments.query else 1
        if arguments.query and not matches:
            print(
                f"No implementation-plan heading matched: {arguments.query}",
                file=sys.stderr,
            )
            return 1
        print(f"Implementation plan: {result['path']}")
        if arguments.query:
            print(
                f"Heading matches for {arguments.query!r}: "
                f"{len(matches)} (showing {len(shown_sections)})"
            )
        elif len(shown_sections) < len(matches):
            print(
                f"Showing {len(shown_sections)} of {len(matches)} headings; "
                "use --limit 0 for all."
            )
        for section in shown_sections:
            indent = "  " * max(section["level"] - 1, 0)
            path = section.get("path")
            suffix = f" [{path}]" if arguments.query and path else ""
            print(f"{indent}- line {section['line']}: {section['title']}{suffix}")
        return 0

    if arguments.command == "coverage":
        scope_lane = arguments.lane
        scope_family = arguments.family
        if scope_lane is not None and scope_family is not None:
            message = "coverage accepts either --lane or --family, not both"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        family_item = None
        if scope_family is not None:
            family_item, _ = indexed_item(index, tests, scope_family)
            if family_item is None:
                message = f"No indexed roadmap family matched: {scope_family}"
                if arguments.json:
                    print(
                        json.dumps(
                            {
                                "found": False,
                                "scope_family": scope_family,
                                "error": message,
                            },
                            indent=2,
                        )
                    )
                else:
                    print(message, file=sys.stderr)
                return 1
        scoped_tests = (
            tests
            if scope_lane is None and family_item is None
            else (
                item_tests(family_item, tests)
                if family_item is not None
                else [
                    test
                    for test in tests
                    if any(
                        str(tag).casefold() == scope_lane.casefold()
                        for tag in strings(test.get("tags"))
                    )
                ]
            )
        )
        references = [
            requirement
            for test in scoped_tests
            for requirement in test.get("requirements", [])
        ]
        requirement_section_rows = [
            row
            for test in scoped_tests
            for row in requirement_section_mapping_rows(test)
            if isinstance(row, dict)
        ]
        requirement_section_pairs = {
            (row.get("lab_requirement_id"), row.get("standard_section"))
            for row in requirement_section_rows
            if row.get("lab_requirement_id")
        }
        resolved_requirement_section_pairs = {
            pair
            for pair in requirement_section_pairs
            if pair[1]
        }
        mapped_tests = sum(bool(test.get("requirements")) for test in scoped_tests)
        unresolved = sorted(
            {
                identifier
                for test in scoped_tests
                for identifier in test.get("unresolved_requirement_ids", [])
            }
        )
        recorded_assertion_count = sum(
            value
            for test in scoped_tests
            for value in [test.get("assertions")]
            if isinstance(value, int)
        )
        mapping = index.get("mapping", {})
        if not isinstance(mapping, dict):
            mapping = {}
        lane_assertion_counts = mapping.get("lane_assertion_counts", {})
        if not isinstance(lane_assertion_counts, dict):
            lane_assertion_counts = {}
        configured_assertion_count = lane_assertion_counts.get(scope_lane)
        assertion_count = (
            configured_assertion_count
            if isinstance(configured_assertion_count, int)
            else recorded_assertion_count
        )
        result = {
            "scope_lane": scope_lane,
            "scope_family": scope_family,
            "roadmap_owner": (
                roadmap_item_summary(family_item)
                if isinstance(family_item, dict)
                else None
            ),
            "test_count": len(scoped_tests),
            "assertion_count": assertion_count,
            "recorded_assertion_count": recorded_assertion_count,
            "assertion_count_source": (
                "indexed-lane-total"
                if isinstance(configured_assertion_count, int)
                else "plan-entry-records"
            ),
            "tests_with_source_location": sum(
                bool(test.get("source_locations")) for test in scoped_tests
            ),
            "tests_without_source_location": sum(
                not test.get("source_locations") for test in scoped_tests
            ),
            "tests_with_requirement_mapping": mapped_tests,
            "tests_without_requirement_mapping": len(scoped_tests) - mapped_tests,
            "requirement_reference_count": len(references),
            "unique_requirement_count": len(
                {requirement["lab_requirement_id"] for requirement in references}
            ),
            "resolved_requirement_reference_count": sum(
                requirement.get("standard") is not None for requirement in references
            ),
            "requirement_section_pair_count": len(requirement_section_pairs),
            "resolved_requirement_section_pair_count": len(
                resolved_requirement_section_pairs
            ),
            "unresolved_requirement_section_mapping_count": sum(
                bool(row.get("unresolved")) for row in requirement_section_rows
            ),
            "unique_standard_clause_count": len(
                {
                    clause
                    for test in scoped_tests
                    for clause in test.get("standard_clauses", [])
                }
            ),
            "unresolved_requirement_ids": unresolved,
        }
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
        else:
            scope_text = (
                f" in roadmap family {scope_family!r}"
                if scope_family
                else f" in lane {scope_lane!r}" if scope_lane else ""
            )
            print(f"C++ Catch2 cases{scope_text}: {result['test_count']}")
            assertion_note = (
                f" (indexed lane total; plan rows record {result['recorded_assertion_count']})"
                if result.get("assertion_count_source") == "indexed-lane-total"
                else ""
            )
            print(f"Assertions: {result['assertion_count']}{assertion_note}")
            print(
                f"Mapped to Lab requirements: {result['tests_with_requirement_mapping']} "
                f"({result['tests_without_requirement_mapping']} without a requirement mapping)"
            )
            print(
                f"C++ source locations: {result['tests_with_source_location']} "
                f"({result['tests_without_source_location']} unlocated)"
            )
            print(
                f"Requirement references: {result['requirement_reference_count']} "
                f"across {result['unique_requirement_count']} unique IDs"
            )
            print(
                f"Resolved 2025 references: {result['resolved_requirement_reference_count']}; "
                f"standard clauses/subsections: {result['unique_standard_clause_count']}"
            )
            print(
                "Direct requirement-to-2025 subsection pairs: "
                f"{result['resolved_requirement_section_pair_count']} unique "
                f"({result['requirement_section_pair_count']} including unresolved; "
                f"{result['unresolved_requirement_section_mapping_count']} unresolved)"
            )
            if unresolved:
                print("Unresolved requirement IDs:")
                print("\n".join(f"- {identifier}" for identifier in unresolved[:20]))
        return 0

    if arguments.command == "gaps":
        if arguments.limit < 0:
            message = "gaps --limit must be non-negative"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        if arguments.family:
            family_folded = arguments.family.casefold()
            family_exists = any(
                isinstance(item, dict)
                and str(item.get("id") or "").casefold() == family_folded
                for item in index.get("items", [])
            )
            if not family_exists:
                message = f"gaps --family did not match an indexed roadmap family: {arguments.family}"
                if arguments.json:
                    print(json.dumps({"found": False, "error": message}, indent=2))
                else:
                    print(message, file=sys.stderr)
                return 1
        result = requirement_gap_inventory(
            standard_requirements,
            tests,
            index=index,
            query=arguments.query,
            document=arguments.document,
            clause=arguments.clause,
            coverage_kind=arguments.coverage_kind,
            family=arguments.family,
            limit=arguments.limit,
        )
        if arguments.json:
            if arguments.summary:
                result = requirement_gap_summary_data(result)
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        scope = []
        for label, value in (
            ("family", arguments.family),
            ("query", arguments.query),
            ("document", arguments.document),
            ("clause", arguments.clause),
            ("coverage_kind", arguments.coverage_kind),
        ):
            if value:
                scope.append(f"{label}={value!r}")
        scope_text = f" ({', '.join(scope)})" if scope else ""
        print(
            "2025 requirement gaps"
            + scope_text
            + ": "
            f"{result['uncovered_requirement_count']} uncovered of "
            f"{result['total_requirement_count']} filtered; "
            f"{result['covered_requirement_count']} covered "
            f"({result['coverage_percent']:.2f}% mapped)"
        )
        groups = result.get("groups", [])
        if groups:
            suffix = (
                f"; showing {len(groups)} of {result['group_count']} subsections"
                if result["shown_group_count"] < result["group_count"]
                else ""
            )
            print("Uncovered subsections" + suffix + ":")
            for group in groups:
                print(
                    f"- {group['standard_section']}: "
                    f"{group['uncovered_requirement_count']} uncovered / "
                    f"{group['total_requirement_count']} total; "
                    f"sample={', '.join(group.get('uncovered_requirement_ids', []))}"
                )
        requirements = [] if arguments.summary else result.get("requirements", [])
        if requirements:
            suffix = (
                f"; showing {len(requirements)} of {result['uncovered_requirement_count']} records"
                if result["shown_requirement_count"] < result["uncovered_requirement_count"]
                else ""
            )
            print("Requirement records" + suffix + ":")
            for requirement in requirements:
                source = requirement.get("source") or {}
                source_text = (
                    f"{source.get('path')}:{source.get('line')}"
                    if isinstance(source, dict) and source.get("path")
                    else "<unlocated>"
                )
                title = compact_prose(requirement.get("title"), 160)
                print(
                    f"- {requirement.get('id')} -> {requirement.get('standard_section')} "
                    f"({source_text}) {title}"
                )
                if requirement.get("requirement_query"):
                    print(f"  query={requirement['requirement_query']}")
        elif not groups:
            print("No uncovered requirements matched the supplied filters.")
        return 0

    if arguments.command == "lanes":
        if arguments.family_query and arguments.family:
            message = "lanes accepts either a family argument or --family, not both"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        requested_family = arguments.family_query or arguments.family
        requested_limit = arguments.limit
        if requested_limit is None:
            # ``lanes --compact``/``--summary`` are intended for work selection;
            # keep those views bounded so a tag inventory cannot fill a context
            # window.  The unqualified command retains its complete inventory.
            requested_limit = 40 if arguments.compact or arguments.summary else 0
        result = lane_inventory(
            index,
            tests,
            family=requested_family,
            unmapped_only=arguments.unmapped,
            disposition=arguments.disposition,
            limit=requested_limit,
        )
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result.get("found") else 1
        if not result.get("found"):
            print(result.get("error", "lane inventory could not be resolved"), file=sys.stderr)
            return 1
        scope = (
            f" in roadmap family {requested_family!r}"
            if requested_family
            else ""
        )
        if result["shown_count"] == result["count"]:
            print(f"{result['count']} exact Catch2 lane tags{scope}.")
        else:
            print(
                f"{result['count']} exact Catch2 lane tags{scope}; "
                f"showing {result['shown_count']}. "
                "Use --limit 0 for all."
            )
        if arguments.unmapped or arguments.disposition != "all":
            if arguments.disposition == "explicit":
                filter_text = "lanes with explicit no-standalone-surface dispositions."
            elif arguments.disposition == "unclassified":
                filter_text = "lanes with rows still needing a requirement mapping."
            else:
                filter_text = "lanes with explicit or unclassified missing requirement mappings."
            print(f"Filter: {filter_text}")
        for lane_value in result["lanes"]:
            print(
                f"- {lane_value['tag']}: {lane_value['state']} "
                f"cases={lane_value['test_count']} "
                f"mapped={lane_value['mapped_count']} "
                f"unclassified={lane_value['unclassified_count']} "
                f"explicit={lane_value['explicit_disposition_count']} "
                f"source_drift={lane_value['source_drift_count']} "
                f"candidates={lane_value['candidate_count']} "
                f"assertions={lane_value['assertion_count']} "
                f"requirements={lane_value['requirement_count']} "
                f"sections={lane_value['standard_section_count']} "
                f"direct_pairs={lane_value['resolved_requirement_section_pair_count']}"
            )
            if (
                lane_value.get("assertion_count_source") == "indexed-lane-total"
                and lane_value.get("recorded_assertion_count")
                != lane_value.get("assertion_count")
            ):
                print(
                    "  assertion_source=indexed-lane-total; "
                    f"plan_row_assertions={lane_value.get('recorded_assertion_count', 0)}"
                )
            if lane_value.get("next_test"):
                print(
                    f"  next={compact_prose(lane_value['next_test'], 180)}"
                    f" ({lane_value.get('next_source') or '<unlocated>'}; "
                    f"assertions={lane_value.get('next_assertions')}; "
                    f"requirements={len(lane_value.get('next_requirement_ids', []))}; "
                    f"sections={len(lane_value.get('next_standard_sections', []))})"
                )
                if lane_value.get("next_trace_command"):
                    print(f"  trace={lane_value['next_trace_command']}")
            elif lane_value.get("review_test"):
                # Explicit no-standalone-surface rows are still useful when
                # auditing the taxonomy, but they are not runnable work.  A
                # distinct label prevents a bounded lane listing from
                # turning review material into a false implementation head.
                print(
                    f"  review_only={compact_prose(lane_value['review_test'], 180)}"
                    f" ({lane_value.get('review_source') or '<unlocated>'}; "
                    f"assertions={lane_value.get('review_assertions')})"
                )
                if lane_value.get("review_trace_command"):
                    print(f"  review_trace={lane_value['review_trace_command']}")
            print(f"  focus={lane_value['focus_command']}")
            if lane_value.get("unmapped_command"):
                print(f"  unmapped={lane_value['unmapped_command']}")
            if lane_value.get("ctest_command"):
                print(f"  ctest={lane_value['ctest_command']}")
        return 0
    if arguments.command == "unmapped":
        family_item, scoped_tests = scoped_plan_tests(
            index,
            tests,
            lane=arguments.lane,
            family=arguments.family,
        )
        if arguments.lane is not None and arguments.family is not None:
            message = "unmapped accepts either --lane or --family, not both"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        if arguments.family is not None and family_item is None:
            message = f"No indexed roadmap family matched: {arguments.family}"
            if arguments.json:
                print(
                    json.dumps(
                        {
                            "found": False,
                            "scope_family": arguments.family,
                            "error": message,
                        },
                        indent=2,
                    )
                )
            else:
                print(message, file=sys.stderr)
            return 1
        unmapped_tests = [test for test in scoped_tests if not test.get("requirements")]
        disposition_counts = Counter(
            traceability_state(test)
            for test in unmapped_tests
        )
        if arguments.disposition == "explicit":
            selected = [
                test
                for test in unmapped_tests
                if traceability_state(test) == "explicit-disposition"
            ]
        elif arguments.disposition == "unclassified":
            selected = [
                test
                for test in unmapped_tests
                if traceability_state(test) == "unclassified"
            ]
        else:
            selected = unmapped_tests
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        if arguments.show_contract_candidates:
            selected = [
                {
                    **test,
                    "contract_candidates": contract_candidates_for_test(
                        test, contract_test_candidates
                    ),
                }
                for test in selected
            ]
        result = {
            "scope_lane": arguments.lane,
            "scope_family": arguments.family,
            "disposition": arguments.disposition,
            "total_count": len(unmapped_tests),
            "disposition_counts": dict(sorted(disposition_counts.items())),
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "unlocated":
        family_item, scoped_tests = scoped_plan_tests(
            index,
            tests,
            lane=arguments.lane,
            family=arguments.family,
        )
        if arguments.lane is not None and arguments.family is not None:
            message = "unlocated accepts either --lane or --family, not both"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        if arguments.family is not None and family_item is None:
            message = f"No indexed roadmap family matched: {arguments.family}"
            if arguments.json:
                print(
                    json.dumps(
                        {
                            "found": False,
                            "scope_family": arguments.family,
                            "error": message,
                        },
                        indent=2,
                    )
                )
            else:
                print(message, file=sys.stderr)
            return 1
        selected = [test for test in scoped_tests if not test.get("source_locations")]
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "scope_lane": arguments.lane,
            "scope_family": arguments.family,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "unplanned":
        selected = unplanned_source_cases(
            tests,
            source_locations,
            arguments.path,
        )
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "source_path_query": arguments.path,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "lane":
        selected = select_tests(tests, arguments.tag, "lane")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        mapping = index.get("mapping", {})
        lane_owners = (
            mapping.get("lane_owners", {}) if isinstance(mapping, dict) else {}
        )
        roadmap_owner = (
            lane_owners.get(arguments.tag)
            if isinstance(lane_owners, dict)
            else None
        )
        lane_handles = {}
        configured_lane_handles = (
            mapping.get("lane_handles", {}) if isinstance(mapping, dict) else {}
        )
        if isinstance(configured_lane_handles, dict):
            configured = configured_lane_handles.get(arguments.tag)
            if isinstance(configured, dict):
                lane_handles = dict(configured)
        result: dict[str, Any] = {
            "tag": arguments.tag,
            "roadmap_owner": roadmap_owner,
            "lane_handles": lane_handles,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "test":
        selected = select_tests(tests, arguments.query, "test")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "case":
        folded = arguments.query.casefold()
        selected = [
            test
            for test in tests
            if str(test.get("id", "")).casefold() == folded
            or str(test.get("test_case", "")).casefold() == folded
        ]
        result = {
            "query": arguments.query,
            "found": len(selected) == 1,
            "ambiguous": len(selected) > 1,
            "count": len(selected),
            "shown_count": min(len(selected), 1),
            "case": case_card_record(selected[0], index, arguments.query)
            if len(selected) == 1
            else None,
            "matches": [test_summary_data(test) for test in selected[:8]],
        }
    elif arguments.command == "source":
        selected = tests_for_source_path(tests, arguments.path)
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "source_path_query": arguments.path,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    elif arguments.command == "requirement":
        selected = select_tests(tests, arguments.query, "requirement")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
        if not selected:
            # A requirement lookup is also the reverse entry point for an
            # uncovered pinned-2025 requirement.  Keep this fallback bounded
            # and local: it reads the checked-in corpus and plan only, never
            # reopening or resynchronizing the Requirements Lab.
            gap_inventory = requirement_gap_inventory(
                standard_requirements,
                tests,
                index=index,
                query=arguments.query,
                limit=arguments.limit,
            )
            if gap_inventory.get("uncovered_requirement_count", 0):
                result["gap_inventory"] = gap_inventory
    elif arguments.command == "section":
        selected = select_tests(tests, arguments.query, "section")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
        if not selected:
            # A section lookup is also useful before a C++ case exists. Keep
            # the reverse mapping useful for a new-case family handoff by
            # returning the uncovered pinned-2025 records for that exact
            # canonical subsection.
            gap_inventory = requirement_gap_inventory(
                standard_requirements,
                tests,
                index=index,
                query=arguments.query,
                limit=arguments.limit,
            )
            if gap_inventory.get("uncovered_requirement_count", 0):
                result["gap_inventory"] = gap_inventory
    elif arguments.command == "trace":
        match_kind, matches = resolve_trace_query(tests, arguments.query)
        if match_kind is None:
            result = {
                "query": arguments.query,
                "found": False,
                "match_kind": None,
                "count": 0,
                "shown_count": 0,
                "tests": [],
            }
        else:
            selected = matches if not arguments.limit else matches[: arguments.limit]
            result = {
                "query": arguments.query,
                "found": True,
                "match_kind": match_kind,
                "count": len(matches),
                "shown_count": len(selected),
                "tests": [
                    trace_record(test, match_kind, arguments.query, index)
                    for test in selected
                ],
            }
    elif arguments.command == "matrix":
        query = arguments.query
        active_lane = None
        if query is None:
            active_work = indexed_work_slice(index, tests, None, source_locations)
            active_lane = active_work.get("lane") if active_work.get("found") else None
            if isinstance(active_lane, str) and active_lane:
                match_kind = "active-lane"
                matches = [
                    test for test in tests if active_lane in strings(test.get("tags"))
                ]
                query = active_lane
            else:
                match_kind = None
                matches = []
        else:
            match_kind, matches = resolve_matrix_query(index, tests, query)
        matches = sorted(matches, key=source_location_sort_key)
        selected = matches if not arguments.limit else matches[: arguments.limit]
        result = {
            "query": query,
            "requested_query": arguments.query,
            "active_lane": active_lane,
            "found": bool(matches),
            "match_kind": match_kind,
            "count": len(matches),
            "shown_count": len(selected),
            "tests": [matrix_summary_data(test, index) for test in selected],
        }
    elif arguments.command == "search":
        selected = select_tests(tests, arguments.query, "search")
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": arguments.query,
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    else:
        print(f"query_rti_work: unsupported command {arguments.command}", file=sys.stderr)
        return 2

    if arguments.command == "case":
        if not result.get("found"):
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            elif result.get("ambiguous"):
                print(
                    f"Exact case handle is ambiguous: {arguments.query!r} "
                    f"matched {result.get('count', 0)} cases. Use the plan id "
                    "shown below or a narrower exact title.",
                    file=sys.stderr,
                )
                for match in result.get("matches", []):
                    if isinstance(match, dict):
                        print(
                            f"- {match.get('id', '<unnamed>')}: "
                            f"{match.get('test_case', '<unnamed test>')}",
                            file=sys.stderr,
                        )
            else:
                print(
                    "No exact case handle matched. Use test or search for "
                    "discovery, then pass the plan id or full TEST_CASE title.",
                    file=sys.stderr,
                )
            return 1
        card = result.get("case")
        if not isinstance(card, dict):
            print("query_rti_work: exact case card is unavailable", file=sys.stderr)
            return 2
        if arguments.json:
            print(
                json.dumps(
                    card if not arguments.summary else {
                        key: card.get(key)
                        for key in (
                            "query",
                            "id",
                            "test_case",
                            "status",
                            "source_locations",
                            "source_state",
                            "assertions",
                            "callback_models",
                            "requirements",
                            "standard_sections",
                            "requirement_section_mappings",
                            "cpp_api_surfaces",
                            "roadmap_owner",
                            "lane",
                            "commands",
                        )
                    },
                    indent=2,
                    sort_keys=True,
                )
            )
            return 0
        print(text_case_card(card, compact=arguments.compact or arguments.summary))
        return 0

    if arguments.command == "trace":
        if not result.get("found"):
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            else:
                print(
                    "No exact trace handle matched. Use test, requirement, "
                    "section, lane, or search for a broader query.",
                    file=sys.stderr,
                )
            return 1
        if arguments.json:
            if arguments.summary:
                result["tests"] = [
                    trace_summary_record(record) for record in result["tests"]
                ]
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        shown_count = result.get("shown_count", result["count"])
        kind = result.get("match_kind") or "handle"
        if shown_count == result["count"]:
            print(f"Trace ({kind}): {result['count']} matched.")
        else:
            print(
                f"Trace ({kind}): {result['count']} matched; "
                f"showing {shown_count}."
            )
        print(
            "\n".join(
                text_trace(record, summary=arguments.summary)
                for record in result["tests"]
            )
        )
        return 0

    if arguments.command == "matrix":
        if not result.get("found"):
            if arguments.json:
                print(json.dumps(result, indent=2, sort_keys=True))
            else:
                query = result.get("requested_query") or "<active indexed lane>"
                print(
                    f"No exact matrix handle matched: {query}. Use trace for an "
                    "exact handle or search for discovery.",
                    file=sys.stderr,
                )
            return 1
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0
        shown_count = result.get("shown_count", result["count"])
        kind = result.get("match_kind") or "handle"
        query = result.get("query") or "<active indexed lane>"
        if shown_count == result["count"]:
            print(f"Traceability matrix ({kind}; {query}): {result['count']} matched.")
        else:
            print(
                f"Traceability matrix ({kind}; {query}): {result['count']} matched; "
                f"showing {shown_count}."
            )
        print(
            "\n".join(
                text_matrix_row(
                    test,
                    index,
                    compact=arguments.compact or arguments.summary,
                )
                for test in result["tests"]
            )
        )
        return 0

    if arguments.command == "unplanned":
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result["tests"] else 1
        path_suffix = (
            f" under source paths containing {arguments.path!r}"
            if arguments.path
            else ""
        )
        if not result["tests"]:
            print(f"No unplanned C++ TEST_CASE declarations{path_suffix}.", file=sys.stderr)
            return 1
        shown_count = result["shown_count"]
        if shown_count == result["count"]:
            print(f"{result['count']} unplanned C++ TEST_CASE declarations{path_suffix}.")
        else:
            print(
                f"{result['count']} unplanned C++ TEST_CASE declarations{path_suffix}; "
                f"showing {shown_count}."
            )
        for record in result["tests"]:
            print(
                f"- {record.get('test_case', '<unnamed test>')}: "
                f"{source_location_text(record)}"
            )
        return 0

    if arguments.json:
        if arguments.summary:
            result["tests"] = [test_summary_data(test) for test in result["tests"]]
            gap_inventory = result.get("gap_inventory")
            if isinstance(gap_inventory, dict):
                result["gap_inventory"] = requirement_gap_lookup_summary_data(
                    gap_inventory
                )
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    if not result["tests"]:
        if arguments.command == "unlocated":
            scope_text = (
                f" in roadmap family {arguments.family!r}"
                if getattr(arguments, "family", None)
                else f" in lane {arguments.lane!r}"
                if getattr(arguments, "lane", None)
                else ""
            )
            print(f"No unlocated Catch2 plan cases{scope_text}.", file=sys.stderr)
        elif arguments.command == "unmapped":
            scope_text = (
                f" in roadmap family {arguments.family!r}"
                if getattr(arguments, "family", None)
                else f" in lane {arguments.lane!r}"
                if getattr(arguments, "lane", None)
                else ""
            )
            print(f"No unmapped Catch2 cases{scope_text}.", file=sys.stderr)
        elif arguments.command == "source":
            print(
                f"No Catch2 cases matched source path {arguments.path!r}.",
                file=sys.stderr,
            )
        elif arguments.command in {"requirement", "section"} and isinstance(
            result.get("gap_inventory"), dict
        ):
            gap_inventory = result["gap_inventory"]
            records = gap_inventory.get("requirements", [])
            if not isinstance(records, list):
                records = []
            if arguments.command == "requirement":
                print(
                    f"No mapped Catch2 cases matched requirement query {arguments.query!r}."
                )
            else:
                print(
                    f"No mapped Catch2 cases matched section query {arguments.query!r}."
                )
            print(
                "Uncovered pinned 2025 requirements: "
                f"{gap_inventory.get('uncovered_requirement_count', 0)}"
            )
            if records:
                shown_records = records[:8] if arguments.summary else records
                print("Requirement gap records:")
                for requirement in shown_records:
                    source = requirement.get("source") or {}
                    source_text = (
                        f"{source.get('path')}:{source.get('line')}"
                        if isinstance(source, dict) and source.get("path")
                        else "<unlocated>"
                    )
                    title = compact_prose(requirement.get("title"), 160)
                    print(
                        f"- {requirement.get('id')} -> "
                        f"{requirement.get('standard_section')} ({source_text}) {title}"
                    )
                    statement = compact_prose(requirement.get("statement"), 360)
                    if statement:
                        print(f"  statement: {statement}")
                if len(records) > len(shown_records):
                    print(
                        f"  ... (+{len(records) - len(shown_records)}); "
                        "use --json or a narrower query for the remaining records"
                    )
            return 0
        else:
            print(f"No Catch2 cases matched {arguments.command} query.", file=sys.stderr)
        return 1
    shown_count = result.get("shown_count", result["count"])
    if arguments.command == "lane" and result.get("roadmap_owner"):
        print(f"Roadmap owner: {result['roadmap_owner']}")
    if arguments.command == "lane" and result.get("lane_handles"):
        for name, value in result["lane_handles"].items():
            print(f"{name}: {value}")
        ctest_command = lane_ctest_command(result["lane_handles"])
        if ctest_command:
            print(f"ctest_command: {ctest_command}")
    if arguments.command == "unlocated":
        scope_text = (
            f" in roadmap family {arguments.family!r}"
            if getattr(arguments, "family", None)
            else f" in lane {arguments.lane!r}"
            if getattr(arguments, "lane", None)
            else ""
        )
        if shown_count == result["count"]:
            print(f"{result['count']} Catch2 plan cases{scope_text} have no matching TEST_CASE source declaration.")
        else:
            print(
                f"{result['count']} Catch2 plan cases{scope_text} have no matching TEST_CASE source declaration; "
                f"showing {shown_count}."
            )
    elif arguments.command == "unmapped":
        scope_text = (
            f" in roadmap family {arguments.family!r}"
            if getattr(arguments, "family", None)
            else f" in lane {arguments.lane!r}"
            if getattr(arguments, "lane", None)
            else ""
        )
        if shown_count == result["count"]:
            print(f"{result['count']} unmapped Catch2 cases{scope_text}.")
        else:
            print(f"{result['count']} unmapped Catch2 cases{scope_text}; showing {shown_count}.")
        disposition_counts = result.get("disposition_counts", {})
        if isinstance(disposition_counts, dict):
            explicit_count = disposition_counts.get("explicit-disposition", 0)
            unclassified_count = disposition_counts.get("unclassified", 0)
            if result.get("disposition") == "all":
                print(
                    "Disposition split: "
                    f"explicit={explicit_count}; unclassified={unclassified_count}. "
                    "Use --disposition explicit or --disposition unclassified."
                )
            else:
                print(f"Disposition filter: {result.get('disposition')}")
    elif arguments.command == "source":
        if shown_count == result["count"]:
            print(
                f"{result['count']} Catch2 cases under source paths containing "
                f"{arguments.path!r}."
            )
        else:
            print(
                f"{result['count']} Catch2 cases under source paths containing "
                f"{arguments.path!r}; showing {shown_count}."
            )
    elif shown_count == result["count"]:
        print(f"{result['count']} Catch2 cases matched.")
    else:
        print(f"{result['count']} Catch2 cases matched; showing {shown_count}.")
    # Reverse lookups are commonly the first bounded query after a roadmap
    # family is selected.  Their useful payload is the direct requirement ->
    # canonical subsection relationship, not the full per-case contract
    # provenance.  Keep summary output on the same compact matrix row used by
    # ``matrix`` so ``section``/``requirement`` do not expand into a large
    # repeated report.  The full ``test`` view remains available when callers
    # need every field.
    if arguments.summary:
        renderer = lambda test: text_query_summary_row(
            arguments.command,
            test,
            index,
            getattr(arguments, "query", None),
        )
    else:
        renderer = lambda test: text_test(
            test,
            arguments.verbose,
            arguments.compact,
        )
    print("\n".join(renderer(test) for test in result["tests"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
