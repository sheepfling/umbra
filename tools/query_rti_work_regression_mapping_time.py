# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import MappingLookupInputs


def run_time_mapping_rows(inputs: MappingLookupInputs) -> None:
    index = inputs.index
    tests = inputs.tests
    timestamped_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(timestamped_regional, dict):
        raise AssertionError("focused timestamped regional process row is absent")
    if query_rti_work.source_location_text(timestamped_regional) != (
        "cpp/tests/ieee1516_2025_connection_tso_regional_interaction_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused timestamped regional process source pointer drifted"
        )
    if timestamped_regional.get("assertions") != 71:
        raise AssertionError(
            "focused timestamped regional process assertion count drifted"
        )
    if timestamped_regional.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused timestamped regional process row is not mapped")
    timestamped_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-process-regional-interaction", limit=0
    )
    if timestamped_regional_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused timestamped regional process lane is not complete"
        )
    if timestamped_regional_focus.get("mapped_test_count") != 6:
        raise AssertionError(
            "focused timestamped regional process mapped count drifted"
        )
    if timestamped_regional_focus.get("assertion_count") != 71:
        raise AssertionError(
            "focused timestamped regional process lane assertion total drifted"
        )
    if timestamped_regional_focus.get("recorded_assertion_count") != 453:
        raise AssertionError(
            "focused timestamped regional process recorded assertion total drifted"
        )
    if timestamped_regional_focus.get("requirement_count") != 32:
        raise AssertionError(
            "focused timestamped regional process requirement total drifted"
        )
    if timestamped_regional_focus.get("standard_section_count") != 19:
        raise AssertionError(
            "focused timestamped regional process section total drifted"
        )
    if timestamped_regional_focus.get("requirement_section_pair_count") != 32:
        raise AssertionError("focused timestamped regional process direct pair drifted")
    timestamped_regional_handles = timestamped_regional_focus.get("lane_handles")
    if (
        not isinstance(timestamped_regional_handles, dict)
        or timestamped_regional_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused timestamped regional process target handle drifted"
        )
    timestamped_regional_links = query_rti_work.roadmap_links_for_test(
        index, timestamped_regional, limit=0
    )
    if (
        not timestamped_regional_links
        or timestamped_regional_links[0].get("id") != ("transport-and-conformance")
        or timestamped_regional_links[0].get("match") != "lane_owner"
    ):
        raise AssertionError(
            "timestamped regional process trace did not prioritize its owning roadmap family"
        )
    multi_recipient_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-multi-recipient-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(multi_recipient_regional, dict):
        raise AssertionError("focused process multi-recipient regional row is absent")
    if query_rti_work.source_location_text(multi_recipient_regional) != (
        "cpp/tests/ieee1516_2025_connection_multi_recipient_regional_interaction_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process multi-recipient regional source pointer drifted"
        )
    if multi_recipient_regional.get("assertions") != 268:
        raise AssertionError(
            "focused process multi-recipient regional assertion count drifted"
        )
    if multi_recipient_regional.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process multi-recipient regional row is not mapped"
        )
    if (
        "requirement-candidate-content-clauses-09-data-distribution-management-page-225-l27-7"
        not in multi_recipient_regional.get("lab_requirement_ids", [])
    ):
        raise AssertionError(
            "process regional-interaction lane lost its FDD switch-initialization requirement"
        )
    multi_recipient_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "process-multi-recipient-regional-interaction", limit=0
    )
    if multi_recipient_regional_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process multi-recipient regional lane is not complete"
        )
    if multi_recipient_regional_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process multi-recipient regional mapped count drifted"
        )
    if multi_recipient_regional_focus.get("assertion_count") != 268:
        raise AssertionError(
            "focused process multi-recipient regional lane assertion total drifted"
        )
    if multi_recipient_regional_focus.get("requirement_count") != 15:
        raise AssertionError(
            "focused process multi-recipient regional requirement total drifted"
        )
    if multi_recipient_regional_focus.get("standard_section_count") != 8:
        raise AssertionError(
            "focused process multi-recipient regional section total drifted"
        )
    if multi_recipient_regional_focus.get("requirement_section_pair_count") != 15:
        raise AssertionError(
            "focused process multi-recipient regional direct pair drifted"
        )
    multi_recipient_regional_handles = multi_recipient_regional_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(multi_recipient_regional_handles, dict)
        or multi_recipient_regional_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process multi-recipient regional target handle drifted"
        )
    regional_trace_links = query_rti_work.roadmap_links_for_test(
        index, multi_recipient_regional, limit=0
    )
    if (
        not regional_trace_links
        or regional_trace_links[0].get("id") != ("transport-and-conformance")
        or regional_trace_links[0].get("match") != "lane_owner"
    ):
        raise AssertionError(
            "regional process trace did not prioritize its owning roadmap family"
        )
    multi_recipient_ordering = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-multi-recipient-interaction-ordering-integration"
        ),
        None,
    )
    if not isinstance(multi_recipient_ordering, dict):
        raise AssertionError("focused process multi-recipient ordering row is absent")
    if query_rti_work.source_location_text(multi_recipient_ordering) != (
        "cpp/tests/ieee1516_2025_connection_multi_recipient_interaction_ordering_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process multi-recipient ordering source pointer drifted"
        )
    if multi_recipient_ordering.get("assertions") != 79:
        raise AssertionError(
            "focused process multi-recipient ordering assertion count drifted"
        )
    if multi_recipient_ordering.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process multi-recipient ordering row is not mapped"
        )
    multi_recipient_ordering_focus = query_rti_work.focused_lane_result(
        index, tests, "process-multi-recipient-callback-ordering", limit=0
    )
    if multi_recipient_ordering_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process multi-recipient ordering lane is not complete"
        )
    if multi_recipient_ordering_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process multi-recipient ordering mapped count drifted"
        )
    if multi_recipient_ordering_focus.get("assertion_count") != 79:
        raise AssertionError(
            "focused process multi-recipient ordering lane assertion total drifted"
        )
    if multi_recipient_ordering_focus.get("requirement_count") != 10:
        raise AssertionError(
            "focused process multi-recipient ordering requirement total drifted"
        )
    if multi_recipient_ordering_focus.get("standard_section_count") != 5:
        raise AssertionError(
            "focused process multi-recipient ordering section total drifted"
        )
    if multi_recipient_ordering_focus.get("requirement_section_pair_count") != 10:
        raise AssertionError(
            "focused process multi-recipient ordering direct pair drifted"
        )
    multi_recipient_ordering_handles = multi_recipient_ordering_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(multi_recipient_ordering_handles, dict)
        or multi_recipient_ordering_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process multi-recipient ordering target handle drifted"
        )
    available_dimensions_hierarchy = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-available-dimensions-hierarchy-integration-recovered-source-mapping"
        ),
        None,
    )
    if not isinstance(available_dimensions_hierarchy, dict):
        raise AssertionError(
            "focused process available-dimensions hierarchy row is absent"
        )
    if query_rti_work.source_location_text(available_dimensions_hierarchy) != (
        "cpp/tests/ieee1516_2025_connection_available_dimensions_hierarchy_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process available-dimensions hierarchy source pointer drifted"
        )
    if available_dimensions_hierarchy.get("assertions") != 28:
        raise AssertionError(
            "focused process available-dimensions hierarchy assertion count drifted"
        )
    if (
        available_dimensions_hierarchy.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused process available-dimensions hierarchy row is not mapped"
        )
    available_dimensions_hierarchy_focus = query_rti_work.focused_lane_result(
        index, tests, "process-available-dimensions-hierarchy", limit=0
    )
    if available_dimensions_hierarchy_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process available-dimensions hierarchy lane is not complete"
        )
    if available_dimensions_hierarchy_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process available-dimensions hierarchy mapped count drifted"
        )
    if available_dimensions_hierarchy_focus.get("assertion_count") != 28:
        raise AssertionError(
            "focused process available-dimensions hierarchy lane assertion total drifted"
        )
    if available_dimensions_hierarchy_focus.get("requirement_count") != 4:
        raise AssertionError(
            "focused process available-dimensions hierarchy requirement total drifted"
        )
    if available_dimensions_hierarchy_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused process available-dimensions hierarchy section total drifted"
        )
    if available_dimensions_hierarchy_focus.get("requirement_section_pair_count") != 4:
        raise AssertionError(
            "focused process available-dimensions hierarchy direct pair drifted"
        )
    reverse_fom_lookup = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-process-fom-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_fom_lookup, dict):
        raise AssertionError("focused process reverse-FOM lookup row is absent")
    if query_rti_work.source_location_text(reverse_fom_lookup) != (
        "cpp/tests/ieee1516_2025_connection_reverse_fom_lookup_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process reverse-FOM lookup source pointer drifted"
        )
    if reverse_fom_lookup.get("assertions") != 20:
        raise AssertionError(
            "focused process reverse-FOM lookup assertion count drifted"
        )
    if reverse_fom_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process reverse-FOM lookup row is not mapped")
    reverse_attribute_parameter_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-attribute-parameter-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_attribute_parameter_lookup, dict):
        raise AssertionError(
            "focused process attribute/parameter reverse lookup row is absent"
        )
    if query_rti_work.source_location_text(reverse_attribute_parameter_lookup) != (
        "cpp/tests/ieee1516_2025_connection_attribute_parameter_lookup_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process attribute/parameter reverse lookup source pointer drifted"
        )
    if reverse_attribute_parameter_lookup.get("assertions") != 24:
        raise AssertionError(
            "focused process attribute/parameter reverse lookup assertion count drifted"
        )
    if (
        reverse_attribute_parameter_lookup.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused process attribute/parameter reverse lookup row is not mapped"
        )
    if set(reverse_attribute_parameter_lookup.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-10.9.1",
        "hla-1516.1-2025:clause-10.10.3",
        "hla-1516.1-2025:clause-10.16.1",
    }:
        raise AssertionError(
            "focused process attribute/parameter reverse lookup sections drifted"
        )
    reverse_dimension_transportation_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-dimension-transportation-name-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(reverse_dimension_transportation_lookup, dict):
        raise AssertionError(
            "focused process dimension/transportation reverse lookup row is absent"
        )
    if query_rti_work.source_location_text(reverse_dimension_transportation_lookup) != (
        "cpp/tests/ieee1516_2025_connection_dimension_transportation_lookup_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process dimension/transportation reverse lookup source pointer drifted"
        )
    if reverse_dimension_transportation_lookup.get("assertions") != 16:
        raise AssertionError(
            "focused process dimension/transportation reverse lookup assertion count drifted"
        )
    if (
        reverse_dimension_transportation_lookup.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused process dimension/transportation reverse lookup row is not mapped"
        )
    if set(reverse_dimension_transportation_lookup.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-9.1.2",
        "hla-1516.1-2025:clause-10.19",
        "hla-1516.1-2025:clause-10.20.4",
    }:
        raise AssertionError(
            "focused process dimension/transportation reverse lookup sections drifted"
        )
    reverse_fom_error_matrix = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-reverse-fom-error-matrix-integration"
        ),
        None,
    )
    if not isinstance(reverse_fom_error_matrix, dict):
        raise AssertionError("focused process reverse-FOM error matrix row is absent")
    if query_rti_work.source_location_text(reverse_fom_error_matrix) != (
        "cpp/tests/ieee1516_2025_connection_reverse_fom_error_matrix_catch2.cpp:459"
    ):
        raise AssertionError(
            "focused process reverse-FOM error matrix source pointer drifted"
        )
    if reverse_fom_error_matrix.get("assertions") != 16:
        raise AssertionError(
            "focused process reverse-FOM error matrix assertion count drifted"
        )
    if reverse_fom_error_matrix.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused process reverse-FOM error matrix row is not mapped"
        )
    if set(reverse_fom_error_matrix.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-9.1.2",
        "hla-1516.1-2025:clause-10.19",
        "hla-1516.1-2025:clause-10.20.4",
    }:
        raise AssertionError(
            "focused process reverse-FOM error matrix sections drifted"
        )
    reverse_fom_lookup_focus = query_rti_work.focused_lane_result(
        index, tests, "reverse-fom-lookup", limit=0
    )
    if reverse_fom_lookup_focus.get("lane_state") != "complete":
        raise AssertionError("focused process reverse-FOM lookup lane is not complete")
    if reverse_fom_lookup_focus.get("mapped_test_count") != 4:
        raise AssertionError("focused process reverse-FOM lookup mapped count drifted")
    if reverse_fom_lookup_focus.get("assertion_count") != 76:
        raise AssertionError(
            "focused process reverse-FOM lookup lane assertion total drifted"
        )
    if reverse_fom_lookup_focus.get("requirement_count") != 14:
        raise AssertionError(
            "focused process reverse-FOM lookup requirement total drifted"
        )
    if reverse_fom_lookup_focus.get("standard_section_count") != 10:
        raise AssertionError("focused process reverse-FOM lookup section total drifted")
    if reverse_fom_lookup_focus.get("requirement_section_pair_count") != 14:
        raise AssertionError("focused process reverse-FOM lookup direct pair drifted")
    reverse_fom_lookup_handles = reverse_fom_lookup_focus.get("lane_handles")
    if (
        not isinstance(reverse_fom_lookup_handles, dict)
        or reverse_fom_lookup_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("focused process reverse-FOM lookup target handle drifted")
    shorthand_reverse_fom_kind, shorthand_reverse_fom_matches = (
        query_rti_work.resolve_matrix_query(index, tests, "getObjectClassName")
    )
    if shorthand_reverse_fom_kind != "api" or not any(
        test.get("id") == reverse_fom_lookup.get("id")
        for test in shorthand_reverse_fom_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused reverse-FOM lookup row"
        )
    object_instance_lookup = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-object-instance-lookup-focused-integration"
        ),
        None,
    )
    if not isinstance(object_instance_lookup, dict):
        raise AssertionError("focused process object-instance lookup row is absent")
    if query_rti_work.source_location_text(object_instance_lookup) != (
        "cpp/tests/ieee1516_2025_connection_object_instance_lookup_catch2.cpp:458"
    ):
        raise AssertionError(
            "focused process object-instance lookup source pointer drifted"
        )
    if object_instance_lookup.get("assertions") != 18:
        raise AssertionError(
            "focused process object-instance lookup assertion count drifted"
        )
    if object_instance_lookup.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process object-instance lookup row is not mapped")
    object_instance_lookup_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-lookup", limit=0
    )
    if object_instance_lookup_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process object-instance lookup lane is not complete"
        )
    if object_instance_lookup_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process object-instance lookup mapped count drifted"
        )
    if object_instance_lookup_focus.get("assertion_count") != 18:
        raise AssertionError(
            "focused process object-instance lookup lane assertion total drifted"
        )
    if object_instance_lookup_focus.get("requirement_count") != 5:
        raise AssertionError(
            "focused process object-instance lookup requirement total drifted"
        )
    if object_instance_lookup_focus.get("standard_section_count") != 2:
        raise AssertionError(
            "focused process object-instance lookup section total drifted"
        )
    object_instance_lookup_handles = object_instance_lookup_focus.get("lane_handles")
    if (
        not isinstance(object_instance_lookup_handles, dict)
        or object_instance_lookup_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process object-instance lookup target handle drifted"
        )
    shorthand_object_instance_kind, shorthand_object_instance_matches = (
        query_rti_work.resolve_matrix_query(index, tests, "getObjectInstanceHandle")
    )
    if shorthand_object_instance_kind != "api" or not any(
        test.get("id") == object_instance_lookup.get("id")
        for test in shorthand_object_instance_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused object-instance lookup row"
        )
    process_next_message = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-process-time-advance-next-message-focused"
        ),
        None,
    )
    if not isinstance(process_next_message, dict):
        raise AssertionError("focused process NMR/NMRA row is absent")
    if query_rti_work.source_location_text(process_next_message) != (
        "cpp/tests/ieee1516_2025_connection_time_advance_next_message_catch2.cpp:458"
    ):
        raise AssertionError("focused process NMR/NMRA source pointer drifted")
    if process_next_message.get("assertions") != 25:
        raise AssertionError("focused process NMR/NMRA assertion count drifted")
    if process_next_message.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process NMR/NMRA row is not mapped")
    process_next_message_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-next-message-focused", limit=0
    )
    if process_next_message_focus.get("lane_state") != "complete":
        raise AssertionError("focused process NMR/NMRA lane is not complete")
    if process_next_message_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process NMR/NMRA mapped count drifted")
    if process_next_message_focus.get("assertion_count") != 25:
        raise AssertionError("focused process NMR/NMRA lane assertion total drifted")
    if process_next_message_focus.get("requirement_count") != 5:
        raise AssertionError("focused process NMR/NMRA requirement total drifted")
    if process_next_message_focus.get("standard_section_count") != 4:
        raise AssertionError("focused process NMR/NMRA section total drifted")
    process_next_message_handles = process_next_message_focus.get("lane_handles")
    if (
        not isinstance(process_next_message_handles, dict)
        or process_next_message_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("focused process NMR/NMRA target handle drifted")
    shorthand_api_kind, shorthand_api_matches = query_rti_work.resolve_matrix_query(
        index, tests, "next_message_request"
    )
    if shorthand_api_kind != "api" or not any(
        test.get("id") == process_next_message.get("id")
        for test in shorthand_api_matches
        if isinstance(test, dict)
    ):
        raise AssertionError(
            "matrix API method shorthand did not resolve the focused process NMR/NMRA row"
        )
    process_next_message_queued = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-next-message-queued-tso-focused-integration"
        ),
        None,
    )
    if not isinstance(process_next_message_queued, dict):
        raise AssertionError("focused process queued-TSO NMR/NMRA row is absent")
    if query_rti_work.source_location_text(process_next_message_queued) != (
        "cpp/tests/ieee1516_2025_connection_time_advance_next_message_queued_tso_catch2.cpp:458"
    ):
        raise AssertionError("focused process queued-TSO source pointer drifted")
    if process_next_message_queued.get("assertions") != 232:
        raise AssertionError("focused process queued-TSO assertion count drifted")
    if process_next_message_queued.get("callback_models") != [
        "HLA_EVOKED",
        "HLA_IMMEDIATE",
    ]:
        raise AssertionError(
            "focused process queued-TSO callback-model mapping drifted"
        )
    if process_next_message_queued.get("primary_lane") != (
        "process-time-advance-next-message-queued-focused"
    ):
        raise AssertionError("focused process queued-TSO primary lane drifted")
    if process_next_message_queued.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process queued-TSO row is not mapped")
    process_next_message_queued_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-next-message-queued-focused", limit=0
    )
    if process_next_message_queued_focus.get("lane_state") != "complete":
        raise AssertionError("focused process queued-TSO lane is not complete")
    if process_next_message_queued_focus.get("mapped_test_count") != 16:
        raise AssertionError("focused process queued-TSO mapped count drifted")
    if process_next_message_queued_focus.get("assertion_count") != 543:
        raise AssertionError("focused process queued-TSO lane assertion total drifted")
    if process_next_message_queued_focus.get("requirement_count") != 54:
        raise AssertionError("focused process queued-TSO requirement total drifted")
    if process_next_message_queued_focus.get("standard_section_count") != 32:
        raise AssertionError("focused process queued-TSO section total drifted")
    process_next_message_queued_handles = process_next_message_queued_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_next_message_queued_handles, dict)
        or process_next_message_queued_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("focused process queued-TSO target handle drifted")
    process_time_advance_available = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-time-advance-available-focused-integration"
        ),
        None,
    )
    if not isinstance(process_time_advance_available, dict):
        raise AssertionError("focused process Available-form row is absent")
    if query_rti_work.source_location_text(process_time_advance_available) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_tara_boundary_catch2.cpp:601"
    ):
        raise AssertionError("focused process Available-form source pointer drifted")
    if process_time_advance_available.get("assertions") != 18:
        raise AssertionError("focused process Available-form assertion count drifted")
    if (
        process_time_advance_available.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused process Available-form row is not mapped")
    process_time_advance_available_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-available-focused", limit=0
    )
    if process_time_advance_available_focus.get("lane_state") != "complete":
        raise AssertionError("focused process Available-form lane is not complete")
    if process_time_advance_available_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process Available-form mapped count drifted")
    if process_time_advance_available_focus.get("assertion_count") != 15:
        raise AssertionError(
            "focused process Available-form lane assertion total drifted"
        )
    if process_time_advance_available_focus.get("requirement_count") != 4:
        raise AssertionError("focused process Available-form requirement total drifted")
    if process_time_advance_available_focus.get("standard_section_count") != 3:
        raise AssertionError("focused process Available-form section total drifted")
    process_time_advance_available_handles = process_time_advance_available_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_time_advance_available_handles, dict)
        or process_time_advance_available_handles.get("catch2_target")
        != "umbra_ieee1516_2025_catch2"
    ):
        raise AssertionError("focused process Available-form target handle drifted")
    process_modify_lookahead_grant = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-modify-lookahead-grant-focused-integration"
        ),
        None,
    )
    if not isinstance(process_modify_lookahead_grant, dict):
        raise AssertionError("focused process grant-time lookahead row is absent")
    if query_rti_work.source_location_text(process_modify_lookahead_grant) != (
        "cpp/tests/ieee1516_2025_connection_modify_lookahead_grant_catch2.cpp:458"
    ):
        raise AssertionError(
            "focused process grant-time lookahead source pointer drifted"
        )
    if process_modify_lookahead_grant.get("assertions") != 51:
        raise AssertionError(
            "focused process grant-time lookahead assertion count drifted"
        )
    if (
        process_modify_lookahead_grant.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused process grant-time lookahead row is not mapped")
    process_modify_lookahead_grant_focus = query_rti_work.focused_lane_result(
        index, tests, "process-modify-lookahead-grant-focused", limit=0
    )
    if process_modify_lookahead_grant_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused process grant-time lookahead lane is not complete"
        )
    if process_modify_lookahead_grant_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused process grant-time lookahead mapped count drifted"
        )
    if process_modify_lookahead_grant_focus.get("assertion_count") != 51:
        raise AssertionError(
            "focused process grant-time lookahead lane assertion total drifted"
        )
    if process_modify_lookahead_grant_focus.get("requirement_count") != 9:
        raise AssertionError(
            "focused process grant-time lookahead requirement total drifted"
        )
    if process_modify_lookahead_grant_focus.get("standard_section_count") != 7:
        raise AssertionError(
            "focused process grant-time lookahead section total drifted"
        )
    process_modify_lookahead_grant_handles = process_modify_lookahead_grant_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_modify_lookahead_grant_handles, dict)
        or process_modify_lookahead_grant_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError(
            "focused process grant-time lookahead target handle drifted"
        )
    process_modify_lookahead = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-modify-lookahead-focused-integration"
        ),
        None,
    )
    if not isinstance(process_modify_lookahead, dict):
        raise AssertionError("focused process modify-lookahead row is absent")
    if query_rti_work.source_location_text(process_modify_lookahead) != (
        "cpp/tests/ieee1516_2025_connection_modify_lookahead_catch2.cpp:458"
    ):
        raise AssertionError("focused process modify-lookahead source pointer drifted")
    if process_modify_lookahead.get("assertions") != 23:
        raise AssertionError("focused process modify-lookahead assertion count drifted")
    if process_modify_lookahead.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process modify-lookahead row is not mapped")
    if "process-time-role" in process_modify_lookahead.get("tags", []):
        raise AssertionError(
            "focused process modify-lookahead row contaminated the time-role lane"
        )
    process_modify_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-modify-lookahead-focused", limit=0
    )
    if process_modify_lookahead_focus.get("lane_state") != "complete":
        raise AssertionError("focused process modify-lookahead lane is not complete")
    if process_modify_lookahead_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process modify-lookahead mapped count drifted")
    if process_modify_lookahead_focus.get("assertion_count") != 23:
        raise AssertionError(
            "focused process modify-lookahead lane assertion total drifted"
        )
    if process_modify_lookahead_focus.get("requirement_count") != 4:
        raise AssertionError(
            "focused process modify-lookahead requirement total drifted"
        )
    if process_modify_lookahead_focus.get("standard_section_count") != 2:
        raise AssertionError("focused process modify-lookahead section total drifted")
    process_modify_lookahead_handles = process_modify_lookahead_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(process_modify_lookahead_handles, dict)
        or process_modify_lookahead_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("focused process modify-lookahead target handle drifted")
    process_query_lookahead = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-process-query-lookahead-focused-integration"
        ),
        None,
    )
    if not isinstance(process_query_lookahead, dict):
        raise AssertionError("focused process query-lookahead row is absent")
    if query_rti_work.source_location_text(process_query_lookahead) != (
        "cpp/tests/ieee1516_2025_connection_query_lookahead_catch2.cpp:458"
    ):
        raise AssertionError("focused process query-lookahead source pointer drifted")
    if process_query_lookahead.get("assertions") != 13:
        raise AssertionError("focused process query-lookahead assertion count drifted")
    if process_query_lookahead.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused process query-lookahead row is not mapped")
    process_query_lookahead_focus = query_rti_work.focused_lane_result(
        index, tests, "process-query-lookahead-focused", limit=0
    )
    if process_query_lookahead_focus.get("lane_state") != "complete":
        raise AssertionError("focused process query-lookahead lane is not complete")
    if process_query_lookahead_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused process query-lookahead mapped count drifted")
    if process_query_lookahead_focus.get("assertion_count") != 13:
        raise AssertionError(
            "focused process query-lookahead lane assertion total drifted"
        )
    if process_query_lookahead_focus.get("requirement_count") != 2:
        raise AssertionError(
            "focused process query-lookahead requirement total drifted"
        )
    if process_query_lookahead_focus.get("standard_section_count") != 2:
        raise AssertionError("focused process query-lookahead section total drifted")
    process_query_lookahead_handles = process_query_lookahead_focus.get("lane_handles")
    if (
        not isinstance(process_query_lookahead_handles, dict)
        or process_query_lookahead_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("focused process query-lookahead target handle drifted")
