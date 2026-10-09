# CTest lane targets and JUnit/package test orchestration for Catch2 cases.
# Included after the executable target declarations in the same directory scope.

    # Catch2 tags are the source of truth for focused CTest lanes.  Make the
    # taxonomy executable so a new test cannot silently escape both its
    # execution-scope lane and all domain lanes.
    add_test(
      NAME umbra.ieee1516_2025.catch2_tag_taxonomy
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_catch2_test_tags.py
        --catch2 $<TARGET_FILE:umbra_ieee1516_2025_catch2>
    )
    set_tests_properties(umbra.ieee1516_2025.catch2_tag_taxonomy
      PROPERTIES LABELS foundation
    )

    # Multi-config generators need CTest's explicit configuration flag. Do not
    # pass an empty `-C` for a single-config build that intentionally omits
    # CMAKE_BUILD_TYPE.
    if(CMAKE_CONFIGURATION_TYPES)
      set(_umbra_ctest_configuration_arguments -C "$<CONFIG>")
    elseif(CMAKE_BUILD_TYPE)
      set(_umbra_ctest_configuration_arguments -C "${CMAKE_BUILD_TYPE}")
    else()
      set(_umbra_ctest_configuration_arguments)
    endif()

    function(umbra_add_ctest_lane TARGET_NAME LABEL_REGEX)
      set(_umbra_lane_dependencies umbra_ieee1516_2025_catch2)
      if(ARGN)
        list(APPEND _umbra_lane_dependencies ${ARGN})
      endif()
      add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_CTEST_COMMAND}
          --test-dir "${CMAKE_BINARY_DIR}"
          ${_umbra_ctest_configuration_arguments}
          --output-on-failure
          -L "${LABEL_REGEX}"
        DEPENDS ${_umbra_lane_dependencies}
        COMMENT "Running Umbra ${TARGET_NAME} development test lane"
        USES_TERMINAL
        VERBATIM
      )
    endfunction()

    # A named service lane is intended to be a complete edit/build/test slice:
    # real C++ behavior plus its Requirements-Lab and API-contract checks. Keep
    # that promise executable as new services are added.
    function(umbra_add_ctest_service_lane TARGET_NAME LABEL)
      umbra_add_ctest_lane(${TARGET_NAME} "^${LABEL}$")
      set_property(
        GLOBAL
        APPEND
        PROPERTY UMBRA_CTEST_SERVICE_LANE_LABELS
        "${LABEL}"
      )
    endfunction()

    # The rapid lane has no Requirements-Lab contract checks; it is intended
    # for a tight edit/build/test loop.  Domain lanes combine tagged Catch2
    # cases with the matching source/API traceability checks above.
    # The rapid lane includes the three small foundation smoke tests as well
    # as Catch2's foundation/unit cases.  Make those executables explicit
    # dependencies so a fresh single-config build cannot report them as
    # "Not Run" merely because only the Catch2 binary was built.
    umbra_add_ctest_lane(
      umbra_test_rapid
      "^(foundation|unit)$"
      umbra_ieee1516_2025_headers_smoke
      umbra_ieee1516_2025_binding_shell_smoke
      umbra_federate_lifecycle_smoke
    )
    # Foundation is a standalone lane, so include the small header/binding/
    # lifecycle smoke executables when a clean build invokes it directly.
    # Without these explicit dependencies CTest can report the registered
    # smoke tests as "Not Run" even though the Catch2 foundation cases pass.
    umbra_add_ctest_lane(
      umbra_test_foundation
      "^foundation$"
      umbra_ieee1516_2025_headers_smoke
      umbra_ieee1516_2025_binding_shell_smoke
      umbra_federate_lifecycle_smoke
    )
    umbra_add_ctest_lane(umbra_test_federation_management "^federation-management$")
    umbra_add_ctest_service_lane(umbra_test_federation_listing federation-listing)
    umbra_add_ctest_service_lane(umbra_test_explicit_mim explicit-mim)
    umbra_add_ctest_lane(umbra_test_time_management "^time-management$")
    umbra_add_ctest_lane(umbra_test_float_time "^float-time$")
    umbra_add_ctest_lane(
      umbra_test_directed_target_departure
      "^directed-target-departure$"
    )
    # A fault-cutoff lane keeps the page-50 inclusive TSO regression local to
    # its cross-service Connection Lost/time-management edit loop.
    umbra_add_ctest_lane(
      umbra_test_connection_lost_tso_cutoff
      "^connection-lost-tso-cutoff$"
    )
    # A narrow composition/catalog lane protects the OMT P/S capability
    # fields without conflating them with run-time declaration state.
    umbra_add_ctest_lane(
      umbra_test_fom_directed_interaction_sharing
      "^directed-interaction-sharing$"
    )
    umbra_add_ctest_lane(
      umbra_test_fom_name_conventions
      "^fom-name-conventions$"
    )
    umbra_add_ctest_service_lane(umbra_test_time_advance_request time-advance-request)
    umbra_add_ctest_service_lane(
      umbra_test_time_advance_request_available
      time-advance-request-available
    )
    umbra_add_ctest_service_lane(umbra_test_next_message_request next-message-request)
    umbra_add_ctest_service_lane(
      umbra_test_next_message_request_available
      next-message-request-available
    )
    umbra_add_ctest_service_lane(umbra_test_flush_queue_request flush-queue-request)
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_default_region_attribute_update
      timestamped-default-region-attribute-update
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_attribute_update
      timestamped-regional-attribute-update
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_default_region_attribute_reenable
      timestamped-default-region-attribute-reenable
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_default_region_attribute_regulation_reenable
      timestamped-default-region-attribute-regulation-reenable
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_interaction_regulation_reenable_changed_lookahead
      timestamped-interaction-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_attribute_update_regulation_reenable_changed_lookahead
      timestamped-attribute-update-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_object_deletion_regulation_reenable_changed_lookahead
      timestamped-object-deletion-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_directed_interaction_regulation_reenable_changed_lookahead
      timestamped-directed-interaction-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_interaction_regulation_reenable_changed_lookahead
      timestamped-regional-interaction-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_ts_regional_attr_reenable_changed_lookahead
      timestamped-regional-attribute-update-regulation-reenable-changed-lookahead
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_attribute_reenable
      timestamped-regional-attribute-reenable
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_request_provider_response
      timestamped-regional-request-provider-response
    )
    umbra_add_ctest_service_lane(
      umbra_test_object_class_request_provider_response
      object-class-request-provider-response
    )
    # These response/mutation labels currently contain only Requirements-Lab
    # and API traceability checks. Keep them runnable as narrow CTest lanes,
    # but do not advertise them as complete service lanes until a real Catch2
    # behavior case is tagged for each one.
    umbra_add_ctest_lane(
      umbra_test_object_instance_request_provider_response
      object-instance-request-provider-response
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_interaction_reenable
      timestamped-regional-interaction-reenable
    )
    umbra_add_ctest_service_lane(
      umbra_test_delay_subscription_evaluation
      delay-subscription-evaluation
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_default_region_interaction
      timestamped-default-region-interaction
    )
    umbra_add_ctest_service_lane(umbra_test_retract retract)
    umbra_add_ctest_service_lane(umbra_test_federate_save_begun federate-save-begun)
    umbra_add_ctest_service_lane(umbra_test_federate_save_complete federate-save-complete)
    umbra_add_ctest_service_lane(umbra_test_timed_save timed-save)
    umbra_add_ctest_service_lane(umbra_test_abort_federation_save abort-federation-save)
    umbra_add_ctest_service_lane(
      umbra_test_query_federation_save_status
      query-federation-save-status
    )
    umbra_add_ctest_service_lane(
      umbra_test_federation_save_status_response_service_report
      federation-save-status-response-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_request_federation_save
      request-federation-save
    )
    umbra_add_ctest_service_lane(
      umbra_test_initiate_federate_save_service_report
      initiate-federate-save-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_federation_saved_service_report
      federation-saved-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_request_federation_restore
      request-federation-restore
    )
    umbra_add_ctest_service_lane(
      umbra_test_confirm_federation_restoration_request_service_report
      confirm-federation-restoration-request-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_federation_restore_begun_service_report
      federation-restore-begun-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_initiate_federate_restore_service_report
      initiate-federate-restore-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_federate_restore_complete
      federate-restore-complete
    )
    umbra_add_ctest_service_lane(
      umbra_test_abort_federation_restore
      abort-federation-restore
    )
    umbra_add_ctest_service_lane(
      umbra_test_query_federation_restore_status
      query-federation-restore-status
    )
    umbra_add_ctest_service_lane(
      umbra_test_federation_restore_status_response_service_report
      federation-restore-status-response-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_resign_federation_execution
      resign-federation-execution
    )
    umbra_add_ctest_service_lane(
      umbra_test_federate_resigned_service_report
      federate-resigned-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_service_report
      connection-lost-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_automatic_resign
      connection-lost-automatic-resign
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_final_federate
      connection-lost-final-federate
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_error_path
      connection-lost-error-path
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_negotiated_cancellation
      connection-lost-negotiated-cancellation
    )
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_report_ordering
      connection-lost-report-ordering
    )
    umbra_add_ctest_lane(
      umbra_test_connection_lost_directed_selector_mutation
      connection-lost-directed-selector-mutation
    )
    umbra_add_ctest_lane(
      umbra_test_connection_lost_regional_selector_mutation
      connection-lost-regional-selector-mutation
    )
    umbra_add_ctest_service_lane(
      umbra_test_register_federation_synchronization_point
      register-federation-synchronization-point
    )
    umbra_add_ctest_service_lane(
      umbra_test_confirm_synchronization_point_registration_service_report
      confirm-synchronization-point-registration-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_announce_synchronization_point_service_report
      announce-synchronization-point-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_federation_synchronized_service_report
      federation-synchronized-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_synchronization_point_achieved
      synchronization-point-achieved
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_directed_interaction
      timestamped-directed-interaction
    )
    umbra_add_ctest_service_lane(
      umbra_test_change_interaction_order_type
      change-interaction-order-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_change_attribute_order_type
      change-attribute-order-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_change_default_attribute_order_type
      change-default-attribute-order-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_change_default_attribute_transportation_type
      change-default-attribute-transportation-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_query_attribute_transportation_type
      query-attribute-transportation-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_query_interaction_transportation_type
      query-interaction-transportation-type
    )
    umbra_add_ctest_service_lane(
      umbra_test_transportation_type_lookup
      transportation-type-lookup
    )
    umbra_add_ctest_service_lane(
      umbra_test_query_attribute_ownership
      query-attribute-ownership
    )
    umbra_add_ctest_service_lane(
      umbra_test_unconditional_attribute_ownership_divestiture
      unconditional-attribute-ownership-divestiture
    )
    umbra_add_ctest_service_lane(
      umbra_test_ownership_assumption_research
      ownership-assumption-research
    )
    umbra_add_ctest_service_lane(
      umbra_test_ownership_assumption_search_epoch
      ownership-assumption-search-epoch
    )
    umbra_add_ctest_service_lane(
      umbra_test_ownership_assumption_search_continuation
      ownership-assumption-search-continuation
    )
    umbra_add_ctest_service_lane(
      umbra_test_ownership_acquisition_cancellation_race
      ownership-acquisition-cancellation-race
    )
    umbra_add_ctest_service_lane(
      umbra_test_negotiated_willing_to_acquire
      negotiated-willing-to-acquire
    )
    umbra_add_ctest_service_lane(
      umbra_test_local_delete_object_instance
      local-delete-object-instance
    )
    umbra_add_ctest_service_lane(
      umbra_test_local_delete_failure
      local-delete-failure
    )
    umbra_add_ctest_lane(
      umbra_test_delete_failure
      delete-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_publish_object_class_attributes
      publish-object-class-attributes
    )
    umbra_add_ctest_service_lane(
      umbra_test_publish_object_class_directed_interactions
      publish-object-class-directed-interactions
    )
    umbra_add_ctest_service_lane(
      umbra_test_unpublish_object_class_directed_interactions
      unpublish-object-class-directed-interactions
    )
    umbra_add_ctest_service_lane(
      umbra_test_subscribe_object_class_directed_interactions
      subscribe-object-class-directed-interactions
    )
    umbra_add_ctest_service_lane(
      umbra_test_unsubscribe_object_class_directed_interactions
      unsubscribe-object-class-directed-interactions
    )
    umbra_add_ctest_service_lane(
      umbra_test_unpublish_object_class_attributes
      unpublish-object-class-attributes
    )
    umbra_add_ctest_service_lane(
      umbra_test_subscribe_object_class_attributes
      subscribe-object-class-attributes
    )
    umbra_add_ctest_service_lane(
      umbra_test_unsubscribe_object_class_attributes
      unsubscribe-object-class-attributes
    )
    umbra_add_ctest_service_lane(
      umbra_test_subscribe_interaction_class
      subscribe-interaction-class
    )
    umbra_add_ctest_service_lane(
      umbra_test_unsubscribe_interaction_class
      unsubscribe-interaction-class
    )
    umbra_add_ctest_service_lane(
      umbra_test_publish_interaction_class
      publish-interaction-class
    )
    umbra_add_ctest_service_lane(
      umbra_test_unpublish_interaction_class
      unpublish-interaction-class
    )
    umbra_add_ctest_service_lane(
      umbra_test_declaration_relevance_advisory_service_report
      declaration-relevance-advisory-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_declaration_relevance_advisory
      declaration-relevance-advisory-regional
    )
    umbra_add_ctest_service_lane(
      umbra_test_discover_object_instance_service_report
      discover-object-instance-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_remove_object_instance_service_report
      remove-object-instance-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_object_instance_name_reservation_service_report
      object-instance-name-reservation-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_object_name_reservation_failure
      object-name-reservation-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_object_attribute_association_service_report
      regional-object-attribute-association-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_object_attribute_subscription_service_report
      regional-object-attribute-subscription-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_interaction_subscription_service_report
      regional-interaction-subscription-service-report
    )
    umbra_add_ctest_lane(
      umbra_test_ordinary_regional_interaction_service_report
      ordinary-regional-interaction-service-report
    )
    umbra_add_ctest_lane(
      umbra_test_ordinary_regional_interaction_failure
      ordinary-regional-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_remove_object_instance_service_report
      timestamped-remove-object-instance-service-report
    )
    umbra_add_ctest_lane(
      umbra_test_timestamped_delete_object_instance_service_report
      timestamped-delete-object-instance-service-report
    )
    umbra_add_ctest_lane(
      umbra_test_timestamped_delete_failure
      timestamped-delete-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_provide_attribute_value_update_service_report
      provide-attribute-value-update-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_auto_provide_service_report
      auto-provide-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_provide_attribute_value_update_class_service_report
      provide-attribute-value-update-class-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_attribute_update_failure
      timestamped-attribute-update-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_attribute_update_service_report
      timestamped-attribute-update-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_attribute_update_service_report
      timestamped-regional-attribute-update-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_attribute_update_service_report
      ordinary-regional-attribute-update-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_attribute_update_failure
      ordinary-regional-attribute-update-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_regional_attribute_update_failure
      timestamped-regional-attribute-update-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_send_interaction_failure
      timestamped-send-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_send_interaction_service_report
      timestamped-send-interaction-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_receive_order_send_interaction_service_report_interaction
      receive-order-send-interaction-service-report-interaction
    )
    umbra_add_ctest_lane(
      umbra_test_timestamped_regional_interaction_service_report
      timestamped-regional-interaction-service-report
    )
    umbra_add_ctest_lane(
      umbra_test_timestamped_regional_interaction_failure
      timestamped-regional-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_directed_interaction_failure
      timestamped-directed-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_directed_interaction_service_report
      timestamped-directed-interaction-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_directed_send_interaction_service_report_interaction
      directed-send-interaction-service-report-interaction
    )
    umbra_add_ctest_service_lane(
      umbra_test_provide_attribute_value_update_regional_service_report
      provide-attribute-value-update-regional-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_regional_attribute_value_update_request_service_report
      regional-attribute-value-update-request-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_commit_region_modifications_service_report
      commit-region-modifications-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_delete_region_service_report
      delete-region-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_set_range_bounds_service_report
      set-range-bounds-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_nonvoid_failure
      ddm-nonvoid-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_mutation_failure
      ddm-mutation-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_regional_failure
      ddm-regional-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_regional_object_failure
      ddm-regional-object-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_regional_association_failure
      ddm-regional-association-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_ddm_regional_multi_attribute
      ddm-regional-multi-attribute
    )
    umbra_add_ctest_service_lane(
      umbra_test_declaration_interaction_failure
      declaration-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_declaration_subscription_failure
      declaration-subscription-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_directed_declaration_failure
      directed-declaration-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_directed_subscription_failure
      directed-subscription-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_object_attribute_declaration_failure
      object-attribute-declaration-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_object_attribute_subscription_failure
      object-attribute-subscription-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_request_interaction_transportation_type_change
      request-interaction-transportation-type-change
    )
    umbra_add_ctest_service_lane(
      umbra_test_request_attribute_transportation_type_change
      request-attribute-transportation-type-change
    )
    umbra_add_ctest_lane(umbra_test_time_role "^time-role$")
    umbra_add_ctest_lane(umbra_test_modify_lookahead "^modify-lookahead$")
    umbra_add_ctest_lane(umbra_test_asynchronous_delivery "^asynchronous-delivery$")
    umbra_add_ctest_lane(umbra_test_ddm "^ddm$")
    umbra_add_ctest_lane(umbra_test_mom "^mom$")
    umbra_add_ctest_lane(umbra_test_service_reporting "^service-reporting$")
    umbra_add_ctest_lane(umbra_test_service_report_file "^service-report-file$")
    umbra_add_ctest_service_lane(umbra_test_service_report_store service-report-store)
    umbra_add_ctest_service_lane(
      umbra_test_service_report_file_lifecycle
      service-report-file-lifecycle
    )
    umbra_add_ctest_lane(umbra_test_support_switches "^support-switches$")
    umbra_add_ctest_lane(umbra_test_fom "^fom$")
    umbra_add_ctest_lane(
      umbra_test_fom_member_name_uniqueness
      "^fom-member-name-uniqueness$"
    )
    umbra_add_ctest_lane(umbra_test_fom_omt_complete_model "^omt-complete-model$")
    umbra_add_ctest_service_lane(
      umbra_test_fom_source_diagnostics
      fom-source-diagnostics
    )
    # Annex C composition is intentionally a dedicated lane: it combines all
    # C.1-C.10 Catch2 cases with their exact Requirements-Lab contracts but
    # does not claim a service/API-contract lane.
    umbra_add_ctest_lane(umbra_test_fom_annex_c "^annex-c$")
    umbra_add_ctest_lane(umbra_test_object_management "^object-management$")
    umbra_add_ctest_lane(umbra_test_ownership_management "^ownership-management$")
    umbra_add_ctest_lane(umbra_test_declaration_management "^declaration-management$")
    umbra_add_ctest_lane(umbra_test_interaction_management "^interaction-management$")
    umbra_add_ctest_lane(umbra_test_save_restore "^save-restore$")
    umbra_add_ctest_service_lane(umbra_test_region_lifecycle region-lifecycle)
    umbra_add_ctest_lane(umbra_test_callbacks "^callbacks$")
    # A cross-cutting callback-model lane keeps direct-dispatch regressions
    # fast without forcing a whole domain lane.
    umbra_add_ctest_lane(umbra_test_callback_immediate "^callback-immediate$")
    # The roadmap's multi-federate callback-ordering slice spans several
    # domains. Keep its existing Catch2 tag directly runnable as one focused
    # edit/build/test lane instead of requiring a full-suite scan.
    umbra_add_ctest_lane(
      umbra_test_multi_federate_callback_ordering
      "^multi-federate-callback-ordering$"
    )

    get_property(
      _umbra_ctest_service_lane_labels
      GLOBAL
      PROPERTY UMBRA_CTEST_SERVICE_LANE_LABELS
    )
    set(_umbra_ctest_service_lane_arguments)
    foreach(_umbra_ctest_service_lane_label IN LISTS _umbra_ctest_service_lane_labels)
      list(APPEND _umbra_ctest_service_lane_arguments
        --lane "${_umbra_ctest_service_lane_label}"
      )
    endforeach()
    if(CMAKE_CONFIGURATION_TYPES)
      set(_umbra_ctest_service_lane_configuration_arguments --config "$<CONFIG>")
    elseif(CMAKE_BUILD_TYPE)
      set(_umbra_ctest_service_lane_configuration_arguments --config "${CMAKE_BUILD_TYPE}")
    else()
      set(_umbra_ctest_service_lane_configuration_arguments)
    endif()
    add_test(
      NAME umbra.ieee1516_2025.focused_service_lane_catalog
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_ctest_service_lanes.py
        --ctest ${CMAKE_CTEST_COMMAND}
        --test-dir ${CMAKE_BINARY_DIR}
        ${_umbra_ctest_service_lane_configuration_arguments}
        ${_umbra_ctest_service_lane_arguments}
    )
    set_tests_properties(umbra.ieee1516_2025.focused_service_lane_catalog
      PROPERTIES LABELS foundation
    )
    add_custom_target(umbra_test_all
      COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir "${CMAKE_BINARY_DIR}"
        ${_umbra_ctest_configuration_arguments}
        --output-on-failure
      DEPENDS umbra_ieee1516_2025_catch2
      COMMENT "Running the complete Umbra CTest regression suite"
      USES_TERMINAL
      VERBATIM
    )

    # Keep the installable profile as a named, reproducible edit/build/test
    # slice.  The test script builds every exported runtime target, stages the
    # package, validates its profile/resource manifests, and runs a clean
    # downstream consumer; callers need not remember a target-ordering recipe.
    set(_umbra_installable_package_dependencies
      umbra_rti
      umbra_rti_2010
      umbra_fedtime
      umbra_authorizer
    )
    if(TARGET umbra_process_federation_service_probe)
      list(APPEND _umbra_installable_package_dependencies
        umbra_process_federation_service_probe
      )
    endif()
    add_custom_target(umbra_test_installable_package
      COMMAND ${CMAKE_CTEST_COMMAND}
        --test-dir "${CMAKE_BINARY_DIR}"
        ${_umbra_ctest_configuration_arguments}
        --output-on-failure
        -L "^installable-package$"
      DEPENDS ${_umbra_installable_package_dependencies}
      COMMENT "Running Umbra installable-package profile smoke"
      USES_TERMINAL
      VERBATIM
    )

    add_custom_target(umbra_catch2_junit
      COMMAND ${CMAKE_COMMAND} -E make_directory "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}"
      COMMAND $<TARGET_FILE:umbra_ieee1516_2025_catch2>
        "[compliance]"
        --allow-running-no-tests
        --reporter junit
        --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/catch2-results.xml"
      DEPENDS umbra_ieee1516_2025_catch2
      COMMENT "Writing Catch2 compliance JUnit artifact"
      VERBATIM
    )

    # Emit JUnit for the exact Catch2 tag in the active mapped process-boundary lane.
    if(TARGET umbra_ieee1516_2025_connection_catch2)
      # The public process-boundary source is independently buildable; the
      # aggregate target remains the fallback for profiles that do not enable
      # the embedded service.
      set(_umbra_process_boundary_public_target
        umbra_ieee1516_2025_connection_catch2
      )
    else()
      set(_umbra_process_boundary_public_target
        umbra_ieee1516_2025_catch2
      )
    endif()
    if(
      TARGET umbra_process_boundary_private_catch2
      AND TARGET umbra_ieee1516_2025_connection_catch2
      AND Python3_Interpreter_FOUND
    )
      # The indexed process-boundary lane contains independently buildable
      # private service/transport, public endpoint, and ownership/query cases.
      # Keep every source target separate and merge only its valid XML report;
      # the damaged aggregate federation-management translation unit must not
      # be the hidden source of JUnit evidence.
      set(_umbra_process_boundary_junit_commands
        COMMAND $<TARGET_FILE:umbra_process_boundary_private_catch2>
          "[process-boundary]"
          --allow-running-no-tests
          --reporter junit
          --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-private.xml"
        COMMAND $<TARGET_FILE:${_umbra_process_boundary_public_target}>
          "[process-boundary]"
          --allow-running-no-tests
          --reporter junit
          --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-public.xml"
      )
      set(_umbra_process_boundary_junit_inputs
        "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-private.xml"
        "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-public.xml"
      )
      set(_umbra_process_boundary_junit_dependencies
        umbra_process_boundary_private_catch2
        ${_umbra_process_boundary_public_target}
      )
      # These dedicated ownership/query targets are also registered with the
      # process-boundary CTest label. Include them when present so the JUnit
      # artifact covers every independently buildable source target; the
      # aggregate umbrella registration remains intentionally CTest-only.
      if(TARGET umbra_attribute_ownership_query_catch2)
        list(APPEND _umbra_process_boundary_junit_commands
          COMMAND $<TARGET_FILE:umbra_attribute_ownership_query_catch2>
            "[process-boundary]"
            --allow-running-no-tests
            --reporter junit
            --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-query.xml"
        )
        list(APPEND _umbra_process_boundary_junit_inputs
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-query.xml"
        )
        list(APPEND _umbra_process_boundary_junit_dependencies
          umbra_attribute_ownership_query_catch2
        )
      endif()
      if(TARGET umbra_attribute_ownership_acquisition_if_available_catch2)
        list(APPEND _umbra_process_boundary_junit_commands
          COMMAND $<TARGET_FILE:umbra_attribute_ownership_acquisition_if_available_catch2>
            "[process-boundary]"
            --allow-running-no-tests
            --reporter junit
            --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-acquisition-if-available.xml"
        )
        list(APPEND _umbra_process_boundary_junit_inputs
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-acquisition-if-available.xml"
        )
        list(APPEND _umbra_process_boundary_junit_dependencies
          umbra_attribute_ownership_acquisition_if_available_catch2
        )
      endif()
      if(TARGET umbra_attribute_ownership_acquisition_catch2)
        list(APPEND _umbra_process_boundary_junit_commands
          COMMAND $<TARGET_FILE:umbra_attribute_ownership_acquisition_catch2>
            "[process-boundary]"
            --allow-running-no-tests
            --reporter junit
            --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-acquisition.xml"
        )
        list(APPEND _umbra_process_boundary_junit_inputs
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary-ownership-acquisition.xml"
        )
        list(APPEND _umbra_process_boundary_junit_dependencies
          umbra_attribute_ownership_acquisition_catch2
        )
      endif()
      add_custom_target(umbra_process_boundary_junit
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary"
        ${_umbra_process_boundary_junit_commands}
        COMMAND ${Python3_EXECUTABLE}
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/merge_junit_reports.py"
          --output
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary.xml"
          ${_umbra_process_boundary_junit_inputs}
        DEPENDS
          ${_umbra_process_boundary_junit_dependencies}
        COMMENT "Writing complete process-boundary Catch2 JUnit artifact"
        VERBATIM
      )
    else()
      add_custom_target(umbra_process_boundary_junit
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary"
        COMMAND $<TARGET_FILE:${_umbra_process_boundary_public_target}>
          "[process-boundary]"
          --allow-running-no-tests
          --reporter junit
          --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/process-boundary/process-boundary.xml"
        DEPENDS ${_umbra_process_boundary_public_target}
        COMMENT "Writing process-boundary Catch2 JUnit artifact"
        VERBATIM
      )
    endif()
