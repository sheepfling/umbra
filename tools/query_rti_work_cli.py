"""Command-line presentation and dispatch for query_rti_work."""

from __future__ import annotations

import json
import sys
from collections import Counter
from typing import cast

if __package__:
    from .query_rti_work import (
        JsonObject,
        all_mapped_tests,
        case_card_record,
        compact_prose,
        contract_candidates_for_test,
        contract_drift_report,
        cpp_tck_requirement_gap_inventory,
        cpp_tck_requirement_gap_summary_data,
        dashboard_snapshot,
        filtered_plan_sections,
        focused_lane_result,
        implementation_plan_sections,
        index_status,
        index_warnings,
        indexed_item,
        indexed_unlocated_plan_summary,
        indexed_work_queue,
        indexed_work_slice,
        is_non_executable_source_status,
        is_planned_test,
        item_handles,
        item_matches_lane,
        item_tests,
        lab_issues_snapshot,
        lane_ctest_command,
        lane_inventory,
        live_item_focus,
        live_next_action,
        load_contract_links,
        load_contract_test_candidates,
        load_json,
        load_standard_requirements,
        load_test_source_locations,
        matrix_crosswalk_rows,
        matrix_summary_data,
        ready_slice,
        recent_completed_views,
        relative_path,
        requirement_gap_inventory,
        requirement_gap_lookup_summary_data,
        requirement_gap_summary_data,
        requirement_section_mapping_rows,
        resolve_matrix_query,
        resolve_trace_query,
        resume_snapshot,
        roadmap_checklist,
        roadmap_inventory,
        roadmap_item_summary,
        roadmap_summary_data,
        scoped_plan_tests,
        select_search_tests,
        select_tests,
        source_location_sort_key,
        source_location_text,
        source_pointer_state,
        strings,
        test_summary_data,
        tests_for_source_path,
        text_case_card,
        text_gap_preview,
        text_matrix_crosswalk_row,
        text_matrix_row,
        text_query_summary_row,
        text_search_summary_row,
        text_test,
        text_test_summary,
        text_trace,
        trace_record,
        trace_summary_record,
        traceability_state,
        unlocated_display,
        unplanned_source_cases,
        validate_index,
    )
    from .query_rti_work_parser import build_parser
else:
    from query_rti_work import (
        JsonObject,
        all_mapped_tests,
        case_card_record,
        compact_prose,
        contract_candidates_for_test,
        contract_drift_report,
        cpp_tck_requirement_gap_inventory,
        cpp_tck_requirement_gap_summary_data,
        dashboard_snapshot,
        filtered_plan_sections,
        focused_lane_result,
        implementation_plan_sections,
        index_status,
        index_warnings,
        indexed_item,
        indexed_unlocated_plan_summary,
        indexed_work_queue,
        indexed_work_slice,
        is_non_executable_source_status,
        is_planned_test,
        item_handles,
        item_matches_lane,
        item_tests,
        lab_issues_snapshot,
        lane_ctest_command,
        lane_inventory,
        live_item_focus,
        live_next_action,
        load_contract_links,
        load_contract_test_candidates,
        load_json,
        load_standard_requirements,
        load_test_source_locations,
        matrix_crosswalk_rows,
        matrix_summary_data,
        ready_slice,
        recent_completed_views,
        relative_path,
        requirement_gap_inventory,
        requirement_gap_lookup_summary_data,
        requirement_gap_summary_data,
        requirement_section_mapping_rows,
        resolve_matrix_query,
        resolve_trace_query,
        resume_snapshot,
        roadmap_checklist,
        roadmap_inventory,
        roadmap_item_summary,
        roadmap_summary_data,
        scoped_plan_tests,
        select_search_tests,
        select_tests,
        source_location_sort_key,
        source_location_text,
        source_pointer_state,
        strings,
        test_summary_data,
        tests_for_source_path,
        text_case_card,
        text_gap_preview,
        text_matrix_crosswalk_row,
        text_matrix_row,
        text_query_summary_row,
        text_search_summary_row,
        text_test,
        text_test_summary,
        text_trace,
        trace_record,
        trace_summary_record,
        traceability_state,
        unlocated_display,
        unplanned_source_cases,
        validate_index,
    )
    from query_rti_work_parser import build_parser


def main() -> int:
    # Keep bounded query commands usable in Windows PowerShell sessions whose
    # legacy console encoding cannot represent section marks or arrows.
    sys.stdout.reconfigure(errors="backslashreplace")
    arguments = build_parser().parse_args()
    discovery_commands = {"requirement", "section", "search"}
    if arguments.command in discovery_commands | {"roadmap"}:
        if arguments.limit is None:
            detailed = arguments.json or arguments.verbose
            arguments.limit = (
                (12 if detailed else 3)
                if arguments.command == "roadmap"
                else (20 if detailed else 5)
            )
        if (
            arguments.command == "roadmap"
            and not arguments.json
            and not arguments.verbose
        ):
            arguments.compact = True
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
        standard_requirements = load_standard_requirements(
            bundle,
            arguments.contract_directory,
        )
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
            f"{plan_counts.get('mapped_cases', 0)} requirement-mapped, "
            f"{plan_counts.get('unmapped_cases', 0)} without a requirement mapping "
            f"({plan_counts.get('explicit_unmapped_dispositions', 0)} explicitly dispositioned; "
            f"{plan_counts.get('unclassified_unmapped_cases', 0)} unclassified)."
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
            if next_card.get("plan_id"):
                plan_label = (
                    "proposed_plan_id"
                    if next_card.get("plan_id_status") == "proposed"
                    else "plan_id"
                )
                print(f"  {plan_label}={next_card['plan_id']}")
            print(
                f"  case={compact_prose(next_card.get('test_case'), 220)} "
                f"source={next_card.get('source_location') or next_card.get('source_target') or '<unlocated>'}"
            )
            if next_card.get("ctest_target"):
                print(f"  ctest_target={next_card['ctest_target']}")
            if next_card.get("ctest_filter"):
                print(f"  ctest_filter={next_card['ctest_filter']}")
            print(
                (
                    "  contract_mapping="
                    if next_card.get("mapping_status")
                    == "mapped-from-portable-tck-contracts"
                    else "  mapping_seed_only="
                    if next_card.get("mapping_seed_plan_id")
                    else "  mapping="
                )
                + f"{next_card.get('requirement_count', 0)} requirements; "
                f"{next_card.get('standard_section_count', 0)} sections; "
                f"{next_card.get('requirement_section_pair_count', 0)} direct pairs; "
                f"{next_card.get('cpp_api_surface_count', 0)} APIs"
            )
            mapping_pairs = next_card.get("requirement_section_pairs")
            if isinstance(mapping_pairs, list) and mapping_pairs:
                pair_text = [
                    f"{pair.get('lab_requirement_id')} -> {pair.get('standard_section')}"
                    for pair in mapping_pairs[:4]
                    if isinstance(pair, dict)
                    and pair.get("lab_requirement_id")
                    and pair.get("standard_section")
                ]
                remaining_pairs = max(0, len(mapping_pairs) - len(pair_text))
                if pair_text:
                    suffix = f"; +{remaining_pairs} more" if remaining_pairs else ""
                    print("  mapping_pairs=" + "; ".join(pair_text) + suffix)

            for handle_name in (
                "case_command",
                "matrix_requirement_command",
                "matrix_section_command",
            ):
                if next_card.get("mapping_status") == "seed-from-existing-case":
                    # A proposed case is not in the plan yet, so these exact
                    # case/matrix queries would be premature.  Show them from
                    # the scoped ready view after the proposed row is recorded.
                    continue
                handle = next_card.get(handle_name)
                if isinstance(handle, str) and handle:
                    print(f"  {handle_name.removesuffix('_command')}={handle}")
            if next_card.get("next_action"):
                print(f"  next={compact_prose(next_card.get('next_action'), 260)}")
            if next_card.get("mapping_status") == "seed-from-existing-case":
                print(
                    "  mapping_seed="
                    f"{next_card.get('mapping_seed_plan_id')} "
                    f"status={next_card.get('mapping_status', '<unspecified>')}"
                )
            elif (
                next_card.get("mapping_status") == "mapped-from-portable-tck-contracts"
            ):
                print(f"  contract_companion={next_card.get('mapping_seed_plan_id')}")
            if next_card.get("seed_trace_command"):
                print(f"  seed_trace={next_card['seed_trace_command']}")
        elif isinstance(next_card, dict):
            if next_card.get("selection_required"):
                print(
                    "Next: choose one bounded family action (no Requirements Lab rescan)"
                )
            else:
                print(
                    "Next: "
                    f"{next_card.get('state', 'none')}; "
                    f"{next_card.get('reason', 'choose one bounded family action')}"
                )
            if next_card.get("recommended_family_id"):
                print(f"  recommended_family={next_card['recommended_family_id']}")
            if next_card.get("family_choice_command"):
                print(f"  family_choice={next_card['family_choice_command']}")
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
                    next_action = option.get("next_action")
                    if isinstance(next_action, str) and next_action.strip():
                        print(f"  next_action={compact_prose(next_action, 360)}")
                    for name in (
                        "work_query",
                        "lane_discovery_command",
                        "mapping_lane_discovery_command",
                        "gap_query",
                        "matrix_requirement_command",
                        "matrix_section_command",
                    ):
                        handle = option.get(name)
                        if isinstance(handle, str) and handle:
                            print(f"  {name.removesuffix('_command')}={handle}")
                    if not (arguments.summary or arguments.compact):
                        for line in text_gap_preview(option):
                            print(line)
                remaining = next_card.get("family_options_remaining", 0)
                if remaining:
                    print(
                        f"  ... {remaining} more family choice(s); use dashboard for the preview"
                    )
        else:
            print("Next: no bounded indexed handoff")

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
                    "ready",
                    "post_mapping_check",
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
        verified_restore = result.get("latest_verified_restore_slice")
        if isinstance(verified_restore, dict):
            verification = verified_restore.get("verification")
            run_count = (
                verification.get("focused_runs")
                if isinstance(verification, dict)
                else None
            )
            print(
                "Verified fix: "
                f"{verified_restore.get('id', '<unnamed>')} "
                f"({run_count if run_count is not None else '?'} focused C++ runs)"
            )
            queries = verified_restore.get("queries")
            if isinstance(queries, dict) and queries.get("representative_case"):
                print(f"   verified_case={queries['representative_case']}")
        source_reconciliation = result.get("source_only_reconciliation")
        if isinstance(source_reconciliation, dict) and source_reconciliation.get(
            "count", 0
        ):
            print(
                "Source-only reconciliation head: "
                f"{compact_prose(source_reconciliation.get('test_query'), 180)} "
                f"source={source_reconciliation.get('source_location') or '<unlocated>'}"
            )
            print(f"   opt_in={source_reconciliation.get('ready_command')}")
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
                    f"source={next_card.get('source_location') or next_card.get('source_target') or '<unlocated>'}"
                )
                if next_card.get("plan_id"):
                    plan_label = (
                        "proposed_plan_id"
                        if next_card.get("plan_id_status") == "proposed"
                        else "plan_id"
                    )
                    print(f"   {plan_label}={next_card['plan_id']}")
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
                print(
                    "Next: choose one bounded queued family action (no Requirements Lab rescan)"
                )
                print(
                    f"   recommended: {family_options[0].get('id', '<unnamed>')}"
                    if isinstance(family_options[0], dict)
                    else "   recommended: <none>"
                )
                if next_card.get("family_choice_command"):
                    print(f"   family_choice={next_card['family_choice_command']}")
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
                    if arguments.summary:
                        continue
                    if option.get("lane_discovery_command"):
                        print(
                            f"      lane_discovery={option['lane_discovery_command']}"
                        )
                    if option.get("mapping_lane_discovery_command"):
                        print(
                            "      mapping_lane_discovery="
                            f"{option['mapping_lane_discovery_command']}"
                        )
                    if option.get("gap_query"):
                        print(f"      gaps={option['gap_query']}")
                    if option.get("matrix_requirement_command"):
                        print(
                            "      matrix_requirements="
                            f"{option['matrix_requirement_command']}"
                        )
                    if option.get("matrix_section_command"):
                        print(
                            f"      matrix_sections={option['matrix_section_command']}"
                        )
                    if not arguments.summary:
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
            not test.get("requirements") and traceability_state(test) == "unclassified"
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
            (arguments.lane is None and arguments.family is None)
            or (
                arguments.family is not None
                and isinstance(item, dict)
                and str(item.get("id") or "").casefold() == arguments.family.casefold()
            )
            or (arguments.lane is not None and item_matches_lane(item, arguments.lane))
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
                test.get("id") for test in scoped_tests if is_planned_test(test)
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
                    else f" for lane {arguments.lane!r}"
                    if arguments.lane
                    else ""
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
                else f" for lane {arguments.lane!r}"
                if arguments.lane
                else ""
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
                            for identifier in result["unlocated_test_ids"][
                                :sample_limit
                            ]
                        )
                    )
                    if len(result["unlocated_test_ids"]) > sample_limit:
                        print(
                            f"- ... {len(result['unlocated_test_ids']) - sample_limit} more; "
                            "use check --json for the complete list"
                        )
                else:
                    print("Unlocated Catch2 plan entries:")
                    print(
                        "\n".join(
                            f"- {identifier}"
                            for identifier in result["unlocated_test_ids"]
                        )
                    )
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
                    f"{name}={count}" for name, count in sorted(action_counts.items())
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
                    print(
                        f"   unlocated_head_trace={row['unlocated_head_trace_command']}"
                    )
            if row.get("planned_head"):
                print(f"   planned_head={compact_prose(row['planned_head'], 160)}")
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
                f'"{latest.get("test_case", "")}" --summary --compact'
            )
            latest_lane = latest.get("lane")
            latest_focus_lane = (
                latest.get("focus_lane") or latest.get("primary_lane") or latest_lane
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
        if (
            isinstance(indexed_counts, dict)
            and indexed_queue_counts != live_queue_counts
        ):
            print(
                "Traceability index snapshot differs from the live local counts; run the bounded queries before editing the index."
            )
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
                    selectors.append(f"next_source_lane={item.get('next_source_lane')}")
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
                                f'"{pointer.get("query")}" --summary --compact'
                            )
                if selectors:
                    print(f"   {' '.join(selectors)}")
                live_action = live_next_action(item)
                if live_action:
                    print(f"   next={compact_prose(live_action, 220)}")
            return 0
        for item in sorted(
            result["roadmap_items"], key=lambda value: value.get("priority", 999)
        ):
            if item.get("status") != "open":
                continue
            print(f"{item.get('priority', '?')}. {item.get('id')}: {item.get('title')}")
            if arguments.compact:
                print(
                    f"   kind={item.get('kind')} tag_count={len(item.get('query_tags', []))} "
                    f"tagged_cases={item.get('matching_test_count', 0)}"
                )
            else:
                counts = (
                    ", ".join(
                        f"{tag}={count}"
                        for tag, count in item["matching_test_counts"].items()
                    )
                    or "no tagged cases"
                )
                print(f"   kind={item.get('kind')} tags=({counts})")
            focus_value = item.get("live_current_focus") or item.get("current_focus")
            if focus_value:
                focus = compact_prose(focus_value) if arguments.compact else focus_value
                print(f"   focus={focus}")
            if item.get("next_task"):
                task = (
                    compact_prose(item.get("next_task"))
                    if arguments.compact
                    else item.get("next_task")
                )
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            live_action = live_next_action(item)
            next_action = (
                compact_prose(live_action)
                if arguments.compact and live_action
                else live_action
            )
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
                if ready_result.get("selection_required"):
                    print(
                        "Next bounded implementation choice (select one; no Requirements Lab rescan):"
                    )
                    if ready_result.get("recommended_family_id"):
                        print(
                            "recommended_family="
                            f"{ready_result['recommended_family_id']}"
                        )
                    if ready_result.get("family_choice_command"):
                        print(f"family_choice={ready_result['family_choice_command']}")
                else:
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
                            print(
                                f"  lane_discovery={option['lane_discovery_command']}"
                            )
                        if option.get("mapping_lane_discovery_command"):
                            print(
                                "  mapping_lane_discovery="
                                f"{option['mapping_lane_discovery_command']}"
                            )
                        if option.get("gap_query"):
                            print(f"  gaps={option['gap_query']}")
                        if option.get("matrix_requirement_command"):
                            print(
                                "  matrix_requirements="
                                f"{option['matrix_requirement_command']}"
                            )
                        if option.get("matrix_section_command"):
                            print(
                                f"  matrix_sections={option['matrix_section_command']}"
                            )
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
                    f"fallback_queue_pointer: {owner.get('id', '<none>')}: "
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
            if ready_result.get("case_command"):
                print(f"case_command: {ready_result['case_command']}")
            if ready_result.get("matrix_requirement_command"):
                print(
                    "matrix_requirement_command: "
                    f"{ready_result['matrix_requirement_command']}"
                )
            if ready_result.get("matrix_section_command"):
                print(
                    f"matrix_section_command: {ready_result['matrix_section_command']}"
                )
            if ready_result.get("seed_trace_command"):
                print(f"seed_trace_command: {ready_result['seed_trace_command']}")
            if ready_result.get("seed_matrix_requirement_command"):
                print(
                    "seed_matrix_requirement_command: "
                    f"{ready_result['seed_matrix_requirement_command']}"
                )
            if ready_result.get("seed_matrix_section_command"):
                print(
                    "seed_matrix_section_command: "
                    f"{ready_result['seed_matrix_section_command']}"
                )
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
        if arguments.family:
            requested_family = arguments.family.strip().casefold()
            open_items = [
                item
                for item in open_items
                if isinstance(item.get("id"), str)
                and item["id"].casefold() == requested_family
            ]
            if not open_items:
                print(f"No open roadmap family matches: {arguments.family}")
                return 1
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
                    "source_requirement_ids": strings(
                        item.get("next_source_requirement_ids")
                    ),
                    "source_standard_sections": strings(
                        item.get("next_source_standard_sections")
                    ),
                    "source_api_surfaces": strings(
                        item.get("next_source_api_surfaces")
                    ),
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
                        isinstance(global_queue, dict) and global_queue.get("count", 0)
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
                    (
                        "next_process_package_timestamped_test",
                        "process_package_timestamped_test",
                    ),
                    (
                        "next_process_package_timestamped_ctest_filter",
                        "process_package_timestamped_ctest",
                    ),
                    (
                        "next_process_package_parameterized_test",
                        "process_package_parameterized_test",
                    ),
                    (
                        "next_process_package_parameterized_ctest_filter",
                        "process_package_parameterized_ctest",
                    ),
                    (
                        "next_process_package_connection_loss_test",
                        "process_package_connection_loss_test",
                    ),
                    (
                        "next_process_package_connection_loss_ctest_filter",
                        "process_package_connection_loss_ctest",
                    ),
                    (
                        "next_process_package_connection_loss_delete_objects_test",
                        "process_package_connection_loss_delete_objects_test",
                    ),
                    (
                        "next_process_package_connection_loss_delete_objects_ctest_filter",
                        "process_package_connection_loss_delete_objects_ctest",
                    ),
                    (
                        "next_process_package_object_registration_test",
                        "process_package_object_registration_test",
                    ),
                    (
                        "next_process_package_object_registration_ctest_filter",
                        "process_package_object_registration_ctest",
                    ),
                    (
                        "next_process_package_named_registration_test",
                        "process_package_named_registration_test",
                    ),
                    (
                        "next_process_package_named_registration_ctest_filter",
                        "process_package_named_registration_ctest",
                    ),
                    (
                        "next_process_package_attribute_update_test",
                        "process_package_attribute_update_test",
                    ),
                    (
                        "next_process_package_attribute_update_ctest_filter",
                        "process_package_attribute_update_ctest",
                    ),
                    (
                        "next_process_package_directed_retraction_test",
                        "process_package_directed_retraction_test",
                    ),
                    (
                        "next_process_package_directed_retraction_ctest_filter",
                        "process_package_directed_retraction_ctest",
                    ),
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
                    (
                        "next_process_package_catalog_verifier",
                        "process_package_catalog_verifier",
                    ),
                    (
                        "next_process_package_junit_artifact",
                        "process_package_junit_artifact",
                    ),
                ):
                    if item.get(field):
                        print(f"   {label}={item.get(field)}")
                if item.get("next_test_query"):
                    test_label = (
                        "baseline_test"
                        if item.get("next_test_role") == "baseline"
                        else "test"
                    )
                    print(
                        f"   {test_label}={compact_prose(item.get('next_test_query'), 180)}"
                    )
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
                        print(
                            f"   next_source_location={item.get('next_source_location')}"
                        )
                    if item.get("next_source_lane"):
                        print(f"   next_source_lane={item.get('next_source_lane')}")
                    source_requirement_ids = strings(
                        item.get("next_source_requirement_ids")
                    )
                    source_standard_sections = strings(
                        item.get("next_source_standard_sections")
                    )
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
                        print(
                            f"   next_source_ctest={item.get('next_source_ctest_filter')}"
                        )
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
            tags = strings(item.get("query_tags")) + strings(
                item.get("focused_lane_tags")
            )
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
                task = (
                    compact_prose(item.get("next_task"))
                    if arguments.compact
                    else item.get("next_task")
                )
                print(f"   task={task}")
            handles = item_handles(item, arguments.compact)
            if handles:
                print(f"   handles={' '.join(handles)}")
            live_action = live_next_action(item)
            next_action = (
                compact_prose(live_action)
                if arguments.compact and live_action
                else live_action
            )
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
            if arguments.summary:
                print(
                    f"- {row.get('id', '<unnamed>')}: "
                    f"status={row.get('status', '<unspecified>')} "
                    f"action={row.get('action_state', '<unspecified>')} "
                    f"cases={row.get('matching_test_count', 0)} "
                    f"mapped={counts.get('mapped_case_count', 0)} "
                    f"candidates={counts.get('executable_candidate_count', 0)} "
                    f"requirements={counts.get('requirement_count', 0)} "
                    f"sections={counts.get('standard_section_count', 0)} "
                    f"pairs={counts.get('requirement_section_pair_count', 0)}"
                )
                print(
                    "  work=python tools/query_rti_work.py work "
                    f"{row.get('id', '<unnamed>')} --summary --compact"
                )
                action = row.get("next_action")
                if isinstance(action, str) and action.strip():
                    print(f"  next={compact_prose(action, 180)}")
                continue
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
                        "representative_case_command",
                        "representative_trace_command",
                        "next_case_command",
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
                            + (
                                f", ... (+{len(requirement_ids) - 8})"
                                if len(requirement_ids) > 8
                                else ""
                            )
                        )
                    if sections:
                        print(
                            "  next_test_standard_sections: "
                            + ", ".join(sections[:8])
                            + (
                                f", ... (+{len(sections) - 8})"
                                if len(sections) > 8
                                else ""
                            )
                        )
                    pair_rows = pointer.get("requirement_section_mappings")
                    if isinstance(pair_rows, list) and pair_rows:
                        pair_preview = [
                            f"{row.get('lab_requirement_id')} -> {row.get('standard_section') or '<unresolved>'}"
                            for row in pair_rows[:8]
                            if isinstance(row, dict)
                        ]
                        if pair_preview:
                            remaining = pointer.get(
                                "requirement_section_mappings_remaining", 0
                            )
                            suffix = f", ... (+{remaining})" if remaining else ""
                            print(
                                "  next_test_pairs: " + ", ".join(pair_preview) + suffix
                            )
            commands = row.get("commands") or {}
            for label in (
                "case",
                "trace",
                "matrix_requirement",
                "matrix_section",
                "work",
                "focus",
                "matrix",
            ):
                if commands.get(label):
                    print(f"  {label}: {commands[label]}")
            action = row.get("next_action")
            if isinstance(action, str) and action.strip():
                print(
                    "  next: "
                    + (
                        compact_prose(action, 240)
                        if arguments.compact or arguments.summary
                        else action
                    )
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
            if result.get("selection_required"):
                print(
                    "no executable indexed C++ case is ready; select one bounded "
                    "roadmap action (no Requirements Lab rescan)"
                )
                if result.get("recommended_family_id"):
                    print(f"recommended_family: {result['recommended_family_id']}")
                if result.get("family_choice_command"):
                    print(f"family_choice: {result['family_choice_command']}")
            else:
                print(result.get("reason", "no bounded implementation handoff"))
            family_options = result.get("family_options")
            if isinstance(family_options, list) and family_options:
                print(
                    "family_options: bounded open families (choose one; no Lab rescan)"
                )
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
                    show_mapping_preview = not (arguments.summary or arguments.compact)
                    if (
                        result.get("requested_family")
                        and requirement_preview
                        and show_mapping_preview
                    ):
                        suffix = (
                            f", ... (+{option.get('requirement_ids_remaining', 0)})"
                            if option.get("requirement_ids_remaining")
                            else ""
                        )
                        print(
                            "  requirements=" + ",".join(requirement_preview) + suffix
                        )
                    if (
                        result.get("requested_family")
                        and section_preview
                        and show_mapping_preview
                    ):
                        suffix = (
                            f", ... (+{option.get('standard_sections_remaining', 0)})"
                            if option.get("standard_sections_remaining")
                            else ""
                        )
                        print(
                            "  standard_sections=" + ",".join(section_preview) + suffix
                        )
                    pair_preview = option.get("requirement_section_pairs_preview") or []
                    if (
                        result.get("requested_family")
                        and pair_preview
                        and show_mapping_preview
                    ):
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
                        print(f"  lane_discovery={option['lane_discovery_command']}")
                    if option.get("mapping_lane_discovery_command"):
                        print(
                            "  mapping_lane_discovery="
                            f"{option['mapping_lane_discovery_command']}"
                        )
                    if option.get("gap_query"):
                        print(f"  gaps={option['gap_query']}")
                    if option.get("matrix_requirement_command"):
                        print(
                            "  matrix_requirements="
                            f"{option['matrix_requirement_command']}"
                        )
                    if option.get("matrix_section_command"):
                        print(f"  matrix_sections={option['matrix_section_command']}")
                    if not (arguments.summary or arguments.compact):
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
                "next_command: python tools/query_rti_work.py next --summary --compact"
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
                f"fallback_queue_pointer: {owner.get('id', '<none>')}: "
                f"{owner.get('title', '<unnamed roadmap family>')}"
            )
        print(f"state: {result.get('state')}")
        if result.get("handoff_kind"):
            print(
                f"handoff_kind: {result['handoff_kind']} "
                f"runnable={result.get('runnable', True)}"
            )
        if result.get("family_id") and not result.get("requested_family"):
            print(f"family: {result['family_id']}")
        if result.get("plan_id"):
            plan_label = (
                "proposed_plan_id"
                if result.get("plan_id_status") == "proposed"
                else "plan_id"
            )
            print(f"{plan_label}: {result['plan_id']}")
        print(f"test: {result.get('test_case', '<unnamed>')}")
        if result.get("source_location"):
            print(f"source: {result['source_location']}")
        elif result.get("source_target"):
            print(
                f"source_target: {result['source_target']} (TEST_CASE not declared yet)"
            )
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
        mapping_label = (
            "tck_contract_mapping"
            if result.get("mapping_status") == "mapped-from-portable-tck-contracts"
            else "mapping_seed_only"
            if result.get("mapping_seed_plan_id")
            else "mapping"
        )
        print(
            f"{mapping_label}: "
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

        ready_pairs = result.get("requirement_section_pairs")
        has_ready_pairs = isinstance(ready_pairs, list) and bool(ready_pairs)
        if result.get("requirement_ids") and not (
            arguments.compact and has_ready_pairs
        ):
            print(f"requirements: {ready_preview(strings(result['requirement_ids']))}")
        if result.get("standard_sections") and not (
            arguments.compact and has_ready_pairs
        ):
            print(
                "standard_sections: "
                f"{ready_preview(strings(result['standard_sections']))}"
            )
        if has_ready_pairs and not (
            arguments.compact and result.get("mapping_seed_plan_id")
        ):
            pair_text = [
                f"{pair.get('lab_requirement_id')} -> {pair.get('standard_section')}"
                for pair in ready_pairs[:4]
                if isinstance(pair, dict)
                and pair.get("lab_requirement_id")
                and pair.get("standard_section")
            ]
            if pair_text:
                remaining_pairs = max(0, len(ready_pairs) - len(pair_text))
                suffix = f"; +{remaining_pairs} more" if remaining_pairs else ""
                print("direct_pairs: " + "; ".join(pair_text) + suffix)
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
        if result.get("mapping_seed_plan_id") and not arguments.compact:
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
        for display_name, field_name in (
            ("case", "case_command"),
            ("matrix_requirements", "matrix_requirement_command"),
            ("matrix_sections", "matrix_section_command"),
            ("trace_command", "trace_command"),
            ("seed_trace_command", "seed_trace_command"),
            ("seed_matrix_requirement_command", "seed_matrix_requirement_command"),
            ("seed_matrix_section_command", "seed_matrix_section_command"),
            ("implementation_command", "implementation_command"),
            ("focus_command", "focus_command"),
            ("check_command", "check_command"),
            ("post_mapping_focus_command", "post_mapping_focus_command"),
            ("post_mapping_check_command", "post_mapping_check_command"),
        ):
            if result.get(field_name):
                if (
                    arguments.compact
                    and result.get("mapping_seed_plan_id")
                    and field_name
                    in {
                        "seed_matrix_requirement_command",
                        "seed_matrix_section_command",
                        "implementation_command",
                    }
                ):
                    continue
                separator = (
                    "="
                    if display_name
                    in {
                        "case",
                        "matrix_requirements",
                        "matrix_sections",
                    }
                    else ": "
                )
                if (
                    result.get("mapping_seed_plan_id")
                    and field_name == "implementation_command"
                ):
                    display_name = "seed_implementation_command"
                print(f"{display_name}{separator}{result[field_name]}")
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
                suffix = (
                    f", ... (+{len(requirement_ids) - 8})"
                    if len(requirement_ids) > 8
                    else ""
                )
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
            print("candidates: none; no source-located unimplemented cases remain.")
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
                print(f"task: {compact_prose(result['task'], 360)}")
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
                baseline_state = baseline.get("state") or baseline.get(
                    "status", "<unspecified>"
                )
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
                source_requirement_ids = strings(
                    result.get("next_source_requirement_ids")
                )
                if source_requirement_ids:
                    print(
                        f"next_slice_requirements: {', '.join(source_requirement_ids)}"
                    )
                source_api_surfaces = strings(result.get("next_source_api_surfaces"))
                if source_api_surfaces:
                    print(f"next_slice_api_surfaces: {', '.join(source_api_surfaces)}")
                if result.get("next_source_ctest_filter"):
                    print(f"next_slice_ctest: {result['next_source_ctest_filter']}")
            sections = strings(result.get("standard_sections"))
            plan_ids = strings(result.get("plan_ids"))
            work_mapping = result.get("work_mapping")
            if isinstance(work_mapping, dict):
                print(
                    "work_mapping: "
                    f"cases={work_mapping.get('case_count', 0)}; "
                    f"mapped={work_mapping.get('mapped_case_count', 0)}; "
                    f"requirements={work_mapping.get('requirement_count', 0)}; "
                    f"standard_sections={work_mapping.get('standard_section_count', 0)}; "
                    f"direct_pairs={work_mapping.get('resolved_requirement_section_pair_count', 0)}; "
                    f"assertions={work_mapping.get('assertion_count', 0)}"
                )
                requirement_preview = strings(
                    work_mapping.get("requirement_ids_preview")
                )
                show_mapping_preview = not (arguments.summary or arguments.compact)
                if requirement_preview and show_mapping_preview:
                    print(f"work_requirements: {', '.join(requirement_preview)}")
                section_preview = strings(work_mapping.get("standard_sections_preview"))
                if section_preview and show_mapping_preview:
                    print(f"work_standard_sections: {', '.join(section_preview)}")
                pair_preview = work_mapping.get("requirement_section_pairs_preview")
                if (
                    isinstance(pair_preview, list)
                    and pair_preview
                    and show_mapping_preview
                ):
                    pairs = [
                        f"{row.get('lab_requirement_id')} -> {row.get('standard_section')}"
                        for row in pair_preview
                        if isinstance(row, dict)
                    ]
                    if pairs:
                        print(f"work_direct_pairs: {', '.join(pairs)}")
            mapping_commands = result.get("work_mapping_commands")
            if isinstance(mapping_commands, dict):
                for name, command in mapping_commands.items():
                    if isinstance(command, str) and command:
                        print(f"work_matrix_{name}: {command}")
            print(
                f"mapped work handles: standard_sections={len(sections)}; "
                f"plan_ids={len(plan_ids)}; "
                f"catalog_gaps={len(strings(result.get('catalog_gap_plan_ids')))}"
            )
            if result.get("next_action"):
                print(f"next_action: {compact_prose(result['next_action'], 360)}")
            if result.get("process_package_junit_artifact"):
                print(
                    "process_package_junit_artifact: "
                    f"{result['process_package_junit_artifact']}"
                )
            if result.get("process_package_catalog_verifier"):
                print(
                    "process_package_catalog_verifier: "
                    f"{result['process_package_catalog_verifier']}"
                )
            package_lane_handles = (
                ("process_package", "process_package"),
                ("process_package_timestamped", "timestamped"),
                ("process_package_parameterized", "parameterized"),
                ("process_package_connection_loss", "connection_loss"),
                (
                    "process_package_connection_loss_delete_objects",
                    "connection_loss_delete_objects",
                ),
                ("process_package_object_registration", "object_registration"),
                ("process_package_named_registration", "named_registration"),
                ("process_package_attribute_update", "attribute_update"),
                ("process_package_directed_retraction", "directed_retraction"),
                ("process_package_federation_save_restore", "federation_save_restore"),
                (
                    "process_package_federation_save_restore_failure",
                    "federation_save_restore_failure",
                ),
                (
                    "process_package_federation_save_restore_abort",
                    "federation_save_restore_abort",
                ),
                (
                    "process_package_federation_save_restore_status",
                    "federation_save_restore_status",
                ),
            )
            for prefix, _label in package_lane_handles:
                test_name = result.get(f"{prefix}_test")
                ctest_label = result.get(f"{prefix}_ctest_filter")
                if test_name or ctest_label:
                    print(
                        f"{prefix}: "
                        f"test={test_name or '<none>'}; label={ctest_label or '<none>'}"
                    )
            print("commands:")
            for command in result.get("commands", [])[:5]:
                print(f"  {command}")
            if len(result.get("commands", [])) > 5:
                print(
                    "  (additional package/JUnit commands available with work --compact or --json)"
                )
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
        work_mapping = result.get("work_mapping")
        if isinstance(work_mapping, dict):
            print(
                "work_mapping: "
                f"cases={work_mapping.get('case_count', 0)}; "
                f"mapped={work_mapping.get('mapped_case_count', 0)}; "
                f"requirements={work_mapping.get('requirement_count', 0)}; "
                f"standard_sections={work_mapping.get('standard_section_count', 0)}; "
                f"direct_pairs={work_mapping.get('resolved_requirement_section_pair_count', 0)}"
            )
        mapping_commands = result.get("work_mapping_commands")
        if isinstance(mapping_commands, dict):
            for name, command in mapping_commands.items():
                if isinstance(command, str) and command:
                    print(f"work_matrix_{name}: {command}")
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
            print(f"process_package_ctest: {result['process_package_ctest_filter']}")
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
        if result.get("process_package_connection_loss_delete_objects_test"):
            print(
                "process_package_connection_loss_delete_objects_test: "
                f"{result['process_package_connection_loss_delete_objects_test']}"
            )
        if result.get("process_package_connection_loss_delete_objects_ctest_filter"):
            print(
                "process_package_connection_loss_delete_objects_ctest: "
                f"{result['process_package_connection_loss_delete_objects_ctest_filter']}"
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
        for prefix, label in (
            ("process_package_federation_save_restore", "federation_save_restore"),
            (
                "process_package_federation_save_restore_failure",
                "federation_save_restore_failure",
            ),
            (
                "process_package_federation_save_restore_abort",
                "federation_save_restore_abort",
            ),
            (
                "process_package_federation_save_restore_status",
                "federation_save_restore_status",
            ),
        ):
            test_name = result.get(f"{prefix}_test")
            ctest_label = result.get(f"{prefix}_ctest_filter")
            if test_name:
                print(f"{prefix}_test: {test_name}")
            if ctest_label:
                print(f"{prefix}_ctest: {ctest_label}")
        if result.get("process_package_catalog_verifier"):
            print(
                "process_package_catalog_verifier: "
                f"{result['process_package_catalog_verifier']}"
            )
        if result.get("process_package_junit_artifact"):
            print(
                "process_package_junit_artifact: "
                f"{result['process_package_junit_artifact']}"
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
            all_slices if not arguments.limit else all_slices[: arguments.limit]
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
            print(
                f"  source: {source_location_text({'source_locations': slice_value.get('source_locations', [])})}"
            )
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
                print(
                    f"No indexed roadmap item matched: {arguments.id}", file=sys.stderr
                )
            return 1
        shown_tests = (
            matching_tests if not arguments.limit else matching_tests[: arguments.limit]
        )
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
        if arguments.summary:
            print(f"{item_value.get('id')}: {item_value.get('title')}")
            print(
                f"status={item_value.get('status')}; "
                f"priority={item_value.get('priority')}; "
                f"cases={len(matching_tests)}; showing={len(shown_tests)}"
            )
            live_focus = live_item_focus(item_value, matching_tests, index=index)
            if live_focus:
                print(f"focus={compact_prose(live_focus, 180)}")
            live_action = live_next_action(item_value)
            if live_action:
                print(f"next={compact_prose(live_action, 180)}")
            for test in shown_tests:
                requirements = strings(test.get("lab_requirement_ids"))
                sections = strings(test.get("standard_sections"))
                pair_count = len(requirement_section_mapping_rows(test))
                query = test.get("id") or test.get("test_case") or "<unnamed>"
                print(
                    f"- {query}: {compact_prose(test.get('test_case'), 120)}; "
                    f"state={test.get('status', '<unspecified>')}; "
                    f"source={source_location_text(test)}; "
                    f"requirements={len(requirements)}; sections={len(sections)}; "
                    f"direct_pairs={pair_count}"
                )
                print(
                    "  case=python tools/query_rti_work.py case "
                    f'"{query}" --summary --compact'
                )
            if len(shown_tests) < len(matching_tests):
                print(
                    f"... {len(matching_tests) - len(shown_tests)} more cases; "
                    "use --limit 0 to list all or query one exact case for mappings"
                )
            return 0
        print(f"{item_value.get('id')}: {item_value.get('title')}")
        print(
            f"status: {item_value.get('status')}; priority: {item_value.get('priority')}"
        )
        print(f"anchor: {item_value.get('roadmap_anchor')}")
        query_tags = strings(item_value.get("query_tags"))
        if arguments.compact:
            print(f"query tag count: {len(query_tags)}")
        else:
            print(f"query tags: {', '.join(query_tags) or '<none>'}")
        live_focus = live_item_focus(item_value, matching_tests, index=index)
        if live_focus:
            focus = compact_prose(live_focus) if arguments.compact else live_focus
            print(f"current focus: {focus}")
        if item_value.get("next_task") and not arguments.compact:
            task = (
                compact_prose(item_value.get("next_task"))
                if arguments.compact
                else item_value.get("next_task")
            )
            print(f"next task: {task}")
        handles = item_handles(item_value, arguments.compact)
        if handles:
            print(f"handles: {' '.join(handles)}")
        live_action = live_next_action(item_value)
        next_action = (
            compact_prose(live_action)
            if arguments.compact and live_action
            else live_action
        )
        print(f"next: {next_action}")
        if len(shown_tests) == len(matching_tests):
            print(f"{len(matching_tests)} tagged Catch2 cases.")
        else:
            print(
                f"{len(matching_tests)} tagged Catch2 cases; showing {len(shown_tests)}."
            )
        if shown_tests:
            renderer = (
                text_test_summary
                if arguments.summary
                else lambda test: text_test(test, arguments.verbose, arguments.compact)
            )
            print("\n".join(renderer(test) for test in shown_tests))
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
                item_tests(family_item, tests, index=index)
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
            pair for pair in requirement_section_pairs if pair[1]
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
                else f" in lane {scope_lane!r}"
                if scope_lane
                else ""
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

    if arguments.command == "tck-gaps":
        if arguments.limit < 0:
            message = "tck-gaps --limit must be non-negative"
            if arguments.json:
                print(json.dumps({"found": False, "error": message}, indent=2))
            else:
                print(message, file=sys.stderr)
            return 2
        try:
            catalog = load_json(arguments.catalog)
            result = cpp_tck_requirement_gap_inventory(
                standard_requirements,
                catalog,
                tests,
                query=arguments.query,
                document=arguments.document,
                clause=arguments.clause,
                coverage_kind=arguments.coverage_kind,
                limit=arguments.limit,
            )
        except (ValueError, OSError) as error:
            print(f"query_rti_work: {error}", file=sys.stderr)
            return 2

        if arguments.json:
            if arguments.summary:
                result = cpp_tck_requirement_gap_summary_data(result)
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0

        scope = []
        for label, value in (
            ("query", arguments.query),
            ("document", arguments.document),
            ("clause", arguments.clause),
            ("coverage_kind", arguments.coverage_kind),
        ):
            if value:
                scope.append(f"{label}={value!r}")
        scope_text = f" ({', '.join(scope)})" if scope else ""
        print(
            "Portable C++ TCK direct requirement coverage" + scope_text + ": "
            f"{result['mapped_requirement_count']} of "
            f"{result['total_requirement_count']} mapped "
            f"({result['coverage_percent']:.2f}%); "
            f"{result['uncovered_requirement_count']} absent from direct catalog mappings; "
            f"{result['promoted_requirement_count']} promoted, "
            f"{result['candidate_only_requirement_count']} candidate-only; "
            f"{result['invalid_mapping_count']} unresolved Lab mapping(s); "
            f"{result['section_disagreement_count']} section crosswalk review(s); "
            f"{result['non_lab_mapping_count']} non-Lab references catalog-wide"
        )
        groups = result.get("groups", [])
        if groups:
            suffix = (
                f"; showing {len(groups)} of {result['group_count']} subsections"
                if result["shown_group_count"] < result["group_count"]
                else ""
            )
            print("C++ TCK-unmapped subsections" + suffix + ":")
            for group in groups:
                print(
                    f"- {group['standard_section']}: "
                    f"{group['uncovered_requirement_count']} absent / "
                    f"{group['total_requirement_count']} total; "
                    f"direct={group['mapped_requirement_count']}, "
                    f"promoted={group['promoted_requirement_count']}, "
                    f"candidate-only={group['candidate_only_requirement_count']}; "
                    f"sample={', '.join(group.get('uncovered_requirement_ids', []))}"
                )
        if not (arguments.summary or arguments.compact):
            requirements = result.get("requirements", [])
            if requirements:
                suffix = (
                    f"; showing {len(requirements)} of "
                    f"{result['uncovered_requirement_count']} records"
                    if result["shown_requirement_count"]
                    < result["uncovered_requirement_count"]
                    else ""
                )
                print("Requirement records" + suffix + ":")
                for requirement in requirements:
                    print(
                        f"- {requirement['id']} -> "
                        f"{requirement.get('standard_section') or '<unresolved>'}: "
                        f"{compact_prose(requirement.get('title'), 160)}; "
                        f"mapped Catch2 cases={requirement['catch2_case_count']}"
                    )
                    if requirement.get("catch2_requirement_query"):
                        print(f"  catch2={requirement['catch2_requirement_query']}")
                    source = requirement.get("source")
                    if isinstance(source, dict) and source.get("path"):
                        line = source.get("line")
                        location = (
                            f"{source['path']}:{line}" if line else source["path"]
                        )
                        print(f"  standard_source={location}")
                    for case in requirement.get("catch2_cases", []):
                        print(
                            f"  - {case.get('case_id')}: {case.get('test_case')} "
                            f"({case.get('source_location') or 'source unlocated'})"
                        )
                    if requirement.get("statement"):
                        print(
                            "  statement="
                            + compact_prose(requirement.get("statement"), 320)
                        )
        if result["invalid_mapping_count"] and not arguments.summary:
            print("Unresolved Lab catalog mappings:")
            for finding in result.get("invalid_mappings", []):
                print(
                    f"- {finding.get('scenario_id')}: {finding.get('requirement_id')} "
                    f"catalog={finding.get('catalog_section')!r}, "
                    f"expected={finding.get('expected_section')!r}"
                )
        if result["section_disagreement_count"] and not arguments.summary:
            print(
                "Catalog subsection crosswalk reviews (not counted as uncovered IDs):"
            )
            for finding in result.get("section_disagreements", []):
                print(
                    f"- {finding.get('scenario_id')}: {finding.get('requirement_id')} "
                    f"catalog={finding.get('catalog_section')!r}, "
                    f"exported/crosswalk={finding.get('expected_section')!r}"
                )
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
        if arguments.family:
            coverage_text = (
                f"this family maps {result['covered_requirement_count']} of "
                f"{result['total_requirement_count']} filtered requirements "
                f"({result['coverage_percent']:.2f}%); "
                f"{result['uncovered_requirement_count']} are not mapped to this family; "
                f"across all families, "
                f"{result['globally_covered_requirement_count']} covered and "
                f"{result['globally_uncovered_requirement_count']} uncovered"
            )
        else:
            coverage_text = (
                f"{result['uncovered_requirement_count']} globally uncovered of "
                f"{result['total_requirement_count']} filtered; "
                f"{result['covered_requirement_count']} globally covered "
                f"({result['coverage_percent']:.2f}% mapped)"
            )
        print("2025 requirement gaps" + scope_text + ": " + coverage_text)
        groups = result.get("groups", [])
        if groups:
            suffix = (
                f"; showing {len(groups)} of {result['group_count']} subsections"
                if result["shown_group_count"] < result["group_count"]
                else ""
            )
            heading = (
                "Family-local uncovered subsections"
                if arguments.family
                else "Globally uncovered subsections"
            )
            print(heading + suffix + ":")
            for group in groups:
                global_coverage = (
                    f"; globally {group['globally_covered_requirement_count']} covered / "
                    f"{group['globally_uncovered_requirement_count']} uncovered"
                    if arguments.family
                    else ""
                )
                local_coverage_label = (
                    "family-covered" if arguments.family else "globally-covered"
                )
                print(
                    f"- {group['standard_section']}: "
                    f"{group['uncovered_requirement_count']} uncovered / "
                    f"{group['total_requirement_count']} total; "
                    f"{local_coverage_label}={group['covered_requirement_count']}"
                    f"{global_coverage}; "
                    f"sample={', '.join(group.get('uncovered_requirement_ids', []))}"
                )
        requirements = [] if arguments.summary else result.get("requirements", [])
        if requirements:
            suffix = (
                f"; showing {len(requirements)} of {result['uncovered_requirement_count']} records"
                if result["shown_requirement_count"]
                < result["uncovered_requirement_count"]
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
                    + (
                        f" [global coverage: {requirement.get('global_coverage')}]"
                        if arguments.family
                        else ""
                    )
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
            focused_only=arguments.focused,
            limit=requested_limit,
        )
        if arguments.json:
            print(json.dumps(result, indent=2, sort_keys=True))
            return 0 if result.get("found") else 1
        if not result.get("found"):
            print(
                result.get("error", "lane inventory could not be resolved"),
                file=sys.stderr,
            )
            return 1
        scope = f" in roadmap family {requested_family!r}" if requested_family else ""
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
                filter_text = (
                    "lanes with explicit or unclassified missing requirement mappings."
                )
            print(f"Filter: {filter_text}")
        if result.get("focused_only"):
            print(
                "Filter: focused roadmap lanes only "
                f"({result.get('focused_tag_count', 0)} indexed aliases)."
            )
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
            if lane_value.get(
                "assertion_count_source"
            ) == "indexed-lane-total" and lane_value.get(
                "recorded_assertion_count"
            ) != lane_value.get("assertion_count"):
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
            traceability_state(test) for test in unmapped_tests
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
            lane_owners.get(arguments.tag) if isinstance(lane_owners, dict) else None
        )
        lane_handles = {}
        configured_lane_handles = (
            mapping.get("lane_handles", {}) if isinstance(mapping, dict) else {}
        )
        if isinstance(configured_lane_handles, dict):
            configured = configured_lane_handles.get(arguments.tag)
            if isinstance(configured, dict):
                lane_handles = dict(configured)
        result: JsonObject = cast(
            JsonObject,
            {
                "tag": arguments.tag,
                "roadmap_owner": roadmap_owner,
                "lane_handles": lane_handles,
                "count": matched_count,
                "shown_count": len(selected),
                "tests": selected,
            },
        )
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
        if arguments.group_by == "test":
            selected = matches if not arguments.limit else matches[: arguments.limit]
            result = {
                "query": query,
                "requested_query": arguments.query,
                "active_lane": active_lane,
                "found": bool(matches),
                "match_kind": match_kind,
                "group_by": "test",
                "count": len(matches),
                "shown_count": len(selected),
                "tests": [matrix_summary_data(test, index) for test in selected],
            }
        else:
            crosswalk = matrix_crosswalk_rows(
                matches,
                arguments.group_by,
                match_kind=match_kind,
                query=query,
            )
            selected = (
                crosswalk if not arguments.limit else crosswalk[: arguments.limit]
            )
            result = {
                "query": query,
                "requested_query": arguments.query,
                "active_lane": active_lane,
                "found": bool(crosswalk),
                "match_kind": match_kind,
                "group_by": arguments.group_by,
                "count": len(crosswalk),
                "shown_count": len(selected),
                "rows": selected,
                "matched_test_count": len(matches),
            }
    elif arguments.command == "search":
        search_terms = [str(term) for term in arguments.query]
        all_terms = arguments.all_terms or (
            len(search_terms) > 1 and not arguments.any_term
        )
        selected = select_search_tests(
            tests,
            search_terms,
            all_terms=all_terms,
        )
        matched_count = len(selected)
        if arguments.limit:
            selected = selected[: arguments.limit]
        result = {
            "query": " ".join(search_terms),
            "terms": search_terms,
            "match_mode": "all-terms" if all_terms else "any-term",
            "count": matched_count,
            "shown_count": len(selected),
            "tests": selected,
        }
    else:
        print(
            f"query_rti_work: unsupported command {arguments.command}", file=sys.stderr
        )
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
                    card
                    if not arguments.summary
                    else {
                        key: card.get(key)
                        for key in (
                            "query",
                            "id",
                            "test_case",
                            "status",
                            "source_locations",
                            "source_state",
                            "replacement_plan_ids",
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
            print(f"Trace ({kind}): {result['count']} matched; showing {shown_count}.")
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
        group_by = result.get("group_by") or "test"
        if shown_count == result["count"]:
            print(
                f"Traceability matrix ({kind}; {query}; group_by={group_by}): "
                f"{result['count']} matched."
            )
        else:
            print(
                f"Traceability matrix ({kind}; {query}; group_by={group_by}): "
                f"{result['count']} matched; "
                f"showing {shown_count}."
            )
        if group_by == "test":
            rendered = "\n".join(
                text_matrix_row(
                    test,
                    index,
                    compact=arguments.compact or arguments.summary,
                )
                for test in result["tests"]
            )
        else:
            rendered = "\n".join(
                text_matrix_crosswalk_row(row) for row in result.get("rows", [])
            )
        print(rendered)
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
            print(
                f"No unplanned C++ TEST_CASE declarations{path_suffix}.",
                file=sys.stderr,
            )
            return 1
        shown_count = result["shown_count"]
        if shown_count == result["count"]:
            print(
                f"{result['count']} unplanned C++ TEST_CASE declarations{path_suffix}."
            )
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
        elif arguments.command == "lane" and result.get("lane_handles"):
            print(
                f"No Catch2 cases are attached to lane {arguments.tag!r}; "
                "the indexed execution handle is available below."
            )
            ctest_command = lane_ctest_command(result["lane_handles"])
            for name, value in result["lane_handles"].items():
                if name == "ctest_command" and ctest_command:
                    continue
                print(f"{name}: {value}")
            if ctest_command:
                print(f"ctest_command: {ctest_command}")
            return 0
        else:
            print(
                f"No Catch2 cases matched {arguments.command} query.", file=sys.stderr
            )
        return 1
    shown_count = result.get("shown_count", result["count"])
    if arguments.command == "lane" and result.get("roadmap_owner"):
        print(f"Roadmap owner: {result['roadmap_owner']}")
    if arguments.command == "lane" and result.get("lane_handles"):
        ctest_command = lane_ctest_command(result["lane_handles"])
        for name, value in result["lane_handles"].items():
            if name == "ctest_command" and ctest_command:
                continue
            print(f"{name}: {value}")
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
            print(
                f"{result['count']} Catch2 plan cases{scope_text} have no matching TEST_CASE source declaration."
            )
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
            print(
                f"{result['count']} unmapped Catch2 cases{scope_text}; showing {shown_count}."
            )
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
    if arguments.summary or (
        arguments.command in discovery_commands and not arguments.verbose
    ):

        def renderer(test: JsonObject) -> str:
            if arguments.command == "search":
                row = (
                    text_test(test, True, arguments.compact)
                    if arguments.verbose
                    else text_search_summary_row(test)
                )
            else:
                row = text_query_summary_row(
                    arguments.command,
                    test,
                    index,
                    getattr(arguments, "query", None),
                )
            if arguments.command in discovery_commands:
                case_id = json.dumps(str(test.get("id")), ensure_ascii=False)
                case_handle = f"case: python tools/query_rti_work.py case {case_id}"
                if arguments.command == "search":
                    # Search is the broadest normal discovery surface. Keep
                    # five-hit output within its line budget by attaching the
                    # exact follow-up to the bounded mapping preview.
                    row += f"; {case_handle}"
                else:
                    row += f"\n  {case_handle}"
            return row

        if arguments.command in discovery_commands:
            print(
                "Use the case command for focused execution; --verbose for provenance; --limit 0 for all matches."
            )
    else:
        renderer = lambda test: text_test(
            test,
            arguments.verbose,
            arguments.compact,
        )
    print("\n".join(renderer(test) for test in result["tests"]))
    return 0
