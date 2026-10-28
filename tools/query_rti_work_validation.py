"""Validation of the indexed roadmap and its source mappings."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from pathlib import Path

if __package__:
    from .query_rti_work import (
        REPOSITORY_ROOT,
        JsonObject,
        TestSourceLocation,
        is_planned_test,
        item_matches_lane,
        json_object_rows,
        load_cpp_tck_scenarios,
        scoped_plan_tests,
        source_location_text,
        source_locations_for_test,
        strings,
        test_lane_identifiers,
        unplanned_source_cases,
    )
else:
    from query_rti_work import (
        REPOSITORY_ROOT,
        JsonObject,
        TestSourceLocation,
        is_planned_test,
        item_matches_lane,
        json_object_rows,
        load_cpp_tck_scenarios,
        scoped_plan_tests,
        source_location_text,
        source_locations_for_test,
        strings,
        test_lane_identifiers,
        unplanned_source_cases,
    )


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
    known_tags = {tag for test in tests for tag in test_lane_identifiers(test)}
    # Portable C++ TCK lanes are a separate test catalog, not Catch2 rows.
    # Roadmap items may point at either catalog, so their lane query handles
    # are valid even when no Catch2 case carries the same tag.
    for scenario in load_cpp_tck_scenarios().values():
        known_tags.update(strings(scenario.get("lane_queries")))
    # Keep the roadmap's verified-slice cards queryable as well. These cards
    # preserve exact CTest lanes while a TCK scenario is being promoted or
    # represented in the separate catalog.
    recent_completed_slices = index.get("recent_completed_slices", [])
    if isinstance(recent_completed_slices, list):
        for completed_slice in recent_completed_slices:
            if isinstance(completed_slice, dict):
                known_tags.update(strings(completed_slice.get("lane_queries")))
                known_tags.update(strings(completed_slice.get("primary_lane")))
                known_tags.update(strings(completed_slice.get("focus_lane")))
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
        test_id for test in tests if isinstance(test_id := test.get("id"), str)
    }
    known_cpp_tck_scenario_ids = set(load_cpp_tck_scenarios())
    known_test_cases = {
        test_case
        for test in tests
        if isinstance(test_case := test.get("test_case"), str)
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
                    errors.append(f"active_handoff {field} must be a non-empty string")
            family_id = active_handoff.get("family")
            if isinstance(family_id, str) and family_id not in known_family_ids:
                errors.append(
                    "active_handoff family is absent from the roadmap index: "
                    f"{family_id}"
                )
            seed_id = active_handoff.get("mapping_seed_plan_id")
            seed_is_known = isinstance(seed_id, str) and (
                seed_id in known_test_ids
                or (
                    family_id == "portable-cpp-tck-2025"
                    and seed_id in known_cpp_tck_scenario_ids
                )
            )
            if isinstance(seed_id, str) and not seed_is_known:
                errors.append(
                    "active_handoff mapping_seed_plan_id is absent from its test catalog: "
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
                or any(
                    not isinstance(value, str) or not value.strip()
                    for value in acceptance
                )
            ):
                errors.append(
                    "active_handoff acceptance must contain only non-empty strings"
                )
            api_surfaces = active_handoff.get("api_surfaces")
            if api_surfaces is not None and (
                not isinstance(api_surfaces, list)
                or any(
                    not isinstance(value, str) or not value.strip()
                    for value in api_surfaces
                )
            ):
                errors.append(
                    "active_handoff api_surfaces must contain only non-empty strings"
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
        scoped_item_ids = (
            {str(family_item.get("id") or "").casefold()}
            if isinstance(family_item, dict)
            else set()
        )
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
                errors.append(
                    f"{item_id or '<unnamed>'} anchor is absent from {roadmap_path}"
                )
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
            "next_process_package_connection_loss_delete_objects_test",
            "next_process_package_connection_loss_delete_objects_ctest_filter",
            "next_process_package_object_registration_test",
            "next_process_package_object_registration_ctest_filter",
            "next_process_package_named_registration_test",
            "next_process_package_named_registration_ctest_filter",
            "next_process_package_attribute_update_test",
            "next_process_package_attribute_update_ctest_filter",
            "next_process_package_directed_retraction_test",
            "next_process_package_directed_retraction_ctest_filter",
            "next_process_package_federation_save_restore_test",
            "next_process_package_federation_save_restore_ctest_filter",
            "next_process_package_federation_save_restore_failure_test",
            "next_process_package_federation_save_restore_failure_ctest_filter",
            "next_process_package_federation_save_restore_abort_test",
            "next_process_package_federation_save_restore_abort_ctest_filter",
            "next_process_package_federation_save_restore_status_test",
            "next_process_package_federation_save_restore_status_ctest_filter",
            "next_process_package_catalog_verifier",
            "next_process_package_junit_artifact",
        ):
            value = item.get(field)
            if value is not None and (not isinstance(value, str) or not value.strip()):
                errors.append(
                    f"{item_id or '<unnamed>'} {field} must be a non-empty string"
                )
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
        if next_task is not None and (
            not isinstance(next_task, str) or not next_task.strip()
        ):
            errors.append(
                f"{item_id or '<unnamed>'} next_task must be a non-empty string"
            )
        next_lane = item.get("next_lane")
        if isinstance(next_lane, str) and next_lane not in known_tags:
            errors.append(
                f"{item_id or '<unnamed>'} next_lane is absent from the Catch2 plan: {next_lane}"
            )
        next_source_lane = item.get("next_source_lane")
        if isinstance(next_source_lane, str) and next_source_lane not in known_tags:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_lane is absent from the Catch2 plan: {next_source_lane}"
            )
        next_test_query = item.get("next_test_query")
        if isinstance(next_test_query, str) and next_test_query not in known_test_cases:
            errors.append(
                f"{item_id or '<unnamed>'} next_test_query is absent from the Catch2 plan: {next_test_query}"
            )
        next_source_test_query = item.get("next_source_test_query")
        next_source_location = item.get("next_source_location")
        next_source_state = item.get("next_source_state")
        if next_source_test_query is not None and not isinstance(
            next_source_test_query, str
        ):
            errors.append(
                f"{item_id or '<unnamed>'} next_source_test_query must be a non-empty string"
            )
        if next_source_location is not None and not isinstance(
            next_source_location, str
        ):
            errors.append(
                f"{item_id or '<unnamed>'} next_source_location must be a non-empty string"
            )
        if next_source_state is not None and next_source_state not in {
            "unplanned-source",
            "planned",
            "exhausted",
        }:
            errors.append(
                f"{item_id or '<unnamed>'} next_source_state must be unplanned-source, planned, or exhausted"
            )
        if isinstance(next_source_test_query, str) and next_source_test_query.strip():
            if (
                not isinstance(next_source_location, str)
                or not next_source_location.strip()
            ):
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
            if not (
                next_ctest_filter.startswith("[") and next_ctest_filter.endswith("]")
            ):
                errors.append(
                    f"{item_id or '<unnamed>'} next_ctest_filter must be a Catch2 tag filter: {next_ctest_filter}"
                )
            elif next_ctest_filter[1:-1] not in known_tags:
                errors.append(
                    f"{item_id or '<unnamed>'} next_ctest_filter is absent from the Catch2 plan: {next_ctest_filter}"
                )
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
                errors.append(
                    f"{item_id or '<unnamed>'} {field} must contain only non-empty strings"
                )
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
                    errors.append(
                        f"{item_id or '<unnamed>'} next_plan_ids references unknown Catch2 plan id: {plan_id}"
                    )
        next_source_requirement_ids = item.get("next_source_requirement_ids")
        if isinstance(next_source_requirement_ids, list):
            for requirement_id in next_source_requirement_ids:
                if (
                    isinstance(requirement_id, str)
                    and requirement_id not in standard_requirements
                ):
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
        if catalog_gap_plan_ids is not None and not isinstance(
            catalog_gap_plan_ids, list
        ):
            errors.append(
                f"{item_id or '<unnamed>'} catalog_gap_plan_ids must be an array"
            )
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
        item_tags = strings(item.get("query_tags")) + strings(
            item.get("focused_lane_tags")
        )
        if focus_lane is not None:
            item_tags = [
                tag for tag in item_tags if tag.casefold() == focus_lane.casefold()
            ]
        for tag in item_tags:
            if tag not in known_tags:
                errors.append(
                    f"{item_id or '<unnamed>'} query tag is absent from the Catch2 plan: {tag}"
                )
    mapped_tests_by_id = {
        test.get("id"): test
        for test in tests
        if isinstance(test, dict) and isinstance(test.get("id"), str)
    }
    # A lane/family check is the fast focused gate used while developing a
    # slice.  Validate only the selected test rows here; the unqualified
    # ``check`` command still receives all tests through ``scoped_tests`` and
    # remains the repository-wide integrity audit.  Keeping this aligned with
    # the roadmap-item scope prevents unrelated mapping defects from blocking
    # a focused lane check.
    for test in scoped_tests:
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
                    f"Catch2 test {test_id} repeats {field}: " + ", ".join(duplicates)
                )
        for requirement_id in strings(
            test.get("selected_requirements_lab_requirement_ids")
        ):
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
            if (
                canonical_section is not None
                and canonical_section not in mapped_sections
            ):
                errors.append(
                    f"Catch2 test {test_id} omits canonical 2025 standard section "
                    f"{canonical_section} for requirement {requirement_id}"
                )
    recent_slices = index.get("recent_completed_slices", [])
    if not isinstance(recent_slices, list):
        if recent_slices is not None:
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
        test_id for test in tests if isinstance(test_id := test.get("id"), str)
    }
    scoped_test_ids = {
        test_id for test in scoped_tests if isinstance(test_id := test.get("id"), str)
    }
    scoped_test_cases = {
        test_case
        for test in scoped_tests
        if isinstance(test_case := test.get("test_case"), str)
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
        if (
            focus_family is not None
            and slice_value.get("plan_id") not in scoped_test_ids
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
            matches: list[JsonObject] = []
        else:
            matches = [test for test in tests if test.get("test_case") == test_query]
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
                errors.append(
                    f"{label} duplicates recent completion test_query: {test_query}"
                )
            else:
                seen_recent_queries.add(test_query)
        source_location = slice_value.get("source_location")
        expected_locations = {
            f"{location.get('path')}:{location.get('line')}"
            for test in matches
            for location in json_object_rows(test.get("source_locations"))
            if isinstance(location.get("path"), str)
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
            not isinstance(section, str) or not section.strip() for section in sections
        ):
            errors.append(f"{label} standard_sections must contain non-empty strings")
        elif any(section not in known_standard_sections for section in sections):
            errors.append(
                f"{label} standard_sections references an unknown 2025 section"
            )
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
            and isinstance(source_query := item.get("next_source_test_query"), str)
            and source_query.strip()
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
            if (
                pointer_query != queue_head_query
                or pointer_location != queue_head_location
            ):
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
            and focus_lane is None
            and focus_family is None
        ):
            errors.append(
                f"Catch2 test {test.get('id', '<unnamed>')} has no matching TEST_CASE source location"
            )
    documents = index.get("documents", [])
    if not isinstance(documents, list):
        errors.append("roadmap index documents must be an array")
        documents = []
    for document in documents:
        if not isinstance(document, dict):
            errors.append("roadmap index document entry must be an object")
            continue
        document_path = document.get("path")
        if (
            not isinstance(document_path, str)
            or not (REPOSITORY_ROOT / document_path).is_file()
        ):
            errors.append(f"roadmap index document is missing: {document_path}")
    return errors
