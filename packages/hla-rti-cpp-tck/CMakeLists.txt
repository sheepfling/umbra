cmake_minimum_required(VERSION 3.20)

project(hla_rti_cpp_tck LANGUAGES CXX)

set(HLA_RTI_TCK_API_INCLUDE_DIR "" CACHE PATH
  "Directory containing the official IEEE 1516.1-2025 C++ API headers")
set(HLA_RTI_TCK_PROVIDER_LIBRARIES "" CACHE STRING
  "Semicolon-separated provider libraries and link targets")
set(HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS "" CACHE STRING
  "Semicolon-separated provider compile definitions")
set(HLA_RTI_TCK_PROVIDER_LINK_OPTIONS "" CACHE STRING
  "Semicolon-separated provider link options")
set(HLA_RTI_TCK_TEST_SCENARIOS "" CACHE STRING
  "Semicolon-separated portable TCK scenario IDs to register with CTest")
set(HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS "" CACHE STRING
  "Semicolon-separated candidate scenario IDs to label separately in CTest")
set(HLA_RTI_TCK_TEST_CALLBACK_MODELS "" CACHE STRING
  "Semicolon-separated callback models to register with CTest")
set(HLA_RTI_TCK_TEST_COMMON_ARGUMENTS "" CACHE STRING
  "Semicolon-separated adapter arguments shared by each CTest scenario")
set(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS 0 CACHE STRING
  "CTest timeout for each portable TCK scenario, or 0 for no CTest timeout")

if(NOT HLA_RTI_TCK_API_INCLUDE_DIR)
  message(FATAL_ERROR
    "HLA_RTI_TCK_API_INCLUDE_DIR must point at the official RTI headers")
endif()

add_executable(hla_rti_cpp_tck
  src/main.cpp
  src/tck_auto_provide_scenarios.cpp
  src/tck_receive_order_callback_cancellation.cpp
  src/tck_attribute_value_update_request_response.cpp
  src/tck_object_class_timestamped_provider_response.cpp
  src/tck_update_rate_scenarios.cpp
  src/tck_federation_mom_content_scenarios.cpp
  src/tck_federation_synchronization_mom_queries.cpp
  src/tck_multi_attribute_delivery_scenarios.cpp
  src/tck_timestamped_interaction_scheduling_scenarios.cpp
  src/tck_flush_queue_nonlimiting_galt_scenarios.cpp
  src/tck_nrg_regulator_resignation.cpp
  src/tck_nrg_disabled_regulator_activation.cpp
  src/tck_constrained_tar_disable_release.cpp
  src/tck_save_active_callback_resignation.cpp
  src/tck_timed_save_fqr_callback_resignation.cpp
  src/tck_timed_save_tara_callback_resignation.cpp
  src/tck_timed_save_nmra_callback_resignation.cpp
  src/tck_timed_save_mixed_fqr_tar.cpp
  src/tck_timed_save_ordinary_before_fqr.cpp
  src/tck_timed_save_all_advance_modes.cpp
  src/tck_timed_save_per_member_tso.cpp
  src/tck_timed_save_flush_queue_admission.cpp
  src/tck_timed_save_available_admission.cpp
  src/tck_timed_save_next_message_admission.cpp
  src/tck_timed_save_available_next_message_member_admission.cpp
  src/tck_timed_save_resigned_constrained_member.cpp
  src/tck_restore_pending_flush_queue_request.cpp
  src/tck_restore_resignation_callback_lifetime.cpp
  src/tck_restore_pending_time_advance.cpp
  src/tck_restore_pending_time_regulation_callback.cpp
  src/tck_restore_mixed_pending_time_role_callbacks.cpp
  src/tck_restore_pending_advance_modes.cpp
  src/tck_restore_pending_tso_nmr.cpp
  src/tck_restore_pending_tso_nmra_attribute.cpp
  src/tck_restore_pending_tso_nmra_delete.cpp
  src/tck_restore_mixed_tso_four_member.cpp
  src/tck_restore_terminal_tso_attribute.cpp
  src/tck_restore_terminal_tso_interaction.cpp
  src/tck_restore_terminal_tso_directed_interaction.cpp
  src/tck_restore_time_window_state.cpp
  src/tck_restore_deferred_lookahead.cpp
  src/tck_fqr_deferred_lookahead.cpp
  src/tck_fqr_undefined_galt_nrg.cpp
  src/tck_default_region_class_request_equivalence.cpp
  src/tck_restore_multi_pending_time_advance.cpp
  src/tck_restore_pending_flush_queue_immediate.cpp
  src/tck_restore_object_snapshot_status.cpp
  src/tck_restore_tso_retraction_uniqueness.cpp
  src/tck_restore_tso_directed_multi_recipient.cpp
  src/tck_restore_live_tso_retraction_record.cpp
  src/tck_restore_live_tso_directed_interaction.cpp
  src/tck_restore_live_tso_interaction_multi_recipient.cpp
  src/tck_restore_live_tso_attribute_multi_recipient.cpp
  src/tck_restore_default_region_attribute_multi_recipient.cpp
  src/tck_restore_regional_interaction_multi_recipient.cpp
  src/tck_timed_restore_regional_attribute_multi_recipient.cpp
  src/tck_timestamped_directed_interaction_subscription_kind.cpp
  src/tck_timestamped_directed_zero_lookahead_tar_boundary.cpp
  src/tck_directed_interaction_received_class_projection.cpp
  src/tck_directed_retraction_mixed_fanout.cpp
  src/tck_ownership_cancellation_idle.cpp
  src/tck_ownership_denial_idle.cpp
  src/tck_ownership_denial_query.cpp
  src/tck_ownership_transition_query.cpp
  src/tck_ownership_reacquisition_query.cpp
  src/tck_ownership_competing_if_available.cpp
  src/tck_ownership_mixed_if_available.cpp
  src/tck_ownership_unpublish_owned.cpp
  src/tck_directed_interaction_no_applicable_subscription.cpp
  src/tck_directed_interaction_subscription_accumulation.cpp
  src/tck_federation_lifecycle_scenarios.cpp
  src/tck_inherited_derived_delivery_scenarios.cpp
  src/tck_callback_controls_object_ownership.cpp
  src/tck_callback_controls_time_transport.cpp
  src/tck_connection_lifecycle.cpp
  src/portable_tck.cpp
  src/tck_portable_scenario_runner.cpp
  src/tck_timestamped_object_deletion_router.cpp
  src/tck_nmr_future_timestamped_attribute_update.cpp
  src/tck_timestamped_attribute_pending_tara.cpp
  src/tck_timestamped_attribute_tara_router.cpp
  src/tck_portable_member_control_scenarios.cpp
  src/tck_transport_order_type_change.cpp
  src/tck_transport_order_scenarios.cpp
  src/tck_portable_mom_interaction_counts.cpp
  src/tck_portable_resignation_ownership.cpp
  src/tck_portable_service_report_scenarios.cpp
  src/tck_custom_transportation_scenarios.cpp
  src/tck_custom_transportation_interaction_scenarios.cpp
  src/tck_post_resignation_valid_input_scenarios.cpp
  src/tck_regional_auto_provide_scenarios.cpp
  src/tck_portable_region_service_reports.cpp
  src/tck_portable_mom_declaration_queries.cpp
  src/tck_api_foundation.cpp
  src/tck_configuration_authorization.cpp
  src/tck_federation_mom_save_conditionals.cpp
  src/tck_federation_mom_save_status.cpp
  src/tck_joined_federate_mom_state_save_restore.cpp
  src/tck_joined_federate_mom_galt_lits_periodic.cpp
  src/tck_joined_federate_mom_tso_length_periodic.cpp
  src/tck_joined_federate_mom_removed_object_count.cpp
  src/tck_joined_federate_mom_updates_sent_counts.cpp
  src/tck_joined_federate_mom_updates_sent_mim_attribute.cpp
  src/tck_joined_federate_mom_object_instances_updated_mim_attribute.cpp
  src/tck_joined_federate_mom_object_instances_reflected_mim_attribute.cpp
  src/tck_joined_federate_mom_reflections_received_mim_attribute.cpp
  src/tck_joined_federate_mom_object_instances_discovered_mim_attribute.cpp
  src/tck_joined_federate_mom_object_instances_removed_mim_attribute.cpp
  src/tck_joined_federate_mom_object_instances_deleted_mim_attribute.cpp
  src/tck_joined_federate_mom_object_lifecycle.cpp
  src/tck_joined_federate_mom_deletable_object_report.cpp
  src/tck_joined_federate_mom_updated_object_report.cpp
  src/tck_joined_federate_mom_updated_timestamped_object_report.cpp
  src/tck_joined_federate_mom_updated_multi_class_object_report.cpp
  src/tck_joined_federate_mom_updated_null_object_report.cpp
  src/tck_joined_federate_mom_reflected_null_object_report.cpp
  src/tck_joined_federate_mom_reflected_object_report.cpp
  src/tck_joined_federate_mom_reflected_timestamped_object_report.cpp
  src/tck_joined_federate_mom_reflected_multi_class_object_report.cpp
  src/tck_joined_federate_mom_deletable_object_count.cpp
  src/tck_receive_order_attribute_update.cpp
  src/tck_joined_federate_mom_interactions_received_counts.cpp
  src/tck_joined_federate_mom_interactions_sent_counts.cpp
  src/tck_joined_federate_mom_directed_interactions_sent_counts.cpp
  src/tck_mom_transportation_type_change_request.cpp
  src/tck_joined_federate_mom_reflection_counts.cpp
  src/tck_joined_federate_mom_updated_object_count_periodic.cpp
  src/tck_joined_federate_mom_discovered_object_count_periodic.cpp
  src/tck_joined_federate_mom_deleted_object_count_periodic.cpp
  src/tck_joined_federate_mom_time_state_durations.cpp
  src/tck_order_type_controls.cpp
  src/tck_attribute_relevance_scenarios.cpp
  src/tck_joined_federate_mom_directed_interactions_received_counts.cpp
  src/tck_joined_federate_mom_reflections_received_counts.cpp
  src/tck_factory_identity.cpp
  src/tck_lookup_lifecycle_scenarios.cpp
  src/tck_fom_module_scenarios.cpp
  src/tck_fom_data_type_merge.cpp
  src/tck_fom_interaction_parameter_merge.cpp
  src/tck_fom_object_attribute_merge.cpp
  src/tck_fom_object_class_semantics_conflict.cpp
  src/tck_fom_matching_data_types.cpp
  src/tck_fom_class_dimension_merge.cpp
  src/tck_fom_named_variant_alternative_merge.cpp
  src/tck_fom_extendable_variant_hlaother.cpp
  src/tck_fom_enumerated_variant_conflicts.cpp
  src/tck_fom_array_cardinality.cpp
  src/tck_fom_time_representation.cpp
  src/tck_fom_dimension_duplicate.cpp
  src/tck_fom_synchronization_duplicate.cpp
  src/tck_fom_transportation_duplicate.cpp
  src/tck_fom_update_rate_duplicate.cpp
  src/tck_fom_switch_duplicate.cpp
  src/tck_fom_advisory_switch_duplicate.cpp
  src/tck_timestamped_attribute_scenarios.cpp
  src/tck_timestamped_attribute_reflect_region_snapshot.cpp
  src/tck_timestamped_attribute_update_time_advancing_cutoff.cpp
  src/tck_mixed_order_attribute_passel_retraction.cpp
  src/tck_timestamped_attribute_immediate_recipient_retraction.cpp
  src/tck_timestamped_attribute_sender_regulation_transition.cpp
  src/tck_unregulated_sender_timestamped_interaction.cpp
  src/tck_timestamped_interaction_scenarios.cpp
  src/tck_nmr_future_timestamped_object_deletion.cpp
  src/tck_nmra_future_timestamped_object_deletion.cpp
  src/tck_tara_future_timestamped_object_deletion.cpp
  src/tck_timestamped_interaction_boundary_scenarios.cpp
  src/tck_time_management_scenarios.cpp
  src/tck_tar_acknowledged_tso_galt_release.cpp
  src/tck_time_constrained_enable_scenarios.cpp
  src/tck_time_regulation_enable_actual_lookahead.cpp
  src/tck_nmra_received_tso_floor.cpp
  src/tck_nmra_message_set.cpp
  src/tck_fqr_non_time_constrained.cpp
  src/tck_time_coordinated_interaction.cpp
  src/tck_empty_attribute_subscription.cpp
  src/tck_attribute_subscription_mode_transitions.cpp
  src/tck_named_update_rate_subscription.cpp
  src/tck_default_rate_no_reduction.cpp
  src/tck_accumulating_attribute_subscriptions.cpp
  src/tck_passive_subscription_no_start_registration.cpp
  src/tck_selective_attribute_unsubscription.cpp
  src/tck_whole_class_attribute_unsubscription.cpp
  src/tck_empty_attribute_unsubscription.cpp
  src/tck_send_message_time_state_preservation.cpp
  src/tck_timestamped_interaction_preferred_receive_conversion.cpp
  src/tck_timestamped_interaction_sender_regulation_transition.cpp
  src/tck_interaction_fdd_preferred_order_default.cpp
  src/tck_attribute_fdd_preferred_order_default.cpp
  src/tck_timestamped_delete_privilege_preferred_order.cpp
  src/tck_sent_tso_per_recipient_order_conversion.cpp
  src/tck_service_report_scenarios.cpp
  src/tck_service_report_lookup_failure_scenarios.cpp
  src/tck_service_report_lookup_scenarios.cpp
  src/tck_service_report_interaction_scenarios.cpp
  src/tck_service_report_failure_scenarios.cpp
  src/tck_service_report_timestamped_attribute_failure_scenarios.cpp
  src/tck_service_report_timestamped_delete_failure_scenarios.cpp
  src/tck_service_report_attribute_update_scenarios.cpp
  src/tck_service_report_request_attribute_value_update_scenarios.cpp
  src/tck_service_report_local_delete_scenarios.cpp
  src/tck_service_report_name_release_scenarios.cpp
  src/tck_service_report_name_reservation_scenarios.cpp
  src/tck_service_report_object_lifecycle_scenarios.cpp
  src/tck_service_report_timestamped_interaction_scenarios.cpp
  src/tck_service_report_timestamped_directed_interaction_scenarios.cpp
  src/tck_service_report_regional_interaction_scenarios.cpp
  src/tck_service_report_interaction_execution_scenarios.cpp
  src/tck_service_report_object_attribute_declaration_scenarios.cpp
  src/tck_support_services_scenarios.cpp
  src/tck_declaration_management_scenarios.cpp
  src/tck_object_attribute_subscription_lifecycle_scenarios.cpp
  src/tck_object_publication_registration_fence_scenarios.cpp
  src/tck_object_registration_discovery_scenarios.cpp
  src/tck_object_management_scenarios.cpp
  src/tck_object_removal_fanout_scenarios.cpp
  src/tck_ownership_management_scenarios.cpp
  src/tck_ownership_assumption_search.cpp
  src/tck_ownership_divestiture_return_set.cpp
  src/tck_ownership_if_available_negotiated.cpp
  src/tck_if_available_deleted_object.cpp
  src/tck_ownership_after_fom_merge.cpp
  src/tck_resign_divests_all_instance_attributes.cpp
  src/tck_region_lifecycle_scenarios.cpp
  src/tck_delay_subscription_scenarios.cpp
  src/tck_federation_save_restore_interlocks.cpp
  src/tck_data_encoding.cpp
  src/tck_encoder_round_trip_scenarios.cpp
  src/tck_logical_time.cpp
  src/pending_negotiated_restore_tck.cpp
  src/service_report_creation_failure_tck.cpp
)
set_source_files_properties(src/main.cpp PROPERTIES
  COMPILE_DEFINITIONS "main=hla_rti_cpp_tck_original_main")
target_compile_features(hla_rti_cpp_tck PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(hla_rti_cpp_tck PRIVATE /bigobj)
endif()
target_include_directories(hla_rti_cpp_tck PRIVATE
  "${HLA_RTI_TCK_API_INCLUDE_DIR}"
)
# Keep platform-specific API spelling on the provider target (or in
# HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS) so the TCK does not conflict with
# aliases already exported by an installed provider package.
target_compile_definitions(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS}
)
target_link_options(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_LINK_OPTIONS}
)
target_link_libraries(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_LIBRARIES}
)

install(TARGETS hla_rti_cpp_tck RUNTIME DESTINATION bin)
install(DIRECTORY fom/
  DESTINATION share/hla-rti-cpp-tck/fom
  FILES_MATCHING PATTERN "*.xml")

include(CTest)
if(BUILD_TESTING)
  add_test(NAME hla_rti_cpp_tck_help COMMAND hla_rti_cpp_tck --help)

  if(HLA_RTI_TCK_TEST_SCENARIOS AND HLA_RTI_TCK_TEST_CALLBACK_MODELS)
    foreach(_tck_scenario IN LISTS HLA_RTI_TCK_TEST_SCENARIOS)
      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _tck_scenario_slug
        "${_tck_scenario}")
      if(_tck_scenario IN_LIST HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS)
        set(_tck_promotion_label candidate)
      else()
        set(_tck_promotion_label promoted)
      endif()
      if(_tck_scenario STREQUAL
           "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot" OR
         _tck_scenario STREQUAL
           "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot-contract" OR
         _tck_scenario STREQUAL
           "cpp-tck.if-available-during-negotiated-divestiture" OR
         _tck_scenario STREQUAL
           "cpp-tck.if-available-during-negotiated-divestiture-contract" OR
         _tck_scenario STREQUAL "cpp-tck.nmra-ro-earliest-tso-cohort" OR
         _tck_scenario STREQUAL "cpp-tck.nmra-ro-earliest-tso-cohort-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-active-callback-resignation-guard" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-active-callback-resignation-guard-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-fqr-active-callback-resignation-guard" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-fqr-active-callback-resignation-guard-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-tara-active-callback-resignation-guard" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-tara-active-callback-resignation-guard-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-nmra-active-callback-resignation-guard" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-nmra-active-callback-resignation-guard-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-mixed-fqr-tar-members" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-mixed-fqr-tar-members-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-ordinary-boundary-before-fqr" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-ordinary-boundary-before-fqr-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-all-advance-modes" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-all-advance-modes-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-per-member-tso" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-per-member-tso-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-flush-queue-admission" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-flush-queue-admission-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-available-admission" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-available-admission-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-next-message-admission" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-next-message-admission-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-available-next-message-member-admission" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-available-next-message-member-admission-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-resigned-constrained-member" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-resigned-constrained-member-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-resigned-pending-fqr" OR
         _tck_scenario STREQUAL "cpp-tck.timed-save-resigned-pending-fqr-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-pending-flush-queue-request" OR
         _tck_scenario STREQUAL "cpp-tck.restore-pending-flush-queue-request-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-resignation-callback-lifetime" OR
         _tck_scenario STREQUAL "cpp-tck.restore-resignation-callback-lifetime-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-pending-time-advance" OR
         _tck_scenario STREQUAL "cpp-tck.restore-pending-time-advance-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-mixed-tso-four-member" OR
         _tck_scenario STREQUAL "cpp-tck.restore-mixed-tso-four-member-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-attribute" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-attribute-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-interaction" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-interaction-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-directed-interaction" OR
         _tck_scenario STREQUAL "cpp-tck.restore-terminal-tso-directed-interaction-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-tso-retraction-uniqueness" OR
         _tck_scenario STREQUAL "cpp-tck.restore-tso-retraction-uniqueness-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-tso-directed-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.restore-tso-directed-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-retraction-record" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-retraction-record-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-directed-interaction" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-directed-interaction-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-interaction-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-interaction-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-attribute-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.restore-live-tso-attribute-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-default-region-attribute-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.restore-default-region-attribute-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-regional-interaction-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.restore-regional-interaction-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.timed-restore-regional-attribute-multi-recipient" OR
         _tck_scenario STREQUAL "cpp-tck.timed-restore-regional-attribute-multi-recipient-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-time-window-state" OR
         _tck_scenario STREQUAL "cpp-tck.restore-time-window-state-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-deferred-lookahead" OR
         _tck_scenario STREQUAL "cpp-tck.restore-deferred-lookahead-contract" OR
         _tck_scenario STREQUAL "cpp-tck.fqr-deferred-lookahead" OR
         _tck_scenario STREQUAL "cpp-tck.fqr-deferred-lookahead-contract" OR
         _tck_scenario STREQUAL "cpp-tck.fqr-undefined-galt-nrg" OR
         _tck_scenario STREQUAL "cpp-tck.fqr-undefined-galt-nrg-contract" OR
         _tck_scenario STREQUAL "cpp-tck.default-region-class-request-equivalence" OR
         _tck_scenario STREQUAL "cpp-tck.default-region-class-request-equivalence-contract" OR
         _tck_scenario STREQUAL "cpp-tck.restore-multi-pending-time-advance" OR
         _tck_scenario STREQUAL "cpp-tck.restore-multi-pending-time-advance-contract")
        set(_tck_callback_models evoked)
      elseif(_tck_scenario STREQUAL "cpp-tck.restore-pending-time-advance-immediate" OR
             _tck_scenario STREQUAL "cpp-tck.restore-pending-time-advance-immediate-contract" OR
             _tck_scenario STREQUAL "cpp-tck.restore-pending-flush-queue-immediate" OR
             _tck_scenario STREQUAL "cpp-tck.restore-pending-flush-queue-immediate-contract" OR
             _tck_scenario STREQUAL "cpp-tck.restore-object-snapshot-status" OR
             _tck_scenario STREQUAL "cpp-tck.restore-object-snapshot-status-contract")
        set(_tck_callback_models immediate)
      else()
        set(_tck_callback_models ${HLA_RTI_TCK_TEST_CALLBACK_MODELS})
      endif()
      foreach(_tck_callback_model IN LISTS _tck_callback_models)
        set(_tck_test_name
          "hla_rti_cpp_tck.${_tck_scenario_slug}.${_tck_callback_model}")
        add_test(
          NAME "${_tck_test_name}"
          COMMAND hla_rti_cpp_tck
            ${HLA_RTI_TCK_TEST_COMMON_ARGUMENTS}
            --scenario "${_tck_scenario}"
            --callback-model "${_tck_callback_model}"
        )
        set_tests_properties("${_tck_test_name}" PROPERTIES
          LABELS "portable-cpp-tck;scenario;${_tck_callback_model};${_tck_promotion_label}"
          SKIP_REGULAR_EXPRESSION "skipped ")
        if(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS GREATER 0)
          set_tests_properties("${_tck_test_name}" PROPERTIES
            TIMEOUT "${HLA_RTI_TCK_TEST_TIMEOUT_SECONDS}")
        endif()
      endforeach()
    endforeach()
  endif()
endif()
