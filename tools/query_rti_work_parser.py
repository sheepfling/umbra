"""Argument parser construction for query_rti_work."""

from __future__ import annotations

import argparse
from pathlib import Path

if __package__:
    from .query_rti_work import (
        DEFAULT_BUNDLE,
        DEFAULT_CONTRACT_DIRECTORY,
        DEFAULT_CPP_TCK_CATALOG,
        DEFAULT_IMPLEMENTATION_PLAN,
        DEFAULT_INDEX,
        DEFAULT_PLAN,
        DEFAULT_ROADMAP,
        DEFAULT_TEST_ROOT,
    )
else:
    from query_rti_work import (
        DEFAULT_BUNDLE,
        DEFAULT_CONTRACT_DIRECTORY,
        DEFAULT_CPP_TCK_CATALOG,
        DEFAULT_IMPLEMENTATION_PLAN,
        DEFAULT_INDEX,
        DEFAULT_PLAN,
        DEFAULT_ROADMAP,
        DEFAULT_TEST_ROOT,
    )


def build_parser() -> argparse.ArgumentParser:
    value = argparse.ArgumentParser(
        description=(
            "Read-only RTI work and traceability queries against the pinned 2025 "
            "baseline. Start with 'resume'; use 'case' for one test's source, "
            "requirement/subsection pairs and focused execution handles."
        ),
        epilog=(
            "Command card: docs/planning/QUERY-CARD.md. "
            "Use '<command> --help' for options; --summary --compact for bounded "
            "detail. No command here synchronizes the Requirements Lab."
        ),
    )
    value.add_argument("--index", type=Path, default=DEFAULT_INDEX)
    value.add_argument("--plan", type=Path, default=DEFAULT_PLAN)
    value.add_argument("--bundle", type=Path, default=DEFAULT_BUNDLE)
    value.add_argument(
        "--contract-directory", type=Path, default=DEFAULT_CONTRACT_DIRECTORY
    )
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
    value.add_argument(
        "--verbose", action="store_true", help="include per-test next actions"
    )
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
    status = subcommands.add_parser(
        "status", help="show open roadmap items and checklist counts"
    )
    add_output_flags(status)
    queue = subcommands.add_parser(
        "queue",
        help=("show a bounded queue of open roadmap families and exact lane states"),
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
        default=None,
        help="maximum family rows (default: 3 in text, 12 with JSON/verbose; 0 for all)",
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
        help=("exact Catch2 lane tag; omit to use the active indexed work lane"),
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
        default=5,
        help="maximum tests to print (default: 5; use 0 for all)",
    )
    add_output_flags(item)
    plan_outline = subcommands.add_parser(
        "plan",
        help=(
            "show the implementation-plan section outline or filter its heading index"
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
        help=("maximum matching headings to print (default: 20; use 0 for all)"),
    )
    add_output_flags(plan_outline)
    coverage = subcommands.add_parser(
        "coverage", help="summarize C++ test and clause mapping coverage"
    )
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
    tck_gaps = subcommands.add_parser(
        "tck-gaps",
        help=(
            "list pinned 2025 requirements without a direct portable C++ TCK "
            "catalog mapping, with existing Catch2 case pointers"
        ),
    )
    tck_gaps.add_argument(
        "query",
        nargs="?",
        help=(
            "optional case-insensitive requirement id, title, statement, "
            "document, clause, or canonical document:clause filter"
        ),
    )
    tck_gaps.add_argument(
        "--catalog",
        type=Path,
        default=DEFAULT_CPP_TCK_CATALOG,
        help="portable C++ TCK scenario catalog to inspect",
    )
    tck_gaps.add_argument(
        "--document",
        help="limit to a 2025 document id such as hla-1516.1-2025",
    )
    tck_gaps.add_argument(
        "--clause",
        help="limit to a clause or subsection such as clause-8.11.3",
    )
    tck_gaps.add_argument(
        "--coverage-kind",
        choices=("cross-cutting", "document-wide", "transition"),
        help="limit to one pinned corpus coverage kind",
    )
    tck_gaps.add_argument(
        "--limit",
        type=int,
        default=20,
        help=(
            "maximum uncovered requirement records to print (default: 20; "
            "use 0 for all records; subsection groups stay capped at 20)"
        ),
    )
    add_output_flags(tck_gaps)
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
    lane = subcommands.add_parser(
        "lane",
        help="show tests or an indexed execution handle for one exact lane",
    )
    lane.add_argument("tag")
    lane.add_argument(
        "--limit",
        type=int,
        default=20,
        help="maximum tests to print (default: 20; use 0 for all)",
    )
    add_output_flags(lane)
    test = subcommands.add_parser(
        "test", help="find tests by id or case-name substring"
    )
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
        default=None,
        help="maximum tests (default: 5 in text, 20 with JSON/verbose; 0 for all)",
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
        default=None,
        help="maximum tests (default: 5 in text, 20 with JSON/verbose; 0 for all)",
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
    matrix.add_argument(
        "--group-by",
        choices=("test", "requirement", "section"),
        default="test",
        help=(
            "matrix row shape: one row per test (default), per Lab requirement, "
            "or per canonical 2025 subsection"
        ),
    )
    add_output_flags(matrix)
    search = subcommands.add_parser(
        "search",
        help=(
            "find tests by any id, tag, API surface, requirement, or standard "
            "section; multiple terms are ANDed by default"
        ),
    )
    search.add_argument(
        "query",
        nargs="+",
        help=(
            "one quoted phrase or one or more indexed terms; multiple terms "
            "are ANDed unless --any-term is selected"
        ),
    )
    search_mode = search.add_mutually_exclusive_group()
    search_mode.add_argument(
        "--all-terms",
        action="store_true",
        help="require every positional term (the default when more than one is supplied)",
    )
    search_mode.add_argument(
        "--any-term",
        action="store_true",
        help="match any positional term for an intentionally broad lookup",
    )
    search.add_argument(
        "--limit",
        type=int,
        default=None,
        help="maximum tests (default: 5 in text, 20 with JSON/verbose; 0 for all)",
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
        "--focused",
        action="store_true",
        help=(
            "use the owning family's explicit focused_lane_tags instead of "
            "the broad cross-cutting tag inventory"
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
