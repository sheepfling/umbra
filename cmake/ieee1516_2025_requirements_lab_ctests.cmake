# IEEE 1516-2:2025 Requirements Lab CTest registrations.
# The 2010 integrity and conformance registrations remain in the parent CMake file.

    add_test(
      NAME umbra.ieee1516_2_2025.composition_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-composition-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.strict_omt_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-strict-omt-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.synchronization_table_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-synchronization-table-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.synchronization_merge_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-synchronization-merge-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_merge_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-merge-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.transportation_merge_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-transportation-merge-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.update_rate_merge_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-update-rate-merge-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.float_time_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/float-time-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.data_representation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-data-representation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.reference_data_special_instance_identifiers_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-reference-data-special-instance-identifiers-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.attribute_parameter_data_type_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-parameter-data-type-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.attribute_na_companion_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-attribute-na-companion-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.attribute_dynamic_update_condition_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-attribute-dynamic-update-condition-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.attribute_value_required_sharing_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-attribute-value-required-sharing-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.root_hierarchy_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-root-hierarchy-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.enumerated_representation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-enumerated-representation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.array_element_data_type_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-array-element-data-type-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.record_member_data_type_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-record-member-data-type-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_input_data_type_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-input-data-type-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_input_data_type_na_exclusivity_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-input-data-type-na-exclusivity-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_name_uniqueness_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-name-uniqueness-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.class_member_name_uniqueness_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-class-member-name-uniqueness-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.name_conventions_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-name-conventions-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.modification_date_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-modification-date-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_input_data_description_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-input-data-description-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.variant_discriminant_data_type_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-variant-discriminant-data-type-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.variant_discriminant_enumerator_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-variant-discriminant-enumerator-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.variant_discriminant_enumerator_membership_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-variant-discriminant-enumerator-membership-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.variant_discriminant_enumerator_range_semantics_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-variant-discriminant-enumerator-range-semantics-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.table_constraints_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-table-constraints-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.array_cardinality_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-array-cardinality-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_default_value_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-dimension-default-value-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.time_management_switches_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-time-management-switches-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.reference_time_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/reference-time-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.logical_time_encoding_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/logical-time-encoding-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.authorization_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/authorization-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_management_preparation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-management-preparation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.fom_source_diagnostics_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-source-diagnostics-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.fom_source_diagnostics_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-source-diagnostics-api-contract.json
    )
    add_test(
      NAME umbra.package_config
      COMMAND ${CMAKE_COMMAND}
        "-DUMBRA_CMAKE_COMMAND=${CMAKE_COMMAND}"
        "-DUMBRA_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
        "-DUMBRA_PYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
        "-DUMBRA_SOURCE_DIRECTORY=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DUMBRA_BINARY_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}"
        "-DUMBRA_GENERATOR=${CMAKE_GENERATOR}"
        "-DUMBRA_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DUMBRA_CONFIGURATION=$<CONFIG>"
        "-DUMBRA_EXPECT_PROCESS_PROFILE=${UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT}"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_installed_package_smoke.cmake
    )
    set_tests_properties(umbra.package_config PROPERTIES
      LABELS installable-package
    )
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_test(
        NAME umbra.ieee1516_2025.embedded_federation_management_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-management-embedded-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.explicit_mim_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/explicit-mim-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.explicit_mim_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/explicit-mim-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.resign_action_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/resign-action-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.resign_action_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/resign-action-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_lost_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/connection-lost-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_lost_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/connection-lost-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lost_report_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-lost-report-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lost_report_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-lost-report-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_resigned_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-resigned-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_resigned_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-resigned-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.get_time_factory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/get-time-factory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lookup_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-lookup-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-class-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_class_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/interaction-class-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.parameter_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/parameter-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_declaration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/interaction-declaration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_object_class_attributes_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-object-class-attributes-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_object_class_attributes_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-object-class-attributes-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_object_class_directed_interactions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-object-class-directed-interactions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_object_class_directed_interactions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-object-class-directed-interactions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_object_class_directed_interactions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-object-class-directed-interactions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_object_class_directed_interactions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-object-class-directed-interactions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_object_class_directed_interactions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-object-class-directed-interactions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_object_class_directed_interactions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-object-class-directed-interactions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_object_class_directed_interactions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-object-class-directed-interactions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_object_class_directed_interactions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-object-class-directed-interactions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_object_class_attributes_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-object-class-attributes-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_object_class_attributes_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-object-class-attributes-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_object_class_attributes_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-object-class-attributes-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_object_class_attributes_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-object-class-attributes-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_object_class_attributes_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-object-class-attributes-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_object_class_attributes_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-object-class-attributes-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_interaction_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-interaction-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.subscribe_interaction_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/subscribe-interaction-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_interaction_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-interaction-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unsubscribe_interaction_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unsubscribe-interaction-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_interaction_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-interaction-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.publish_interaction_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/publish-interaction-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_interaction_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-interaction-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unpublish_interaction_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unpublish-interaction-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/transportation-type-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/order-type-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/order-type-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_control_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/order-type-control-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/order-type-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_change_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/transportation-type-change-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_change_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/transportation-type-change-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_transportation_type_change_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/interaction-transportation-type-change-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.dimension_lookup_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/dimension-lookup-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/transportation-type-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.dimension_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/dimension-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.region_lifecycle_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/region-lifecycle-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.region_lifecycle_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/region-lifecycle-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/interaction-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_region_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/interaction-region-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-attribute-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.default_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/default-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_region_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-attribute-region-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_multi_attribute_ddm_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-multi-attribute-ddm-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_multi_attribute_ddm_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-multi-attribute-ddm-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_interaction_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/receive-order-interaction-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_interaction_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/receive-order-interaction-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.directed_interaction_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/directed-interaction-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.directed_interaction_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/directed-interaction-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_declaration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-class-attribute-declaration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_declaration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-class-attribute-declaration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.declaration_relevance_advisory_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/declaration-relevance-advisory-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.declaration_relevance_advisory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/declaration-relevance-advisory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_declaration_relevance_advisory_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-declaration-relevance-advisory-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_declaration_relevance_advisory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-declaration-relevance-advisory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_relevance_advisory_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-relevance-advisory-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_relevance_advisory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-relevance-advisory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_value_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/update-rate-value-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_value_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/update-rate-value-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_subscription_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/update-rate-subscription-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.fom_advisory_switches_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fom-advisory-switches-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_advisory_switch_initialization_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federate-advisory-switch-initialization-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.advisories_use_known_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/advisories-use-known-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.advisories_use_known_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/advisories-use-known-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.non_regulated_grant_switch_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/non-regulated-grant-switch-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.non_regulated_grant_switch_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/non-regulated-grant-switch-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.support_switches_table_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/support-switches-table-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.support_switches_service_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/support-switches-service-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.delay_subscription_evaluation_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/delay-subscription-evaluation-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.support_switches_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/support-switches-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.handle_normalization_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/handle-normalization-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.mom_service_reporting_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/mom-service-reporting-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.service_report_file_lifecycle_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/service-report-file-lifecycle-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.service_report_file_lifecycle_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/service-report-file-lifecycle-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.handle_normalization_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/handle-normalization-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.handle_decoding_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/handle-decoding-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.basic_data_elements_helper_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/basic-data-elements-helper-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.basic_data_elements_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/basic-data-elements-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.opaque_data_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/opaque-data-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.fixed_record_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fixed-record-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.fixed_array_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/fixed-array-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.variable_array_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/variable-array-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.variant_record_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/variant-record-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.extendable_variant_record_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/extendable-variant-record-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_callback_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/connection-callback-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_implementation_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/connection-implementation-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federation_execution_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-execution-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.convey_region_designator_callback_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/convey-region-designator-callback-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.auto_provide_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/auto-provide-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.auto_provide_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/auto-provide-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_registration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-registration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_registration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-registration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_name_reservation_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-name-reservation-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_name_reservation_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-name-reservation-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_named_registration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-named-registration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_named_registration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-named-registration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_scope_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-attribute-scope-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_scope_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-attribute-scope-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_deletion_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-deletion-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_deletion_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-instance-deletion-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.local_delete_object_instance_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/local-delete-object-instance-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.local_delete_object_instance_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/local-delete-object-instance-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_attribute_update_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/receive-order-attribute-update-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_attribute_update_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/receive-order-attribute-update-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_request_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-request-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_request_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-request-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_response_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-response-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_response_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-response-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_value_update_request_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-class-attribute-value-update-request-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_value_update_request_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/object-class-attribute-value-update-request-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_with_regions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-with-regions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_with_regions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-value-update-with-regions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_attribute_update_service_report_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-attribute-update-service-report-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.regional_attribute_update_service_report_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/regional-attribute-update-service-report-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_query_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-query-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_query_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-query-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_check_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-check-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_check_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-check-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_if_available_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-acquisition-if-available-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_if_available_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-acquisition-if-available-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-acquisition-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-acquisition-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_divestiture_if_wanted_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_divestiture_if_wanted_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/attribute-ownership-divestiture-if-wanted-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unconditional_attribute_ownership_divestiture_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unconditional-attribute-ownership-divestiture-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unconditional_attribute_ownership_divestiture_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/unconditional-attribute-ownership-divestiture-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.ownership_assumption_research_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/ownership-assumption-research-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.ownership_assumption_research_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/ownership-assumption-research-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_attribute_ownership_divestiture_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/negotiated-attribute-ownership-divestiture-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_attribute_ownership_divestiture_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/negotiated-attribute-ownership-divestiture-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_willing_to_acquire_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/negotiated-willing-to-acquire-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_willing_to_acquire_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/negotiated-willing-to-acquire-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federation_listing_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-listing-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federation_listing_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-listing-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.time_advance_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-advance-requirements-contract.json
      )
      add_test(
      NAME umbra.ieee1516_2025.time_advance_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-advance-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_role_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-role-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_role_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-role-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.modify_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/modify-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.modify_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/modify-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/next-message-request-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/next-message-request-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_advance_request_available_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-advance-request-available-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_advance_request_available_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-advance-request-available-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_available_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/next-message-request-available-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_available_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/next-message-request-available-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.flush_queue_request_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/flush-queue-request-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.flush_queue_request_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/flush-queue-request-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.asynchronous_delivery_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/asynchronous-delivery-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.asynchronous_delivery_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/asynchronous-delivery-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.synchronization_point_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/synchronization-point-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.synchronization_point_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/synchronization-point-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.save_control_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/save-control-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timed_save_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timed-save-requirements-contract.json
    )
      add_test(
        NAME umbra.ieee1516_2025.save_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/save-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.timed_save_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timed-save-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.restore_control_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/restore-control-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.restore_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/restore-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.save_restore_interlock_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/save-restore-interlock-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.save_restore_interlock_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/save-restore-interlock-api-contract.json
      )
    add_test(
      NAME umbra.ieee1516_2025.whole_object_class_declaration_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/whole-object-class-declaration-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.whole_object_class_declaration_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/whole-object-class-declaration-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_time_coordination_foundation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-time-coordination-foundation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_bounds_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-bounds-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_bounds_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-bounds-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_grant_policy_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-grant-policy-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_grant_scheduler_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/time-grant-scheduler-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.tso_message_queue_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/tso-message-queue-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_time_coordination_tso_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/federation-time-coordination-tso-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_interaction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-interaction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.request_retraction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/request-retraction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.request_retraction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/request-retraction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_regulation_reenable_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-regulation-reenable-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_regulation_reenable_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-regulation-reenable-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_interaction_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-interaction-regulation-reenable-changed-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_interaction_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-interaction-regulation-reenable-changed-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-regulation-reenable-changed-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-attribute-update-regulation-reenable-changed-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-object-deletion-regulation-reenable-changed-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-object-deletion-regulation-reenable-changed-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-directed-interaction-regulation-reenable-changed-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-directed-interaction-regulation-reenable-changed-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-interaction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_regulation_reenable_changed_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-attribute-update-regulation-reenable-changed-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_regulation_reenable_changed_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-attribute-update-regulation-reenable-changed-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-attribute-update-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-attribute-update-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.ownership_transfer_update_region_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/ownership-transfer-update-region-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.ownership_transfer_update_region_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/ownership-transfer-update-region-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-object-deletion-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-object-deletion-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-directed-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-directed-interaction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/timestamped-regional-interaction-api-contract.json
    )
  endif()
