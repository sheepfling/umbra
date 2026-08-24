cmake_minimum_required(VERSION 3.23)

project(
  umbra_rti
  VERSION 0.1.0
  DESCRIPTION "A native C++ HLA RTI"
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CTest)
include(CMakePackageConfigHelpers)

option(UMBRA_FETCH_CATCH2 "Download the pinned Catch2 test dependency when it is unavailable" OFF)
option(
  UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR
  "Build the private libxml2-backed IEEE 1516.2 FOM validator"
  OFF
)
option(
  UMBRA_FETCH_LIBXML2
  "Download pinned libxml2 when the private FOM validator is enabled and unavailable"
  OFF
)
option(
  UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT
  "Enable the non-installable embedded federation-management development profile"
  OFF
)
set(
  UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY
  ""
  CACHE PATH
  "Optional unvendored SISO FOM corpus root for development-only 2025 boundary-rejection tests"
)
set(
  UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY
  ""
  CACHE PATH
  "Optional unvendored 2025 FOM corpus root for development-only positive validation tests"
)
if(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY)
  if(NOT UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
    )
  endif()
  get_filename_component(
    UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY
    "${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(NOT IS_DIRECTORY "${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}")
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY is not a directory: ${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}"
    )
  endif()
endif()
if(UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY)
  if(NOT UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
    )
  endif()
  get_filename_component(
    UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY
    "${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(NOT IS_DIRECTORY "${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}")
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY is not a directory: ${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}"
    )
  endif()
endif()
set(UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY "${CMAKE_BINARY_DIR}/compliance"
  CACHE PATH "Directory for generated Catch2 JUnit artifacts"
)

set(UMBRA_IEEE1516_2025_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ieee1516.1-2025/include"
)
if(NOT EXISTS "${UMBRA_IEEE1516_2025_INCLUDE_DIR}/RTI/RTI1516.h")
  message(FATAL_ERROR "The vendored IEEE 1516.1-2025 C++ headers are missing")
endif()

set(UMBRA_IEEE1516_2_2025_RESOURCE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ieee1516.2-2025/resources"
)
if(NOT EXISTS "${UMBRA_IEEE1516_2_2025_RESOURCE_DIR}/schemas/IEEE1516-OMT-2025.xsd")
  message(FATAL_ERROR "The vendored IEEE 1516.2-2025 OMT resources are missing")
endif()

add_library(ieee1516_2025_headers INTERFACE)
add_library(umbra::ieee1516_2025_headers ALIAS ieee1516_2025_headers)
target_include_directories(ieee1516_2025_headers SYSTEM INTERFACE
  $<BUILD_INTERFACE:${UMBRA_IEEE1516_2025_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# The implementation starts as a standards-shaped static binding with an
# embedded connection slice. The official headers prescribe STATIC_RTI for both
# a static librti and its consumers; exporting an incomplete DLL would require
# every official class. The generated fallback overrides every pure virtual
# member and throws RTIinternalError unless a real vertical slice overrides it.
add_library(umbra_rti STATIC
  cpp/src/ieee1516_2025_attribute_handle.cpp
  cpp/src/ieee1516_2025_authorization.cpp
  cpp/src/ieee1516_2025_basic_data_elements.cpp
  cpp/src/ieee1516_2025_binding_shell.cpp
  cpp/src/ieee1516_2025_dimension_handle.cpp
  cpp/src/ieee1516_2025_extendable_variant_record.cpp
  cpp/src/ieee1516_2025_federate_handle.cpp
  cpp/src/ieee1516_2025_fixed_array.cpp
  cpp/src/ieee1516_2025_fixed_record.cpp
  cpp/src/ieee1516_2025_interaction_class_handle.cpp
  cpp/src/ieee1516_2025_message_retraction_handle.cpp
  cpp/src/ieee1516_2025_object_class_handle.cpp
  cpp/src/ieee1516_2025_object_instance_handle.cpp
  cpp/src/ieee1516_2025_opaque_data.cpp
  cpp/src/ieee1516_2025_parameter_handle.cpp
  cpp/src/ieee1516_2025_range_bounds.cpp
  cpp/src/ieee1516_2025_region_handle.cpp
  cpp/src/ieee1516_2025_support.cpp
  cpp/src/ieee1516_2025_logical_time_encoding.cpp
  cpp/src/ieee1516_2025_time.cpp
  cpp/src/ieee1516_2025_transportation_type_handle.cpp
  cpp/src/ieee1516_2025_variable_array.cpp
  cpp/src/ieee1516_2025_variant_record.cpp
  cpp/src/internal/callbacks/callback_dispatcher.cpp
  cpp/src/internal/callbacks/callback_session.cpp
  cpp/src/internal/federation/embedded_transport.cpp
  cpp/src/internal/handles/attribute_handle_directory.cpp
  cpp/src/internal/handles/dimension_handle_directory.cpp
  cpp/src/internal/federation/federate_lifecycle.cpp
  cpp/src/internal/time/federate_time_state.cpp
  cpp/src/internal/federation/federation_management_coordinator.cpp
  cpp/src/internal/federation/federation_registry.cpp
  cpp/src/internal/handles/interaction_class_handle_directory.cpp
  cpp/src/internal/observability/mom_service_report_encoding.cpp
  cpp/src/internal/observability/service_report_store.cpp
  cpp/src/internal/handles/object_class_handle_directory.cpp
  cpp/src/internal/handles/parameter_handle_directory.cpp
  cpp/src/internal/time/federation_time_bounds.cpp
  cpp/src/internal/time/federation_time_coordinator.cpp
  cpp/src/internal/time/federation_time_grant_policy.cpp
  cpp/src/internal/time/reference_time_selection.cpp
  cpp/src/internal/observability/runtime_instrumentation.cpp
  cpp/src/internal/observability/runtime_instrumentation_output.cpp
  cpp/src/internal/time/update_rate_gate.cpp
  cpp/src/internal/time/tso_message_queue.cpp
  cpp/src/internal/runtime/umbra_rti_ambassador.cpp
  cpp/src/internal/runtime/utf8_string.cpp
)
add_library(umbra::rti ALIAS umbra_rti)
set_target_properties(umbra_rti PROPERTIES EXPORT_NAME rti)
target_compile_features(umbra_rti PUBLIC cxx_std_20)
target_compile_definitions(umbra_rti PUBLIC STATIC_RTI)
if(MSVC)
  # The standards-shaped registry and ambassador intentionally keep the
  # vertical-slice state machine in translation units shared by every build
  # profile.  As those official service surfaces grow, MSVC's default 65,535
  # section limit is exceeded even though the code is otherwise valid.
  target_compile_options(umbra_rti PRIVATE /bigobj)
endif()
target_include_directories(umbra_rti
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
)
target_link_libraries(umbra_rti PUBLIC umbra::ieee1516_2025_headers)

# The IEEE binding separates the user-selectable LogicalTimeFactoryFactory
# into libfedtime. Keep that boundary even while Umbra ships the two mandated
# reference time representations with its static RTI library.
add_library(umbra_fedtime STATIC cpp/src/ieee1516_2025_fedtime.cpp)
add_library(umbra::fedtime ALIAS umbra_fedtime)
set_target_properties(umbra_fedtime PROPERTIES EXPORT_NAME fedtime)
target_compile_features(umbra_fedtime PUBLIC cxx_std_20)
target_compile_definitions(umbra_fedtime PUBLIC STATIC_FEDTIME)
target_link_libraries(umbra_fedtime PUBLIC umbra::rti)

# The C++ binding similarly separates the generic AuthorizerFactoryFactory
# entry point into libauthorizer. The reference HLAauthorizer implementation
# stays in the RTI library; this target preserves the standard forwarding
# boundary without inventing third-party RID loading before it exists.
add_library(umbra_authorizer STATIC cpp/src/ieee1516_2025_authorizer.cpp)
add_library(umbra::authorizer ALIAS umbra_authorizer)
set_target_properties(umbra_authorizer PROPERTIES EXPORT_NAME authorizer)
target_compile_features(umbra_authorizer PUBLIC cxx_std_20)
target_compile_definitions(umbra_authorizer PUBLIC STATIC_AUTHORIZER)
target_link_libraries(umbra_authorizer PUBLIC umbra::rti)

if(UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
  find_package(LibXml2 2.15 CONFIG QUIET)
  if(NOT TARGET LibXml2::LibXml2 AND UMBRA_FETCH_LIBXML2)
    include(FetchContent)
    # Keep the private validator self-contained for its test target. The
    # completed SDK packaging decision is deferred until public RTI services
    # link this backend.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_CATALOG OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_DOCS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_HTML OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_HTTP OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_LEGACY OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_MODULES OFF CACHE BOOL "" FORCE)
    # The private materializer serializes a schema-validated FDD into an
    # immutable runtime artifact. This does not enable XML network loading or
    # external entity resolution; those stay disabled in the document loader.
    set(LIBXML2_WITH_OUTPUT ON CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_VALID OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_XINCLUDE OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_XPATH OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_SCHEMAS ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
      LibXml2
      URL https://download.gnome.org/sources/libxml2/2.15/libxml2-2.15.3.tar.xz
      URL_HASH SHA256=78262a6e7ac170d6528ebfe2efccdf220191a5af6a6cd61ea4a9a9a5042c7a07
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(LibXml2)
  endif()
  if(NOT TARGET LibXml2::LibXml2)
    message(FATAL_ERROR
      "UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR requires LibXml2 2.15+; configure with -DUMBRA_FETCH_LIBXML2=ON to fetch the pinned source."
    )
  endif()

  add_library(umbra_fom_validation_backend STATIC
    cpp/src/internal/fom/libxml2_fom_composer.cpp
    cpp/src/internal/fom/libxml2_fom_document.cpp
    cpp/src/internal/fom/libxml2_fom_validator.cpp
  )
  target_compile_features(umbra_fom_validation_backend PRIVATE cxx_std_20)
  target_include_directories(umbra_fom_validation_backend PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
  )
  target_link_libraries(umbra_fom_validation_backend PRIVATE LibXml2::LibXml2)

  if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    # This first public federation-management slice intentionally remains a
    # source-tree development profile. Its libxml2 linkage is not yet part of
    # the installed SDK dependency contract, so the install/package targets
    # below stay disabled for this configuration.
    target_compile_definitions(umbra_rti PRIVATE
      UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY=L"${UMBRA_IEEE1516_2_2025_RESOURCE_DIR}"
    )
    target_link_libraries(umbra_rti PRIVATE umbra_fom_validation_backend)
    message(WARNING
      "UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT is a non-installable development profile; disable it for the packaged SDK."
    )
  endif()
elseif(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  message(FATAL_ERROR
    "UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
  )
endif()

if(BUILD_TESTING)
  add_executable(umbra_ieee1516_2025_headers_smoke cpp/tests/ieee1516_2025_headers_smoke.cpp)
  target_compile_features(umbra_ieee1516_2025_headers_smoke PRIVATE cxx_std_20)
  target_link_libraries(umbra_ieee1516_2025_headers_smoke PRIVATE umbra::ieee1516_2025_headers)
  add_test(NAME umbra.ieee1516_2025.headers COMMAND umbra_ieee1516_2025_headers_smoke)

  add_executable(umbra_ieee1516_2025_binding_shell_smoke
    cpp/tests/ieee1516_2025_binding_shell_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2025_binding_shell_smoke PRIVATE cxx_std_20)
  target_link_libraries(umbra_ieee1516_2025_binding_shell_smoke PRIVATE umbra::rti)
  target_include_directories(umbra_ieee1516_2025_binding_shell_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
  )
  if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    target_compile_definitions(umbra_ieee1516_2025_binding_shell_smoke PRIVATE
      UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
    )
  endif()
  add_test(
    NAME umbra.ieee1516_2025.binding_shell
    COMMAND umbra_ieee1516_2025_binding_shell_smoke
  )

  add_executable(umbra_federate_lifecycle_smoke cpp/tests/federate_lifecycle_smoke.cpp)
  target_compile_features(umbra_federate_lifecycle_smoke PRIVATE cxx_std_20)
  target_include_directories(umbra_federate_lifecycle_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
  )
  target_link_libraries(umbra_federate_lifecycle_smoke PRIVATE umbra::rti)
  add_test(NAME umbra.kernel.federate_lifecycle COMMAND umbra_federate_lifecycle_smoke)

  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    add_test(
      NAME umbra.ieee1516_2025.integrity
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_ieee_headers.py
    )
    add_test(
      NAME umbra.ieee1516_2025.exception_binding
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_ieee_exception_binding.py
    )
    add_test(
      NAME umbra.ieee1516_2025.binding_inventory
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_binding_inventory.py --check
    )
    add_test(
      NAME umbra.ieee1516_2025.binding_shell_generation
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_rti_ambassador_shell.py --check
    )
    add_test(
      NAME umbra.ieee1516_2_2025.resources
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/ieee_1516_2_resources.py --check
    )
    if(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY)
      add_test(
        NAME umbra.external_siso_fom_corpus.integrity
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_external_siso_fom_corpus.py
          --root ${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}
      )
    endif()
    if(UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY)
      add_test(
        NAME umbra.external_2025_fom_corpus.integrity
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_external_2025_fom_corpus.py
          --root ${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}
      )
    endif()
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
    if(NOT UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
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
          -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_installed_package_smoke.cmake
      )
    else()
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
        NAME umbra.ieee1516_2025.update_rate_reduction_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/update-rate-reduction-requirements-contract.json
      )
      set_tests_properties(
        umbra.ieee1516_2025.update_rate_reduction_requirements_traceability
        PROPERTIES LABELS "requirements-lab;traceability;update-rate-reduction"
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
  else()
    message(STATUS "Python interpreter not found; run the header and binding checks manually")
  endif()

  # Requirements-Lab contracts are individual CTest tests, so give them the
  # same domain vocabulary as the Catch2 suite.  These labels are test-routing
  # metadata only: a selected lane is fast development feedback, not a
  # conformance-evidence promotion.
  function(umbra_label_registered_ctest_tests)
    cmake_parse_arguments(UMBRA_TEST_LABEL "" "MATCH" "LABELS" ${ARGN})
    if(NOT UMBRA_TEST_LABEL_MATCH OR NOT UMBRA_TEST_LABEL_LABELS)
      message(FATAL_ERROR
        "umbra_label_registered_ctest_tests requires MATCH and LABELS arguments"
      )
    endif()

    get_property(_umbra_registered_ctest_tests DIRECTORY PROPERTY TESTS)
    foreach(_umbra_registered_ctest_test IN LISTS _umbra_registered_ctest_tests)
      if(_umbra_registered_ctest_test MATCHES "${UMBRA_TEST_LABEL_MATCH}")
        set_property(
          TEST "${_umbra_registered_ctest_test}"
          APPEND
          PROPERTY LABELS ${UMBRA_TEST_LABEL_LABELS}
        )
      endif()
    endforeach()
  endfunction()

  umbra_label_registered_ctest_tests(
    MATCH "_traceability$"
    LABELS requirements-lab traceability
  )
  umbra_label_registered_ctest_tests(
    MATCH "_api_traceability$"
    LABELS api-contract
  )
  umbra_label_registered_ctest_tests(
    MATCH "^(umbra\\.ieee1516_2025\\.(headers|binding_shell|integrity|exception_binding|binding_inventory|binding_shell_generation)|umbra\\.kernel\\.federate_lifecycle)$"
    LABELS foundation
  )
  umbra_label_registered_ctest_tests(
    MATCH "(^umbra\\.ieee1516_2_2025\\.|fom_|external_.*fom)"
    LABELS fom
  )
  # Annex C composition checks are a cross-cutting FOM slice rather than a
  # one-service API lane.  Keep the traceability mapping exact and anchored so
  # a neighboring FOM contract cannot silently widen the focused lane.
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2_2025\\.(composition|synchronization_table|synchronization_merge|dimension_merge|transportation_merge|update_rate_merge)_traceability$"
    LABELS annex-c
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2_2025\\.strict_omt_traceability$"
    LABELS omt-complete-model
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.catch2\\.The FDD materializer emits a strict OMT-positive complete model$"
    LABELS omt-complete-model
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.float_time_traceability$"
    LABELS float-time
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.explicit_mim_(requirements|api)_traceability$"
    LABELS explicit-mim
  )
  umbra_label_registered_ctest_tests(
    MATCH "fom_source_diagnostics_requirements_traceability$"
    LABELS fom-source-diagnostics
  )
  umbra_label_registered_ctest_tests(
    MATCH "fom_source_diagnostics_api_traceability$"
    LABELS fom-source-diagnostics
  )
  umbra_label_registered_ctest_tests(
    MATCH "(federation_management|federation_execution|federation_listing|federate_(lookup|lost_report|resigned|advisory)|connection(_lost|_callback)?|resign_action|automatic_resign|mom_service_reporting)"
    LABELS federation-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "(time|lookahead|galt|lits|message_request|asynchronous|timestamped|tso)"
    LABELS time-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_advance_(requirements|api)_traceability$"
    LABELS time-advance-request
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_advance_request_available_(requirements|api)_traceability$"
    LABELS time-advance-request-available
  )
  umbra_label_registered_ctest_tests(
    MATCH "next_message_request_(requirements|api)_traceability$"
    LABELS next-message-request
  )
  umbra_label_registered_ctest_tests(
    MATCH "next_message_request_available_(requirements|api)_traceability$"
    LABELS next-message-request-available
  )
  umbra_label_registered_ctest_tests(
    MATCH "flush_queue_request_(requirements|api)_traceability$"
    LABELS flush-queue-request
  )
  umbra_label_registered_ctest_tests(
    MATCH "default_region_requirements_traceability$"
    LABELS timestamped-default-region-attribute-update
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-default-region-attribute-update
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-update
  )
  umbra_label_registered_ctest_tests(
    MATCH "default_region_requirements_traceability$"
    LABELS timestamped-default-region-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-default-region-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS timestamped-default-region-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "attribute_value_update_with_regions_(requirements|api)_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_advance_(requirements|api)_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "convey_region_designator_callback_requirements_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_api_traceability$"
    LABELS timestamped-regional-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_class_attribute_value_update_request_(requirements|api)_traceability$"
    LABELS object-class-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_(requirements|api)_traceability$"
    LABELS object-class-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS object-class-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_advance_(requirements|api)_traceability$"
    LABELS object-class-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS object-class-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_interaction_(requirements|api)_traceability$"
    LABELS timestamped-regional-interaction-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS timestamped-regional-interaction-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "convey_region_designator_callback_requirements_traceability$"
    LABELS timestamped-regional-interaction-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "default_region_requirements_traceability$"
    LABELS timestamped-default-region-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_interaction_(requirements|api)_traceability$"
    LABELS timestamped-default-region-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS timestamped-default-region-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "(flush_queue_request|time_advance_request_available|next_message_request_available|time_advance)_(requirements|api)_traceability$"
    LABELS timestamped-default-region-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "request_retraction_(requirements|api)_traceability$"
    LABELS retract
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS change-attribute-order-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS change-default-attribute-order-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS change-interaction-order-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.transportation_type_change_(requirements|api)_traceability$"
    LABELS request-attribute-transportation-type-change
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.transportation_type_change_(requirements|api)_traceability$"
    LABELS change-default-attribute-transportation-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.transportation_type_change_(requirements|api)_traceability$"
    LABELS query-attribute-transportation-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.(transportation_type_change_(requirements|api)|interaction_transportation_type_change_requirements)_traceability$"
    LABELS request-interaction-transportation-type-change
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.(transportation_type_change_api|interaction_transportation_type_change_requirements)_traceability$"
    LABELS query-interaction-transportation-type
  )
  umbra_label_registered_ctest_tests(
    MATCH "transportation_type_api_traceability$"
    LABELS transportation-type-lookup
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.(Embedded transportation type lookup (exposes the mandatory 2025 support pair|resolves a declared FOM transportation per execution)|Embedded (ordinary( regional)?|timestamped( regional)?|directed|timestamped directed)( attribute)? delivery accepts a declared custom FOM transportation)$"
    LABELS transportation-type-lookup
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.attribute_ownership_query_(requirements|api)_traceability$"
    LABELS query-attribute-ownership
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unconditional_attribute_ownership_divestiture_(requirements|api)_traceability$"
    LABELS unconditional-attribute-ownership-divestiture
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.ownership_assumption_research_(requirements|api)_traceability$"
    LABELS ownership-assumption-research
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.negotiated_willing_to_acquire_(requirements|api)_traceability$"
    LABELS negotiated-willing-to-acquire
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.local_delete_object_instance_(requirements|api)_traceability$"
    LABELS local-delete-object-instance
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.local_delete_object_instance_(requirements|api)_traceability$"
    LABELS local-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed local-delete object-instance invocations$"
    LABELS local-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed local-delete object-instance invocations through MOM interaction$"
    LABELS local-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.delete_object_instance_(requirements|api)_traceability$"
    LABELS delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed receive-order Delete Object Instance invocations$"
    LABELS delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction$"
    LABELS delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS local-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.publish_object_class_attributes_(requirements|api)_traceability$"
    LABELS publish-object-class-attributes
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.publish_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS publish-object-class-directed-interactions
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unpublish_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS unpublish-object-class-directed-interactions
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.subscribe_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS subscribe-object-class-directed-interactions
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unsubscribe_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS unsubscribe-object-class-directed-interactions
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.timestamped_directed_interaction_(requirements|api)_traceability$"
    LABELS timestamped-directed-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.timestamped_directed_interaction_(requirements|api)_traceability$"
    LABELS timestamped-directed-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped Send Directed Interaction invocations$"
    LABELS timestamped-directed-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Send Directed Interaction invocations through MOM interaction$"
    LABELS timestamped-directed-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-directed-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.timestamped_directed_interaction_(requirements|api)_traceability$"
    LABELS timestamped-directed-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records timestamped Send Directed Interaction before directed callback$"
    LABELS timestamped-directed-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-directed-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unpublish_object_class_attributes_(requirements|api)_traceability$"
    LABELS unpublish-object-class-attributes
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.subscribe_object_class_attributes_(requirements|api)_traceability$"
    LABELS subscribe-object-class-attributes
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unsubscribe_object_class_attributes_(requirements|api)_traceability$"
    LABELS unsubscribe-object-class-attributes
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.subscribe_interaction_class_(requirements|api)_traceability$"
    LABELS subscribe-interaction-class
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unsubscribe_interaction_class_(requirements|api)_traceability$"
    LABELS unsubscribe-interaction-class
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.publish_interaction_class_(requirements|api)_traceability$"
    LABELS publish-interaction-class
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.unpublish_interaction_class_(requirements|api)_traceability$"
    LABELS unpublish-interaction-class
  )
  umbra_label_registered_ctest_tests(
    MATCH "declaration_relevance_advisory_(requirements|api)_traceability$"
    LABELS declaration-relevance-advisory-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS declaration-relevance-advisory-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_registration_(requirements|api)_traceability$"
    LABELS discover-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS discover-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_deletion_(requirements|api)_traceability$"
    LABELS remove-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_deletion_(requirements|api)_traceability$"
    LABELS delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS remove-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_name_reservation_(requirements|api)_traceability$"
    LABELS object-instance-name-reservation-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_name_reservation_(requirements|api)_traceability$"
    LABELS object-name-reservation-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS object-name-reservation-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS object-instance-name-reservation-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS regional-object-attribute-association-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS regional-object-attribute-association-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS regional-object-attribute-subscription-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS regional-object-attribute-subscription-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "interaction_region_(requirements|api)_traceability$"
    LABELS regional-interaction-subscription-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS regional-interaction-subscription-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional interaction subscriptions through MOM interaction$"
    LABELS regional-interaction-subscription-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "interaction_region_(requirements|api)_traceability$"
    LABELS ordinary-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback$"
    LABELS ordinary-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction$"
    LABELS ordinary-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ordinary-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "interaction_region_(requirements|api)_traceability$"
    LABELS ordinary-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed regional Send Interaction With Regions invocations$"
    LABELS ordinary-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction$"
    LABELS ordinary-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ordinary-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_interaction_(requirements|api)_traceability$"
    LABELS timestamped-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records timestamped Send Interaction With Regions before interaction callback$"
    LABELS timestamped-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted timestamped regional Send Interaction With Regions through MOM interaction$"
    LABELS timestamped-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_interaction_(requirements|api)_traceability$"
    LABELS timestamped-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped regional Send Interaction With Regions invocations$"
    LABELS timestamped-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped regional Send Interaction With Regions invocations through MOM interaction$"
    LABELS timestamped-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_object_deletion_(requirements|api)_traceability$"
    LABELS timestamped-remove-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_object_deletion_(requirements|api)_traceability$"
    LABELS timestamped-delete-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records timestamped Delete Object Instance before removal callback$"
    LABELS timestamped-delete-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_object_deletion_(requirements|api)_traceability$"
    LABELS timestamped-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped Delete Object Instance invocations$"
    LABELS timestamped-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Delete Object Instance invocations through MOM interaction$"
    LABELS timestamped-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-remove-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-delete-object-instance-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-delete-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.attribute_value_update_request_(requirements|api)_traceability$"
    LABELS provide-attribute-value-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS provide-attribute-value-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.auto_provide_(requirements|api)_traceability$"
    LABELS auto-provide-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS auto-provide-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.object_class_attribute_value_update_request_(requirements|api)_traceability$"
    LABELS provide-attribute-value-update-class-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\.ieee1516_2025\.catch2\.Embedded service reporting records timestamped Update Attribute Values before reflection callback$"
    LABELS timestamped-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped Update Attribute Values invocations$"
    LABELS timestamped-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Update Attribute Values invocations through MOM interaction$"
    LABELS timestamped-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS provide-attribute-value-update-class-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records timestamped regional Update Attribute Values before reflection callback$"
    LABELS timestamped-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted timestamped regional Update Attribute Values through MOM interaction$"
    LABELS timestamped-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "regional_attribute_update_service_report_(requirements|api)_traceability$"
    LABELS ordinary-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records regional Update Attribute Values before reflection callback$"
    LABELS ordinary-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional Update Attribute Values through MOM interaction$"
    LABELS ordinary-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ordinary-regional-attribute-update-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "regional_attribute_update_service_report_(requirements|api)_traceability$"
    LABELS ordinary-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed regional Update Attribute Values invocations$"
    LABELS ordinary-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed ordinary regional Update Attribute Values invocations through MOM interaction$"
    LABELS ordinary-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ordinary-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "regional_attribute_update_service_report_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped regional Update Attribute Values invocations$"
    LABELS timestamped-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped regional Update Attribute Values invocations through MOM interaction$"
    LABELS timestamped-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-regional-attribute-update-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_interaction_(requirements|api)_traceability$"
    LABELS timestamped-send-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed timestamped Send Interaction invocations$"
    LABELS timestamped-send-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed timestamped Send Interaction invocations through MOM interaction$"
    LABELS timestamped-send-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-send-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_interaction_(requirements|api)_traceability$"
    LABELS timestamped-send-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records timestamped Send Interaction before interaction callback$"
    LABELS timestamped-send-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS timestamped-send-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.attribute_value_update_with_regions_(requirements|api)_traceability$"
    LABELS provide-attribute-value-update-regional-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS provide-attribute-value-update-regional-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.attribute_value_update_with_regions_(requirements|api)_traceability$"
    LABELS regional-attribute-value-update-request-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS regional-attribute-value-update-request-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS commit-region-modifications-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS commit-region-modifications-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS delete-region-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS delete-region-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS set-range-bounds-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS set-range-bounds-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS ddm-nonvoid-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ddm-nonvoid-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS ddm-mutation-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ddm-mutation-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "interaction_region_(requirements|api)_traceability$"
    LABELS ddm-regional-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ddm-regional-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS ddm-regional-object-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ddm-regional-object-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS ddm-regional-association-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS ddm-regional-association-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(publish|unpublish)_interaction_class_(requirements|api)_traceability$"
    LABELS declaration-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS declaration-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(subscribe|unsubscribe)_interaction_class_(requirements|api)_traceability$"
    LABELS declaration-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS declaration-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(publish|unpublish)_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS directed-declaration-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS directed-declaration-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(subscribe|unsubscribe)_object_class_directed_interactions_(requirements|api)_traceability$"
    LABELS directed-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS directed-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(publish|unpublish)_object_class_attributes_(requirements|api)_traceability$"
    LABELS object-attribute-declaration-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS object-attribute-declaration-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(subscribe|unsubscribe)_object_class_attributes_(requirements|api)_traceability$"
    LABELS object-attribute-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS object-attribute-subscription-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "(time_role|modify_lookahead)"
    LABELS time-role
  )
  umbra_label_registered_ctest_tests(
    MATCH "modify_lookahead"
    LABELS modify-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "asynchronous_delivery"
    LABELS asynchronous-delivery
  )
  # The regional asynchronous-delivery companion selects both the switch
  # contracts and the regional interaction contracts. Keep those traceability
  # checks in the focused lane as well as the broader regional service lane.
  umbra_label_registered_ctest_tests(
    MATCH "interaction_region_(requirements|api)_traceability$"
    LABELS asynchronous-delivery
  )
  umbra_label_registered_ctest_tests(
    MATCH "convey_region_designator_callback_requirements_traceability$"
    LABELS asynchronous-delivery
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_api_traceability$"
    LABELS asynchronous-delivery
  )
  umbra_label_registered_ctest_tests(
    MATCH "synchronization_point_(requirements|api)_traceability$"
    LABELS
      register-federation-synchronization-point
      synchronization-point-achieved
  )
  umbra_label_registered_ctest_tests(
    MATCH "synchronization_point_(requirements|api)_traceability$"
    LABELS confirm-synchronization-point-registration-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "synchronization_point_(requirements|api)_traceability$"
    LABELS announce-synchronization-point-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "synchronization_point_(requirements|api)_traceability$"
    LABELS federation-synchronized-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "save_control_(requirements|api)_traceability$"
    LABELS initiate-federate-save-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS initiate-federate-save-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "save_control_(requirements|api)_traceability$"
    LABELS federation-saved-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS federation-saved-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "save_control_(requirements|api)_traceability$"
    LABELS federation-save-status-response-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS federation-save-status-response-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "restore_control_(requirements|api)_traceability$"
    LABELS confirm-federation-restoration-request-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS confirm-federation-restoration-request-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "restore_control_(requirements|api)_traceability$"
    LABELS federation-restore-begun-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS federation-restore-begun-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "restore_control_(requirements|api)_traceability$"
    LABELS initiate-federate-restore-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS initiate-federate-restore-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "restore_control_(requirements|api)_traceability$"
    LABELS federation-restore-status-response-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS federation-restore-status-response-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "regional_multi_attribute_ddm_(requirements|api)_traceability$"
    LABELS ddm-regional-multi-attribute
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded two-dimensional regional object updates filter independent attribute sources$"
    LABELS ddm-regional-multi-attribute
  )
  umbra_label_registered_ctest_tests(
    MATCH "(ddm|dimension|region|convey_region|range_bounds)"
    LABELS ddm
  )
  umbra_label_registered_ctest_tests(
    MATCH "(mom|service_report|federate_lost_report)"
    LABELS mom
  )
  umbra_label_registered_ctest_tests(
    MATCH "(mom_service_reporting|service_report|report_service)"
    LABELS service-reporting
  )
  umbra_label_registered_ctest_tests(
    MATCH "(mom_service_reporting|service_report_file)"
    LABELS service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "service_report_file_lifecycle_(requirements|api)_traceability$"
    LABELS service-report-file-lifecycle
  )
  umbra_label_registered_ctest_tests(
    MATCH "(support_switches_(table_requirements|service_requirements|api)|declaration_relevance_advisory_api|attribute_relevance_advisory_api|object_attribute_scope_api)_traceability$"
    LABELS support-switches
  )
  umbra_label_registered_ctest_tests(
    MATCH "delay_subscription_evaluation_requirements_traceability$"
    LABELS delay-subscription-evaluation
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_api_traceability$"
    LABELS delay-subscription-evaluation
  )
  umbra_label_registered_ctest_tests(
    MATCH "(object_(class|instance)|attribute_(value_update|scope)|whole_object|local_delete)"
    LABELS object-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "(ownership|resign_action)"
    LABELS ownership-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "(declaration|publish|subscribe|advisory|relevance)"
    LABELS declaration-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "(interaction|parameter)"
    LABELS interaction-management
  )
  umbra_label_registered_ctest_tests(
    MATCH "(save|restore)"
    LABELS save-restore
  )
  umbra_label_registered_ctest_tests(
    MATCH "save_control_(requirements|api)_traceability$"
    LABELS federate-save-begun federate-save-complete abort-federation-save
           query-federation-save-status request-federation-save
  )
  umbra_label_registered_ctest_tests(
    MATCH "restore_control_(requirements|api)_traceability$"
    LABELS
      request-federation-restore
      confirm-federation-restoration-request-service-report
      federation-restore-begun-service-report
      initiate-federate-restore-service-report
      federate-restore-complete
      abort-federation-restore
      query-federation-restore-status
      federation-restore-status-response-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "resign_action_(requirements|api)_traceability$"
    LABELS resign-federation-execution
  )
  umbra_label_registered_ctest_tests(
    MATCH "federate_resigned_(requirements|api)_traceability$"
    LABELS federate-resigned-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-automatic-resign
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-final-federate
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-error-path
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-negotiated-cancellation
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-automatic-resign
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-final-federate
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-negotiated-cancellation
  )
  umbra_label_registered_ctest_tests(
    MATCH "resign_action_(requirements|api)_traceability$"
    LABELS connection-lost-final-federate
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_instance_name_reservation_(requirements|api)_traceability$"
    LABELS connection-lost-final-federate
  )
  umbra_label_registered_ctest_tests(
    MATCH "attribute_ownership_acquisition_(requirements|api)_traceability$"
    LABELS connection-lost-negotiated-cancellation
  )
  umbra_label_registered_ctest_tests(
    MATCH "negotiated_attribute_ownership_divestiture_(requirements|api)_traceability$"
    LABELS connection-lost-negotiated-cancellation
  )
  umbra_label_registered_ctest_tests(
    MATCH "timed_save_(requirements|api)_traceability$"
    LABELS timed-save
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-report-ordering
  )
  umbra_label_registered_ctest_tests(
    MATCH "federate_lost_report_(requirements|api)_traceability$"
    LABELS connection-lost-report-ordering
  )
  umbra_label_registered_ctest_tests(
    MATCH "resign_action_requirements_traceability$"
    LABELS connection-lost-report-ordering
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-report-ordering
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-directed-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_directed_interaction_(requirements|api)_traceability$"
    LABELS connection-lost-directed-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-directed-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "attribute_ownership_check_(requirements|api)_traceability$"
    LABELS connection-lost-directed-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "connection_lost_(requirements|api)_traceability$"
    LABELS connection-lost-regional-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_(requirements|api)_traceability$"
    LABELS connection-lost-regional-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "object_attribute_region_(requirements|api)_traceability$"
    LABELS connection-lost-regional-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS connection-lost-regional-selector-mutation
  )
  umbra_label_registered_ctest_tests(
    MATCH "support_switches_(service_requirements|api)_traceability$"
    LABELS connection-lost-regional-selector-mutation
  )

  find_package(Catch2 3 CONFIG QUIET)
  if(NOT Catch2_FOUND AND UMBRA_FETCH_CATCH2)
    include(FetchContent)
    set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(UMBRA_CATCH2_GIT_CONFIG)
    if(WIN32)
      # Prefer the Windows trust store when Git's bundled CA bundle is stale.
      set(UMBRA_CATCH2_GIT_CONFIG GIT_CONFIG http.sslBackend=schannel)
    endif()
    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      # Catch2 v3.15.3; resolve the release tag to this immutable commit.
      GIT_TAG 8b08d4d79514f45f7e4ce2a607ac9c94e920d1bb
      GIT_SHALLOW TRUE
      ${UMBRA_CATCH2_GIT_CONFIG}
    )
    FetchContent_MakeAvailable(Catch2)
  endif()

  if(TARGET Catch2::Catch2WithMain)
    add_executable(umbra_ieee1516_2025_catch2
      cpp/tests/ieee1516_2025_headers_catch2.cpp
      cpp/tests/ieee1516_2025_authorization_catch2.cpp
      cpp/tests/ieee1516_2025_basic_data_elements_catch2.cpp
      cpp/tests/mom_service_report_encoding_catch2.cpp
      cpp/tests/service_report_store_catch2.cpp
      cpp/tests/runtime_instrumentation_catch2.cpp
      cpp/tests/attribute_handle_catch2.cpp
      cpp/tests/dimension_handle_catch2.cpp
      cpp/tests/message_retraction_handle_catch2.cpp
      cpp/tests/region_handle_catch2.cpp
      cpp/tests/callback_dispatcher_catch2.cpp
      cpp/tests/federate_lifecycle_catch2.cpp
      cpp/tests/federate_time_state_catch2.cpp
      cpp/tests/federation_time_bounds_catch2.cpp
      cpp/tests/federation_time_coordinator_catch2.cpp
      cpp/tests/federation_time_grant_policy_catch2.cpp
      cpp/tests/tso_message_queue_catch2.cpp
      cpp/tests/federate_handle_catch2.cpp
      cpp/tests/interaction_class_handle_catch2.cpp
      cpp/tests/object_class_handle_catch2.cpp
      cpp/tests/object_instance_handle_catch2.cpp
      cpp/tests/parameter_handle_catch2.cpp
      cpp/tests/transportation_type_handle_catch2.cpp
      cpp/tests/utf8_string_catch2.cpp
      cpp/tests/variable_length_data_catch2.cpp
      cpp/tests/federation_registry_catch2.cpp
      cpp/tests/ieee1516_2025_connection_catch2.cpp
      cpp/tests/ieee1516_2025_time_catch2.cpp
    )
    if(UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
      target_sources(umbra_ieee1516_2025_catch2 PRIVATE
        cpp/tests/libxml2_fom_composer_catch2.cpp
        cpp/tests/libxml2_fom_validator_catch2.cpp
      )
      if(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/external_siso_fom_catch2.cpp
        )
        target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
          UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY="${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}"
        )
      endif()
      if(UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/external_2025_fom_catch2.cpp
        )
        target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
          UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY="${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}"
        )
      endif()
      if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/ieee1516_2025_federation_management_catch2.cpp
        )
      endif()
      target_link_libraries(umbra_ieee1516_2025_catch2 PRIVATE umbra_fom_validation_backend)
      target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
    endif()
    target_compile_features(umbra_ieee1516_2025_catch2 PRIVATE cxx_std_20)
    if(MSVC)
      # The focused federation-management Catch2 translation unit intentionally
      # keeps the C++ service matrix in one source file.  MSVC's default object
      # section limit is too small for that standards-traceability lane.
      target_compile_options(umbra_ieee1516_2025_catch2 PRIVATE /bigobj)
    endif()
    target_include_directories(umbra_ieee1516_2025_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_link_libraries(umbra_ieee1516_2025_catch2 PRIVATE
      umbra::rti
      umbra::fedtime
      umbra::authorizer
      Catch2::Catch2WithMain
    )
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
    endif()
    # Register every Catch2 case independently, and expose its existing tags
    # as CTest labels.  This makes the tag taxonomy usable through both
    # `ctest -L` and the named development targets below without duplicating a
    # second, hand-maintained list of C++ tests.
    include(Catch)
    catch_discover_tests(umbra_ieee1516_2025_catch2
      TEST_PREFIX "umbra.ieee1516_2025.catch2."
      ADD_TAGS_AS_LABELS
      # Keep CTest invocation read-only with respect to the discovery manifest.
      # PRE_TEST discovery lets concurrent focused lanes race while they both
      # try to generate the same include file. POST_BUILD generates it once
      # when the Catch2 executable changes, before any lane runs.
      DISCOVERY_MODE POST_BUILD
    )

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
      add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_CTEST_COMMAND}
          --test-dir "${CMAKE_BINARY_DIR}"
          ${_umbra_ctest_configuration_arguments}
          --output-on-failure
          -L "${LABEL_REGEX}"
        DEPENDS umbra_ieee1516_2025_catch2
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
    umbra_add_ctest_lane(umbra_test_rapid "^(foundation|unit)$")
    umbra_add_ctest_lane(umbra_test_foundation "^foundation$")
    umbra_add_ctest_lane(umbra_test_federation_management "^federation-management$")
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
    umbra_add_ctest_service_lane(
      umbra_test_connection_lost_directed_selector_mutation
      connection-lost-directed-selector-mutation
    )
    umbra_add_ctest_service_lane(
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
    umbra_add_ctest_service_lane(
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
    umbra_add_ctest_service_lane(
      umbra_test_ordinary_regional_interaction_service_report
      ordinary-regional-interaction-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_ordinary_regional_interaction_failure
      ordinary-regional-interaction-failure
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_remove_object_instance_service_report
      timestamped-remove-object-instance-service-report
    )
    umbra_add_ctest_service_lane(
      umbra_test_timestamped_delete_object_instance_service_report
      timestamped-delete-object-instance-service-report
    )
    umbra_add_ctest_service_lane(
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
      umbra_test_timestamped_regional_interaction_service_report
      timestamped-regional-interaction-service-report
    )
    umbra_add_ctest_service_lane(
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
    umbra_add_ctest_service_lane(
      umbra_test_service_report_file_lifecycle
      service-report-file-lifecycle
    )
    umbra_add_ctest_lane(umbra_test_support_switches "^support-switches$")
    umbra_add_ctest_lane(umbra_test_fom "^fom$")
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
    # A cross-cutting callback-model lane keeps direct-dispatch regressions
    # fast without forcing a whole domain lane.
    umbra_add_ctest_lane(umbra_test_callback_immediate "^callback-immediate$")

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
  else()
    message(STATUS "Catch2 v3 not found; configure with -DUMBRA_FETCH_CATCH2=ON or provide Catch2::Catch2WithMain")
  endif()
endif()

if(NOT UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
install(TARGETS ieee1516_2025_headers
  EXPORT umbra_rti_targets
)
install(TARGETS umbra_rti umbra_fedtime umbra_authorizer
  EXPORT umbra_rti_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY "${UMBRA_IEEE1516_2025_INCLUDE_DIR}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES third_party/ieee1516.1-2025/NOTICE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/umbra_rti
)
install(DIRECTORY "${UMBRA_IEEE1516_2_2025_RESOURCE_DIR}/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/umbra_rti/ieee1516.2-2025
)
install(FILES
  third_party/ieee1516.2-2025/NOTICE
  third_party/ieee1516.2-2025/manifest.json
  third_party/ieee1516.2-2025/resource-digests.json
  DESTINATION ${CMAKE_INSTALL_DATADIR}/umbra_rti/ieee1516.2-2025
)
install(EXPORT umbra_rti_targets
  FILE umbra_rti_targets.cmake
  NAMESPACE umbra::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umbra_rti
)

configure_package_config_file(
  cmake/umbra_rti-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umbra_rti
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-config-version.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/umbra_rti
)
endif()
