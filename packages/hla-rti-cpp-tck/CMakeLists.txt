cmake_minimum_required(VERSION 3.20)

project(hla_rti_cpp_tck LANGUAGES CXX)

set(HLA_RTI_TCK_API_INCLUDE_DIR "" CACHE PATH
  "Directory containing the official IEEE 1516.1-2025 C++ API headers")
set(HLA_RTI_TCK_PROVIDER_LIBRARIES "" CACHE STRING
  "Semicolon-separated provider libraries and link targets")
set(HLA_RTI_TCK_ADDITIONAL_RUNTIME_DIRECTORIES
  "$ENV{HLA_RTI_TCK_ADDITIONAL_RUNTIME_DIRECTORIES}" CACHE STRING
  "Semicolon-separated directories for transitive provider runtime DLL dependencies")
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
set(HLA_RTI_TCK_TEST_SETTINGS_PARSE_FAILURE_INPUT "" CACHE STRING
  "Adapter-provided settings text known to fail parsing for the focused Connect TCK")
set(HLA_RTI_TCK_TEST_UNSUPPORTED_TRANSPORT_SETTINGS "" CACHE STRING
  "Adapter-provided settings that make the fallback scenario's custom transports unavailable")
set(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS 0 CACHE STRING
  "CTest timeout for each portable TCK scenario, or 0 for no CTest timeout")
set(HLA_RTI_TCK_NMR_PROCESS_FIXTURE "" CACHE FILEPATH
  "Umbra process federation fixture used by process-separated NMR TCK adapters")
set(HLA_RTI_TCK_NMR_PROVIDER_BIN "" CACHE PATH
  "Provider runtime directory used by process-separated NMR TCK adapters")

if(NOT HLA_RTI_TCK_API_INCLUDE_DIR)
  message(FATAL_ERROR
    "HLA_RTI_TCK_API_INCLUDE_DIR must point at the official RTI headers")
endif()

add_executable(hla_rti_cpp_tck
  src/main.cpp
  src/tck_argument_file.cpp
  src/tck_federation_supplied_custom_tradt.cpp
  src/tck_scenario_dispatch.cpp
  src/tck_auto_provide_scenarios.cpp
  src/tck_receive_order_callback_cancellation.cpp
  src/tck_attribute_value_update_request_response.cpp
  src/tck_object_class_timestamped_provider_response.cpp
  src/tck_update_rate_scenarios.cpp
  src/tck_federation_mom_content_scenarios.cpp
  src/tck_federation_mom_published_interactions.cpp
  src/tck_federation_mom_s_leaf_subscription.cpp
  src/tck_federate_mom_set_switches_extra_parameter.cpp
  src/tck_federate_mom_category_c_interaction.cpp
  src/tck_federate_mom_report_added_subclass.cpp
  src/tck_service_report_extension_parameter.cpp
  src/tck_mom_service_optional_attribute_list_unsubscribe.cpp
  src/tck_mom_ownership_transfer_prohibition.cpp
  src/tck_mom_save_restore_interactions.cpp
  src/tck_federation_mom_table8_scenarios.cpp
  src/tck_federation_synchronization_mom_queries.cpp
  src/tck_synchronization_point_independent_tracking.cpp
  src/tck_multi_attribute_delivery_scenarios.cpp
  src/tck_attribute_subscription_mode_exclusivity.cpp
  src/tck_interaction_multi_recipient_fanout.cpp
  src/tck_fqr_process_scenarios.cpp
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
  src/tck_untimed_save_time_constrained_admission.cpp
  src/tck_federate_save_timestamp_eligibility.cpp
  src/tck_federation_save_request_replacement.cpp
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
  src/tck_restore_save_after_federation_destruction.cpp
  src/tck_restore_terminal_tso_attribute.cpp
  src/tck_restore_terminal_tso_interaction.cpp
  src/tck_restore_terminal_tso_directed_interaction.cpp
  src/tck_restore_time_window_state.cpp
  src/tck_restore_deferred_lookahead.cpp
  src/tck_fqr_deferred_lookahead.cpp
  src/tck_fqr_undefined_galt_nrg.cpp
  src/tck_nmr_lits_galt_scenarios.cpp
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
  src/tck_federate_save_identity.cpp
  src/tck_federation_join_intent.cpp
  src/tck_inherited_derived_delivery_scenarios.cpp
  src/tck_callback_controls_object_ownership.cpp
  src/tck_callback_controls_time_transport.cpp
  src/tck_evoke_multiple_callbacks_minimum_duration.cpp
  src/tck_evoke_multiple_callbacks_maximum_duration.cpp
  src/tck_evoke_multiple_callbacks_millisecond_precision.cpp
  src/tck_connection_lifecycle.cpp
  src/tck_connection_configuration_fallback.cpp
  src/tck_connection_settings.cpp
  src/tck_connection_session_cardinality.cpp
  src/tck_transport_public_interop.cpp
  src/tck_websocket_transport.cpp
  src/tck_tls_transport.cpp
  src/tck_tls_websocket_transport.cpp
  src/portable_tck.cpp
  src/tck_negotiated_divestiture_confirmation_flow.cpp
  src/tck_federate_update_threshold_scenarios.cpp
  src/tck_portable_scenario_runner.cpp
  src/tck_nmr_regional_timestamped_interaction.cpp
  src/tck_timestamped_tso_message_set_router.cpp
  src/tck_nmra_galt_boundary_router.cpp
  src/tck_nmr_message_set_router.cpp
  src/tck_nmr_future_timestamped_attribute_update.cpp
  src/tck_nmra_future_timestamped_attribute_update.cpp
  src/tck_timestamped_attribute_pending_tara.cpp
  src/tck_timestamped_attribute_tara_router.cpp
  src/tck_portable_member_control_scenarios.cpp
  src/tck_reliable_transport_one_copy.cpp
  src/tck_unsupported_fom_transportation_fallback.cpp
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
  src/tck_federate_mom_object_instance_information.cpp
  src/tck_api_foundation.cpp
  src/tck_annex_c_api_header_inventory.cpp
  src/tck_language_api_transparency.cpp
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
  src/tck_attribute_scope_ownership_acquisition.cpp
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
  src/tck_fom_member_data_type.cpp
  src/tck_fom_attribute_na_companions.cpp
  src/tck_fom_attribute_dynamic_update_condition.cpp
  src/tck_fom_attribute_value_required_sharing.cpp
  src/tck_fom_attribute_required_text.cpp
  src/tck_fom_reference_data_type_name.cpp
  src/tck_fom_simple_data_type_name.cpp
  src/tck_fom_model_identification_name.cpp
  src/tck_fom_enumerated_name_conventions.cpp
  src/tck_fom_array_table_rules.cpp
  src/tck_fom_root_hierarchy.cpp
  src/tck_fom_tag_data_type.cpp
  src/tck_fom_inherited_member_names.cpp
  src/tck_fom_equivalent_class_conflict.cpp
  src/tck_fom_reference_class_resolution.cpp
  src/tck_fom_enumerated_representation.cpp
  src/tck_fom_array_element_category.cpp
  src/tck_fom_record_member_category.cpp
  src/tck_fom_variant_discriminant_category.cpp
  src/tck_fom_variant_record_encoding.cpp
  src/tck_fom_variant_alternative_type_category.cpp
  src/tck_fom_variant_alternative_name_convention.cpp
  src/tck_fom_variant_discriminant_name_convention.cpp
  src/tck_fom_variant_record_name_convention.cpp
  src/tck_fom_variant_enumerator_syntax.cpp
  src/tck_fom_variant_enumerator_membership.cpp
  src/tck_fom_variant_range_semantics.cpp
  src/tck_fom_reference_attribute_resolution.cpp
  src/tck_fom_data_representation_resolution.cpp
  src/tck_fom_noncompliant_module.cpp
  src/tck_fom_incomplete_class_definition.cpp
  src/tck_fom_transportation_reference_resolution.cpp
  src/tck_fom_directed_reference_resolution.cpp
  src/tck_fom_dimension_reference_resolution.cpp
  src/tck_fom_dimension_duplicate.cpp
  src/tck_fom_synchronization_duplicate.cpp
  src/tck_fom_transportation_duplicate.cpp
  src/tck_fom_update_rate_duplicate.cpp
  src/tck_fom_switch_duplicate.cpp
  src/tck_fom_advisory_switch_duplicate.cpp
  src/tck_timestamped_attribute_scenarios.cpp
  src/tck_timestamped_attribute_reflect_region_snapshot.cpp
  src/tck_timestamped_attribute_update_time_advancing_cutoff.cpp
  src/tck_timestamped_update_before_delete.cpp
  src/tck_mixed_order_attribute_passel_retraction.cpp
  src/tck_timestamped_attribute_immediate_recipient_retraction.cpp
  src/tck_timestamped_attribute_sender_regulation_transition.cpp
  src/tck_unregulated_sender_timestamped_interaction.cpp
    src/tck_unregulated_sender_timestamped_attribute_update.cpp
  src/tck_timestamped_interaction_scenarios.cpp
  src/tck_nmr_future_timestamped_object_deletion.cpp
  src/tck_nmra_future_timestamped_object_deletion.cpp
  src/tck_tara_future_timestamped_object_deletion.cpp
  src/tck_tara_future_timestamped_interaction.cpp
  src/tck_timestamped_interaction_boundary_scenarios.cpp
  src/tck_time_management_scenarios.cpp
  src/tck_tara_undefined_galt_scenarios.cpp
  src/tck_non_regulating_federate_peer_advance.cpp
  src/tck_tar_acknowledged_tso_galt_release.cpp
  src/tck_time_constrained_enable_scenarios.cpp
  src/tck_time_regulation_enable_actual_lookahead.cpp
  src/tck_time_role_pending_enable.cpp
  src/tck_disable_time_regulation_send_order.cpp
  src/tck_nmr_message_set.cpp
  src/tck_nmra_received_tso_floor.cpp
  src/tck_nmra_message_set.cpp
  src/tck_fqr_non_time_constrained.cpp
  src/tck_time_coordinated_interaction.cpp
  src/tck_empty_attribute_subscription.cpp
  src/tck_attribute_subscription_mode_transitions.cpp
  src/tck_named_update_rate_subscription.cpp
  src/tck_default_rate_no_reduction.cpp
  src/tck_allow_relaxed_ddm_overlap_superset.cpp
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
  src/tck_mom_predefined_constructs.cpp
  src/tck_service_report_object_attribute_declaration_scenarios.cpp
  src/tck_support_services_scenarios.cpp
  src/tck_handle_encoding_mim_types.cpp
  src/tck_handle_decode_semantic_validity.cpp
  src/tck_declaration_management_scenarios.cpp
  src/tck_default_region_interaction_declaration_services.cpp
  src/tck_regional_subscription_triple_isolation.cpp
  src/tck_object_attribute_subscription_lifecycle_scenarios.cpp
  src/tck_object_publication_registration_fence_scenarios.cpp
  src/tck_object_registration_discovery_scenarios.cpp
  src/tck_object_management_scenarios.cpp
  src/tck_privilege_attribute_availability.cpp
  src/tck_object_removal_fanout_scenarios.cpp
  src/tck_ownership_acquisition_repeat_pending.cpp
  src/tck_acquisition_publication_guard.cpp
  src/tck_ownership_management_scenarios.cpp
  src/tck_negotiated_divestiture_duplicate_request.cpp
  src/tck_negotiated_divestiture_if_wanted_completion.cpp
  src/tck_ownership_assumption_search.cpp
  src/tck_ownership_divestiture_return_set.cpp
  src/tck_ownership_if_available_negotiated.cpp
  src/tck_if_available_deleted_object.cpp
  src/tck_ownership_after_fom_merge.cpp
  src/tck_resign_divests_all_instance_attributes.cpp
  src/tck_region_lifecycle_scenarios.cpp
  src/tck_regional_interaction_mixed_absent_unsubscribe.cpp
  src/tck_regional_class_request_update.cpp
  src/tck_regional_empty_request_region.cpp
  src/tck_regional_interaction_partial_default_range.cpp
  src/tck_regional_object_attribute_dimension_inheritance.cpp
  src/tck_registration_atomicity_scenario.cpp
  src/tck_registration_default_region_scenario.cpp
  src/tck_registration_invalid_region_scenario.cpp
  src/tck_default_region_association_replacement_scenario.cpp
  src/tck_delay_subscription_scenarios.cpp
  src/tck_federation_save_restore_interlocks.cpp
  src/tck_data_encoding.cpp
  src/tck_variant_record_discriminant_padding.cpp
  src/tck_variant_record_unmapped_discriminant.cpp
  src/tck_extendable_variant_record_length_padding.cpp
  src/tck_extendable_variant_record_alternative_padding.cpp
  src/tck_extendable_variant_record_unmapped_discriminant.cpp
  src/tck_extendable_variant_record_rejects_boundary_over_eight.cpp
  src/tck_fixed_array_element_sequence.cpp
  src/tck_variable_array_element_sequence.cpp
  src/tck_variable_array_first_element_padding.cpp
  src/tck_variable_array_inter_element_padding.cpp
  src/tck_data_encoding_fixed_width_integers.cpp
  src/tck_data_encoding_value_operations.cpp
  src/tck_data_encoding_complex_constituents.cpp
  src/tck_data_encoding_16bit_external_memory.cpp
  src/tck_data_encoding_array_copy_clone.cpp
  src/tck_data_encoding_record_copy_clone.cpp
  src/tck_data_encoding_extendable_variant_copy_clone.cpp
  src/tck_data_encoding_extendable_variant_raw_buffer.cpp
  src/tck_data_encoding_unicode_external_memory.cpp
  src/tck_data_encoding_ascii_external_memory.cpp
  src/tck_data_encoding_scalar_external_memory.cpp
  src/tck_data_encoding_floating_external_memory.cpp
  src/tck_encoder_round_trip_scenarios.cpp
  src/tck_logical_time.cpp
  src/tck_logical_time_header_layout.cpp
  src/pending_negotiated_restore_tck.cpp
  src/service_report_creation_failure_tck.cpp
)
set_source_files_properties(src/main.cpp PROPERTIES
  COMPILE_DEFINITIONS "main=hla_rti_cpp_tck_original_main")
if(HLA_RTI_TCK_TEST_BUDGET_SCRIPT)
  include("${HLA_RTI_TCK_TEST_BUDGET_SCRIPT}")
  umbra_guard_test_executable(hla_rti_cpp_tck)
endif()

target_compile_features(hla_rti_cpp_tck PRIVATE cxx_std_20)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # The unmodified IEEE 1516.1-2025 encoding header uses Microsoft's _int64
  # keyword for every _WIN32 compiler. Keep that licensed header exact and
  # provide the MinGW spelling to the standalone portable TCK target.
  target_compile_definitions(hla_rti_cpp_tck PRIVATE "_int64=long long")
endif()
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

add_library(hla_rti_cpp_tck_cpp14_api_surface OBJECT
  src/tck_cpp14_api_surface.cpp
)
set_target_properties(hla_rti_cpp_tck_cpp14_api_surface PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(hla_rti_cpp_tck_cpp14_api_surface PRIVATE
  "${HLA_RTI_TCK_API_INCLUDE_DIR}"
)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_definitions(hla_rti_cpp_tck_cpp14_api_surface PRIVATE
    "_int64=long long"
  )
endif()
add_dependencies(hla_rti_cpp_tck hla_rti_cpp_tck_cpp14_api_surface)

# A Windows import library links the runner but does not make its provider DLL
# discoverable. Keep the runner beside its own build artifacts and use only
# provider runtime directories plus Windows system directories at launch.
set(_tck_provider_runtime_directories)
if(WIN32)
  foreach(_provider_library IN LISTS HLA_RTI_TCK_PROVIDER_LIBRARIES)
    if(TARGET "${_provider_library}")
      get_target_property(_provider_location "${_provider_library}" IMPORTED_LOCATION)
      if(NOT _provider_location)
        get_target_property(_provider_configurations
          "${_provider_library}" IMPORTED_CONFIGURATIONS)
        foreach(_provider_configuration IN LISTS _provider_configurations)
          get_target_property(_provider_location "${_provider_library}"
            "IMPORTED_LOCATION_${_provider_configuration}")
          if(_provider_location)
            break()
          endif()
        endforeach()
      endif()
      if(_provider_location)
        get_filename_component(_provider_runtime_directory
          "${_provider_location}" DIRECTORY)
        list(APPEND _tck_provider_runtime_directories
          "${_provider_runtime_directory}")
      endif()
    elseif(IS_ABSOLUTE "${_provider_library}")
      get_filename_component(_provider_runtime_directory
        "${_provider_library}" DIRECTORY)
      get_filename_component(_provider_library_directory_name
        "${_provider_runtime_directory}" NAME)
      get_filename_component(_provider_package_prefix
        "${_provider_runtime_directory}" DIRECTORY)
      if(_provider_library_directory_name MATCHES "^lib(64)?$" AND
         EXISTS "${_provider_package_prefix}/bin")
        set(_provider_runtime_directory "${_provider_package_prefix}/bin")
      endif()
      list(APPEND _tck_provider_runtime_directories
        "${_provider_runtime_directory}")
    endif()
  endforeach()
  if(HLA_RTI_TCK_ADDITIONAL_RUNTIME_DIRECTORIES)
    list(APPEND _tck_provider_runtime_directories
      ${HLA_RTI_TCK_ADDITIONAL_RUNTIME_DIRECTORIES})
  endif()
  list(REMOVE_DUPLICATES _tck_provider_runtime_directories)
endif()
if(_tck_provider_runtime_directories)
  set(_tck_test_path_entries ${_tck_provider_runtime_directories})
else()
  set(_tck_test_path_entries)
endif()
if(WIN32)
  set(_tck_system_root "$ENV{SystemRoot}")
  if(NOT _tck_system_root)
    set(_tck_system_root "C:/Windows")
  endif()
  list(APPEND _tck_test_path_entries
    "${_tck_system_root}/System32"
    "${_tck_system_root}"
  )
  list(REMOVE_DUPLICATES _tck_test_path_entries)
  string(JOIN ";" _tck_test_path ${_tck_test_path_entries})
  # ENVIRONMENT is a CMake list property, so escape its embedded PATH
  # separators to keep the full search path in one environment assignment.
  string(REPLACE ";" "\\;" _tck_test_path_property "${_tck_test_path}")
  set(_tck_provider_test_environment "PATH=${_tck_test_path_property}")
endif()

if(WIN32)
  set(_tck_runtime_smoke_script
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/smoke_windows_executable.cmake")
  add_custom_command(TARGET hla_rti_cpp_tck POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DUMBRA_SMOKE_EXECUTABLE=$<TARGET_FILE:hla_rti_cpp_tck>"
      "-DUMBRA_SMOKE_ARGUMENTS=--help"
      "-DUMBRA_SMOKE_RUNTIME_DIRECTORIES=$<JOIN:${_tck_provider_runtime_directories},@>"
      -P "${_tck_runtime_smoke_script}"
    COMMENT "Checking the portable C++ TCK with a restricted Windows DLL search path"
    VERBATIM
  )
  set_property(TARGET hla_rti_cpp_tck APPEND PROPERTY LINK_DEPENDS
    "${_tck_runtime_smoke_script}")
endif()

install(TARGETS hla_rti_cpp_tck RUNTIME DESTINATION bin)
install(DIRECTORY fom/
  DESTINATION share/hla-rti-cpp-tck/fom
  FILES_MATCHING PATTERN "*.xml")

include(CTest)
if(BUILD_TESTING)
  set(_tck_common_arguments_file
    "${CMAKE_CURRENT_BINARY_DIR}/tck-common-arguments-$<CONFIG>.txt")
  if(HLA_RTI_TCK_TEST_COMMON_ARGUMENTS)
    file(GENERATE
      OUTPUT "${_tck_common_arguments_file}"
      CONTENT "$<JOIN:${HLA_RTI_TCK_TEST_COMMON_ARGUMENTS},\n>\n")
  else()
    file(GENERATE OUTPUT "${_tck_common_arguments_file}" CONTENT "")
  endif()

  add_test(NAME hla_rti_cpp_tck_help COMMAND hla_rti_cpp_tck --help)
  if(_tck_provider_test_environment)
    set_tests_properties(hla_rti_cpp_tck_help PROPERTIES
      ENVIRONMENT "${_tck_provider_test_environment}")
  endif()

  if(HLA_RTI_TCK_TEST_SCENARIOS AND HLA_RTI_TCK_TEST_CALLBACK_MODELS)
    foreach(_tck_scenario IN LISTS HLA_RTI_TCK_TEST_SCENARIOS)
      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _tck_scenario_slug
        "${_tck_scenario}")
      if(_tck_scenario IN_LIST HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS)
        set(_tck_promotion_label candidate)
      else()
        set(_tck_promotion_label promoted)
      endif()
      if(_tck_scenario IN_LIST HLA_RTI_TCK_TEST_EVOKED_ONLY_SCENARIOS)
        set(_tck_callback_models evoked)
      elseif(_tck_scenario IN_LIST HLA_RTI_TCK_TEST_IMMEDIATE_ONLY_SCENARIOS)
        set(_tck_callback_models immediate)
      else()
        set(_tck_callback_models ${HLA_RTI_TCK_TEST_CALLBACK_MODELS})
      endif()
      foreach(_tck_callback_model IN LISTS _tck_callback_models)
        set(_tck_test_name
          "hla_rti_cpp_tck.${_tck_scenario_slug}.${_tck_callback_model}")
        set(_tck_process_adapter FALSE)
        if(HLA_RTI_TCK_NMR_PROCESS_FIXTURE AND
           HLA_RTI_TCK_NMR_PROVIDER_BIN AND
           (_tck_scenario STREQUAL "cpp-tck.nmr-future-tso-arrival-while-pending" OR
            _tck_scenario STREQUAL "cpp-tck.nmr-future-tso-arrival-while-pending-contract" OR
            _tck_scenario STREQUAL "cpp-tck.nmr-directed-future-tso-arrival-while-pending" OR
            _tck_scenario STREQUAL "cpp-tck.nmr-directed-future-tso-arrival-while-pending-contract" OR
            _tck_scenario STREQUAL "cpp-tck.nmra-directed-future-tso-arrival-while-pending" OR
            _tck_scenario STREQUAL "cpp-tck.nmra-directed-future-tso-arrival-while-pending-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-nmr-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-nmr-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-nmra-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-nmra-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-tara-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-attribute-update-future-tara-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-tara-queued-equal-boundary" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-tara-queued-equal-boundary-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-tara-equal-boundary" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-tara-equal-boundary-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-nmra-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-nmra-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-nmr-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-nmr-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-tara-arrival" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-object-deletion-future-tara-arrival-contract" OR
            _tck_scenario STREQUAL "cpp-tck.nmra-future-tso-arrival-pending-request" OR
            _tck_scenario STREQUAL "cpp-tck.nmra-no-tso-waits-for-galt" OR
            _tck_scenario STREQUAL "cpp-tck.nmra-no-tso-waits-for-galt-contract" OR
            _tck_scenario STREQUAL "cpp-tck.nmr-request-below-galt-immediate-grant" OR
            _tck_scenario STREQUAL "cpp-tck.nmr-request-below-galt-immediate-grant-contract" OR
            _tck_scenario STREQUAL "cpp-tck.tara-no-tso-undefined-galt-nrg" OR
            _tck_scenario STREQUAL "cpp-tck.tara-no-tso-undefined-galt-nrg-contract" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-flush-queue-future-input" OR
            _tck_scenario STREQUAL "cpp-tck.timestamped-interaction-flush-queue-future-input-contract"))
          find_package(Python3 COMPONENTS Interpreter REQUIRED)
          set(_tck_process_adapter TRUE)
          add_test(
            NAME "${_tck_test_name}"
            COMMAND "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/adapters/current-process/run_nmr_future_arrival.py"
              --tck-executable "$<TARGET_FILE:hla_rti_cpp_tck>"
              --process-fixture "${HLA_RTI_TCK_NMR_PROCESS_FIXTURE}"
              --provider-bin "${HLA_RTI_TCK_NMR_PROVIDER_BIN}"
              --callback-model "${_tck_callback_model}"
              --scenario "${_tck_scenario}"
              --results "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        elseif(_tck_scenario STREQUAL
                 "cpp-tck.fom-standard-invalid-input-corpus" OR
               _tck_scenario STREQUAL
                 "cpp-tck.fom-standard-invalid-input-corpus-contract")
          file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tck-results")
          add_test(
            NAME "${_tck_test_name}"
            COMMAND hla_rti_cpp_tck
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
              --standard-invalid-fom
                "${CMAKE_CURRENT_SOURCE_DIR}/fom/attribute-unresolved-data-type-tck.xml"
              --invalid-fom
                "${CMAKE_CURRENT_SOURCE_DIR}/fom/invalid-malformed-tck.xml"
              --invalid-fom
                "${CMAKE_CURRENT_SOURCE_DIR}/fom/invalid-namespace-tck.xml"
              --invalid-fom
                "${CMAKE_CURRENT_SOURCE_DIR}/fom/invalid-duplicate-tck.xml"
              --results
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        elseif(_tck_scenario STREQUAL
                 "cpp-tck.fom-invalid-create-atomicity" OR
               _tck_scenario STREQUAL
                 "cpp-tck.fom-invalid-create-atomicity-contract")
          file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tck-results")
          add_test(
            NAME "${_tck_test_name}"
            COMMAND hla_rti_cpp_tck
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
              --invalid-fom
                "${CMAKE_CURRENT_SOURCE_DIR}/fom/attribute-invalid-update-type-tck.xml"
              --results
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        elseif(_tck_scenario STREQUAL
             "cpp-tck.authorizer-disabled-service-rejects-credentials")
          find_package(Python3 COMPONENTS Interpreter REQUIRED)
          add_test(
            NAME "${_tck_test_name}"
            COMMAND "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/adapters/current-process/run_authorizer_disabled_credentials.py"
              "$<TARGET_FILE:hla_rti_cpp_tck>"
              --
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
          )
        elseif(_tck_scenario STREQUAL
             "cpp-tck.authorizer-connect-service-decision" OR
           _tck_scenario STREQUAL
             "cpp-tck.authorizer-unrecognized-credentials-unauthorized")
          find_package(Python3 COMPONENTS Interpreter REQUIRED)
          add_test(
            NAME "${_tck_test_name}"
            COMMAND "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/adapters/current-process/run_authorizer_connect_decision.py"
              "$<TARGET_FILE:hla_rti_cpp_tck>"
              --
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
          )
        elseif((_tck_scenario STREQUAL
                  "cpp-tck.authorizer-rid-configuration-decision" OR
                _tck_scenario STREQUAL
                  "cpp-tck.authorizer-single-service-per-rid-profile") AND
               HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET)
          find_package(Python3 COMPONENTS Interpreter REQUIRED)
          add_test(
            NAME "${_tck_test_name}"
            COMMAND "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/adapters/current-process/run_authorizer_rid_configuration.py"
              "$<TARGET_FILE:hla_rti_cpp_tck>"
              --
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
          )
        elseif((_tck_scenario STREQUAL
                "cpp-tck.authorizer-federation-service-decision" OR
                _tck_scenario STREQUAL
                "cpp-tck.authorizer-destroy-service-decision" OR
                _tck_scenario STREQUAL
                "cpp-tck.authorizer-join-service-decision" OR
                _tck_scenario STREQUAL
                "cpp-tck.authorizer-no-credentials-connect" OR
                _tck_scenario STREQUAL
                "cpp-tck.authorizer-rid-configuration-decision" OR
                _tck_scenario STREQUAL
                "cpp-tck.authorizer-single-service-per-rid-profile") AND
               HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET)
          find_package(Python3 COMPONENTS Interpreter REQUIRED)
          if(_tck_scenario STREQUAL
             "cpp-tck.authorizer-destroy-service-decision")
            set(_tck_authorizer_service "TckDestroyGate")
          elseif(_tck_scenario STREQUAL
                 "cpp-tck.authorizer-join-service-decision")
            set(_tck_authorizer_service "TckJoinGate")
          elseif(_tck_scenario STREQUAL
                 "cpp-tck.authorizer-no-credentials-connect")
            set(_tck_authorizer_service "TckNoCredentialsGate")
          else()
            set(_tck_authorizer_service "TckFederationGate")
          endif()
          add_test(
            NAME "${_tck_test_name}"
            COMMAND "${Python3_EXECUTABLE}"
              "${CMAKE_CURRENT_SOURCE_DIR}/adapters/current-process/run_authorizer_federation_decision.py"
              "$<TARGET_FILE:hla_rti_cpp_tck>"
              "${_tck_authorizer_service}"
              --
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
          )
        elseif(_tck_scenario STREQUAL
               "cpp-tck.connection-additional-settings-parse-failure")
          if(NOT HLA_RTI_TCK_TEST_SETTINGS_PARSE_FAILURE_INPUT)
            message(FATAL_ERROR
              "The connection settings parse-failure TCK requires an adapter-provided malformed settings value")
          endif()
          file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tck-results")
          add_test(
            NAME "${_tck_test_name}"
            COMMAND hla_rti_cpp_tck
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
              --additional-settings
                "${HLA_RTI_TCK_TEST_SETTINGS_PARSE_FAILURE_INPUT}"
              --results
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        elseif(_tck_scenario STREQUAL
               "cpp-tck.unsupported-fom-transportation-fallback")
          if(NOT HLA_RTI_TCK_TEST_UNSUPPORTED_TRANSPORT_SETTINGS)
            message(FATAL_ERROR
              "The transportation-fallback TCK requires adapter-provided unsupported-transport settings")
          endif()
          file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tck-results")
          add_test(
            NAME "${_tck_test_name}"
            COMMAND hla_rti_cpp_tck
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
              --additional-settings
                "${HLA_RTI_TCK_TEST_UNSUPPORTED_TRANSPORT_SETTINGS}"
              --results
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        else()
          file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tck-results")
          add_test(
            NAME "${_tck_test_name}"
            COMMAND hla_rti_cpp_tck
              --common-arguments-file "${_tck_common_arguments_file}"
              --scenario "${_tck_scenario}"
              --callback-model "${_tck_callback_model}"
              --results
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.json"
              --junit
                "${CMAKE_CURRENT_BINARY_DIR}/tck-results/${_tck_scenario_slug}-${_tck_callback_model}.junit.xml"
          )
        endif()
        set(_tck_test_labels
          "portable-cpp-tck;scenario;${_tck_callback_model};${_tck_promotion_label}")
        if(_tck_scenario STREQUAL
           "cpp-tck.nmra-future-tso-arrival-pending-request")
          list(APPEND _tck_test_labels
            "process-time-advance-nmra-future-arrival-pending-request")
        endif()
        if(_tck_scenario STREQUAL
             "cpp-tck.nmr-request-below-galt-immediate-grant" OR
           _tck_scenario STREQUAL
             "cpp-tck.nmr-request-below-galt-immediate-grant-contract")
          list(APPEND _tck_test_labels nmr-cross-federate-grant)
        endif()
        if(_tck_scenario STREQUAL
             "cpp-tck.tara-no-tso-undefined-galt-nrg" OR
           _tck_scenario STREQUAL
             "cpp-tck.tara-no-tso-undefined-galt-nrg-contract")
          list(APPEND _tck_test_labels tara-no-tso-cross-federate-grant)
        endif()
        if(_tck_scenario STREQUAL
             "cpp-tck.timestamped-interaction-flush-queue-future-input" OR
           _tck_scenario STREQUAL
             "cpp-tck.timestamped-interaction-flush-queue-future-input-contract")
          list(APPEND _tck_test_labels fqr-cross-federate-delivery)
        endif()
        if(_tck_process_adapter)
          list(APPEND _tck_test_labels process-adapter)
        endif()
        # Primary/contract twins and callback-model variants can use the same
        # deterministic federation name. Serialize only that scenario family;
        # unrelated portable TCK scenarios remain eligible for parallel runs.
        string(REGEX REPLACE "_contract$" "" _tck_resource_slug
          "${_tck_scenario_slug}")
        set(_tck_resource_lock "portable_cpp_tck_${_tck_resource_slug}")
        set_tests_properties("${_tck_test_name}" PROPERTIES
          LABELS "${_tck_test_labels}"
          SKIP_REGULAR_EXPRESSION "skipped "
          RESOURCE_LOCK "${_tck_resource_lock}")
        if(_tck_provider_test_environment)
          set_tests_properties("${_tck_test_name}" PROPERTIES
            ENVIRONMENT "${_tck_provider_test_environment}")
        endif()
        if(_tck_scenario STREQUAL
             "cpp-tck.federation-supplied-custom-tradt" AND
           HLA_RTI_TCK_CUSTOM_TRADT_PROVIDER_TARGET)
          set_property(TEST "${_tck_test_name}" APPEND PROPERTY ENVIRONMENT
            "UMBRA_FEDTIME_PROVIDER_LIBRARY=$<TARGET_FILE:${HLA_RTI_TCK_CUSTOM_TRADT_PROVIDER_TARGET}>")
          set_property(TEST "${_tck_test_name}" APPEND PROPERTY
            ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:${HLA_RTI_TCK_CUSTOM_TRADT_PROVIDER_TARGET}>")
        endif()
        if((_tck_scenario STREQUAL
              "cpp-tck.authorizer-federation-service-decision" OR
            _tck_scenario STREQUAL
              "cpp-tck.authorizer-destroy-service-decision" OR
            _tck_scenario STREQUAL
              "cpp-tck.authorizer-join-service-decision" OR
            _tck_scenario STREQUAL
              "cpp-tck.authorizer-no-credentials-connect" OR
            _tck_scenario STREQUAL
              "cpp-tck.authorizer-rid-configuration-decision" OR
            _tck_scenario STREQUAL
              "cpp-tck.authorizer-single-service-per-rid-profile") AND
           HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET)
          set_property(TEST "${_tck_test_name}" APPEND PROPERTY ENVIRONMENT
            "UMBRA_AUTHORIZER_PROVIDER_LIBRARY=$<TARGET_FILE:${HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET}>")
          set_property(TEST "${_tck_test_name}" APPEND PROPERTY
            ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:${HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET}>")
        endif()
        if(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS GREATER 0)
          set_tests_properties("${_tck_test_name}" PROPERTIES
            TIMEOUT "${HLA_RTI_TCK_TEST_TIMEOUT_SECONDS}")
        endif()
      endforeach()
    endforeach()
  endif()
endif()
