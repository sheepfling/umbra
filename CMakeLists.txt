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
  cpp/src/ieee1516_2025_binding_shell.cpp
  cpp/src/ieee1516_2025_dimension_handle.cpp
  cpp/src/ieee1516_2025_federate_handle.cpp
  cpp/src/ieee1516_2025_interaction_class_handle.cpp
  cpp/src/ieee1516_2025_message_retraction_handle.cpp
  cpp/src/ieee1516_2025_object_class_handle.cpp
  cpp/src/ieee1516_2025_object_instance_handle.cpp
  cpp/src/ieee1516_2025_parameter_handle.cpp
  cpp/src/ieee1516_2025_range_bounds.cpp
  cpp/src/ieee1516_2025_region_handle.cpp
  cpp/src/ieee1516_2025_support.cpp
  cpp/src/ieee1516_2025_time.cpp
  cpp/src/ieee1516_2025_transportation_type_handle.cpp
  cpp/src/internal/callback_dispatcher.cpp
  cpp/src/internal/callback_session.cpp
  cpp/src/internal/embedded_transport.cpp
  cpp/src/internal/attribute_handle_directory.cpp
  cpp/src/internal/dimension_handle_directory.cpp
  cpp/src/internal/federate_lifecycle.cpp
  cpp/src/internal/federate_time_state.cpp
  cpp/src/internal/federation_management_coordinator.cpp
  cpp/src/internal/federation_registry.cpp
  cpp/src/internal/interaction_class_handle_directory.cpp
  cpp/src/internal/object_class_handle_directory.cpp
  cpp/src/internal/parameter_handle_directory.cpp
  cpp/src/internal/federation_time_bounds.cpp
  cpp/src/internal/federation_time_coordinator.cpp
  cpp/src/internal/federation_time_grant_policy.cpp
  cpp/src/internal/reference_time_selection.cpp
  cpp/src/internal/tso_message_queue.cpp
  cpp/src/internal/umbra_rti_ambassador.cpp
  cpp/src/internal/utf8_string.cpp
)
add_library(umbra::rti ALIAS umbra_rti)
set_target_properties(umbra_rti PROPERTIES EXPORT_NAME rti)
target_compile_features(umbra_rti PUBLIC cxx_std_20)
target_compile_definitions(umbra_rti PUBLIC STATIC_RTI)
target_include_directories(umbra_rti PRIVATE
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
    cpp/src/internal/libxml2_fom_composer.cpp
    cpp/src/internal/libxml2_fom_document.cpp
    cpp/src/internal/libxml2_fom_validator.cpp
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
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-composition-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.data_representation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-data-representation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.table_constraints_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-table-constraints-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.array_cardinality_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-array-cardinality-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.dimension_default_value_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-dimension-default-value-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2_2025.time_management_switches_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-time-management-switches-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.reference_time_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/reference-time-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_management_preparation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-management-preparation-requirements-contract.json
    )
    if(NOT UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_test(
        NAME umbra.package_config
        COMMAND ${CMAKE_COMMAND}
          "-DUMBRA_CMAKE_COMMAND=${CMAKE_COMMAND}"
          "-DUMBRA_CTEST_COMMAND=${CMAKE_CTEST_COMMAND}"
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
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-management-embedded-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.resign_action_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/resign-action-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.resign_action_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/resign-action-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_lost_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/connection-lost-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.connection_lost_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/connection-lost-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_resigned_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federate-resigned-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_resigned_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federate-resigned-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.get_time_factory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/get-time-factory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lookup_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federate-lookup-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federate-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-class-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_class_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/interaction-class-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.parameter_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/parameter-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_declaration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/interaction-declaration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/transportation-type-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/order-type-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/order-type-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_control_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/order-type-control-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.order_type_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/order-type-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_change_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/transportation-type-change-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.transportation_type_change_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/transportation-type-change-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_transportation_type_change_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/interaction-transportation-type-change-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.dimension_lookup_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/dimension-lookup-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.dimension_lookup_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/dimension-lookup-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.region_lifecycle_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/region-lifecycle-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.region_lifecycle_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/region-lifecycle-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/interaction-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.interaction_region_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/interaction-region-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-attribute-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.default_region_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/default-region-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_region_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-attribute-region-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_interaction_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/receive-order-interaction-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_interaction_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/receive-order-interaction-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.directed_interaction_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/directed-interaction-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.directed_interaction_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/directed-interaction-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_declaration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-class-attribute-declaration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_declaration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-class-attribute-declaration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.declaration_relevance_advisory_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/declaration-relevance-advisory-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.declaration_relevance_advisory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/declaration-relevance-advisory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_relevance_advisory_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-relevance-advisory-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_relevance_advisory_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-relevance-advisory-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_value_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/update-rate-value-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_value_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/update-rate-value-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.update_rate_subscription_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/update-rate-subscription-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.fom_advisory_switches_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/fom-advisory-switches-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federate_advisory_switch_initialization_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federate-advisory-switch-initialization-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.advisories_use_known_class_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/advisories-use-known-class-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.advisories_use_known_class_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/advisories-use-known-class-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.non_regulated_grant_switch_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/non-regulated-grant-switch-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.non_regulated_grant_switch_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/non-regulated-grant-switch-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2_2025.support_switches_table_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/support-switches-table-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.support_switches_service_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/support-switches-service-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.delay_subscription_evaluation_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/delay-subscription-evaluation-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.support_switches_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/support-switches-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.handle_normalization_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/handle-normalization-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.handle_normalization_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/handle-normalization-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.convey_region_designator_callback_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/convey-region-designator-callback-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.auto_provide_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/auto-provide-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.auto_provide_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/auto-provide-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_registration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-registration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_registration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-registration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_name_reservation_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-name-reservation-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_name_reservation_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-name-reservation-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_named_registration_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-named-registration-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_named_registration_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-named-registration-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_scope_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-attribute-scope-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_attribute_scope_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-attribute-scope-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_deletion_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-deletion-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_instance_deletion_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-instance-deletion-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.local_delete_object_instance_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/local-delete-object-instance-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.local_delete_object_instance_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/local-delete-object-instance-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_attribute_update_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/receive-order-attribute-update-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.receive_order_attribute_update_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/receive-order-attribute-update-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_request_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-request-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_request_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-request-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_response_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-response-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_response_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-response-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_value_update_request_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-class-attribute-value-update-request-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.object_class_attribute_value_update_request_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/object-class-attribute-value-update-request-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_with_regions_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-with-regions-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_value_update_with_regions_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-value-update-with-regions-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_query_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-query-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_query_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-query-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_check_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-check-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_check_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-check-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_if_available_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-acquisition-if-available-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_if_available_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-acquisition-if-available-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-acquisition-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_acquisition_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-acquisition-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_divestiture_if_wanted_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-divestiture-if-wanted-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.attribute_ownership_divestiture_if_wanted_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/attribute-ownership-divestiture-if-wanted-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unconditional_attribute_ownership_divestiture_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/unconditional-attribute-ownership-divestiture-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.unconditional_attribute_ownership_divestiture_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/unconditional-attribute-ownership-divestiture-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_attribute_ownership_divestiture_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/negotiated-attribute-ownership-divestiture-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.negotiated_attribute_ownership_divestiture_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/negotiated-attribute-ownership-divestiture-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federation_listing_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-listing-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.federation_listing_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-listing-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.time_advance_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-advance-requirements-contract.json
      )
      add_test(
      NAME umbra.ieee1516_2025.time_advance_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-advance-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_role_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-role-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_role_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-role-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.modify_lookahead_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/modify-lookahead-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.modify_lookahead_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/modify-lookahead-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/next-message-request-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/next-message-request-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_advance_request_available_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-advance-request-available-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_advance_request_available_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-advance-request-available-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_available_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/next-message-request-available-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.next_message_request_available_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/next-message-request-available-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.flush_queue_request_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/flush-queue-request-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.flush_queue_request_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/flush-queue-request-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.asynchronous_delivery_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/asynchronous-delivery-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.asynchronous_delivery_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/asynchronous-delivery-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.synchronization_point_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/synchronization-point-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.synchronization_point_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/synchronization-point-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.save_control_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/save-control-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timed_save_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timed-save-requirements-contract.json
    )
      add_test(
        NAME umbra.ieee1516_2025.save_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/save-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.timed_save_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timed-save-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.restore_control_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/restore-control-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.restore_control_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/restore-control-api-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.save_restore_interlock_requirements_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/save-restore-interlock-requirements-contract.json
      )
      add_test(
        NAME umbra.ieee1516_2025.save_restore_interlock_api_traceability
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
          --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/save-restore-interlock-api-contract.json
      )
    add_test(
      NAME umbra.ieee1516_2025.whole_object_class_declaration_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/whole-object-class-declaration-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.whole_object_class_declaration_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/whole-object-class-declaration-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_time_coordination_foundation_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-time-coordination-foundation-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_bounds_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-bounds-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_bounds_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-bounds-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_grant_policy_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-grant-policy-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.time_grant_scheduler_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/time-grant-scheduler-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.tso_message_queue_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/tso-message-queue-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.federation_time_coordination_tso_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/federation-time-coordination-tso-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.request_retraction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/request-retraction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.request_retraction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/request-retraction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_attribute_update_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-attribute-update-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-regional-attribute-update-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_attribute_update_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-regional-attribute-update-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.ownership_transfer_update_region_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/ownership-transfer-update-region-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.ownership_transfer_update_region_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/ownership-transfer-update-region-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-object-deletion-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_object_deletion_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-object-deletion-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-directed-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_directed_interaction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-directed-interaction-api-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_requirements_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-regional-interaction-requirements-contract.json
    )
    add_test(
      NAME umbra.ieee1516_2025.timestamped_regional_interaction_api_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check
        --contract ${CMAKE_CURRENT_SOURCE_DIR}/compliance/timestamped-regional-interaction-api-contract.json
    )
  endif()
  else()
    message(STATUS "Python interpreter not found; run the header and binding checks manually")
  endif()

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
    target_include_directories(umbra_ieee1516_2025_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_link_libraries(umbra_ieee1516_2025_catch2 PRIVATE
      umbra::rti
      umbra::fedtime
      Catch2::Catch2WithMain
    )
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
    endif()
    add_test(
      NAME umbra.ieee1516_2025.catch2
      COMMAND umbra_ieee1516_2025_catch2
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
install(TARGETS umbra_rti umbra_fedtime
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
