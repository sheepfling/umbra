# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import (
    LaneCatalogInputs,
)


def run_lane_catalog(inputs: LaneCatalogInputs) -> query_rti_work.JsonObject:
    index = inputs.index
    plan = inputs.plan
    process_item = inputs.process_item
    process_lane = inputs.process_lane
    process_rows = inputs.process_rows
    queue_by_id = inputs.queue_by_id
    source_health = inputs.source_health
    source_locations = inputs.source_locations
    standard_requirements = inputs.standard_requirements
    tests = inputs.tests
    # Broad family tags are intentionally numerous and overlap across owners.
    # The focused inventory must stay a small, explicit family-owned selector
    # so normal work selection does not expand into taxonomy-sized output.
    focused_process_inventory = query_rti_work.lane_inventory(
        index,
        tests,
        family="transport-and-conformance",
        focused_only=True,
        limit=0,
    )
    focused_process_tags = {
        row.get("tag")
        for row in focused_process_inventory.get("lanes", [])
        if isinstance(row, dict) and row.get("tag")
    }
    expected_focused_process_tags = set(process_item.get("focused_lane_tags", []))
    if (
        focused_process_inventory.get("focused_only") is not True
        or focused_process_inventory.get("focused_tag_count")
        != len(expected_focused_process_tags)
        or focused_process_tags != expected_focused_process_tags
        or "transport" in focused_process_tags
    ):
        raise AssertionError(
            "focused transport lane inventory did not stay within its explicit aliases"
        )
    process_handles = process_lane.get("lane_handles")
    if not isinstance(process_handles, dict):
        raise AssertionError("process-boundary lane lost its evidence handles")
    if process_handles.get("ctest_execution_count") != 209:
        raise AssertionError("process-boundary CTest execution baseline drifted")
    if process_handles.get("junit_ctest_execution_count") != 119:
        raise AssertionError("process-boundary JUnit CTest scope baseline drifted")
    if process_handles.get("junit_testcase_count") != 471:
        raise AssertionError("process-boundary JUnit testcase baseline drifted")
    if process_handles.get("junit_assertion_count") != 17918:
        raise AssertionError("process-boundary JUnit assertion baseline drifted")
    if process_handles.get("junit_failures") != 0:
        raise AssertionError("process-boundary JUnit report is not green")
    if process_handles.get("junit_errors") != 0:
        raise AssertionError("process-boundary JUnit report has errors")
    if process_handles.get("junit_input_targets") != [
        "umbra_process_boundary_private_catch2",
        "umbra_ieee1516_2025_connection_catch2",
        "umbra_attribute_ownership_query_catch2",
        "umbra_attribute_ownership_acquisition_if_available_catch2",
        "umbra_attribute_ownership_acquisition_catch2",
    ]:
        raise AssertionError(
            "process-boundary JUnit inputs are not independently buildable"
        )
    if process_handles.get("junit_input_artifacts") != [
        "compliance/process-boundary/process-boundary-private.xml",
        "compliance/process-boundary/process-boundary-public.xml",
        "compliance/process-boundary/process-boundary-ownership-query.xml",
        "compliance/process-boundary/process-boundary-ownership-acquisition-if-available.xml",
        "compliance/process-boundary/process-boundary-ownership-acquisition.xml",
    ]:
        raise AssertionError("process-boundary JUnit input artifacts are not indexed")
    if process_handles.get("junit_merge_script") != "tools/merge_junit_reports.py":
        raise AssertionError("process-boundary JUnit merge script handle drifted")
    directed_process = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-directed-interaction-integration"
        ),
        None,
    )
    if not isinstance(directed_process, dict):
        raise AssertionError("directed process plan row is absent")
    if (
        query_rti_work.source_location_text(directed_process)
        != "cpp/tests/ieee1516_2025_connection_directed_interaction_catch2.cpp:457"
        or directed_process.get("assertions") != 22
        or directed_process.get("primary_lane")
        != "process-directed-interaction-routing"
        or len(directed_process.get("lab_requirement_ids", [])) != 11
        or len(directed_process.get("standard_sections", [])) != 8
        or len(directed_process.get("selected_cpp_api_surface_ids", [])) != 5
        or "process-directed-interaction-routing"
        not in directed_process.get("tags", [])
    ):
        raise AssertionError("directed process source/evidence mapping drifted")
    directed_process_focus = query_rti_work.focused_lane_result(
        index, tests, "process-directed-interaction-routing", limit=0
    )
    if (
        directed_process_focus.get("lane_state") != "complete"
        or directed_process_focus.get("roadmap_owner") != "transport-and-conformance"
        or directed_process_focus.get("mapped_test_count") != 1
        or directed_process_focus.get("assertion_count") != 22
        or directed_process_focus.get("recorded_assertion_count") != 22
        or directed_process_focus.get("requirement_count") != 11
        or directed_process_focus.get("standard_section_count") != 8
        or directed_process_focus.get("requirement_section_pair_count") != 11
    ):
        raise AssertionError("directed process lane card drifted")
    directed_process_handles = directed_process_focus.get("lane_handles")
    if not isinstance(directed_process_handles, dict) or (
        directed_process_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or directed_process_handles.get("ctest_label")
        != "process-directed-interaction-routing"
        or "routes a directed interaction through a configured process endpoint"
        not in str(directed_process_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("directed process CTest handle drifted")
    directed_process_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-directed-interaction-multi-recipient-integration"
        ),
        None,
    )
    if not isinstance(directed_process_multi, dict):
        raise AssertionError("multi-recipient directed process plan row is absent")
    if (
        query_rti_work.source_location_text(directed_process_multi)
        != "cpp/tests/ieee1516_2025_connection_directed_interaction_multi_recipient_catch2.cpp:457"
        or directed_process_multi.get("assertions") != 104
        or directed_process_multi.get("primary_lane")
        != "process-directed-interaction-multi-recipient"
        or len(directed_process_multi.get("lab_requirement_ids", [])) != 14
        or len(directed_process_multi.get("standard_sections", [])) != 10
        or len(directed_process_multi.get("selected_cpp_api_surface_ids", [])) != 12
        or directed_process_multi.get("callback_models")
        != ["HLA_EVOKED", "HLA_IMMEDIATE"]
        or "process-directed-interaction-multi-recipient"
        not in directed_process_multi.get("tags", [])
    ):
        raise AssertionError(
            "multi-recipient directed process source/evidence mapping drifted"
        )
    directed_process_multi_focus = query_rti_work.focused_lane_result(
        index, tests, "process-directed-interaction-multi-recipient", limit=0
    )
    if (
        directed_process_multi_focus.get("lane_state") != "complete"
        or directed_process_multi_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or directed_process_multi_focus.get("mapped_test_count") != 1
        or directed_process_multi_focus.get("assertion_count") != 104
        or directed_process_multi_focus.get("recorded_assertion_count") != 104
        or directed_process_multi_focus.get("requirement_count") != 14
        or directed_process_multi_focus.get("standard_section_count") != 10
        or directed_process_multi_focus.get("requirement_section_pair_count") != 14
    ):
        raise AssertionError("multi-recipient directed process lane card drifted")
    directed_process_multi_handles = directed_process_multi_focus.get("lane_handles")
    if not isinstance(directed_process_multi_handles, dict) or (
        directed_process_multi_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or directed_process_multi_handles.get("ctest_label")
        != "process-directed-interaction-multi-recipient"
        or "deliver one directed interaction to multiple subscribed recipients"
        not in str(directed_process_multi_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("multi-recipient directed process CTest handle drifted")
    directed_process_multi_links = query_rti_work.roadmap_links_for_test(
        index, directed_process_multi, limit=0
    )
    if (
        not directed_process_multi_links
        or directed_process_multi_links[0].get("id") != ("transport-and-conformance")
        or directed_process_multi_links[0].get("match") != "lane_owner"
    ):
        raise AssertionError(
            "multi-recipient directed process trace did not prioritize its owning roadmap family"
        )
    directed_process_transport = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-directed-interaction-transportation-override-integration"
        ),
        None,
    )
    if not isinstance(directed_process_transport, dict):
        raise AssertionError("directed transportation process plan row is absent")
    if (
        query_rti_work.source_location_text(directed_process_transport)
        != "cpp/tests/ieee1516_2025_connection_process_directed_interaction_transportation_catch2.cpp:475"
        or directed_process_transport.get("assertions") != 66
        or directed_process_transport.get("primary_lane")
        != "process-directed-interaction-transportation"
        or len(directed_process_transport.get("lab_requirement_ids", [])) != 7
        or len(directed_process_transport.get("standard_sections", [])) != 3
        or len(directed_process_transport.get("selected_cpp_api_surface_ids", [])) != 7
        or directed_process_transport.get("callback_models")
        != ["HLA_EVOKED", "HLA_IMMEDIATE"]
        or "process-directed-interaction-transportation"
        not in directed_process_transport.get("tags", [])
    ):
        raise AssertionError(
            "directed transportation process source/evidence mapping drifted"
        )
    directed_process_transport_focus = query_rti_work.focused_lane_result(
        index, tests, "process-directed-interaction-transportation", limit=0
    )
    if (
        directed_process_transport_focus.get("lane_state") != "complete"
        or directed_process_transport_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or directed_process_transport_focus.get("mapped_test_count") != 1
        or directed_process_transport_focus.get("assertion_count") != 66
        or directed_process_transport_focus.get("recorded_assertion_count") != 66
        or directed_process_transport_focus.get("requirement_count") != 7
        or directed_process_transport_focus.get("standard_section_count") != 3
        or directed_process_transport_focus.get("requirement_section_pair_count") != 7
    ):
        raise AssertionError("directed transportation process lane card drifted")
    directed_process_transport_handles = directed_process_transport_focus.get(
        "lane_handles"
    )
    if not isinstance(directed_process_transport_handles, dict) or (
        directed_process_transport_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or directed_process_transport_handles.get("ctest_label")
        != "process-directed-interaction-transportation"
        or "preserve a directed interaction transportation override"
        not in str(directed_process_transport_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("directed transportation process CTest handle drifted")
    directed_process_transport_links = query_rti_work.roadmap_links_for_test(
        index, directed_process_transport, limit=0
    )
    if (
        not directed_process_transport_links
        or directed_process_transport_links[0].get("id") != "transport-and-conformance"
        or directed_process_transport_links[0].get("match") != "lane_owner"
    ):
        raise AssertionError(
            "directed transportation process trace did not prioritize its owning roadmap family"
        )
    directed_transport_query_focus = query_rti_work.focused_lane_result(
        index, tests, "directed-interaction-transportation-query", limit=0
    )
    if (
        directed_transport_query_focus.get("lane_state") != "missing"
        or directed_transport_query_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or directed_transport_query_focus.get("mapped_test_count") != 0
        or directed_transport_query_focus.get("assertion_count") != 0
        or directed_transport_query_focus.get("recorded_assertion_count") != 0
        or directed_transport_query_focus.get("requirement_count") != 0
        or directed_transport_query_focus.get("standard_section_count") != 0
        or directed_transport_query_focus.get("requirement_section_pair_count") != 0
        or directed_transport_query_focus.get("lane_handles")
    ):
        raise AssertionError(
            "unimplemented embedded directed-transportation query lane advertises evidence"
        )
    directed_process_transport_query = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-directed-interaction-transportation-query-integration"
        ),
        None,
    )
    if not isinstance(directed_process_transport_query, dict):
        raise AssertionError("directed transportation process query plan row is absent")
    if (
        query_rti_work.source_location_text(directed_process_transport_query)
        != "cpp/tests/ieee1516_2025_connection_interaction_transportation_catch2.cpp:247"
        or directed_process_transport_query.get("assertions") != 44
        or directed_process_transport_query.get("primary_lane")
        != "process-directed-interaction-transportation-query"
        or len(directed_process_transport_query.get("lab_requirement_ids", [])) != 11
        or len(directed_process_transport_query.get("standard_sections", [])) != 5
        or len(directed_process_transport_query.get("selected_cpp_api_surface_ids", []))
        != 10
        or directed_process_transport_query.get("callback_models")
        != ["HLA_EVOKED", "HLA_IMMEDIATE"]
        or "process-directed-interaction-transportation-query"
        not in directed_process_transport_query.get("tags", [])
    ):
        raise AssertionError(
            "directed transportation process query source/evidence mapping drifted"
        )
    directed_process_transport_query_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-directed-interaction-transportation-query",
        limit=0,
    )
    if (
        directed_process_transport_query_focus.get("lane_state") != "complete"
        or directed_process_transport_query_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or directed_process_transport_query_focus.get("mapped_test_count") != 1
        or directed_process_transport_query_focus.get("assertion_count") != 44
        or directed_process_transport_query_focus.get("recorded_assertion_count") != 44
        or directed_process_transport_query_focus.get("requirement_count") != 11
        or directed_process_transport_query_focus.get("standard_section_count") != 5
        or directed_process_transport_query_focus.get("requirement_section_pair_count")
        != 11
    ):
        raise AssertionError("directed transportation process query lane card drifted")
    directed_process_transport_query_handles = (
        directed_process_transport_query_focus.get("lane_handles")
    )
    if not isinstance(directed_process_transport_query_handles, dict) or (
        directed_process_transport_query_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or directed_process_transport_query_handles.get("ctest_label")
        != "process-directed-interaction-transportation-query"
        or "reports a directed interaction transportation override"
        not in str(directed_process_transport_query_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "directed transportation process query CTest handle drifted"
        )
    directed_process_transport_query_links = query_rti_work.roadmap_links_for_test(
        index, directed_process_transport_query, limit=0
    )
    if (
        not directed_process_transport_query_links
        or directed_process_transport_query_links[0].get("id")
        != "transport-and-conformance"
        or directed_process_transport_query_links[0].get("match") != "lane_owner"
    ):
        raise AssertionError(
            "directed transportation process query trace did not prioritize its owning roadmap family"
        )
    regional_transport = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-endpoint-regional-interaction-transportation-override-integration"
        ),
        None,
    )
    if not isinstance(regional_transport, dict):
        raise AssertionError("regional transportation plan row is absent")
    if (
        query_rti_work.source_location_text(regional_transport)
        != "cpp/tests/ieee1516_2025_connection_regional_interaction_transportation_override_catch2.cpp:126"
        or regional_transport.get("assertions") != 86
        or regional_transport.get("status")
        != "implemented-private-foundation-not-conformance-evidence"
        or len(regional_transport.get("lab_requirement_ids", [])) != 22
        or len(regional_transport.get("standard_sections", [])) != 10
        or len(regional_transport.get("selected_cpp_api_surface_ids", [])) != 17
    ):
        raise AssertionError("regional transportation source/evidence mapping drifted")
    regional_transport_focus = query_rti_work.focused_lane_result(
        index, tests, "process-transportation-regional-interaction-control", limit=0
    )
    if (
        regional_transport_focus.get("lane_state") != "complete"
        or regional_transport_focus.get("mapped_test_count") != 1
        or regional_transport_focus.get("assertion_count") != 84
        or regional_transport_focus.get("recorded_assertion_count") != 86
        or regional_transport_focus.get("requirement_count") != 22
        or regional_transport_focus.get("standard_section_count") != 10
        or regional_transport_focus.get("requirement_section_pair_count") != 22
    ):
        raise AssertionError("regional transportation lane card drifted")
    regional_transport_handles = regional_transport_focus.get("lane_handles")
    if not isinstance(regional_transport_handles, dict) or (
        regional_transport_handles.get("catch2_target") != "umbra_ieee1516_2025_catch2"
        or "regional interaction transportation override"
        not in str(regional_transport_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("regional transportation CTest handle drifted")
    timestamped_regional_transport = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-regional-interaction-transportation-override-integration"
        ),
        None,
    )
    if not isinstance(timestamped_regional_transport, dict):
        raise AssertionError("timestamped regional transportation plan row is absent")
    if (
        query_rti_work.source_location_text(timestamped_regional_transport)
        != "cpp/tests/ieee1516_2025_connection_process_tso_regional_interaction_transportation_catch2.cpp:547"
        or timestamped_regional_transport.get("assertions") != 122
        or timestamped_regional_transport.get("status")
        != "implemented-private-foundation-not-conformance-evidence"
        or len(timestamped_regional_transport.get("lab_requirement_ids", [])) != 22
        or len(timestamped_regional_transport.get("standard_sections", [])) != 12
        or len(timestamped_regional_transport.get("selected_cpp_api_surface_ids", []))
        != 23
    ):
        raise AssertionError(
            "timestamped regional transportation source/evidence mapping drifted"
        )
    timestamped_regional_transport_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-transportation-timestamped-regional-interaction-control",
        limit=0,
    )
    if (
        timestamped_regional_transport_focus.get("lane_state") != "complete"
        or timestamped_regional_transport_focus.get("mapped_test_count") != 1
        or timestamped_regional_transport_focus.get("assertion_count") != 122
        or timestamped_regional_transport_focus.get("recorded_assertion_count") != 122
        or timestamped_regional_transport_focus.get("requirement_count") != 22
        or timestamped_regional_transport_focus.get("standard_section_count") != 12
        or timestamped_regional_transport_focus.get("requirement_section_pair_count")
        != 22
    ):
        raise AssertionError("timestamped regional transportation lane card drifted")
    timestamped_regional_transport_handles = timestamped_regional_transport_focus.get(
        "lane_handles"
    )
    if not isinstance(timestamped_regional_transport_handles, dict) or (
        timestamped_regional_transport_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or "timestamped regional interaction transportation override"
        not in str(timestamped_regional_transport_handles.get("ctest_filter") or "")
        or timestamped_regional_transport_handles.get("ctest_label")
        != "process-transportation-timestamped-regional-interaction-control"
    ):
        raise AssertionError("timestamped regional transportation CTest handle drifted")
    object_queue_row = queue_by_id.get("object-ddm-ownership")
    if not isinstance(object_queue_row, dict):
        raise AssertionError("queue lost the object/ownership handoff")
    if (
        object_queue_row.get("state") != "complete-pointer"
        or object_queue_row.get("action_state") != "evidence-complete"
        or object_queue_row.get("planned_count") != 0
        or object_queue_row.get("next_test_query")
        != "Process immediate delayed regional callback completes before resign returns"
        or object_queue_row.get("next_test_state") != "complete"
        or object_queue_row.get("next_test_pointer", {}).get("case_query")
        != "umbra-cpp-process-deferred-regional-resign-fence"
        or object_queue_row.get("next_test_pointer", {}).get(
            "requirement_section_pair_count"
        )
        != 3
        or "gaps --family portable-cpp-tck-2025"
        not in str(object_queue_row.get("next_action") or "")
    ):
        raise AssertionError("object/ownership queue handoff drifted")
    push_confirm = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-confirm-divestiture-process-push-integration"
        ),
        None,
    )
    if not isinstance(push_confirm, dict):
        raise AssertionError("push Confirm Divestiture row is absent")
    if (
        push_confirm.get("status")
        != "implemented-public-foundation-not-conformance-evidence"
        or query_rti_work.source_location_text(push_confirm)
        != "cpp/tests/ieee1516_2025_process_confirm_divestiture_push_catch2.cpp:53"
        or push_confirm.get("assertions") != 43
        or len(push_confirm.get("lab_requirement_ids", [])) != 4
        or len(push_confirm.get("standard_sections", [])) != 4
        or len(push_confirm.get("selected_cpp_api_surface_ids", [])) != 15
    ):
        raise AssertionError("push Confirm Divestiture mapping drifted")
    push_confirm_focus = query_rti_work.focused_lane_result(
        index, tests, "process-confirm-divestiture-push", limit=0
    )
    if (
        push_confirm_focus.get("lane_state") != "complete"
        or push_confirm_focus.get("action_state") != "evidence-complete"
        or push_confirm_focus.get("planned_count") != 0
        or push_confirm_focus.get("mapped_test_count") != 1
        or push_confirm_focus.get("requirement_count") != 4
        or push_confirm_focus.get("standard_section_count") != 4
        or push_confirm_focus.get("requirement_section_pair_count") != 4
        or push_confirm_focus.get("assertion_count") != 43
    ):
        raise AssertionError("push Confirm Divestiture focus card drifted")
    assumption_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-confirm-divestiture-process-assumption-integration"
        ),
        None,
    )
    if not isinstance(assumption_case, dict):
        raise AssertionError("Confirm Divestiture assumption row is absent")
    if (
        assumption_case.get("status")
        != "implemented-public-foundation-not-conformance-evidence"
        or query_rti_work.source_location_text(assumption_case)
        != "cpp/tests/ieee1516_2025_confirm_divestiture_process_assumption_catch2.cpp:62"
        or assumption_case.get("assertions") != 60
        or len(assumption_case.get("lab_requirement_ids", [])) != 1
        or len(assumption_case.get("standard_sections", [])) != 1
        or len(assumption_case.get("selected_cpp_api_surface_ids", [])) != 17
    ):
        raise AssertionError("Confirm Divestiture assumption mapping drifted")
    assumption_focus = query_rti_work.focused_lane_result(
        index, tests, "process-confirm-divestiture-assumption", limit=0
    )
    if (
        assumption_focus.get("lane_state") != "complete"
        or assumption_focus.get("action_state") != "evidence-complete"
        or assumption_focus.get("planned_count") != 0
        or assumption_focus.get("mapped_test_count") != 1
        or assumption_focus.get("requirement_count") != 1
        or assumption_focus.get("standard_section_count") != 1
        or assumption_focus.get("requirement_section_pair_count") != 1
        or assumption_focus.get("assertion_count") != 60
    ):
        raise AssertionError("Confirm Divestiture assumption focus card drifted")
    ownership_lane = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-check", limit=1
    )
    if (
        ownership_lane.get("mapped_test_count") != 1
        or ownership_lane.get("assertion_count") != 18
        or ownership_lane.get("requirement_section_pair_count") != 3
        or ownership_lane.get("roadmap_owner") != "object-ddm-ownership"
    ):
        raise AssertionError("process-ownership-check lane baseline drifted")
    ownership_test = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-check-process-integration"
        ),
        None,
    )
    if not isinstance(ownership_test, dict):
        raise AssertionError("process ownership-check plan row is absent")
    if ownership_test.get("primary_lane") != "process-ownership-check":
        raise AssertionError("process ownership-check primary lane drifted")
    ownership_links = query_rti_work.roadmap_links_for_test(index, ownership_test)
    ownership_owner = query_rti_work.primary_roadmap_link(ownership_links)
    if (
        not isinstance(ownership_owner, dict)
        or ownership_owner.get("id") != "object-ddm-ownership"
    ):
        raise AssertionError("process ownership-check roadmap owner drifted")
    ownership_query_lane = query_rti_work.focused_lane_result(
        index, tests, "process-ownership-query", limit=1
    )
    if (
        ownership_query_lane.get("mapped_test_count") != 1
        or ownership_query_lane.get("assertion_count") != 26
        or ownership_query_lane.get("requirement_section_pair_count") != 6
        or ownership_query_lane.get("roadmap_owner") != "object-ddm-ownership"
    ):
        raise AssertionError("process-ownership-query lane baseline drifted")
    ownership_query_test = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-ownership-query-process-integration"
        ),
        None,
    )
    if not isinstance(ownership_query_test, dict):
        raise AssertionError("process ownership-query plan row is absent")
    if ownership_query_test.get("primary_lane") != "process-ownership-query":
        raise AssertionError("process ownership-query primary lane drifted")
    ownership_query_links = query_rti_work.roadmap_links_for_test(
        index, ownership_query_test
    )
    ownership_query_owner = query_rti_work.primary_roadmap_link(ownership_query_links)
    if (
        not isinstance(ownership_query_owner, dict)
        or ownership_query_owner.get("id") != "object-ddm-ownership"
    ):
        raise AssertionError("process ownership-query roadmap owner drifted")
    process_family_item, process_family_tests = query_rti_work.indexed_item(
        index, tests, "time-save-restore"
    )
    if process_family_item is None or not process_family_tests:
        raise AssertionError(
            "time-save-restore family did not resolve for queue checks"
        )
    process_family_counts = query_rti_work.live_item_mapping_counts(
        process_family_item, tests, index=index
    )
    if process_rows[0].get(
        "requirement_section_pair_count"
    ) != process_family_counts.get("requirement_section_pair_count"):
        raise AssertionError("queue direct pair count diverges from family mapping")
    scoped_process_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        focus_lane="process-boundary",
    )
    if scoped_process_check_errors:
        raise AssertionError(
            "process-boundary scoped roadmap check is not clean: "
            + "; ".join(scoped_process_check_errors[:3])
        )

    # A focused lane check must not be held hostage by a malformed mapping in
    # an unrelated lane. The unqualified check remains the repository-wide
    # gate and must still report that same injected defect.
    unrelated_probe = {
        "id": "query-rti-work-regression-unrelated-invalid-mapping",
        "test_case": "unrelated invalid mapping probe",
        "tags": ["query-rti-work-regression-unrelated-invalid-mapping"],
        "selected_requirements_lab_requirement_ids": [
            "requirement-not-in-the-2025-standard"
        ],
        "standard_sections": [],
    }
    probe_tests = [*tests, unrelated_probe]
    probe_plan = dict(plan)
    probe_plan["tests"] = probe_tests
    scoped_probe_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        probe_plan,
        standard_requirements,
        probe_tests,
        source_locations=source_locations,
        focus_lane="process-boundary",
    )
    if any(unrelated_probe["id"] in error for error in scoped_probe_errors):
        raise AssertionError("a focused lane check validated an unrelated plan row")
    repository_probe_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        probe_plan,
        standard_requirements,
        probe_tests,
        source_locations=source_locations,
    )
    if not any(
        unrelated_probe["id"] in error and "unknown 2025 requirement" in error
        for error in repository_probe_errors
    ):
        raise AssertionError(
            "the repository-wide roadmap check did not validate every plan row"
        )

    # The normal integrity gate is live-plan only.  Keep the append-only
    # completion ledger behind an explicit strict-audit switch so replaced
    # historical plan/source handles cannot consume the implementation turn.
    live_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
    )
    if live_check_errors:
        raise AssertionError(
            "live roadmap check is not clean: " + "; ".join(live_check_errors[:3])
        )
    # Historical source splits are audited explicitly with --historical; they
    # must not make this live-index regression fail.  The synthetic probe
    # below still verifies that strict mode checks the append-only ledger.
    historical_probe = dict(index)
    historical_probe["recent_completed_slices"] = [
        {"plan_id": "query-rti-work-regression-history-probe"}
    ]
    live_probe_errors = query_rti_work.validate_index(
        historical_probe,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
    )
    if any("recent_completed_slices" in error for error in live_probe_errors):
        raise AssertionError(
            "live roadmap check unexpectedly validated historical completion rows"
        )
    historical_probe_errors = query_rti_work.validate_index(
        historical_probe,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        include_historical=True,
    )
    if not any(
        "recent_completed_slices[0]" in error for error in historical_probe_errors
    ):
        raise AssertionError(
            "strict historical check did not validate the completion ledger"
        )
    fom_check_errors = query_rti_work.validate_index(
        index,
        query_rti_work.DEFAULT_ROADMAP,
        plan,
        standard_requirements,
        tests,
        source_locations=source_locations,
        focus_family="fom-module-declaration-management",
    )
    if fom_check_errors:
        raise AssertionError(
            "FOM module-management family check is not clean: "
            + "; ".join(fom_check_errors[:3])
        )

    # Baseline pointers are part of the one-screen work handoff.  Keep their
    # mapping counts synchronized with the exact Catch2 row so status/queue
    # can answer the common "what does this test cover?" question without a
    # second broad lookup.
    status = query_rti_work.index_status(
        index,
        tests,
        query_rti_work.roadmap_checklist(query_rti_work.DEFAULT_ROADMAP),
        source_locations,
    )
    if (
        not isinstance(source_health, dict)
        or source_health.get("files_scanned", 0) <= 0
    ):
        raise AssertionError("source-index health did not report scanned C++ files")
    if source_health.get("status") not in {"ok", "attention"}:
        raise AssertionError("source-index health returned an unknown status")
    latest = status.get("latest_completed_slice")
    if not isinstance(latest, dict):
        raise AssertionError("status omitted the latest completed slice handoff")
    latest_rows = [test for test in tests if test.get("id") == latest.get("plan_id")]
    if len(latest_rows) != 1:
        raise AssertionError("latest completed slice must resolve one exact plan row")
    latest_row = latest_rows[0]
    latest_plan_row = next(
        row for row in plan["tests"] if row.get("id") == latest["plan_id"]
    )
    for field in ("test_case", "focus_lane", "primary_lane", "assertions"):
        if latest.get(field) != latest_plan_row.get(field):
            raise AssertionError(
                f"latest completed slice {field} differs from its plan row"
            )
    if latest.get("source_location") != query_rti_work.source_location_text(latest_row):
        raise AssertionError("latest completed slice source pointer drifted")
    if (
        latest.get("requirements") != len(latest_row.get("lab_requirement_ids", []))
        or latest.get("standard_sections")
        != len(latest_row.get("standard_sections", []))
        or latest.get("api_surfaces")
        != len(latest_row.get("selected_cpp_api_surface_ids", []))
    ):
        raise AssertionError(
            "latest completed slice evidence counts differ from its plan row"
        )
    pushed_evoked_work_item = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-unconsumed-pushed-ownership-assumption-evoked-integration"
        ),
        None,
    )
    if not isinstance(pushed_evoked_work_item, dict):
        raise AssertionError("evoked pushed ownership-assumption plan row is absent")
    if (
        query_rti_work.source_location_text(pushed_evoked_work_item)
        != "cpp/tests/ieee1516_2025_process_unconsumed_pushed_ownership_assumption_evoked_catch2.cpp:62"
        or pushed_evoked_work_item.get("assertions") != 33
        or pushed_evoked_work_item.get("status")
        != "implemented-in-noninstallable-development-profile"
        or len(pushed_evoked_work_item.get("lab_requirement_ids", [])) != 6
        or len(pushed_evoked_work_item.get("standard_sections", [])) != 4
        or len(pushed_evoked_work_item.get("selected_cpp_api_surface_ids", [])) != 14
    ):
        raise AssertionError(
            "evoked pushed ownership-assumption source/evidence mapping drifted"
        )
    pushed_evoked_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-pushed-ownership-assumption-evoked",
        limit=0,
    )
    if (
        pushed_evoked_focus.get("lane_state") != "complete"
        or pushed_evoked_focus.get("mapped_test_count") != 1
        or pushed_evoked_focus.get("assertion_count") != 33
        or pushed_evoked_focus.get("requirement_count") != 6
        or pushed_evoked_focus.get("standard_section_count") != 4
        or pushed_evoked_focus.get("requirement_section_pair_count") != 6
    ):
        raise AssertionError("evoked pushed ownership-assumption lane card drifted")
    pushed_evoked_handles = pushed_evoked_focus.get("lane_handles")
    if (
        not isinstance(pushed_evoked_handles, dict)
        or pushed_evoked_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
        or "RTIambassadors deliver an unconsumed pushed ownership-assumption callback through HLA_EVOKED"
        not in str(pushed_evoked_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("evoked pushed ownership-assumption CTest handle drifted")
    durable_pending_work_item = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-pushed-ownership-assumption-evoked-save-restore-integration"
        ),
        None,
    )
    if not isinstance(durable_pending_work_item, dict):
        raise AssertionError("durable pending ownership-assumption plan row is absent")
    if (
        query_rti_work.source_location_text(durable_pending_work_item)
        != "cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_evoked_save_restore_catch2.cpp:62"
        or durable_pending_work_item.get("assertions") != 73
        or durable_pending_work_item.get("status")
        != "implemented-in-noninstallable-development-profile"
        or len(durable_pending_work_item.get("lab_requirement_ids", [])) != 10
        or len(durable_pending_work_item.get("standard_sections", [])) != 7
        or len(durable_pending_work_item.get("selected_cpp_api_surface_ids", [])) != 24
    ):
        raise AssertionError(
            "durable pending ownership-assumption source/evidence mapping drifted"
        )
    durable_pending_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-pushed-ownership-assumption-evoked-save-restore",
        limit=0,
    )
    if (
        durable_pending_focus.get("lane_state") != "complete"
        or durable_pending_focus.get("mapped_test_count") != 1
        or durable_pending_focus.get("assertion_count") != 73
        or durable_pending_focus.get("requirement_count") != 10
        or durable_pending_focus.get("standard_section_count") != 7
        or durable_pending_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("durable pending ownership-assumption lane card drifted")
    durable_pending_handles = durable_pending_focus.get("lane_handles")
    if (
        not isinstance(durable_pending_handles, dict)
        or durable_pending_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
        or "RTIambassadors preserve an unconsumed pushed ownership-assumption callback across HLA_EVOKED save and restore"
        not in str(durable_pending_handles.get("ctest_filter") or "")
    ):
        raise AssertionError(
            "durable pending ownership-assumption CTest handle drifted"
        )
    push_process_restore_work_item = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-pushed-ownership-assumption-restore-integration"
        ),
        None,
    )
    if not isinstance(push_process_restore_work_item, dict):
        raise AssertionError("pushed process restore work-item plan row is absent")
    if (
        query_rti_work.source_location_text(push_process_restore_work_item)
        != "cpp/tests/ieee1516_2025_process_pushed_ownership_assumption_save_restore_catch2.cpp:62"
        or push_process_restore_work_item.get("assertions") != 55
        or push_process_restore_work_item.get("status")
        != "implemented-in-noninstallable-development-profile"
        or len(push_process_restore_work_item.get("lab_requirement_ids", [])) != 10
        or len(push_process_restore_work_item.get("standard_sections", [])) != 7
        or len(push_process_restore_work_item.get("selected_cpp_api_surface_ids", []))
        != 18
    ):
        raise AssertionError(
            "pushed process restore work-item source/evidence mapping drifted"
        )
    push_process_restore_work_item_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-federation-restore-work-item-ownership-assumption-push",
        limit=0,
    )
    if (
        push_process_restore_work_item_focus.get("lane_state") != "complete"
        or push_process_restore_work_item_focus.get("mapped_test_count") != 1
        or push_process_restore_work_item_focus.get("assertion_count") != 55
        or push_process_restore_work_item_focus.get("requirement_count") != 10
        or push_process_restore_work_item_focus.get("standard_section_count") != 7
        or push_process_restore_work_item_focus.get("requirement_section_pair_count")
        != 10
    ):
        raise AssertionError("pushed process restore work-item lane card drifted")
    push_process_restore_work_item_handles = push_process_restore_work_item_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(push_process_restore_work_item_handles, dict)
        or push_process_restore_work_item_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
        or "RTIambassadors preserve pushed ownership-assumption delivery across save and restore"
        not in str(push_process_restore_work_item_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("pushed process restore work-item CTest handle drifted")
    process_restore_work_item = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-restored-ownership-assumption-integration"
        ),
        None,
    )
    if not isinstance(process_restore_work_item, dict):
        raise AssertionError("public process restore work-item plan row is absent")
    if (
        query_rti_work.source_location_text(process_restore_work_item)
        != "cpp/tests/ieee1516_2025_process_restored_ownership_assumption_catch2.cpp:62"
        or process_restore_work_item.get("assertions") != 46
        or process_restore_work_item.get("status")
        != "implemented-in-noninstallable-development-profile"
        or len(process_restore_work_item.get("lab_requirement_ids", [])) != 10
        or len(process_restore_work_item.get("standard_sections", [])) != 7
        or len(process_restore_work_item.get("selected_cpp_api_surface_ids", [])) != 18
    ):
        raise AssertionError(
            "public process restore work-item source/evidence mapping drifted"
        )
    private_process_restore_work_item = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-save-commit-process-local-ownership-assumption-work-unit"
        ),
        None,
    )
    if not isinstance(private_process_restore_work_item, dict) or (
        query_rti_work.source_location_text(private_process_restore_work_item)
        != "cpp/tests/ieee1516_2025_federation_registry_process_local_ownership_assumption_work_item_restore_catch2.cpp:3"
        or private_process_restore_work_item.get("assertions") != 35
    ):
        raise AssertionError("private process restore work-item seam drifted")
    process_restore_work_item_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-federation-restore-work-item-ownership-assumption",
        limit=0,
    )
    if (
        process_restore_work_item_focus.get("lane_state") != "complete"
        or process_restore_work_item_focus.get("mapped_test_count") != 2
        or process_restore_work_item_focus.get("assertion_count") != 81
        or process_restore_work_item_focus.get("requirement_count") != 12
        or process_restore_work_item_focus.get("standard_section_count") != 9
        or process_restore_work_item_focus.get("requirement_section_pair_count") != 12
    ):
        raise AssertionError("process restore work-item lane card drifted")
    process_restore_work_item_handles = process_restore_work_item_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_restore_work_item_handles, dict)
        or process_restore_work_item_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_catch2"
        or "RTIambassadors deliver restored ownership-assumption work through a configured process endpoint"
        not in str(process_restore_work_item_handles.get("ctest_filter") or "")
        or process_restore_work_item_handles.get("private_foundation_ctest_target")
        != "umbra_federation_registry_catch2"
        or "Process-local restore returns route-free ownership-assumption work"
        not in str(
            process_restore_work_item_handles.get("private_foundation_ctest_filter")
            or ""
        )
    ):
        raise AssertionError("process restore work-item CTest handle drifted")
    regional_callback_gating = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-regional-interaction-callback-gating"
        ),
        None,
    )
    if not isinstance(regional_callback_gating, dict):
        raise AssertionError("regional callback-gating plan row is absent")
    if (
        query_rti_work.source_location_text(regional_callback_gating)
        != "cpp/tests/ieee1516_2025_connection_process_tso_regional_interaction_arrival_catch2.cpp:6"
        or regional_callback_gating.get("assertions") != 69
        or regional_callback_gating.get("status")
        != "implemented-private-foundation-not-conformance-evidence"
        or len(regional_callback_gating.get("lab_requirement_ids", [])) != 28
        or len(regional_callback_gating.get("standard_sections", [])) != 16
        or len(regional_callback_gating.get("selected_cpp_api_surface_ids", [])) != 20
    ):
        raise AssertionError("regional callback-gating source/evidence mapping drifted")
    regional_callback_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-regional-interaction-callback-gating", limit=0
    )
    if (
        regional_callback_focus.get("lane_state") != "complete"
        or regional_callback_focus.get("mapped_test_count") != 2
        or regional_callback_focus.get("assertion_count") != 138
        or regional_callback_focus.get("recorded_assertion_count") != 138
        or regional_callback_focus.get("requirement_count") != 30
        or regional_callback_focus.get("standard_section_count") != 18
        or regional_callback_focus.get("requirement_section_pair_count") != 30
    ):
        raise AssertionError("regional callback-gating lane card drifted")
    regional_callback_handles = regional_callback_focus.get("lane_handles")
    if (
        not isinstance(regional_callback_handles, dict)
        or regional_callback_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or "callbacks are disabled"
        not in str(regional_callback_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("regional callback-gating CTest handle drifted")
    recent_views = query_rti_work.recent_completed_views(index, tests)
    if not recent_views or recent_views[0].get("plan_id") != latest.get("plan_id"):
        raise AssertionError("recent completed-slice ordering drifted")
    mixed_region = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mixed-region-interaction-validation-integration"
        ),
        None,
    )
    if not isinstance(mixed_region, dict):
        raise AssertionError(
            "mixed-dimensional regional interaction plan row is absent"
        )
    if (
        query_rti_work.source_location_text(mixed_region)
        != ("cpp/tests/mixed_region_interaction_validation_catch2.cpp:119")
        or mixed_region.get("assertions") != 37
    ):
        raise AssertionError(
            "mixed-dimensional regional interaction source/evidence drifted"
        )
    if mixed_region.get("traceability_state") != "requirements-mapped":
        raise AssertionError("mixed-dimensional regional interaction row is not mapped")
    mixed_region_focus = query_rti_work.focused_lane_result(
        index, tests, "mixed-region-interaction-validation", limit=0
    )
    if (
        mixed_region_focus.get("lane_state") != "complete"
        or mixed_region_focus.get("mapped_test_count") != 1
        or mixed_region_focus.get("assertion_count") != 37
        or mixed_region_focus.get("requirement_count") != 4
        or mixed_region_focus.get("standard_section_count") != 1
        or mixed_region_focus.get("requirement_section_pair_count") != 4
    ):
        raise AssertionError("mixed-dimensional regional interaction lane card drifted")
    mixed_region_handles = mixed_region_focus.get("lane_handles")
    if (
        not isinstance(mixed_region_handles, dict)
        or mixed_region_handles.get("catch2_target")
        != "umbra_mixed_region_interaction_validation_catch2"
    ):
        raise AssertionError(
            "mixed-dimensional regional interaction target handle drifted"
        )
    mixed_region_case = query_rti_work.case_card_record(
        mixed_region, index, "umbra-cpp-mixed-region-interaction-validation-integration"
    )
    if mixed_region_case.get("lane") != "mixed-region-interaction-validation":
        raise AssertionError("exact case card did not resolve the mixed-region lane")
    if mixed_region_case.get("commands", {}).get("ctest") is None:
        raise AssertionError("exact case card omitted the mixed-region CTest handle")
    for command_key in ("matrix_requirement", "matrix_section"):
        if not mixed_region_case.get("commands", {}).get(command_key):
            raise AssertionError(
                f"exact case card omitted the mixed-region {command_key} handle"
            )

    whole_class_unsubscribe = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration"
        ),
        None,
    )
    if not isinstance(whole_class_unsubscribe, dict):
        raise AssertionError(
            "whole-class default-region unsubscription plan row is absent"
        )
    if (
        query_rti_work.source_location_text(whole_class_unsubscribe)
        != ("cpp/tests/default_region_interaction_routing_catch2.cpp:241")
        or whole_class_unsubscribe.get("assertions") != 20
    ):
        raise AssertionError("whole-class default-region source/evidence drifted")
    if whole_class_unsubscribe.get("traceability_state") != "requirements-mapped":
        raise AssertionError("whole-class default-region row is not mapped")
    if whole_class_unsubscribe.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("whole-class default-region section mapping drifted")
    if whole_class_unsubscribe.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l35-11",
    ]:
        raise AssertionError("whole-class default-region requirement mapping drifted")
    whole_class_unsubscribe_focus = query_rti_work.focused_lane_result(
        index, tests, "whole-class-unsubscribe", limit=0
    )
    if (
        whole_class_unsubscribe_focus.get("lane_state") != "complete"
        or whole_class_unsubscribe_focus.get("mapped_test_count") != 1
        or whole_class_unsubscribe_focus.get("assertion_count") != 20
        or whole_class_unsubscribe_focus.get("requirement_count") != 1
        or whole_class_unsubscribe_focus.get("standard_section_count") != 1
        or whole_class_unsubscribe_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("whole-class default-region lane card drifted")
    whole_class_unsubscribe_handles = whole_class_unsubscribe_focus.get("lane_handles")
    if (
        not isinstance(whole_class_unsubscribe_handles, dict)
        or whole_class_unsubscribe_handles.get("catch2_target")
        != "umbra_default_region_interaction_routing_catch2"
        or whole_class_unsubscribe_handles.get("ctest_label")
        != "whole-class-unsubscribe"
    ):
        raise AssertionError("whole-class default-region lane handles drifted")
    whole_class_unsubscribe_case = query_rti_work.case_card_record(
        whole_class_unsubscribe,
        index,
        "umbra-cpp-whole-class-interaction-unsubscribe-default-region-integration",
    )
    if (
        whole_class_unsubscribe_case.get("traceability_state") != "requirements-mapped"
        or whole_class_unsubscribe_case.get("lane") != "whole-class-unsubscribe"
        or "ctest" not in whole_class_unsubscribe_case.get("commands", {})
    ):
        raise AssertionError("whole-class default-region card lost its handles")

    no_common_dimension = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-no-common-dimension-integration"
        ),
        None,
    )
    if not isinstance(no_common_dimension, dict):
        raise AssertionError("no-common-dimension regional object plan row is absent")
    if (
        query_rti_work.source_location_text(no_common_dimension)
        != ("cpp/tests/regional_object_attribute_routing_catch2.cpp:486")
        or no_common_dimension.get("assertions") != 45
    ):
        raise AssertionError("no-common-dimension source/evidence drifted")
    if no_common_dimension.get("traceability_state") != "requirements-mapped":
        raise AssertionError("no-common-dimension row is not mapped")
    if no_common_dimension.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("no-common-dimension section mapping drifted")
    if no_common_dimension.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l68-21",
    ]:
        raise AssertionError("no-common-dimension requirement mapping drifted")
    no_common_dimension_focus = query_rti_work.focused_lane_result(
        index, tests, "no-common-dimension", limit=0
    )
    if (
        no_common_dimension_focus.get("lane_state") != "complete"
        or no_common_dimension_focus.get("mapped_test_count") != 1
        or no_common_dimension_focus.get("assertion_count") != 45
        or no_common_dimension_focus.get("requirement_count") != 1
        or no_common_dimension_focus.get("standard_section_count") != 1
        or no_common_dimension_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("no-common-dimension lane card drifted")
    no_common_dimension_handles = no_common_dimension_focus.get("lane_handles")
    if (
        not isinstance(no_common_dimension_handles, dict)
        or no_common_dimension_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
        or no_common_dimension_handles.get("ctest_label") != "no-common-dimension"
    ):
        raise AssertionError("no-common-dimension lane handles drifted")
    no_common_dimension_case = query_rti_work.case_card_record(
        no_common_dimension,
        index,
        "umbra-cpp-regional-object-attribute-no-common-dimension-integration",
    )
    if (
        no_common_dimension_case.get("traceability_state") != "requirements-mapped"
        or no_common_dimension_case.get("lane") != "no-common-dimension"
        or "ctest" not in no_common_dimension_case.get("commands", {})
    ):
        raise AssertionError("no-common-dimension card lost its handles")

    subscription_isolation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-object-attribute-subscription-isolation-integration"
        ),
        None,
    )
    if not isinstance(subscription_isolation, dict):
        raise AssertionError("regional subscription-isolation plan row is absent")
    if (
        query_rti_work.source_location_text(subscription_isolation)
        != ("cpp/tests/regional_object_attribute_routing_catch2.cpp:343")
        or subscription_isolation.get("assertions") != 40
    ):
        raise AssertionError("regional subscription-isolation source/evidence drifted")
    if subscription_isolation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional subscription-isolation row is not mapped")
    if subscription_isolation.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.8",
    ]:
        raise AssertionError("regional subscription-isolation section mapping drifted")
    if subscription_isolation.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-233-l57-16",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-233-l63-18",
    ]:
        raise AssertionError(
            "regional subscription-isolation requirement mapping drifted"
        )
    subscription_isolation_focus = query_rti_work.focused_lane_result(
        index, tests, "regional-object-attribute-subscription-isolation", limit=0
    )
    if (
        subscription_isolation_focus.get("lane_state") != "complete"
        or subscription_isolation_focus.get("roadmap_owner") != "object-ddm-ownership"
        or subscription_isolation_focus.get("mapped_test_count") != 1
        or subscription_isolation_focus.get("assertion_count") != 40
        or subscription_isolation_focus.get("requirement_count") != 2
        or subscription_isolation_focus.get("standard_section_count") != 1
        or subscription_isolation_focus.get("requirement_section_pair_count") != 2
    ):
        raise AssertionError("regional subscription-isolation lane card drifted")
    subscription_isolation_handles = subscription_isolation_focus.get("lane_handles")
    if (
        not isinstance(subscription_isolation_handles, dict)
        or subscription_isolation_handles.get("catch2_target")
        != "umbra_regional_object_attribute_routing_catch2"
        or subscription_isolation_handles.get("ctest_label")
        != "regional-object-attribute-subscription-isolation"
        or "Embedded regional object-attribute subscriptions remain independent from ordinary declarations"
        not in str(subscription_isolation_handles.get("ctest_filter") or "")
    ):
        raise AssertionError("regional subscription-isolation lane handles drifted")
    subscription_isolation_case = query_rti_work.case_card_record(
        subscription_isolation,
        index,
        "umbra-cpp-regional-object-attribute-subscription-isolation-integration",
    )
    if (
        subscription_isolation_case.get("traceability_state") != "requirements-mapped"
        or subscription_isolation_case.get("lane")
        != "regional-object-attribute-subscription-isolation"
        or "ctest" not in subscription_isolation_case.get("commands", {})
    ):
        raise AssertionError("regional subscription-isolation card lost its handles")

    time_axis_independence = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timestamped-regional-attribute-update-tar-nmr-integration"
        ),
        None,
    )
    if not isinstance(time_axis_independence, dict):
        raise AssertionError("time-axis-independence plan row is absent")
    if (
        query_rti_work.source_location_text(time_axis_independence)
        != ("cpp/tests/timestamped_regional_attribute_update_tar_nmr_catch2.cpp:151")
        or time_axis_independence.get("assertions") != 94
    ):
        raise AssertionError("time-axis-independence source/evidence drifted")
    if time_axis_independence.get("traceability_state") != "requirements-mapped":
        raise AssertionError("time-axis-independence row is not mapped")
    if set(time_axis_independence.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-06-object-management-page-120-l83-22",
        "requirement-candidate-content-clauses-06-object-management-page-121-l36-10",
        "requirement-candidate-content-clauses-06-object-management-page-122-l85-25",
        "requirement-candidate-content-clauses-06-object-management-page-122-l103-31",
        "requirement-candidate-content-clauses-08-time-management-page-185-l17-5",
        "requirement-candidate-content-clauses-08-time-management-page-194-l21-3",
        "requirement-candidate-content-clauses-08-time-management-page-194-l69-19",
        "requirement-candidate-content-clauses-08-time-management-page-197-l20-3",
        "requirement-candidate-content-clauses-08-time-management-page-197-l50-13",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l89-28",
    }:
        raise AssertionError("time-axis-independence requirement mapping drifted")
    if set(time_axis_independence.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.10.5",
        "hla-1516.1-2025:clause-6.11.1",
        "hla-1516.1-2025:clause-8.1.6",
        "hla-1516.1-2025:clause-8.8",
        "hla-1516.1-2025:clause-8.10.2",
        "hla-1516.1-2025:clause-9.1.4",
    }:
        raise AssertionError("time-axis-independence section mapping drifted")
    time_axis_focus = query_rti_work.focused_lane_result(
        index, tests, "time-axis-independence", limit=0
    )
    if (
        time_axis_focus.get("lane_state") != "complete"
        or time_axis_focus.get("mapped_test_count") != 1
        or time_axis_focus.get("assertion_count") != 94
        or time_axis_focus.get("requirement_count") != 10
        or time_axis_focus.get("standard_section_count") != 7
        or time_axis_focus.get("requirement_section_pair_count") != 10
    ):
        raise AssertionError("time-axis-independence lane card drifted")
    time_axis_handles = time_axis_focus.get("lane_handles")
    if (
        not isinstance(time_axis_handles, dict)
        or time_axis_handles.get("catch2_target")
        != "umbra_timestamped_regional_attribute_update_tar_nmr_catch2"
        or time_axis_handles.get("ctest_label") != "time-axis-independence"
    ):
        raise AssertionError("time-axis-independence lane handles drifted")
    time_axis_case = query_rti_work.case_card_record(
        time_axis_independence,
        index,
        "umbra-cpp-timestamped-regional-attribute-update-tar-nmr-integration",
    )
    if (
        time_axis_case.get("traceability_state") != "requirements-mapped"
        or time_axis_case.get("lane") != "time-axis-independence"
        or "ctest" not in time_axis_case.get("commands", {})
    ):
        raise AssertionError("time-axis-independence card lost its handles")

    object_name_reservation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-object-instance-name-reservation-integration"
        ),
        None,
    )
    if not isinstance(object_name_reservation, dict):
        raise AssertionError("object-instance-name-reservation plan row is absent")
    if (
        query_rti_work.source_location_text(object_name_reservation)
        != ("cpp/tests/object_instance_name_reservation_catch2.cpp:95")
        or object_name_reservation.get("assertions") != 68
    ):
        raise AssertionError("object-instance-name-reservation source/evidence drifted")
    if object_name_reservation.get("traceability_state") != "requirements-mapped":
        raise AssertionError("object-instance-name-reservation row is not mapped")
    if set(object_name_reservation.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-06-object-management-page-113-l153-40",
        "requirement-candidate-content-clauses-06-object-management-page-113-l21-3",
        "requirement-candidate-content-clauses-06-object-management-page-113-l24-4",
        "requirement-candidate-content-clauses-06-object-management-page-114-l110-26",
        "requirement-candidate-content-clauses-06-object-management-page-114-l119-29",
        "requirement-candidate-content-clauses-06-object-management-page-115-l101-27",
        "requirement-candidate-content-clauses-06-object-management-page-115-l104-28",
        "requirement-candidate-content-clauses-06-object-management-page-115-l107-29",
        "requirement-candidate-content-clauses-06-object-management-page-115-l110-30",
        "requirement-candidate-content-clauses-06-object-management-page-117-l36-6",
        "requirement-candidate-content-clauses-06-object-management-page-117-l45-9",
        "requirement-candidate-content-clauses-06-object-management-page-117-l48-10",
    }:
        raise AssertionError(
            "object-instance-name-reservation requirement mapping drifted"
        )
    if set(object_name_reservation.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-6.2",
        "hla-1516.1-2025:clause-6.3",
        "hla-1516.1-2025:clause-6.4.3",
        "hla-1516.1-2025:clause-6.5.3",
        "hla-1516.1-2025:clause-6.7.5",
    }:
        raise AssertionError("object-instance-name-reservation section mapping drifted")
    object_name_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-name-reservation", limit=0
    )
    if (
        object_name_focus.get("lane_state") != "complete"
        or object_name_focus.get("mapped_test_count") != 1
        or object_name_focus.get("assertion_count") != 68
        or object_name_focus.get("requirement_count") != 12
        or object_name_focus.get("standard_section_count") != 5
        or object_name_focus.get("requirement_section_pair_count") != 12
    ):
        raise AssertionError("object-instance-name-reservation lane card drifted")
    object_name_handles = object_name_focus.get("lane_handles")
    if (
        not isinstance(object_name_handles, dict)
        or object_name_handles.get("catch2_target")
        != "umbra_object_instance_name_reservation_catch2"
        or object_name_handles.get("ctest_label") != "object-instance-name-reservation"
    ):
        raise AssertionError("object-instance-name-reservation lane handles drifted")
    object_name_case = query_rti_work.case_card_record(
        object_name_reservation,
        index,
        "umbra-cpp-object-instance-name-reservation-integration",
    )
    if (
        object_name_case.get("traceability_state") != "requirements-mapped"
        or object_name_case.get("lane") != "object-instance-name-reservation"
        or "ctest" not in object_name_case.get("commands", {})
    ):
        raise AssertionError("object-instance-name-reservation card lost its handles")

    strict_relaxed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-public-regional-automatic-provision-relaxed-ddm-integration"
        ),
        None,
    )
    if not isinstance(strict_relaxed, dict):
        raise AssertionError("strict-relaxed-ddm-boundary plan row is absent")
    if (
        query_rti_work.source_location_text(strict_relaxed)
        != ("cpp/tests/regional_auto_provide_response_catch2.cpp:148")
        or strict_relaxed.get("assertions") != 240
    ):
        raise AssertionError("strict-relaxed-ddm-boundary source/evidence drifted")
    if strict_relaxed.get("traceability_state") != "requirements-mapped":
        raise AssertionError("strict-relaxed-ddm-boundary row is not mapped")
    if set(strict_relaxed.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-01-overview-page-019-l62-7",
        "requirement-candidate-content-clauses-04-federation-management-page-076-l18-2",
        "requirement-candidate-content-clauses-06-object-management-page-110-l16-4",
        "requirement-candidate-content-clauses-06-object-management-page-110-l34-10",
        "requirement-candidate-content-clauses-06-object-management-page-120-l86-23",
        "requirement-candidate-content-clauses-06-object-management-page-120-l110-31",
        "requirement-candidate-content-clauses-06-object-management-page-121-l36-10",
        "requirement-candidate-content-clauses-06-object-management-page-122-l103-31",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l71-22",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l88-26",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l100-30",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-241-l145-45",
        "requirement-source-refinement-relaxed-ddm-overlap",
        "requirement-source-refinement-relaxed-ddm-delivery",
        "requirement-candidate-content-clauses-10-support-services-page-282-l152-47",
    }:
        raise AssertionError("strict-relaxed-ddm-boundary requirement mapping drifted")
    if set(strict_relaxed.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-1",
        "hla-1516.1-2025:clause-4.32",
        "hla-1516.1-2025:clause-6.1.10",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.10.5",
        "hla-1516.1-2025:clause-6.11.1",
        "hla-1516.1-2025:clause-9.1.4",
        "hla-1516.1-2025:clause-9.1.8",
        "hla-1516.1-2025:clause-9.13.1",
        "hla-1516.1-2025:clause-10.55.1",
    }:
        raise AssertionError("strict-relaxed-ddm-boundary section mapping drifted")
    strict_relaxed_focus = query_rti_work.focused_lane_result(
        index, tests, "strict-relaxed-ddm-boundary", limit=0
    )
    if (
        strict_relaxed_focus.get("lane_state") != "complete"
        or strict_relaxed_focus.get("mapped_test_count") != 1
        or strict_relaxed_focus.get("assertion_count") != 240
        or strict_relaxed_focus.get("requirement_count") != 15
        or strict_relaxed_focus.get("standard_section_count") != 10
        or strict_relaxed_focus.get("requirement_section_pair_count") != 15
    ):
        raise AssertionError("strict-relaxed-ddm-boundary lane card drifted")
    strict_relaxed_handles = strict_relaxed_focus.get("lane_handles")
    if (
        not isinstance(strict_relaxed_handles, dict)
        or strict_relaxed_handles.get("catch2_target")
        != "umbra_regional_auto_provide_response_catch2"
        or strict_relaxed_handles.get("ctest_label") != "strict-relaxed-ddm-boundary"
    ):
        raise AssertionError("strict-relaxed-ddm-boundary lane handles drifted")
    strict_relaxed_case = query_rti_work.case_card_record(
        strict_relaxed,
        index,
        "umbra-cpp-public-regional-automatic-provision-relaxed-ddm-integration",
    )
    if (
        strict_relaxed_case.get("traceability_state") != "requirements-mapped"
        or strict_relaxed_case.get("lane") != "strict-relaxed-ddm-boundary"
        or "ctest" not in strict_relaxed_case.get("commands", {})
    ):
        raise AssertionError("strict-relaxed-ddm-boundary card lost its handles")

    subscription_dimension = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-interaction-subscription-dimension-validation-integration"
        ),
        None,
    )
    if not isinstance(subscription_dimension, dict):
        raise AssertionError("regional subscription-dimension plan row is absent")
    if (
        query_rti_work.source_location_text(subscription_dimension)
        != ("cpp/tests/mixed_region_interaction_validation_catch2.cpp:228")
        or subscription_dimension.get("assertions") != 36
    ):
        raise AssertionError("regional subscription-dimension source/evidence drifted")
    if subscription_dimension.get("traceability_state") != "requirements-mapped":
        raise AssertionError("regional subscription-dimension row is not mapped")
    if subscription_dimension.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("regional subscription-dimension section mapping drifted")
    if subscription_dimension.get("lab_requirement_ids") != [
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l20-6",
    ]:
        raise AssertionError(
            "regional subscription-dimension requirement mapping drifted"
        )
    subscription_dimension_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-dimension-validation", limit=0
    )
    if (
        subscription_dimension_focus.get("lane_state") != "complete"
        or subscription_dimension_focus.get("mapped_test_count") != 1
        or subscription_dimension_focus.get("assertion_count") != 36
        or subscription_dimension_focus.get("requirement_count") != 1
        or subscription_dimension_focus.get("standard_section_count") != 1
        or subscription_dimension_focus.get("requirement_section_pair_count") != 1
    ):
        raise AssertionError("regional subscription-dimension lane card drifted")
    subscription_dimension_handles = subscription_dimension_focus.get("lane_handles")
    if (
        not isinstance(subscription_dimension_handles, dict)
        or subscription_dimension_handles.get("catch2_target")
        != "umbra_mixed_region_interaction_validation_catch2"
        or subscription_dimension_handles.get("ctest_label")
        != "subscription-dimension-validation"
    ):
        raise AssertionError("regional subscription-dimension lane handles drifted")
    subscription_dimension_case = query_rti_work.case_card_record(
        subscription_dimension,
        index,
        "umbra-cpp-regional-interaction-subscription-dimension-validation-integration",
    )
    if (
        subscription_dimension_case.get("traceability_state") != "requirements-mapped"
        or subscription_dimension_case.get("lane")
        != "subscription-dimension-validation"
        or "ctest" not in subscription_dimension_case.get("commands", {})
    ):
        raise AssertionError("regional subscription-dimension card lost its handles")

    subscription_empty = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-interaction-subscription-empty-set-integration"
        ),
        None,
    )
    if not isinstance(subscription_empty, dict):
        raise AssertionError("empty regional subscription-set plan row is absent")
    if (
        query_rti_work.source_location_text(subscription_empty)
        != ("cpp/tests/multi_region_interaction_routing_catch2.cpp:307")
        or subscription_empty.get("assertions") != 23
    ):
        raise AssertionError("empty regional subscription-set source/evidence drifted")
    if subscription_empty.get("traceability_state") != "requirements-mapped":
        raise AssertionError("empty regional subscription-set row is not mapped")
    if subscription_empty.get("standard_sections") != [
        "hla-1516.1-2025:clause-9.1.4",
    ]:
        raise AssertionError("empty regional subscription-set section mapping drifted")
    if set(subscription_empty.get("lab_requirement_ids", [])) != {
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l14-4",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l8-2",
        "requirement-candidate-content-clauses-09-data-distribution-management-page-221-l41-13",
    }:
        raise AssertionError(
            "empty regional subscription-set requirement mapping drifted"
        )
    subscription_empty_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-empty-set", limit=0
    )
    if (
        subscription_empty_focus.get("lane_state") != "complete"
        or subscription_empty_focus.get("mapped_test_count") != 1
        or subscription_empty_focus.get("assertion_count") != 23
        or subscription_empty_focus.get("requirement_count") != 3
        or subscription_empty_focus.get("standard_section_count") != 1
        or subscription_empty_focus.get("requirement_section_pair_count") != 3
    ):
        raise AssertionError("empty regional subscription-set lane card drifted")
    subscription_empty_handles = subscription_empty_focus.get("lane_handles")
    if (
        not isinstance(subscription_empty_handles, dict)
        or subscription_empty_handles.get("catch2_target")
        != "umbra_multi_region_interaction_routing_catch2"
        or subscription_empty_handles.get("ctest_label") != "subscription-empty-set"
    ):
        raise AssertionError("empty regional subscription-set lane handles drifted")
    subscription_empty_case = query_rti_work.case_card_record(
        subscription_empty,
        index,
        "umbra-cpp-regional-interaction-subscription-empty-set-integration",
    )
    if (
        subscription_empty_case.get("traceability_state") != "requirements-mapped"
        or subscription_empty_case.get("lane") != "subscription-empty-set"
        or "ctest" not in subscription_empty_case.get("commands", {})
    ):
        raise AssertionError(
            "exact empty regional subscription-set card lost its handles"
        )

    passive_object = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-passive-object-attribute-subscription-integration"
        ),
        None,
    )
    if not isinstance(passive_object, dict):
        raise AssertionError("passive regional object-attribute plan row is absent")
    if (
        query_rti_work.source_location_text(passive_object)
        != "cpp/tests/passive_object_attribute_subscription_catch2.cpp:120"
        or passive_object.get("assertions") != 66
        or passive_object.get("traceability_state") != "requirements-mapped"
        or passive_object.get("standard_sections")
        != [
            "hla-1516.1-2025:clause-5.8",
            "hla-1516.1-2025:clause-9.8",
            "hla-1516.1-2025:clause-9.8.4",
        ]
        or set(passive_object.get("lab_requirement_ids", []))
        != {
            "requirement-candidate-content-clauses-05-declaration-management-page-096-l105-32",
            "requirement-candidate-content-clauses-05-declaration-management-page-096-l117-36",
            "requirement-candidate-content-clauses-05-declaration-management-page-096-l123-38",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-233-l30-7",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-233-l36-9",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l14-3",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l26-7",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l38-11",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l44-13",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l56-17",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l59-18",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l62-19",
            "requirement-candidate-content-clauses-09-data-distribution-management-page-234-l161-49",
        }
    ):
        raise AssertionError("passive regional object-attribute mapping drifted")
    if len(passive_object.get("selected_cpp_api_surface_ids", [])) != 10:
        raise AssertionError("passive regional object-attribute API mapping drifted")
    passive_object_focus = query_rti_work.focused_lane_result(
        index, tests, "passive-regional-object-attribute-transition", limit=0
    )
    if (
        passive_object_focus.get("lane_state") != "complete"
        or passive_object_focus.get("mapped_test_count") != 1
        or passive_object_focus.get("assertion_count") != 66
        or passive_object_focus.get("requirement_count") != 13
        or passive_object_focus.get("standard_section_count") != 3
        or passive_object_focus.get("requirement_section_pair_count") != 13
    ):
        raise AssertionError("passive regional object-attribute lane card drifted")
    passive_object_handles = passive_object_focus.get("lane_handles")
    if (
        not isinstance(passive_object_handles, dict)
        or passive_object_handles.get("catch2_target")
        != "umbra_passive_object_attribute_subscription_catch2"
        or passive_object_handles.get("ctest_label")
        != "passive-regional-object-attribute-transition"
    ):
        raise AssertionError("passive regional object-attribute lane handles drifted")
    passive_object_case = query_rti_work.case_card_record(
        passive_object,
        index,
        "umbra-cpp-passive-object-attribute-subscription-integration",
    )
    if (
        passive_object_case.get("traceability_state") != "requirements-mapped"
        or passive_object_case.get("lane")
        != "passive-regional-object-attribute-transition"
        or "ctest" not in passive_object_case.get("commands", {})
    ):
        raise AssertionError("passive regional object-attribute case lost its handles")

    return status
