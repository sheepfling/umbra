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
  src/tck_receive_order_callback_cancellation.cpp
  src/tck_attribute_value_update_request_response.cpp
  src/tck_update_rate_scenarios.cpp
  src/tck_federation_mom_content_scenarios.cpp
  src/tck_federation_synchronization_mom_queries.cpp
  src/tck_multi_attribute_delivery_scenarios.cpp
  src/tck_timestamped_interaction_scheduling_scenarios.cpp
  src/tck_federation_lifecycle_scenarios.cpp
  src/tck_inherited_derived_delivery_scenarios.cpp
  src/tck_callback_controls_object_ownership.cpp
  src/tck_callback_controls_time_transport.cpp
  src/tck_connection_lifecycle.cpp
  src/portable_tck.cpp
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
  src/tck_timestamped_attribute_scenarios.cpp
  src/tck_mixed_order_attribute_passel_retraction.cpp
  src/tck_timestamped_attribute_sender_regulation_transition.cpp
  src/tck_unregulated_sender_timestamped_interaction.cpp
  src/tck_timestamped_interaction_scenarios.cpp
  src/tck_nmr_future_timestamped_object_deletion.cpp
  src/tck_nmra_future_timestamped_object_deletion.cpp
  src/tck_timestamped_interaction_boundary_scenarios.cpp
  src/tck_time_management_scenarios.cpp
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
target_compile_features(hla_rti_cpp_tck PRIVATE cxx_std_17)
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
      foreach(_tck_callback_model IN LISTS HLA_RTI_TCK_TEST_CALLBACK_MODELS)
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
