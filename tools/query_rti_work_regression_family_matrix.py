# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import FamilyMatrixInputs
from query_rti_work_regression_support import (
    TARGET_TEST,
    expected_family_gap_head,
)


def run_family_matrix(inputs: FamilyMatrixInputs) -> None:
    active_handoff = inputs.active_handoff
    index = inputs.index
    queue = inputs.queue
    restored_default_region_multi = inputs.restored_default_region_multi
    source_locations = inputs.source_locations
    standard_requirements = inputs.standard_requirements
    status = inputs.status
    tests = inputs.tests
    # The mapped view includes §6.11 from its direct requirement pair in
    # addition to the 14 sections explicitly listed on the plan row.
    if len(restored_default_region_multi.get("standard_sections", [])) != 15:
        raise AssertionError(
            "focused multi-recipient default-region restore section mapping drifted"
        )
    if len(restored_default_region_multi.get("selected_cpp_api_surface_ids", [])) != 24:
        raise AssertionError(
            "focused multi-recipient default-region restore API mapping drifted"
        )
    restored_default_region_multi_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "timestamped-default-region-attribute-restore-multi-recipient",
        limit=0,
    )
    if restored_default_region_multi_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused multi-recipient default-region restore lane is not complete"
        )
    if restored_default_region_multi_focus.get("mapped_test_count") != 2:
        raise AssertionError(
            "focused multi-recipient default-region restore mapped count drifted"
        )
    if restored_default_region_multi_focus.get("assertion_count") != 123:
        raise AssertionError(
            "focused multi-recipient default-region restore lane assertion total drifted"
        )
    restored_default_region_multi_handles = restored_default_region_multi_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(restored_default_region_multi_handles, dict)
        or restored_default_region_multi_handles.get("catch2_target")
        != "umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2"
    ):
        raise AssertionError(
            "focused multi-recipient default-region restore target handle drifted"
        )
    timed_regional_interaction = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-restore-live-tso-regional-interaction-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_interaction, dict):
        raise AssertionError("focused timed regional-interaction restore row is absent")
    if query_rti_work.source_location_text(timed_regional_interaction) != (
        "cpp/tests/ieee1516_2025_timed_restore_live_tso_regional_interaction_catch2.cpp:6"
    ):
        raise AssertionError(
            "focused timed regional-interaction restore source pointer drifted"
        )
    if timed_regional_interaction.get("assertions") != 55:
        raise AssertionError(
            "focused timed regional-interaction restore assertion count drifted"
        )
    if timed_regional_interaction.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused timed regional-interaction restore row is not mapped"
        )
    if len(timed_regional_interaction.get("lab_requirement_ids", [])) != 18:
        raise AssertionError(
            "focused timed regional-interaction restore requirement mapping drifted"
        )
    if len(timed_regional_interaction.get("standard_sections", [])) != 15:
        raise AssertionError(
            "focused timed regional-interaction restore section mapping drifted"
        )
    if len(timed_regional_interaction.get("selected_cpp_api_surface_ids", [])) != 24:
        raise AssertionError(
            "focused timed regional-interaction restore API mapping drifted"
        )
    timed_regional_interaction_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-interaction-timed-restore", limit=0
    )
    if timed_regional_interaction_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused timed regional-interaction restore lane is not complete"
        )
    if timed_regional_interaction_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused timed regional-interaction restore mapped count drifted"
        )
    if timed_regional_interaction_focus.get("assertion_count") != 55:
        raise AssertionError(
            "focused timed regional-interaction restore lane assertion total drifted"
        )
    timed_regional_interaction_handles = timed_regional_interaction_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(timed_regional_interaction_handles, dict)
        or timed_regional_interaction_handles.get("catch2_target")
        != "umbra_timed_restore_live_tso_regional_interaction_catch2"
    ):
        raise AssertionError(
            "focused timed regional-interaction restore target handle drifted"
        )
    regional_interaction_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timestamped-regional-interaction-resignation-integration"
        ),
        None,
    )
    if not isinstance(regional_interaction_resignation, dict):
        raise AssertionError(
            "focused regional-interaction source-resignation row is absent"
        )
    if query_rti_work.source_location_text(regional_interaction_resignation) != (
        "cpp/tests/timed_live_tso_regional_interaction_source_resignation_catch2.cpp:14"
    ):
        raise AssertionError(
            "focused regional-interaction source-resignation source pointer drifted"
        )
    if regional_interaction_resignation.get("assertions") != 60:
        raise AssertionError(
            "focused regional-interaction source-resignation assertion count drifted"
        )
    if (
        regional_interaction_resignation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused regional-interaction source-resignation row is not mapped"
        )
    if len(regional_interaction_resignation.get("lab_requirement_ids", [])) != 5:
        raise AssertionError(
            "focused regional-interaction source-resignation requirement mapping drifted"
        )
    if len(regional_interaction_resignation.get("standard_sections", [])) != 5:
        raise AssertionError(
            "focused regional-interaction source-resignation section mapping drifted"
        )
    if (
        len(regional_interaction_resignation.get("selected_cpp_api_surface_ids", []))
        != 13
    ):
        raise AssertionError(
            "focused regional-interaction source-resignation API mapping drifted"
        )
    regional_interaction_resignation_focus = query_rti_work.focused_lane_result(
        index, tests, "timestamped-regional-interaction-source-resignation", limit=0
    )
    if regional_interaction_resignation_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused regional-interaction source-resignation lane is not complete"
        )
    if regional_interaction_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused regional-interaction source-resignation mapped count drifted"
        )
    if regional_interaction_resignation_focus.get("assertion_count") != 60:
        raise AssertionError(
            "focused regional-interaction source-resignation lane assertion total drifted"
        )
    regional_interaction_resignation_handles = (
        regional_interaction_resignation_focus.get("lane_handles")
    )
    if (
        not isinstance(regional_interaction_resignation_handles, dict)
        or regional_interaction_resignation_handles.get("catch2_target")
        != "umbra_timed_live_tso_regional_interaction_source_resignation_catch2"
    ):
        raise AssertionError(
            "focused regional-interaction source-resignation target handle drifted"
        )
    regional_interaction_restore_multi = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-restore-live-tso-regional-interaction-multi-recipient-integration"
        ),
        None,
    )
    if not isinstance(regional_interaction_restore_multi, dict):
        raise AssertionError(
            "focused regional-interaction restore multi-recipient row is absent"
        )
    if query_rti_work.source_location_text(regional_interaction_restore_multi) != (
        "cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp:169"
    ):
        raise AssertionError(
            "focused regional-interaction restore multi-recipient source pointer drifted"
        )
    if regional_interaction_restore_multi.get("assertions") != 128:
        raise AssertionError(
            "focused regional-interaction restore multi-recipient assertion count drifted"
        )
    if (
        regional_interaction_restore_multi.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused regional-interaction restore multi-recipient row is not mapped"
        )
    if len(regional_interaction_restore_multi.get("lab_requirement_ids", [])) != 11:
        raise AssertionError(
            "focused regional-interaction restore multi-recipient requirement mapping drifted"
        )
    if len(regional_interaction_restore_multi.get("standard_sections", [])) != 11:
        raise AssertionError(
            "focused regional-interaction restore multi-recipient section mapping drifted"
        )
    if (
        len(regional_interaction_restore_multi.get("selected_cpp_api_surface_ids", []))
        != 35
    ):
        raise AssertionError(
            "focused regional-interaction restore multi-recipient API mapping drifted"
        )
    regional_interaction_restore_multi_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "timestamped-regional-interaction-restore-multi-recipient",
        limit=0,
    )
    if regional_interaction_restore_multi_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused regional-interaction restore multi-recipient lane is not complete"
        )
    if regional_interaction_restore_multi_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused regional-interaction restore multi-recipient mapped count drifted"
        )
    if regional_interaction_restore_multi_focus.get("assertion_count") != 128:
        raise AssertionError(
            "focused regional-interaction restore multi-recipient lane assertion total drifted"
        )
    regional_interaction_restore_multi_handles = (
        regional_interaction_restore_multi_focus.get("lane_handles")
    )
    if (
        not isinstance(regional_interaction_restore_multi_handles, dict)
        or regional_interaction_restore_multi_handles.get("catch2_target")
        != "umbra_restore_live_tso_regional_interaction_multi_recipient_catch2"
    ):
        raise AssertionError(
            "focused regional-interaction restore multi-recipient target handle drifted"
        )
    timed_regional_attribute_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-live-tso-regional-attribute-update-source-resignation-after-restore-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_attribute_resignation, dict):
        raise AssertionError(
            "focused timed regional-attribute source-resignation row is absent"
        )
    if query_rti_work.source_location_text(timed_regional_attribute_resignation) != (
        "cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp:180"
    ):
        raise AssertionError(
            "focused timed regional-attribute source-resignation source pointer drifted"
        )
    if timed_regional_attribute_resignation.get("assertions") != 106:
        raise AssertionError(
            "focused timed regional-attribute source-resignation assertion count drifted"
        )
    if (
        timed_regional_attribute_resignation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused timed regional-attribute source-resignation row is not mapped"
        )
    if len(timed_regional_attribute_resignation.get("lab_requirement_ids", [])) != 27:
        raise AssertionError(
            "focused timed regional-attribute source-resignation requirement mapping drifted"
        )
    if len(timed_regional_attribute_resignation.get("standard_sections", [])) != 19:
        raise AssertionError(
            "focused timed regional-attribute source-resignation section mapping drifted"
        )
    if (
        len(
            timed_regional_attribute_resignation.get("selected_cpp_api_surface_ids", [])
        )
        != 22
    ):
        raise AssertionError(
            "focused timed regional-attribute source-resignation API mapping drifted"
        )
    timed_regional_attribute_resignation_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "tso-regional-attribute-update-timed-live-resignation-state",
        limit=0,
    )
    if timed_regional_attribute_resignation_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused timed regional-attribute source-resignation lane is not complete"
        )
    if timed_regional_attribute_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused timed regional-attribute source-resignation mapped count drifted"
        )
    if timed_regional_attribute_resignation_focus.get("assertion_count") != 106:
        raise AssertionError(
            "focused timed regional-attribute source-resignation lane assertion total drifted"
        )
    timed_regional_attribute_resignation_handles = (
        timed_regional_attribute_resignation_focus.get("lane_handles")
    )
    if (
        not isinstance(timed_regional_attribute_resignation_handles, dict)
        or timed_regional_attribute_resignation_handles.get("catch2_target")
        != "umbra_timed_regional_attr_source_resign_restore_catch2"
    ):
        raise AssertionError(
            "focused timed regional-attribute source-resignation target handle drifted"
        )
    timed_regional_attribute_restore = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timed-restore-live-tso-explicit-regional-attribute-update-integration"
        ),
        None,
    )
    if not isinstance(timed_regional_attribute_restore, dict):
        raise AssertionError("focused timed regional-attribute restore row is absent")
    if query_rti_work.source_location_text(timed_regional_attribute_restore) != (
        "cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp:190"
    ):
        raise AssertionError(
            "focused timed regional-attribute restore source pointer drifted"
        )
    if timed_regional_attribute_restore.get("assertions") != 84:
        raise AssertionError(
            "focused timed regional-attribute restore assertion count drifted"
        )
    if (
        timed_regional_attribute_restore.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused timed regional-attribute restore row is not mapped"
        )
    if len(timed_regional_attribute_restore.get("lab_requirement_ids", [])) != 23:
        raise AssertionError(
            "focused timed regional-attribute restore requirement mapping drifted"
        )
    if len(timed_regional_attribute_restore.get("standard_sections", [])) != 16:
        raise AssertionError(
            "focused timed regional-attribute restore section mapping drifted"
        )
    if (
        len(timed_regional_attribute_restore.get("selected_cpp_api_surface_ids", []))
        != 23
    ):
        raise AssertionError(
            "focused timed regional-attribute restore API mapping drifted"
        )
    timed_regional_attribute_restore_focus = query_rti_work.focused_lane_result(
        index, tests, "tso-regional-attribute-update-timed-live-restore-state", limit=0
    )
    if timed_regional_attribute_restore_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused timed regional-attribute restore lane is not complete"
        )
    if timed_regional_attribute_restore_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused timed regional-attribute restore mapped count drifted"
        )
    if timed_regional_attribute_restore_focus.get("assertion_count") != 84:
        raise AssertionError(
            "focused timed regional-attribute restore lane assertion total drifted"
        )
    timed_regional_attribute_restore_handles = (
        timed_regional_attribute_restore_focus.get("lane_handles")
    )
    if (
        not isinstance(timed_regional_attribute_restore_handles, dict)
        or timed_regional_attribute_restore_handles.get("catch2_target")
        != "umbra_timed_restore_regional_attr_catch2"
    ):
        raise AssertionError(
            "focused timed regional-attribute restore target handle drifted"
        )
    timestamped_regional_attribute_resignation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-timestamped-regional-attribute-update-resignation-integration"
        ),
        None,
    )
    if not isinstance(timestamped_regional_attribute_resignation, dict):
        raise AssertionError(
            "focused timestamped regional-attribute resignation row is absent"
        )
    if query_rti_work.source_location_text(
        timestamped_regional_attribute_resignation
    ) != ("cpp/tests/timestamped_regional_attribute_update_resignation_catch2.cpp:146"):
        raise AssertionError(
            "focused timestamped regional-attribute resignation source pointer drifted"
        )
    if timestamped_regional_attribute_resignation.get("assertions") != 58:
        raise AssertionError(
            "focused timestamped regional-attribute resignation assertion count drifted"
        )
    if (
        timestamped_regional_attribute_resignation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused timestamped regional-attribute resignation row is not mapped"
        )
    if (
        len(timestamped_regional_attribute_resignation.get("lab_requirement_ids", []))
        != 8
    ):
        raise AssertionError(
            "focused timestamped regional-attribute resignation requirement mapping drifted"
        )
    if (
        len(timestamped_regional_attribute_resignation.get("standard_sections", []))
        != 6
    ):
        raise AssertionError(
            "focused timestamped regional-attribute resignation section mapping drifted"
        )
    if (
        len(
            timestamped_regional_attribute_resignation.get(
                "selected_cpp_api_surface_ids", []
            )
        )
        != 14
    ):
        raise AssertionError(
            "focused timestamped regional-attribute resignation API mapping drifted"
        )
    timestamped_regional_attribute_resignation_focus = (
        query_rti_work.focused_lane_result(
            index, tests, "timestamped-regional-attribute-update-resignation", limit=0
        )
    )
    if timestamped_regional_attribute_resignation_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused timestamped regional-attribute resignation lane is not complete"
        )
    if timestamped_regional_attribute_resignation_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused timestamped regional-attribute resignation mapped count drifted"
        )
    if timestamped_regional_attribute_resignation_focus.get("assertion_count") != 58:
        raise AssertionError(
            "focused timestamped regional-attribute resignation lane assertion total drifted"
        )
    timestamped_regional_attribute_resignation_handles = (
        timestamped_regional_attribute_resignation_focus.get("lane_handles")
    )
    if (
        not isinstance(timestamped_regional_attribute_resignation_handles, dict)
        or timestamped_regional_attribute_resignation_handles.get("catch2_target")
        != "umbra_timestamped_regional_attribute_update_resignation_catch2"
    ):
        raise AssertionError(
            "focused timestamped regional-attribute resignation target handle drifted"
        )
    registration_discovery = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded object-instance registration/discovery honors 2025 publication, promotion, and callback lifecycle"
        ),
        None,
    )
    if not isinstance(registration_discovery, dict):
        raise AssertionError(
            "focused object-instance registration/discovery plan row is absent"
        )
    if query_rti_work.source_location_text(registration_discovery) != (
        "cpp/tests/object_instance_registration_discovery_catch2.cpp:78"
    ):
        raise AssertionError(
            "focused object-instance registration/discovery source pointer drifted"
        )
    if registration_discovery.get("assertions") != 81:
        raise AssertionError(
            "focused object-instance registration/discovery assertion count drifted"
        )
    registration_discovery_focus = query_rti_work.focused_lane_result(
        index, tests, "object-instance-registration-discovery", limit=0
    )
    if registration_discovery_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused object-instance registration/discovery lane is not complete"
        )
    if registration_discovery_focus.get("mapped_test_count") != 2:
        raise AssertionError(
            "focused object-instance registration/discovery mapped count drifted"
        )
    if registration_discovery_focus.get("assertion_count") != 109:
        raise AssertionError(
            "focused object-instance registration/discovery lane assertion total drifted"
        )
    registration_discovery_handles = registration_discovery_focus.get("lane_handles")
    if (
        not isinstance(registration_discovery_handles, dict)
        or registration_discovery_handles.get("catch2_target")
        != "umbra_object_instance_registration_discovery_catch2"
    ):
        raise AssertionError(
            "focused object-instance registration/discovery target handle drifted"
        )
    transportation_stability = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded custom transportation handles remain stable across an additional FOM join"
        ),
        None,
    )
    if not isinstance(transportation_stability, dict):
        raise AssertionError(
            "focused transportation-handle stability plan row is absent"
        )
    if query_rti_work.source_location_text(transportation_stability) != (
        "cpp/tests/custom_transportation_handle_stability_catch2.cpp:44"
    ):
        raise AssertionError(
            "focused transportation-handle stability source pointer drifted"
        )
    if transportation_stability.get("assertions") != 21:
        raise AssertionError(
            "focused transportation-handle stability assertion count drifted"
        )
    transportation_stability_focus = query_rti_work.focused_lane_result(
        index, tests, "transportation-handle-stability", limit=0
    )
    if transportation_stability_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused transportation-handle stability lane is not complete"
        )
    if transportation_stability_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused transportation-handle stability mapped count drifted"
        )
    if transportation_stability_focus.get("assertion_count") != 21:
        raise AssertionError(
            "focused transportation-handle stability lane assertion total drifted"
        )
    transportation_stability_handles = transportation_stability_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(transportation_stability_handles, dict)
        or transportation_stability_handles.get("catch2_target")
        != "umbra_custom_transportation_handle_stability_catch2"
    ):
        raise AssertionError(
            "focused transportation-handle stability target handle drifted"
        )
    current_fdd = next(
        (
            test
            for test in tests
            if test.get("test_case")
            == "Embedded federation MOM exposes and refreshes HLAcurrentFDD"
        ),
        None,
    )
    if not isinstance(current_fdd, dict):
        raise AssertionError("focused HLAcurrentFDD plan row is absent")
    current_fdd_source = query_rti_work.source_location_text(current_fdd)
    if not current_fdd_source.startswith(
        "cpp/tests/federation_mom_current_fdd_catch2.cpp:"
    ):
        raise AssertionError("focused HLAcurrentFDD source file pointer drifted")
    if current_fdd.get("assertions") != 60:
        raise AssertionError("focused HLAcurrentFDD assertion count drifted")
    current_fdd_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-mom-current-fdd", limit=0
    )
    if current_fdd_focus.get("lane_state") != "complete":
        raise AssertionError("focused HLAcurrentFDD lane is not complete")
    if current_fdd_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused HLAcurrentFDD mapped count drifted")
    if current_fdd_focus.get("assertion_count") != 60:
        raise AssertionError("focused HLAcurrentFDD lane assertion total drifted")
    if current_fdd_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError("focused HLAcurrentFDD direct pair count drifted")
    current_fdd_handles = current_fdd_focus.get("lane_handles")
    if (
        not isinstance(current_fdd_handles, dict)
        or current_fdd_handles.get("catch2_target")
        != "umbra_federation_mom_current_fdd_catch2"
    ):
        raise AssertionError("focused HLAcurrentFDD Catch2 handle drifted")
    # The plan retains a legacy alias with the same Catch2 title; select the
    # focused canonical row explicitly so stale alias metadata cannot win.
    content_reports = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-mom-content-reports-focused-integration"
        ),
        None,
    )
    if not isinstance(content_reports, dict):
        raise AssertionError("focused federation MOM content-report plan row is absent")
    if content_reports.get("assertions") != 55:
        raise AssertionError(
            "focused federation MOM content-report assertion count drifted"
        )
    content_reports_source = query_rti_work.source_location_text(content_reports)
    if not content_reports_source.startswith(
        "cpp/tests/federation_mom_current_fdd_catch2.cpp:"
    ):
        raise AssertionError(
            "focused federation MOM content-report source pointer drifted"
        )
    content_reports_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-mom-content-reports", limit=0
    )
    if content_reports_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused federation MOM content-report lane is not complete"
        )
    if content_reports_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused federation MOM content-report mapped count drifted"
        )
    if content_reports_focus.get("assertion_count") != 55:
        raise AssertionError(
            "focused federation MOM content-report lane assertion total drifted"
        )
    if content_reports_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused federation MOM content-report direct pair count drifted"
        )
    content_reports_handles = content_reports_focus.get("lane_handles")
    if (
        not isinstance(content_reports_handles, dict)
        or content_reports_handles.get("catch2_target")
        != "umbra_federation_mom_current_fdd_catch2"
    ):
        raise AssertionError(
            "focused federation MOM content-report Catch2 handle drifted"
        )
    time_item = next(
        item
        for item in status["roadmap_items"]
        if item.get("id") == "time-save-restore"
    )
    pointer = time_item.get("next_test_pointer")
    if not isinstance(pointer, dict) or pointer.get("state") != "complete":
        raise AssertionError("active baseline pointer is not classified as complete")
    expected = next(
        test for test in tests if test.get("test_case") == pointer.get("query")
    )
    if pointer.get("requirement_count") != len(expected.get("lab_requirement_ids", [])):
        raise AssertionError(
            "baseline pointer requirement count diverges from its test"
        )
    if pointer.get("standard_section_count") != len(
        expected.get("standard_sections", [])
    ):
        raise AssertionError("baseline pointer section count diverges from its test")
    if pointer.get("cpp_api_surface_count") != len(
        expected.get("selected_cpp_api_surface_ids", [])
    ):
        raise AssertionError("baseline pointer API count diverges from its test")
    queue_time_item = next(
        row for row in queue["items"] if row.get("id") == "time-save-restore"
    )
    queue_pointer = queue_time_item.get("next_test_pointer")
    if queue_pointer != query_rti_work.next_test_pointer(time_item, tests):
        raise AssertionError("queue baseline pointer diverges from status pointer")

    # Family status must keep intentional dispositions separate from rows that
    # still need a Lab mapping decision.  This is the compact, plan-derived
    # handoff consumed by ``status --summary``; it must not rely on stale
    # narrative counters in the roadmap index.
    object_item = next(
        item
        for item in status["roadmap_items"]
        if item.get("id") == "object-ddm-ownership"
    )
    object_tests = query_rti_work.item_tests(object_item, tests, index=index)
    object_counts = object_item.get("live_mapping_counts")
    if not isinstance(object_counts, dict):
        raise AssertionError("status family row omitted live mapping counts")
    expected_explicit = sum(
        not test.get("requirements")
        and query_rti_work.traceability_state(test) == "explicit-disposition"
        for test in object_tests
    )
    expected_unclassified = sum(
        not test.get("requirements")
        and query_rti_work.traceability_state(test) == "unclassified"
        for test in object_tests
    )
    if object_counts.get("explicit_disposition_count") != expected_explicit:
        raise AssertionError(
            "status family explicit-disposition count diverges from plan"
        )
    if object_counts.get("unclassified_count") != expected_unclassified:
        raise AssertionError("status family unclassified count diverges from plan")
    object_pairs = {
        (row.get("lab_requirement_id"), row.get("standard_section"))
        for test in object_tests
        for row in query_rti_work.requirement_section_mapping_rows(test)
        if isinstance(row, dict) and row.get("lab_requirement_id")
    }
    if object_counts.get("requirement_section_pair_count") != len(object_pairs):
        raise AssertionError("status family direct pair count diverges from plan")
    if object_counts.get("resolved_requirement_section_pair_count") != sum(
        pair[1] is not None for pair in object_pairs
    ):
        raise AssertionError("status family resolved pair count diverges from plan")
    focus_text = object_item.get("live_current_focus") or ""
    if "explicit dispositions" not in focus_text or "unclassified" not in focus_text:
        raise AssertionError("status family focus text collapsed mapping dispositions")

    ready = query_rti_work.ready_slice(index, tests, source_locations)
    if ready.get("state") == "unplanned-source":
        raise AssertionError(
            "default ready handoff promoted an unrelated source-only declaration"
        )
    if ready.get("state") in {"planned", "mapping"}:
        ready_lane = ready.get("source_lane")
        lane_owners = index.get("mapping", {}).get("lane_owners", {})
        expected_owner = (
            lane_owners.get(ready_lane)
            if isinstance(lane_owners, dict) and isinstance(ready_lane, str)
            else None
        )
        if isinstance(expected_owner, str):
            roadmap_owner = ready.get("roadmap_owner")
            if (
                ready.get("family_id") != expected_owner
                or ready.get("work_id") != expected_owner
                or not isinstance(roadmap_owner, dict)
                or roadmap_owner.get("id") != expected_owner
            ):
                raise AssertionError(
                    "ready handoff family diverges from the exact planned case lane owner"
                )
    if not ready.get("found"):
        options = ready.get("family_options")
        if not isinstance(options, list) or not options:
            raise AssertionError(
                "an exhausted bounded queue must expose at least one open family option"
            )
        if not all(
            isinstance(option, dict) and option.get("work_query") for option in options
        ):
            raise AssertionError(
                "bounded family options must carry copyable work queries"
            )
        if not all(
            isinstance(option, dict) and option.get("next_action") for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an actionable next step"
            )
        if not all(
            isinstance(option, dict) and option.get("lane_discovery_command")
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry a lane-discovery command"
            )
        recommended_option = next(
            (
                option
                for option in options
                if isinstance(option, dict)
                and option.get("id") == ready.get("recommended_family_id")
            ),
            None,
        )
        if not isinstance(recommended_option, dict) or (
            "--focused"
            not in str(recommended_option.get("lane_discovery_command") or "")
            or "--unmapped"
            in str(recommended_option.get("lane_discovery_command") or "")
        ):
            raise AssertionError(
                "recommended family handoff did not use its focused lane selector"
            )
        if not all(
            isinstance(option, dict)
            and option.get("mapping_lane_discovery_command")
            and "--disposition unclassified" in option["mapping_lane_discovery_command"]
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an unclassified mapping command"
            )
        if not all(
            isinstance(option, dict)
            and option.get("gap_query")
            and "query_rti_work.py gaps --family" in option["gap_query"]
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry an exact family gap query"
            )
        if not all(
            isinstance(option, dict)
            and "--group-by requirement"
            in str(option.get("matrix_requirement_command") or "")
            and "--group-by section" in str(option.get("matrix_section_command") or "")
            for option in options
        ):
            raise AssertionError(
                "bounded family options must carry requirement/section matrix commands"
            )

    # The formerly proposed handoff is now a direct mapped case.  Its exact
    # lane must remain queryable after promotion so the completed slice cannot
    # disappear back into a broad family search.
    transport_restore_immediate = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-federation-restore-work-item-ownership-assumption-immediate",
        limit=0,
    )
    if (
        transport_restore_immediate.get("lane_state") != "complete"
        or transport_restore_immediate.get("mapped_test_count") != 1
        or transport_restore_immediate.get("assertion_count") != 52
        or transport_restore_immediate.get("requirement_count") != 11
        or transport_restore_immediate.get("standard_section_count") != 8
        or transport_restore_immediate.get("requirement_section_pair_count") != 11
    ):
        raise AssertionError(
            "completed HLA_IMMEDIATE restore-work-item lane is not queryable"
        )

    active_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-tso-attribute-retraction-reenable",
        status="open",
        limit=4,
    )
    active_families = active_roadmap.get("families")
    if not isinstance(active_families, list) or len(active_families) != 1:
        raise AssertionError("active handoff reverse roadmap lookup drifted")
    lane_matches = active_families[0].get("lane_matches")
    if not isinstance(lane_matches, list) or not any(
        isinstance(match, dict)
        and match.get("tag") == "process-tso-attribute-retraction-reenable"
        and match.get("representative_test_id")
        == "umbra-cpp-process-tso-attribute-retraction-reenable-integration"
        and match.get("requirement_section_pair_count") == 5
        for match in lane_matches
    ):
        raise AssertionError(
            "completed process TSO lane reverse lookup omitted its mapping"
        )

    transport_option = {
        "gap_preview": query_rti_work.family_gap_preview(
            index,
            tests,
            standard_requirements,
            "transport-and-conformance",
        )
    }
    transport_gap = transport_option.get("gap_preview")
    transport_requirement = (
        transport_gap.get("requirement") if isinstance(transport_gap, dict) else None
    )
    if not isinstance(transport_requirement, dict):
        raise AssertionError("new-case family handoff omitted its gap preview")
    expected_transport_gap = expected_family_gap_head(
        index,
        tests,
        standard_requirements,
        "transport-and-conformance",
    )
    expected_transport_requirement = expected_transport_gap.get("requirement")
    expected_transport_section = expected_transport_gap.get("section")
    if (
        not isinstance(expected_transport_requirement, dict)
        or transport_requirement.get("id") != expected_transport_requirement.get("id")
        or transport_requirement.get("document_id") != "hla-1516.1-2025"
        or transport_requirement.get("standard_section") != expected_transport_section
        or "--summary --compact"
        not in str(transport_requirement.get("requirement_query") or "")
        or "--summary --compact"
        not in str(transport_requirement.get("section_query") or "")
    ):
        raise AssertionError(
            "new-case family gap preview omitted exact requirement/section handles"
        )
    transport_gap_text = "\n".join(
        query_rti_work.text_gap_preview(transport_option)
        if isinstance(transport_option, dict)
        else []
    )
    expected_gap_scope = (
        "gap_scope=family-mapped requirements="
        f"{expected_transport_gap['family_mapped_requirement_count']}; "
        f"represented subsections={expected_transport_gap['family_section_count']}; "
        f"{expected_transport_gap['candidate_count']} global gaps in represented subsections "
        f"({expected_transport_gap['preview_candidate_count']} IEEE 1516.1-2025 previewable; "
        f"{expected_transport_gap['out_of_scope_candidate_count']} other-document gaps excluded; "
        f"{expected_transport_gap['inventory_only_count']} inventory-only excluded)"
    )
    if (
        expected_gap_scope not in transport_gap_text
        or f"gap_inventory_head={transport_requirement['id']} -> {expected_transport_section} "
        "(coverage inventory only; not a queued implementation task)"
        not in transport_gap_text
        or f"gap_requirement=python tools/query_rti_work.py requirement {transport_requirement['id']}"
        not in transport_gap_text
        or f"gap_section=python tools/query_rti_work.py section {expected_transport_section}"
        not in transport_gap_text
    ):
        raise AssertionError("new-case family text omitted its bounded gap handles")

    # Family summaries and the bounded gap preview must select the same rows.
    # Lane-owner handles (for example ``interaction-class-handle``) are easy
    # to omit when a family uses only its broad query tags; protect the
    # one-screen counters against that split-brain state.
    transport_inventory = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="transport-and-conformance",
        status="open",
        limit=0,
    )
    transport_families = transport_inventory.get("families")
    if not isinstance(transport_families, list) or len(transport_families) != 1:
        raise AssertionError("transport family exact-id query did not remain singular")
    transport_row = transport_families[0]
    transport_counts = transport_row.get("live_mapping_counts")
    if (
        not isinstance(transport_counts, dict)
        or transport_row.get("matching_test_count")
        != transport_counts.get("case_count")
        or transport_counts.get("requirement_count")
        != transport_gap.get("covered_requirement_count")
    ):
        raise AssertionError(
            "transport family summary and gap preview selected different mapping rows"
        )

    source_only_ready = query_rti_work.ready_slice(
        index,
        tests,
        source_locations,
        include_source_only=True,
    )
    if source_only_ready.get("state") == "active-handoff":
        if source_only_ready.get("plan_id") != active_handoff.get(
            "plan_id"
        ) or not source_only_ready.get("source_location"):
            raise AssertionError(
                "explicit source-only opt-in displaced or dropped the active handoff"
            )
    elif source_only_ready.get("state") == "unplanned-source":
        if not source_only_ready.get("source_location"):
            raise AssertionError(
                "explicit source-only handoff omitted its source location"
            )
    elif source_only_ready.get("state") not in {
        "none",
        "planned",
        "new-case-needed",
        "verification-needed",
    }:
        raise AssertionError(
            "explicit source-only opt-in returned an unexpected queue state"
        )

    scoped_ready = query_rti_work.ready_slice(
        index,
        tests,
        source_locations,
        "object-ddm-ownership",
    )
    if scoped_ready.get("found"):
        if scoped_ready.get("requested_family") != "object-ddm-ownership":
            raise AssertionError(
                "ready --family returned a handoff for a different family"
            )
        owner = scoped_ready.get("owner")
        roadmap_owner = scoped_ready.get("roadmap_owner")
        if not isinstance(owner, dict) or owner.get("id") != "object-ddm-ownership":
            raise AssertionError(
                "ready --family did not retain the selected family as owner"
            )
        if (
            not isinstance(roadmap_owner, dict)
            or roadmap_owner.get("id") != "object-ddm-ownership"
        ):
            raise AssertionError("ready --family returned a different roadmap owner")
        if not scoped_ready.get("next_action"):
            raise AssertionError("selected family handoff omitted its next action")
        if scoped_ready.get("state") == "mapping":
            for field in (
                "plan_id",
                "source_location",
                "trace_command",
                "focus_command",
                "check_command",
            ):
                if not scoped_ready.get(field):
                    raise AssertionError(f"mapping handoff omitted its {field} handle")
        if scoped_ready.get("state") == "planned" and (
            scoped_ready.get("plan_id")
            != "umbra-cpp-confirm-divestiture-process-assumption-integration"
            or scoped_ready.get("source_lane")
            != "process-confirm-divestiture-assumption"
            or not scoped_ready.get("ctest_filter")
            or len(scoped_ready.get("requirement_ids", [])) != 1
            or len(scoped_ready.get("standard_sections", [])) != 1
        ):
            raise AssertionError("planned family handoff omitted exact mapping handles")
    else:
        scoped_options = scoped_ready.get("family_options")
        if not isinstance(scoped_options, list) or [
            option.get("id") for option in scoped_options if isinstance(option, dict)
        ] != ["object-ddm-ownership"]:
            raise AssertionError(
                "ready --family did not keep an exhausted-family response scoped"
            )
        if not scoped_options[0].get("next_action"):
            raise AssertionError("selected family response omitted its next action")
        if not scoped_options[0].get("gap_query"):
            raise AssertionError("selected family response omitted its gap query")

    object_work = query_rti_work.indexed_work_slice(
        index, tests, "object-ddm-ownership", source_locations
    )
    object_work_commands = object_work.get("commands", [])
    if not any(
        isinstance(command, str) and "lanes --family object-ddm-ownership" in command
        for command in object_work_commands
    ):
        raise AssertionError(
            "family work query omitted its bounded lane-discovery command"
        )
    if not any(
        isinstance(command, str) and "ready --family object-ddm-ownership" in command
        for command in object_work_commands
    ):
        raise AssertionError("family work query omitted its scoped ready command")

    # An explicitly requested family with its own current action must not
    # inherit the stale pointer text/counts of a broader parent that happens
    # to reference it through ``next_work_id``.
    transport_work = query_rti_work.indexed_work_slice(
        index, tests, "transport-and-conformance", source_locations
    )
    transport_parent = transport_work.get("parent_item")
    transport_target = transport_work.get("work_item")
    transport_index_item = next(
        (
            item
            for item in index.get("items", [])
            if isinstance(item, dict) and item.get("id") == "transport-and-conformance"
        ),
        None,
    )
    transport_current_action = (
        query_rti_work.live_next_action(transport_index_item)
        if isinstance(transport_index_item, dict)
        else None
    )
    if (
        not isinstance(transport_parent, dict)
        or transport_parent.get("id") != "transport-and-conformance"
        or not isinstance(transport_target, dict)
        or transport_target.get("id") != "transport-and-conformance"
        or not isinstance(transport_current_action, str)
        or transport_work.get("task") != transport_current_action
    ):
        raise AssertionError(
            "explicit transport family work query inherited a stale parent pointer"
        )

    api_kind, api_matches = query_rti_work.resolve_trace_query(
        tests, "api.2025.cpp.rtiambassador.querylogicaltime.cb29c063787c"
    )
    if api_kind != "api" or len(api_matches) < 2:
        raise AssertionError(
            "exact C++ API surface lookup did not resolve the Query Logical Time rows"
        )

    family_item, family_tests = query_rti_work.indexed_item(
        index, tests, "object-ddm-ownership"
    )
    if family_item is None or not family_tests:
        raise AssertionError("indexed roadmap family did not resolve to tagged tests")
    family_matrix_kind, family_matrix_matches = query_rti_work.resolve_matrix_query(
        index, tests, "OBJECT-DDM-OWNERSHIP"
    )
    if family_matrix_kind != "family":
        raise AssertionError("matrix did not resolve an indexed roadmap family id")
    if family_matrix_matches != family_tests:
        raise AssertionError(
            "family matrix resolution diverged from indexed item tests"
        )
    if not any(test.get("test_case") == TARGET_TEST for test in family_matrix_matches):
        raise AssertionError("target test is not reachable through its roadmap family")
    scoped_family, scoped_rows = query_rti_work.scoped_plan_tests(
        index, tests, family="object-ddm-ownership"
    )
    if scoped_family != family_item or scoped_rows != family_tests:
        raise AssertionError("family scope resolution diverged from item/matrix rows")

    # The aggregate matrix views are a standards-review shortcut, not a
    # second mapping source.  Protect both groupings so a requirement or
    # subsection review can find the exact case ids without expanding the
    # whole family matrix.
    target_family_test = next(
        (test for test in family_tests if test.get("test_case") == TARGET_TEST),
        None,
    )
    if not isinstance(target_family_test, dict):
        raise AssertionError("family crosswalk target test is not indexed")
    target_requirement = next(
        iter(query_rti_work.strings(target_family_test.get("lab_requirement_ids"))),
        None,
    )
    target_section = next(
        iter(query_rti_work.strings(target_family_test.get("standard_sections"))),
        None,
    )
    if not target_requirement or not target_section:
        raise AssertionError("family crosswalk target lost direct mapping handles")
    requirement_crosswalk = query_rti_work.matrix_crosswalk_rows(
        family_tests, "requirement"
    )
    requirement_row = next(
        (row for row in requirement_crosswalk if row.get("key") == target_requirement),
        None,
    )
    if not isinstance(requirement_row, dict):
        raise AssertionError(
            "requirement-centric matrix crosswalk lost target requirement"
        )
    if requirement_row.get("test_count", 0) < 1:
        raise AssertionError("requirement-centric crosswalk lost target case count")
    # The preview is intentionally capped for cross-cutting requirements; an
    # exact case id remains available through ``matrix <case-id>``.
    preview_ids = {
        test.get("id")
        for test in requirement_row.get("tests", [])
        if isinstance(test, dict)
    }
    if target_family_test.get("id") not in preview_ids and not isinstance(
        requirement_row.get("remaining_test_count"), int
    ):
        raise AssertionError("requirement-centric crosswalk lost bounded case preview")
    if target_section not in requirement_row.get("standard_sections", []):
        raise AssertionError("requirement-centric crosswalk lost target subsection")
    exact_requirement_crosswalk = query_rti_work.matrix_crosswalk_rows(
        family_tests,
        "requirement",
        match_kind="requirement",
        query=target_requirement,
    )
    if (
        len(exact_requirement_crosswalk) != 1
        or exact_requirement_crosswalk[0].get("key") != target_requirement
    ):
        raise AssertionError("exact requirement matrix lookup was not narrowed")
    section_crosswalk = query_rti_work.matrix_crosswalk_rows(family_tests, "section")
    section_row = next(
        (row for row in section_crosswalk if row.get("key") == target_section),
        None,
    )
    if not isinstance(section_row, dict):
        raise AssertionError(
            "subsection-centric matrix crosswalk lost target subsection"
        )
    if target_requirement not in section_row.get("lab_requirement_ids", []):
        raise AssertionError("subsection-centric crosswalk lost target requirement")
    if query_rti_work.trace_section_match(
        "hla-1516.1-2025:clause-4.11", "hla-1516.1-2025:clause-4.1.1"
    ):
        raise AssertionError("dotted subsection lookup merged 4.1.1 with 4.11")
    exact_section_crosswalk = query_rti_work.matrix_crosswalk_rows(
        family_tests,
        "section",
        match_kind="section",
        query=target_section,
    )
    if (
        len(exact_section_crosswalk) != 1
        or exact_section_crosswalk[0].get("key") != target_section
    ):
        raise AssertionError("exact subsection matrix lookup was not narrowed")

    mixed_restore_case = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-public-fresh-registry-federation-save-restore-filesystem-pending-mixed-negotiated-ownership-integration"
        ),
        None,
    )
    if (
        not isinstance(mixed_restore_case, dict)
        or mixed_restore_case.get("assertions") is not None
    ):
        raise AssertionError(
            "mixed restore case should keep its assertion total unspecified"
        )
    mixed_restore_rows = query_rti_work.matrix_crosswalk_rows(
        [mixed_restore_case], "requirement"
    )
    if not mixed_restore_rows or any(
        row.get("assertions_unspecified_test_count") != 1
        or "assertions=<unspecified>"
        not in query_rti_work.text_matrix_crosswalk_row(row)
        for row in mixed_restore_rows
    ):
        raise AssertionError(
            "matrix should not display an unrecorded assertion total as zero"
        )

    # Roadmap discovery is intentionally a separate bounded index.  Keep one
    # thematic query protected so a contributor can find the process family
    # and its exact work/focus/matrix handles without enumerating all lanes or
    # reopening the unchanged Requirements Lab.
    roadmap_process = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process",
        status="open",
        limit=0,
    )
    if roadmap_process.get("count") != len(roadmap_process.get("families", [])):
        raise AssertionError("roadmap search count diverges from its unbounded rows")
    process_family_row = next(
        (
            row
            for row in roadmap_process.get("families", [])
            if isinstance(row, dict) and row.get("id") == "transport-and-conformance"
        ),
        None,
    )
    if not isinstance(process_family_row, dict):
        raise AssertionError("roadmap process search lost the transport family")
    if process_family_row.get("next_lane") != "process-boundary":
        raise AssertionError("roadmap process row lost its exact next lane")
    if process_family_row.get("action_state") not in {
        "planned",
        "new-case-needed",
    }:
        raise AssertionError(
            "roadmap process row lost its current planned-or-next-case action state"
        )
    process_commands = process_family_row.get("commands")
    if not isinstance(process_commands, dict) or not all(
        process_commands.get(name) for name in ("work", "matrix")
    ):
        raise AssertionError("roadmap process row lost work/matrix handles")
    process_counts = process_family_row.get("live_mapping_counts")
    if not isinstance(process_counts, dict) or not process_counts.get(
        "requirement_count"
    ):
        raise AssertionError("roadmap process row lost live requirement counts")

    # A family id is an exact, stable handle.  It must not expand into other
    # families that happen to carry the same cross-cutting tag.
    exact_process_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="transport-and-conformance",
        status="open",
        limit=0,
    )
    exact_process_rows = exact_process_roadmap.get("families", [])
    if exact_process_roadmap.get("count") != 1 or len(exact_process_rows) != 1:
        raise AssertionError(
            "exact transport family-id lookup expanded beyond one roadmap family"
        )
    if exact_process_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "exact transport family-id lookup resolved the wrong family"
        )

    callback_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="multi-federate-callback-ordering",
        status="open",
        limit=0,
    )
    callback_rows = callback_roadmap.get("families", [])
    if callback_roadmap.get("count") != 1 or len(callback_rows) != 1:
        raise AssertionError(
            "roadmap callback family query lost its exact family handle"
        )
    if callback_rows[0].get("action_state") != "evidence-complete":
        raise AssertionError("roadmap callback family was exposed as a runnable action")

    constrained_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-time-constrained-pending",
        status="open",
        limit=0,
    )
    constrained_rows = constrained_roadmap.get("families", [])
    if constrained_roadmap.get("count") != 1 or len(constrained_rows) != 1:
        raise AssertionError(
            "roadmap exact constrained-pending lane query lost its family handle"
        )
    if constrained_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap constrained-pending lane resolved to the wrong owner"
        )

    malformed_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-malformed-encoding",
        status="open",
        limit=0,
    )
    malformed_rows = malformed_roadmap.get("families", [])
    if malformed_roadmap.get("count") != 1 or len(malformed_rows) != 1:
        raise AssertionError(
            "roadmap exact malformed-encoding lane query lost its family handle"
        )
    if malformed_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap malformed-encoding lane resolved to the wrong owner"
        )

    scheduler_roadmap = query_rti_work.roadmap_inventory(
        index,
        tests,
        query="process-time-advance-federation-scheduler",
        status="open",
        limit=0,
    )
    scheduler_rows = scheduler_roadmap.get("families", [])
    if scheduler_roadmap.get("count") != 1 or len(scheduler_rows) != 1:
        raise AssertionError(
            "roadmap exact federation-scheduler query lost its family handle"
        )
    if scheduler_rows[0].get("id") != "transport-and-conformance":
        raise AssertionError(
            "roadmap federation-scheduler query resolved to the wrong owner"
        )

    scheduler_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-advance-federation-scheduler", limit=1
    )
    if scheduler_focus.get("lane_state") != "complete":
        raise AssertionError("process federation-scheduler lane is not complete")
    if scheduler_focus.get("mapped_test_count") != 7:
        raise AssertionError("process federation-scheduler mapped count drifted")
    if scheduler_focus.get("assertion_count") != 30:
        raise AssertionError("process federation-scheduler assertion total drifted")
    if scheduler_focus.get("requirement_section_pair_count") != 9:
        raise AssertionError("process federation-scheduler mapping pair count drifted")
    if set(scheduler_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-8.10.2",
        "hla-1516.1-2025:clause-8.11",
        "hla-1516.1-2025:clause-8.11.3",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5",
        "hla-1516.1-2025:clause-8.6",
        "hla-1516.1-2025:clause-8.8",
        "hla-1516.1-2025:clause-8.9",
    }:
        raise AssertionError("process federation-scheduler section preview drifted")
    scheduler_handles = scheduler_focus.get("lane_handles")
    if (
        not isinstance(scheduler_handles, dict)
        or scheduler_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("process federation-scheduler Catch2 handle drifted")

    tso_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-interaction-before-grant", limit=1
    )
    if tso_focus.get("lane_state") != "complete":
        raise AssertionError("process TSO interaction lane is not complete")
    if tso_focus.get("mapped_test_count") != 1:
        raise AssertionError("process TSO interaction mapped count drifted")
    if tso_focus.get("assertion_count") != 62:
        raise AssertionError("process TSO interaction assertion total drifted")
    if tso_focus.get("requirement_section_pair_count") != 20:
        raise AssertionError("process TSO interaction mapping pair count drifted")
    if set(tso_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-10.59.4",
        "hla-1516.1-2025:clause-4",
        "hla-1516.1-2025:clause-5.1.3",
        "hla-1516.1-2025:clause-6.12",
        "hla-1516.1-2025:clause-6.13",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.1.6",
        "hla-1516.1-2025:clause-8.17",
        "hla-1516.1-2025:clause-8.19",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5",
        "hla-1516.1-2025:clause-8.6",
        "hla-1516.1-2025:clause-8.8",
    }:
        raise AssertionError("process TSO interaction section preview drifted")
    tso_handles = tso_focus.get("lane_handles")
    if (
        not isinstance(tso_handles, dict)
        or tso_handles.get("catch2_target") != "umbra_ieee1516_2025_connection_catch2"
    ):
        raise AssertionError("process TSO interaction Catch2 handle drifted")

    tso_in_transit_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-in-transit", limit=1
    )
    if (
        tso_in_transit_focus.get("lane_state") != "complete"
        or tso_in_transit_focus.get("roadmap_owner") != "transport-and-conformance"
        or tso_in_transit_focus.get("mapped_test_count") != 4
        or tso_in_transit_focus.get("assertion_count") != 62
        or tso_in_transit_focus.get("requirement_section_pair_count") != 24
        or tso_in_transit_focus.get("standard_section_count") != 17
    ):
        raise AssertionError("process TSO in-transit lane card drifted")
    tso_in_transit_handles = tso_in_transit_focus.get("lane_handles")
    if (
        not isinstance(tso_in_transit_handles, dict)
        or tso_in_transit_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_in_transit_handles.get("ctest_label") != "process-boundary"
    ):
        raise AssertionError("process TSO in-transit Catch2 handle drifted")

    tso_attribute_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-before-grant", limit=1
    )
    if tso_attribute_focus.get("lane_state") != "complete":
        raise AssertionError("process TSO attribute lane is not complete")
    if tso_attribute_focus.get("roadmap_owner") != "transport-and-conformance":
        raise AssertionError("process TSO attribute lane owner drifted")
    if tso_attribute_focus.get("mapped_test_count") != 2:
        raise AssertionError("process TSO attribute mapped count drifted")
    if tso_attribute_focus.get("assertion_count") != 114:
        raise AssertionError("process TSO attribute assertion total drifted")
    if tso_attribute_focus.get("requirement_section_pair_count") != 17:
        raise AssertionError("process TSO attribute mapping pair count drifted")
    if set(tso_attribute_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-5.8",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.8.4",
        "hla-1516.1-2025:clause-6.9.3",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.1.6",
        "hla-1516.1-2025:clause-8.17",
        "hla-1516.1-2025:clause-8.19",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.5",
        "hla-1516.1-2025:clause-8.6",
        "hla-1516.1-2025:clause-8.8",
    }:
        raise AssertionError("process TSO attribute section preview drifted")
    tso_attribute_handles = tso_attribute_focus.get("lane_handles")
    if not isinstance(tso_attribute_handles, dict) or tso_attribute_handles.get(
        "catch2_targets"
    ) != [
        "umbra_process_boundary_private_catch2",
        "umbra_ieee1516_2025_connection_catch2",
    ]:
        raise AssertionError("process TSO attribute Catch2 handles drifted")

    tso_attribute_retraction_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-before-callback", limit=1
    )
    if (
        tso_attribute_retraction_focus.get("lane_state") != "complete"
        or tso_attribute_retraction_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_retraction_focus.get("mapped_test_count") != 1
        or tso_attribute_retraction_focus.get("assertion_count") != 51
        or tso_attribute_retraction_focus.get("recorded_assertion_count") != 51
        or tso_attribute_retraction_focus.get("requirement_section_pair_count") != 20
        or tso_attribute_retraction_focus.get("standard_section_count") != 14
    ):
        raise AssertionError("process TSO attribute retraction lane card drifted")
    if set(tso_attribute_retraction_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-5.8",
        "hla-1516.1-2025:clause-6.10",
        "hla-1516.1-2025:clause-6.8.4",
        "hla-1516.1-2025:clause-6.9.3",
        "hla-1516.1-2025:clause-8",
        "hla-1516.1-2025:clause-8.1.5",
        "hla-1516.1-2025:clause-8.1.6",
        "hla-1516.1-2025:clause-8.17",
        "hla-1516.1-2025:clause-8.19",
        "hla-1516.1-2025:clause-8.2",
        "hla-1516.1-2025:clause-8.22.3",
        "hla-1516.1-2025:clause-8.5",
        "hla-1516.1-2025:clause-8.6",
        "hla-1516.1-2025:clause-8.8",
    }:
        raise AssertionError("process TSO attribute retraction section preview drifted")
    tso_attribute_retraction_handles = tso_attribute_retraction_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(tso_attribute_retraction_handles, dict)
        or tso_attribute_retraction_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_retraction_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors suppress a retracted timestamped process attribute before the callback"
        not in tso_attribute_retraction_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute retraction handle drifted")

    tso_attribute_fanout_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-fanout", limit=1
    )
    if (
        tso_attribute_fanout_focus.get("lane_state") != "complete"
        or tso_attribute_fanout_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_fanout_focus.get("mapped_test_count") != 1
        or tso_attribute_fanout_focus.get("assertion_count") != 100
        or tso_attribute_fanout_focus.get("recorded_assertion_count") != 100
        or tso_attribute_fanout_focus.get("requirement_section_pair_count") != 20
        or tso_attribute_fanout_focus.get("standard_section_count") != 14
    ):
        raise AssertionError("process TSO attribute fanout lane card drifted")
    tso_attribute_fanout_handles = tso_attribute_fanout_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_fanout_handles, dict)
        or tso_attribute_fanout_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_fanout_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors fan out retained timestamped process attributes around a retracted middle update"
        not in tso_attribute_fanout_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute fanout handle drifted")

    tso_attribute_regional = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-retraction-regional-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_regional, dict):
        raise AssertionError("process TSO attribute regional plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_regional) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_regional_catch2.cpp:459"
    ):
        raise AssertionError("process TSO attribute regional source pointer drifted")
    if (
        tso_attribute_regional.get("assertions") != 122
        or tso_attribute_regional.get("primary_lane")
        != "process-tso-attribute-retraction-regional"
        or tso_attribute_regional.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError("process TSO attribute regional plan evidence drifted")
    tso_attribute_regional_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-regional", limit=0
    )
    if (
        tso_attribute_regional_focus.get("lane_state") != "complete"
        or tso_attribute_regional_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_regional_focus.get("mapped_test_count") != 1
        or tso_attribute_regional_focus.get("assertion_count") != 122
        or tso_attribute_regional_focus.get("recorded_assertion_count") != 122
        or tso_attribute_regional_focus.get("requirement_count") != 25
        or tso_attribute_regional_focus.get("standard_section_count") != 18
        or tso_attribute_regional_focus.get("requirement_section_pair_count") != 25
    ):
        raise AssertionError("process TSO attribute regional lane card drifted")
    tso_attribute_regional_handles = tso_attribute_regional_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_regional_handles, dict)
        or tso_attribute_regional_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_regional_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve regional scope while retracting a middle timestamped process attribute"
        not in tso_attribute_regional_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute regional execution handle drifted")
    tso_attribute_regional_case = query_rti_work.case_card_record(
        tso_attribute_regional,
        index,
        "umbra-cpp-process-tso-attribute-retraction-regional-integration",
    )
    if (
        tso_attribute_regional_case.get("lane")
        != "process-tso-attribute-retraction-regional"
        or tso_attribute_regional_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO regional case card lost its CTest handle"
        )

    tso_attribute_reenable = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-retraction-reenable-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_reenable, dict):
        raise AssertionError("process TSO attribute re-enable plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_reenable) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_attribute_retraction_reenable_catch2.cpp:459"
    ):
        raise AssertionError("process TSO attribute re-enable source pointer drifted")
    if (
        tso_attribute_reenable.get("assertions") != 48
        or tso_attribute_reenable.get("primary_lane")
        != "process-tso-attribute-retraction-reenable"
        or tso_attribute_reenable.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError("process TSO attribute re-enable plan evidence drifted")
    tso_attribute_reenable_focus = query_rti_work.focused_lane_result(
        index, tests, "process-tso-attribute-retraction-reenable", limit=0
    )
    if (
        tso_attribute_reenable_focus.get("lane_state") != "complete"
        or tso_attribute_reenable_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_reenable_focus.get("mapped_test_count") != 1
        or tso_attribute_reenable_focus.get("assertion_count") != 48
        or tso_attribute_reenable_focus.get("recorded_assertion_count") != 48
        or tso_attribute_reenable_focus.get("requirement_count") != 5
        or tso_attribute_reenable_focus.get("standard_section_count") != 4
        or tso_attribute_reenable_focus.get("requirement_section_pair_count") != 5
    ):
        raise AssertionError("process TSO attribute re-enable lane card drifted")
    tso_attribute_reenable_handles = tso_attribute_reenable_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_reenable_handles, dict)
        or tso_attribute_reenable_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_reenable_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process attribute retraction designator across time-regulation re-enable"
        not in tso_attribute_reenable_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO attribute re-enable execution handle drifted")
    tso_attribute_reenable_case = query_rti_work.case_card_record(
        tso_attribute_reenable,
        index,
        "umbra-cpp-process-tso-attribute-retraction-reenable-integration",
    )
    if (
        tso_attribute_reenable_case.get("lane")
        != "process-tso-attribute-retraction-reenable"
        or tso_attribute_reenable_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO attribute re-enable case card lost its CTest handle"
        )

    tso_attribute_changed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-attribute-update-regulation-reenable-changed-lookahead-integration"
        ),
        None,
    )
    if not isinstance(tso_attribute_changed, dict):
        raise AssertionError("process TSO changed-lookahead plan row is absent")
    if query_rti_work.source_location_text(tso_attribute_changed) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_attribute_update_regulation_reenable_changed_lookahead_catch2.cpp:459"
    ):
        raise AssertionError("process TSO changed-lookahead source pointer drifted")
    if (
        tso_attribute_changed.get("assertions") != 70
        or tso_attribute_changed.get("primary_lane")
        != "process-tso-attribute-update-regulation-reenable-changed-lookahead"
        or tso_attribute_changed.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError("process TSO changed-lookahead plan evidence drifted")
    tso_attribute_changed_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-attribute-update-regulation-reenable-changed-lookahead",
        limit=0,
    )
    if (
        tso_attribute_changed_focus.get("lane_state") != "complete"
        or tso_attribute_changed_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_attribute_changed_focus.get("mapped_test_count") != 1
        or tso_attribute_changed_focus.get("assertion_count") != 70
        or tso_attribute_changed_focus.get("recorded_assertion_count") != 70
        or tso_attribute_changed_focus.get("requirement_count") != 11
        or tso_attribute_changed_focus.get("standard_section_count") != 8
        or tso_attribute_changed_focus.get("requirement_section_pair_count") != 11
    ):
        raise AssertionError("process TSO changed-lookahead lane card drifted")
    tso_attribute_changed_handles = tso_attribute_changed_focus.get("lane_handles")
    if (
        not isinstance(tso_attribute_changed_handles, dict)
        or tso_attribute_changed_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_attribute_changed_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process attribute update across time-regulation re-enable with changed lookahead"
        not in tso_attribute_changed_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO changed-lookahead execution handle drifted")
    tso_attribute_changed_case = query_rti_work.case_card_record(
        tso_attribute_changed,
        index,
        "umbra-cpp-process-tso-attribute-update-regulation-reenable-changed-lookahead-integration",
    )
    if (
        tso_attribute_changed_case.get("lane")
        != "process-tso-attribute-update-regulation-reenable-changed-lookahead"
        or tso_attribute_changed_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO changed-lookahead case card lost its CTest handle"
        )

    tso_interaction_changed = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-interaction-regulation-reenable-changed-lookahead-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_changed, dict):
        raise AssertionError(
            "process TSO interaction changed-lookahead plan row is absent"
        )
    if query_rti_work.source_location_text(tso_interaction_changed) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_interaction_regulation_reenable_changed_lookahead_catch2.cpp:459"
    ):
        raise AssertionError(
            "process TSO interaction changed-lookahead source pointer drifted"
        )
    if (
        tso_interaction_changed.get("assertions") != 67
        or tso_interaction_changed.get("primary_lane")
        != "process-tso-interaction-regulation-reenable-changed-lookahead"
        or tso_interaction_changed.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError(
            "process TSO interaction changed-lookahead plan evidence drifted"
        )
    tso_interaction_changed_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-regulation-reenable-changed-lookahead",
        limit=0,
    )
    if (
        tso_interaction_changed_focus.get("lane_state") != "complete"
        or tso_interaction_changed_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_changed_focus.get("mapped_test_count") != 1
        or tso_interaction_changed_focus.get("assertion_count") != 67
        or tso_interaction_changed_focus.get("recorded_assertion_count") != 67
        or tso_interaction_changed_focus.get("requirement_count") != 30
        or tso_interaction_changed_focus.get("standard_section_count") != 17
        or tso_interaction_changed_focus.get("requirement_section_pair_count") != 30
    ):
        raise AssertionError(
            "process TSO interaction changed-lookahead lane card drifted"
        )
    tso_interaction_changed_handles = tso_interaction_changed_focus.get("lane_handles")
    if (
        not isinstance(tso_interaction_changed_handles, dict)
        or tso_interaction_changed_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_changed_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors preserve a timestamped process interaction across time-regulation re-enable with changed lookahead"
        not in tso_interaction_changed_handles.get("ctest_filter", "")
    ):
        raise AssertionError(
            "process TSO interaction changed-lookahead execution handle drifted"
        )
    tso_interaction_changed_case = query_rti_work.case_card_record(
        tso_interaction_changed,
        index,
        "umbra-cpp-process-tso-interaction-regulation-reenable-changed-lookahead-integration",
    )
    if (
        tso_interaction_changed_case.get("lane")
        != "process-tso-interaction-regulation-reenable-changed-lookahead"
        or tso_interaction_changed_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO interaction changed-lookahead case card lost its CTest handle"
        )

    tso_interaction_multiple = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-process-tso-interaction-multiple-message-ordering-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_multiple, dict):
        raise AssertionError("process TSO multiple-message plan row is absent")
    if query_rti_work.source_location_text(tso_interaction_multiple) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_interaction_multiple_message_ordering_catch2.cpp:459"
    ):
        raise AssertionError("process TSO multiple-message source pointer drifted")
    if (
        tso_interaction_multiple.get("assertions") != 51
        or tso_interaction_multiple.get("primary_lane")
        != "process-tso-interaction-multiple-message-ordering"
        or tso_interaction_multiple.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError("process TSO multiple-message plan evidence drifted")
    tso_interaction_multiple_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-multiple-message-ordering",
        limit=0,
    )
    if (
        tso_interaction_multiple_focus.get("lane_state") != "complete"
        or tso_interaction_multiple_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_multiple_focus.get("mapped_test_count") != 1
        or tso_interaction_multiple_focus.get("assertion_count") != 51
        or tso_interaction_multiple_focus.get("recorded_assertion_count") != 51
        or tso_interaction_multiple_focus.get("requirement_count") != 20
        or tso_interaction_multiple_focus.get("standard_section_count") != 14
        or tso_interaction_multiple_focus.get("requirement_section_pair_count") != 20
    ):
        raise AssertionError("process TSO multiple-message lane card drifted")
    tso_interaction_multiple_handles = tso_interaction_multiple_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(tso_interaction_multiple_handles, dict)
        or tso_interaction_multiple_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_multiple_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors deliver multiple timestamped process interactions in FIFO order before one grant"
        not in tso_interaction_multiple_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO multiple-message execution handle drifted")
    tso_interaction_multiple_case = query_rti_work.case_card_record(
        tso_interaction_multiple,
        index,
        "umbra-cpp-process-tso-interaction-multiple-message-ordering-integration",
    )
    if (
        tso_interaction_multiple_case.get("lane")
        != "process-tso-interaction-multiple-message-ordering"
        or tso_interaction_multiple_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO multiple-message case card lost its CTest handle"
        )

    tso_interaction_fanout = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-process-tso-interaction-fanout-integration"
        ),
        None,
    )
    if not isinstance(tso_interaction_fanout, dict):
        raise AssertionError("process TSO fan-out plan row is absent")
    if query_rti_work.source_location_text(tso_interaction_fanout) != (
        "cpp/tests/ieee1516_2025_connection_process_tso_interaction_fanout_catch2.cpp:459"
    ):
        raise AssertionError("process TSO fan-out source pointer drifted")
    if (
        tso_interaction_fanout.get("assertions") != 133
        or tso_interaction_fanout.get("primary_lane")
        != "process-tso-interaction-fanout"
        or tso_interaction_fanout.get("callback_gate_modes")
        != ["disabled-queued-reenabled"]
        or tso_interaction_fanout.get("traceability_state") != "requirements-mapped"
    ):
        raise AssertionError("process TSO fan-out plan evidence drifted")
    tso_interaction_fanout_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "process-tso-interaction-fanout",
        limit=0,
    )
    if (
        tso_interaction_fanout_focus.get("lane_state") != "complete"
        or tso_interaction_fanout_focus.get("roadmap_owner")
        != "transport-and-conformance"
        or tso_interaction_fanout_focus.get("mapped_test_count") != 1
        or tso_interaction_fanout_focus.get("assertion_count") != 133
        or tso_interaction_fanout_focus.get("recorded_assertion_count") != 133
        or tso_interaction_fanout_focus.get("requirement_count") != 20
        or tso_interaction_fanout_focus.get("standard_section_count") != 14
        or tso_interaction_fanout_focus.get("requirement_section_pair_count") != 20
    ):
        raise AssertionError("process TSO fan-out lane card drifted")
    tso_interaction_fanout_handles = tso_interaction_fanout_focus.get("lane_handles")
    if (
        not isinstance(tso_interaction_fanout_handles, dict)
        or tso_interaction_fanout_handles.get("catch2_target")
        != "umbra_ieee1516_2025_connection_catch2"
        or tso_interaction_fanout_handles.get("ctest_label") != "process-boundary"
        or "RTIambassadors fan out multiple timestamped process interactions FIFO to every subscribed receiver before one grant"
        not in tso_interaction_fanout_handles.get("ctest_filter", "")
    ):
        raise AssertionError("process TSO fan-out execution handle drifted")
    tso_interaction_fanout_case = query_rti_work.case_card_record(
        tso_interaction_fanout,
        index,
        "umbra-cpp-process-tso-interaction-fanout-integration",
    )
    if (
        tso_interaction_fanout_case.get("lane") != "process-tso-interaction-fanout"
        or tso_interaction_fanout_case.get("commands", {}).get("ctest") is None
    ):
        raise AssertionError(
            "exact process TSO fan-out case card lost its CTest handle"
        )

    process_time_focus = query_rti_work.focused_lane_result(
        index, tests, "process-time-role", limit=1
    )
    if process_time_focus.get("requirement_section_pair_count") != 3:
        raise AssertionError("process-time-role direct mapping pair count drifted")
    if set(process_time_focus.get("standard_sections", [])) != {
        "hla-1516.1-2025:clause-8.5",
        "hla-1516.1-2025:clause-8.6",
        "hla-1516.1-2025:clause-8.7",
    }:
        raise AssertionError("process-time-role section preview drifted")
