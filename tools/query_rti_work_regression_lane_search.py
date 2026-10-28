# ruff: noqa: TRY004
# These AssertionErrors report broken query-result contracts, not bad inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import LaneSearchInputs
from query_rti_work_regression_support import TARGET_TEST


def run_lane_search(inputs: LaneSearchInputs) -> int:
    index = inputs.index
    pairs = inputs.pairs
    source_locations = inputs.source_locations
    tests = inputs.tests
    # Lane discovery is the bounded bridge between a family card and one exact
    # test.  It must expose mapping state and a copyable next-test handle
    # without requiring a broad tag/source search.
    family_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        limit=0,
    )
    if not family_lanes.get("found") or not family_lanes.get("lanes"):
        raise AssertionError("family lane inventory did not resolve any exact tags")
    deletion_lane = next(
        (
            row
            for row in family_lanes["lanes"]
            if isinstance(row, dict) and row.get("tag") == "object-instance-deletion"
        ),
        None,
    )
    if not isinstance(deletion_lane, dict):
        raise AssertionError(
            "object-instance-deletion lane was absent from family inventory"
        )
    if deletion_lane.get("mapped_count") != 1 or deletion_lane.get(
        "unclassified_count"
    ):
        raise AssertionError("object-instance-deletion lane mapping counts drifted")
    if deletion_lane.get("resolved_requirement_section_pair_count") != 8:
        raise AssertionError("object-instance-deletion direct mapping count drifted")
    if not deletion_lane.get("focus_command") or not deletion_lane.get("ctest_command"):
        raise AssertionError("object-instance-deletion lane lost execution handles")
    unmapped_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        limit=0,
    )
    if not unmapped_lanes.get("found") or not unmapped_lanes.get("lanes"):
        raise AssertionError("unmapped family lane inventory was unexpectedly empty")
    object_lane = next(
        (
            row
            for row in unmapped_lanes["lanes"]
            if isinstance(row, dict) and row.get("tag") == "object-management"
        ),
        None,
    )
    if not isinstance(object_lane, dict):
        raise AssertionError("unmapped lane inventory lost the object-management queue")
    if object_lane.get("unclassified_count"):
        raise AssertionError(
            "object-management lane still reports an unclassified catalog row"
        )
    if object_lane.get("explicit_disposition_count", 0) < 9:
        raise AssertionError(
            "object-management lane lost its explicit catalog dispositions"
        )
    if object_lane.get("next_test") == (
        "Embedded receive-order Update Attribute Values honors 2025 passel and callback lifecycle"
    ):
        raise AssertionError(
            "mapped receive-order update still appears in the unmapped queue"
        )
    if object_lane.get("next_test") is not None:
        raise AssertionError(
            "explicit no-standalone-surface rows leaked into the implementation head"
        )
    if not object_lane.get("review_test") or not object_lane.get(
        "review_trace_command"
    ):
        raise AssertionError(
            "unmapped lane inventory omitted its explicit review handle"
        )

    # A family work card must expose live mapping totals, not only the
    # legacy pointer fields used by older active handoffs. This prevents a
    # broad family from appearing to have zero standard sections or plan
    # rows when its exact lane inventory is already fully indexed.
    transport_work = query_rti_work.indexed_work_slice(
        index,
        tests,
        "transport-and-conformance",
        source_locations,
    )
    transport_mapping = transport_work.get("work_mapping")
    if not isinstance(transport_mapping, dict):
        raise AssertionError("family work card omitted its live mapping totals")
    # Compare against the selected plan rows, not a historical snapshot that
    # has to be edited every time another C++ case joins this family.
    _, transport_tests = query_rti_work.scoped_plan_tests(
        index, tests, family="transport-and-conformance"
    )
    transport_pairs = {
        (row["lab_requirement_id"], row.get("standard_section"))
        for test in transport_tests
        for row in query_rti_work.requirement_section_mapping_rows(test)
        if row.get("lab_requirement_id")
    }
    expected_transport_totals = {
        "case_count": len(transport_tests),
        "mapped_case_count": sum(
            bool(test.get("requirements")) for test in transport_tests
        ),
        "requirement_count": len({pair[0] for pair in transport_pairs}),
        "standard_section_count": len(
            {
                section
                for test in transport_tests
                for section in test.get("standard_sections", [])
            }
        ),
        "resolved_requirement_section_pair_count": sum(
            bool(pair[1]) for pair in transport_pairs
        ),
        "assertion_count": sum(
            value
            for test in transport_tests
            for value in [test.get("assertions")]
            if isinstance(value, int)
        ),
    }
    for field, expected in expected_transport_totals.items():
        if transport_mapping.get(field) != expected:
            raise AssertionError(
                f"transport family {field} diverges from its plan rows"
            )
    transport_work_commands = transport_work.get("work_mapping_commands")
    if (
        not isinstance(transport_work_commands, dict)
        or "--group-by requirement"
        not in transport_work_commands.get("requirement", "")
        or "--group-by section" not in transport_work_commands.get("section", "")
    ):
        raise AssertionError("family work card lost direct crosswalk commands")

    # Keep intentional no-standalone-surface dispositions separate from rows
    # that still need a Lab mapping decision.  The CLI exposes this as
    # ``lanes --disposition`` so a family inventory can be queried without
    # treating an explicit disposition as missing work.
    explicit_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        disposition="explicit",
        limit=0,
    )
    if explicit_lanes.get("disposition") != "explicit":
        raise AssertionError("explicit lane inventory lost its disposition filter")
    if not explicit_lanes.get("lanes") or any(
        row.get("explicit_disposition_count", 0) <= 0
        for row in explicit_lanes.get("lanes", [])
        if isinstance(row, dict)
    ):
        raise AssertionError("explicit lane inventory returned a non-explicit lane")
    unclassified_lanes = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        unmapped_only=True,
        disposition="unclassified",
        limit=0,
    )
    if unclassified_lanes.get("disposition") != "unclassified":
        raise AssertionError("unclassified lane inventory lost its disposition filter")
    if unclassified_lanes.get("lanes"):
        raise AssertionError(
            "object-ddm-ownership unexpectedly exposes an unclassified lane"
        )

    # An exact lane query through ``roadmap`` should be a one-command bridge
    # to the owning family and the same lane/focus/trace handles.  This keeps
    # resume work bounded when a contributor remembers the taxonomy tag but
    # not its roadmap owner.
    roadmap_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="joined-federate-mom-deleted-object-count-periodic",
        status="open",
        limit=0,
    )
    if roadmap_lane.get("count") != 1 or not roadmap_lane.get("families"):
        raise AssertionError("exact lane roadmap search did not resolve one family")
    roadmap_lane_row = roadmap_lane["families"][0]
    if roadmap_lane_row.get("lane_match_count") != 1:
        raise AssertionError("exact lane roadmap search omitted its lane match")
    roadmap_lane_match = roadmap_lane_row.get("lane_matches", [None])[0]
    if not isinstance(roadmap_lane_match, dict):
        raise AssertionError("exact lane roadmap search returned no lane mapping")
    if (
        roadmap_lane_match.get("tag")
        != "joined-federate-mom-deleted-object-count-periodic"
    ):
        raise AssertionError("exact lane roadmap search returned the wrong lane")
    if roadmap_lane_match.get("mapped_count") != 1:
        raise AssertionError("exact lane roadmap search lost mapped case count")
    if not roadmap_lane_match.get("focus_command") or not roadmap_lane_match.get(
        "representative_trace_command"
    ):
        raise AssertionError("exact lane roadmap search omitted focus/trace handles")
    if not roadmap_lane_match.get("representative_case_command") or (
        "umbra-cpp-joined-federate-mom-deleted-object-count-periodic-integration"
        not in roadmap_lane_match.get("representative_case_command", "")
    ):
        raise AssertionError("exact lane roadmap search omitted its case handle")
    if roadmap_lane_match.get("representative_test") != (
        "Embedded joined-federate MOM exposes HLAobjectInstancesDeleted count"
    ):
        raise AssertionError(
            "exact lane roadmap search returned the wrong representative test"
        )
    if any(
        roadmap_lane_match.get(pointer) is not None
        for pointer in (
            "next_test",
            "next_test_id",
            "next_source",
            "next_case_command",
            "next_trace_command",
        )
    ):
        raise AssertionError(
            "completed lane roadmap search should expose no lane-specific next-test pointer"
        )

    # A newly added process lane also shares the broad process-boundary tag;
    # the explicit lane-owner join must still return one roadmap family.
    exception_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-mom-exception-report",
        status="open",
        limit=0,
    )
    if exception_lane.get("count") != 1 or not exception_lane.get("families"):
        raise AssertionError(
            "exact process MOM exception lane did not resolve one owning family"
        )
    exception_lane_row = exception_lane["families"][0]
    if exception_lane_row.get("id") != "transport-and-conformance":
        raise AssertionError(
            "exact process MOM exception lane resolved the wrong family"
        )
    exception_lane_matches = exception_lane_row.get("lane_matches") or []
    if len(exception_lane_matches) != 1 or exception_lane_matches[0].get("tag") != (
        "process-mom-exception-report"
    ):
        raise AssertionError(
            "exact process MOM exception lane omitted its focused mapping"
        )
    malformed_parameter_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-mom-malformed-parameter",
        status="open",
        limit=0,
    )
    if (
        malformed_parameter_lane.get("count") != 1
        or not malformed_parameter_lane.get("families")
        or malformed_parameter_lane["families"][0].get("id")
        != "transport-and-conformance"
    ):
        raise AssertionError(
            "exact process MOM malformed-parameter lane did not resolve one owning family"
        )
    malformed_parameter_matches = (
        malformed_parameter_lane["families"][0].get("lane_matches") or []
    )
    if (
        len(malformed_parameter_matches) != 1
        or malformed_parameter_matches[0].get("tag")
        != "process-mom-malformed-parameter"
    ):
        raise AssertionError(
            "exact process MOM malformed-parameter lane omitted its focused mapping"
        )
    malformed_value_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-mom-malformed-value",
        status="open",
        limit=0,
    )
    if (
        malformed_value_lane.get("count") != 1
        or not malformed_value_lane.get("families")
        or malformed_value_lane["families"][0].get("id") != "transport-and-conformance"
    ):
        raise AssertionError(
            "exact process MOM malformed-value lane did not resolve one owning family"
        )
    malformed_value_matches = (
        malformed_value_lane["families"][0].get("lane_matches") or []
    )
    if (
        len(malformed_value_matches) != 1
        or malformed_value_matches[0].get("tag") != "process-mom-malformed-value"
    ):
        raise AssertionError(
            "exact process MOM malformed-value lane omitted its focused mapping"
        )
    malformed_value_row = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-malformed-value-integration"
        ),
        None,
    )
    if not isinstance(malformed_value_row, dict) or (
        query_rti_work.source_location_text(malformed_value_row)
        != "cpp/tests/ieee1516_2025_connection_process_mom_malformed_value_catch2.cpp:459"
        or malformed_value_row.get("assertions") != 30
        or len(malformed_value_row.get("lab_requirement_ids", [])) != 3
        or len(malformed_value_row.get("standard_sections", [])) != 2
        or len(malformed_value_row.get("selected_cpp_api_surface_ids", [])) != 11
    ):
        raise AssertionError("malformed-value source/evidence mapping drifted")
    malformed_value_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-malformed-value", limit=0
    )
    if (
        malformed_value_focus.get("lane_state") != "complete"
        or malformed_value_focus.get("mapped_test_count") != 1
        or malformed_value_focus.get("assertion_count") != 30
        or malformed_value_focus.get("recorded_assertion_count") != 30
        or malformed_value_focus.get("requirement_count") != 3
        or malformed_value_focus.get("standard_section_count") != 2
        or malformed_value_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("malformed-value lane card drifted")
    malformed_value_handles = malformed_value_focus.get("lane_handles")
    if not isinstance(malformed_value_handles, dict) or (
        malformed_value_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or malformed_value_handles.get("ctest_label") != "process-mom-malformed-value"
        or "RTIambassador reports a malformed process HLAsetSwitches value"
        not in str(malformed_value_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("malformed-value CTest handle drifted")

    malformed_resign_lane = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-mom-malformed-resign-action",
        status="open",
        limit=0,
    )
    if (
        malformed_resign_lane.get("count") != 1
        or not malformed_resign_lane.get("families")
        or malformed_resign_lane["families"][0].get("id") != "transport-and-conformance"
    ):
        raise AssertionError(
            "exact process MOM malformed-resign-action lane did not resolve one owning family"
        )
    malformed_resign_matches = (
        malformed_resign_lane["families"][0].get("lane_matches") or []
    )
    if (
        len(malformed_resign_matches) != 1
        or malformed_resign_matches[0].get("tag")
        != "process-mom-malformed-resign-action"
    ):
        raise AssertionError(
            "exact process MOM malformed-resign-action lane omitted its focused mapping"
        )
    malformed_resign_row = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-mom-malformed-resign-action-integration"
        ),
        None,
    )
    if not isinstance(malformed_resign_row, dict) or (
        query_rti_work.source_location_text(malformed_resign_row)
        != "cpp/tests/ieee1516_2025_connection_process_mom_malformed_resign_action_catch2.cpp:459"
        or malformed_resign_row.get("assertions") != 30
        or len(malformed_resign_row.get("lab_requirement_ids", [])) != 3
        or len(malformed_resign_row.get("standard_sections", [])) != 2
        or len(malformed_resign_row.get("selected_cpp_api_surface_ids", [])) != 12
    ):
        raise AssertionError("malformed-resign-action source/evidence mapping drifted")
    malformed_resign_focus = query_rti_work.focused_lane_result(
        index, tests, "process-mom-malformed-resign-action", limit=0
    )
    if (
        malformed_resign_focus.get("lane_state") != "complete"
        or malformed_resign_focus.get("mapped_test_count") != 1
        or malformed_resign_focus.get("assertion_count") != 30
        or malformed_resign_focus.get("recorded_assertion_count") != 30
        or malformed_resign_focus.get("requirement_count") != 3
        or malformed_resign_focus.get("standard_section_count") != 2
        or malformed_resign_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("malformed-resign-action lane card drifted")
    malformed_resign_handles = malformed_resign_focus.get("lane_handles")
    if not isinstance(malformed_resign_handles, dict) or (
        malformed_resign_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or malformed_resign_handles.get("ctest_label")
        != "process-mom-malformed-resign-action"
        or "RTIambassador reports a malformed process HLAresignAction parameter"
        not in str(malformed_resign_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("malformed-resign-action CTest handle drifted")

    # Custom transportation delivery is a deliberate traceability boundary:
    # Eight implemented delivery forms have an explicit no-standalone-Lab
    # disposition, while three focused cases map initial regional reflection
    # and transportation controls. Keep that distinction queryable by one
    # exact taxonomy tag and keep the aggregate execution result separate from
    # the focused custom-transportation-type-control mappings.
    custom_rows = [
        test for test in tests if "custom-transportation" in test.get("tags", [])
    ]
    if len(custom_rows) != 11:
        raise AssertionError(
            "custom-transportation taxonomy row count drifted: "
            f"expected 11, observed {len(custom_rows)}"
        )
    explicit_custom = [
        test
        for test in custom_rows
        if test.get("traceability_state") == "explicit-disposition"
    ]
    unclassified_custom = [
        test for test in custom_rows if test.get("traceability_state") == "unclassified"
    ]
    mapped_custom = [
        test
        for test in custom_rows
        if test.get("traceability_state") == "requirements-mapped"
    ]
    if (
        len(explicit_custom) != 8
        or len(unclassified_custom) != 0
        or len(mapped_custom) != 3
    ):
        raise AssertionError(
            "custom-transportation disposition split drifted: "
            f"mapped={len(mapped_custom)}, explicit={len(explicit_custom)}, "
            f"unclassified={len(unclassified_custom)}"
        )
    custom_focus = query_rti_work.focused_lane_result(
        index, tests, "custom-transportation", limit=0
    )
    if custom_focus.get("lane_state") != "complete":
        raise AssertionError(
            "custom-transportation lane should be complete after aggregate repair"
        )
    if custom_focus.get("mapped_test_count") != 3:
        raise AssertionError("focus lost custom-transportation mapped count")
    if custom_focus.get("explicit_disposition_count") != 8:
        raise AssertionError(
            "focus lost custom-transportation explicit disposition count"
        )
    if custom_focus.get("unclassified_count") != 0:
        raise AssertionError("focus retained an unexpected custom-transportation gap")

    # The region lifecycle is the first newly isolated DDM target.  Keep its
    # exact taxonomy, requirement mapping, and standalone execution handle
    # protected so future aggregate edits cannot make this slice undiscoverable.
    region_rows = [test for test in tests if "region-lifecycle" in test.get("tags", [])]
    if len(region_rows) != 7:
        raise AssertionError(
            "region-lifecycle taxonomy row count drifted: "
            f"expected 7, observed {len(region_rows)}"
        )
    if any(
        test.get("traceability_state") != "requirements-mapped" for test in region_rows
    ):
        raise AssertionError("region-lifecycle retained an unmapped plan row")
    region_focus = query_rti_work.focused_lane_result(
        index, tests, "region-lifecycle", limit=0
    )
    if region_focus.get("lane_state") != "complete":
        raise AssertionError("region-lifecycle lane should be complete")
    if region_focus.get("mapped_test_count") != 7:
        raise AssertionError("focus lost region-lifecycle mapped count")
    if region_focus.get("unclassified_count") != 0:
        raise AssertionError("region-lifecycle retained an unexpected mapping gap")
    region_inventory = query_rti_work.lane_inventory(
        index,
        tests,
        family="object-ddm-ownership",
        limit=0,
    )
    region_lane = next(
        (
            row
            for row in region_inventory.get("lanes", [])
            if isinstance(row, dict) and row.get("tag") == "region-lifecycle"
        ),
        None,
    )
    if not isinstance(region_lane, dict):
        raise AssertionError("region-lifecycle lane was absent from family inventory")
    if region_lane.get("resolved_requirement_section_pair_count") != 38:
        raise AssertionError("region-lifecycle direct mapping count drifted")
    region_handles = region_focus.get("lane_handles")
    if (
        not isinstance(region_handles, dict)
        or region_handles.get("catch2_target") != "umbra_region_lifecycle_catch2"
    ):
        raise AssertionError("region-lifecycle lost its standalone C++ target handle")

    # Query Attribute Ownership now has standalone owner/unowned and MOM
    # interaction slices. Protect their exact titles, assertion counts,
    # mappings, and executable handles so navigation remains one bounded
    # command.
    ownership_query = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-query-standalone"
        ),
        None,
    )
    if not isinstance(ownership_query, dict):
        raise AssertionError("standalone Query Attribute Ownership row is absent")
    if ownership_query.get("assertions") != 48:
        raise AssertionError(
            "standalone Query Attribute Ownership assertion count drifted"
        )
    if ownership_query.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone Query Attribute Ownership row is not mapped")
    ownership_query_focus = query_rti_work.focused_lane_result(
        index, tests, "query-attribute-ownership", limit=0
    )
    if ownership_query_focus.get("lane_state") != "complete":
        raise AssertionError("query-attribute-ownership lane state drifted")
    if ownership_query_focus.get("mapped_test_count") != 19:
        raise AssertionError("query-attribute-ownership mapped count drifted")
    if ownership_query_focus.get("assertion_count") != 637:
        raise AssertionError("query-attribute-ownership assertion total drifted")
    if ownership_query_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "query-attribute-ownership retained an unexpected mapping gap"
        )
    ownership_query_handles = ownership_query_focus.get("lane_handles")
    if (
        not isinstance(ownership_query_handles, dict)
        or ownership_query_handles.get("catch2_target")
        != "umbra_attribute_ownership_query_catch2"
    ):
        raise AssertionError(
            "query-attribute-ownership lost its standalone C++ target handle"
        )

    if_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-if-available-standalone"
        ),
        None,
    )
    if not isinstance(if_available, dict):
        raise AssertionError("standalone If Available acquisition row is absent")
    if if_available.get("assertions") != 54:
        raise AssertionError(
            "standalone If Available acquisition assertion count drifted"
        )
    if if_available.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone If Available acquisition row is not mapped")
    if_available_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-ownership-acquisition-if-available", limit=0
    )
    if if_available_focus.get("lane_state") != "complete":
        raise AssertionError("If Available acquisition lane should be complete")
    if if_available_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "If Available acquisition retained an unexpected mapping gap"
        )
    if_available_handles = if_available_focus.get("lane_handles")
    if (
        not isinstance(if_available_handles, dict)
        or if_available_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_if_available_catch2"
    ):
        raise AssertionError(
            "If Available acquisition lost its standalone C++ target handle"
        )

    process_if_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-if-available-process-integration"
        ),
        None,
    )
    if not isinstance(process_if_available, dict):
        raise AssertionError("process If Available acquisition row is absent")
    if process_if_available.get("assertions") != 22:
        raise AssertionError("process If Available acquisition assertion count drifted")
    if process_if_available.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process If Available acquisition row is not mapped")
    process_if_available_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition-if-available", limit=0
    )
    if (
        process_if_available_focus.get("lane_state") != "complete"
        or process_if_available_focus.get("mapped_test_count") != 1
        or process_if_available_focus.get("assertion_count") != 22
        or process_if_available_focus.get("requirement_count") != 4
        or process_if_available_focus.get("standard_section_count") != 3
        or process_if_available_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("process If Available acquisition lane card drifted")
    process_if_available_handles = process_if_available_focus.get("lane_handles")
    if (
        not isinstance(process_if_available_handles, dict)
        or process_if_available_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_if_available_catch2"
    ):
        raise AssertionError(
            "process If Available acquisition lost its C++ target handle"
        )

    process_acquisition = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition, dict):
        raise AssertionError("process regular acquisition row is absent")
    if process_acquisition.get("assertions") != 22:
        raise AssertionError("process regular acquisition assertion count drifted")
    if process_acquisition.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process regular acquisition row is not mapped")
    process_acquisition_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition", limit=0
    )
    if (
        process_acquisition_focus.get("lane_state") != "complete"
        or process_acquisition_focus.get("mapped_test_count") != 1
        or process_acquisition_focus.get("assertion_count") != 22
        or process_acquisition_focus.get("requirement_count") != 2
        or process_acquisition_focus.get("standard_section_count") != 2
        or process_acquisition_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("process regular acquisition lane card drifted")
    process_acquisition_handles = process_acquisition_focus.get("lane_handles")
    if (
        not isinstance(process_acquisition_handles, dict)
        or process_acquisition_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError("process regular acquisition lost its C++ target handle")

    process_acquisition_release = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-release-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_release, dict):
        raise AssertionError("process owner-release acquisition row is absent")
    if process_acquisition_release.get("assertions") != 30:
        raise AssertionError(
            "process owner-release acquisition assertion count drifted"
        )
    if process_acquisition_release.get("traceability_state") != "requirements-mapped":
        raise AssertionError("process owner-release acquisition row is not mapped")
    process_acquisition_release_focus = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-acquisition-release", limit=0
    )
    if (
        process_acquisition_release_focus.get("lane_state") != "complete"
        or process_acquisition_release_focus.get("mapped_test_count") != 1
        or process_acquisition_release_focus.get("assertion_count") != 30
        or process_acquisition_release_focus.get("requirement_count") != 3
        or process_acquisition_release_focus.get("standard_section_count") != 2
        or process_acquisition_release_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("process owner-release acquisition lane card drifted")
    process_acquisition_release_handles = process_acquisition_release_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_acquisition_release_handles, dict)
        or process_acquisition_release_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError(
            "process owner-release acquisition lost its C++ target handle"
        )

    process_acquisition_release_denied = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-release-denied-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_release_denied, dict):
        raise AssertionError("process owner-release-denied row is absent")
    if process_acquisition_release_denied.get("assertions") != 43:
        raise AssertionError("process owner-release-denied assertion count drifted")
    if (
        process_acquisition_release_denied.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process owner-release-denied row is not mapped")
    process_acquisition_release_denied_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-ownership-acquisition-release-denied",
        limit=0,
    )
    if (
        process_acquisition_release_denied_focus.get("lane_state") != "complete"
        or process_acquisition_release_denied_focus.get("mapped_test_count") != 1
        or process_acquisition_release_denied_focus.get("assertion_count") != 43
        or process_acquisition_release_denied_focus.get("requirement_count") != 5
        or process_acquisition_release_denied_focus.get("standard_section_count") != 4
        or process_acquisition_release_denied_focus.get(
            "requirement_section_pair_count"
        )
        != 5
    ):
        raise AssertionError("process owner-release-denied lane card drifted")
    process_acquisition_release_denied_handles = (
        process_acquisition_release_denied_focus.get("lane_handles")
    )
    if (
        not isinstance(process_acquisition_release_denied_handles, dict)
        or process_acquisition_release_denied_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError("process owner-release-denied lost its C++ target handle")

    process_acquisition_cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-acquisition-cancellation-process-integration"
        ),
        None,
    )
    if not isinstance(process_acquisition_cancellation, dict):
        raise AssertionError("process ownership-cancellation row is absent")
    if (
        query_rti_work.source_location_text(process_acquisition_cancellation)
        != ("cpp/tests/attribute_ownership_acquisition_catch2.cpp:1136")
        or process_acquisition_cancellation.get("assertions") != 32
    ):
        raise AssertionError("process ownership-cancellation source/evidence drifted")
    if (
        process_acquisition_cancellation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("process ownership-cancellation row is not mapped")
    process_acquisition_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-ownership-acquisition-cancellation",
        limit=0,
    )
    if (
        process_acquisition_cancellation_focus.get("lane_state") != "complete"
        or process_acquisition_cancellation_focus.get("mapped_test_count") != 1
        or process_acquisition_cancellation_focus.get("assertion_count") != 32
        or process_acquisition_cancellation_focus.get("requirement_count") != 3
        or process_acquisition_cancellation_focus.get("standard_section_count") != 3
        or process_acquisition_cancellation_focus.get("requirement_section_pair_count")
        != 3
    ):
        raise AssertionError("process ownership-cancellation lane card drifted")
    process_acquisition_cancellation_handles = (
        process_acquisition_cancellation_focus.get("lane_handles")
    )
    if (
        not isinstance(process_acquisition_cancellation_handles, dict)
        or process_acquisition_cancellation_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError(
            "process ownership-cancellation lost its C++ target handle"
        )
    if (
        process_acquisition_cancellation_handles.get("ctest_label")
        != "process-boundary"
    ):
        raise AssertionError("process ownership-cancellation CTest label drifted")

    process_negotiated_cancellation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-negotiated-divestiture-cancellation-process-integration"
        ),
        None,
    )
    if not isinstance(process_negotiated_cancellation, dict):
        raise AssertionError(
            "process negotiated-divestiture cancellation row is absent"
        )
    if (
        query_rti_work.source_location_text(process_negotiated_cancellation)
        != (
            "cpp/tests/ieee1516_2025_negotiated_divestiture_cancellation_process_catch2.cpp:64"
        )
        or process_negotiated_cancellation.get("assertions") != 38
    ):
        raise AssertionError(
            "process negotiated-divestiture cancellation source/evidence drifted"
        )
    if (
        process_negotiated_cancellation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "process negotiated-divestiture cancellation row is not mapped"
        )
    process_negotiated_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-negotiated-divestiture-cancellation",
        limit=0,
    )
    if (
        process_negotiated_cancellation_focus.get("lane_state") != "complete"
        or process_negotiated_cancellation_focus.get("mapped_test_count") != 1
        or process_negotiated_cancellation_focus.get("assertion_count") != 38
        or process_negotiated_cancellation_focus.get("requirement_count") != 5
        or process_negotiated_cancellation_focus.get("standard_section_count") != 4
        or process_negotiated_cancellation_focus.get("requirement_section_pair_count")
        != 5
    ):
        raise AssertionError(
            "process negotiated-divestiture cancellation lane card drifted"
        )
    process_negotiated_cancellation_handles = process_negotiated_cancellation_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_negotiated_cancellation_handles, dict)
        or process_negotiated_cancellation_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError(
            "process negotiated-divestiture cancellation lost its C++ target handle"
        )
    if process_negotiated_cancellation_handles.get("ctest_label") != "process-boundary":
        raise AssertionError(
            "process negotiated-divestiture cancellation CTest label drifted"
        )

    regular_acquisition = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-attribute-ownership-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(regular_acquisition, dict):
        raise AssertionError("standalone regular acquisition row is absent")
    if regular_acquisition.get("assertions") != 74:
        raise AssertionError("standalone regular acquisition assertion count drifted")
    if regular_acquisition.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone regular acquisition row is not mapped")
    regular_acquisition_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-ownership-acquisition", limit=0
    )
    if regular_acquisition_focus.get("lane_state") != "complete":
        raise AssertionError("regular acquisition lane should be complete")
    if regular_acquisition_focus.get("unclassified_count") != 0:
        raise AssertionError("regular acquisition retained an unexpected mapping gap")
    regular_acquisition_handles = regular_acquisition_focus.get("lane_handles")
    if (
        not isinstance(regular_acquisition_handles, dict)
        or regular_acquisition_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
    ):
        raise AssertionError(
            "regular acquisition lost its standalone C++ target handle"
        )

    # Negotiated divestiture now has an explicit §7.2 assumption-callback
    # lane. Protect the one-test reverse map, owning family, assertion total,
    # and focused CTest handle so this narrow slice remains directly
    # queryable as the aggregate ownership tests evolve.
    negotiated_assumption = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-negotiated-assumption-integration"
        ),
        None,
    )
    if not isinstance(negotiated_assumption, dict):
        raise AssertionError("negotiated-assumption row is absent")
    if negotiated_assumption.get("assertions") != 30:
        raise AssertionError("negotiated-assumption assertion count drifted")
    if negotiated_assumption.get("traceability_state") != "requirements-mapped":
        raise AssertionError("negotiated-assumption row is not mapped")
    if negotiated_assumption.get("standard_sections") != ["hla-1516.1-2025:clause-7.2"]:
        raise AssertionError("negotiated-assumption section mapping drifted")
    negotiated_assumption_focus = query_rti_work.focused_lane_result(
        index, tests, "negotiated-assumption", limit=0
    )
    if negotiated_assumption_focus.get("lane_state") != "complete":
        raise AssertionError("negotiated-assumption lane should be complete")
    if negotiated_assumption_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError("negotiated-assumption lane lost its roadmap owner")
    if negotiated_assumption_focus.get("mapped_test_count") != 1:
        raise AssertionError("negotiated-assumption mapped count drifted")
    if negotiated_assumption_focus.get("assertion_count") != 30:
        raise AssertionError("negotiated-assumption lane assertion total drifted")
    negotiated_assumption_handles = negotiated_assumption_focus.get("lane_handles")
    if (
        not isinstance(negotiated_assumption_handles, dict)
        or negotiated_assumption_handles.get("catch2_target")
        != "umbra_negotiated_willing_to_acquire_candidate_catch2"
    ):
        raise AssertionError("negotiated-assumption lost its focused C++ target handle")
    if query_rti_work.lane_ctest_command(negotiated_assumption_handles) != (
        'ctest --test-dir <build-dir> -C Debug -R "^umbra\\.negotiated_willing_to_acquire_candidate\\.catch2\\.Embedded Negotiated Attribute Ownership Divestiture forwards its tag to an assumption candidate$" '
        "--output-on-failure"
    ):
        raise AssertionError("negotiated-assumption lost its focused CTest command")

    regional_region_context = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-region-context-integration"
        ),
        None,
    )
    if not isinstance(regional_region_context, dict):
        raise AssertionError("regional object region-context row is absent")
    if regional_region_context.get("assertions") != 34:
        raise AssertionError("regional object region-context assertion count drifted")
    if len(regional_region_context.get("lab_requirement_ids", [])) != 5:
        raise AssertionError("regional object region-context requirement count drifted")
    if regional_region_context.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional object region-context row is not mapped")
    if regional_region_context.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.2",
    ]:
        raise AssertionError("regional object region-context section mapping drifted")
    regional_region_context_focus = query_rti_work.focused_lane_result(
        index, tests, "regional-object-attribute-region-context", limit=0
    )
    if regional_region_context_focus.get("lane_state") != "complete":
        raise AssertionError("regional object region-context lane should be complete")
    if regional_region_context_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError(
            "regional object region-context lane lost its roadmap owner"
        )
    if regional_region_context_focus.get("mapped_test_count") != 1:
        raise AssertionError("regional object region-context mapped count drifted")
    if regional_region_context_focus.get("assertion_count") != 34:
        raise AssertionError("regional object region-context assertion total drifted")
    regional_region_context_handles = regional_region_context_focus.get("lane_handles")
    if (
        not isinstance(regional_region_context_handles, dict)
        or regional_region_context_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
    ):
        raise AssertionError(
            "regional object region-context lost its focused C++ target handle"
        )
    if query_rti_work.lane_ctest_command(regional_region_context_handles) != (
        'ctest --test-dir <build-dir> -C Debug -R "^umbra\\.regional_object_attribute_routing\\.catch2\\.Embedded regional object services reject region dimensions outside available object dimensions$" '
        "--output-on-failure"
    ):
        raise AssertionError(
            "regional object region-context lost its focused CTest command"
        )

    unconditional_divestiture = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-unconditional-attribute-ownership-divestiture-standalone"
        ),
        None,
    )
    if not isinstance(unconditional_divestiture, dict):
        raise AssertionError("standalone unconditional-divestiture row is absent")
    if unconditional_divestiture.get("assertions") != 116:
        raise AssertionError(
            "standalone unconditional-divestiture assertion count drifted"
        )
    if unconditional_divestiture.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone unconditional-divestiture row is not mapped")
    unconditional_focus = query_rti_work.focused_lane_result(
        index, tests, "unconditional-attribute-ownership-divestiture", limit=0
    )
    if unconditional_focus.get("lane_state") != "complete":
        raise AssertionError("unconditional-divestiture lane should be complete")
    if unconditional_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "unconditional-divestiture retained an unexpected mapping gap"
        )
    unconditional_handles = unconditional_focus.get("lane_handles")
    if (
        not isinstance(unconditional_handles, dict)
        or unconditional_handles.get("catch2_target")
        != "umbra_unconditional_attribute_ownership_divestiture_catch2"
    ):
        raise AssertionError(
            "unconditional-divestiture lost its standalone C++ target handle"
        )

    resign_action = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-divestiture-standalone"
        ),
        None,
    )
    if not isinstance(resign_action, dict):
        raise AssertionError("standalone resign-action divestiture row is absent")
    if resign_action.get("assertions") != 31:
        raise AssertionError(
            "standalone resign-action divestiture assertion count drifted"
        )
    if resign_action.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign-action divestiture row is not mapped")
    resign_action_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-unconditional-divestiture", limit=0
    )
    if resign_action_focus.get("lane_state") != "complete":
        raise AssertionError("resign-action divestiture lane should be complete")
    if resign_action_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "resign-action divestiture retained an unexpected mapping gap"
        )
    resign_action_handles = resign_action_focus.get("lane_handles")
    if (
        not isinstance(resign_action_handles, dict)
        or resign_action_handles.get("catch2_target")
        != "umbra_resign_action_unconditional_divestiture_catch2"
    ):
        raise AssertionError(
            "resign-action divestiture lost its standalone C++ target handle"
        )

    connection_loss_unconditional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-unconditional-divest-standalone"
        ),
        None,
    )
    if not isinstance(connection_loss_unconditional, dict):
        raise AssertionError(
            "standalone Connection Lost automatic-divestiture row is absent"
        )
    if connection_loss_unconditional.get("assertions") != 38:
        raise AssertionError(
            "standalone Connection Lost automatic-divestiture assertion count drifted"
        )
    if connection_loss_unconditional.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "standalone Connection Lost automatic-divestiture row is not mapped"
        )
    connection_loss_focus = query_rti_work.focused_lane_result(
        index, tests, "connection-lost-automatic-unconditional-divestiture", limit=0
    )
    if connection_loss_focus.get("lane_state") != "complete":
        raise AssertionError(
            "Connection Lost automatic-divestiture lane should be complete"
        )
    if connection_loss_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "Connection Lost automatic-divestiture retained an unexpected mapping gap"
        )
    connection_loss_handles = connection_loss_focus.get("lane_handles")
    if (
        not isinstance(connection_loss_handles, dict)
        or connection_loss_handles.get("catch2_target")
        != "umbra_connection_loss_automatic_unconditional_divestiture_catch2"
    ):
        raise AssertionError(
            "Connection Lost automatic-divestiture lost its standalone C++ target handle"
        )

    connection_loss_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-unconditional-divest-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_immediate, dict):
        raise AssertionError("HLA_IMMEDIATE Connection Lost companion row is absent")
    if connection_loss_immediate.get("assertions") != 33:
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost companion assertion count drifted"
        )
    if connection_loss_immediate.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost companion row is not mapped"
        )
    connection_loss_immediate_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-unconditional-divestiture-immediate",
        limit=0,
    )
    if connection_loss_immediate_focus.get("lane_state") != "complete":
        raise AssertionError("HLA_IMMEDIATE Connection Lost lane should be complete")
    if connection_loss_immediate_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost lane retained an unexpected mapping gap"
        )
    connection_loss_immediate_handles = connection_loss_immediate_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(connection_loss_immediate_handles, dict)
        or connection_loss_immediate_handles.get("catch2_target")
        != "umbra_connection_loss_automatic_unconditional_divestiture_catch2"
    ):
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost lane lost its standalone C++ target handle"
        )

    connection_loss_cancel = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(connection_loss_cancel, dict):
        raise AssertionError("standalone Connection Lost cancellation row is absent")
    if connection_loss_cancel.get("assertions") != 62:
        raise AssertionError(
            "standalone Connection Lost cancellation assertion count drifted"
        )
    if connection_loss_cancel.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "standalone Connection Lost cancellation row is not mapped"
        )
    connection_loss_cancel_focus = query_rti_work.focused_lane_result(
        index, tests, "connection-lost-automatic-cancel-pending-acquisition", limit=0
    )
    if connection_loss_cancel_focus.get("lane_state") != "complete":
        raise AssertionError("Connection Lost cancellation lane should be complete")
    if connection_loss_cancel_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "Connection Lost cancellation retained an unexpected mapping gap"
        )
    connection_loss_cancel_handles = connection_loss_cancel_focus.get("lane_handles")
    if (
        not isinstance(connection_loss_cancel_handles, dict)
        or connection_loss_cancel_handles.get("catch2_target")
        != "umbra_connection_loss_automatic_cancel_pending_acquisition_catch2"
    ):
        raise AssertionError(
            "Connection Lost cancellation lost its standalone C++ target handle"
        )

    connection_loss_cancel_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-cancel-pending-acquisition-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_cancel_immediate, dict):
        raise AssertionError("HLA_IMMEDIATE Connection Lost cancellation row is absent")
    if connection_loss_cancel_immediate.get("assertions") != 41:
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost cancellation assertion count drifted"
        )
    if (
        connection_loss_cancel_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost cancellation row is not mapped"
        )
    connection_loss_cancel_immediate_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-cancel-pending-acquisition-immediate",
        limit=0,
    )
    if (
        connection_loss_cancel_immediate_focus.get("lane_state") != "complete"
        or connection_loss_cancel_immediate_focus.get("mapped_test_count") != 1
        or connection_loss_cancel_immediate_focus.get("assertion_count") != 41
        or connection_loss_cancel_immediate_focus.get("requirement_count") != 5
        or connection_loss_cancel_immediate_focus.get("standard_section_count") != 5
        or connection_loss_cancel_immediate_focus.get("requirement_section_pair_count")
        != 5
        or connection_loss_cancel_immediate_focus.get("unclassified_count") != 0
    ):
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost cancellation lane card drifted"
        )
    connection_loss_cancel_immediate_handles = (
        connection_loss_cancel_immediate_focus.get("lane_handles")
    )
    if (
        not isinstance(connection_loss_cancel_immediate_handles, dict)
        or connection_loss_cancel_immediate_handles.get("catch2_target")
        != "umbra_connection_loss_automatic_cancel_pending_acquisition_catch2"
        or "Immediate callbacks cancel a lost federate's pending ownership acquisition synchronously"
        not in str(connection_loss_cancel_immediate_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "HLA_IMMEDIATE Connection Lost cancellation CTest handle drifted"
        )

    connection_loss_no_action_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-no-action-policy-immediate-integration"
        ),
        None,
    )
    if not isinstance(connection_loss_no_action_immediate, dict):
        raise AssertionError("HLA_IMMEDIATE NoAction Connection Lost row is absent")
    if connection_loss_no_action_immediate.get("assertions") != 31:
        raise AssertionError(
            "HLA_IMMEDIATE NoAction Connection Lost assertion count drifted"
        )
    if (
        connection_loss_no_action_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("HLA_IMMEDIATE NoAction Connection Lost row is not mapped")
    connection_loss_no_action_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-no-action-immediate",
        limit=0,
    )
    if (
        connection_loss_no_action_focus.get("lane_state") != "complete"
        or connection_loss_no_action_focus.get("mapped_test_count") != 1
        or connection_loss_no_action_focus.get("assertion_count") != 31
        or connection_loss_no_action_focus.get("requirement_count") != 5
        or connection_loss_no_action_focus.get("standard_section_count") != 5
        or connection_loss_no_action_focus.get("requirement_section_pair_count") != 5
        or connection_loss_no_action_focus.get("unclassified_count") != 0
    ):
        raise AssertionError("HLA_IMMEDIATE NoAction Connection Lost lane card drifted")
    connection_loss_no_action_handles = connection_loss_no_action_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(connection_loss_no_action_handles, dict)
        or connection_loss_no_action_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Immediate callbacks apply the bounded automatic NoAction forced-resign policy synchronously"
        not in str(connection_loss_no_action_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "HLA_IMMEDIATE NoAction Connection Lost CTest handle drifted"
        )

    connection_loss_final_immediate = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-connection-lost-final-federate-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_final_immediate, dict):
        raise AssertionError(
            "HLA_IMMEDIATE final-federate Connection Lost row is absent"
        )
    if connection_loss_final_immediate.get("assertions") != 23:
        raise AssertionError(
            "HLA_IMMEDIATE final-federate Connection Lost assertion count drifted"
        )
    if (
        connection_loss_final_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "HLA_IMMEDIATE final-federate Connection Lost row is not mapped"
        )
    connection_loss_final_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-final-federate-immediate",
        limit=0,
    )
    if (
        connection_loss_final_focus.get("lane_state") != "complete"
        or connection_loss_final_focus.get("mapped_test_count") != 1
        or connection_loss_final_focus.get("assertion_count") != 23
        or connection_loss_final_focus.get("requirement_count") != 6
        or connection_loss_final_focus.get("standard_section_count") != 6
        or connection_loss_final_focus.get("requirement_section_pair_count") != 6
        or connection_loss_final_focus.get("unclassified_count") != 0
    ):
        raise AssertionError(
            "HLA_IMMEDIATE final-federate Connection Lost lane card drifted"
        )
    connection_loss_final_handles = connection_loss_final_focus.get("lane_handles")
    if (
        not isinstance(connection_loss_final_handles, dict)
        or connection_loss_final_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Immediate callbacks apply the final-federate forced directive-two rule synchronously"
        not in str(connection_loss_final_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "HLA_IMMEDIATE final-federate Connection Lost CTest handle drifted"
        )

    connection_loss_delete_divest_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-delete-then-divest-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_delete_divest_immediate, dict):
        raise AssertionError(
            "HLA_IMMEDIATE delete-then-divest Connection Lost row is absent"
        )
    if connection_loss_delete_divest_immediate.get("assertions") != 39:
        raise AssertionError(
            "HLA_IMMEDIATE delete-then-divest Connection Lost assertion count drifted"
        )
    if (
        connection_loss_delete_divest_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-then-divest Connection Lost row is not mapped"
        )
    connection_loss_delete_divest_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-delete-then-divest-immediate",
        limit=0,
    )
    if (
        connection_loss_delete_divest_focus.get("lane_state") != "complete"
        or connection_loss_delete_divest_focus.get("mapped_test_count") != 1
        or connection_loss_delete_divest_focus.get("assertion_count") != 39
        or connection_loss_delete_divest_focus.get("requirement_count") != 5
        or connection_loss_delete_divest_focus.get("standard_section_count") != 5
        or connection_loss_delete_divest_focus.get("requirement_section_pair_count")
        != 5
        or connection_loss_delete_divest_focus.get("unclassified_count") != 0
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-then-divest Connection Lost lane card drifted"
        )
    connection_loss_delete_divest_handles = connection_loss_delete_divest_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(connection_loss_delete_divest_handles, dict)
        or connection_loss_delete_divest_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Immediate callbacks apply the configured automatic delete-then-divest directive synchronously"
        not in str(connection_loss_delete_divest_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-then-divest Connection Lost CTest handle drifted"
        )

    connection_loss_delete_objects_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-delete-objects-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_delete_objects_immediate, dict):
        raise AssertionError(
            "HLA_IMMEDIATE delete-objects Connection Lost row is absent"
        )
    if connection_loss_delete_objects_immediate.get("assertions") != 25:
        raise AssertionError(
            "HLA_IMMEDIATE delete-objects Connection Lost assertion count drifted"
        )
    if (
        connection_loss_delete_objects_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-objects Connection Lost row is not mapped"
        )
    connection_loss_delete_objects_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-delete-objects-immediate",
        limit=0,
    )
    if (
        connection_loss_delete_objects_focus.get("lane_state") != "complete"
        or connection_loss_delete_objects_focus.get("mapped_test_count") != 1
        or connection_loss_delete_objects_focus.get("assertion_count") != 25
        or connection_loss_delete_objects_focus.get("requirement_count") != 5
        or connection_loss_delete_objects_focus.get("standard_section_count") != 5
        or connection_loss_delete_objects_focus.get("requirement_section_pair_count")
        != 5
        or connection_loss_delete_objects_focus.get("unclassified_count") != 0
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-objects Connection Lost lane card drifted"
        )
    connection_loss_delete_objects_handles = connection_loss_delete_objects_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(connection_loss_delete_objects_handles, dict)
        or connection_loss_delete_objects_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Immediate callbacks apply the configured automatic delete-objects directive synchronously"
        not in str(connection_loss_delete_objects_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "HLA_IMMEDIATE delete-objects Connection Lost CTest handle drifted"
        )

    connection_loss_cancel_delete_divest_immediate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-connection-lost-automatic-cancel-delete-divest-immediate"
        ),
        None,
    )
    if not isinstance(connection_loss_cancel_delete_divest_immediate, dict):
        raise AssertionError("mixed directive-5 Connection Lost row is absent")
    if connection_loss_cancel_delete_divest_immediate.get("assertions") != 45:
        raise AssertionError(
            "mixed directive-5 Connection Lost assertion count drifted"
        )
    if (
        connection_loss_cancel_delete_divest_immediate.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("mixed directive-5 Connection Lost row is not mapped")
    connection_loss_cancel_delete_divest_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "connection-lost-automatic-cancel-delete-divest-immediate",
        limit=0,
    )
    if (
        connection_loss_cancel_delete_divest_focus.get("lane_state") != "complete"
        or connection_loss_cancel_delete_divest_focus.get("mapped_test_count") != 1
        or connection_loss_cancel_delete_divest_focus.get("assertion_count") != 45
        or connection_loss_cancel_delete_divest_focus.get("requirement_count") != 7
        or connection_loss_cancel_delete_divest_focus.get("standard_section_count") != 7
        or connection_loss_cancel_delete_divest_focus.get(
            "requirement_section_pair_count"
        )
        != 7
        or connection_loss_cancel_delete_divest_focus.get("unclassified_count") != 0
    ):
        raise AssertionError("mixed directive-5 Connection Lost lane card drifted")
    connection_loss_cancel_delete_divest_handles = (
        connection_loss_cancel_delete_divest_focus.get("lane_handles")
    )
    if (
        not isinstance(connection_loss_cancel_delete_divest_handles, dict)
        or connection_loss_cancel_delete_divest_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
        or "Immediate callbacks apply the configured automatic cancel-then-delete-then-divest directive synchronously"
        not in str(
            connection_loss_cancel_delete_divest_handles.get("ctest_filter") or ""
        )
    ):
        raise AssertionError("mixed directive-5 Connection Lost CTest handle drifted")

    resign_pending = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-resign-action-pending-acquisition-standalone"
        ),
        None,
    )
    if not isinstance(resign_pending, dict):
        raise AssertionError("standalone resign pending-acquisition row is absent")
    if resign_pending.get("assertions") != 23:
        raise AssertionError(
            "standalone resign pending-acquisition assertion count drifted"
        )
    if resign_pending.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign pending-acquisition row is not mapped")
    resign_pending_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-pending-acquisition-rejection", limit=0
    )
    if resign_pending_focus.get("lane_state") != "complete":
        raise AssertionError("resign pending-acquisition lane should be complete")
    if resign_pending_focus.get("unclassified_count") != 0:
        raise AssertionError(
            "resign pending-acquisition retained an unexpected mapping gap"
        )
    resign_pending_handles = resign_pending_focus.get("lane_handles")
    if (
        not isinstance(resign_pending_handles, dict)
        or resign_pending_handles.get("catch2_target")
        != "umbra_resign_action_pending_acquisition_rejection_catch2"
    ):
        raise AssertionError(
            "resign pending-acquisition lost its standalone C++ target handle"
        )

    # The three directive-3 resignation forms share the same public action but
    # deliberately remain separate lanes. Keep each exact source row,
    # assertion total, mapping state, and executable handle protected so the
    # next ownership task is discoverable without reopening the aggregate file.
    for row_id, lane, assertion_count, target_name in (
        (
            "umbra-cpp-resign-action-cancel-pending-standalone",
            "resign-action-cancel-pending-acquisition",
            24,
            "umbra_resign_action_cancel_pending_acquisition_catch2",
        ),
        (
            "umbra-cpp-resign-action-cancel-negotiated-standalone",
            "resign-action-cancel-negotiated-pending",
            25,
            "umbra_resign_action_cancel_negotiated_pending_catch2",
        ),
        (
            "umbra-cpp-resign-action-cancel-if-available-standalone",
            "resign-action-cancel-if-available-pending",
            22,
            "umbra_resign_action_cancel_if_available_pending_catch2",
        ),
    ):
        cancellation_row = next(
            (test for test in tests if test.get("id") == row_id),
            None,
        )
        if not isinstance(cancellation_row, dict):
            raise AssertionError(f"standalone resignation row is absent: {row_id}")
        if cancellation_row.get("assertions") != assertion_count:
            raise AssertionError(f"resignation assertion count drifted: {lane}")
        if cancellation_row.get("traceability_state") != "requirements-mapped":
            raise AssertionError(f"resignation row is not mapped: {lane}")
        cancellation_focus = query_rti_work.focused_lane_result(
            index, tests, lane, limit=0
        )
        if cancellation_focus.get("lane_state") != "complete":
            raise AssertionError(f"resignation lane should be complete: {lane}")
        if cancellation_focus.get("unclassified_count") != 0:
            raise AssertionError(f"resignation lane retained an unmapped case: {lane}")
        cancellation_handles = cancellation_focus.get("lane_handles")
        if (
            not isinstance(cancellation_handles, dict)
            or cancellation_handles.get("catch2_target") != target_name
        ):
            raise AssertionError(f"resignation lane lost its C++ target handle: {lane}")

    # Directive 2 is a separate resignation lane from the directive-3
    # cancellation forms above. Keep its delete-privileged object behavior
    # independently indexed so queue selection never has to rediscover the
    # aggregate source case.
    delete_objects_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-delete-objects-standalone"
        ),
        None,
    )
    if not isinstance(delete_objects_row, dict):
        raise AssertionError("standalone resign delete-objects row is absent")
    if delete_objects_row.get("assertions") != 25:
        raise AssertionError("standalone resign delete-objects assertion count drifted")
    if delete_objects_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone resign delete-objects row is not mapped")
    delete_objects_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-delete-objects", limit=0
    )
    if delete_objects_focus.get("lane_state") != "complete":
        raise AssertionError("resign delete-objects lane should be complete")
    if delete_objects_focus.get("unclassified_count") != 0:
        raise AssertionError("resign delete-objects retained an unmapped case")
    delete_objects_handles = delete_objects_focus.get("lane_handles")
    if (
        not isinstance(delete_objects_handles, dict)
        or delete_objects_handles.get("catch2_target")
        != "umbra_resign_action_delete_objects_catch2"
    ):
        raise AssertionError("resign delete-objects lane lost its C++ target handle")

    final_federate_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-resign-action-final-federate-standalone"
        ),
        None,
    )
    if not isinstance(final_federate_row, dict):
        raise AssertionError("standalone final-federate resignation row is absent")
    if final_federate_row.get("assertions") != 22:
        raise AssertionError("standalone final-federate assertion count drifted")
    if final_federate_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone final-federate row is not mapped")
    final_federate_focus = query_rti_work.focused_lane_result(
        index, tests, "resign-action-final-federate", limit=0
    )
    if final_federate_focus.get("lane_state") != "complete":
        raise AssertionError("resign final-federate lane should be complete")
    if final_federate_focus.get("unclassified_count") != 0:
        raise AssertionError("resign final-federate retained an unmapped case")
    final_federate_handles = final_federate_focus.get("lane_handles")
    if (
        not isinstance(final_federate_handles, dict)
        or final_federate_handles.get("catch2_target")
        != "umbra_resign_action_final_federate_catch2"
    ):
        raise AssertionError("resign final-federate lane lost its C++ target handle")

    update_rate_row = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-update-rate-value-standalone"
        ),
        None,
    )
    if not isinstance(update_rate_row, dict):
        raise AssertionError("standalone update-rate value row is absent")
    if update_rate_row.get("assertions") != 40:
        raise AssertionError("standalone update-rate value assertion count drifted")
    if update_rate_row.get("traceability_state") != "requirements-mapped":
        raise AssertionError("standalone update-rate value row is not mapped")
    update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-value", limit=0
    )
    if update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("update-rate value lane should be complete")
    if update_rate_focus.get("unclassified_count") != 0:
        raise AssertionError("update-rate value retained an unmapped case")
    update_rate_handles = update_rate_focus.get("lane_handles")
    if (
        not isinstance(update_rate_handles, dict)
        or update_rate_handles.get("catch2_target") != "umbra_update_rate_value_catch2"
    ):
        raise AssertionError("update-rate value lane lost its C++ target handle")

    ddm_focus = query_rti_work.focused_lane_result(index, tests, "ddm", limit=0)
    # ``ddm`` is an intentionally broad taxonomy view. Its cross-cutting
    # restore evidence is now source-located and implemented, so it must not
    # be reported as disabled source drift.
    if ddm_focus.get("lane_state") != "complete":
        raise AssertionError("ddm lane state drifted")
    if ddm_focus.get("source_drift_count") != 0:
        raise AssertionError("ddm source-drift count drifted")
    if ddm_focus.get("planned_count") != 0:
        raise AssertionError("ddm planned-row count drifted")
    if ddm_focus.get("actionable_source_drift_count") != 0:
        raise AssertionError("ddm actionable source-drift count drifted")
    if ddm_focus.get("disabled_artifact_ids") != []:
        raise AssertionError("ddm retains a disabled historical artifact")
    if ddm_focus.get("unclassified_count") != 0:
        raise AssertionError("ddm retained an unexpected unclassified case")

    if not query_rti_work.select_tests(tests, "PROCESS-BOUNDARY", "lane"):
        raise AssertionError("lane lookup should accept case-insensitive exact tags")
    uppercase_focus = query_rti_work.focused_lane_result(
        index, tests, "PROCESS-BOUNDARY", limit=1
    )
    if uppercase_focus.get("lane_state") != "complete":
        raise AssertionError(
            "case-insensitive focused lookup lost the lane's completed state"
        )
    if not uppercase_focus.get("lane_handles"):
        raise AssertionError(
            "case-insensitive focused lane lost its configured handles"
        )

    directed_focus = query_rti_work.focused_lane_result(
        index, tests, "directed", limit=0
    )
    if directed_focus.get("test_count") != 50:
        raise AssertionError("directed lane plan count drifted")
    if directed_focus.get("mapped_test_count") != 47:
        raise AssertionError("directed lane mapped count drifted")
    directed_handles = directed_focus.get("lane_handles")
    if (
        not isinstance(directed_handles, dict)
        or directed_handles.get("ctest_label") != "directed"
    ):
        raise AssertionError("directed lane lost its cross-target CTest label")
    if query_rti_work.lane_ctest_command(directed_handles) != (
        'ctest --test-dir <build-dir> -C Debug -L "^directed$" --output-on-failure'
    ):
        raise AssertionError("label-backed directed lane lost its CTest command")

    regional_focus = query_rti_work.focused_lane_result(
        index, tests, "regional-object-attribute-routing", limit=1
    )
    if regional_focus.get("roadmap_owner") != "object-ddm-ownership":
        raise AssertionError("regional object-attribute lane lost its roadmap owner")
    regional_routing = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-routing-integration"
        ),
        None,
    )
    if not isinstance(regional_routing, dict):
        raise AssertionError("regional object-attribute routing row is absent")
    if regional_routing.get("assertions") != 61:
        raise AssertionError(
            "regional object-attribute routing assertion count drifted"
        )
    if len(regional_routing.get("lab_requirement_ids", [])) != 23:
        raise AssertionError(
            "regional object-attribute routing requirement count drifted"
        )
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l113-36"
        not in regional_routing.get("lab_requirement_ids", [])
    ):
        raise AssertionError(
            "regional object-attribute routing lost the §9.1.3.2 relationship anchor"
        )
    if regional_routing.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional object-attribute routing row is not mapped")
    if regional_routing.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.2",
        "hla-1516.1-2025:clause-9.1.6",
        "hla-1516.1-2025:clause-9.5",
        "hla-1516.1-2025:clause-9.6",
        "hla-1516.1-2025:clause-9.7.5",
        "hla-1516.1-2025:clause-9.8",
        "hla-1516.1-2025:clause-9.9.3",
    ]:
        raise AssertionError(
            "regional object-attribute routing section mapping drifted"
        )
    if regional_focus.get("lane_state") != "complete":
        raise AssertionError("regional object-attribute lane should be complete")
    if regional_focus.get("mapped_test_count") != 1:
        raise AssertionError("regional object-attribute mapped count drifted")
    if regional_focus.get("assertion_count") != 61:
        raise AssertionError("regional object-attribute lane assertion total drifted")
    regional_handles = regional_focus.get("lane_handles")
    if (
        not isinstance(regional_handles, dict)
        or regional_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
    ):
        raise AssertionError(
            "regional object-attribute lane lost its focused C++ target handle"
        )

    zero_dim = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-zero-dimensional-region-nonoverlap-integration"
        ),
        None,
    )
    if not isinstance(zero_dim, dict):
        raise AssertionError("zero-dimensional region row is absent")
    if query_rti_work.source_location_text(zero_dim) != (
        "cpp/tests/regional_object_attribute_routing_catch2.cpp:736"
    ):
        raise AssertionError("zero-dimensional region source pointer drifted")
    if zero_dim.get("assertions") != 26:
        raise AssertionError("zero-dimensional region assertion count drifted")
    if zero_dim.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l23-7"
    ]:
        raise AssertionError("zero-dimensional region requirement mapping drifted")
    if zero_dim.get("standard_sections") != ["hla-1516.1-2025:clause-9.1.3.1"]:
        raise AssertionError("zero-dimensional region section mapping drifted")
    zero_dim_focus = query_rti_work.focused_lane_result(
        index, tests, "zero-dimensional-region", limit=0
    )
    if zero_dim_focus.get("lane_state") != "complete":
        raise AssertionError("zero-dimensional region lane should be complete")
    if zero_dim_focus.get("mapped_test_count") != 1:
        raise AssertionError("zero-dimensional region mapped count drifted")
    if zero_dim_focus.get("assertion_count") != 26:
        raise AssertionError("zero-dimensional region lane assertion total drifted")
    zero_dim_handles = zero_dim_focus.get("lane_handles")
    if (
        not isinstance(zero_dim_handles, dict)
        or zero_dim_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
        or zero_dim_handles.get("ctest_label") != "zero-dimensional-region"
    ):
        raise AssertionError("zero-dimensional region lane handles drifted")

    zero_dim_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-zero-dimensional-regional-interaction-nonoverlap-integration"
        ),
        None,
    )
    if not isinstance(zero_dim_interaction, dict):
        raise AssertionError("zero-dimensional regional-interaction row is absent")
    if query_rti_work.source_location_text(zero_dim_interaction) != (
        "cpp/tests/zero_dimensional_regional_interaction_catch2.cpp:91"
    ):
        raise AssertionError(
            "zero-dimensional regional-interaction source pointer drifted"
        )
    if zero_dim_interaction.get("assertions") != 28:
        raise AssertionError(
            "zero-dimensional regional-interaction assertion count drifted"
        )
    if zero_dim_interaction.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-219-l23-7"
    ]:
        raise AssertionError(
            "zero-dimensional regional-interaction requirement mapping drifted"
        )
    if zero_dim_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.1",
    ]:
        raise AssertionError(
            "zero-dimensional regional-interaction section mapping drifted"
        )
    zero_dim_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "zero-dimensional-regional-interaction", limit=0
    )
    if zero_dim_interaction_focus.get("lane_state") != "complete":
        raise AssertionError(
            "zero-dimensional regional-interaction lane should be complete"
        )
    if zero_dim_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "zero-dimensional regional-interaction mapped count drifted"
        )
    if zero_dim_interaction_focus.get("assertion_count") != 28:
        raise AssertionError(
            "zero-dimensional regional-interaction lane assertion total drifted"
        )
    zero_dim_interaction_handles = zero_dim_interaction_focus.get("lane_handles")
    if (
        not isinstance(zero_dim_interaction_handles, dict)
        or zero_dim_interaction_handles.get("catch2_target")
        != "umbra_zero_dimensional_regional_interaction_catch2"
        or zero_dim_interaction_handles.get("ctest_label")
        != "zero-dimensional-regional-interaction"
    ):
        raise AssertionError(
            "zero-dimensional regional-interaction lane handles drifted"
        )

    focused_positive_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-default-region-interaction-routing-focused-integration"
        ),
        None,
    )
    if not isinstance(focused_positive_interaction, dict):
        raise AssertionError(
            "focused positive-dimensional regional-interaction row is absent"
        )
    if query_rti_work.source_location_text(focused_positive_interaction) != (
        "cpp/tests/default_region_interaction_routing_catch2.cpp:89"
    ):
        raise AssertionError(
            "focused positive-dimensional interaction source pointer drifted"
        )
    if focused_positive_interaction.get("assertions") != 58:
        raise AssertionError(
            "focused positive-dimensional interaction assertion count drifted"
        )
    if len(focused_positive_interaction.get("lab_requirement_ids", [])) != 10:
        raise AssertionError(
            "focused positive-dimensional interaction requirement mapping drifted"
        )
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-236-l176-53"
        not in focused_positive_interaction.get("lab_requirement_ids", [])
    ):
        raise AssertionError(
            "regional subscription-independence requirement mapping drifted"
        )
    if focused_positive_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.3",
        "hla-1516.1-2025:clause-9.1.4",
        "hla-1516.1-2025:clause-9.1.5",
        "hla-1516.1-2025:clause-9.1.7",
        "hla-1516.1-2025:clause-9.10",
    ]:
        raise AssertionError(
            "focused positive-dimensional interaction section mapping drifted"
        )
    focused_positive_pairs = query_rti_work.matrix_summary_data(
        focused_positive_interaction, index
    ).get("requirement_section_mappings", [])
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-236-l176-53",
        "hla-1516.1-2025:clause-9.10",
    ) not in {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for row in focused_positive_pairs
        if isinstance(row, dict)
    }:
        raise AssertionError(
            "regional subscription-independence direct crosswalk drifted"
        )
    focused_positive_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "focused-positive-dimensional-regional-interaction", limit=0
    )
    if focused_positive_interaction_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused positive-dimensional interaction lane should be complete"
        )
    if focused_positive_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused positive-dimensional interaction mapped count drifted"
        )
    if focused_positive_interaction_focus.get("assertion_count") != 58:
        raise AssertionError(
            "focused positive-dimensional interaction lane assertion total drifted"
        )
    if focused_positive_interaction_focus.get("requirement_count") != 10:
        raise AssertionError(
            "focused positive-dimensional interaction lane requirement total drifted"
        )
    if focused_positive_interaction_focus.get("standard_section_count") != 5:
        raise AssertionError(
            "focused positive-dimensional interaction lane section total drifted"
        )
    focused_positive_interaction_handles = focused_positive_interaction_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(focused_positive_interaction_handles, dict)
        or focused_positive_interaction_handles.get("catch2_target")
        != "umbra_default_region_interaction_routing_catch2"
        or focused_positive_interaction_handles.get("ctest_label")
        != "focused-positive-dimensional-regional-interaction"
    ):
        raise AssertionError(
            "focused positive-dimensional interaction lane handles drifted"
        )

    multi_region_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-multi-region-interaction-routing-integration"
        ),
        None,
    )
    if not isinstance(multi_region_interaction, dict):
        raise AssertionError("multi-region interaction row is absent")
    if query_rti_work.source_location_text(multi_region_interaction) != (
        "cpp/tests/multi_region_interaction_routing_catch2.cpp:106"
    ):
        raise AssertionError("multi-region interaction source pointer drifted")
    if multi_region_interaction.get("assertions") != 107:
        raise AssertionError("multi-region interaction assertion count drifted")
    if len(multi_region_interaction.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("multi-region interaction requirement mapping drifted")
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-236-l143-42"
        not in multi_region_interaction.get("lab_requirement_ids", [])
    ):
        raise AssertionError(
            "multi-region regional-subscription requirement mapping drifted"
        )
    if multi_region_interaction.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.3.3",
        "hla-1516.1-2025:clause-9.1.4",
        "hla-1516.1-2025:clause-9.10",
    ]:
        raise AssertionError("multi-region interaction section mapping drifted")
    multi_region_pairs = query_rti_work.matrix_summary_data(
        multi_region_interaction, index
    ).get("requirement_section_mappings", [])
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-236-l143-42",
        "hla-1516.1-2025:clause-9.10",
    ) not in {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for row in multi_region_pairs
        if isinstance(row, dict)
    }:
        raise AssertionError(
            "multi-region direct regional-subscription crosswalk drifted"
        )
    multi_region_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "multi-region-regional-interaction", limit=0
    )
    if multi_region_interaction_focus.get("lane_state") != "complete":
        raise AssertionError("multi-region interaction lane should be complete")
    if multi_region_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError("multi-region interaction mapped count drifted")
    if multi_region_interaction_focus.get("assertion_count") != 107:
        raise AssertionError("multi-region interaction lane assertion total drifted")
    if multi_region_interaction_focus.get("requirement_count") != 8:
        raise AssertionError("multi-region interaction lane requirement total drifted")
    if multi_region_interaction_focus.get("standard_section_count") != 3:
        raise AssertionError("multi-region interaction lane section total drifted")
    multi_region_interaction_handles = multi_region_interaction_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(multi_region_interaction_handles, dict)
        or multi_region_interaction_handles.get("catch2_target")
        != "umbra_multi_region_interaction_routing_catch2"
        or multi_region_interaction_handles.get("ctest_label")
        != "multi-region-regional-interaction"
    ):
        raise AssertionError("multi-region interaction lane handles drifted")

    match_kind, trace_matches = query_rti_work.resolve_trace_query(tests, TARGET_TEST)
    if match_kind != "test" or len(trace_matches) != 1:
        raise AssertionError(
            f"exact trace handle did not resolve deterministically: {match_kind}, "
            f"{len(trace_matches)} matches"
        )

    # Developers normally have the printed subsection number at hand, not the
    # longer corpus key.  Keep the three supported spellings equivalent so the
    # section, trace, and reverse-matrix entry points remain easy to query.
    for section_query in (
        "6.8.4",
        "clause-6.8.4",
        "hla-1516.1-2025:clause-6.8.4",
    ):
        section_matches = query_rti_work.select_tests(tests, section_query, "section")
        if not section_matches:
            raise AssertionError(f"section lookup did not resolve {section_query!r}")
        section_kind, section_trace_matches = query_rti_work.resolve_trace_query(
            tests, section_query
        )
        if section_kind != "section" or not section_trace_matches:
            raise AssertionError(
                f"trace section lookup did not resolve {section_query!r}"
            )
        matrix_kind, matrix_matches = query_rti_work.resolve_matrix_query(
            index, tests, section_query
        )
        if matrix_kind != "section" or not matrix_matches:
            raise AssertionError(
                f"matrix section lookup did not resolve {section_query!r}"
            )

    # Multi-word discovery is intentionally bounded and defaults to an AND
    # across indexed fields.  Keep the explicit broad escape hatch separate so
    # a short search cannot silently become the normal roadmap workflow.
    restore_work_matches = query_rti_work.select_search_tests(
        tests, ["restore", "work"], all_terms=True
    )
    restore_any_matches = query_rti_work.select_search_tests(
        tests, ["restore", "work"], all_terms=False
    )
    if not restore_work_matches:
        raise AssertionError("multi-term indexed search lost the restore/work seed")
    if not set(map(id, restore_work_matches)).issubset(
        set(map(id, restore_any_matches))
    ):
        raise AssertionError("AND search is not a subset of the explicit broad search")
    mapped_search_hit = next(
        (
            test
            for test in restore_work_matches
            if len(query_rti_work.requirement_section_mapping_rows(test)) > 3
        ),
        None,
    )
    if mapped_search_hit is None:
        raise AssertionError("restore/work search lost its mapped traceability seed")
    search_summary = query_rti_work.text_search_summary_row(mapped_search_hit)
    search_mappings = query_rti_work.requirement_section_mapping_rows(mapped_search_hit)
    if f"direct_pairs={len(search_mappings)}" not in search_summary:
        raise AssertionError("search summary omitted the exact direct-pair count")
    if "requirement_section_mappings:" not in search_summary:
        raise AssertionError("search summary omitted its bounded direct-pair preview")
    for mapping in search_mappings[:1]:
        expected_pair = (
            f"{mapping.get('lab_requirement_id')} -> "
            f"{mapping.get('standard_section') or '<unresolved>'}"
        )
        if expected_pair not in search_summary:
            raise AssertionError(
                "search summary preview diverged from the direct mapping"
            )
    second_mapping = search_mappings[1]
    second_pair = (
        f"{second_mapping.get('lab_requirement_id')} -> "
        f"{second_mapping.get('standard_section') or '<unresolved>'}"
    )
    if second_pair in search_summary or "... (+" not in search_summary:
        raise AssertionError("search mapping preview is not bounded to one pair")
    if not query_rti_work.test_matches_lane(
        {"primary_lane": "metadata-only-lane", "tags": []},
        "metadata-only-lane",
    ):
        raise AssertionError("plan-row lane aliases are not queryable")
    parsed_search = query_rti_work.build_parser().parse_args(
        ["search", "restore", "work"]
    )
    if parsed_search.query != ["restore", "work"] or parsed_search.any_term:
        raise AssertionError("multi-term search parser drifted from AND defaults")
    parsed_broad_search = query_rti_work.build_parser().parse_args(
        ["search", "restore", "--any-term"]
    )
    if not parsed_broad_search.any_term:
        raise AssertionError("explicit broad search escape hatch is unavailable")
    parsed_tck_gaps = query_rti_work.build_parser().parse_args(
        [
            "tck-gaps",
            "--document",
            "hla-1516.1-2025",
            "--clause",
            "clause-8.6",
            "--summary",
        ]
    )
    if (
        parsed_tck_gaps.command != "tck-gaps"
        or parsed_tck_gaps.document != "hla-1516.1-2025"
        or parsed_tck_gaps.clause != "clause-8.6"
        or not parsed_tck_gaps.summary
    ):
        raise AssertionError("portable TCK direct-gap query parser drifted")

    # The implementation plan is a second source of navigation context, not
    # another prose search surface.  Protect the heading-only filter so a
    # resume can select a section and line without loading the full document.
    plan_sections = query_rti_work.implementation_plan_sections(
        query_rti_work.DEFAULT_IMPLEMENTATION_PLAN
    )
    if not plan_sections:
        raise AssertionError("implementation-plan heading index is empty")
    if any(not section.get("path") for section in plan_sections):
        raise AssertionError("implementation-plan headings lost breadcrumb paths")
    plan_matches, plan_shown = query_rti_work.filtered_plan_sections(
        plan_sections,
        "current indexed",
        2,
    )
    if not plan_matches or len(plan_shown) != min(2, len(plan_matches)):
        raise AssertionError(
            "filtered implementation-plan heading query is not bounded"
        )
    if not all(
        "current indexed" in section.get("path", "").casefold()
        for section in plan_shown
    ):
        raise AssertionError(
            "filtered plan headings do not carry the requested breadcrumb"
        )
    _, all_plan_rows = query_rti_work.filtered_plan_sections(plan_sections, None, 0)
    if len(all_plan_rows) != len(plan_sections):
        raise AssertionError("unbounded implementation-plan outline dropped headings")

    print(
        "query-rti-work: direct requirement-to-2025-section matrix regression "
        f"passed ({len(pairs)} mapped requirements)"
    )
    return 0
