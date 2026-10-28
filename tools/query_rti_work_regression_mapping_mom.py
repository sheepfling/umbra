# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import MappingLookupInputs


def run_mom_mapping_rows(inputs: MappingLookupInputs) -> query_rti_work.JsonObject:
    index = inputs.index
    tests = inputs.tests
    object_instances_reflected_report_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instances-reflected-report", limit=0
    )
    if object_instances_reflected_report_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected lane is not complete"
        )
    if object_instances_reflected_report_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected mapped count drifted"
        )
    if object_instances_reflected_report_focus.get("assertion_count") != 51:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected assertion total drifted"
        )
    if object_instances_reflected_report_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected requirement total drifted"
        )
    if object_instances_reflected_report_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected section total drifted"
        )
    if (
        object_instances_reflected_report_focus.get("requirement_section_pair_count")
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected direct pair drifted"
        )
    object_instances_reflected_report_handles = (
        object_instances_reflected_report_focus.get("lane_handles")
    )
    if (
        not isinstance(object_instances_reflected_report_handles, dict)
        or object_instances_reflected_report_handles.get("catch2_target")
        != "umbra_joined_federate_mom_object_instances_reflected_report_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected target handle drifted"
        )
    object_instance_information_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instance-information-report-integration"
        ),
        None,
    )
    if not isinstance(object_instance_information_report, dict):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation row is absent"
        )
    if query_rti_work.source_location_text(object_instance_information_report) != (
        "cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp:146"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation source pointer drifted"
        )
    if object_instance_information_report.get("assertions") != 71:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation assertion count drifted"
        )
    if (
        object_instance_information_report.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation row is not mapped"
        )
    if len(object_instance_information_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation requirement mapping drifted"
        )
    if len(object_instance_information_report.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation section mapping drifted"
        )
    if (
        len(object_instance_information_report.get("selected_cpp_api_surface_ids", []))
        != 13
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation API mapping drifted"
        )
    object_instance_information_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instance-information-report", limit=0
    )
    if object_instance_information_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation lane is not complete"
        )
    if object_instance_information_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation mapped count drifted"
        )
    if object_instance_information_focus.get("assertion_count") != 71:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation assertion total drifted"
        )
    if object_instance_information_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation requirement total drifted"
        )
    if object_instance_information_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation section total drifted"
        )
    if object_instance_information_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation direct pair drifted"
        )
    object_instance_information_handles = object_instance_information_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(object_instance_information_handles, dict)
        or object_instance_information_handles.get("catch2_target")
        != "umbra_joined_federate_mom_object_instance_information_report_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstanceInformation target handle drifted"
        )
    discovered_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-discovered-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(discovered_object_count_periodic_mom, dict):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered row is absent"
        )
    if query_rti_work.source_location_text(discovered_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp:127"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered source pointer drifted"
        )
    if discovered_object_count_periodic_mom.get("assertions") != 66:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered assertion count drifted"
        )
    if (
        discovered_object_count_periodic_mom.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered row is not mapped"
        )
    if len(discovered_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered requirement mapping drifted"
        )
    if len(discovered_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered section mapping drifted"
        )
    if (
        len(
            discovered_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])
        )
        != 14
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered API mapping drifted"
        )
    discovered_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-discovered-object-count-periodic", limit=0
    )
    if discovered_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered lane is not complete"
        )
    if discovered_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered mapped count drifted"
        )
    if discovered_object_count_periodic_focus.get("assertion_count") != 66:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered assertion total drifted"
        )
    if discovered_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered requirement total drifted"
        )
    if discovered_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered section total drifted"
        )
    if (
        discovered_object_count_periodic_focus.get("requirement_section_pair_count")
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered direct pair drifted"
        )
    discovered_object_count_periodic_handles = (
        discovered_object_count_periodic_focus.get("lane_handles")
    )
    if (
        not isinstance(discovered_object_count_periodic_handles, dict)
        or discovered_object_count_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_discovered_object_count_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDiscovered target handle drifted"
        )
    removed_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-removed-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(removed_object_count_periodic_mom, dict):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved row is absent"
        )
    if query_rti_work.source_location_text(removed_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp:145"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved source pointer drifted"
        )
    if removed_object_count_periodic_mom.get("assertions") != 78:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved assertion count drifted"
        )
    if (
        removed_object_count_periodic_mom.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved row is not mapped"
        )
    if len(removed_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved requirement mapping drifted"
        )
    if len(removed_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved section mapping drifted"
        )
    if (
        len(removed_object_count_periodic_mom.get("selected_cpp_api_surface_ids", []))
        != 14
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved API mapping drifted"
        )
    removed_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-removed-object-count-periodic", limit=0
    )
    if removed_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved lane is not complete"
        )
    if removed_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved mapped count drifted"
        )
    if removed_object_count_periodic_focus.get("assertion_count") != 78:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved assertion total drifted"
        )
    if removed_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved requirement total drifted"
        )
    if removed_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved section total drifted"
        )
    if removed_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved direct pair drifted"
        )
    removed_object_count_periodic_handles = removed_object_count_periodic_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(removed_object_count_periodic_handles, dict)
        or removed_object_count_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_removed_object_count_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRemoved target handle drifted"
        )
    deleted_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-deleted-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(deleted_object_count_periodic_mom, dict):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted row is absent"
        )
    if query_rti_work.source_location_text(deleted_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp:108"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted source pointer drifted"
        )
    if deleted_object_count_periodic_mom.get("assertions") != 66:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted assertion count drifted"
        )
    if (
        deleted_object_count_periodic_mom.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted row is not mapped"
        )
    if len(deleted_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted requirement mapping drifted"
        )
    if len(deleted_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted section mapping drifted"
        )
    if (
        len(deleted_object_count_periodic_mom.get("selected_cpp_api_surface_ids", []))
        != 12
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted API mapping drifted"
        )
    deleted_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-deleted-object-count-periodic", limit=0
    )
    if deleted_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted lane is not complete"
        )
    if deleted_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted mapped count drifted"
        )
    if deleted_object_count_periodic_focus.get("assertion_count") != 66:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted assertion total drifted"
        )
    if deleted_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted requirement total drifted"
        )
    if deleted_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted section total drifted"
        )
    if deleted_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted direct pair drifted"
        )
    deleted_object_count_periodic_handles = deleted_object_count_periodic_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(deleted_object_count_periodic_handles, dict)
        or deleted_object_count_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_deleted_object_count_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesDeleted target handle drifted"
        )
    registered_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-registered-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(registered_object_count_periodic_mom, dict):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered row is absent"
        )
    if query_rti_work.source_location_text(registered_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp:115"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered source pointer drifted"
        )
    if registered_object_count_periodic_mom.get("assertions") != 112:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered assertion count drifted"
        )
    if (
        registered_object_count_periodic_mom.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered row is not mapped"
        )
    if len(registered_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered requirement mapping drifted"
        )
    if len(registered_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered section mapping drifted"
        )
    if (
        len(
            registered_object_count_periodic_mom.get("selected_cpp_api_surface_ids", [])
        )
        != 12
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered API mapping drifted"
        )
    registered_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-registered-object-count-periodic", limit=0
    )
    if registered_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered lane is not complete"
        )
    if registered_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered mapped count drifted"
        )
    if registered_object_count_periodic_focus.get("assertion_count") != 112:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered assertion total drifted"
        )
    if registered_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered requirement total drifted"
        )
    if registered_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered section total drifted"
        )
    if (
        registered_object_count_periodic_focus.get("requirement_section_pair_count")
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered direct pair drifted"
        )
    registered_object_count_periodic_handles = (
        registered_object_count_periodic_focus.get("lane_handles")
    )
    if (
        not isinstance(registered_object_count_periodic_handles, dict)
        or registered_object_count_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_registered_object_count_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesRegistered target handle drifted"
        )
    updated_object_count_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-updated-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(updated_object_count_periodic_mom, dict):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated row is absent"
        )
    if query_rti_work.source_location_text(updated_object_count_periodic_mom) != (
        "cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp:109"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated source pointer drifted"
        )
    if updated_object_count_periodic_mom.get("assertions") != 77:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated assertion count drifted"
        )
    if (
        updated_object_count_periodic_mom.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated row is not mapped"
        )
    if len(updated_object_count_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated requirement mapping drifted"
        )
    if len(updated_object_count_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated section mapping drifted"
        )
    if (
        len(updated_object_count_periodic_mom.get("selected_cpp_api_surface_ids", []))
        != 12
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated API mapping drifted"
        )
    updated_object_count_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-updated-object-count-periodic", limit=0
    )
    if updated_object_count_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated lane is not complete"
        )
    if updated_object_count_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated mapped count drifted"
        )
    if updated_object_count_periodic_focus.get("assertion_count") != 77:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated assertion total drifted"
        )
    if updated_object_count_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated requirement total drifted"
        )
    if updated_object_count_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated section total drifted"
        )
    if updated_object_count_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated direct pair drifted"
        )
    updated_object_count_periodic_handles = updated_object_count_periodic_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(updated_object_count_periodic_handles, dict)
        or updated_object_count_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_updated_object_count_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAobjectInstancesUpdated target handle drifted"
        )
    updates_sent_periodic_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-updates-sent-periodic-integration"
        ),
        None,
    )
    if not isinstance(updates_sent_periodic_mom, dict):
        raise AssertionError("focused joined-federate HLAupdatesSent row is absent")
    if query_rti_work.source_location_text(updates_sent_periodic_mom) != (
        "cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp:108"
    ):
        raise AssertionError(
            "focused joined-federate HLAupdatesSent source pointer drifted"
        )
    if updates_sent_periodic_mom.get("assertions") != 56:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent assertion count drifted"
        )
    if updates_sent_periodic_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAupdatesSent row is not mapped")
    if len(updates_sent_periodic_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent requirement mapping drifted"
        )
    if len(updates_sent_periodic_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent section mapping drifted"
        )
    if len(updates_sent_periodic_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent API mapping drifted"
        )
    updates_sent_periodic_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-updates-sent-periodic", limit=0
    )
    if updates_sent_periodic_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAupdatesSent lane is not complete"
        )
    if updates_sent_periodic_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent mapped count drifted"
        )
    if updates_sent_periodic_focus.get("assertion_count") != 56:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent assertion total drifted"
        )
    if updates_sent_periodic_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent requirement total drifted"
        )
    if updates_sent_periodic_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent section total drifted"
        )
    if updates_sent_periodic_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAupdatesSent direct pair drifted"
        )
    updates_sent_periodic_handles = updates_sent_periodic_focus.get("lane_handles")
    if (
        not isinstance(updates_sent_periodic_handles, dict)
        or updates_sent_periodic_handles.get("catch2_target")
        != "umbra_joined_federate_mom_updates_sent_periodic_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAupdatesSent target handle drifted"
        )
    ro_length_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-ro-length-periodic-integration"
        ),
        None,
    )
    if not isinstance(ro_length_mom, dict):
        raise AssertionError("focused joined-federate RO-length MOM row is absent")
    if query_rti_work.source_location_text(ro_length_mom) != (
        "cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp:134"
    ):
        raise AssertionError("focused joined-federate RO-length source pointer drifted")
    if ro_length_mom.get("assertions") != 63:
        raise AssertionError(
            "focused joined-federate RO-length assertion count drifted"
        )
    if ro_length_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate RO-length row is not mapped")
    if len(ro_length_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate RO-length requirement mapping drifted"
        )
    if len(ro_length_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate RO-length section mapping drifted"
        )
    if len(ro_length_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError("focused joined-federate RO-length API mapping drifted")
    ro_length_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-ro-length-periodic", limit=0
    )
    if ro_length_focus.get("lane_state") != "complete":
        raise AssertionError("focused joined-federate RO-length lane is not complete")
    if ro_length_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused joined-federate RO-length mapped count drifted")
    if ro_length_focus.get("assertion_count") != 63:
        raise AssertionError(
            "focused joined-federate RO-length assertion total drifted"
        )
    if ro_length_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate RO-length requirement total drifted"
        )
    if ro_length_focus.get("standard_section_count") != 1:
        raise AssertionError("focused joined-federate RO-length section total drifted")
    if ro_length_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused joined-federate RO-length direct pair drifted")
    ro_length_handles = ro_length_focus.get("lane_handles")
    if (
        not isinstance(ro_length_handles, dict)
        or ro_length_handles.get("catch2_target")
        != "umbra_joined_federate_mom_ro_length_periodic_catch2"
    ):
        raise AssertionError("focused joined-federate RO-length target handle drifted")
    deletable_count_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-deletable-object-count-periodic-integration"
        ),
        None,
    )
    if not isinstance(deletable_count_mom, dict):
        raise AssertionError(
            "focused joined-federate deletable-count MOM row is absent"
        )
    if query_rti_work.source_location_text(deletable_count_mom) != (
        "cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp:108"
    ):
        raise AssertionError(
            "focused joined-federate deletable-count source pointer drifted"
        )
    if deletable_count_mom.get("assertions") != 55:
        raise AssertionError(
            "focused joined-federate deletable-count assertion count drifted"
        )
    if deletable_count_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused joined-federate deletable-count row is not mapped"
        )
    if len(deletable_count_mom.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate deletable-count requirement mapping drifted"
        )
    if len(deletable_count_mom.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate deletable-count section mapping drifted"
        )
    if len(deletable_count_mom.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError(
            "focused joined-federate deletable-count API mapping drifted"
        )
    deletable_count_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-deletable-object-count", limit=0
    )
    if deletable_count_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate deletable-count lane is not complete"
        )
    if deletable_count_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate deletable-count mapped count drifted"
        )
    if deletable_count_focus.get("assertion_count") != 55:
        raise AssertionError(
            "focused joined-federate deletable-count assertion total drifted"
        )
    if deletable_count_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate deletable-count requirement total drifted"
        )
    if deletable_count_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate deletable-count section total drifted"
        )
    if deletable_count_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate deletable-count direct pair drifted"
        )
    deletable_count_handles = deletable_count_focus.get("lane_handles")
    if (
        not isinstance(deletable_count_handles, dict)
        or deletable_count_handles.get("catch2_target")
        != "umbra_joined_federate_mom_deletable_object_count_catch2"
    ):
        raise AssertionError(
            "focused joined-federate deletable-count target handle drifted"
        )
    service_reporting_interlock = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-mom-service-reporting-interlock-integration"
        ),
        None,
    )
    if not isinstance(service_reporting_interlock, dict):
        raise AssertionError("focused service-reporting interlock row is absent")
    if query_rti_work.source_location_text(service_reporting_interlock) != (
        "cpp/tests/mom_service_reporting_interlock_catch2.cpp:50"
    ):
        raise AssertionError(
            "focused service-reporting interlock source pointer drifted"
        )
    if service_reporting_interlock.get("assertions") != 41:
        raise AssertionError(
            "focused service-reporting interlock assertion count drifted"
        )
    if service_reporting_interlock.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused service-reporting interlock row is not mapped")
    service_reporting_interlock_focus = query_rti_work.focused_lane_result(
        index, tests, "service-reporting-interlock", limit=0
    )
    if service_reporting_interlock_focus.get("lane_state") != "complete":
        raise AssertionError("focused service-reporting interlock lane is not complete")
    if service_reporting_interlock_focus.get("mapped_test_count") != 2:
        raise AssertionError("focused service-reporting interlock mapped count drifted")
    if service_reporting_interlock_focus.get("assertion_count") != 41:
        raise AssertionError(
            "focused service-reporting interlock assertion total drifted"
        )
    service_reporting_interlock_handles = service_reporting_interlock_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(service_reporting_interlock_handles, dict)
        or service_reporting_interlock_handles.get("catch2_target")
        != "umbra_mom_service_reporting_interlock_catch2"
    ):
        raise AssertionError(
            "focused service-reporting interlock target handle drifted"
        )
    negotiated_cancellation_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-interaction-cancel-negotiated-attribute-ownership-divestiture-integration"
        ),
        None,
    )
    if not isinstance(negotiated_cancellation_mom, dict):
        raise AssertionError("focused negotiated cancellation MOM row is absent")
    if query_rti_work.source_location_text(negotiated_cancellation_mom) != (
        "cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp:547"
    ):
        raise AssertionError("focused negotiated cancellation source pointer drifted")
    if negotiated_cancellation_mom.get("assertions") != 87:
        raise AssertionError("focused negotiated cancellation assertion count drifted")
    if negotiated_cancellation_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused negotiated cancellation row is not mapped")
    negotiated_cancellation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-negotiated-attribute-ownership-divestiture-service-report-interaction",
        limit=0,
    )
    if negotiated_cancellation_focus.get("lane_state") != "complete":
        raise AssertionError("focused negotiated cancellation lane is not complete")
    if negotiated_cancellation_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused negotiated cancellation mapped count drifted")
    if negotiated_cancellation_focus.get("assertion_count") != 87:
        raise AssertionError("focused negotiated cancellation assertion total drifted")
    negotiated_cancellation_handles = negotiated_cancellation_focus.get("lane_handles")
    if (
        not isinstance(negotiated_cancellation_handles, dict)
        or negotiated_cancellation_handles.get("catch2_target")
        != "umbra_negotiated_attribute_ownership_divestiture_pending_catch2"
    ):
        raise AssertionError("focused negotiated cancellation target handle drifted")
    cancellation_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-interaction-cancel-attribute-ownership-acquisition-integration"
        ),
        None,
    )
    if not isinstance(cancellation_mom, dict):
        raise AssertionError(
            "focused Cancel Attribute Ownership Acquisition MOM row is absent"
        )
    if query_rti_work.source_location_text(cancellation_mom) != (
        "cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp:387"
    ):
        raise AssertionError("focused cancellation MOM source pointer drifted")
    if cancellation_mom.get("assertions") != 77:
        raise AssertionError("focused cancellation MOM assertion count drifted")
    if cancellation_mom.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused cancellation MOM row is not mapped")
    cancellation_mom_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "cancel-attribute-ownership-acquisition-service-report-interaction",
        limit=0,
    )
    if cancellation_mom_focus.get("lane_state") != "complete":
        raise AssertionError("focused cancellation MOM lane is not complete")
    if cancellation_mom_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused cancellation MOM mapped count drifted")
    if cancellation_mom_focus.get("assertion_count") != 77:
        raise AssertionError("focused cancellation MOM assertion total drifted")
    cancellation_mom_handles = cancellation_mom_focus.get("lane_handles")
    if (
        not isinstance(cancellation_mom_handles, dict)
        or cancellation_mom_handles.get("catch2_target")
        != "umbra_attribute_ownership_acquisition_cancellation_catch2"
    ):
        raise AssertionError("focused cancellation MOM target handle drifted")
    ownership_mom = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mom-query-attribute-ownership-interaction-integration"
        ),
        None,
    )
    if not isinstance(ownership_mom, dict):
        raise AssertionError("focused Query Attribute Ownership MOM row is absent")
    if query_rti_work.source_location_text(ownership_mom) != (
        "cpp/tests/attribute_ownership_query_catch2.cpp:379"
    ):
        raise AssertionError(
            "focused Query Attribute Ownership MOM source pointer drifted"
        )
    if ownership_mom.get("assertions") != 67:
        raise AssertionError(
            "focused Query Attribute Ownership MOM assertion count drifted"
        )
    ownership_mom_focus = query_rti_work.focused_lane_result(
        index, tests, "query-attribute-ownership-service-report-interaction", limit=0
    )
    if ownership_mom_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused Query Attribute Ownership MOM lane is not complete"
        )
    if ownership_mom_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused Query Attribute Ownership MOM mapped count drifted"
        )
    if ownership_mom_focus.get("assertion_count") != 67:
        raise AssertionError(
            "focused Query Attribute Ownership MOM assertion total drifted"
        )
    ownership_mom_handles = ownership_mom_focus.get("lane_handles")
    if (
        not isinstance(ownership_mom_handles, dict)
        or ownership_mom_handles.get("catch2_target")
        != "umbra_attribute_ownership_query_catch2"
    ):
        raise AssertionError(
            "focused Query Attribute Ownership MOM target handle drifted"
        )
    attribute_relevance = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded attribute relevance advisories follow scope transitions"
        ),
        None,
    )
    if not isinstance(attribute_relevance, dict):
        raise AssertionError("focused attribute-relevance plan row is absent")
    if query_rti_work.source_location_text(attribute_relevance) != (
        "cpp/tests/attribute_relevance_advisory_catch2.cpp:265"
    ):
        raise AssertionError("focused attribute-relevance source pointer drifted")
    if attribute_relevance.get("assertions") != 106:
        raise AssertionError("focused attribute-relevance assertion count drifted")
    attribute_relevance_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-relevance-scope-transition", limit=0
    )
    if attribute_relevance_focus.get("lane_state") != "complete":
        raise AssertionError("focused attribute-relevance lane is not complete")
    if attribute_relevance_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused attribute-relevance mapped count drifted")
    if attribute_relevance_focus.get("assertion_count") != 106:
        raise AssertionError("focused attribute-relevance lane assertion total drifted")
    attribute_relevance_handles = attribute_relevance_focus.get("lane_handles")
    if (
        not isinstance(attribute_relevance_handles, dict)
        or attribute_relevance_handles.get("catch2_target")
        != "umbra_attribute_relevance_advisory_catch2"
    ):
        raise AssertionError("focused attribute-relevance target handle drifted")
    attribute_relevance_rate_reissue = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-advisory-rate-reissue-integration"
        ),
        None,
    )
    if not isinstance(attribute_relevance_rate_reissue, dict):
        raise AssertionError("focused attribute-relevance rate-reissue row is absent")
    if query_rti_work.source_location_text(attribute_relevance_rate_reissue) != (
        "cpp/tests/attribute_relevance_rate_reissue_catch2.cpp:109"
    ):
        raise AssertionError(
            "focused attribute-relevance rate-reissue source pointer drifted"
        )
    if attribute_relevance_rate_reissue.get("assertions") != 142:
        raise AssertionError(
            "focused attribute-relevance rate-reissue assertion count drifted"
        )
    if (
        attribute_relevance_rate_reissue.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused attribute-relevance rate-reissue row is not mapped"
        )
    if len(attribute_relevance_rate_reissue.get("lab_requirement_ids", [])) != 7:
        raise AssertionError(
            "focused attribute-relevance rate-reissue requirement mapping drifted"
        )
    if len(attribute_relevance_rate_reissue.get("standard_sections", [])) != 2:
        raise AssertionError(
            "focused attribute-relevance rate-reissue section mapping drifted"
        )
    if (
        len(attribute_relevance_rate_reissue.get("selected_cpp_api_surface_ids", []))
        != 3
    ):
        raise AssertionError(
            "focused attribute-relevance rate-reissue API mapping drifted"
        )
    attribute_relevance_rate_reissue_focus = query_rti_work.focused_lane_result(
        index, tests, "attribute-relevance-rate-reissue", limit=0
    )
    if attribute_relevance_rate_reissue_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused attribute-relevance rate-reissue lane is not complete"
        )
    if attribute_relevance_rate_reissue_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused attribute-relevance rate-reissue mapped count drifted"
        )
    if attribute_relevance_rate_reissue_focus.get("assertion_count") != 142:
        raise AssertionError(
            "focused attribute-relevance rate-reissue lane assertion total drifted"
        )
    attribute_relevance_rate_reissue_handles = (
        attribute_relevance_rate_reissue_focus.get("lane_handles")
    )
    if (
        not isinstance(attribute_relevance_rate_reissue_handles, dict)
        or attribute_relevance_rate_reissue_handles.get("catch2_target")
        != "umbra_attribute_relevance_rate_reissue_catch2"
    ):
        raise AssertionError(
            "focused attribute-relevance rate-reissue target handle drifted"
        )
    regional_attribute_relevance_rate_designator = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-regional-attribute-relevance-advisory-rate-designator-integration"
        ),
        None,
    )
    if not isinstance(regional_attribute_relevance_rate_designator, dict):
        raise AssertionError(
            "focused regional attribute-relevance rate-designator row is absent"
        )
    if query_rti_work.source_location_text(
        regional_attribute_relevance_rate_designator
    ) != ("cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp:105"):
        raise AssertionError("focused regional rate-designator source pointer drifted")
    if regional_attribute_relevance_rate_designator.get("assertions") != 100:
        raise AssertionError("focused regional rate-designator assertion count drifted")
    if (
        regional_attribute_relevance_rate_designator.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused regional rate-designator row is not mapped")
    if (
        len(regional_attribute_relevance_rate_designator.get("lab_requirement_ids", []))
        != 8
    ):
        raise AssertionError(
            "focused regional rate-designator requirement mapping drifted"
        )
    if (
        len(regional_attribute_relevance_rate_designator.get("standard_sections", []))
        != 3
    ):
        raise AssertionError("focused regional rate-designator section mapping drifted")
    if (
        len(
            regional_attribute_relevance_rate_designator.get(
                "selected_cpp_api_surface_ids", []
            )
        )
        != 7
    ):
        raise AssertionError("focused regional rate-designator API mapping drifted")
    regional_attribute_relevance_rate_designator_focus = (
        query_rti_work.focused_lane_result(
            index,
            tests,
            "regional-attribute-relevance-rate-designator",
            limit=0,
        )
    )
    if (
        regional_attribute_relevance_rate_designator_focus.get("lane_state")
        != "complete"
    ):
        raise AssertionError("focused regional rate-designator lane is not complete")
    if regional_attribute_relevance_rate_designator_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused regional rate-designator mapped count drifted")
    if regional_attribute_relevance_rate_designator_focus.get("assertion_count") != 100:
        raise AssertionError(
            "focused regional rate-designator lane assertion total drifted"
        )
    regional_attribute_relevance_rate_designator_handles = (
        regional_attribute_relevance_rate_designator_focus.get("lane_handles")
    )
    if (
        not isinstance(regional_attribute_relevance_rate_designator_handles, dict)
        or regional_attribute_relevance_rate_designator_handles.get("catch2_target")
        != "umbra_regional_attribute_relevance_rate_designator_catch2"
    ):
        raise AssertionError("focused regional rate-designator target handle drifted")
    known_class_disabled = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-known-class-disabled-subscription-integration"
        ),
        None,
    )
    if not isinstance(known_class_disabled, dict):
        raise AssertionError("focused known-class-disabled advisory row is absent")
    if query_rti_work.source_location_text(known_class_disabled) != (
        "cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp:97"
    ):
        raise AssertionError(
            "focused known-class-disabled advisory source pointer drifted"
        )
    if known_class_disabled.get("assertions") != 35:
        raise AssertionError(
            "focused known-class-disabled advisory assertion count drifted"
        )
    if known_class_disabled.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused known-class-disabled advisory row is not mapped")
    if len(known_class_disabled.get("lab_requirement_ids", [])) != 18:
        raise AssertionError(
            "focused known-class-disabled advisory requirement mapping drifted"
        )
    if len(known_class_disabled.get("standard_sections", [])) != 13:
        raise AssertionError(
            "focused known-class-disabled advisory section mapping drifted"
        )
    if len(known_class_disabled.get("selected_cpp_api_surface_ids", [])) != 18:
        raise AssertionError(
            "focused known-class-disabled advisory API mapping drifted"
        )
    known_class_disabled_focus = query_rti_work.focused_lane_result(
        index, tests, "known-class-disabled", limit=0
    )
    if known_class_disabled_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused known-class-disabled advisory lane is not complete"
        )
    if known_class_disabled_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused known-class-disabled advisory mapped count drifted"
        )
    if known_class_disabled_focus.get("assertion_count") != 35:
        raise AssertionError(
            "focused known-class-disabled advisory assertion total drifted"
        )
    known_class_disabled_handles = known_class_disabled_focus.get("lane_handles")
    if (
        not isinstance(known_class_disabled_handles, dict)
        or known_class_disabled_handles.get("catch2_target")
        != "umbra_attribute_relevance_known_class_disabled_subscription_catch2"
    ):
        raise AssertionError(
            "focused known-class-disabled advisory target handle drifted"
        )
    known_class_enabled = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-attribute-relevance-known-class-enabled-subscription-integration"
        ),
        None,
    )
    if not isinstance(known_class_enabled, dict):
        raise AssertionError("focused known-class-enabled advisory row is absent")
    if query_rti_work.source_location_text(known_class_enabled) != (
        "cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp:97"
    ):
        raise AssertionError(
            "focused known-class-enabled advisory source pointer drifted"
        )
    if known_class_enabled.get("assertions") != 30:
        raise AssertionError(
            "focused known-class-enabled advisory assertion count drifted"
        )
    if known_class_enabled.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused known-class-enabled advisory row is not mapped")
    if len(known_class_enabled.get("lab_requirement_ids", [])) != 15:
        raise AssertionError(
            "focused known-class-enabled advisory requirement mapping drifted"
        )
    if len(known_class_enabled.get("standard_sections", [])) != 13:
        raise AssertionError(
            "focused known-class-enabled advisory section mapping drifted"
        )
    if len(known_class_enabled.get("selected_cpp_api_surface_ids", [])) != 17:
        raise AssertionError("focused known-class-enabled advisory API mapping drifted")
    known_class_enabled_focus = query_rti_work.focused_lane_result(
        index, tests, "known-class-enabled", limit=0
    )
    if known_class_enabled_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused known-class-enabled advisory lane is not complete"
        )
    if known_class_enabled_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused known-class-enabled advisory mapped count drifted"
        )
    if known_class_enabled_focus.get("assertion_count") != 30:
        raise AssertionError(
            "focused known-class-enabled advisory assertion total drifted"
        )
    known_class_enabled_handles = known_class_enabled_focus.get("lane_handles")
    if (
        not isinstance(known_class_enabled_handles, dict)
        or known_class_enabled_handles.get("catch2_target")
        != "umbra_attribute_relevance_known_class_enabled_subscription_catch2"
    ):
        raise AssertionError(
            "focused known-class-enabled advisory target handle drifted"
        )
    passive_regional_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-update-rate-lookup-passive-regional-subscription-integration"
        ),
        None,
    )
    if not isinstance(passive_regional_update_rate, dict):
        raise AssertionError("focused passive regional update-rate row is absent")
    if query_rti_work.source_location_text(passive_regional_update_rate) != (
        "cpp/tests/update_rate_passive_regional_subscription_catch2.cpp:73"
    ):
        raise AssertionError(
            "focused passive regional update-rate source pointer drifted"
        )
    if passive_regional_update_rate.get("assertions") != 43:
        raise AssertionError(
            "focused passive regional update-rate assertion count drifted"
        )
    if passive_regional_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused passive regional update-rate row is not mapped")
    if len(passive_regional_update_rate.get("lab_requirement_ids", [])) != 15:
        raise AssertionError(
            "focused passive regional update-rate requirement mapping drifted"
        )
    if len(passive_regional_update_rate.get("standard_sections", [])) != 12:
        raise AssertionError(
            "focused passive regional update-rate section mapping drifted"
        )
    if len(passive_regional_update_rate.get("selected_cpp_api_surface_ids", [])) != 21:
        raise AssertionError("focused passive regional update-rate API mapping drifted")
    passive_regional_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-passive-regional-subscription", limit=0
    )
    if passive_regional_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused passive regional update-rate lane is not complete"
        )
    if passive_regional_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused passive regional update-rate mapped count drifted"
        )
    if passive_regional_update_rate_focus.get("assertion_count") != 43:
        raise AssertionError(
            "focused passive regional update-rate assertion total drifted"
        )
    passive_regional_update_rate_handles = passive_regional_update_rate_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(passive_regional_update_rate_handles, dict)
        or passive_regional_update_rate_handles.get("catch2_target")
        != "umbra_update_rate_passive_regional_subscription_catch2"
    ):
        raise AssertionError(
            "focused passive regional update-rate target handle drifted"
        )
    mixed_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-mixed-update-rate-subscriptions-independent-attribute-gating-integration"
        ),
        None,
    )
    if not isinstance(mixed_update_rate, dict):
        raise AssertionError("focused mixed update-rate row is absent")
    if query_rti_work.source_location_text(mixed_update_rate) != (
        "cpp/tests/mixed_update_rate_subscriptions_catch2.cpp:199"
    ):
        raise AssertionError("focused mixed update-rate source pointer drifted")
    if mixed_update_rate.get("assertions") != 37:
        raise AssertionError("focused mixed update-rate assertion count drifted")
    if mixed_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused mixed update-rate row is not mapped")
    if len(mixed_update_rate.get("lab_requirement_ids", [])) != 8:
        raise AssertionError("focused mixed update-rate requirement mapping drifted")
    if len(mixed_update_rate.get("standard_sections", [])) != 2:
        raise AssertionError("focused mixed update-rate section mapping drifted")
    if len(mixed_update_rate.get("selected_cpp_api_surface_ids", [])) != 15:
        raise AssertionError("focused mixed update-rate API mapping drifted")
    mixed_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-mixed-attribute-gating", limit=0
    )
    if mixed_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError("focused mixed update-rate lane is not complete")
    if mixed_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused mixed update-rate mapped count drifted")
    if mixed_update_rate_focus.get("assertion_count") != 37:
        raise AssertionError("focused mixed update-rate assertion total drifted")
    mixed_update_rate_handles = mixed_update_rate_focus.get("lane_handles")
    if (
        not isinstance(mixed_update_rate_handles, dict)
        or mixed_update_rate_handles.get("catch2_target")
        != "umbra_mixed_update_rate_subscriptions_catch2"
    ):
        raise AssertionError("focused mixed update-rate target handle drifted")
    teardown_update_rate = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-teardown-preserves-update-rate-history-integration"
        ),
        None,
    )
    if not isinstance(teardown_update_rate, dict):
        raise AssertionError("focused federation-teardown update-rate row is absent")
    if query_rti_work.source_location_text(teardown_update_rate) != (
        "cpp/tests/federation_teardown_update_rate_history_catch2.cpp:158"
    ):
        raise AssertionError(
            "focused federation-teardown update-rate source pointer drifted"
        )
    if teardown_update_rate.get("assertions") != 46:
        raise AssertionError(
            "focused federation-teardown update-rate assertion count drifted"
        )
    if teardown_update_rate.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused federation-teardown update-rate row is not mapped"
        )
    if len(teardown_update_rate.get("lab_requirement_ids", [])) != 9:
        raise AssertionError(
            "focused federation-teardown update-rate requirement mapping drifted"
        )
    if len(teardown_update_rate.get("standard_sections", [])) != 5:
        raise AssertionError(
            "focused federation-teardown update-rate section mapping drifted"
        )
    if len(teardown_update_rate.get("selected_cpp_api_surface_ids", [])) != 15:
        raise AssertionError(
            "focused federation-teardown update-rate API mapping drifted"
        )
    teardown_update_rate_focus = query_rti_work.focused_lane_result(
        index, tests, "update-rate-federation-teardown-isolation", limit=0
    )
    if teardown_update_rate_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused federation-teardown update-rate lane is not complete"
        )
    if teardown_update_rate_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused federation-teardown update-rate mapped count drifted"
        )
    if teardown_update_rate_focus.get("assertion_count") != 46:
        raise AssertionError(
            "focused federation-teardown update-rate assertion total drifted"
        )
    teardown_update_rate_handles = teardown_update_rate_focus.get("lane_handles")
    if (
        not isinstance(teardown_update_rate_handles, dict)
        or teardown_update_rate_handles.get("catch2_target")
        != "umbra_federation_teardown_update_rate_history_catch2"
    ):
        raise AssertionError(
            "focused federation-teardown update-rate target handle drifted"
        )
    restored_default_region_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-restore-live-tso-default-region-attribute-update-multi-recipient-integration"
        ),
        None,
    )
    if not isinstance(restored_default_region_multi, dict):
        raise AssertionError(
            "focused multi-recipient default-region restore row is absent"
        )
    if query_rti_work.source_location_text(restored_default_region_multi) != (
        "cpp/tests/ieee1516_2025_timestamped_default_region_attribute_restore_multi_recipient_catch2.cpp:6"
    ):
        raise AssertionError(
            "focused multi-recipient default-region restore source pointer drifted"
        )
    if restored_default_region_multi.get("assertions") != 123:
        raise AssertionError(
            "focused multi-recipient default-region restore assertion count drifted"
        )
    if restored_default_region_multi.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused multi-recipient default-region restore row is not mapped"
        )
    if len(restored_default_region_multi.get("lab_requirement_ids", [])) != 18:
        raise AssertionError(
            "focused multi-recipient default-region restore requirement mapping drifted"
        )
    return restored_default_region_multi
