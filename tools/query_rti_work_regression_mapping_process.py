# ruff: noqa: TRY004
# These assertions report broken query-result contracts, not bad caller inputs.
from __future__ import annotations

import query_rti_work
from query_rti_work_regression_context import MappingLookupInputs


def run_process_mapping_rows(inputs: MappingLookupInputs) -> None:
    index = inputs.index
    tests = inputs.tests
    public_process_restart = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-public-process-restart-application-value-focused"
        ),
        None,
    )
    if not isinstance(public_process_restart, dict):
        raise AssertionError(
            "focused public process-restart application-value row is absent"
        )
    if query_rti_work.source_location_text(public_process_restart) != (
        "cpp/tests/public_process_restart_application_value_catch2.cpp:189"
    ):
        raise AssertionError(
            "focused public process-restart application-value source pointer drifted"
        )
    if public_process_restart.get("assertions") != 82:
        raise AssertionError(
            "focused public process-restart application-value assertion count drifted"
        )
    if public_process_restart.get("traceability_state") != "requirements-mapped":
        raise AssertionError(
            "focused public process-restart application-value row is not mapped"
        )
    public_process_restart_focus = query_rti_work.focused_lane_result(
        index, tests, "public-process-restart-application-value-focused", limit=0
    )
    if public_process_restart_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused public process-restart application-value lane is not complete"
        )
    if public_process_restart_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused public process-restart application-value mapped count drifted"
        )
    if public_process_restart_focus.get("assertion_count") != 82:
        raise AssertionError(
            "focused public process-restart application-value lane assertion total drifted"
        )
    if public_process_restart_focus.get("requirement_count") != 9:
        raise AssertionError(
            "focused public process-restart application-value requirement total drifted"
        )
    if public_process_restart_focus.get("standard_section_count") != 5:
        raise AssertionError(
            "focused public process-restart application-value section total drifted"
        )
    public_process_restart_handles = public_process_restart_focus.get("lane_handles")
    if (
        not isinstance(public_process_restart_handles, dict)
        or public_process_restart_handles.get("catch2_target")
        != "umbra_public_process_restart_application_value_catch2"
    ):
        raise AssertionError(
            "focused public process-restart application-value target handle drifted"
        )
    register_sync = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-service-report-file-register-federation-synchronization-point-integration"
        ),
        None,
    )
    if not isinstance(register_sync, dict):
        raise AssertionError("focused synchronization-point report row is absent")
    if query_rti_work.source_location_text(register_sync) != (
        "cpp/tests/service_report_file_confirm_synchronization_point_registration_catch2.cpp:220"
    ):
        raise AssertionError(
            "focused synchronization-point report source pointer drifted"
        )
    if register_sync.get("assertions") != 18:
        raise AssertionError(
            "focused synchronization-point report assertion count drifted"
        )
    if register_sync.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused synchronization-point report row is not mapped")
    register_sync_focus = query_rti_work.focused_lane_result(
        index,
        tests,
        "register-federation-synchronization-point-service-report",
        limit=0,
    )
    if register_sync_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused synchronization-point report lane is not complete"
        )
    if register_sync_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused synchronization-point report mapped count drifted"
        )
    if register_sync_focus.get("assertion_count") != 18:
        raise AssertionError(
            "focused synchronization-point report lane assertion total drifted"
        )
    save_history = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-federation-save-commit-filesystem-process-restart-save-history-unit"
        ),
        None,
    )
    if not isinstance(save_history, dict):
        raise AssertionError("focused federation save-history row is absent")
    if query_rti_work.source_location_text(save_history) != (
        "cpp/tests/ieee1516_2025_federation_registry_save_history_conditionals_filesystem_restart_catch2.cpp:3"
    ):
        raise AssertionError("focused federation save-history source pointer drifted")
    if save_history.get("assertions") != 42:
        raise AssertionError("focused federation save-history assertion count drifted")
    if save_history.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused federation save-history row is not mapped")
    if save_history.get("requirements_lab_mapping_id") != (
        "m93.federation-save-history-process-restart"
    ):
        raise AssertionError("focused federation save-history mapping handle drifted")
    if len(save_history.get("lab_requirement_ids", [])) != 10:
        raise AssertionError(
            "focused federation save-history requirement mapping drifted"
        )
    if len(save_history.get("standard_sections", [])) != 8:
        raise AssertionError("focused federation save-history section mapping drifted")
    if len(save_history.get("selected_cpp_api_surface_ids", [])) != 5:
        raise AssertionError("focused federation save-history API mapping drifted")
    save_history_focus = query_rti_work.focused_lane_result(
        index, tests, "federation-save-history-process-restart", limit=0
    )
    if save_history_focus.get("lane_state") != "complete":
        raise AssertionError("focused federation save-history lane is not complete")
    if save_history_focus.get("mapped_test_count") != 1:
        raise AssertionError("focused federation save-history mapped count drifted")
    if save_history_focus.get("assertion_count") != 42:
        raise AssertionError(
            "focused federation save-history lane assertion total drifted"
        )
    if save_history_focus.get("standard_section_count") != 8:
        raise AssertionError(
            "focused federation save-history lane section total drifted"
        )
    save_history_handles = save_history_focus.get("lane_handles")
    if (
        not isinstance(save_history_handles, dict)
        or save_history_handles.get("catch2_target")
        != "umbra_federation_registry_catch2"
    ):
        raise AssertionError("focused federation save-history target handle drifted")
    subscription_generation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-private-federation-registry-subscription-generation-restore-unit"
        ),
        None,
    )
    if not isinstance(subscription_generation, dict):
        raise AssertionError("focused subscription-generation row is absent")
    if query_rti_work.source_location_text(subscription_generation) != (
        "cpp/tests/federation_registry_composed_fom_catch2.cpp:926"
    ):
        raise AssertionError("focused subscription-generation source pointer drifted")
    if subscription_generation.get("assertions") != 31:
        raise AssertionError("focused subscription-generation assertion count drifted")
    if subscription_generation.get("traceability_state") != "explicit-disposition":
        raise AssertionError("focused subscription-generation row must remain internal")
    if subscription_generation.get("lab_requirement_ids") != []:
        raise AssertionError(
            "focused subscription-generation row acquired a normative requirement"
        )
    if subscription_generation.get("standard_sections") != []:
        raise AssertionError(
            "focused subscription-generation row acquired a normative section"
        )
    if subscription_generation.get("selected_cpp_api_surface_ids") != []:
        raise AssertionError(
            "focused subscription-generation unit row acquired public APIs"
        )
    subscription_generation_focus = query_rti_work.focused_lane_result(
        index, tests, "subscription-generation", limit=0
    )
    if subscription_generation_focus.get("lane_state") != "complete":
        raise AssertionError("focused subscription-generation lane is not complete")
    if subscription_generation_focus.get("mapped_test_count") != 0:
        raise AssertionError("focused subscription-generation mapped count drifted")
    if subscription_generation_focus.get("explicit_disposition_count") != 1:
        raise AssertionError(
            "focused subscription-generation disposition count drifted"
        )
    if subscription_generation_focus.get("assertion_count") != 31:
        raise AssertionError(
            "focused subscription-generation lane assertion total drifted"
        )
    if subscription_generation_focus.get("standard_section_count") != 0:
        raise AssertionError(
            "focused subscription-generation lane section total drifted"
        )
    subscription_generation_handles = subscription_generation_focus.get("lane_handles")
    if (
        not isinstance(subscription_generation_handles, dict)
        or subscription_generation_handles.get("catch2_target")
        != "umbra_fom_composer_catch2"
    ):
        raise AssertionError("focused subscription-generation target handle drifted")
    writer_failure = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-service-report-writer-failure-integration"
        ),
        None,
    )
    if not isinstance(writer_failure, dict):
        raise AssertionError("focused service-report writer-failure row is absent")
    if query_rti_work.source_location_text(writer_failure) != (
        "cpp/tests/service_report_writer_failure_catch2.cpp:92"
    ):
        raise AssertionError(
            "focused service-report writer-failure source pointer drifted"
        )
    if writer_failure.get("assertions") != 10:
        raise AssertionError(
            "focused service-report writer-failure assertion count drifted"
        )
    if writer_failure.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused service-report writer-failure row is not mapped")
    if writer_failure.get("requirements_lab_mapping_id") != (
        "m91.embedded-service-report-writer-creation-failure"
    ):
        raise AssertionError(
            "focused service-report writer-failure mapping handle drifted"
        )
    if len(writer_failure.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused service-report writer-failure requirement mapping drifted"
        )
    if len(writer_failure.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused service-report writer-failure section mapping drifted"
        )
    if len(writer_failure.get("selected_cpp_api_surface_ids", [])) != 6:
        raise AssertionError(
            "focused service-report writer-failure API mapping drifted"
        )
    writer_failure_focus = query_rti_work.focused_lane_result(
        index, tests, "service-report-writer-failure", limit=0
    )
    if writer_failure_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused service-report writer-failure lane is not complete"
        )
    if writer_failure_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused service-report writer-failure mapped count drifted"
        )
    if writer_failure_focus.get("explicit_disposition_count") != 1:
        raise AssertionError(
            "focused service-report writer-failure disposition count drifted"
        )
    if writer_failure_focus.get("assertion_count") != 27:
        raise AssertionError(
            "focused service-report writer-failure lane assertion total drifted"
        )
    if writer_failure_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused service-report writer-failure lane section total drifted"
        )
    writer_failure_handles = writer_failure_focus.get("lane_handles")
    if (
        not isinstance(writer_failure_handles, dict)
        or writer_failure_handles.get("catch2_target")
        != "umbra_service_report_writer_failure_catch2"
    ):
        raise AssertionError(
            "focused service-report writer-failure target handle drifted"
        )
    mom_federate_set_switches = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-mom-federate-set-switches-integration"
        ),
        None,
    )
    if not isinstance(mom_federate_set_switches, dict):
        raise AssertionError("focused joined-federate HLAsetSwitches row is absent")
    if query_rti_work.source_location_text(mom_federate_set_switches) != (
        "cpp/tests/mom_federate_set_switches_catch2.cpp:67"
    ):
        raise AssertionError(
            "focused joined-federate HLAsetSwitches source pointer drifted"
        )
    if mom_federate_set_switches.get("assertions") != 45:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches assertion count drifted"
        )
    if mom_federate_set_switches.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused joined-federate HLAsetSwitches row is not mapped")
    if len(mom_federate_set_switches.get("lab_requirement_ids", [])) != 5:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches requirement mapping drifted"
        )
    if len(mom_federate_set_switches.get("standard_sections", [])) != 3:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches section mapping drifted"
        )
    if len(mom_federate_set_switches.get("selected_cpp_api_surface_ids", [])) != 12:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches API mapping drifted"
        )
    mom_federate_set_switches_focus = query_rti_work.focused_lane_result(
        index, tests, "mom-federate-set-switches", limit=0
    )
    if mom_federate_set_switches_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAsetSwitches lane is not complete"
        )
    if mom_federate_set_switches_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches mapped count drifted"
        )
    if mom_federate_set_switches_focus.get("assertion_count") != 45:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches lane assertion total drifted"
        )
    if mom_federate_set_switches_focus.get("requirement_count") != 5:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches lane requirement total drifted"
        )
    if mom_federate_set_switches_focus.get("standard_section_count") != 3:
        raise AssertionError(
            "focused joined-federate HLAsetSwitches lane section total drifted"
        )
    mom_federate_set_switches_handles = mom_federate_set_switches_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(mom_federate_set_switches_handles, dict)
        or mom_federate_set_switches_handles.get("catch2_target")
        != "umbra_mom_federate_set_switches_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAsetSwitches target handle drifted"
        )
    focused_attribute_transportation = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-focused-attribute-transportation-type-control-integration"
        ),
        None,
    )
    if not isinstance(focused_attribute_transportation, dict):
        raise AssertionError("focused attribute transportation row is absent")
    if query_rti_work.source_location_text(focused_attribute_transportation) != (
        "cpp/tests/custom_transportation_type_control_catch2.cpp:102"
    ):
        raise AssertionError("focused attribute transportation source pointer drifted")
    if focused_attribute_transportation.get("assertions") != 57:
        raise AssertionError("focused attribute transportation assertion count drifted")
    if (
        focused_attribute_transportation.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError("focused attribute transportation row is not mapped")
    if len(focused_attribute_transportation.get("lab_requirement_ids", [])) != 9:
        raise AssertionError(
            "focused attribute transportation requirement mapping drifted"
        )
    if len(focused_attribute_transportation.get("standard_sections", [])) != 4:
        raise AssertionError("focused attribute transportation section mapping drifted")
    if (
        len(focused_attribute_transportation.get("selected_cpp_api_surface_ids", []))
        != 13
    ):
        raise AssertionError("focused attribute transportation API mapping drifted")
    focused_attribute_transportation_lane = query_rti_work.focused_lane_result(
        index, tests, "custom-transportation-type-control", limit=0
    )
    if focused_attribute_transportation_lane.get("lane_state") != "complete":
        raise AssertionError("focused attribute transportation lane is not complete")
    if focused_attribute_transportation_lane.get("mapped_test_count") != 1:
        raise AssertionError("focused attribute transportation mapped count drifted")
    if focused_attribute_transportation_lane.get("assertion_count") != 57:
        raise AssertionError(
            "focused attribute transportation lane assertion total drifted"
        )
    if focused_attribute_transportation_lane.get("requirement_count") != 9:
        raise AssertionError(
            "focused attribute transportation lane requirement total drifted"
        )
    if focused_attribute_transportation_lane.get("standard_section_count") != 4:
        raise AssertionError(
            "focused attribute transportation lane section total drifted"
        )
    focused_attribute_transportation_handles = (
        focused_attribute_transportation_lane.get("lane_handles")
    )
    if (
        not isinstance(focused_attribute_transportation_handles, dict)
        or focused_attribute_transportation_handles.get("catch2_target")
        != "umbra_custom_transportation_type_control_catch2"
    ):
        raise AssertionError("focused attribute transportation target handle drifted")
    focused_timestamped_slices = [
        {
            "id": "umbra-cpp-delay-subscription-evaluation-timestamped-directed-interaction-integration",
            "mapping": "m59.embedded-delay-subscription-evaluation-timestamped-directed-interaction",
            "source": "cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp:129",
            "assertions": 166,
            "lane": "delay-subscription-evaluation-timestamped-directed-interaction",
            "requirements": 3,
            "sections": 1,
        },
        {
            "id": "umbra-cpp-timestamped-directed-interaction-tso-retraction-integration",
            "mapping": "m60.embedded-timestamped-directed-interaction-tso-retraction",
            "source": "cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp:150",
            "assertions": 56,
            "lane": "timestamped-directed-interaction-tso-retraction",
            "requirements": 15,
            "sections": 13,
        },
    ]
    for expectation in focused_timestamped_slices:
        focused_slice = next(
            (test for test in tests if test.get("id") == expectation["id"]),
            None,
        )
        if not isinstance(focused_slice, dict):
            raise AssertionError(f"focused {expectation['mapping']} row is absent")
        if query_rti_work.source_location_text(focused_slice) != expectation["source"]:
            raise AssertionError(
                f"focused {expectation['mapping']} source pointer drifted"
            )
        if focused_slice.get("assertions") != expectation["assertions"]:
            raise AssertionError(
                f"focused {expectation['mapping']} assertion count drifted"
            )
        if focused_slice.get("requirements_lab_mapping_id") != expectation["mapping"]:
            raise AssertionError(
                f"focused {expectation['mapping']} mapping handle drifted"
            )
        if (
            len(focused_slice.get("lab_requirement_ids", []))
            != expectation["requirements"]
        ):
            raise AssertionError(
                f"focused {expectation['mapping']} requirement mapping drifted"
            )
        if len(focused_slice.get("standard_sections", [])) != expectation["sections"]:
            raise AssertionError(
                f"focused {expectation['mapping']} section mapping drifted"
            )
        timestamped_focus = query_rti_work.focused_lane_result(
            index, tests, expectation["lane"], limit=0
        )
        if timestamped_focus.get("lane_state") != "complete":
            raise AssertionError(f"focused {expectation['lane']} lane is not complete")
        if timestamped_focus.get("mapped_test_count") != 1:
            raise AssertionError(f"focused {expectation['lane']} mapped count drifted")
        if timestamped_focus.get("assertion_count") != expectation["assertions"]:
            raise AssertionError(
                f"focused {expectation['lane']} assertion total drifted"
            )
        if timestamped_focus.get("standard_section_count") != expectation["sections"]:
            raise AssertionError(f"focused {expectation['lane']} section total drifted")
    mom_auto_provide = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-auto-provide-mom-integration"
        ),
        None,
    )
    if not isinstance(mom_auto_provide, dict):
        raise AssertionError("focused MOM Auto Provide row is absent")
    if query_rti_work.source_location_text(mom_auto_provide) != (
        "cpp/tests/auto_provide_mom_catch2.cpp:120"
    ):
        raise AssertionError("focused MOM Auto Provide source pointer drifted")
    if mom_auto_provide.get("assertions") != 41:
        raise AssertionError("focused MOM Auto Provide assertion count drifted")
    if mom_auto_provide.get("traceability_state") != "explicit-disposition":
        raise AssertionError("focused MOM Auto Provide disposition drifted")
    if len(mom_auto_provide.get("lab_requirement_ids", [])) != 0:
        raise AssertionError("focused MOM Auto Provide Lab requirement mapping drifted")
    if len(mom_auto_provide.get("standard_sections", [])) != 3:
        raise AssertionError("focused MOM Auto Provide section mapping drifted")
    if len(mom_auto_provide.get("selected_cpp_api_surface_ids", [])) != 18:
        raise AssertionError("focused MOM Auto Provide API mapping drifted")
    mom_auto_provide_focus = query_rti_work.focused_lane_result(
        index, tests, "mom-auto-provide-switch-mutation", limit=0
    )
    if mom_auto_provide_focus.get("lane_state") != "complete":
        raise AssertionError("focused MOM Auto Provide lane is not complete")
    if mom_auto_provide_focus.get("mapped_test_count") != 0:
        raise AssertionError("focused MOM Auto Provide mapped count drifted")
    if mom_auto_provide_focus.get("assertion_count") != 41:
        raise AssertionError("focused MOM Auto Provide lane assertion total drifted")
    if mom_auto_provide_focus.get("standard_section_count") != 3:
        raise AssertionError("focused MOM Auto Provide lane section total drifted")
    mom_auto_provide_handles = mom_auto_provide_focus.get("lane_handles")
    if (
        not isinstance(mom_auto_provide_handles, dict)
        or mom_auto_provide_handles.get("catch2_target")
        != "umbra_auto_provide_mom_catch2"
    ):
        raise AssertionError("focused MOM Auto Provide target handle drifted")
    handle_normalization = next(
        (
            test
            for test in tests
            if test.get("id") == "umbra-cpp-handle-normalization-integration"
        ),
        None,
    )
    if not isinstance(handle_normalization, dict):
        raise AssertionError("focused handle-normalization row is absent")
    if query_rti_work.source_location_text(handle_normalization) != (
        "cpp/tests/handle_normalization_catch2.cpp:60"
    ):
        raise AssertionError("focused handle-normalization source pointer drifted")
    if handle_normalization.get("assertions") != 52:
        raise AssertionError("focused handle-normalization assertion count drifted")
    if handle_normalization.get("traceability_state") != "requirements-mapped":
        raise AssertionError("focused handle-normalization row is not mapped")
    if len(handle_normalization.get("lab_requirement_ids", [])) != 6:
        raise AssertionError("focused handle-normalization requirement mapping drifted")
    if len(handle_normalization.get("standard_sections", [])) != 6:
        raise AssertionError("focused handle-normalization section mapping drifted")
    if len(handle_normalization.get("selected_cpp_api_surface_ids", [])) != 5:
        raise AssertionError("focused handle-normalization API mapping drifted")
    handle_normalization_focus = query_rti_work.focused_lane_result(
        index, tests, "handle-normalization", limit=0
    )
    if handle_normalization_focus.get("lane_state") != "complete":
        raise AssertionError("focused handle-normalization lane is not complete")
    if handle_normalization_focus.get("mapped_test_count") != 2:
        raise AssertionError("focused handle-normalization mapped count drifted")
    if handle_normalization_focus.get("assertion_count") != 75:
        raise AssertionError("focused handle-normalization assertion total drifted")
    if handle_normalization_focus.get("requirement_count") != 6:
        raise AssertionError("focused handle-normalization requirement total drifted")
    if handle_normalization_focus.get("standard_section_count") != 6:
        raise AssertionError("focused handle-normalization section total drifted")
    if handle_normalization_focus.get("requirement_section_pair_count") != 6:
        raise AssertionError("focused handle-normalization direct pair drifted")
    handle_normalization_handles = handle_normalization_focus.get("lane_handles")
    if (
        not isinstance(handle_normalization_handles, dict)
        or handle_normalization_handles.get("catch2_target")
        != "umbra_handle_normalization_catch2"
    ):
        raise AssertionError("focused handle-normalization target handle drifted")
    object_instances_updated_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-updated-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_updated_report, dict):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated row is absent"
        )
    if query_rti_work.source_location_text(object_instances_updated_report) != (
        "cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp:137"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated source pointer drifted"
        )
    if object_instances_updated_report.get("assertions") != 42:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated assertion count drifted"
        )
    if (
        object_instances_updated_report.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated row is not mapped"
        )
    if len(object_instances_updated_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated requirement mapping drifted"
        )
    if len(object_instances_updated_report.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated section mapping drifted"
        )
    if (
        len(object_instances_updated_report.get("selected_cpp_api_surface_ids", []))
        != 12
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated API mapping drifted"
        )
    object_instances_updated_report_focus = query_rti_work.focused_lane_result(
        index, tests, "joined-federate-mom-object-instances-updated-report", limit=0
    )
    if object_instances_updated_report_focus.get("lane_state") != "complete":
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated lane is not complete"
        )
    if object_instances_updated_report_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated mapped count drifted"
        )
    if object_instances_updated_report_focus.get("assertion_count") != 42:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated assertion total drifted"
        )
    if object_instances_updated_report_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated requirement total drifted"
        )
    if object_instances_updated_report_focus.get("standard_section_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated section total drifted"
        )
    if object_instances_updated_report_focus.get("requirement_section_pair_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated direct pair drifted"
        )
    object_instances_updated_report_handles = object_instances_updated_report_focus.get(
        "lane_handles"
    )
    if (
        not isinstance(object_instances_updated_report_handles, dict)
        or object_instances_updated_report_handles.get("catch2_target")
        != "umbra_joined_federate_mom_object_instances_updated_report_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesUpdated target handle drifted"
        )
    object_instances_that_can_be_deleted_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-that-can-be-deleted-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_that_can_be_deleted_report, dict):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted row is absent"
        )
    if query_rti_work.source_location_text(
        object_instances_that_can_be_deleted_report
    ) != (
        "cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp:137"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted source pointer drifted"
        )
    if object_instances_that_can_be_deleted_report.get("assertions") != 53:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted assertion count drifted"
        )
    if (
        object_instances_that_can_be_deleted_report.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted row is not mapped"
        )
    if (
        len(object_instances_that_can_be_deleted_report.get("lab_requirement_ids", []))
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted requirement mapping drifted"
        )
    if (
        len(object_instances_that_can_be_deleted_report.get("standard_sections", []))
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted section mapping drifted"
        )
    if (
        len(
            object_instances_that_can_be_deleted_report.get(
                "selected_cpp_api_surface_ids", []
            )
        )
        != 12
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted API mapping drifted"
        )
    object_instances_that_can_be_deleted_report_focus = (
        query_rti_work.focused_lane_result(
            index,
            tests,
            "joined-federate-mom-object-instances-that-can-be-deleted-report",
            limit=0,
        )
    )
    if (
        object_instances_that_can_be_deleted_report_focus.get("lane_state")
        != "complete"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted lane is not complete"
        )
    if object_instances_that_can_be_deleted_report_focus.get("mapped_test_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted mapped count drifted"
        )
    if object_instances_that_can_be_deleted_report_focus.get("assertion_count") != 53:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted assertion total drifted"
        )
    if object_instances_that_can_be_deleted_report_focus.get("requirement_count") != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted requirement total drifted"
        )
    if (
        object_instances_that_can_be_deleted_report_focus.get("standard_section_count")
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted section total drifted"
        )
    if (
        object_instances_that_can_be_deleted_report_focus.get(
            "requirement_section_pair_count"
        )
        != 1
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted direct pair drifted"
        )
    object_instances_that_can_be_deleted_report_handles = (
        object_instances_that_can_be_deleted_report_focus.get("lane_handles")
    )
    if (
        not isinstance(object_instances_that_can_be_deleted_report_handles, dict)
        or object_instances_that_can_be_deleted_report_handles.get("catch2_target")
        != "umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesThatCanBeDeleted target handle drifted"
        )
    object_instances_reflected_report = next(
        (
            test
            for test in tests
            if test.get("id")
            == "umbra-cpp-joined-federate-mom-object-instances-reflected-report-integration"
        ),
        None,
    )
    if not isinstance(object_instances_reflected_report, dict):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected row is absent"
        )
    if query_rti_work.source_location_text(object_instances_reflected_report) != (
        "cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp:160"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected source pointer drifted"
        )
    if object_instances_reflected_report.get("assertions") != 51:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected assertion count drifted"
        )
    if (
        object_instances_reflected_report.get("traceability_state")
        != "requirements-mapped"
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected row is not mapped"
        )
    if len(object_instances_reflected_report.get("lab_requirement_ids", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected requirement mapping drifted"
        )
    if len(object_instances_reflected_report.get("standard_sections", [])) != 1:
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected section mapping drifted"
        )
    if (
        len(object_instances_reflected_report.get("selected_cpp_api_surface_ids", []))
        != 14
    ):
        raise AssertionError(
            "focused joined-federate HLAreportObjectInstancesReflected API mapping drifted"
        )
