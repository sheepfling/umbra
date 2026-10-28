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
Use ``matrix --group-by requirement`` or ``matrix --group-by section`` when a
standards review needs one row per Lab requirement or canonical 2025 subsection;
each aggregate retains exact pair/test/assertion counts and bounded case ids.
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
``search`` accepts one quoted phrase or multiple terms; multiple terms are
ANDed by default so a bounded discovery query such as ``search restore work``
does not require a repository-wide scan. Use ``--any-term`` only when a broad
taxonomy lookup is intentional.
``contract-drift`` is the bounded local contract-selector guard: it compares
native ``cpp/tests/...::Title`` references with current C++ declarations and
counts external portable-TCK symbols without treating them as native drift.
``lanes`` is the bounded forward view: an optional roadmap-family scope becomes
one row per exact Catch2 tag with mapping state and a deterministic next-case
handle, so lane selection does not require a broad plan search.  Its optional
disposition filter separates intentional no-standalone-surface decisions from
rows that still need a mapping decision.
Use ``lanes --focused`` with a family to restrict discovery to that family's
explicit ``focused_lane_tags`` aliases; the default still exposes broad
cross-cutting tags for taxonomy review.
It also accepts an exact indexed roadmap family id as an aggregate handle.
``roadmap`` is the bounded family search: an optional title/tag/action,
Requirements-Lab id, canonical 2025 subsection, or official C++ API query
returns live family counts and copyable work/focus/matrix handles without
printing roadmap prose. An exact Catch2 lane-tag query additionally embeds the
lane's representative test/source/case/trace and focused CTest handles, so
lane ownership and mapping can be resolved in one command. The same card
adds exact case/trace and requirement-/section-grouped matrix handles for the
roadmap's ``next_test`` pointer.
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
``work <family-id>`` additionally exposes live distinct-case/assertion totals
and copyable requirement- and subsection-grouped matrix commands; those
totals are derived from the selected Catch2 rows rather than legacy pointer
metadata.
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
from collections.abc import Iterable, Mapping, Sequence
from pathlib import Path
from typing import Any, TypeAlias, TypedDict, cast

JsonScalar: TypeAlias = str | int | float | bool | None
JsonValue: TypeAlias = JsonScalar | Sequence["JsonValue"] | Mapping[str, "JsonValue"]
JsonObject: TypeAlias = dict[str, JsonValue]


class TestSourceLocation(TypedDict):
    path: str
    line: int


class ControlCharacterSample(TypedDict):
    path: str
    line: int
    codepoint: str


class ConditionalSample(TypedDict):
    path: str
    open_conditionals: int
    unmatched_endif_lines: list[int]


class TestSourceHealth(TypedDict):
    status: str
    files_scanned: int
    files_with_control_characters: int
    control_character_count: int
    control_character_samples: list[ControlCharacterSample]
    files_with_unbalanced_conditionals: int
    unbalanced_conditional_count: int
    unbalanced_conditional_samples: list[ConditionalSample]
    hint: str


class RequirementGapGroup(TypedDict):
    standard_section: str | None
    total_requirement_count: int
    mapped_requirement_count: int
    promoted_requirement_count: int
    candidate_only_requirement_count: int
    uncovered_requirement_count: int
    uncovered_requirement_ids: list[str]


class RequirementGapSection(TypedDict):
    standard_section: str | None
    document_id: str | None
    clause_id: str | None
    total_requirement_count: int
    covered_requirement_count: int
    uncovered_requirement_count: int
    globally_covered_requirement_count: int
    globally_uncovered_requirement_count: int
    uncovered_requirement_ids: list[str]


class RequirementSectionPair(TypedDict):
    lab_requirement_id: str
    standard_section: str | None


class ActiveHandoffRequirementRow(TypedDict):
    lab_requirement_id: str
    standard_section: str
    document_id: str
    clause_id: str
    title: JsonValue
    unresolved: bool


class RequirementSectionOverride(TypedDict):
    standard_section: str
    document_id: str
    clause_id: str
    title: JsonValue
    clause: JsonValue
    requirements_lab_issue_id: JsonValue
    basis: JsonValue


class LiveItemMappingCounts(TypedDict):
    case_count: int
    mapped_case_count: int
    explicit_disposition_count: int
    unclassified_count: int
    planned_count: int
    source_unlocated_count: int
    executable_candidate_count: int
    requirement_count: int
    standard_section_count: int
    cpp_api_surface_count: int
    assertion_count: int
    requirement_section_pair_count: int
    resolved_requirement_section_pair_count: int
    unresolved_requirement_section_mapping_count: int
    requirement_ids_preview: list[str]
    requirement_ids_remaining: int
    standard_sections_preview: list[str]
    standard_sections_remaining: int
    requirement_section_pairs_preview: list[RequirementSectionPair]
    requirement_section_pairs_remaining: int


class MatrixCrosswalkAccumulator(TypedDict):
    group_by: str
    key: str
    lab_requirement_id: str | None
    standard_section: str | None
    standard_sections: set[str]
    lab_requirement_ids: set[str]
    requirement_titles: dict[str, str]
    tests: list[JsonObject]
    assertion_count: int
    assertions_unspecified_test_count: int
    pair_count: int


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_INDEX = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP-INDEX.json"
DEFAULT_PLAN = (
    REPOSITORY_ROOT / "compliance" / "requirements-lab" / "catch2-test-plan.json"
)
DEFAULT_BUNDLE = REPOSITORY_ROOT / ".compliance" / "corpus-bundle.json"
DEFAULT_CONTRACT_DIRECTORY = REPOSITORY_ROOT / "compliance" / "requirements-lab"
DEFAULT_CPP_TCK_CATALOG = (
    REPOSITORY_ROOT / "compliance" / "catalogs" / "cpp-tck-scenario-catalog.json"
)
DEFAULT_LAB_ISSUES = DEFAULT_CONTRACT_DIRECTORY / "known-issues.json"
DEFAULT_ROADMAP = REPOSITORY_ROOT / "docs" / "planning" / "ROADMAP.md"
DEFAULT_IMPLEMENTATION_PLAN = (
    REPOSITORY_ROOT / "docs" / "planning" / "IMPLEMENTATION-PLAN.md"
)
DEFAULT_TEST_ROOT = REPOSITORY_ROOT / "cpp" / "tests"


_CATCH2_TEST_CASE = re.compile(
    r'\bTEST_CASE(?:_[A-Za-z0-9_]+)?\s*\(\s*"((?:\\.|[^"\\])*)"',
    re.MULTILINE,
)
_CPP_CONDITIONAL_DIRECTIVE = re.compile(
    r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)$"
)


def load_json(path: Path) -> JsonObject:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read JSON file {path}: {error}") from error
    if not isinstance(value, dict):
        # A decoded file with the wrong root shape is malformed content.
        raise ValueError(f"JSON root must be an object: {path}")  # noqa: TRY004
    # json.loads has JSON-only values; the object check above establishes the
    # mapping boundary for every caller of this helper.
    return cast(JsonObject, value)


def load_cpp_tck_scenarios() -> dict[str, JsonObject]:
    catalog = load_json(DEFAULT_CPP_TCK_CATALOG)
    rows = catalog.get("scenarios", [])
    if not isinstance(rows, list):
        return {}
    scenarios: dict[str, JsonObject] = {}
    for value in rows:
        if not isinstance(value, dict):
            continue
        row = cast(JsonObject, value)
        scenario_id = row.get("id")
        if isinstance(scenario_id, str):
            scenarios[scenario_id] = row
    return scenarios


def json_object_rows(value: JsonValue | None) -> list[JsonObject]:
    """Narrow one decoded JSON array to the object records it contains."""

    if not isinstance(value, list):
        return []
    return [cast(JsonObject, row) for row in value if isinstance(row, dict)]


def json_object_value(value: JsonValue | None) -> JsonObject | None:
    """Narrow one decoded JSON value when it contains an object record."""

    return cast(JsonObject, value) if isinstance(value, dict) else None


def json_integer(value: JsonValue | None, default: int = 0) -> int:
    """Read an integer field from decoded JSON without accepting booleans."""

    return value if isinstance(value, int) and not isinstance(value, bool) else default


def roadmap_priority(item: JsonObject) -> int:
    """Return an indexed roadmap priority or its established last-place value."""

    priority = item.get("priority")
    return priority if isinstance(priority, int) else 999


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


def text_gap_preview(option: JsonObject, prefix: str = "  ") -> list[str]:
    """Render the bounded uncovered-requirement handoff for one family row.

    ``ready``/``next``/``resume`` all use the same family-choice projection.
    Keep the human-readable form as small as the JSON form: one globally
    uncovered requirement from the family's subsection with the largest gap,
    its 2025 subsection, and copyable reverse-lookups. Explicitly non-callable
    table-structure rows remain in the full ``gaps`` inventory, not as work
    candidates.
    """

    preview = option.get("gap_preview")
    if not isinstance(preview, dict):
        return []
    covered = preview.get("covered_requirement_count")
    family_mapped = preview.get("family_mapped_requirement_count", covered)
    family_section_count = preview.get("family_section_count")
    if isinstance(family_mapped, int):
        coverage_text = f"family-mapped requirements={family_mapped}"
        if isinstance(family_section_count, int):
            coverage_text += f"; represented subsections={family_section_count}"
        global_candidates = preview.get("global_uncovered_candidate_count")
        if isinstance(global_candidates, int):
            previewable = preview.get("preview_candidate_count", 0)
            inventory_only = preview.get("inventory_only_gap_count", 0)
            out_of_scope = preview.get("out_of_scope_candidate_count", 0)
            coverage_text += (
                f"; {global_candidates} global gaps in represented subsections"
            )
            if (
                isinstance(previewable, int)
                and isinstance(inventory_only, int)
                and isinstance(out_of_scope, int)
            ):
                coverage_text += (
                    f" ({previewable} IEEE 1516.1-2025 previewable; "
                    f"{out_of_scope} other-document gaps excluded; "
                    f"{inventory_only} inventory-only excluded)"
                )
        lines = [f"{prefix}gap_scope={coverage_text}"]
    else:
        lines = []
    requirement = preview.get("requirement")
    if not isinstance(requirement, dict):
        inventory_only = preview.get("inventory_only_gap_count", 0)
        if isinstance(inventory_only, int) and inventory_only:
            lines.append(
                f"{prefix}gap_candidate=none; {inventory_only} uncovered structural rows are inventory-only because the corpus says table structure is not itself a callable API mapping"
            )
        else:
            lines.append(
                f"{prefix}gap_candidate=none; no previewable uncovered row in the family's mapped subsections"
            )
        return lines
    requirement_id = requirement.get("id") or "<unidentified>"
    standard_section = requirement.get("standard_section") or "<unresolved>"
    lines.append(
        f"{prefix}gap_inventory_head={requirement_id} -> {standard_section} "
        "(coverage inventory only; not a queued implementation task)"
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


def unlocated_display(row: JsonObject) -> str:
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


def is_planned_test(test: JsonObject) -> bool:
    """Return whether a plan row is an intentional future implementation slice.

    Planned rows are allowed to precede their C++ ``TEST_CASE`` declaration so
    the roadmap can carry the exact title, lane, and mapping before source
    work begins.  They are not source-reconciliation defects and must remain
    visible to the planned-row queue.
    """

    return str(test.get("status") or "").casefold() == "planned"


def is_non_executable_source_status(test: JsonObject) -> bool:
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
    explicit_command = handles.get("ctest_command")
    if isinstance(explicit_command, str) and explicit_command.strip():
        return explicit_command
    ctest_filter = handles.get("ctest_filter")
    if isinstance(ctest_filter, str) and ctest_filter.strip():
        return (
            "ctest --test-dir <build-dir> -C Debug -R "
            f'"{ctest_filter}" --output-on-failure'
        )
    ctest_label = handles.get("ctest_label")
    if isinstance(ctest_label, str) and ctest_label.strip():
        return (
            "ctest --test-dir <build-dir> -C Debug -L "
            f'"^{ctest_label}$" --output-on-failure'
        )
    return None


def indexed_lane_assertion_count(index: JsonObject, lane: str) -> int | None:
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


def item_handles(item: JsonObject, compact: bool = False) -> list[str]:
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
        handles.append(f"work_query={compact_prose(item['next_work_query'], 180)}")
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
    if item.get("next_process_package_connection_loss_delete_objects_test"):
        handles.append(
            "process_package_connection_loss_delete_objects_test="
            f"{item['next_process_package_connection_loss_delete_objects_test']}"
        )
    if item.get("next_process_package_connection_loss_delete_objects_ctest_filter"):
        handles.append(
            "process_package_connection_loss_delete_objects_ctest="
            f"{item['next_process_package_connection_loss_delete_objects_ctest_filter']}"
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
    for field, label in (
        (
            "next_process_package_federation_save_restore_test",
            "process_package_federation_save_restore_test",
        ),
        (
            "next_process_package_federation_save_restore_ctest_filter",
            "process_package_federation_save_restore_ctest",
        ),
        (
            "next_process_package_federation_save_restore_failure_test",
            "process_package_federation_save_restore_failure_test",
        ),
        (
            "next_process_package_federation_save_restore_failure_ctest_filter",
            "process_package_federation_save_restore_failure_ctest",
        ),
        (
            "next_process_package_federation_save_restore_abort_test",
            "process_package_federation_save_restore_abort_test",
        ),
        (
            "next_process_package_federation_save_restore_abort_ctest_filter",
            "process_package_federation_save_restore_abort_ctest",
        ),
        (
            "next_process_package_federation_save_restore_status_test",
            "process_package_federation_save_restore_status_test",
        ),
        (
            "next_process_package_federation_save_restore_status_ctest_filter",
            "process_package_federation_save_restore_status_ctest",
        ),
    ):
        if item.get(field):
            handles.append(f"{label}={item[field]}")
    if item.get("next_process_package_catalog_verifier"):
        handles.append(
            "process_package_catalog_verifier="
            f"{item['next_process_package_catalog_verifier']}"
        )
    if item.get("next_process_package_junit_artifact"):
        handles.append(
            "process_package_junit_artifact="
            f"{item['next_process_package_junit_artifact']}"
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


def _issue_search_values(value: JsonValue) -> Iterable[str]:
    """Yield scalar values from one issue record for bounded lookup."""

    if isinstance(value, dict):
        for key, nested in value.items():
            yield str(key)
            yield from _issue_search_values(nested)
    elif isinstance(value, list):
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
) -> JsonObject:
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
    issues = cast(list[JsonObject], raw_issues)
    normalized_query = search_key(query) if query else ""
    clause_query_text = query.strip() if query else ""
    clause_query = bool(re.fullmatch(r"\d+(?:\.\d+)*", clause_query_text))
    exact_id_query = bool(normalized_query) and any(
        search_key(issue.get("id")) == normalized_query for issue in issues
    )
    selected: list[JsonObject] = []
    for issue in issues:
        issue_status = str(issue.get("status") or "unknown")
        if status != "all" and issue_status.casefold() != status.casefold():
            continue
        if normalized_query:
            # A stable issue ID is an exact identifier, not a fuzzy text
            # query. In particular, RL-252 must not also return later issues
            # whose `recurrence_of` field cites RL-252.
            exact_issue_id = search_key(issue.get("id")) == normalized_query
            if exact_id_query:
                if not exact_issue_id:
                    continue
            elif clause_query:
                clause_pattern = re.compile(
                    rf"{re.escape(clause_query_text)}(?:\.\d+)*"
                )
                clause_values = (
                    str(issue.get("exported_clause") or "").strip(),
                    str(issue.get("normative_clause") or "").strip(),
                )
                if not any(
                    clause_pattern.fullmatch(clause_value) is not None
                    for clause_value in clause_values
                ):
                    continue
            else:
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


def _known_cpp_if_value(expression: str) -> bool | None:
    """Return a value only for literal preprocessor conditions we can prove."""

    expression = expression.split("//", 1)[0].split("/*", 1)[0].strip()
    if re.fullmatch(r"0+[uUlL]*", expression):
        return False
    if re.fullmatch(r"1+[uUlL]*", expression):
        return True
    return None


def _mask_disabled_cpp_source(source: str) -> str:
    """Hide declarations inside provably disabled literal-zero branches.

    Keep line endings so derived source pointers retain their original line
    numbers. Unknown macro conditions remain searchable: either branch may
    be active in the project's configured build.
    """

    masked_lines: list[str] = []
    conditional_stack: list[tuple[bool, bool | None]] = []
    disabled = False
    for line in source.splitlines(keepends=True):
        directive = _CPP_CONDITIONAL_DIRECTIVE.match(line)
        if directive is None:
            if disabled:
                masked_lines.append(
                    "".join(
                        character if character in "\r\n" else " " for character in line
                    )
                )
            else:
                masked_lines.append(line)
            continue

        keyword, expression = directive.groups()
        if keyword in {"if", "ifdef", "ifndef"}:
            parent_disabled = disabled
            condition = _known_cpp_if_value(expression) if keyword == "if" else None
            conditional_stack.append((parent_disabled, condition))
            disabled = parent_disabled or condition is False
        elif keyword == "elif" and conditional_stack:
            parent_disabled, prior_condition = conditional_stack[-1]
            condition = _known_cpp_if_value(expression)
            disabled = parent_disabled or prior_condition is True or condition is False
            if prior_condition is False and condition is True:
                conditional_stack[-1] = (parent_disabled, True)
            elif condition is None:
                conditional_stack[-1] = (parent_disabled, None)
        elif keyword == "else" and conditional_stack:
            parent_disabled, prior_condition = conditional_stack[-1]
            disabled = parent_disabled or prior_condition is True
            conditional_stack[-1] = (
                parent_disabled,
                True if prior_condition is False else None,
            )
        elif keyword == "endif":
            if conditional_stack:
                parent_disabled, _ = conditional_stack.pop()
                disabled = parent_disabled
        masked_lines.append(line)
    return "".join(masked_lines)


def load_test_source_locations(
    root: Path,
    *,
    include_health: bool = False,
) -> (
    dict[str, list[TestSourceLocation]]
    | tuple[dict[str, list[TestSourceLocation]], TestSourceHealth]
):
    """Index Catch2 test names to their C++ source locations.

    The test plan remains the traceability source of truth.  This bounded
    derived index only makes the plan actionable by locating the corresponding
    ``TEST_CASE`` declaration, and is intentionally rebuilt per query so it
    cannot become a second, stale plan.
    """

    if not root.is_dir():
        raise ValueError(f"test source root is not a directory: {root}")
    locations: dict[str, list[TestSourceLocation]] = defaultdict(list)
    health: TestSourceHealth = {
        "status": "ok",
        "hint": "",
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
            keyword = directive.group(1)
            if keyword == "endif":
                if conditional_depth == 0:
                    unbalanced_conditionals.append(line_number)
                else:
                    conditional_depth -= 1
            elif keyword in {"if", "ifdef", "ifndef"}:
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
        indexed_source = _mask_disabled_cpp_source(source)
        for match in _CATCH2_TEST_CASE.finditer(indexed_source):
            name = decode_cpp_string(match.group(1))
            line = indexed_source.count("\n", 0, match.start()) + 1
            locations[name].append(
                {
                    "path": relative_path(path),
                    "line": line,
                }
            )
    if health["status"] == "attention":
        hints: list[str] = []
        if health["files_with_control_characters"]:
            hints.append("inspect git diff --check for control-byte damage")
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


def source_location_text(test: JsonObject) -> str:
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
    test: JsonObject,
    locations: Sequence[TestSourceLocation],
) -> list[TestSourceLocation]:
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


def source_location_sort_key(test: JsonObject) -> tuple[str, int, str]:
    """Sort source-backed records by path and numeric declaration line.

    Source locations are rendered as ``path:line`` strings for humans, but
    sorting those strings makes line 51392 appear before line 5461.  Queue
    selection must follow the actual declaration order so the roadmap pointer
    can advance one source case at a time without another repository scan.
    """

    locations = json_object_rows(test.get("source_locations"))
    first = locations[0] if locations else {}
    path = str(first.get("path", ""))
    line = first.get("line")
    return (
        path.casefold(),
        line if isinstance(line, int) else 0,
        str(test.get("test_case") or test.get("id") or ""),
    )


def source_pointer_state(item: JsonObject) -> str:
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


def live_next_action(item: JsonObject) -> str | None:
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
    item: JsonObject,
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
    evidence-complete family, backlog shaping, and external review.

    The classification is derived from the already indexed plan/source join;
    it never searches the Requirements Lab or infers a new requirement.
    """

    # A completed lane remains evidence-complete even when its roadmap family
    # has a different planned companion queued next.  Conversely, a planned
    # row on the family's explicitly owned lane is a planned action even when
    # next_test_role has not been duplicated into the family metadata.
    if lane_state == "complete-pointer":
        return "evidence-complete"
    if lane_state == "planned" or (
        item.get("next_test_role") == "planned" and lane_state is None
    ):
        return "planned"
    if candidate_count:
        return "implementation"
    if unclassified_count:
        return "mapping"
    if actionable_source_drift_count:
        return "source-reconciliation"

    action = str(item.get("next_action") or "").casefold()
    if (
        action.lstrip().startswith("submit ")
        or "submit the existing raw" in action[:240]
    ):
        return "external-review"
    if (
        "no executable c++ implementation slice is currently queued" in action
        or "not a new-case task" in action
    ):
        return "backlog-shaping"
    if item.get("next_source_state") == "exhausted":
        return "evidence-complete"
    if lane_state == "complete-pointer":
        return "evidence-complete"
    if "keep this item open for newly identified evidence only" in action:
        return "evidence-complete"
    if "queue" in action and "exhaust" in action:
        return "evidence-complete"
    return "new-case-needed"


def source_locations_for_test(
    test_case: Any,
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
) -> list[TestSourceLocation]:
    """Resolve a plan entry, including entries grouping cases with semicolons."""

    if not isinstance(test_case, str):
        return []
    candidates = [test_case]
    candidates.extend(part.strip() for part in test_case.split(";") if part.strip())
    resolved: list[TestSourceLocation] = []
    seen: set[tuple[str, int]] = set()
    for candidate in candidates:
        for location in source_locations.get(candidate, []):
            key = (location["path"], location["line"])
            if key in seen:
                continue
            seen.add(key)
            resolved.append(location)
    return resolved


def tests_for_source_path(
    tests: Sequence[JsonObject],
    path_query: str,
) -> list[JsonObject]:
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
            folded_query in str(location.get("path", "")).replace("\\", "/").casefold()
            for location in json_object_rows(test.get("source_locations"))
        )
    ]

    def sort_key(test: JsonObject) -> tuple[str, int, str]:
        locations = json_object_rows(test.get("source_locations"))
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
    tests: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
    source_path_query: str | None = None,
) -> list[JsonObject]:
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
    records: list[JsonObject] = []
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
            cast(
                JsonObject,
                {
                    "test_case": test_case,
                    "source_locations": cast(JsonValue, locations),
                    "source_state": "unplanned-source",
                },
            )
        )
    records.sort(key=source_location_sort_key)
    return records


def unplanned_source_summary(
    tests: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
) -> JsonObject:
    """Return only the global source-queue count and deterministic head.

    Work-selection output should expose the next unplanned declaration when a
    narrower indexed queue is exhausted, but it must not print or retain the
    whole source catalog.  The full bounded queue remains available through
    ``unplanned``.
    """

    queue = unplanned_source_cases(tests, source_locations)
    result: JsonObject = {"count": len(queue)}
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


def load_standard_requirements(
    bundle: JsonObject,
    contract_directory: Path | None = None,
) -> dict[str, JsonObject]:
    requirements: dict[str, JsonObject] = {}
    documents = bundle.get("documents", [])
    if not isinstance(documents, list):
        documents = []
    for document in documents:
        if not isinstance(document, dict):
            continue
        document_id = document.get("document_id")
        if not isinstance(document_id, str) or not document_id.endswith("-2025"):
            continue
        document_requirements = document.get("requirements", [])
        if not isinstance(document_requirements, list):
            continue
        for requirement in document_requirements:
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
                "api_surface_scope": requirement.get("api_surface_scope"),
                "api_surface_note": requirement.get("api_surface_note"),
                "transition_ids": strings(requirement.get("transition_ids")),
                "source": requirement.get("source"),
            }
    # Keep immutable Lab identifiers and exported metadata intact, while
    # applying only an explicit, source-backed Umbra crosswalk when a known
    # extraction defect assigns the wrong canonical subsection. This makes
    # section queries useful without rewriting the pinned corpus bundle.
    directory = contract_directory or DEFAULT_CONTRACT_DIRECTORY
    for path in sorted(directory.glob("*-requirements-contract.json")):
        contract = load_json(path)
        # These are malformed serialized records, so ValueError describes the
        # file content rather than a wrong Python call argument.
        overrides = contract.get("canonical_standard_section_overrides", [])
        if not isinstance(overrides, list):
            raise ValueError(  # noqa: TRY004
                f"{relative_path(path)} canonical_standard_section_overrides must be an array"
            )
        for override in overrides:
            if not isinstance(override, dict):
                raise ValueError(  # noqa: TRY004
                    f"{relative_path(path)} has a non-object canonical section override"
                )
            requirement_id = override.get("requirements_lab_requirement_id")
            document_id = override.get("document_id")
            exported_clause_id = override.get("exported_clause_id")
            canonical_clause_id = override.get("canonical_clause_id")
            standard = (
                requirements.get(requirement_id)
                if isinstance(requirement_id, str)
                else None
            )
            if (
                standard is None
                or standard.get("document_id") != document_id
                or standard.get("clause_id") != exported_clause_id
                or not isinstance(canonical_clause_id, str)
                or not canonical_clause_id.startswith("clause-")
            ):
                raise ValueError(
                    f"{relative_path(path)} has a stale or incomplete canonical section override "
                    f"for {requirement_id!r}"
                )
            previous = standard.get("canonical_section_override")
            if (
                isinstance(previous, dict)
                and previous.get("canonical_clause_id") != canonical_clause_id
            ):
                raise ValueError(
                    f"conflicting canonical section overrides for {requirement_id}"
                )
            standard["exported_clause_id"] = exported_clause_id
            standard["exported_clause"] = standard.get("clause")
            standard["clause_id"] = canonical_clause_id
            standard["clause"] = canonical_clause_id.removeprefix("clause-")
            standard["canonical_section_override"] = {
                key: value
                for key, value in override.items()
                if key != "requirements_lab_requirement_id"
            }
    return requirements


def requirement_gap_inventory(
    standard_requirements: Mapping[str, JsonObject],
    tests: Sequence[JsonObject],
    *,
    index: JsonObject | None = None,
    query: str | None = None,
    document: str | None = None,
    clause: str | None = None,
    coverage_kind: str | None = None,
    family: str | None = None,
    limit: int = 20,
) -> JsonObject:
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
        family_item = next(
            (
                item
                for item in json_object_rows(index.get("items") if index else None)
                if str(item.get("id") or "").casefold() == family_folded
            ),
            None,
        )
        scoped_tests = (
            item_tests(family_item, tests, index=index)
            if family_item is not None
            else []
        )

    covered_ids = {
        requirement_id
        for test in scoped_tests
        for requirement_id in strings(test.get("lab_requirement_ids"))
        if requirement_id in standard_requirements
    }

    def matches(requirement: JsonObject) -> bool:
        document_id = str(requirement.get("document_id") or "")
        clause_id = str(requirement.get("clause_id") or "")
        if document and document.casefold() not in document_id.casefold():
            return False
        if (
            clause
            and clause.casefold() not in clause_id.casefold()
            and clause.casefold() not in str(requirement.get("clause") or "").casefold()
        ):
            return False
        if (
            coverage_kind
            and str(requirement.get("coverage_kind") or "").casefold()
            != coverage_kind.casefold()
        ):
            return False
        if not query:
            return True
        source = requirement.get("source")
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
            source.get("path") if isinstance(source, dict) else None,
        )
        folded = query.casefold()
        return any(
            isinstance(value, str) and folded in value.casefold() for value in values
        )

    filtered = [
        requirement
        for requirement in standard_requirements.values()
        if matches(requirement)
    ]
    uncovered = []
    for requirement in filtered:
        identifier = requirement.get("id")
        if not isinstance(identifier, str) or identifier not in covered_ids:
            uncovered.append(requirement)
    uncovered.sort(
        key=lambda requirement: (
            str(requirement.get("document_id") or ""),
            str(requirement.get("clause_id") or ""),
            int(requirement.get("ordinal") or 0),
            str(requirement.get("id") or ""),
        )
    )

    globally_covered_ids = {
        requirement_id
        for test in tests
        for requirement_id in strings(test.get("lab_requirement_ids"))
        if requirement_id in standard_requirements
    }

    grouped: defaultdict[str, RequirementGapSection] = defaultdict(
        lambda: {
            "standard_section": None,
            "document_id": None,
            "clause_id": None,
            "total_requirement_count": 0,
            "covered_requirement_count": 0,
            "uncovered_requirement_count": 0,
            "globally_covered_requirement_count": 0,
            "globally_uncovered_requirement_count": 0,
            "uncovered_requirement_ids": [],
        }
    )
    for requirement in filtered:
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        identifier = requirement.get("id")
        if (
            not isinstance(document_id, str)
            or not isinstance(clause_id, str)
            or not isinstance(identifier, str)
        ):
            continue
        key = f"{document_id}:{clause_id}"
        row = grouped[key]
        row["standard_section"] = key
        row["document_id"] = document_id
        row["clause_id"] = clause_id
        row["total_requirement_count"] += 1
        if identifier in globally_covered_ids:
            row["globally_covered_requirement_count"] += 1
        else:
            row["globally_uncovered_requirement_count"] += 1
        if identifier in covered_ids:
            row["covered_requirement_count"] += 1
        else:
            row["uncovered_requirement_count"] += 1
            if len(row["uncovered_requirement_ids"]) < 8:
                row["uncovered_requirement_ids"].append(identifier)
    groups = sorted(
        (row for row in grouped.values() if row["uncovered_requirement_count"]),
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

    def requirement_record(requirement: JsonObject) -> JsonObject:
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        identifier = requirement.get("id")
        source = requirement.get("source")
        return {
            "id": identifier,
            "coverage": "uncovered",
            "coverage_scope": "family-local" if family else "global",
            "global_coverage": (
                "covered" if identifier in globally_covered_ids else "uncovered"
            ),
            "document_id": document_id,
            "clause": requirement.get("clause"),
            "clause_id": clause_id,
            "standard_section": (
                f"{document_id}:{clause_id}" if document_id and clause_id else None
            ),
            "title": requirement.get("title"),
            "statement": requirement.get("statement"),
            "kind": requirement.get("kind"),
            "coverage_kind": requirement.get("coverage_kind"),
            "semantic_group_id": requirement.get("semantic_group_id"),
            "api_surface_scope": requirement.get("api_surface_scope"),
            "api_surface_note": requirement.get("api_surface_note"),
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
        isinstance(identifier := requirement.get("id"), str)
        and identifier in covered_ids
        for requirement in filtered
    )
    globally_covered_count = sum(
        isinstance(identifier := requirement.get("id"), str)
        and identifier in globally_covered_ids
        for requirement in filtered
    )
    return {
        "query": query,
        "document": document,
        "clause": clause,
        "coverage_kind": coverage_kind,
        "family": family,
        "coverage_scope": "family-local" if family else "global",
        "total_requirement_count": total_count,
        "covered_requirement_count": covered_count,
        "uncovered_requirement_count": len(uncovered),
        "globally_covered_requirement_count": globally_covered_count,
        "globally_uncovered_requirement_count": total_count - globally_covered_count,
        "coverage_percent": (
            round((covered_count / total_count) * 100, 2) if total_count else 0.0
        ),
        "group_count": len(groups),
        "shown_group_count": len(shown_groups),
        "shown_requirement_count": len(shown_requirements),
        "groups": cast(JsonValue, shown_groups),
        "requirements": [
            requirement_record(requirement) for requirement in shown_requirements
        ],
    }


def cpp_tck_requirement_gap_inventory(
    standard_requirements: Mapping[str, JsonObject],
    catalog: JsonObject,
    tests: Sequence[JsonObject],
    *,
    query: str | None = None,
    document: str | None = None,
    clause: str | None = None,
    coverage_kind: str | None = None,
    limit: int = 20,
) -> JsonObject:
    """Find pinned requirements absent from direct C++ TCK catalog mappings.

    This is intentionally separate from ``gaps``: that command measures the
    Catch2 plan, while this view measures only explicit Lab requirement IDs
    in ``cpp-tck-scenario-catalog.json``. Candidate catalog rows count as
    represented, but are reported separately from promoted rows. Catalog
    subsection disagreements are surfaced separately because a catalog may
    carry a source-backed crosswalk beyond the exported clause field.
    """

    if catalog.get("kind") != "portable-cpp-tck-catalog":
        raise ValueError("catalog is not a portable C++ TCK scenario catalog")
    rows = catalog.get("scenarios")
    if not isinstance(rows, list):
        # Catalog shape failures are invalid file content, not API misuse.
        raise ValueError(  # noqa: TRY004
            "portable C++ TCK catalog scenarios must be an array"
        )

    scenarios: list[JsonObject] = []
    for row in rows:
        if not isinstance(row, dict):
            continue
        scenario = cast(JsonObject, row)
        scenario_id = scenario.get("id")
        if isinstance(scenario_id, str) and scenario_id.startswith("cpp-tck."):
            scenarios.append(scenario)
    mapped_promoted: set[str] = set()
    mapped_candidate: set[str] = set()
    mapped_other: set[str] = set()
    mapping_sections_by_id: defaultdict[str, set[str]] = defaultdict(set)
    invalid_mappings: list[JsonObject] = []
    invalid_mapping_count = 0
    non_lab_mapping_count = 0
    section_disagreement_count_by_requirement: Counter[str] = Counter()
    section_disagreements: list[JsonObject] = []
    direct_mapping_count = 0

    for scenario in scenarios:
        scenario_id = str(scenario.get("id"))
        promotion = str(scenario.get("promotion") or "").casefold()
        mappings = scenario.get("requirement_mappings", [])
        if not isinstance(mappings, list):
            mappings = []
        for mapping in mappings:
            if not isinstance(mapping, dict):
                continue
            requirement_id = mapping.get("requirement_id") or mapping.get(
                "lab_requirement_id"
            )
            section = mapping.get("standard_section")
            if not isinstance(requirement_id, str) or not requirement_id:
                continue
            standard = standard_requirements.get(requirement_id)
            if standard is None:
                # The catalog also carries its separate cpp-1516.* API
                # requirement vocabulary. Those IDs are not Lab requirement
                # IDs and must neither close a Lab gap nor be misreported as
                # broken Lab references.
                if not requirement_id.startswith("cpp-"):
                    invalid_mapping_count += 1
                    if len(invalid_mappings) < 20:
                        invalid_mappings.append(
                            {
                                "scenario_id": scenario_id,
                                "requirement_id": requirement_id,
                                "catalog_section": section,
                                "expected_section": None,
                            }
                        )
                else:
                    non_lab_mapping_count += 1
                continue
            if not isinstance(section, str) or not section:
                invalid_mapping_count += 1
                if len(invalid_mappings) < 20:
                    invalid_mappings.append(
                        {
                            "scenario_id": scenario_id,
                            "requirement_id": requirement_id,
                            "catalog_section": section,
                            "expected_section": (
                                f"{standard.get('document_id')}:{standard.get('clause_id')}"
                            ),
                        }
                    )
                continue
            expected_section = (
                f"{standard.get('document_id')}:{standard.get('clause_id')}"
            )
            if section != expected_section:
                section_disagreement_count_by_requirement[requirement_id] += 1
                section_disagreements.append(
                    {
                        "scenario_id": scenario_id,
                        "requirement_id": requirement_id,
                        "catalog_section": section,
                        "expected_section": expected_section,
                    }
                )
            direct_mapping_count += 1
            mapping_sections_by_id[requirement_id].add(section)
            if promotion == "promoted":
                mapped_promoted.add(requirement_id)
            elif promotion == "candidate":
                mapped_candidate.add(requirement_id)
            else:
                mapped_other.add(requirement_id)

    def requirement_identifier(requirement: JsonObject) -> str | None:
        value = requirement.get("id")
        return value if isinstance(value, str) else None

    def requirement_ordinal(requirement: JsonObject) -> int:
        value = requirement.get("ordinal")
        if value is None:
            return 0
        if isinstance(value, (str, int, float, bool)):
            return int(value)
        raise TypeError("requirement ordinal must be a number or numeric string")

    def matches(requirement: JsonObject) -> bool:
        document_id = str(requirement.get("document_id") or "")
        clause_id = str(requirement.get("clause_id") or "")
        requirement_id = requirement_identifier(requirement) or ""
        if document and document.casefold() not in document_id.casefold():
            return False
        if clause:
            folded_clause = clause.casefold()
            requirement_clause_matches = (
                folded_clause in clause_id.casefold()
                or folded_clause in str(requirement.get("clause") or "").casefold()
            )
            catalog_clause_matches = any(
                folded_clause in section.casefold()
                for section in mapping_sections_by_id.get(requirement_id, set())
            )
            if not requirement_clause_matches and not catalog_clause_matches:
                return False
        if (
            coverage_kind
            and str(requirement.get("coverage_kind") or "").casefold()
            != coverage_kind.casefold()
        ):
            return False
        if not query:
            return True
        source = requirement.get("source")
        source_path = source.get("path") if isinstance(source, dict) else None
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
            source_path,
        )
        folded = query.casefold()
        return any(
            isinstance(value, str) and folded in value.casefold() for value in values
        )

    filtered = [
        requirement
        for requirement in standard_requirements.values()
        if matches(requirement)
    ]
    filtered_ids = {
        requirement_id
        for requirement in filtered
        if (requirement_id := requirement_identifier(requirement)) is not None
    }
    filtered.sort(
        key=lambda requirement: (
            str(requirement.get("document_id") or ""),
            str(requirement.get("clause_id") or ""),
            requirement_ordinal(requirement),
            str(requirement.get("id") or ""),
        )
    )
    mapped_ids = mapped_promoted | mapped_candidate | mapped_other
    uncovered = [
        requirement
        for requirement in filtered
        if requirement_identifier(requirement) not in mapped_ids
    ]

    catch2_cases: defaultdict[str, list[JsonObject]] = defaultdict(list)
    for test in tests:
        test_id = test.get("id")
        if not isinstance(test_id, str):
            continue
        for requirement_id in strings(test.get("lab_requirement_ids")):
            catch2_cases[requirement_id].append(
                {
                    "case_id": test_id,
                    "test_case": test.get("test_case"),
                    "source_location": source_location_text(test),
                }
            )

    groups: defaultdict[str, RequirementGapGroup] = defaultdict(
        lambda: {
            "standard_section": None,
            "total_requirement_count": 0,
            "mapped_requirement_count": 0,
            "promoted_requirement_count": 0,
            "candidate_only_requirement_count": 0,
            "uncovered_requirement_count": 0,
            "uncovered_requirement_ids": [],
        }
    )
    for requirement in filtered:
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        requirement_id = requirement_identifier(requirement)
        if not isinstance(document_id, str) or not isinstance(clause_id, str):
            continue
        section = f"{document_id}:{clause_id}"
        group = groups[section]
        group["standard_section"] = section
        group["total_requirement_count"] += 1
        if requirement_id is not None and requirement_id in mapped_ids:
            group["mapped_requirement_count"] += 1
        else:
            group["uncovered_requirement_count"] += 1
            if (
                requirement_id is not None
                and len(group["uncovered_requirement_ids"]) < 8
            ):
                group["uncovered_requirement_ids"].append(requirement_id)
        if requirement_id is not None and requirement_id in mapped_promoted:
            group["promoted_requirement_count"] += 1
        elif requirement_id is not None and requirement_id in mapped_candidate:
            group["candidate_only_requirement_count"] += 1
    shown_groups = sorted(
        (group for group in groups.values() if group["uncovered_requirement_count"]),
        key=lambda group: (
            -group["uncovered_requirement_count"],
            str(group.get("standard_section") or ""),
        ),
    )
    requested_limit = max(limit, 0)
    shown_requirements = (
        uncovered if requested_limit == 0 else uncovered[:requested_limit]
    )
    group_limit = 20 if requested_limit == 0 else max(1, min(20, requested_limit))

    requirement_records: list[JsonObject] = []
    for requirement in shown_requirements:
        requirement_id = requirement_identifier(requirement)
        case_rows = (
            catch2_cases.get(requirement_id, []) if requirement_id is not None else []
        )
        document_id = requirement.get("document_id")
        clause_id = requirement.get("clause_id")
        requirement_records.append(
            {
                "id": requirement_id,
                "coverage": "unmapped-in-cpp-tck-catalog",
                "document_id": document_id,
                "clause": requirement.get("clause"),
                "clause_id": clause_id,
                "standard_section": f"{document_id}:{clause_id}"
                if document_id and clause_id
                else None,
                "title": requirement.get("title"),
                "statement": requirement.get("statement"),
                "source": requirement.get("source"),
                "catch2_case_count": len(case_rows),
                "catch2_cases": case_rows[:3],
                "catch2_requirement_query": (
                    f"python tools/query_rti_work.py requirement {requirement_id} "
                    "--summary --compact"
                    if isinstance(requirement_id, str)
                    else None
                ),
            }
        )

    total_count = len(filtered)
    mapped_count = sum(requirement.get("id") in mapped_ids for requirement in filtered)
    return {
        "query": query,
        "document": document,
        "clause": clause,
        "coverage_kind": coverage_kind,
        "coverage_scope": "cpp-tck-direct-requirement-mappings",
        "scenario_count": len(scenarios),
        "direct_mapping_count": direct_mapping_count,
        "total_requirement_count": total_count,
        "mapped_requirement_count": mapped_count,
        "promoted_requirement_count": sum(
            requirement.get("id") in mapped_promoted for requirement in filtered
        ),
        "candidate_only_requirement_count": sum(
            requirement.get("id") in mapped_candidate
            and requirement.get("id") not in mapped_promoted
            for requirement in filtered
        ),
        "uncovered_requirement_count": len(uncovered),
        "coverage_percent": round((mapped_count / total_count) * 100, 2)
        if total_count
        else 0.0,
        "non_lab_mapping_count": non_lab_mapping_count,
        "invalid_mapping_count": invalid_mapping_count,
        "invalid_mappings": invalid_mappings,
        "section_disagreement_count": sum(
            section_disagreement_count_by_requirement.get(requirement_id, 0)
            for requirement_id in filtered_ids
        ),
        "section_disagreements": [
            finding
            for finding in section_disagreements
            if finding.get("requirement_id") in filtered_ids
        ][:20],
        "group_count": len(shown_groups),
        "shown_group_count": min(len(shown_groups), group_limit),
        "shown_requirement_count": len(requirement_records),
        "groups": cast(JsonValue, shown_groups[:group_limit]),
        "requirements": requirement_records,
    }


def requirement_gap_summary_data(result: JsonObject) -> JsonObject:
    """Return the bounded gap-card view without full requirement records.

    Subsection aggregates are enough to select the next exact query.  Keeping
    the requirement records opt-in is important for JSON consumers too:
    ``gaps --summary --limit 0`` must not expand into a context-sized dump.
    """

    summary = dict(result)
    summary["requirements"] = []
    summary["shown_requirement_count"] = 0
    return summary


def cpp_tck_requirement_gap_summary_data(result: JsonObject) -> JsonObject:
    """Return TCK subsection aggregates without expanding requirement records."""

    summary = dict(result)
    summary["requirements"] = []
    summary["invalid_mappings"] = []
    summary["section_disagreements"] = []
    summary["shown_requirement_count"] = 0
    return summary


def requirement_gap_lookup_summary_data(
    result: JsonObject,
    *,
    preview_limit: int = 8,
) -> JsonObject:
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
    preview: list[JsonObject] = []
    for record in selected:
        if not isinstance(record, dict):
            continue
        record_object = cast(JsonObject, record)
        bounded_record = dict(record_object)
        bounded_record["title"] = compact_prose(record_object.get("title"), 160)
        bounded_record["statement"] = compact_prose(record_object.get("statement"), 360)
        preview.append(bounded_record)
    summary["requirements"] = preview
    summary["shown_requirement_count"] = len(preview)
    summary["requirement_preview_limit"] = bounded_limit
    summary["requirement_preview_truncated"] = len(records) > len(selected)
    return summary


def family_gap_preview(
    index: JsonObject,
    tests: Sequence[JsonObject],
    standard_requirements: Mapping[str, JsonObject] | None,
    family: str | None,
) -> JsonObject | None:
    """Return one bounded coverage-inventory pointer for a roadmap family.

    Family queue rows deliberately avoid printing the full 2025 gap inventory.
    When a family is explicitly marked ``new-case-needed``, this helper picks
    one globally uncovered inventory row from a subsection already represented
    by that family's C++ tests. The row is not an implementation
    recommendation. It avoids surfacing requirements already covered
    elsewhere and corpus rows explicitly identified as non-callable table
    structure. It only joins the already-loaded plan and pinned corpus.
    """

    if not isinstance(standard_requirements, Mapping) or not isinstance(family, str):
        return None
    if not family.strip():
        return None
    gaps = requirement_gap_inventory(
        standard_requirements,
        tests,
        index=index,
        family=family,
        # The family prompt should lead with its largest uncovered subsection,
        # not the first generic requirement in corpus order. Keeping the
        # complete filtered inventory here is bounded to the pinned corpus
        # and lets us select an exact candidate from that subsection.
        limit=0,
    )
    records = gaps.get("requirements")
    if not isinstance(records, list) or not records:
        return {
            "family": family,
            "total_requirement_count": gaps.get("total_requirement_count", 0),
            "covered_requirement_count": gaps.get("covered_requirement_count", 0),
            "uncovered_requirement_count": gaps.get("uncovered_requirement_count", 0),
            "global_uncovered_candidate_count": 0,
            "coverage_percent": gaps.get("coverage_percent", 0.0),
            "requirement": None,
        }
    roadmap_items = index.get("items", [])
    if not isinstance(roadmap_items, list):
        roadmap_items = []
    family_entry = next(
        (
            item
            for item in roadmap_items
            if isinstance(item, dict)
            and str(item.get("id") or "").casefold() == family.casefold()
        ),
        None,
    )
    family_tests = (
        item_tests(family_entry, tests, index=index)
        if isinstance(family_entry, dict)
        else []
    )
    family_pairs: set[tuple[str, str]] = set()
    for test in family_tests:
        for row in requirement_section_mapping_rows(test):
            requirement_id = row.get("lab_requirement_id")
            section = row.get("standard_section")
            if (
                isinstance(requirement_id, str)
                and requirement_id
                and isinstance(section, str)
                and section
            ):
                family_pairs.add((requirement_id, section))
    family_section_counts: Counter[str] = Counter(
        str(section) for _, section in family_pairs if section
    )
    family_sections = set(family_section_counts)
    globally_covered_ids = {
        requirement_id
        for test in tests
        for requirement_id in strings(test.get("lab_requirement_ids"))
        if requirement_id in standard_requirements
    }
    globally_uncovered_ids = set(standard_requirements).difference(globally_covered_ids)
    global_candidates = [
        record
        for record in records
        if isinstance(record, dict)
        and record.get("id") in globally_uncovered_ids
        and record.get("standard_section") in family_sections
    ]
    # This RTI roadmap family needs callable 1516.1 requirements. The shared
    # corpus also contains 1516.2 OMT requirements; they remain visible in the
    # inventory, but must not be promoted as the next RTI implementation
    # handoff merely because an FOM-related case represents that subsection.
    out_of_scope_candidates = [
        candidate
        for candidate in global_candidates
        if str(candidate.get("document_id") or "").casefold() != "hla-1516.1-2025"
    ]
    implementation_candidates = [
        candidate
        for candidate in global_candidates
        if str(candidate.get("document_id") or "").casefold() == "hla-1516.1-2025"
    ]
    # The pinned corpus explicitly labels these table-shape entries as
    # cross-cutting and says the table structure is not a callable API
    # mapping. Keep them in `gaps` and the total inventory, but do not present
    # them as a suggested Catch2 implementation task.
    inventory_only_candidates = [
        candidate
        for candidate in implementation_candidates
        if str(candidate.get("api_surface_scope") or "").casefold() == "cross-cutting"
        and str(candidate.get("api_surface_note") or "").strip().casefold()
        == "table structure is not itself a callable api mapping."
    ]
    preview_candidates = [
        candidate
        for candidate in implementation_candidates
        if candidate not in inventory_only_candidates
    ]
    candidate_sections = {
        str(candidate.get("standard_section"))
        for candidate in preview_candidates
        if candidate.get("standard_section")
    }
    selected_section = min(
        candidate_sections,
        key=lambda section: (
            -sum(
                candidate.get("standard_section") == section
                for candidate in preview_candidates
            ),
            -family_section_counts.get(section, 0),
            section,
        ),
        default=None,
    )
    record = next(
        (
            candidate
            for candidate in preview_candidates
            if candidate.get("standard_section") == selected_section
        ),
        None,
    )
    if not isinstance(record, dict):
        return {
            "family": family,
            "total_requirement_count": gaps.get("total_requirement_count", 0),
            "covered_requirement_count": gaps.get("covered_requirement_count", 0),
            "uncovered_requirement_count": gaps.get("uncovered_requirement_count", 0),
            "global_uncovered_candidate_count": len(global_candidates),
            "out_of_scope_candidate_count": len(out_of_scope_candidates),
            "preview_candidate_count": len(preview_candidates),
            "inventory_only_gap_count": len(inventory_only_candidates),
            "family_mapped_requirement_count": len(
                {requirement_id for requirement_id, _ in family_pairs}
            ),
            "family_section_count": len(family_sections),
            "coverage_percent": gaps.get("coverage_percent", 0.0),
            "requirement": None,
        }
    record = cast(JsonObject, record)
    requirement_id = record.get("id")
    standard_section = record.get("standard_section")
    clause_id = record.get("clause_id")
    return {
        "family": family,
        "total_requirement_count": gaps.get("total_requirement_count", 0),
        "covered_requirement_count": gaps.get("covered_requirement_count", 0),
        "uncovered_requirement_count": gaps.get("uncovered_requirement_count", 0),
        "global_uncovered_candidate_count": len(global_candidates),
        "out_of_scope_candidate_count": len(out_of_scope_candidates),
        "preview_candidate_count": len(preview_candidates),
        "inventory_only_gap_count": len(inventory_only_candidates),
        "family_mapped_requirement_count": len(
            {requirement_id for requirement_id, _ in family_pairs}
        ),
        "family_section_count": len(family_sections),
        "coverage_percent": gaps.get("coverage_percent", 0.0),
        "selection_basis": (
            "largest globally uncovered IEEE 1516.1-2025 requirement gap within a subsection already mapped by this family; inventory only"
        ),
        "requirement": {
            "id": requirement_id,
            "document_id": record.get("document_id"),
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


def load_contract_links(directory: Path) -> dict[str, list[JsonObject]]:
    links: dict[str, list[JsonObject]] = defaultdict(list)
    for path in sorted(directory.glob("*-requirements-contract.json")):
        try:
            contract = load_json(path)
        except ValueError:
            continue
        requirements = contract.get("requirements", [])
        if not isinstance(requirements, list):
            continue
        for requirement in requirements:
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


def load_contract_test_candidates(directory: Path) -> dict[str, list[JsonObject]]:
    """Index exact test references exported by local Requirements-Lab contracts.

    The normal plan remains authoritative: this reverse index never assigns a
    requirement or changes a Catch2 row.  It only lets the bounded ``unmapped``
    query show a contributor which existing contract records already name the
    exact test, including the candidate Lab ID and clause.
    """

    candidates: defaultdict[str, list[JsonObject]] = defaultdict(list)
    seen: defaultdict[str, set[tuple[str, ...]]] = defaultdict(set)
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
                identity = tuple(
                    json.dumps(record[field], sort_keys=True)
                    for field in (
                        "contract",
                        "contract_requirement_id",
                        "lab_requirement_id",
                        "clause_id",
                        "source_path",
                    )
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
    test: JsonObject,
    candidates: Mapping[str, Sequence[JsonObject]],
) -> list[JsonObject]:
    """Return exact contract references for one current plan/source row."""

    test_case = test.get("test_case")
    if not isinstance(test_case, str) or not test_case.strip():
        return []
    raw_locations = test.get("source_locations")
    source_paths: set[str] = set()
    if isinstance(raw_locations, list):
        for raw_location in raw_locations:
            if not isinstance(raw_location, dict):
                continue
            location_path = raw_location.get("path")
            if isinstance(location_path, str) and location_path:
                source_paths.add(location_path.replace("\\", "/").strip())
    keys = [test_case]
    keys.extend(f"{path}::{test_case}" for path in sorted(source_paths))
    result: list[JsonObject] = []
    seen: set[tuple[str | None, ...]] = set()
    for key in keys:
        for candidate in candidates.get(key, []):
            identity = tuple(
                value if isinstance(value, str) else None
                for value in (
                    candidate.get("contract"),
                    candidate.get("contract_requirement_id"),
                    candidate.get("lab_requirement_id"),
                    candidate.get("clause_id"),
                    candidate.get("source_path"),
                )
            )
            if identity in seen:
                continue
            seen.add(identity)
            candidate_copy = cast(JsonObject, dict(candidate))
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
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
    *,
    query: str | None = None,
    limit: int = 20,
) -> JsonObject:
    """Report stale local contract-to-Catch2 selectors in one bounded query.

    The native C++ source index is the only source of current selectors.  A
    ``cpp/tests/...::Title`` reference is clean only when both path and title
    still match. References outside the indexed C++ test tree (including Java
    TCK source paths) are counted as external to this native-only guard.
    This is intentionally read-only and does not load or resynchronize the
    Requirements Lab export.
    """

    folded_query = (
        query.casefold().strip() if isinstance(query, str) and query.strip() else None
    )
    source_keys = {
        (str(location.get("path") or "").replace("\\", "/"), location.get("line"))
        for locations in source_locations.values()
        for location in locations
        if isinstance(location, dict) and location.get("path")
    }
    findings: list[JsonObject] = []
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
            reference_path = reference.replace("\\", "/").strip()
            if "/" in reference_path and not reference_path.startswith("cpp/tests/"):
                external_reference_count += 1
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
    return cast(
        JsonObject,
        {
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
        },
    )


def requirement_view(
    lab_id: str,
    standard_requirements: Mapping[str, JsonObject],
    contract_links: Mapping[str, Sequence[JsonObject]],
) -> JsonObject:
    standard = standard_requirements.get(lab_id)
    value: JsonObject = {
        "lab_requirement_id": lab_id,
        "standard": standard,
        "contracts": contract_links.get(lab_id, []),
    }
    if standard is None:
        value["unresolved"] = True
    return value


def mapped_test(
    test: JsonObject,
    standard_requirements: Mapping[str, JsonObject],
    contract_links: Mapping[str, Sequence[JsonObject]],
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
) -> JsonObject:
    requirement_ids = strings(test.get("selected_requirements_lab_requirement_ids"))
    requirements = [
        requirement_view(identifier, standard_requirements, contract_links)
        for identifier in requirement_ids
    ]
    mapping_view = {
        "requirements": requirements,
        "requirement_section_mapping_overrides": test.get(
            "requirement_section_mapping_overrides"
        )
        or [],
    }
    requirement_mappings = requirement_section_mapping_rows(mapping_view)
    clauses = {
        section
        for row in requirement_mappings
        if isinstance(section := row.get("standard_section"), str) and section
    }
    # Keep the two useful views together: ``standard_clauses`` is the exact
    # requirement-to-subsection join, while ``standard_sections`` also keeps
    # any additional clause explicitly declared on the test row. This lets a
    # case card show its complete standards slice without implying that every
    # listed clause has a direct Requirements-Lab requirement pair.
    requirement_clauses = sorted(clauses)
    explicit_sections = {
        section for section in strings(test.get("standard_sections")) if ":" in section
    }
    all_sections = sorted(set(requirement_clauses) | explicit_sections)
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
        "ctest_target": test.get("ctest_target"),
        "ctest_filter": test.get("ctest_filter"),
        "source_target": test.get("source_target"),
        "primary_lane": test.get("primary_lane"),
        "focus_lane": test.get("focus_lane"),
        "requirements_lab_mapping_id": test.get("requirements_lab_mapping_id"),
        "requirements_lab_api_surface_status": test.get(
            "requirements_lab_api_surface_status"
        ),
        "replacement_plan_ids": strings(test.get("replacement_plan_ids")),
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
        "requirement_section_mapping_overrides": test.get(
            "requirement_section_mapping_overrides"
        )
        or [],
        "lab_requirement_ids": requirement_ids,
        "standard_clauses": requirement_clauses,
        "standard_sections": all_sections,
        "source_locations": cast(JsonValue, locations),
        "source_state": "located" if locations else "unlocated",
        "unresolved_requirement_ids": [
            requirement["lab_requirement_id"]
            for requirement in requirements
            if requirement.get("unresolved")
        ],
    }


def all_mapped_tests(
    plan: JsonObject,
    standard_requirements: Mapping[str, JsonObject],
    contract_links: Mapping[str, Sequence[JsonObject]],
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
) -> list[JsonObject]:
    plan_tests = plan.get("tests", [])
    if not isinstance(plan_tests, list):
        return []
    return [
        mapped_test(
            cast(JsonObject, test),
            standard_requirements,
            contract_links,
            source_locations,
        )
        for test in plan_tests
        if isinstance(test, dict)
    ]


def roadmap_checklist(path: Path) -> list[JsonObject]:
    entries: list[JsonObject] = []
    section = ""
    heading = re.compile(r"^#{1,6}\s+(.+?)\s*$")
    checkbox = re.compile(r"^-\s+\[([ xX])\]\s+(.+?)\s*$")
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), 1
    ):
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
                    "status": "complete"
                    if checkbox_match.group(1).lower() == "x"
                    else "open",
                    "title": checkbox_match.group(2),
                }
            )
    return entries


class ImplementationPlanSection(TypedDict):
    line: int
    level: int
    title: str
    path: str


def implementation_plan_sections(path: Path) -> list[ImplementationPlanSection]:
    """Return the stable section outline without loading the plan prose.

    The breadcrumb is derived from headings only.  It gives a filtered plan
    query enough context to open the exact section without printing the full
    implementation-plan prose into a work-selection context.
    """

    sections: list[ImplementationPlanSection] = []
    heading = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
    parents: list[tuple[int, str]] = []
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), 1
    ):
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
    sections: Sequence[ImplementationPlanSection],
    query: str | None,
    limit: int,
) -> tuple[list[ImplementationPlanSection], list[ImplementationPlanSection]]:
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
    index: JsonObject,
    tests: Sequence[JsonObject],
    roadmap_entries: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]] | None = None,
) -> JsonObject:
    items = json_object_rows(index.get("items"))
    tag_counts = Counter(tag for test in tests for tag in strings(test.get("tags")))
    enriched_items: list[JsonObject] = []
    for item in items:
        tags = strings(item.get("query_tags"))
        tags.extend(strings(item.get("focused_lane_tags")))
        pointer = next_test_pointer(item, tests)
        matching_tests = item_tests(item, tests, index=index)
        enriched_items.append(
            cast(
                JsonObject,
                {
                    **item,
                    "matching_test_counts": {
                        tag: tag_counts.get(tag, 0) for tag in tags
                    },
                    # A roadmap item usually carries several overlapping tags.
                    # Keep the per-tag counts for discovery, but also publish the
                    # number of distinct Catch2 cases so compact status output
                    # cannot look larger merely because a case has many tags.
                    "matching_test_count": len(matching_tests),
                    "next_test_pointer": pointer,
                    "live_current_focus": live_item_focus(item, tests, index=index),
                    "live_mapping_counts": live_item_mapping_counts(
                        item, tests, index=index
                    ),
                },
            )
        )
    mapping = index.get("mapping")
    current_counts = (
        mapping.get("current_counts", {}) if isinstance(mapping, dict) else {}
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
            not test.get("requirements") and traceability_state(test) == "unclassified"
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
                for location in json_object_rows(test.get("source_locations"))
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
                test for test in tests if lane_name in strings(test.get("tags"))
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
    latest_value = (
        mapping.get("latest_completed_slice") if isinstance(mapping, dict) else None
    )
    latest_completed_slice = (
        cast(JsonObject, latest_value) if isinstance(latest_value, dict) else None
    )
    roadmap_counts = Counter(
        status
        for entry in roadmap_entries
        if isinstance(status := entry.get("status"), str)
    )
    return cast(
        JsonObject,
        {
            "roadmap_items": enriched_items,
            "roadmap_checklist": roadmap_entries,
            "roadmap_counts": dict(roadmap_counts),
            "test_count": len(tests),
            "mapping_counts": current_counts
            if isinstance(current_counts, dict)
            else {},
            "derived_mapping_counts": derived_counts,
            "global_source_queue": global_source_queue,
            "latest_completed_slice": latest_completed_slice,
        },
    )


def item_tests(
    item: JsonObject,
    tests: Sequence[JsonObject],
    *,
    index: JsonObject | None = None,
) -> list[JsonObject]:
    """Return tests belonging to an indexed roadmap item through its handles.

    Broad ``query_tags`` and ``focused_lane_tags`` remain the normal family
    taxonomy.  The indexed lane-owner table and a family's ``next_lane`` are
    also authoritative relationships, however; omitting them made family
    counts disagree with ``roadmap_links_for_test`` and its gap preview.  Add
    those exact handles here so summary, gap, and reverse-trace views select
    the same distinct Catch2 rows.
    """

    tags = set(strings(item.get("query_tags")))
    tags.update(strings(item.get("focused_lane_tags")))
    next_lane = item.get("next_lane")
    if isinstance(next_lane, str) and next_lane.strip():
        tags.add(next_lane)
    mapping = index.get("mapping") if index is not None else item.get("mapping")
    lane_owners = mapping.get("lane_owners") if isinstance(mapping, dict) else None
    if isinstance(lane_owners, dict):
        tags.update(
            lane
            for lane, owner in lane_owners.items()
            if isinstance(lane, str) and owner == item.get("id") and lane.strip()
        )
    if not tags:
        return []
    return [test for test in tests if tags.intersection(strings(test.get("tags")))]


def planned_tests_for_item(
    item: JsonObject,
    tests: Sequence[JsonObject],
    *,
    index: JsonObject,
) -> list[JsonObject]:
    """Return planned rows owned by this family, not broad-tag neighbors.

    Catch2 plan tags intentionally support cross-cutting searches, so a planned
    row may overlap several broad families.  When its primary lane has one
    explicit roadmap owner, count and queue that planned row only under that
    owner.  Rows without a lane-owner decision retain the legacy tag-based
    relationship until they are classified.
    """

    mapping = index.get("mapping")
    lane_owners = mapping.get("lane_owners") if isinstance(mapping, dict) else None
    family_id = item.get("id")
    planned: list[JsonObject] = []
    for test in item_tests(item, tests, index=index):
        if not is_planned_test(test):
            continue
        test_lane = test.get("primary_lane")
        lane_owner = (
            lane_owners.get(test_lane)
            if isinstance(lane_owners, dict) and isinstance(test_lane, str)
            else None
        )
        if (
            isinstance(family_id, str)
            and isinstance(lane_owner, str)
            and lane_owner.casefold() != family_id.casefold()
        ):
            continue
        planned.append(test)
    return planned


def test_lane_identifiers(test: JsonObject) -> list[str]:
    """Return every exact lane handle published by one Catch2 plan row.

    Catch2 tags remain the canonical executable lane handles, but plan rows
    also publish ``primary_lane``/``focus_lane`` metadata for roadmap joins.
    Treating those fields as query aliases keeps ``focus`` and ``check`` from
    silently missing a row when a lane is recorded in the plan before its
    tag is copied into the taxonomy list.
    """

    identifiers = strings(test.get("tags"))
    for field in ("primary_lane", "focus_lane", "next_lane"):
        value = test.get(field)
        if isinstance(value, str) and value.strip() and value not in identifiers:
            identifiers.append(value)
    return identifiers


def test_matches_lane(test: JsonObject, lane: str) -> bool:
    """Match an exact lane against tags or published plan-row lane aliases."""

    folded_lane = lane.casefold()
    return any(
        str(identifier).casefold() == folded_lane
        for identifier in test_lane_identifiers(test)
    )


def scoped_plan_tests(
    index: JsonObject,
    tests: Sequence[JsonObject],
    *,
    lane: str | None = None,
    family: str | None = None,
) -> tuple[JsonObject | None, list[JsonObject]]:
    """Resolve one optional exact lane or roadmap-family scope.

    Lane and family selectors are intentionally separate handles.  Returning
    the family item alongside the rows lets coverage/check render the owner
    without repeating a broad roadmap lookup.
    """

    if family is not None:
        folded_family = family.casefold()
        raw_items = index.get("items")
        family_item = (
            next(
                (
                    cast(JsonObject, item)
                    for item in raw_items
                    if isinstance(raw_items, list)
                    and isinstance(item, dict)
                    and str(item.get("id") or "").casefold() == folded_family
                ),
                None,
            )
            if isinstance(raw_items, list)
            else None
        )
        if family_item is None:
            return None, []
        return family_item, item_tests(family_item, tests, index=index)
    if lane is not None:
        return None, [test for test in tests if test_matches_lane(test, lane)]
    return None, tests


def live_item_focus(
    item: JsonObject,
    tests: Sequence[JsonObject],
    *,
    index: JsonObject | None = None,
) -> str | None:
    """Return a compact, derived focus card instead of stale roadmap prose.

    The long ``current_focus`` field in the roadmap is a human-maintained
    narrative and can lag the indexed plan after a slice is added or repaired.
    Status output should therefore expose counts derived from the same plan it
    is querying, even for families that do not carry narrative focus prose;
    the archival narrative remains available in the JSON index.
    """

    matching = item_tests(item, tests, index=index)
    mapped = sum(bool(test.get("requirements")) for test in matching)
    explicit = sum(
        not test.get("requirements")
        and traceability_state(test) == "explicit-disposition"
        for test in matching
    )
    unclassified = sum(
        not test.get("requirements") and traceability_state(test) == "unclassified"
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
    item: JsonObject,
    tests: Sequence[JsonObject],
    *,
    index: JsonObject | None = None,
) -> LiveItemMappingCounts:
    """Return compact, plan-derived mapping counts for one roadmap family.

    The roadmap's tag sets overlap by design.  These counts therefore use the
    distinct Catch2 rows selected by :func:`item_tests`, and separate an
    intentional no-standalone-surface disposition from a row that still needs
    a requirements decision.  Keeping this structured beside the human focus
    string lets status consumers make a bounded decision without a second
    ``test``/``trace`` query.
    """

    matching = item_tests(item, tests, index=index)
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
        if not test.get("requirements") and traceability_state(test) == "unclassified"
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
    requirement_section_pairs: set[tuple[str, str | None]] = set()
    requirement_ids: set[str] = set()
    for row in requirement_section_rows:
        requirement_id = row.get("lab_requirement_id")
        standard_section = row.get("standard_section")
        if isinstance(requirement_id, str):
            requirement_ids.add(requirement_id)
            requirement_section_pairs.add(
                (
                    requirement_id,
                    standard_section if isinstance(standard_section, str) else None,
                )
            )
    resolved_requirement_section_pairs = {
        pair for pair in requirement_section_pairs if pair[1] is not None
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
    sorted_requirement_section_pairs = sorted(
        (
            requirement_id,
            standard_section,
        )
        for requirement_id, standard_section in resolved_requirement_section_pairs
        if standard_section is not None
    )
    assertion_count = sum(
        value
        for test in matching
        for value in [test.get("assertions")]
        if isinstance(value, int)
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
        "assertion_count": assertion_count,
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


def item_matches_lane(item: JsonObject, lane: str) -> bool:
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
    index: JsonObject,
    test: JsonObject,
    limit: int = 8,
) -> list[JsonObject]:
    """Return bounded roadmap-family links for one mapped Catch2 case.

    An optional plan-row ``primary_lane`` is ranked first, followed by the
    explicit ``mapping.lane_owners`` relation, exact
    ``next_lane``/``focused_lane_tags`` handles, and finally broad tag overlap.
    This keeps a trace result pointed at the family that owns a focused slice
    while retaining broader roadmap relationships for context.
    """

    test_tags = set(test_lane_identifiers(test))
    primary_lane = str(test.get("primary_lane") or "").strip()
    candidates: list[tuple[tuple[int, int, int, str], JsonObject]] = []
    mapping = index.get("mapping")
    lane_owners = mapping.get("lane_owners") if isinstance(mapping, dict) else None
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    raw_items = index.get("items")
    items = raw_items if isinstance(raw_items, list) else []
    for raw_item in items:
        item = raw_item
        if not isinstance(item, dict):
            continue
        item = cast(JsonObject, item)
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
        priority = item.get("priority")
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
                    priority if isinstance(priority, int) else 999,
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


def primary_roadmap_link(roadmap_items: Sequence[JsonObject]) -> JsonObject | None:
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


def next_test_pointer(item: JsonObject, tests: Sequence[JsonObject]) -> JsonObject:
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
        status.startswith(("implemented", "verified")) for status in status_counts
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
    elif any(
        status.startswith(("implemented", "verified")) for status in status_counts
    ):
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
        for location in json_object_rows(test.get("source_locations"))
    ]
    pair_rows: list[JsonObject] = []
    seen_pairs: set[tuple[str, str | None]] = set()
    for test in matches:
        for row in requirement_section_mapping_rows(test):
            requirement_id = row.get("lab_requirement_id")
            if not isinstance(requirement_id, str) or not requirement_id:
                continue
            standard_section = row.get("standard_section")
            pair = (
                requirement_id,
                standard_section if isinstance(standard_section, str) else None,
            )
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
    unique_plan_ids = sorted(
        {
            str(test.get("id"))
            for test in matches
            if isinstance(test.get("id"), str) and test.get("id")
        },
        key=str.casefold,
    )
    # Keep the exact next-test pointer self-contained.  A caller should not
    # have to translate a roadmap title into a plan id before it can open the
    # case/trace/matrix cards that carry the direct requirement-to-subsection
    # joins.  Prefer a unique plan id; when a title deliberately resolves to
    # several plan rows, retain the exact title so the ambiguity remains
    # visible to the normal ``case`` command.
    case_query = unique_plan_ids[0] if len(unique_plan_ids) == 1 else query
    result = {
        "query": query,
        "case_query": case_query,
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
        "traceability_states": sorted({traceability_state(test) for test in matches}),
        "source_locations": source_locations[:8],
    }
    if isinstance(case_query, str) and case_query:
        result["case_command"] = (
            f'python tools/query_rti_work.py case "{case_query}" --summary --compact'
        )
        result["trace_command"] = (
            f'python tools/query_rti_work.py trace "{case_query}" --summary --compact'
        )
        result["matrix_requirement_command"] = (
            f'python tools/query_rti_work.py matrix "{case_query}" '
            "--group-by requirement --summary --compact"
        )
        result["matrix_section_command"] = (
            f'python tools/query_rti_work.py matrix "{case_query}" '
            "--group-by section --summary --compact"
        )
    if len(source_locations) > 8:
        result["source_locations_remaining"] = len(source_locations) - 8
    if len(pair_rows) > 12:
        result["requirement_section_mappings_remaining"] = len(pair_rows) - 12
    return result


def index_warnings(
    index_result: JsonObject,
    lane: str | None = None,
) -> list[str]:
    """Return bounded warnings for open items whose test handle is complete."""

    warnings: list[str] = []
    for item in json_object_rows(index_result.get("roadmap_items")):
        if item.get("status") != "open":
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


def roadmap_item_summary(item: JsonObject) -> JsonObject:
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
    item: JsonObject,
    tests: Sequence[JsonObject] | None = None,
    *,
    index: JsonObject | None = None,
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
        for test in item_tests(item, tests, index=index):
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
    index: JsonObject,
    tests: Sequence[JsonObject],
    *,
    query: str | None = None,
    status: str = "open",
    limit: int = 12,
) -> JsonObject:
    """Return a bounded, live roadmap-family search result.

    The roadmap index is the discovery surface; Catch2 rows remain the source
    of truth for live mapping counts.  Keeping this join in one command means
    a contributor can search family ids, titles, tags, next actions, requirement
    ids, canonical 2025 subsections, or official C++ API surfaces without
    opening the roadmap prose or enumerating every Catch2 lane.
    """

    raw_items = index.get("items")
    items = (
        [cast(JsonObject, item) for item in raw_items if isinstance(item, dict)]
        if isinstance(raw_items, list)
        else []
    )
    folded_query = search_key(query) if isinstance(query, str) and query.strip() else ""
    exact_index_ids: set[str] = set()
    active_handoff = index.get("active_handoff")
    active_handoff_family: str | None = None
    if isinstance(active_handoff, dict) and folded_query:
        active_identity_values = strings(
            active_handoff.get(field) for field in ("id", "family", "lane")
        )
        if any(folded_query == search_key(value) for value in active_identity_values):
            family_value = active_handoff.get("family")
            if isinstance(family_value, str) and family_value:
                active_handoff_family = family_value
    if folded_query:
        # An indexed family id is a stable, unambiguous handle.  Prefer it
        # over exact matches in overlapping query-tag/current-focus fields;
        # otherwise a lookup such as ``roadmap transport-and-conformance``
        # can expand into every family that carries that cross-cutting tag.
        exact_family_ids = {
            str(item.get("id"))
            for item in items
            if item.get("id") and search_key(item.get("id")) == folded_query
        }
        if exact_family_ids:
            exact_index_ids = exact_family_ids
        else:
            # An exact lane owner is stronger than a shared taxonomy tag.  A
            # focused plan row can carry ``process-boundary`` (or another
            # broad tag) that appears under several roadmap families; the
            # explicit owner map is the deterministic join for the one lane
            # the contributor actually named.
            mapping = index.get("mapping")
            lane_owners = (
                mapping.get("lane_owners", {}) if isinstance(mapping, dict) else {}
            )
            exact_lane_owner = (
                next(
                    (
                        owner
                        for lane, owner in lane_owners.items()
                        if search_key(lane) == folded_query
                        and isinstance(owner, str)
                        and owner
                    ),
                    None,
                )
                if isinstance(lane_owners, dict)
                else None
            )
            exact_index_ids = (
                {exact_lane_owner}
                if exact_lane_owner is not None
                else {
                    str(item.get("id"))
                    for item in items
                    if any(
                        folded_query == search_key(value)
                        for value in roadmap_search_values(item)
                    )
                    and item.get("id")
                }
            )
        if active_handoff_family is not None and any(
            str(item.get("id") or "").casefold() == active_handoff_family.casefold()
            for item in items
        ):
            exact_index_ids.add(active_handoff_family)
    selected: list[JsonObject] = []
    for item in items:
        item_status = str(item.get("status") or "").casefold()
        if status != "all" and item_status != status.casefold():
            continue
        if folded_query:
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
                for value in roadmap_search_values(item, tests, index=index)
            ):
                continue
        selected.append(item)
    selected.sort(
        key=lambda item: (
            item.get("priority") if isinstance(item.get("priority"), int) else 999,
            str(item.get("id") or "").casefold(),
        )
    )

    rows: list[JsonObject] = []
    for item in selected:
        matching_tests = item_tests(item, tests, index=index)
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
                if lane_query and search_key(tag) == lane_query
            },
            key=str.casefold,
        )
        lane_matches: list[JsonObject] = []
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
            inventory_lanes = inventory.get("lanes")
            inventory_rows = (
                inventory_lanes if isinstance(inventory_lanes, list) else []
            )
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
                        "next_requirement_ids": lane_row.get(
                            "next_requirement_ids", []
                        ),
                        "next_standard_sections": lane_row.get(
                            "next_standard_sections", []
                        ),
                        "representative_test": lane_row.get("representative_test"),
                        "representative_test_id": lane_row.get(
                            "representative_test_id"
                        ),
                        "representative_source": lane_row.get("representative_source"),
                        "representative_requirement_ids": lane_row.get(
                            "representative_requirement_ids", []
                        ),
                        "representative_standard_sections": lane_row.get(
                            "representative_standard_sections", []
                        ),
                        "representative_case_command": (
                            f'python tools/query_rti_work.py case "{lane_row.get("representative_test_id")}" '
                            "--summary --compact"
                            if lane_row.get("representative_test_id")
                            else None
                        ),
                        "representative_trace_command": lane_row.get(
                            "representative_trace_command"
                        ),
                        "next_case_command": (
                            f'python tools/query_rti_work.py case "{lane_row.get("next_test_id")}" '
                            "--summary --compact"
                            if lane_row.get("next_test_id")
                            else None
                        ),
                        "focus_command": lane_row.get("focus_command"),
                        "ctest_command": lane_row.get("ctest_command"),
                        "next_trace_command": lane_row.get("next_trace_command"),
                    }
                )
        mapping_counts = live_item_mapping_counts(item, tests, index=index)
        family_planned_tests = planned_tests_for_item(item, tests, index=index)
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
        lane_state_for_action = None
        if (
            isinstance(lane_snapshot, dict)
            and lane_snapshot.get("lane_state") == "complete"
        ):
            lane_state_for_action = "complete-pointer"
        elif (
            isinstance(lane_snapshot, dict)
            and lane_snapshot.get("lane_state") == "planned"
        ) or family_planned_tests:
            lane_state_for_action = "planned"
        lane_snapshot_data = lane_snapshot if isinstance(lane_snapshot, dict) else {}
        lane_snapshot_is_available = (
            isinstance(lane_snapshot, dict)
            and lane_snapshot_data.get("lane_state") != "missing"
        )
        candidate_count = (
            lane_snapshot_data.get("candidate_count")
            if lane_snapshot_is_available
            else None
        )
        if not isinstance(candidate_count, int):
            candidate_count = mapping_counts.get("executable_candidate_count")
        if not isinstance(candidate_count, int):
            candidate_count = 0
        unclassified_count = (
            lane_snapshot_data.get("unclassified_count")
            if lane_snapshot_is_available
            else None
        )
        if not isinstance(unclassified_count, int):
            unclassified_count = mapping_counts.get("unclassified_count")
        if not isinstance(unclassified_count, int):
            unclassified_count = 0
        actionable_source_drift_count = (
            lane_snapshot_data.get("actionable_source_drift_count")
            if lane_snapshot_is_available
            else None
        )
        if not isinstance(actionable_source_drift_count, int):
            actionable_source_drift_count = sum(
                not test.get("source_locations")
                and not is_planned_test(test)
                and not is_non_executable_source_status(test)
                for test in matching_tests
            )
        action_state = queue_action_state(
            item,
            candidate_count=candidate_count,
            unclassified_count=unclassified_count,
            actionable_source_drift_count=actionable_source_drift_count,
            lane_state=lane_state_for_action,
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
        pointer_case_query = pointer.get("case_query")
        if isinstance(pointer_case_query, str) and pointer_case_query:
            commands.update(
                {
                    "case": (
                        f'python tools/query_rti_work.py case "{pointer_case_query}" '
                        "--summary --compact"
                    ),
                    "trace": (
                        f'python tools/query_rti_work.py trace "{pointer_case_query}" '
                        "--summary --compact"
                    ),
                    "matrix_requirement": (
                        f'python tools/query_rti_work.py matrix "{pointer_case_query}" '
                        "--group-by requirement --summary --compact"
                    ),
                    "matrix_section": (
                        f'python tools/query_rti_work.py matrix "{pointer_case_query}" '
                        "--group-by section --summary --compact"
                    ),
                }
            )
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
            "active_pointer_lane": pointer_owner.get("next_lane")
            if pointer_owner
            else None,
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
            and str(item.get("id") or "").casefold() == active_handoff_family.casefold()
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


def roadmap_summary_data(row: JsonObject, limit: int = 8) -> JsonObject:
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
            result["next_test_pointer"]["requirement_section_mappings"] = pair_rows[
                :limit
            ]
            if len(pair_rows) > limit:
                result["next_test_pointer"][
                    "requirement_section_mappings_remaining"
                ] = len(pair_rows) - limit
    lane_matches = row.get("lane_matches")
    if isinstance(lane_matches, list):
        bounded_lanes: list[JsonObject] = []
        for lane in lane_matches[:limit]:
            if not isinstance(lane, dict):
                continue
            bounded_lane = cast(JsonObject, dict(lane))
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


def bounded_requirement_ids(test: JsonObject, limit: int = 12) -> JsonObject:
    """Return exact requirement handles without expanding a cross-cutting case."""

    values = strings(test.get("lab_requirement_ids"))
    result: JsonObject = {
        "requirement_count": len(values),
        "requirement_ids": values[:limit],
    }
    if len(values) > limit:
        result["requirement_ids_remaining"] = len(values) - limit
    return result


def traceability_state(test: JsonObject) -> str:
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
    index: JsonObject,
    tests: Sequence[JsonObject],
    requested_id: str | None = None,
    source_locations: Mapping[str, Sequence[TestSourceLocation]] | None = None,
) -> JsonObject:
    """Resolve one actionable slice without scanning roadmap prose.

    The human roadmap item and the active implementation item are deliberately
    separate in the index: a broad parent can point at a narrower
    ``next_work_id``.  This resolver makes that relationship explicit and joins
    only the exact baseline test, lane counts, and canonical 2025 sections.
    """

    items = json_object_rows(index.get("items"))
    by_id = {
        item_id: item
        for item in items
        if isinstance(item_id := item.get("id"), str) and item_id
    }
    if requested_id is None:
        open_items = sorted(
            (item for item in items if item.get("status") == "open"),
            key=roadmap_priority,
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
        return cast(
            JsonObject,
            {
                "found": False,
                "requested_id": requested_id,
                "reason": "no indexed open roadmap item matched"
                if requested_id is None
                else "no indexed roadmap item matched",
            },
        )

    # A narrow work item may be referenced by a broader priority owner and
    # intentionally carry no duplicate pointer fields of its own.  Resolve
    # that reverse edge only when the requested item is truly pointer-only.
    # If it has its own current action, an explicit ``work <family-id>`` query
    # must not inherit stale historical counts from the parent.
    if (
        requested_id is not None
        and not any(
            pointer_owner.get(field)
            for field in (
                "next_lane",
                "next_ctest_filter",
                "next_test_query",
                "next_work_query",
            )
        )
        and not str(pointer_owner.get("next_action") or "").strip()
    ):
        owners = sorted(
            (item for item in items if item.get("next_work_id") == requested_id),
            key=roadmap_priority,
        )
        if owners:
            pointer_owner = owners[0]

    target_id = pointer_owner.get("next_work_id") or pointer_owner.get("id")
    target = (
        by_id.get(target_id, pointer_owner)
        if isinstance(target_id, str)
        else pointer_owner
    )
    lane = pointer_owner.get("next_lane") or target.get("next_lane")
    ctest_filter = pointer_owner.get("next_ctest_filter") or target.get(
        "next_ctest_filter"
    )
    package_target = pointer_owner.get("next_package_target") or target.get(
        "next_package_target"
    )
    package_ctest_filter = pointer_owner.get("next_package_ctest_filter") or target.get(
        "next_package_ctest_filter"
    )
    package_manifest = pointer_owner.get("next_package_manifest") or target.get(
        "next_package_manifest"
    )
    junit_target = pointer_owner.get("next_junit_target") or target.get(
        "next_junit_target"
    )
    junit_artifact = pointer_owner.get("next_junit_artifact") or target.get(
        "next_junit_artifact"
    )
    process_probe_target = pointer_owner.get("next_process_probe_target") or target.get(
        "next_process_probe_target"
    )
    process_package_target = pointer_owner.get(
        "next_process_package_target"
    ) or target.get("next_process_package_target")
    process_package_test = pointer_owner.get("next_process_package_test") or target.get(
        "next_process_package_test"
    )
    process_package_ctest_filter = pointer_owner.get(
        "next_process_package_ctest_filter"
    ) or target.get("next_process_package_ctest_filter")
    process_package_timestamped_test = pointer_owner.get(
        "next_process_package_timestamped_test"
    ) or target.get("next_process_package_timestamped_test")
    process_package_timestamped_ctest_filter = pointer_owner.get(
        "next_process_package_timestamped_ctest_filter"
    ) or target.get("next_process_package_timestamped_ctest_filter")
    process_package_parameterized_test = pointer_owner.get(
        "next_process_package_parameterized_test"
    ) or target.get("next_process_package_parameterized_test")
    process_package_parameterized_ctest_filter = pointer_owner.get(
        "next_process_package_parameterized_ctest_filter"
    ) or target.get("next_process_package_parameterized_ctest_filter")
    process_package_connection_loss_test = pointer_owner.get(
        "next_process_package_connection_loss_test"
    ) or target.get("next_process_package_connection_loss_test")
    process_package_connection_loss_ctest_filter = pointer_owner.get(
        "next_process_package_connection_loss_ctest_filter"
    ) or target.get("next_process_package_connection_loss_ctest_filter")
    process_package_connection_loss_delete_objects_test = pointer_owner.get(
        "next_process_package_connection_loss_delete_objects_test"
    ) or target.get("next_process_package_connection_loss_delete_objects_test")
    process_package_connection_loss_delete_objects_ctest_filter = pointer_owner.get(
        "next_process_package_connection_loss_delete_objects_ctest_filter"
    ) or target.get("next_process_package_connection_loss_delete_objects_ctest_filter")
    process_package_object_registration_test = pointer_owner.get(
        "next_process_package_object_registration_test"
    ) or target.get("next_process_package_object_registration_test")
    process_package_object_registration_ctest_filter = pointer_owner.get(
        "next_process_package_object_registration_ctest_filter"
    ) or target.get("next_process_package_object_registration_ctest_filter")
    process_package_named_registration_test = pointer_owner.get(
        "next_process_package_named_registration_test"
    ) or target.get("next_process_package_named_registration_test")
    process_package_named_registration_ctest_filter = pointer_owner.get(
        "next_process_package_named_registration_ctest_filter"
    ) or target.get("next_process_package_named_registration_ctest_filter")
    process_package_attribute_update_test = pointer_owner.get(
        "next_process_package_attribute_update_test"
    ) or target.get("next_process_package_attribute_update_test")
    process_package_attribute_update_ctest_filter = pointer_owner.get(
        "next_process_package_attribute_update_ctest_filter"
    ) or target.get("next_process_package_attribute_update_ctest_filter")
    process_package_directed_retraction_test = pointer_owner.get(
        "next_process_package_directed_retraction_test"
    ) or target.get("next_process_package_directed_retraction_test")
    process_package_directed_retraction_ctest_filter = pointer_owner.get(
        "next_process_package_directed_retraction_ctest_filter"
    ) or target.get("next_process_package_directed_retraction_ctest_filter")
    process_package_federation_save_restore_test = pointer_owner.get(
        "next_process_package_federation_save_restore_test"
    ) or target.get("next_process_package_federation_save_restore_test")
    process_package_federation_save_restore_ctest_filter = pointer_owner.get(
        "next_process_package_federation_save_restore_ctest_filter"
    ) or target.get("next_process_package_federation_save_restore_ctest_filter")
    process_package_federation_save_restore_failure_test = pointer_owner.get(
        "next_process_package_federation_save_restore_failure_test"
    ) or target.get("next_process_package_federation_save_restore_failure_test")
    process_package_federation_save_restore_failure_ctest_filter = pointer_owner.get(
        "next_process_package_federation_save_restore_failure_ctest_filter"
    ) or target.get("next_process_package_federation_save_restore_failure_ctest_filter")
    process_package_federation_save_restore_abort_test = pointer_owner.get(
        "next_process_package_federation_save_restore_abort_test"
    ) or target.get("next_process_package_federation_save_restore_abort_test")
    process_package_federation_save_restore_abort_ctest_filter = pointer_owner.get(
        "next_process_package_federation_save_restore_abort_ctest_filter"
    ) or target.get("next_process_package_federation_save_restore_abort_ctest_filter")
    process_package_federation_save_restore_status_test = pointer_owner.get(
        "next_process_package_federation_save_restore_status_test"
    ) or target.get("next_process_package_federation_save_restore_status_test")
    process_package_federation_save_restore_status_ctest_filter = pointer_owner.get(
        "next_process_package_federation_save_restore_status_ctest_filter"
    ) or target.get("next_process_package_federation_save_restore_status_ctest_filter")
    process_package_catalog_verifier = pointer_owner.get(
        "next_process_package_catalog_verifier"
    ) or target.get("next_process_package_catalog_verifier")
    process_package_junit_artifact = pointer_owner.get(
        "next_process_package_junit_artifact"
    ) or target.get("next_process_package_junit_artifact")
    baseline_query = pointer_owner.get("next_test_query") or target.get(
        "next_test_query"
    )
    source_state = source_pointer_state(pointer_owner)
    if source_state == "none" and target is not pointer_owner:
        source_state = source_pointer_state(target)
    source_test_query = pointer_owner.get("next_source_test_query") or target.get(
        "next_source_test_query"
    )
    source_location = pointer_owner.get("next_source_location") or target.get(
        "next_source_location"
    )
    source_lane = pointer_owner.get("next_source_lane") or target.get(
        "next_source_lane"
    )
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
    source_ctest_filter = pointer_owner.get("next_source_ctest_filter") or target.get(
        "next_source_ctest_filter"
    )
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
    lane_snapshot: JsonObject | None = None
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
    if (
        isinstance(process_package_timestamped_test, str)
        and process_package_timestamped_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_timestamped_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_timestamped_ctest_filter, str)
        and process_package_timestamped_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_timestamped_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_parameterized_test, str)
        and process_package_parameterized_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_parameterized_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_parameterized_ctest_filter, str)
        and process_package_parameterized_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_parameterized_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_connection_loss_test, str)
        and process_package_connection_loss_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_connection_loss_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_connection_loss_ctest_filter, str)
        and process_package_connection_loss_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_connection_loss_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_connection_loss_delete_objects_test, str)
        and process_package_connection_loss_delete_objects_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_connection_loss_delete_objects_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_connection_loss_delete_objects_ctest_filter, str)
        and process_package_connection_loss_delete_objects_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_connection_loss_delete_objects_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_object_registration_test, str)
        and process_package_object_registration_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_object_registration_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_object_registration_ctest_filter, str)
        and process_package_object_registration_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_object_registration_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_named_registration_test, str)
        and process_package_named_registration_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_named_registration_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_named_registration_ctest_filter, str)
        and process_package_named_registration_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_named_registration_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_attribute_update_test, str)
        and process_package_attribute_update_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_attribute_update_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_attribute_update_ctest_filter, str)
        and process_package_attribute_update_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_attribute_update_ctest_filter} --output-on-failure"
        )
    if (
        isinstance(process_package_directed_retraction_test, str)
        and process_package_directed_retraction_test
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f'-R "^{process_package_directed_retraction_test}$" --output-on-failure'
        )
    if (
        isinstance(process_package_directed_retraction_ctest_filter, str)
        and process_package_directed_retraction_ctest_filter
    ):
        commands.append(
            "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
            f"-L {process_package_directed_retraction_ctest_filter} --output-on-failure"
        )
    for test_name, label in (
        (
            process_package_federation_save_restore_test,
            "process_package_federation_save_restore",
        ),
        (
            process_package_federation_save_restore_failure_test,
            "process_package_federation_save_restore_failure",
        ),
        (
            process_package_federation_save_restore_abort_test,
            "process_package_federation_save_restore_abort",
        ),
        (
            process_package_federation_save_restore_status_test,
            "process_package_federation_save_restore_status",
        ),
    ):
        if isinstance(test_name, str) and test_name:
            commands.append(
                "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
                f'-R "^{test_name}$" --output-on-failure'
            )
    for ctest_label in (
        process_package_federation_save_restore_ctest_filter,
        process_package_federation_save_restore_failure_ctest_filter,
        process_package_federation_save_restore_abort_ctest_filter,
        process_package_federation_save_restore_status_ctest_filter,
    ):
        if isinstance(ctest_label, str) and ctest_label:
            commands.append(
                "ctest --test-dir <build-dir>/package-smoke-consumer -C Debug "
                f"-L {ctest_label} --output-on-failure"
            )
    if (
        isinstance(process_package_catalog_verifier, str)
        and process_package_catalog_verifier
    ):
        verifier_command = (
            "python tools/verify_process_package_lanes.py --ctest ctest "
            "--test-dir <build-dir>/package-smoke-consumer --config Debug"
        )
        if (
            isinstance(process_package_junit_artifact, str)
            and process_package_junit_artifact
        ):
            verifier_command += f" --junit {process_package_junit_artifact}"
        commands.append(verifier_command)
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
    family_mapping_commands: dict[str, str] = {}
    if isinstance(family_discovery_id, str) and family_discovery_id:
        # Keep the standards crosswalk beside the family handoff. A caller
        # should not have to remember a second syntax or reopen the full plan
        # just to pivot from the implementation family to its direct
        # requirement/subsection evidence.
        family_mapping_commands = {
            "requirement": (
                "python tools/query_rti_work.py matrix "
                f"{family_discovery_id} --group-by requirement "
                "--summary --compact"
            ),
            "section": (
                "python tools/query_rti_work.py matrix "
                f"{family_discovery_id} --group-by section "
                "--summary --compact"
            ),
        }
        commands.extend(family_mapping_commands.values())
    work_mapping = live_item_mapping_counts(target, tests, index=index)
    standard_sections = strings(
        pointer_owner.get("next_standard_sections")
        or target.get("next_standard_sections")
    )
    plan_ids = strings(
        pointer_owner.get("next_plan_ids") or target.get("next_plan_ids")
    )
    catalog_gap_plan_ids = strings(
        pointer_owner.get("catalog_gap_plan_ids") or target.get("catalog_gap_plan_ids")
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
    return cast(
        JsonObject,
        {
            "found": True,
            "requested_id": requested_id,
            "parent_item": roadmap_item_summary(pointer_owner),
            "work_item": roadmap_item_summary(target),
            "work_id": target.get("id"),
            "work_status": pointer_owner.get("next_work_status")
            or target.get("status"),
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
            "process_package_connection_loss_delete_objects_test": process_package_connection_loss_delete_objects_test,
            "process_package_connection_loss_delete_objects_ctest_filter": process_package_connection_loss_delete_objects_ctest_filter,
            "process_package_object_registration_test": process_package_object_registration_test,
            "process_package_object_registration_ctest_filter": process_package_object_registration_ctest_filter,
            "process_package_named_registration_test": process_package_named_registration_test,
            "process_package_named_registration_ctest_filter": process_package_named_registration_ctest_filter,
            "process_package_attribute_update_test": process_package_attribute_update_test,
            "process_package_attribute_update_ctest_filter": process_package_attribute_update_ctest_filter,
            "process_package_directed_retraction_test": process_package_directed_retraction_test,
            "process_package_directed_retraction_ctest_filter": process_package_directed_retraction_ctest_filter,
            "process_package_federation_save_restore_test": process_package_federation_save_restore_test,
            "process_package_federation_save_restore_ctest_filter": process_package_federation_save_restore_ctest_filter,
            "process_package_federation_save_restore_failure_test": process_package_federation_save_restore_failure_test,
            "process_package_federation_save_restore_failure_ctest_filter": process_package_federation_save_restore_failure_ctest_filter,
            "process_package_federation_save_restore_abort_test": process_package_federation_save_restore_abort_test,
            "process_package_federation_save_restore_abort_ctest_filter": process_package_federation_save_restore_abort_ctest_filter,
            "process_package_federation_save_restore_status_test": process_package_federation_save_restore_status_test,
            "process_package_federation_save_restore_status_ctest_filter": process_package_federation_save_restore_status_ctest_filter,
            "process_package_catalog_verifier": process_package_catalog_verifier,
            "process_package_junit_artifact": process_package_junit_artifact,
            "baseline_test_role": pointer_owner.get("next_test_role")
            or target.get("next_test_role"),
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
            # ``standard_sections``/``plan_ids`` above are legacy pointer fields;
            # these live counts are the authoritative family-level view used by
            # ``work <family>`` and its JSON consumers.
            "work_mapping": work_mapping,
            "work_mapping_commands": family_mapping_commands,
            "family_discovery_id": family_discovery_id,
            "family_discovery_command": family_discovery_command,
            "family_ready_command": family_ready_command,
            "commands": commands,
        },
    )


def indexed_item(
    index: JsonObject,
    tests: Sequence[JsonObject],
    item_id: str,
) -> tuple[JsonObject | None, list[JsonObject]]:
    """Resolve one roadmap item and its tagged tests without scanning prose."""

    folded_id = item_id.casefold()
    for item in json_object_rows(index.get("items")):
        if str(item.get("id") or "").casefold() == folded_id:
            return item, item_tests(item, tests, index=index)
    return None, []


def recent_completed_views(
    index: JsonObject,
    tests: Sequence[JsonObject],
    lane: str | None = None,
) -> list[JsonObject]:
    """Join the bounded recent-slice ledger to exact Catch2 test records.

    ``ROADMAP-INDEX.json`` keeps a small, human-curated ledger of the latest
    completed slices so work can resume from evidence without reopening the
    full plan.  The test record remains authoritative for source locations and
    requirement/section mappings; this helper only joins the two views.
    """

    views: list[JsonObject] = []
    # The ledger is append-only so that each completion remains easy to audit.
    # A source split can supersede an older historical row without deleting
    # its audit record; those rows stay in the JSON but are omitted from the
    # live view.  Prefer the explicitly designated latest slice, then present
    # the remaining live rows newest-first.  This keeps status/latest and a
    # bounded ``recent --limit`` query aligned even when an older row was
    # repaired after its original append.
    entries = [
        entry
        for entry in json_object_rows(index.get("recent_completed_slices"))
        if not entry.get("superseded_by")
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
    mapping = index.get("mapping")
    latest_value = (
        mapping.get("latest_completed_slice") if isinstance(mapping, dict) else None
    )
    latest = latest_value if isinstance(latest_value, dict) else None
    latest_plan_id = latest.get("plan_id") if latest is not None else None
    latest_entry = next(
        (entry for entry in entries if entry.get("plan_id") == latest_plan_id),
        None,
    )
    if latest_entry is None and latest is not None and latest_plan_id:
        # The indexed latest pointer can be advanced before its audit-ledger
        # append. Keep the latest bounded query current without rewriting or
        # fabricating a historical ledger record.
        latest_entry = cast(JsonObject, dict(latest))
        latest_entry["test_query"] = latest.get("test_case")
        latest_lane_queries = strings(
            latest.get("focus_lane") or latest.get("primary_lane") or latest.get("lane")
        )
        latest_entry["lane_queries"] = latest_lane_queries
        if lane and lane.casefold() not in {
            value.casefold() for value in latest_lane_queries
        }:
            latest_entry = None
    ordered_entries = ([] if latest_entry is None else [latest_entry]) + [
        entry for entry in reversed(entries) if entry is not latest_entry
    ]
    for entry in ordered_entries:
        view = cast(JsonObject, dict(entry))
        query = entry.get("test_query")
        matches = [
            test
            for test in tests
            if isinstance(query, str) and test.get("test_case") == query
        ]
        view["matched_test_count"] = len(matches)
        view["tests"] = cast(JsonValue, [test_summary_data(test) for test in matches])
        if matches:
            view["source_locations"] = [
                location
                for test in matches
                for location in json_object_rows(test.get("source_locations"))
            ]
            view["resolved_standard_sections"] = sorted(
                {
                    section
                    for test in matches
                    for section in strings(test.get("standard_sections"))
                }
            )
            view["resolved_requirement_count"] = sum(
                len(strings(test.get("lab_requirement_ids"))) for test in matches
            )
        else:
            view.setdefault("source_locations", [])
            view.setdefault("resolved_standard_sections", [])
            view.setdefault("resolved_requirement_count", 0)
        views.append(view)
    return views


def validate_index(
    index: JsonObject,
    roadmap_path: Path,
    plan: JsonObject,
    standard_requirements: Mapping[str, JsonObject],
    tests: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]] | None = None,
    focus_lane: str | None = None,
    focus_family: str | None = None,
    include_historical: bool = False,
) -> list[str]:
    """Validate an indexed roadmap and its source mappings."""
    if __package__:
        from .query_rti_work_validation import validate_index as validator
    else:
        from query_rti_work_validation import validate_index as validator
    return validator(
        index,
        roadmap_path,
        plan,
        standard_requirements,
        tests,
        source_locations,
        focus_lane,
        focus_family,
        include_historical,
    )


def text_test(test: JsonObject, verbose: bool, compact: bool = False) -> str:
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        f"  status: {test.get('status', '<unspecified>')}",
        f"  mapping id: {test.get('requirements_lab_mapping_id') or '<none>'}",
        f"  traceability: {traceability_state(test)}",
        f"  tags: {', '.join(strings(test.get('tags'))) or '<none>'}",
        f"  source: {source_location_text(test)}",
        f"  standard: {', '.join(strings(test.get('standard_clauses'))) or '<unmapped>'}",
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
    requirements = json_object_rows(test.get("requirements"))
    if compact:
        if requirements:
            lines.append("  requirements:")
            for requirement in requirements:
                standard = requirement.get("standard")
                if not isinstance(standard, dict):
                    lines.append(
                        f"    - {requirement['lab_requirement_id']} (unresolved)"
                    )
                    continue
                section = f"{standard.get('document_id')} {standard.get('clause_id')}"
                lines.append(f"    - {requirement['lab_requirement_id']} -> {section}")
        else:
            lines.append("  requirements: <unmapped>")
    else:
        for requirement in requirements:
            standard = requirement.get("standard")
            if not isinstance(standard, dict):
                lines.append(
                    f"  requirement: {requirement['lab_requirement_id']} (unresolved)"
                )
                continue
            section = f"{standard.get('document_id')} {standard.get('clause_id')}"
            source_value = standard.get("source")
            source = source_value if isinstance(source_value, dict) else {}
            source_text = source.get("path")
            if source.get("line") is not None:
                source_text = f"{source_text}:{source['line']}"
            lines.append(
                f"  requirement: {requirement['lab_requirement_id']} -> {section}"
                f"; {standard.get('title', '')}"
            )
            if source_text:
                lines.append(f"    standard source: {source_text}")
            for contract in json_object_rows(requirement.get("contracts")):
                symbol = contract.get("source_symbol")
                if symbol:
                    lines.append(f"    contract: {contract.get('contract')} ({symbol})")
    contract_candidates = test.get("contract_candidates")
    if isinstance(contract_candidates, list):
        lines.append("  contract candidates:")
        if not contract_candidates:
            lines.append("    - <none>")
        else:
            shown_candidates = json_object_rows(contract_candidates[:8])
            for candidate in shown_candidates:
                lab_id = candidate.get("lab_requirement_id", "<unnamed requirement>")
                clause = candidate.get("clause_id") or "<unresolved clause>"
                contract = candidate.get("contract") or "<unnamed contract>"
                source_note = ""
                if candidate.get("source_path_match") is False:
                    source_note = "; source mismatch: " + str(
                        candidate.get("source_path") or "<unnamed source>"
                    )
                lines.append(f"    - {lab_id} -> {clause}; {contract}{source_note}")
            remaining = len(contract_candidates) - len(shown_candidates)
            if remaining > 0:
                lines.append(f"    - ... (+{remaining}); use --json for all candidates")
    next_action = test.get("next_action")
    if verbose and isinstance(next_action, str) and next_action:
        lines.append(f"  next: {next_action}")
    return "\n".join(lines)


def text_test_summary(test: JsonObject) -> str:
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

    def preview(values: Sequence[str]) -> str:
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
        "  replacement plan ids: "
        + (", ".join(strings(test.get("replacement_plan_ids"))) or "<none>"),
        (
            f"  assertions: {test.get('assertions', '<unspecified>')}; "
            f"callback_models={', '.join(strings(test.get('callback_models'))) or '<unspecified>'}; "
            f"delivery_modes={', '.join(strings(test.get('delivery_modes'))) or '<unspecified>'}; "
            f"callback_gate_modes={', '.join(strings(test.get('callback_gate_modes'))) or '<unspecified>'}"
        ),
        (
            f"  mapping id: {mapping}; traceability={traceability_state(test)}; "
            f"requirements={len(requirements)}; sections={len(sections)}; "
            f"cpp_api_surfaces={len(api_surfaces)}"
        ),
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
                for candidate in json_object_rows(contract_candidates[:8])
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
            "  Requirements-Lab API surface: " + compact_prose(api_surface_status, 360),
        )
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        lines.append(f"  source-missing reason: {compact_prose(reason, 180)}")
    replacements = strings(test.get("replacement_plan_ids"))
    if replacements:
        lines.append("  replacement plan ids: " + ", ".join(replacements))
    return "\n".join(lines)


def test_summary_data(test: JsonObject) -> JsonObject:
    """Return the bounded machine-readable shape used by ``--summary``."""

    mapping_rows = requirement_section_mapping_rows(test)
    result = cast(
        JsonObject,
        {
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
        },
    )
    reason = test.get("source_missing_reason")
    if isinstance(reason, str) and reason.strip():
        result["source_missing_reason"] = reason
    replacements = strings(test.get("replacement_plan_ids"))
    if replacements:
        result["replacement_plan_ids"] = replacements
    if "contract_candidates" in test:
        result["contract_candidates"] = test.get("contract_candidates")
    return result


def focused_lane_result(
    index: JsonObject,
    tests: Sequence[JsonObject],
    lane: str,
    limit: int = 8,
) -> JsonObject:
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
        if any(str(tag).casefold() == folded_lane for tag in strings(test.get("tags")))
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
        test for test in source_drift if not is_non_executable_source_status(test)
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
        if not test.get("requirements") and traceability_state(test) == "unclassified"
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
    requirement_section_pairs: set[tuple[str, str | None]] = set()
    mapping_rows: list[JsonObject] = []
    seen_mapping_pairs: set[tuple[str, str | None]] = set()
    for test in selected:
        for row in requirement_section_mapping_rows(test):
            requirement_id = row.get("lab_requirement_id")
            if not isinstance(requirement_id, str) or not requirement_id:
                continue
            raw_section = row.get("standard_section")
            section = raw_section if isinstance(raw_section, str) else None
            pair = (requirement_id, section)
            requirement_section_pairs.add(pair)
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
    mapping = index.get("mapping")
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
            for item in json_object_rows(index.get("items"))
            if item.get("id") == roadmap_owner
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
    candidates = (
        executable_candidates
        if candidate_limit == 0
        else executable_candidates[:candidate_limit]
    )
    result = cast(
        JsonObject,
        {
            "lane": lane,
            "roadmap_owner": roadmap_owner,
            "lane_handles": dict(lane_handles),
            "lane_state": lane_state,
            "action_state": action_state,
            "test_count": len(selected),
            "mapped_test_count": len(mapped),
            "source_located_test_count": len(selected)
            - len(source_drift)
            - len(planned),
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
        },
    )
    if len(mapping_rows) > 12:
        result["requirement_section_mappings_remaining"] = len(mapping_rows) - 12
    return result


def lane_inventory(
    index: JsonObject,
    tests: Sequence[JsonObject],
    *,
    family: str | None = None,
    unmapped_only: bool = False,
    disposition: str = "all",
    focused_only: bool = False,
    limit: int = 40,
) -> JsonObject:
    """List exact lane handles inside an optional roadmap-family scope.

    The normal lane inventory is intentionally a work-selection view rather
    than another broad plan dump.  It groups the already-indexed plan rows by
    their exact Catch2 tags and reports only the counts and one deterministic
    next-case handle per tag.  ``--family`` keeps overlapping tags scoped to
    one roadmap owner, while ``--unmapped`` makes the missing traceability
    queue directly queryable without reopening the Requirements Lab.
    ``disposition`` can narrow that queue to intentional ``explicit``
    decisions or rows still needing a mapping decision (``unclassified``).
    ``focused_only`` uses the owning roadmap family's explicit
    ``focused_lane_tags`` list instead of the broad cross-cutting tag set.
    This keeps a family work-selection query small while preserving the
    default broad inventory for taxonomy review.
    """

    if disposition not in {"all", "explicit", "unclassified"}:
        raise ValueError(
            "lane inventory disposition must be all, explicit, or unclassified"
        )

    family_item, scoped_tests = scoped_plan_tests(index, tests, family=family)
    if family is not None and family_item is None:
        return cast(
            JsonObject,
            {
                "found": False,
                "family": family,
                "disposition": disposition,
                "error": f"No indexed roadmap family matched: {family}",
                "count": 0,
                "shown_count": 0,
                "lanes": [],
            },
        )

    grouped: defaultdict[str, list[JsonObject]] = defaultdict(list)
    for test in scoped_tests:
        for tag in strings(test.get("tags")):
            grouped[tag].append(test)

    mapping = index.get("mapping")
    if not isinstance(mapping, dict):
        mapping = {}
    lane_owners = mapping.get("lane_owners", {})
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    lane_handles = mapping.get("lane_handles", {})
    if not isinstance(lane_handles, dict):
        lane_handles = {}

    focused_tags: set[str] | None = None
    if focused_only:
        # A family-scoped focused inventory is the normal path.  When no
        # family is supplied, use the union of the explicit focused aliases
        # from all indexed families so the option remains a useful bounded
        # discovery view rather than silently returning no rows.
        focused_tags = set()
        if family_item is not None:
            focused_tags.update(
                tag.casefold() for tag in strings(family_item.get("focused_lane_tags"))
            )
        else:
            for item in json_object_rows(index.get("items")):
                focused_tags.update(
                    tag.casefold() for tag in strings(item.get("focused_lane_tags"))
                )

    def case_insensitive_value(
        values: Mapping[str, JsonValue], key: str
    ) -> JsonValue | None:
        if key in values:
            return values[key]
        folded = key.casefold()
        for name, value in values.items():
            if str(name).casefold() == folded:
                return value
        return None

    rows: list[JsonObject] = []
    for tag, lane_tests in grouped.items():
        if focused_tags is not None and tag.casefold() not in focused_tags:
            continue
        mapped_count = sum(bool(test.get("requirements")) for test in lane_tests)
        explicit_count = sum(
            not test.get("requirements")
            and traceability_state(test) == "explicit-disposition"
            for test in lane_tests
        )
        unclassified_count = sum(
            not test.get("requirements") and traceability_state(test) == "unclassified"
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
            and not str(test.get("status") or "").startswith(
                ("implemented", "verified")
            )
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
            row for test in lane_tests for row in requirement_section_mapping_rows(test)
        ]
        requirement_section_pairs: set[tuple[str, str | None]] = set()
        for row in requirement_section_rows:
            requirement_id = row.get("lab_requirement_id")
            section = row.get("standard_section")
            if isinstance(requirement_id, str) and requirement_id:
                requirement_section_pairs.add(
                    (requirement_id, section if isinstance(section, str) else None)
                )
        # ``next_test`` is an actionable handoff.  Explicit no-standalone
        # dispositions are intentionally retained in the inventory, but they
        # are review material rather than implementation work; pointing at
        # one as ``next`` made a completed lane look like it had a runnable
        # case.  Keep a separate review handle below so the disposition can
        # still be inspected without polluting the work queue.
        review_tests: list[JsonObject] = []
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
                and not str(test.get("status") or "").startswith(
                    ("implemented", "verified")
                )
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
        next_case = next_test.get("test_case") if next_test is not None else None
        next_locations = (
            json_object_rows(next_test.get("source_locations"))
            if next_test is not None
            else []
        )
        review_locations = (
            json_object_rows(review_test.get("source_locations"))
            if review_test is not None
            else []
        )
        next_location = next_locations[0] if next_locations else None
        review_location = review_locations[0] if review_locations else None
        review_case = review_test.get("test_case") if review_test is not None else None
        # Completed lanes have no ``next`` case, but they still need a
        # deterministic mapping handle.  Keep one representative plan row
        # so a lane search can jump straight to its requirement/section
        # traceability without first dumping every case in the lane.
        representative_test = (
            min(
                lane_tests,
                key=lambda test: (
                    str(test.get("id") or "").casefold(),
                    str(test.get("test_case") or "").casefold(),
                ),
            )
            if lane_tests
            else None
        )
        representative_locations = (
            json_object_rows(representative_test.get("source_locations"))
            if representative_test is not None
            else []
        )
        representative_location = (
            representative_locations[0] if representative_locations else None
        )
        representative_case = (
            representative_test.get("test_case")
            if representative_test is not None
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
                for item in json_object_rows(index.get("items"))
                if item.get("id") == owner
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
            cast(
                JsonObject,
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
                        next_test.get("assertions")
                        if isinstance(next_test, dict)
                        else None
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
                        if representative_location
                        and representative_location.get("path")
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
                        review_test.get("id") if isinstance(review_test, dict) else None
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
                },
            )
        )

    rows.sort(
        key=lambda row: (
            0
            if json_integer(row.get("unclassified_count"))
            else 1
            if json_integer(row.get("explicit_disposition_count"))
            else 2,
            -json_integer(row.get("unclassified_count")),
            -json_integer(row.get("explicit_disposition_count")),
            -json_integer(row.get("candidate_count")),
            str(row.get("tag") or ""),
        )
    )
    requested_limit = max(limit, 0)
    shown = rows if requested_limit == 0 else rows[:requested_limit]
    return cast(
        JsonObject,
        {
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
            "focused_only": focused_only,
            "focused_tag_count": len(focused_tags)
            if focused_tags is not None
            else None,
            "lanes": shown,
        },
    )


def indexed_work_queue(
    index: JsonObject,
    tests: Sequence[JsonObject],
    limit: int = 8,
) -> JsonObject:
    """Return a bounded queue of open roadmap families.

    The roadmap contains broad, overlapping query tags, so printing every tag
    is a poor work-selection interface.  This view keeps one row per open
    indexed family and joins only its exact next lane (when present).  It
    classifies a row as ready, complete-pointer, source-drift-only, or
    new-case-needed without changing the plan or re-reading the Lab.  The
    separate ``action_state`` says whether that row is an implementation,
    mapping, source-reconciliation, external-review, backlog-shaping,
    deliberate-new-case, or evidence-complete handoff.  It also
    exposes the bounded source-unlocated backlog so a planned next case is
    visible without dumping the full plan; those rows are never executable
    candidates until a matching C++ declaration exists.  Disabled source
    artifacts remain in the diagnostic count, but are excluded from actionable
    queue heads so historical ``#if 0`` declarations cannot block ``ready``.
    """

    items = sorted(
        (
            item
            for item in json_object_rows(index.get("items"))
            if item.get("status") == "open"
        ),
        key=roadmap_priority,
    )
    mapping = index.get("mapping")
    lane_handles = mapping.get("lane_handles", {}) if isinstance(mapping, dict) else {}
    if not isinstance(lane_handles, dict):
        lane_handles = {}
    lane_owners = mapping.get("lane_owners", {}) if isinstance(mapping, dict) else {}
    if not isinstance(lane_owners, dict):
        lane_owners = {}
    rows: list[JsonObject] = []
    for item in items:
        selected = item_tests(item, tests, index=index)
        planned = planned_tests_for_item(item, tests, index=index)
        unlocated = [
            test
            for test in selected
            if not test.get("source_locations") and not is_planned_test(test)
        ]
        actionable_unlocated = [
            test for test in unlocated if not is_non_executable_source_status(test)
        ]
        mapped_count = sum(bool(test.get("requirements")) for test in selected)
        family_case_count = len(selected)
        family_mapped_case_count = mapped_count
        source_drift_count = len(unlocated)
        actionable_source_drift_count = len(actionable_unlocated)
        planned_count = len(planned)
        family_mapping_counts = live_item_mapping_counts(item, tests, index=index)
        unclassified_count = family_mapping_counts.get("unclassified_count", 0)
        candidate_count = sum(
            bool(test.get("source_locations"))
            and not str(test.get("status") or "").startswith(
                ("implemented", "verified")
            )
            and not is_non_executable_source_status(test)
            for test in selected
        )
        lane = item.get("next_lane")
        lane_owner = lane_owners.get(lane) if isinstance(lane, str) else None
        lane_owner_matches = not isinstance(lane_owner, str) or (
            lane_owner.casefold() == str(item.get("id") or "").casefold()
        )
        case_count = len(selected)
        lane_snapshot: JsonObject | None = None
        if isinstance(lane, str) and lane:
            lane_snapshot = focused_lane_result(index, tests, lane, limit=1)
        if lane_snapshot and lane_snapshot.get("lane_state") != "missing":
            case_count = json_integer(lane_snapshot.get("test_count"), case_count)
            mapped_count = json_integer(
                lane_snapshot.get("mapped_test_count"), mapped_count
            )
            unclassified_count = json_integer(
                lane_snapshot.get("unclassified_count"), unclassified_count
            )
            source_drift_count = json_integer(
                lane_snapshot.get("source_drift_count"), source_drift_count
            )
            actionable_source_drift_count = json_integer(
                lane_snapshot.get("actionable_source_drift_count"),
                actionable_source_drift_count,
            )
            if lane_owner_matches:
                planned_count = json_integer(
                    lane_snapshot.get("planned_count"), planned_count
                )
            candidate_count = json_integer(
                lane_snapshot.get("candidate_count"), candidate_count
            )
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
            candidate_count=(
                candidate_count
                if lane_owner_matches
                else family_mapping_counts.get("executable_candidate_count", 0)
            ),
            unclassified_count=(
                unclassified_count
                if lane_owner_matches
                else family_mapping_counts.get("unclassified_count", 0)
            ),
            actionable_source_drift_count=(
                actionable_source_drift_count
                if lane_owner_matches
                else family_mapping_counts.get("actionable_source_drift_count", 0)
            ),
            lane_state=state if lane_owner_matches else None,
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
        family_mapping_counts = live_item_mapping_counts(item, tests, index=index)
        rows.append(
            cast(
                JsonObject,
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
                    "unlocated_head": (unlocated_head_query),
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
                    "focused_lane_count": len(strings(item.get("focused_lane_tags"))),
                    "next_test_query": item.get("next_test_query"),
                    "next_test_pointer": next_test,
                    "next_test_state": next_test.get("state"),
                    "next_action": live_next_action(item),
                    "work_query": item.get("next_work_query"),
                },
            )
        )
    requested_limit = max(limit, 0)
    shown = rows if requested_limit == 0 else rows[:requested_limit]
    action_counts = Counter(
        str(row.get("action_state") or "<unspecified>") for row in rows
    )
    return cast(
        JsonObject,
        {
            "items": shown,
            "shown_count": len(shown),
            "open_count": len(rows),
            "executable_candidate_count": sum(
                json_integer(row.get("candidate_count")) for row in rows
            ),
            "source_drift_count": sum(
                json_integer(row.get("source_drift_count")) for row in rows
            ),
            "actionable_source_drift_count": sum(
                json_integer(row.get("actionable_source_drift_count")) for row in rows
            ),
            "unlocated_case_count": sum(
                json_integer(row.get("unlocated_case_count")) for row in rows
            ),
            "unlocated_actionable_case_count": sum(
                json_integer(row.get("unlocated_actionable_case_count")) for row in rows
            ),
            "planned_case_count": sum(
                json_integer(row.get("planned_count")) for row in rows
            ),
            "mapped_case_count": sum(
                json_integer(row.get("mapped_case_count")) for row in rows
            ),
            "action_counts": dict(sorted(action_counts.items())),
            "queued_action_family_count": sum(
                value
                for name, value in action_counts.items()
                if name != "evidence-complete"
            ),
            "evidence_complete_family_count": action_counts.get("evidence-complete", 0),
        },
    )


def dashboard_snapshot(
    index: JsonObject,
    tests: Sequence[JsonObject],
    roadmap_entries: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]] | None = None,
    source_health: TestSourceHealth | None = None,
    limit: int = 3,
    standard_requirements: Mapping[str, JsonObject] | None = None,
) -> JsonObject:
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
    plan_case_count = status.get("test_count", 0)
    if not isinstance(plan_case_count, int):
        plan_case_count = 0
    unmapped_case_count = mapping_counts.get(
        "catch2_cases_without_lab_requirement_mapping", 0
    )
    if not isinstance(unmapped_case_count, int):
        unmapped_case_count = 0
    comparable_keys = sorted(set(indexed_counts).intersection(mapping_counts))
    live_snapshot = {key: mapping_counts.get(key) for key in comparable_keys}
    indexed_snapshot = {key: indexed_counts.get(key) for key in comparable_keys}
    mapping_index = index.get("mapping", {})
    if not isinstance(mapping_index, dict):
        mapping_index = {}
    stored_traceability = mapping_index.get("current_snapshot", {})
    if not isinstance(stored_traceability, dict):
        stored_traceability = {}
    live_pairs: set[tuple[str, str | None]] = set()
    for test in tests:
        for row in requirement_section_mapping_rows(test):
            requirement_id = row.get("lab_requirement_id")
            section = row.get("standard_section")
            if isinstance(requirement_id, str) and requirement_id:
                live_pairs.add(
                    (requirement_id, section if isinstance(section, str) else None)
                )
    live_mapped_requirement_ids = {
        requirement_id for requirement_id, section in live_pairs if section
    }
    live_standard_requirement_total = (
        len(standard_requirements)
        if isinstance(standard_requirements, Mapping)
        else None
    )
    live_coverage_percent = (
        round(
            100.0 * len(live_mapped_requirement_ids) / live_standard_requirement_total,
            2,
        )
        if isinstance(live_standard_requirement_total, int)
        and live_standard_requirement_total > 0
        else None
    )
    live_traceability = {
        "direct_requirement_section_pairs": len(live_pairs),
        "standard_requirement_total": live_standard_requirement_total,
        "standard_requirement_coverage_percent": live_coverage_percent,
    }
    comparable_traceability_keys = [
        key
        for key, value in live_traceability.items()
        if value is not None and key in stored_traceability
    ]
    stale_traceability_fields = [
        key
        for key in comparable_traceability_keys
        if stored_traceability.get(key) != live_traceability.get(key)
    ]
    traceability_matches = (
        bool(comparable_traceability_keys) and not stale_traceability_fields
    )
    traceability_snapshot = {
        "matches_live": traceability_matches,
        "indexed": {
            key: stored_traceability.get(key) for key in comparable_traceability_keys
        },
        "live": {
            key: live_traceability.get(key) for key in comparable_traceability_keys
        },
        "stale_fields": stale_traceability_fields,
    }

    active = indexed_work_slice(
        index,
        tests,
        source_locations=source_locations,
    )
    next_card: JsonObject | None = None
    handles = {
        "ready": "python tools/query_rti_work.py ready --summary --compact",
        "check": "python tools/query_rti_work.py check --summary --compact",
        "lab_issues": "python tools/query_rti_work.py lab-issues --summary --compact",
    }
    if active.get("found"):
        baseline = active.get("baseline")
        work_item = json_object_value(active.get("work_item"))
        next_card = {
            "work_id": active.get("work_id"),
            "work_status": active.get("work_status"),
            "title": work_item.get("title") if work_item is not None else None,
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
                f"python tools/query_rti_work.py work {work_id} --summary --compact"
            )
        lane = active.get("lane")
        if isinstance(lane, str) and lane:
            handles["focus"] = (
                f"python tools/query_rti_work.py focus {lane} --summary --compact"
            )
            handles["lane_check"] = (
                f"python tools/query_rti_work.py check --lane {lane} "
                "--summary --compact"
            )
            handles["matrix"] = (
                f"python tools/query_rti_work.py matrix {lane} --summary --compact"
            )
        if isinstance(baseline, dict) and isinstance(baseline.get("test_case"), str):
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
        source_locations if isinstance(source_locations, Mapping) else {},
        standard_requirements=standard_requirements,
    )
    if ready.get("found"):
        owner = (
            json_object_value(ready.get("roadmap_owner"))
            or json_object_value(ready.get("owner"))
            or {}
        )
        requirement_ids = strings(ready.get("requirement_ids"))
        standard_sections = strings(ready.get("standard_sections"))
        api_surfaces = strings(ready.get("api_surfaces"))
        requirement_section_pairs = json_object_rows(
            ready.get("requirement_section_pairs")
        )
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
            "active_handoff_id": ready.get("active_handoff_id"),
            "state": ready.get("state"),
            "runnable": ready.get("runnable", True),
            "handoff_kind": ready.get("handoff_kind"),
            "family_id": ready.get("family_id"),
            "work_id": ready.get("work_id") or ready.get("family_id"),
            "work_status": (
                ready.get("status")
                if ready.get("state") in {"planned", "mapping"} and ready.get("status")
                else ready.get("work_status") or owner.get("status")
            ),
            "title": owner.get("title"),
            "test_case": ready.get("test_case"),
            "plan_id": ready.get("plan_id"),
            "plan_id_status": ready.get("plan_id_status"),
            "source_location": ready.get("source_location"),
            "source_target": ready.get("source_target"),
            "source_lane": ready.get("source_lane"),
            "lane_pending": ready.get("lane_pending"),
            "post_mapping_focus_command": ready.get("post_mapping_focus_command"),
            "post_mapping_check_command": ready.get("post_mapping_check_command"),
            "ctest_target": ready.get("ctest_target"),
            "ctest_filter": ready.get("ctest_filter"),
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
            "requirement_section_pairs": requirement_section_pairs[:8],
            "requirement_section_pairs_remaining": max(
                0, len(requirement_section_pairs) - 8
            ),
            "next_action": compact_prose(ready.get("next_action"), 320),
            "acceptance": ready.get("acceptance", []),
            "case_command": ready.get("case_command"),
            "matrix_requirement_command": ready.get("matrix_requirement_command"),
            "matrix_section_command": ready.get("matrix_section_command"),
            "seed_trace_command": ready.get("seed_trace_command"),
            "commands": [
                command
                for command in (
                    ready.get("case_command"),
                    ready.get("trace_command"),
                    ready.get("matrix_requirement_command"),
                    ready.get("matrix_section_command"),
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
            ("case", "case_command"),
            ("trace", "trace_command"),
            ("matrix_requirement", "matrix_requirement_command"),
            ("matrix_section", "matrix_section_command"),
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
                f"python tools/query_rti_work.py matrix {family_id} --summary --compact"
            )
    elif ready.get("reason"):
        next_card = {
            "state": ready.get("state", "unavailable"),
            "reason": ready.get("reason"),
            "selection_required": ready.get("selection_required", False),
            "recommended_family_id": ready.get("recommended_family_id"),
            "family_choice_command": ready.get("family_choice_command"),
            "family_options": ready.get("family_options", []),
            "family_queue_count": ready.get("family_queue_count", 0),
            "evidence_complete_families": ready.get("evidence_complete_families", []),
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
        handles["queue"] = "python tools/query_rti_work.py queue --summary --compact"
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
        latest_test = next(
            (
                test
                for test in tests
                if isinstance(test, dict) and test.get("id") == latest.get("plan_id")
            ),
            None,
        )
        latest_test_summary = (
            test_summary_data(latest_test) if isinstance(latest_test, dict) else {}
        )
        latest_mapping = index.get("mapping", {})
        if not isinstance(latest_mapping, dict):
            latest_mapping = {}
        lane_handle_index = latest_mapping.get("lane_handles", {})
        if not isinstance(lane_handle_index, dict):
            lane_handle_index = {}
        lane_handles = (
            lane_handle_index.get(latest_focus_lane)
            if isinstance(latest_focus_lane, str)
            else None
        )
        if not isinstance(lane_handles, dict) or not lane_handles:
            lane_handles = (
                lane_handle_index.get(latest_lane)
                if isinstance(latest_lane, str)
                else None
            )
        latest_ctest_command = latest.get("ctest_command")
        if (
            not isinstance(latest_ctest_command, str)
            or not latest_ctest_command.strip()
        ):
            latest_ctest_command = None
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
            "ctest_command": lane_ctest_command(lane_handles)
            or lane_ctest_command(
                {
                    "ctest_filter": (
                        latest_test.get("ctest_filter")
                        if isinstance(latest_test, dict)
                        else latest_test_summary.get("ctest_filter")
                    )
                }
            )
            or latest_ctest_command,
        }

    queue = indexed_work_queue(index, tests, limit=max(limit, 0))
    queue_preview = []
    for row in json_object_rows(queue.get("items")):
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
    mapping_index = index.get("mapping", {})
    latest_verified_restore_slice = (
        mapping_index.get("latest_verified_restore_slice")
        if isinstance(mapping_index, dict)
        else None
    )
    if not isinstance(latest_verified_restore_slice, dict):
        latest_verified_restore_slice = None
    return {
        "resume_card": index.get("resume_card", {}),
        "roadmap_counts": status.get("roadmap_counts", {}),
        "plan_counts": {
            "catch2_plan_cases": plan_case_count,
            "planned_cases": mapping_counts.get("catch2_planned_cases", 0),
            "mapped_cases": plan_case_count - unmapped_case_count,
            "unmapped_cases": unmapped_case_count,
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
        # The concrete health record contains only JSON scalars and arrays.
        "source_health": cast(JsonValue, source_health or {}),
        "latest_completed_slice": latest_card,
        "latest_verified_restore_slice": latest_verified_restore_slice,
        "next": next_card,
        "active_handoff": indexed_active_handoff_record(index, tests),
        "deferred_slices": deferred_slices,
        "source_only_reconciliation": source_queue,
        "active_pointer": active_card,
        "queue": queue,
        "index_snapshot": {
            "matches_live": (
                indexed_snapshot == live_snapshot and traceability_matches
            ),
            "comparable_keys": comparable_keys,
            "indexed_counts": indexed_snapshot,
            "live_counts": live_snapshot,
            "traceability": traceability_snapshot,
        },
        "handles": handles,
    }


def resume_snapshot(
    index: JsonObject,
    tests: Sequence[JsonObject],
    roadmap_entries: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]] | None = None,
    source_health: TestSourceHealth | None = None,
    limit: int = 1,
    standard_requirements: Mapping[str, JsonObject] | None = None,
) -> JsonObject:
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

    def compact_next(value: JsonValue) -> JsonObject | None:
        if not isinstance(value, dict):
            return None
        result = {
            key: value.get(key)
            for key in (
                "found",
                "active_handoff_id",
                "state",
                "runnable",
                "handoff_kind",
                "family_id",
                "work_id",
                "work_status",
                "title",
                "test_case",
                "plan_id",
                "plan_id_status",
                "source_location",
                "source_target",
                "source_lane",
                "lane_pending",
                "post_mapping_focus_command",
                "post_mapping_check_command",
                "ctest_target",
                "ctest_filter",
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
                "seed_matrix_requirement_command",
                "seed_matrix_section_command",
                "case_command",
                "trace_command",
                "matrix_requirement_command",
                "matrix_section_command",
                "acceptance",
                "commands",
                "reason",
                "selection_required",
                "recommended_family_id",
                "family_choice_command",
                "family_queue_count",
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
            compact_options: list[JsonObject] = []
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
                            "matrix_requirement_command",
                            "matrix_section_command",
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
            next_card["requirement_section_pairs_remaining"] = max(0, len(pairs) - 8)
        else:
            pair_rows = next_card.get("requirement_section_pairs")
            if isinstance(pair_rows, list):
                pairs = [row for row in pair_rows if isinstance(row, dict)]
                next_card["requirement_section_pairs"] = pairs[:8]

    active_handoff_card = compact_next(dashboard.get("active_handoff"))
    if (
        isinstance(next_card, dict)
        and next_card.get("selection_required")
        and isinstance(active_handoff_card, dict)
        and active_handoff_card.get("found")
    ):
        # If the exhaustive indexed queues have no runnable next slice, honor
        # the explicitly recorded handoff instead of asking the developer to
        # choose a family and rediscover that same work.
        next_card = active_handoff_card

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

    verified_restore = dashboard.get("latest_verified_restore_slice")
    verified_restore_summary = None
    if isinstance(verified_restore, dict):
        verified_restore_summary = {
            key: verified_restore.get(key)
            for key in (
                "id",
                "family",
                "lane",
                "status",
                "implementation_source",
                "summary",
                "verification",
                "queries",
                "ctest",
                "evidence_boundary",
            )
            if key in verified_restore
        }
        plan_ids, remaining = bounded_values(verified_restore.get("plan_ids"))
        if plan_ids:
            verified_restore_summary["plan_ids"] = plan_ids
        if remaining:
            verified_restore_summary["plan_ids_remaining"] = remaining

    queue = dashboard.get("queue")
    queue_summary: JsonObject = {}
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
    deferred_summary = deferred_slices[:4] if isinstance(deferred_slices, list) else []

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
        "active_handoff": active_handoff_card,
        "deferred_slices": deferred_summary,
        "active_pointer": active_summary,
        "latest_completed_slice": latest_summary,
        "latest_verified_restore_slice": verified_restore_summary,
        "source_only_reconciliation": source_only_summary,
        "queue": queue_summary,
        "handles": dashboard.get("handles", {}),
    }


def indexed_unlocated_plan_summary(
    index: JsonObject,
    tests: Sequence[JsonObject],
    family_id: str | None = None,
) -> JsonObject:
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
                for item in json_object_rows(index.get("items"))
                if str(item.get("id") or "").casefold() == folded_family
            ),
            None,
        )
        selected = (
            item_tests(family_item, tests, index=index)
            if isinstance(family_item, dict)
            else []
        )
        planned = (
            planned_tests_for_item(family_item, tests, index=index)
            if isinstance(family_item, dict)
            else []
        )
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
            return cast(
                JsonObject,
                {
                    "count": 0,
                    "planned_count": len(planned),
                    "diagnostic_count": len(diagnostic_unlocated),
                    "family_count": 0,
                    "head": None,
                },
            )
        lane_handles: JsonObject = {}
        mapping = index.get("mapping")
        if isinstance(mapping, dict):
            configured_lanes = mapping.get("lane_handles")
            if isinstance(configured_lanes, dict):
                lane_handles = configured_lanes
        head_lane = (
            head_test.get("primary_lane") if isinstance(head_test, dict) else None
        )
        head_lane_handles = (
            lane_handles.get(head_lane, {}) if isinstance(head_lane, str) else {}
        )
        if not isinstance(head_lane_handles, dict):
            head_lane_handles = {}
        return cast(
            JsonObject,
            {
                "count": len(unlocated) + len(planned),
                "planned_count": len(planned),
                "diagnostic_count": len(diagnostic_unlocated),
                "family_count": 1,
                "head": {
                    "family_id": family_id,
                    "plan_id": head_test.get("id"),
                    "test_case": head_test.get("test_case"),
                    "lane": head_lane,
                    "source_target": head_lane_handles.get("source"),
                    "ctest_target": head_lane_handles.get("catch2_target"),
                    "ctest_filter": head_lane_handles.get("ctest_filter"),
                    "assertions": head_test.get("assertions"),
                    "requirement_ids": strings(head_test.get("lab_requirement_ids")),
                    "standard_sections": strings(head_test.get("standard_sections")),
                    "api_surfaces": strings(
                        head_test.get("selected_cpp_api_surface_ids")
                    ),
                    "trace_command": (
                        f'python tools/query_rti_work.py trace "{head_test.get("test_case")}" '
                        "--summary --compact"
                    ),
                },
            },
        )

    queue = indexed_work_queue(index, tests, limit=0)
    rows = [
        row
        for row in json_object_rows(queue.get("items"))
        if (
            row.get(
                "unlocated_actionable_case_count",
                row.get("unlocated_case_count", 0),
            )
            or row.get("planned_count", 0)
        )
    ]
    head = rows[0] if rows else None
    if not isinstance(head, dict):
        return cast(
            JsonObject,
            {
                "count": 0,
                "family_count": 0,
                "head": None,
            },
        )
    planned_head = head.get("planned_head")
    planned_head_id = head.get("planned_head_id")
    planned_head_is_present = isinstance(planned_head, str) and bool(
        planned_head.strip()
    )
    mapping = index.get("mapping")
    lane_handles: JsonObject = {}
    if isinstance(mapping, dict):
        configured_lanes = mapping.get("lane_handles")
        if isinstance(configured_lanes, dict):
            lane_handles = configured_lanes
    planned_head_lane = (
        head.get("planned_head_lane") if planned_head_is_present else None
    )
    planned_head_handles = (
        lane_handles.get(planned_head_lane, {})
        if isinstance(lane_handles, dict) and isinstance(planned_head_lane, str)
        else {}
    )
    if not isinstance(planned_head_handles, dict):
        planned_head_handles = {}
    return cast(
        JsonObject,
        {
            "count": json_integer(queue.get("planned_case_count"))
            + json_integer(
                queue.get(
                    "unlocated_actionable_case_count",
                    queue.get("unlocated_case_count"),
                )
            ),
            "planned_count": json_integer(queue.get("planned_case_count")),
            "diagnostic_count": json_integer(queue.get("unlocated_case_count")),
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
                "lane": planned_head_lane,
                "source_target": planned_head_handles.get("source"),
                "ctest_target": planned_head_handles.get("catch2_target"),
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
        },
    )


def indexed_active_handoff_record(
    index: JsonObject,
    tests: Sequence[JsonObject],
    requested_family: str | None = None,
) -> JsonObject | None:
    """Resolve the one deliberately selected next slice from the index.

    The normal source/planned queues are intentionally exhaustive.  A small
    ``active_handoff`` card may name either a new case, or an existing mapped
    case that has a concrete open investigation. New cases borrow traceability
    from a mapped seed row; existing cases retain their own direct mapping.
    Both paths use the checked-in index and plan without a Requirements-Lab
    rescan.
    """

    card = index.get("active_handoff")
    if not isinstance(card, dict):
        return None
    if str(card.get("state") or "").strip().casefold() in {
        "complete",
        "completed",
        "closed",
        "done",
        "verified",
    }:
        return None
    family_id = card.get("family")
    if (
        requested_family is not None
        and str(family_id or "").casefold() != requested_family.casefold()
    ):
        return None
    test_case = card.get("test_case")
    if not isinstance(test_case, str) or not test_case.strip():
        return None

    existing = next(
        (
            test
            for test in tests
            if isinstance(test, dict) and test.get("test_case") == test_case
        ),
        None,
    )
    if isinstance(existing, dict):
        plan_id = existing.get("id")
        declared_plan_id = card.get("plan_id")
        if declared_plan_id and declared_plan_id != plan_id:
            return {
                "found": False,
                "state": "unavailable",
                "reason": "active_handoff plan_id does not match its indexed test case",
                "active_handoff_id": card.get("id"),
            }
        summary = test_summary_data(existing)
        pairs = requirement_section_mapping_rows(existing)
        lane = (
            card.get("lane")
            or existing.get("primary_lane")
            or existing.get("focus_lane")
        )
        if not all(
            isinstance(value, str) and value.strip()
            for value in (
                card.get("source_target"),
                card.get("ctest_target"),
                card.get("ctest_filter"),
                lane,
            )
        ):
            return {
                "found": False,
                "state": "unavailable",
                "reason": "active_handoff existing-case pointer is missing source, lane, or CTest handles",
                "active_handoff_id": card.get("id"),
            }
        quoted_plan_id = json.dumps(str(plan_id), ensure_ascii=False)
        return {
            "found": True,
            "runnable": True,
            "handoff_kind": "existing-case",
            "state": "active-handoff",
            "active_handoff_id": card.get("id"),
            "family_id": family_id,
            "plan_id": plan_id,
            "plan_id_status": "indexed",
            "test_case": test_case,
            "source_location": card.get("source_location")
            or existing.get("source_location"),
            "source_target": card.get("source_target"),
            "source_lane": lane,
            "lane_pending": False,
            "ctest_target": card.get("ctest_target"),
            "ctest_filter": card.get("ctest_filter"),
            "mapping_status": "mapped-existing-case",
            "requirement_ids": summary.get("lab_requirement_ids", []),
            "standard_sections": summary.get("standard_sections", []),
            "api_surfaces": summary.get("cpp_api_surfaces", []),
            "requirement_section_pairs": pairs,
            "requirement_section_pair_count": len(pairs),
            "assertions": existing.get("assertions"),
            "status": existing.get("status"),
            "next_action": card.get("objective"),
            "acceptance": card.get("acceptance", []),
            "case_command": (
                f"python tools/query_rti_work.py case {quoted_plan_id} --summary --compact"
            ),
            "matrix_requirement_command": (
                "python tools/query_rti_work.py matrix "
                f"{quoted_plan_id} --group-by requirement --summary --compact"
            ),
            "matrix_section_command": (
                "python tools/query_rti_work.py matrix "
                f"{quoted_plan_id} --group-by section --summary --compact"
            ),
            "trace_command": (
                f"python tools/query_rti_work.py trace {quoted_plan_id} --summary --compact"
            ),
            "implementation_command": (
                f"ctest --test-dir <build-dir> -R {json.dumps(card['ctest_filter'])} --output-on-failure"
            ),
        }

    if family_id == "portable-cpp-tck-2025":
        scenarios_by_id = load_cpp_tck_scenarios()
        plan_id = card.get("plan_id")
        companion_plan_id = card.get("mapping_seed_plan_id")
        scenario = scenarios_by_id.get(plan_id) if isinstance(plan_id, str) else None
        companion = (
            scenarios_by_id.get(companion_plan_id)
            if isinstance(companion_plan_id, str)
            else None
        )
        if (
            isinstance(scenario, dict)
            and str(card.get("state", "")).casefold() != "new-case-needed"
        ):
            contract_refs = list(
                dict.fromkeys(
                    strings(scenario.get("contract_refs"))
                    + (
                        strings(companion.get("contract_refs"))
                        if isinstance(companion, dict)
                        else []
                    )
                )
            )
            requirement_rows: dict[str, ActiveHandoffRequirementRow] = {}
            api_surfaces: set[str] = set()
            for reference in contract_refs:
                contract_path = DEFAULT_CONTRACT_DIRECTORY / reference
                if not contract_path.is_file():
                    continue
                contract = load_json(contract_path)
                if contract.get("kind") == "requirements-reference":
                    requirements = contract.get("requirements", [])
                    if not isinstance(requirements, list):
                        continue
                    for requirement in requirements:
                        if not isinstance(requirement, dict):
                            continue
                        lab_id = requirement.get("requirements_lab_requirement_id")
                        document_id = contract.get("document_id")
                        clause_id = requirement.get("clause_id")
                        if not (
                            isinstance(lab_id, str)
                            and lab_id
                            and isinstance(document_id, str)
                            and document_id
                            and isinstance(clause_id, str)
                            and clause_id
                        ):
                            continue
                        section = f"{document_id}:{clause_id}"
                        requirement_rows.setdefault(
                            lab_id,
                            {
                                "lab_requirement_id": lab_id,
                                "standard_section": section,
                                "document_id": document_id,
                                "clause_id": clause_id,
                                "title": requirement.get("title"),
                                "unresolved": False,
                            },
                        )
                elif contract.get("kind") == "api-reference":
                    api_surface_rows = contract.get("api_surfaces", [])
                    if not isinstance(api_surface_rows, list):
                        continue
                    for api_surface in api_surface_rows:
                        if not isinstance(api_surface, dict):
                            continue
                        api_id = api_surface.get("requirements_lab_api_surface_id")
                        if isinstance(api_id, str) and api_id:
                            api_surfaces.add(api_id)
            # Portable-TCK catalog rows carry their own explicit Lab ID to
            # canonical-section pairs.  A referenced API contract may contain
            # only API-surface data, so relying on requirement-reference
            # contracts alone drops valid, catalog-declared mappings and makes
            # the indexed active handoff appear unavailable.
            for source in (scenario, companion):
                if not isinstance(source, dict):
                    continue
                mappings = source.get("requirement_mappings", [])
                if not isinstance(mappings, list):
                    continue
                for mapping in mappings:
                    if not isinstance(mapping, dict):
                        continue
                    lab_id = mapping.get("requirement_id") or mapping.get(
                        "lab_requirement_id"
                    )
                    section = mapping.get("standard_section")
                    if not isinstance(lab_id, str) or not lab_id:
                        continue
                    if not isinstance(section, str) or ":" not in section:
                        continue
                    document_id, clause_id = section.split(":", 1)
                    if not document_id or not clause_id:
                        continue
                    previous = requirement_rows.get(lab_id)
                    if (
                        isinstance(previous, dict)
                        and previous.get("standard_section") != section
                    ):
                        return {
                            "found": False,
                            "state": "unavailable",
                            "reason": (
                                "portable C++ TCK catalog and requirement contract "
                                f"disagree for {lab_id}"
                            ),
                            "active_handoff_id": card.get("id"),
                        }
                    requirement_rows.setdefault(
                        lab_id,
                        {
                            "lab_requirement_id": lab_id,
                            "standard_section": section,
                            "document_id": document_id,
                            "clause_id": clause_id,
                            "title": mapping.get("title"),
                            "unresolved": False,
                        },
                    )
            pairs = sorted(
                requirement_rows.values(),
                key=lambda row: (row["lab_requirement_id"], row["standard_section"]),
            )
            requirement_ids = sorted({row["lab_requirement_id"] for row in pairs})
            standard_sections = sorted({row["standard_section"] for row in pairs})
            if pairs:
                raw_commands = card.get("commands")
                commands = (
                    {
                        key: value
                        for key, value in raw_commands.items()
                        if isinstance(value, str) and value
                    }
                    if isinstance(raw_commands, dict)
                    else {}
                )
                quoted_requirement_id = json.dumps(
                    requirement_ids[0], ensure_ascii=False
                )
                quoted_standard_section = json.dumps(
                    standard_sections[0], ensure_ascii=False
                )
                return cast(
                    JsonObject,
                    {
                        "found": True,
                        "runnable": True,
                        "handoff_kind": "portable-tck-verification",
                        "state": "active-handoff",
                        "status": str(card.get("state") or "verification-needed"),
                        "active_handoff_id": card.get("id"),
                        "family_id": family_id,
                        "plan_id": card.get("plan_id"),
                        "plan_id_status": "indexed",
                        "test_case": test_case,
                        "source_location": card.get("source_location")
                        or scenario.get("cpp_test"),
                        "source_target": card.get("source_target"),
                        "source_lane": card.get("lane"),
                        "lane_pending": False,
                        "ctest_target": card.get("ctest_target"),
                        "ctest_filter": card.get("ctest_filter"),
                        "mapping_status": "mapped-from-portable-tck-contracts",
                        "mapping_seed_plan_id": card.get("mapping_seed_plan_id"),
                        "mapping_seed_test_case": companion.get("title")
                        if isinstance(companion, dict)
                        else None,
                        "requirement_ids": requirement_ids,
                        "standard_sections": standard_sections,
                        "api_surfaces": sorted(api_surfaces),
                        "requirement_section_pairs": pairs,
                        "requirement_section_pair_count": len(pairs),
                        "matrix_requirement_command": (
                            "python tools/query_rti_work.py matrix "
                            f"{quoted_requirement_id} --group-by requirement --summary --compact"
                        ),
                        "matrix_section_command": (
                            "python tools/query_rti_work.py matrix "
                            f"{quoted_standard_section} --group-by section --summary --compact"
                        ),
                        "next_action": card.get("objective"),
                        "acceptance": card.get("acceptance", []),
                        "commands": commands,
                        "implementation_command": commands.get("focused_tck"),
                    },
                )
            return {
                "found": False,
                "state": "unavailable",
                "reason": "portable C++ TCK handoff has no direct requirement-contract mappings",
                "active_handoff_id": card.get("id"),
            }

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
    api_surfaces = set(
        strings(card.get("api_surfaces"))
        or strings(seed.get("selected_cpp_api_surface_ids"))
    )
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
    quoted_seed_id = json.dumps(str(seed_id), ensure_ascii=False) if seed_id else None
    return cast(
        JsonObject,
        {
            "found": True,
            "runnable": False,
            "handoff_kind": "new-case",
            "state": str(card.get("state") or "new-case-needed"),
            "active_handoff_id": card.get("id"),
            "family_id": family_id,
            "plan_id": card.get("plan_id"),
            "plan_id_status": "proposed" if card.get("plan_id") else None,
            "test_case": test_case,
            "source_location": card.get("source_location"),
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
            "api_surfaces": sorted(api_surfaces),
            "requirement_section_pairs": pairs,
            "requirement_section_pair_count": len(pairs),
            "next_action": card.get("objective"),
            "acceptance": card.get("acceptance", []),
            "commands": commands,
            "seed_trace_command": seed_trace_command,
            "seed_matrix_requirement_command": (
                "python tools/query_rti_work.py matrix "
                f"{quoted_seed_id} --group-by requirement --summary --compact"
                if quoted_seed_id
                else None
            ),
            "seed_matrix_section_command": (
                "python tools/query_rti_work.py matrix "
                f"{quoted_seed_id} --group-by section --summary --compact"
                if quoted_seed_id
                else None
            ),
            "implementation_command": (
                f"python tools/query_rti_work.py case {seed_id} --summary --compact"
                if isinstance(seed_id, str) and seed_id
                else None
            ),
        },
    )


def ready_slice(
    index: JsonObject,
    tests: Sequence[JsonObject],
    source_locations: Mapping[str, Sequence[TestSourceLocation]],
    requested_family: str | None = None,
    include_source_only: bool = False,
    standard_requirements: Mapping[str, JsonObject] | None = None,
) -> JsonObject:
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
        return cast(
            JsonObject,
            {
                "found": False,
                "state": "none",
                "requested_family": requested_family,
                "include_source_only": include_source_only,
                "reason": active.get("reason") or "no indexed work item is open",
            },
        )

    requested_item = next(
        (
            item
            for item in json_object_rows(index.get("items"))
            if str(item.get("id") or "").casefold()
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
        else json_object_value(active.get("parent_item")) or {}
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
            "next_source_api_surfaces": requested_item.get("next_source_api_surfaces"),
            "next_source_ctest_filter": requested_item.get("next_source_ctest_filter"),
            "next_test_query": requested_item.get("next_test_query"),
            "next_test_role": requested_item.get("next_test_role"),
            "next_work_id": requested_item.get("next_work_id"),
            "work_id": requested_item.get("id"),
            "work_status": requested_item.get("status"),
        }
    record: JsonObject | None = None

    # An explicitly indexed source declaration wins over a planned-row queue
    # because it is already present in the checkout and can be run/mapped.
    if source_state == "unplanned-source" and active.get("next_source_test_query"):
        record = cast(
            JsonObject,
            {
                "state": "unplanned-source",
                "family_id": parent.get("id"),
                "test_case": active.get("next_source_test_query"),
                "source_location": active.get("next_source_location"),
                "source_lane": active.get("next_source_lane"),
                "requirement_ids": strings(active.get("next_source_requirement_ids")),
                "standard_sections": strings(
                    active.get("next_source_standard_sections")
                ),
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
            },
        )
    else:
        global_queue = json_object_value(active.get("global_source_queue"))
        if (
            include_source_only
            and source_state == "exhausted"
            and isinstance(global_queue, dict)
            and global_queue.get("count", 0)
        ):
            record = cast(
                JsonObject,
                {
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
                },
            )

        if record is None:
            planned_queue = (
                indexed_unlocated_plan_summary(index, tests, requested_family)
                if requested_family is not None
                else json_object_value(active.get("planned_queue")) or {}
            )
            head = json_object_value(planned_queue.get("head"))
            if head is not None and head.get("test_case"):
                exact = next(
                    (
                        test
                        for test in tests
                        if test.get("test_case") == head.get("test_case")
                    ),
                    None,
                )
                record = cast(
                    JsonObject,
                    {
                        "state": "planned",
                        "family_id": head.get("family_id") or parent.get("id"),
                        "plan_id": head.get("plan_id"),
                        "test_case": head.get("test_case"),
                        "source_location": None,
                        "source_target": head.get("source_target"),
                        "source_lane": head.get("lane"),
                        "ctest_target": head.get("ctest_target"),
                        "requirement_ids": strings(head.get("requirement_ids")),
                        "standard_sections": strings(head.get("standard_sections")),
                        "api_surfaces": strings(head.get("api_surfaces")),
                        "assertions": head.get("assertions"),
                        "ctest_filter": head.get("ctest_filter"),
                        "trace_command": head.get("trace_command"),
                        "next_action": exact.get("next_action")
                        if exact is not None
                        else None,
                        "implementation_command": (
                            f'python tools/query_rti_work.py trace "{head["test_case"]}" '
                            "--summary --compact"
                        ),
                    },
                )
                if exact is not None:
                    exact_summary = test_summary_data(exact)
                    record.update(
                        {
                            "status": exact_summary.get("status"),
                            "tags": strings(exact.get("tags")),
                            "source_locations": exact_summary.get(
                                "source_locations", []
                            ),
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
        for lane_row in json_object_rows(mapping_queue.get("lanes")):
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
            locations = json_object_rows(exact.get("source_locations"))
            location = locations[0] if locations else None
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
            record = cast(
                JsonObject,
                {
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
                },
            )
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
                "requirement_ids_preview": row.get("requirement_ids_preview", []),
                "requirement_ids_remaining": row.get("requirement_ids_remaining", 0),
                "standard_sections_preview": row.get("standard_sections_preview", []),
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
                "explicit_disposition_count": row.get("explicit_disposition_count", 0),
                "next_test_mapping": (
                    {
                        "assertion_count": next_test_pointer.get("assertion_count", 0),
                        "requirement_count": next_test_pointer.get(
                            "requirement_count", 0
                        ),
                        "standard_section_count": next_test_pointer.get(
                            "standard_section_count", 0
                        ),
                        "cpp_api_surface_count": next_test_pointer.get(
                            "cpp_api_surface_count", 0
                        ),
                    }
                    if isinstance(
                        (next_test_pointer := row.get("next_test_pointer")), dict
                    )
                    and next_test_pointer.get("query")
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
                    + (
                        "--focused "
                        if row.get("focused_lane_count", 0)
                        else "--unmapped "
                    )
                    + "--summary --compact --limit 12"
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
                "matrix_requirement_command": (
                    f"python tools/query_rti_work.py matrix {row.get('id')} "
                    "--group-by requirement --summary --compact"
                    if row.get("id")
                    else None
                ),
                "matrix_section_command": (
                    f"python tools/query_rti_work.py matrix {row.get('id')} "
                    "--group-by section --summary --compact"
                    if row.get("id")
                    else None
                ),
            }
            for row in json_object_rows(family_queue.get("items"))
            if (
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
            # A deliberate new-case or backlog-shaping queue may show one
            # bounded coverage-inventory pointer. Evidence-complete and
            # external-review families retain their existing handles and are
            # not presented as implementation work.
            if option.get("action_state") not in {
                "new-case-needed",
                "backlog-shaping",
            }:
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
                if isinstance(option, dict) and option.get("id") == requested_family
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
            action_states = {
                str(option.get("action_state"))
                for option in family_options
                if isinstance(option, dict)
            }
            if action_states <= {"backlog-shaping", "external-review"}:
                reason = (
                    "no executable C++ implementation is queued; only "
                    "backlog-shaping and external-review actions remain"
                )
            else:
                reason = (
                    "the indexed source and planned-row queues are exhausted; "
                    "only explicitly queued family actions remain"
                )
        else:
            reason = (
                "the indexed implementation queues are exhausted; "
                "no family has a runnable or explicitly queued action"
            )
        return cast(
            JsonObject,
            {
                "found": False,
                "state": "none",
                # Keep ``state=none`` for callers that already distinguish an
                # executable handoff from the bounded family fallback.  These
                # explicit fields make the fallback actionable in JSON without
                # requiring a second roadmap or matrix query.
                "selection_required": bool(family_options),
                "recommended_family_id": (
                    family_options[0].get("id")
                    if family_options and isinstance(family_options[0], dict)
                    else None
                ),
                "family_choice_command": (
                    "python tools/query_rti_work.py ready --family <family-id> "
                    "--summary --compact"
                    if family_options
                    else None
                ),
                "requested_family": requested_family,
                "include_source_only": include_source_only,
                "owner": parent,
                "reason": reason,
                "family_options": family_options,
                "family_queue_count": len(family_options),
                "evidence_complete_families": [
                    row.get("id")
                    for row in json_object_rows(family_queue.get("items"))
                    if row.get("action_state") == "evidence-complete"
                ],
            },
        )

    family_id = record.get("family_id")
    # Global planned queues can surface a case through a broad taxonomy tag
    # shared by several roadmap families.  For a case with an explicit lane,
    # the lane-owner index is the narrow, authoritative handoff destination;
    # otherwise a perfectly mapped planned test can be shown under an
    # unrelated high-priority family.  Keep the queued action aligned with
    # its case/focus/check handles.
    mapping = index.get("mapping", {})
    lane_owners = mapping.get("lane_owners", {}) if isinstance(mapping, dict) else {}
    source_lane = record.get("source_lane")
    lane_owner = (
        lane_owners.get(source_lane)
        if isinstance(lane_owners, dict) and isinstance(source_lane, str)
        else None
    )
    if isinstance(lane_owner, str) and any(
        item.get("id") == lane_owner for item in json_object_rows(index.get("items"))
    ):
        family_id = lane_owner
        record["family_id"] = family_id
    family_item = next(
        (
            item
            for item in json_object_rows(index.get("items"))
            if item.get("id") == family_id
        ),
        None,
    )
    record["found"] = True
    record["requested_family"] = requested_family
    record["include_source_only"] = include_source_only
    case_query = record.get("plan_id")
    if record.get("state") == "new-case-needed":
        # The proposed ID is reserved for the future plan row, not yet
        # queryable as a Catch2 case. Surface seed commands instead of giving
        # callers a case/matrix command that cannot resolve until the row is
        # added.
        case_query = None
        if record.get("plan_id"):
            record["plan_id_status"] = "proposed"
    elif not case_query and record.get("state") in {"planned", "mapping"}:
        case_query = record.get("test_case")
    exact_record_test = next(
        (
            test
            for test in tests
            if isinstance(test, dict)
            and (
                test.get("id") == record.get("plan_id")
                or test.get("test_case") == record.get("test_case")
            )
        ),
        None,
    )
    if isinstance(exact_record_test, dict):
        exact_pairs = requirement_section_mapping_rows(exact_record_test)
        record["requirement_section_pairs"] = exact_pairs
        record["requirement_section_pair_count"] = len(exact_pairs)
    if isinstance(case_query, str) and case_query:
        if record.get("handoff_kind") == "portable-tck-verification":
            record["case_command"] = f"python tools/cpp_tck.py --scenario {case_query}"
        else:
            record["case_command"] = (
                f'python tools/query_rti_work.py case "{case_query}" '
                "--summary --compact"
            )
            record["matrix_requirement_command"] = (
                f'python tools/query_rti_work.py matrix "{case_query}" '
                "--group-by requirement --summary --compact"
            )
            record["matrix_section_command"] = (
                f'python tools/query_rti_work.py matrix "{case_query}" '
                "--group-by section --summary --compact"
            )
    record["owner"] = parent
    record["roadmap_owner"] = (
        roadmap_item_summary(family_item) if isinstance(family_item, dict) else parent
    )
    record["active_pointer"] = parent
    # A planned or mapping handoff is owned by the family that contains the
    # exact row, even when the global queue was reached through a different
    # historical active pointer.  Returning that family id keeps the resume
    # card's ``work`` handle aligned with its ``trace``/``focus`` handles.
    record["work_id"] = (
        family_id
        if record.get("state")
        in {"planned", "mapping", "new-case-needed", "active-handoff"}
        and family_id
        else parent.get("id")
    )
    record["active_work_id"] = active.get("work_id")
    record["work_status"] = active.get("work_status")
    active_planned_queue = json_object_value(active.get("planned_queue"))
    record["planned_queue_count"] = (
        json_integer(active_planned_queue.get("count"))
        if active_planned_queue is not None
        else 0
    )
    record["planned_queue_diagnostic_count"] = (
        json_integer(
            active_planned_queue.get("diagnostic_count"),
            json_integer(record.get("planned_queue_count")),
        )
        if active_planned_queue is not None
        else record.get("planned_queue_count", 0)
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
    if record.get("family_id") == "portable-cpp-tck-2025":
        record["check_command"] = (
            f"python tools/cpp_tck.py --scenario {record['plan_id']}"
            if record.get("plan_id")
            else None
        )
        if record.get("lane_pending"):
            # Portable TCK cases are cataloged independently of Catch2 lanes.
            # Their post-mapping validation must check the C++ TCK catalog,
            # not issue a misleading Catch2 focus/check command.
            record["post_mapping_check_command"] = (
                f"python tools/cpp_tck.py --scenario {record['plan_id']}"
                if record.get("plan_id")
                else None
            )
    elif record.get("lane_pending"):
        record["post_mapping_focus_command"] = lane_command
        record["post_mapping_check_command"] = check_command
    else:
        record["focus_command"] = lane_command
        record["check_command"] = check_command
    return record


def trace_requirement_row(requirement: JsonObject) -> JsonObject:
    """Return the direct Lab-requirement to 2025-subsection relationship."""

    standard = requirement.get("standard")
    if not isinstance(standard, dict):
        return cast(
            JsonObject,
            {
                "lab_requirement_id": requirement.get("lab_requirement_id"),
                "unresolved": True,
            },
        )
    mapping = requirement.get("section_mapping")
    mapping = mapping if isinstance(mapping, dict) else None
    source = standard.get("source")
    source_value = source if isinstance(source, dict) else None
    row = cast(
        JsonObject,
        {
            "lab_requirement_id": requirement.get("lab_requirement_id"),
            "document_id": (
                mapping.get("document_id")
                if mapping is not None
                else standard.get("document_id")
            ),
            "clause": (
                mapping.get("clause") if mapping is not None else standard.get("clause")
            ),
            "clause_id": (
                mapping.get("clause_id")
                if mapping is not None
                else standard.get("clause_id")
            ),
            "title": (
                mapping.get("title") if mapping is not None else standard.get("title")
            ),
            "statement": standard.get("statement") or standard.get("machine_statement"),
            "kind": standard.get("kind"),
            "coverage_kind": standard.get("coverage_kind"),
            "source": source_value,
            "unresolved": False,
        },
    )
    if mapping is not None and mapping.get("requirements_lab_issue_id"):
        row["exported_standard_section"] = mapping.get("exported_standard_section")
        row["requirements_lab_issue_id"] = mapping.get("requirements_lab_issue_id")
        row["mapping_basis"] = mapping.get("mapping_basis")
    return row


def requirement_section_mapping_rows(
    test: JsonObject,
) -> list[JsonObject]:
    """Return the direct Lab-requirement to canonical subsection pairs.

    The Catch2 plan stores requirement selections while the 2025 subsection is
    resolved from the pinned CorpusBundle. Source-verified export defects may
    specify a per-requirement override in
    ``requirement_section_mapping_overrides``; the returned row preserves the
    exported section and issue id so this exception is visible and auditable.
    Keep that join in one helper so every bounded query (not only ``matrix``)
    can expose the same relationship without asking a caller to infer it by
    zipping two independently ordered arrays.
    """

    overrides: dict[str, RequirementSectionOverride] = {}
    raw_overrides = test.get("requirement_section_mapping_overrides")
    override_rows = raw_overrides if isinstance(raw_overrides, list) else []
    for raw_override in override_rows:
        if not isinstance(raw_override, dict):
            continue
        override = cast(JsonObject, raw_override)
        requirement_id = override.get("lab_requirement_id")
        section = override.get("standard_section")
        if (
            not isinstance(requirement_id, str)
            or not requirement_id
            or not isinstance(section, str)
            or ":" not in section
        ):
            continue
        document_id, clause_id = section.split(":", 1)
        if not document_id or not clause_id:
            continue
        overrides[requirement_id] = {
            "standard_section": section,
            "document_id": document_id,
            "clause_id": clause_id,
            "title": override.get("title"),
            "clause": override.get("clause"),
            "requirements_lab_issue_id": override.get("requirements_lab_issue_id"),
            "basis": override.get("basis"),
        }

    rows: list[JsonObject] = []
    raw_requirements = test.get("requirements")
    requirement_rows = raw_requirements if isinstance(raw_requirements, list) else []
    for raw_requirement in requirement_rows:
        if not isinstance(raw_requirement, dict):
            continue
        requirement = cast(JsonObject, raw_requirement)
        requirement_id = requirement.get("lab_requirement_id")
        standard = requirement.get("standard")
        override = (
            overrides.get(requirement_id) if isinstance(requirement_id, str) else None
        )
        if not isinstance(standard, dict):
            if override is not None:
                rows.append(
                    {
                        "lab_requirement_id": requirement_id,
                        "standard_section": override["standard_section"],
                        "document_id": override["document_id"],
                        "clause_id": override["clause_id"],
                        "title": override.get("title"),
                        "clause": override.get("clause")
                        or override["clause_id"].removeprefix("clause-"),
                        "exported_standard_section": None,
                        "requirements_lab_issue_id": override.get(
                            "requirements_lab_issue_id"
                        ),
                        "mapping_basis": override.get("basis"),
                        "unresolved": False,
                    }
                )
                continue
            rows.append(
                {
                    "lab_requirement_id": requirement_id,
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
        exported_section = (
            f"{document_id}:{clause_id}"
            if isinstance(document_id, str)
            and document_id
            and isinstance(clause_id, str)
            and clause_id
            else None
        )
        section = (
            override["standard_section"] if override is not None else exported_section
        )
        resolved_document_id = (
            override["document_id"] if override is not None else document_id
        )
        resolved_clause_id = (
            override["clause_id"] if override is not None else clause_id
        )
        rows.append(
            {
                "lab_requirement_id": requirement_id,
                "standard_section": section,
                "document_id": resolved_document_id,
                "clause_id": resolved_clause_id,
                "title": (
                    override.get("title")
                    if override is not None
                    else standard.get("title")
                ),
                "clause": (
                    override.get("clause")
                    or override["clause_id"].removeprefix("clause-")
                    if override is not None
                    else standard.get("clause")
                ),
                "exported_standard_section": (
                    exported_section if override is not None else None
                ),
                "requirements_lab_issue_id": (
                    override.get("requirements_lab_issue_id")
                    if override is not None
                    else None
                ),
                "mapping_basis": (
                    override.get("basis") if override is not None else None
                ),
                "unresolved": section is None,
            }
        )
    return rows


def exact_requirement_match(requirement: JsonObject, query: str) -> bool:
    """Match only stable requirement/contract identifiers for ``trace``."""

    query_folded = query.casefold()
    values = strings(requirement.get("lab_requirement_id"))
    for contract in json_object_rows(requirement.get("contracts")):
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
    # search_key removes dots, so 4.1.1 and 4.11 otherwise collapse to 411.
    # Preserve numeric clause boundaries before accepting normalized aliases.
    section_clause = re.fullmatch(
        r"clause[-_ ]*([0-9]+(?:\.[0-9]+)*)",
        section.rsplit(":", 1)[-1].strip(),
        flags=re.IGNORECASE,
    )
    query_clause = re.search(
        r"(?:^|[:\s])(?:clause[-_ ]*)?([0-9]+(?:\.[0-9]+)*)\s*$",
        query,
        flags=re.IGNORECASE,
    )
    if (
        section_clause
        and query_clause
        and section_clause.group(1) != query_clause.group(1)
    ):
        return False
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
    tests: Sequence[JsonObject],
    query: str,
) -> tuple[str | None, list[JsonObject]]:
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
            for requirement in json_object_rows(test.get("requirements"))
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
    index: JsonObject,
    tests: Sequence[JsonObject],
    query: str,
) -> tuple[str | None, list[JsonObject]]:
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
    test: JsonObject,
    match_kind: str,
    query: str,
    index: JsonObject | None = None,
) -> JsonObject:
    """Build a bounded, direct test-to-requirement trace record."""

    requirements = json_object_rows(test.get("requirements"))
    section_mappings_by_requirement: dict[str, JsonObject] = {}
    for row in requirement_section_mapping_rows(test):
        lab_id = row.get("lab_requirement_id")
        if isinstance(lab_id, str) and lab_id:
            section_mappings_by_requirement[lab_id] = row
    if match_kind == "requirement":
        requirements = [
            requirement
            for requirement in requirements
            if exact_requirement_match(requirement, query)
        ]
    elif match_kind == "section":
        section_requirements: list[JsonObject] = []
        for requirement in requirements:
            lab_id = requirement.get("lab_requirement_id")
            if not isinstance(lab_id, str):
                continue
            section_mapping = section_mappings_by_requirement.get(lab_id)
            section = (
                section_mapping.get("standard_section")
                if section_mapping is not None
                else None
            )
            if isinstance(section, str) and trace_section_match(section, query):
                section_requirements.append(requirement)
        requirements = section_requirements
    trace_requirements: list[JsonObject] = []
    for requirement in requirements:
        lab_id = requirement.get("lab_requirement_id")
        mapping = (
            section_mappings_by_requirement.get(lab_id)
            if isinstance(lab_id, str)
            else None
        )
        trace_requirement = dict(requirement)
        if isinstance(mapping, dict):
            trace_requirement["section_mapping"] = mapping
        trace_requirements.append(trace_requirement)
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
        "replacement_plan_ids": strings(test.get("replacement_plan_ids")),
        "tags": strings(test.get("tags")),
        "cpp_api_surfaces": strings(test.get("selected_cpp_api_surface_ids")),
        "standard_sections": strings(test.get("standard_sections")),
        "roadmap_items": roadmap_items,
        "roadmap_owner": roadmap_owner.get("id") if roadmap_owner else None,
        "requirement_mapping_count": len(strings(test.get("lab_requirement_ids"))),
        "matched_requirement_count": len(requirements),
        "requirement_mappings": [
            trace_requirement_row(requirement) for requirement in trace_requirements
        ],
    }


def trace_summary_record(record: JsonObject, limit: int = 8) -> JsonObject:
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
    test: JsonObject,
    index: JsonObject,
    query: str,
) -> JsonObject:
    """Build one bounded handoff card for an exact Catch2 plan case."""

    trace = trace_record(test, "test", query, index)
    roadmap_items = roadmap_links_for_test(index, test)
    owner = primary_roadmap_link(roadmap_items)
    mapping = index.get("mapping")
    lane_handles_by_tag: JsonObject = {}
    if isinstance(mapping, dict):
        configured_lanes = mapping.get("lane_handles")
        if isinstance(configured_lanes, dict):
            lane_handles_by_tag = configured_lanes
    lane: str | None = None
    lane_handles: JsonObject = {}
    lane_tags = strings(test.get("tags"))
    primary_lane = str(test.get("primary_lane") or "").strip()
    if primary_lane:
        lane_tags = [primary_lane] + [
            tag for tag in lane_tags if tag.casefold() != primary_lane.casefold()
        ]
        lane = primary_lane
        configured = lane_handles_by_tag.get(primary_lane)
        if isinstance(configured, dict):
            lane_handles = configured
    if lane is None:
        for tag in lane_tags:
            configured = lane_handles_by_tag.get(tag)
            if isinstance(configured, dict):
                lane = tag
                lane_handles = configured
                break

    case_id = str(test.get("id") or query)
    case_title = str(test.get("test_case") or case_id)
    quoted_id = json.dumps(case_id, ensure_ascii=False)
    commands: dict[str, str] = {
        "test": (
            f"python tools/query_rti_work.py test {quoted_id} --summary --compact"
        ),
        "trace": (
            f"python tools/query_rti_work.py trace {quoted_id} --summary --compact"
        ),
        "matrix": (
            f"python tools/query_rti_work.py matrix {quoted_id} --summary --compact"
        ),
        "matrix_requirement": (
            "python tools/query_rti_work.py matrix "
            f"{quoted_id} --group-by requirement --summary --compact"
        ),
        "matrix_section": (
            "python tools/query_rti_work.py matrix "
            f"{quoted_id} --group-by section --summary --compact"
        ),
    }
    if lane:
        commands["focus"] = (
            f"python tools/query_rti_work.py focus {lane} --summary --compact"
        )
        commands["check"] = (
            f"python tools/query_rti_work.py check --lane {lane} --summary --compact"
        )
    case_ctest_filter = test.get("ctest_filter")
    if isinstance(case_ctest_filter, str) and case_ctest_filter.strip():
        commands["ctest"] = (
            "ctest --test-dir <build-dir> -C Debug -R "
            f'"{case_ctest_filter}" --output-on-failure'
        )
    else:
        ctest = lane_ctest_command(lane_handles)
        if ctest:
            commands["ctest"] = ctest

    has_case_source = bool(test.get("source_location") or test.get("source_locations"))
    source_target = test.get("source_target")
    if not source_target and not has_case_source:
        source_target = lane_handles.get("source")

    return {
        "query": query,
        "id": case_id,
        "test_case": case_title,
        "status": test.get("status"),
        "traceability_state": traceability_state(test),
        "source_locations": test.get("source_locations", []),
        "source_target": source_target,
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
        "replacement_plan_ids": strings(test.get("replacement_plan_ids")),
        "next_action": test.get("next_action"),
        "roadmap_owner": owner.get("id") if owner else None,
        "roadmap_items": roadmap_items,
        "lane": lane,
        "lane_handles": lane_handles,
        "trace": trace_summary_record(trace),
        "matrix": matrix_summary_data(test, index),
        "commands": commands,
    }


def text_case_card(card: JsonObject, compact: bool = False) -> str:
    """Render one exact case card without expanding unrelated plan rows."""

    requirements = strings(card.get("requirements"))
    sections = strings(card.get("standard_sections"))
    api_surfaces = strings(card.get("cpp_api_surfaces"))
    mappings = card.get("requirement_section_mappings")
    if not isinstance(mappings, list):
        mappings = []

    def preview(values: Sequence[str], limit: int = 8) -> str:
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
    source_text = source_location_text(card)
    if is_planned_test(card) and card.get("source_target"):
        source_text = "<TEST_CASE not declared yet>"
    assertions = card.get("assertions")
    if assertions is None:
        if is_planned_test(card):
            assertions = "<not recorded; case is not implemented>"
        else:
            assertions = "<unspecified>"

    lines = [
        f"Case card: {card.get('id', '<unnamed>')}: {card.get('test_case', '<unnamed test>')}",
        f"status: {card.get('status', '<unspecified>')}",
        f"traceability: {card.get('traceability_state', '<unspecified>')}",
        f"source: {source_text}",
        (
            f"assertions: {assertions}; "
            f"callback_models={', '.join(strings(card.get('callback_models'))) or '<unspecified>'}"
        ),
        f"roadmap_owner: {card.get('roadmap_owner') or '<none>'}",
        f"lane: {card.get('lane') or '<none>'}",
        (
            f"mapping: requirements={len(requirements)} sections={len(sections)} "
            f"direct_pairs={len(mapping_preview)} api_surfaces={len(api_surfaces)}"
        ),
        f"requirements: {preview(requirements)}",
        f"standard_sections: {preview(sections)}",
        f"requirement_section_mappings: {preview(mapping_preview)}",
        f"cpp_api_surfaces: {preview(api_surfaces)}",
    ]
    source_target = card.get("source_target")
    if isinstance(source_target, str) and source_target.strip():
        lines.append(f"source_target: {source_target}")
    lines.append("commands:")
    disposition = card.get("requirements_lab_api_surface_status")
    if isinstance(disposition, str) and disposition.strip():
        lines.insert(
            3,
            "Requirements-Lab note: " + compact_prose(disposition, 360),
        )
    next_action = card.get("next_action")
    if isinstance(next_action, str) and next_action.strip():
        lines.insert(4, "next_action: " + compact_prose(next_action, 360))
    replacements = strings(card.get("replacement_plan_ids"))
    if replacements:
        lines.insert(5, "replacement_plan_ids: " + ", ".join(replacements))
    commands = card.get("commands")
    if isinstance(commands, dict):
        for name in (
            "test",
            "trace",
            "matrix",
            "matrix_requirement",
            "matrix_section",
            "focus",
            "check",
            "ctest",
        ):
            command = commands.get(name)
            if isinstance(command, str) and command:
                lines.append(f"  {name}={command}")
    return "\n".join(lines)


def text_trace(record: JsonObject, summary: bool = False) -> str:
    """Render one direct test → requirement → subsection trace."""

    def preview(values: Sequence[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    tags = strings(record.get("tags"))
    api_surfaces = strings(record.get("cpp_api_surfaces"))
    roadmap_items = json_object_rows(record.get("roadmap_items"))
    roadmap_owner = primary_roadmap_link(roadmap_items)
    lines = [
        f"- {record.get('id', '<unnamed>')}: {record.get('test_case', '<unnamed test>')}",
        f"  status: {record.get('status', '<unspecified>')}",
        f"  source: {source_location_text(record)}",
        f"  mapping id: {record.get('requirements_lab_mapping_id') or '<none>'}",
        "  replacement plan ids: "
        + (", ".join(strings(record.get("replacement_plan_ids"))) or "<none>"),
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
                f"{item.get('id')} ({item.get('match')})" for item in roadmap_items
            )
            or "<none>"
        ),
        "  requirement mappings:",
    ]
    failure_location = record.get("failure_location")
    if isinstance(failure_location, str) and failure_location.strip():
        lines.append(f"  failure_location: {failure_location}")
    failure_summary = record.get("failure_summary")
    if isinstance(failure_summary, str) and failure_summary.strip():
        lines.append(f"  failure_summary: {compact_prose(failure_summary, 240)}")
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
            "  Requirements-Lab API surface: " + compact_prose(api_surface_status, 360),
        )
    mappings = json_object_rows(record.get("requirement_mappings"))
    if not mappings:
        lines.append("    - <none>")
        return "\n".join(lines)
    shown = mappings[:8] if summary else mappings
    for mapping in shown:
        requirement_id = mapping.get("lab_requirement_id", "<unnamed requirement>")
        if mapping.get("unresolved"):
            lines.append(f"    - {requirement_id} (unresolved)")
            continue
        section = (
            ":".join(
                str(value)
                for value in (mapping.get("document_id"), mapping.get("clause_id"))
                if value
            )
            or "<unmapped section>"
        )
        override_note = ""
        issue_id = mapping.get("requirements_lab_issue_id")
        if issue_id:
            exported = mapping.get("exported_standard_section") or "<unresolved>"
            override_note = (
                f" (source-backed override; Lab export={exported}; issue={issue_id})"
            )
        if summary:
            lines.append(f"    - {requirement_id} -> {section}{override_note}")
        else:
            title = mapping.get("title") or ""
            lines.append(f"    - {requirement_id} -> {section}; {title}{override_note}")
            statement = mapping.get("statement")
            if isinstance(statement, str) and statement.strip():
                lines.append(f"      statement: {compact_prose(statement, 220)}")
    remaining = len(mappings) - len(shown)
    if remaining > 0:
        lines.append(f"    - ... (+{remaining}); use trace --json for all rows")
    return "\n".join(lines)


def matrix_summary_data(
    test: JsonObject,
    index: JsonObject,
) -> JsonObject:
    """Return one exact test-to-standard row for the bounded matrix view."""

    result = test_summary_data(test)
    result["tags"] = strings(test.get("tags"))
    roadmap_items = roadmap_links_for_test(index, test)
    result["roadmap_items"] = roadmap_items
    roadmap_owner = primary_roadmap_link(roadmap_items)
    result["roadmap_owner"] = roadmap_owner.get("id") if roadmap_owner else None
    return result


def text_matrix_row(
    test: JsonObject,
    index: JsonObject,
    compact: bool = False,
    mapping_rows: Sequence[JsonObject] | None = None,
) -> str:
    """Render one compact test → requirement → subsection matrix row."""

    requirements = strings(test.get("lab_requirement_ids"))
    sections = strings(test.get("standard_sections"))
    api_surfaces = strings(
        test.get("selected_cpp_api_surface_ids") or test.get("cpp_api_surfaces")
    )
    raw_roadmap_items = test.get("roadmap_items")
    roadmap_items = (
        json_object_rows(raw_roadmap_items)
        if isinstance(raw_roadmap_items, list)
        else []
    )
    if not isinstance(raw_roadmap_items, list):
        roadmap_items = roadmap_links_for_test(index, test)

    def preview(values: Sequence[str], limit: int = 8) -> str:
        if not values:
            return "<none>"
        if not compact:
            return ", ".join(values)
        shown = ", ".join(values[:limit])
        remaining = len(values) - limit
        return f"{shown}, ... (+{remaining})" if remaining > 0 else shown

    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        (
            f"  status={test.get('status', '<unspecified>')} "
            f"source={source_location_text(test)} assertions={test.get('assertions', '<unspecified>')} "
            f"requirements={len(requirements)} sections={len(sections)} api_surfaces={len(api_surfaces)}"
        ),
        f"  requirements: {preview(requirements)}",
        f"  standard_sections: {preview(sections)}",
    ]
    requirement_section_mappings = mapping_rows
    if requirement_section_mappings is None:
        raw_mappings = test.get("requirement_section_mappings")
        if isinstance(raw_mappings, list):
            requirement_section_mappings = json_object_rows(raw_mappings)
        else:
            summary_mappings = matrix_summary_data(test, index).get(
                "requirement_section_mappings"
            )
            requirement_section_mappings = json_object_rows(summary_mappings)
    if requirement_section_mappings:
        mapping_values = [
            f"{row.get('lab_requirement_id', '<unnamed requirement>')} -> "
            f"{row.get('standard_section') or '<unresolved>'}"
            for row in requirement_section_mappings
        ]
        if mapping_values:
            lines.append(f"  requirement_section_mappings: {preview(mapping_values)}")
    if roadmap_items:
        owner = primary_roadmap_link(roadmap_items)
        if owner and owner.get("id"):
            lines.append(f"  roadmap_owner: {owner['id']} ({owner.get('match')})")
        owners = [str(item.get("id")) for item in roadmap_items if item.get("id")]
        if owners:
            lines.append(f"  roadmap: {', '.join(owners)}")
    if api_surfaces and not compact:
        lines.append(f"  cpp_api_surfaces: {preview(api_surfaces)}")
    return "\n".join(lines)


def matrix_crosswalk_rows(
    tests: Sequence[JsonObject],
    group_by: str,
    *,
    match_kind: str | None = None,
    query: str | None = None,
) -> list[JsonObject]:
    """Aggregate direct requirement/subsection pairs for a bounded crosswalk.

    ``matrix`` normally emits one row per Catch2 case.  That is the right
    shape for executing a test, but it makes a standards review unnecessarily
    expensive when the question is "which cases cover this requirement (or
    subsection)?"  This helper keeps the same direct join used by the normal
    matrix and groups it by one stable key without inferring coverage from
    broad tags or independently ordered arrays.

    The returned rows are deliberately compact: each group carries exact
    counts plus a bounded case preview.  ``remaining_test_count`` preserves
    the aggregate size when a heavily reused requirement would otherwise make
    the output unbounded.  When ``match_kind``/``query`` identify one exact
    requirement or subsection, only that direct pair is retained; family and
    lane queries remain whole-slice aggregates.
    """

    if group_by not in {"requirement", "section"}:
        raise ValueError("matrix crosswalk group_by must be requirement or section")

    # Reverse lookups by one exact requirement or subsection should remain
    # narrow even when the matching Catch2 case carries several mappings.
    # Family/lane/active-lane queries intentionally retain every direct pair
    # so they provide the aggregate crosswalk for that bounded work slice.
    filter_requirement = (
        isinstance(query, str) and bool(query.strip()) and match_kind == "requirement"
    )
    filter_section = (
        isinstance(query, str) and bool(query.strip()) and match_kind == "section"
    )

    groups: dict[str, MatrixCrosswalkAccumulator] = {}
    for test in tests:
        pairs = requirement_section_mapping_rows(test)
        # A mapped test may carry a legacy standard-section field without a
        # direct Lab join.  It must not silently become standards coverage in
        # this view; only explicit requirement -> subsection pairs count.
        for pair in pairs:
            requirement_id = str(pair.get("lab_requirement_id") or "").strip()
            section = str(pair.get("standard_section") or "").strip()
            if filter_requirement:
                matching_requirement = any(
                    str(requirement.get("lab_requirement_id") or "").strip()
                    == requirement_id
                    and exact_requirement_match(requirement, str(query))
                    for requirement in json_object_rows(test.get("requirements"))
                )
                if not matching_requirement:
                    continue
            elif filter_section and (
                not section or not trace_section_match(section, str(query))
            ):
                continue
            key = requirement_id if group_by == "requirement" else section
            if not key:
                key = (
                    "<unresolved requirement>"
                    if group_by == "requirement"
                    else "<unresolved section>"
                )
            group = groups.setdefault(
                key,
                cast(
                    MatrixCrosswalkAccumulator,
                    {
                        "group_by": group_by,
                        "key": key,
                        "lab_requirement_id": requirement_id or None,
                        "standard_section": section or None,
                        "standard_sections": set(),
                        "lab_requirement_ids": set(),
                        "requirement_titles": {},
                        "tests": [],
                        "assertion_count": 0,
                        "assertions_unspecified_test_count": 0,
                        "pair_count": 0,
                    },
                ),
            )
            if requirement_id:
                group["lab_requirement_ids"].add(requirement_id)
            if section:
                group["standard_sections"].add(section)
            title = pair.get("title")
            if requirement_id and isinstance(title, str) and title.strip():
                group["requirement_titles"].setdefault(requirement_id, title)
            group["pair_count"] += 1

            test_id = str(test.get("id") or "<unnamed>")
            seen_test_ids = {
                entry_id
                for entry in group["tests"]
                if isinstance(entry_id := entry.get("id"), str)
            }
            if test_id not in seen_test_ids:
                assertions = test.get("assertions")
                if isinstance(assertions, int):
                    group["assertion_count"] += assertions
                else:
                    group["assertions_unspecified_test_count"] += 1
                group["tests"].append(
                    {
                        "id": test_id,
                        "test_case": test.get("test_case"),
                        "status": test.get("status"),
                        "source_locations": cast(
                            JsonValue, json_object_rows(test.get("source_locations"))
                        ),
                        "assertions": assertions,
                    }
                )

    rows: list[JsonObject] = []
    for key, group in sorted(groups.items(), key=lambda item: item[0].casefold()):
        tests_for_group = sorted(
            group["tests"],
            key=lambda test: (
                str(test.get("id") or "").casefold(),
                str(test.get("test_case") or "").casefold(),
            ),
        )
        row = cast(
            JsonObject,
            {
                "group_by": group["group_by"],
                "key": key,
                "lab_requirement_id": group["lab_requirement_id"],
                "standard_section": group["standard_section"],
                "lab_requirement_ids": sorted(group["lab_requirement_ids"]),
                "standard_sections": sorted(group["standard_sections"]),
                "requirement_titles": dict(sorted(group["requirement_titles"].items())),
                "pair_count": group["pair_count"],
                "test_count": len(tests_for_group),
                "assertion_count": group["assertion_count"],
                "assertions_unspecified_test_count": group[
                    "assertions_unspecified_test_count"
                ],
                "tests": cast(JsonValue, tests_for_group[:8]),
            },
        )
        if len(tests_for_group) > 8:
            row["remaining_test_count"] = len(tests_for_group) - 8
        rows.append(row)
    return rows


def text_matrix_crosswalk_row(row: JsonObject) -> str:
    """Render one compact requirement- or subsection-centric matrix row."""

    group_by = row.get("group_by")
    key = row.get("key") or "<unresolved>"
    tests = json_object_rows(row.get("tests"))
    case_ids = [test_id for test in tests if isinstance(test_id := test.get("id"), str)]
    case_preview = ", ".join(case_ids) if case_ids else "<none>"
    remaining = row.get("remaining_test_count")
    if isinstance(remaining, int) and remaining > 0:
        case_preview += f", ... (+{remaining})"
    sections = ", ".join(strings(row.get("standard_sections"))) or "<none>"
    requirements = ", ".join(strings(row.get("lab_requirement_ids"))) or "<none>"
    assertion_count = row.get("assertion_count", 0)
    unspecified_assertion_cases = row.get("assertions_unspecified_test_count", 0)
    test_count = row.get("test_count", 0)
    if isinstance(unspecified_assertion_cases, int) and unspecified_assertion_cases:
        if isinstance(test_count, int) and unspecified_assertion_cases >= test_count:
            assertion_text = "<unspecified>"
        else:
            assertion_text = (
                f"{assertion_count} recorded; {unspecified_assertion_cases} unspecified"
            )
    else:
        assertion_text = str(assertion_count)
    lines = [
        (
            f"- {key}: tests={row.get('test_count', 0)} "
            f"pairs={row.get('pair_count', 0)} assertions={assertion_text}"
        ),
        f"  cases: {case_preview}",
    ]
    if group_by == "requirement":
        lines.append(f"  standard_sections: {sections}")
    else:
        lines.append(f"  requirements: {requirements}")
    return "\n".join(lines)


def text_search_summary_row(test: JsonObject) -> str:
    """Render a small search hit; use ``case`` for the mapping details."""

    requirements = strings(test.get("lab_requirement_ids"))
    sections = strings(test.get("standard_sections"))
    mappings = requirement_section_mapping_rows(test)
    mapping_preview_limit = 1
    mapping_preview = [
        f"{row.get('lab_requirement_id') or '<unnamed requirement>'} -> "
        f"{row.get('standard_section') or '<unresolved>'}"
        for row in mappings[:mapping_preview_limit]
    ]
    if len(mappings) > mapping_preview_limit:
        mapping_preview.append(f"... (+{len(mappings) - mapping_preview_limit})")
    api_surfaces = strings(
        test.get("selected_cpp_api_surface_ids") or test.get("cpp_api_surfaces")
    )
    lines = [
        f"- {test.get('id', '<unnamed>')}: {test.get('test_case', '<unnamed test>')}",
        (
            f"  status={test.get('status', '<unspecified>')}; "
            f"source={source_location_text(test)}; "
            f"assertions={test.get('assertions', '<unspecified>')}; "
            f"traceability={traceability_state(test)}; "
            f"requirements={len(requirements)}; sections={len(sections)}; "
            f"direct_pairs={len(mappings)}; api_surfaces={len(api_surfaces)}"
        ),
    ]
    if mapping_preview:
        lines.append("  requirement_section_mappings: " + ", ".join(mapping_preview))
    return "\n".join(lines)


def text_query_summary_row(
    command: str,
    test: JsonObject,
    index: JsonObject,
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
                    if isinstance(section := row.get("standard_section"), str)
                    and trace_section_match(section, query)
                ]
                # Keep the reverse row exact even when a test matched the
                # section through a legacy/canonical field but has no direct
                # Lab join for that subsection.  Showing every pair in that
                # case makes a bounded lookup look like a fuzzy aggregate.
                mapping_rows = matched_rows
            elif command == "requirement":
                # Use the discovery matcher for contract/clause/document
                # queries too; filtering only by a literal Lab id loses the
                # direct pairs for otherwise valid reverse lookups.
                matched_ids = {
                    lab_id
                    for requirement in json_object_rows(test.get("requirements"))
                    if isinstance(lab_id := requirement.get("lab_requirement_id"), str)
                    and select_tests(
                        [cast(JsonObject, {"requirements": [requirement]})],
                        query,
                        "requirement",
                    )
                }
                matched_rows = [
                    row
                    for row in mapping_rows
                    if isinstance(row, dict)
                    and row.get("lab_requirement_id") in matched_ids
                ]
                mapping_rows = matched_rows
        return text_matrix_row(
            test,
            index,
            compact=True,
            mapping_rows=mapping_rows,
        )
    return text_test_summary(test)


def test_search_values(test: JsonObject) -> list[str]:
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
    for requirement in json_object_rows(test.get("requirements")):
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
        for contract in json_object_rows(requirement.get("contracts")):
            values.extend(strings(contract.get("contract_requirement_id")))
            values.extend(strings(contract.get("source_symbol")))
    for location in json_object_rows(test.get("source_locations")):
        values.extend(strings(location.get("path")))
        if location.get("line") is not None:
            values.append(str(location["line"]))
    return values


def select_search_tests(
    tests: Sequence[JsonObject],
    queries: Sequence[str],
    *,
    all_terms: bool,
) -> list[JsonObject]:
    """Find tests through the bounded indexed search surface.

    A single query preserves the historical substring/phrase behavior.  When
    callers provide multiple positional terms, each term must match at least
    one indexed field by default.  This makes a short multi-word discovery
    command useful without broadening the normal work loop into a source or
    Requirements-Lab scan.
    """

    normalized_terms = [search_key(query) for query in queries if search_key(query)]
    if not normalized_terms:
        return []
    selected: list[tuple[tuple[int, int, int, int], int, JsonObject]] = []
    for position, test in enumerate(tests):
        values = [search_key(value) for value in test_search_values(test)]
        if all_terms:
            matches = all(
                any(term in value for value in values) for term in normalized_terms
            )
        else:
            matches = any(
                any(term in value for value in values) for term in normalized_terms
            )
        if matches:
            primary_values = [
                search_key(value)
                for field in ("id", "test_case", "tags", "selected_cpp_api_surface_ids")
                for value in strings(test.get(field))
            ]
            title_values = [
                search_key(value)
                for value in (test.get("id"), test.get("test_case"))
                if isinstance(value, str)
            ]
            primary_matches = sum(
                any(term in value for value in primary_values)
                for term in normalized_terms
            )
            title_matches = sum(
                any(term in value for value in title_values)
                for term in normalized_terms
            )
            phrase = search_key(" ".join(queries))
            exact_title_phrase = int(
                bool(phrase) and any(phrase in value for value in title_values)
            )
            score = (
                exact_title_phrase,
                int(primary_matches == len(normalized_terms)),
                primary_matches,
                title_matches,
            )
            selected.append((score, position, test))
    selected.sort(key=lambda row: (tuple(-part for part in row[0]), row[1]))
    return [test for _score, _position, test in selected]


def select_tests(
    tests: Sequence[JsonObject], query: str, mode: str
) -> list[JsonObject]:
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
        selected: list[JsonObject] = []
        for test in tests:
            if any(
                trace_section_match(section, query)
                for section in strings(test.get("standard_sections"))
            ):
                selected.append(test)
        return selected
    if mode == "requirement":
        selected: list[JsonObject] = []
        for test in tests:
            for requirement in json_object_rows(test.get("requirements")):
                standard_value = requirement.get("standard")
                standard = standard_value if isinstance(standard_value, dict) else {}
                contract_ids = {
                    contract_id
                    for contract in json_object_rows(requirement.get("contracts"))
                    if isinstance(
                        contract_id := contract.get("contract_requirement_id"), str
                    )
                }
                values: set[JsonScalar] = {
                    value
                    for value in (
                        requirement.get("lab_requirement_id"),
                        standard.get("clause"),
                        standard.get("clause_id"),
                        standard.get("document_id"),
                        standard.get("title"),
                        *contract_ids,
                    )
                    if value is None or isinstance(value, (str, int, float, bool))
                }
                document_id = standard.get("document_id")
                clause_id = standard.get("clause_id")
                if (
                    isinstance(document_id, str)
                    and document_id
                    and isinstance(clause_id, str)
                    and clause_id
                ):
                    # The compact test output publishes this canonical
                    # document:clause key. Accept it as a direct query too,
                    # so selecting a standard subsection does not require a
                    # second lookup to discover the Lab requirement id.
                    values.add(f"{document_id}:{clause_id}")
                    values.add(f"{document_id} {clause_id}")
                source = standard.get("source")
                if isinstance(source, dict):
                    for value in (source.get("path"), source.get("line")):
                        if value is None or isinstance(value, (str, int, float, bool)):
                            values.add(value)
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
        return select_search_tests(tests, [query], all_terms=False)
    return [
        test
        for test in tests
        if folded in str(test.get("id", "")).casefold()
        or folded in str(test.get("test_case", "")).casefold()
    ]


def build_parser() -> argparse.ArgumentParser:
    """Build the public query-work command parser."""
    if __package__:
        from .query_rti_work_parser import build_parser as parser_builder
    else:
        from query_rti_work_parser import build_parser as parser_builder
    return parser_builder()


def main() -> int:
    """Run the query-work command-line interface."""
    if __package__:
        from .query_rti_work_cli import main as cli_main
    else:
        if __name__ == "__main__":
            sys.modules.setdefault("query_rti_work", sys.modules[__name__])
        from query_rti_work_cli import main as cli_main
    return cli_main()


if __name__ == "__main__":
    raise SystemExit(main())
