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
  "Enable the embedded federation-management profile and its packaged FOM resources"
  OFF
)
set(
  UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY
  ""
  CACHE PATH
  "Optional unvendored SISO FOM corpus root for development-only 2010 loading and 2025 boundary tests"
)
set(
  UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY
  ""
  CACHE PATH
  "Optional unvendored 2025 FOM corpus root for development-only positive validation tests"
)
set(
  UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
  ""
  CACHE PATH
  "Optional unvendored IEEE 1516.2-2010 schema/MIM resource root for compatibility-mode tests"
)
set(
  UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH
  ""
  CACHE FILEPATH
  "Optional reviewed third-party IEEE 1516.2-2010 Target Radar FOM module for registration-only tests"
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
if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
  if(NOT UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
    )
  endif()
  get_filename_component(
    UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY
    "${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(NOT IS_DIRECTORY "${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}")
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY is not a directory: ${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
    )
  endif()
endif()
if(UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH)
  if(NOT UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH requires UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY."
    )
  endif()
  if(NOT UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
    )
  endif()
  get_filename_component(
    UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH
    "${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(NOT EXISTS "${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}" OR
     IS_DIRECTORY "${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}")
    message(FATAL_ERROR
      "UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH is not a file: ${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}"
    )
  endif()
endif()
set(UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY "${CMAKE_BINARY_DIR}/compliance"
  CACHE PATH "Directory for generated Catch2 JUnit artifacts"
)

# Keep the selected build profile discoverable after installation.  This is a
# deployment manifest, not a second public API: the official C++ binding still
# carries the runtime configuration (including RtiConfiguration::rtiAddress),
# while the manifest records which optional profile was actually built and
# where its reviewed 1516.2 resources live.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  set(UMBRA_PROFILE_NAME "embedded-federation-management")
  set(UMBRA_PROFILE_PROCESS_ENDPOINT "rtiAddress=tcp://host:port")
else()
  set(UMBRA_PROFILE_NAME "binding-foundation")
  set(UMBRA_PROFILE_PROCESS_ENDPOINT "not-enabled")
endif()
configure_file(
  cmake/umbra_rti-profile.json.in
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-profile.json"
  @ONLY
)

set(UMBRA_IEEE1516_2025_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ieee1516.1-2025/include"
)
if(NOT EXISTS "${UMBRA_IEEE1516_2025_INCLUDE_DIR}/RTI/RTI1516.h")
  message(FATAL_ERROR "The vendored IEEE 1516.1-2025 C++ headers are missing")
endif()

set(UMBRA_IEEE1516_2010_INCLUDE_DIR
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ieee1516.1-2010/include"
)
if(NOT EXISTS "${UMBRA_IEEE1516_2010_INCLUDE_DIR}/RTI/RTI1516.h")
  message(FATAL_ERROR "The vendored IEEE 1516.1-2010 C++ headers are missing")
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

add_library(ieee1516_2010_headers INTERFACE)
add_library(umbra::ieee1516_2010_headers ALIAS ieee1516_2010_headers)
target_include_directories(ieee1516_2010_headers SYSTEM INTERFACE
  $<BUILD_INTERFACE:${UMBRA_IEEE1516_2010_INCLUDE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/umbra/ieee1516.1-2010>
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
  cpp/src/internal/federation/process_transport.cpp
  cpp/src/internal/federation/process_federation_service.cpp
  cpp/src/internal/federation/process_federation_callback_bridge.cpp
  cpp/src/internal/federation/process_federation_client.cpp
  cpp/src/internal/federation/process_transport_session.cpp
  cpp/src/internal/federation/transport_protocol.cpp
  cpp/src/internal/federation/transport_service_protocol.cpp
  cpp/src/internal/handles/attribute_handle_directory.cpp
  cpp/src/internal/handles/dimension_handle_directory.cpp
  cpp/src/internal/federation/federate_lifecycle.cpp
  cpp/src/internal/time/federate_time_state.cpp
    cpp/src/internal/federation/federation_management_coordinator.cpp
    cpp/src/internal/federation/federation_registry.cpp
    cpp/src/internal/federation/federation_state_image.cpp
    cpp/src/internal/federation/federation_save_commit_store.cpp
  cpp/src/internal/handles/interaction_class_handle_directory.cpp
  cpp/src/internal/observability/mom_service_report_encoding.cpp
  cpp/src/internal/observability/service_report_store.cpp
  cpp/src/internal/handles/object_class_handle_directory.cpp
  cpp/src/internal/handles/parameter_handle_directory.cpp
  cpp/src/internal/handles/transportation_type_handle_directory.cpp
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
if(WIN32)
  target_link_libraries(umbra_rti PRIVATE ws2_32)
endif()

# IEEE 1516.1-2010 / 1516e bounded binding. This target owns the official
# basic/composite encoders, reference integer64/float64 logical-time families,
# connection lifecycle, and a deterministic in-process federation /
# declaration / receive-order object callback slice. FOM loading and every
# remaining RTI/MOM ambassador service remain explicit RTIinternalError gates.
add_library(umbra_rti_2010 STATIC
  cpp/src/ieee1516_2010_handles.cpp
  cpp/src/ieee1516_2010_null.cpp
  cpp/src/ieee1516_2010_support.cpp
  cpp/src/internal/runtime/reference_2010.cpp
  cpp/src/ieee1516_2010_basic_data_elements.cpp
  cpp/src/ieee1516_2010_fixed_array.cpp
  cpp/src/ieee1516_2010_fixed_record.cpp
  cpp/src/ieee1516_2010_integer_time.cpp
  cpp/src/ieee1516_2010_variable_array.cpp
  cpp/src/ieee1516_2010_variant_record.cpp
  cpp/src/ieee1516_2010_float_time.cpp
)
add_library(umbra::rti_2010 ALIAS umbra_rti_2010)
set_target_properties(umbra_rti_2010 PROPERTIES EXPORT_NAME rti_2010)
target_compile_features(umbra_rti_2010 PUBLIC cxx_std_14)
target_compile_definitions(umbra_rti_2010 PUBLIC STATIC_RTI STATIC_FEDTIME RTI_DISABLE_WARNINGS)
target_include_directories(umbra_rti_2010
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/ieee1516.1-2010/include>
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
)
target_link_libraries(umbra_rti_2010 PUBLIC umbra::ieee1516_2010_headers)
if(MSVC)
  target_compile_options(umbra_rti_2010 PRIVATE /bigobj)
endif()

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
  set_target_properties(umbra_fom_validation_backend PROPERTIES
    EXPORT_NAME fom_validation_backend
  )

  if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    # Keep both locations in the binary. The source-tree location makes the
    # development profile deterministic; the installed location lets the
    # same binary use the reviewed 1516.2 resources after installation. The
    # package config declares the matching LibXml2 dependency below.
    target_compile_definitions(umbra_rti PRIVATE
      UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY=L"${UMBRA_IEEE1516_2_2025_RESOURCE_DIR}"
      UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL=L"${CMAKE_INSTALL_FULL_DATADIR}/umbra_rti/ieee1516.2-2025"
    )
    if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
      target_compile_definitions(umbra_rti PRIVATE
        UMBRA_EMBEDDED_FOM_2010_RESOURCE_DIRECTORY=L"${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
      )
    endif()
    target_link_libraries(umbra_rti PRIVATE umbra_fom_validation_backend)
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

  add_executable(umbra_ieee1516_2010_binding_shell_smoke
    cpp/tests/ieee1516_2010_binding_shell_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_binding_shell_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_binding_shell_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.binding_shell
    COMMAND umbra_ieee1516_2010_binding_shell_smoke
  )

  add_executable(umbra_ieee1516_2010_headers_smoke
    cpp/tests/ieee1516_2010_headers_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_headers_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_headers_smoke PRIVATE umbra::ieee1516_2010_headers)
  add_test(
    NAME umbra.ieee1516e_2010.headers
    COMMAND umbra_ieee1516_2010_headers_smoke
  )

  add_executable(umbra_ieee1516_2010_encoding_smoke
    cpp/tests/ieee1516_2010_encoding_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_encoding_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_encoding_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.encoding
    COMMAND umbra_ieee1516_2010_encoding_smoke
  )

  add_executable(umbra_ieee1516_2010_composite_encoding_smoke
    cpp/tests/ieee1516_2010_composite_encoding_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_composite_encoding_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_composite_encoding_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.composite_encoding
    COMMAND umbra_ieee1516_2010_composite_encoding_smoke
  )

  add_executable(umbra_ieee1516_2010_integer_time_smoke
    cpp/tests/ieee1516_2010_integer_time_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_integer_time_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_integer_time_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.integer_time
    COMMAND umbra_ieee1516_2010_integer_time_smoke
  )

  add_executable(umbra_ieee1516_2010_float_time_smoke
    cpp/tests/ieee1516_2010_float_time_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_float_time_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_float_time_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.float_time
    COMMAND umbra_ieee1516_2010_float_time_smoke
  )

  add_executable(umbra_ieee1516_2010_time_marshal_smoke
    cpp/tests/ieee1516_2010_time_marshal_smoke.cpp
  )
  target_compile_features(umbra_ieee1516_2010_time_marshal_smoke PRIVATE cxx_std_14)
  target_link_libraries(umbra_ieee1516_2010_time_marshal_smoke PRIVATE umbra::rti_2010)
  add_test(
    NAME umbra.ieee1516e_2010.time_marshal
    COMMAND umbra_ieee1516_2010_time_marshal_smoke
  )

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
      NAME umbra.ieee1516e_2010.binding_shell_generation
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_1516e_cpp_shell.py --check
    )
    add_test(
      NAME umbra.ieee1516e_2010.provider_boundaries
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_1516e_provider_boundaries.py
    )
    add_test(
      NAME umbra.hla.symbolic_names
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_hla_symbolic_names.py
        --root ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_test(
      NAME umbra.fom.rpr_standard_boundary
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_rpr_standard_boundary.py
    )
    add_test(
      NAME umbra.ieee1516_2_2025.resources
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/ieee_1516_2_resources.py --check
    )
    add_test(
      NAME umbra.ieee1516_2025.catch2_plan_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py check-plan
        --bundle ${CMAKE_CURRENT_SOURCE_DIR}/.compliance/corpus-bundle.json
        --plan ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/catch2-test-plan.json
    )
    add_test(
      NAME umbra.planning.work_index
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/query_rti_work.py check
    )
    set_tests_properties(umbra.planning.work_index
      PROPERTIES LABELS "foundation;planning;traceability"
    )
    add_test(
      NAME umbra.planning.query_matrix_regression
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/query_rti_work_regression.py
    )
    set_tests_properties(umbra.planning.query_matrix_regression
      PROPERTIES LABELS "foundation;planning;traceability"
    )
    add_test(
      NAME umbra.cpp_tck.catalog_traceability
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/cpp_tck.py
    )
    set_tests_properties(umbra.cpp_tck.catalog_traceability
      PROPERTIES LABELS "foundation;cpp-tck;traceability"
    )
    add_test(
      NAME umbra.ieee1516_2025.requirements_lab_observations
      COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab.py
        check-observations
        --observations ${CMAKE_CURRENT_SOURCE_DIR}/docs/testing/REQUIREMENTS-LAB-OBSERVATIONS.md
        --baseline ${CMAKE_CURRENT_SOURCE_DIR}/compliance/requirements-lab/observations-historical-baseline.json
    )
    add_test(
      NAME umbra.ieee1516_2025.requirements_lab_observation_guard
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab_observations_regression.py
    )
    set_tests_properties(
      umbra.ieee1516_2025.requirements_lab_observation_guard
      PROPERTIES LABELS "requirements-lab;traceability"
    )
    add_test(
      NAME umbra.ieee1516_2025.requirements_lab_resync_guard
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/requirements_lab_resync_regression.py
    )
    set_tests_properties(
      umbra.ieee1516_2025.requirements_lab_resync_guard
      PROPERTIES LABELS "requirements-lab;traceability"
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
    if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
      set(UMBRA_EXTERNAL_2010_INTEGRITY_ARGS
        --root ${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}
      )
      if(UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH)
        list(APPEND UMBRA_EXTERNAL_2010_INTEGRITY_ARGS
          --target-radar ${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}
        )
      endif()
      add_test(
        NAME umbra.external_2010_fom_resources.integrity
        COMMAND ${Python3_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/verify_external_2010_fom_resources.py
          ${UMBRA_EXTERNAL_2010_INTEGRITY_ARGS}
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
    MATCH "^umbra\\.ieee1516_2_2025\\.class_member_name_uniqueness_traceability$"
    LABELS fom-member-name-uniqueness
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2_2025\\.name_conventions_traceability$"
    LABELS fom-name-conventions
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2_2025\\.modification_date_traceability$"
    LABELS fom-modification-date
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
    MATCH "federation_listing_(requirements|api)_traceability$"
    LABELS federation-listing
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
    MATCH "timestamped_attribute_update_regulation_reenable_requirements_traceability$"
    LABELS timestamped-default-region-attribute-regulation-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_regulation_reenable_api_traceability$"
    LABELS timestamped-default-region-attribute-regulation-reenable
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-attribute-update-regulation-reenable-changed-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_interaction_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-interaction-regulation-reenable-changed-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_object_deletion_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-object-deletion-regulation-reenable-changed-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_directed_interaction_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-directed-interaction-regulation-reenable-changed-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_interaction_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-regional-interaction-regulation-reenable-changed-lookahead
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_regional_attribute_update_regulation_reenable_changed_lookahead_(requirements|api)_traceability$"
    LABELS timestamped-regional-attribute-update-regulation-reenable-changed-lookahead
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
    MATCH "^umbra\\.ieee1516_2025\\.attribute_value_update_request_(requirements|api)_traceability$"
    LABELS object-instance-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "timestamped_attribute_update_(requirements|api)_traceability$"
    LABELS object-instance-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_role_(requirements|api)_traceability$"
    LABELS object-instance-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "time_advance_(requirements|api)_traceability$"
    LABELS object-instance-request-provider-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "order_type_control_(requirements|api)_traceability$"
    LABELS object-instance-request-provider-response
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
    MATCH "umbra\\.ieee1516_2025\\.transportation_type_(requirements|api)_traceability$"
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
    MATCH "umbra\\.ieee1516_2025\\.attribute_ownership_divestiture_if_wanted_(requirements|api)_traceability$"
    LABELS ownership-acquisition-cancellation-race
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.ownership_assumption_research_(requirements|api)_traceability$"
    LABELS ownership-assumption-research
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.ownership_assumption_research_(requirements|api)_traceability$"
    LABELS ownership-assumption-search-epoch
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.ownership_assumption_research_(requirements|api)_traceability$"
    LABELS ownership-assumption-search-continuation
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.attribute_ownership_acquisition_(requirements|api)_traceability$"
    LABELS ownership-acquisition-cancellation-race
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.catch2\\.Embedded cancellation race returns Attribute Ownership Unavailable after owner denial$"
    LABELS ownership-acquisition-cancellation-race
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.catch2\\.Embedded cancellation race returns Attribute Ownership Acquisition Notification after transfer$"
    LABELS ownership-acquisition-cancellation-race
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.catch2\\.Embedded ownership assumption search starts a fresh epoch after transfer$"
    LABELS ownership-assumption-search-epoch
  )
  umbra_label_registered_ctest_tests(
    MATCH "^umbra\\.ieee1516_2025\\.catch2\\.Embedded ownership assumption search advances after a declined callback$"
    LABELS ownership-assumption-search-continuation
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
    MATCH "umbra\\.ieee1516_2025\\.directed_interaction_(requirements|api)_traceability$"
    LABELS directed-send-interaction-service-report-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers directed invocation reports before directed callbacks$"
    LABELS directed-send-interaction-service-report-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS directed-send-interaction-service-report-interaction
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
    MATCH "regional_declaration_relevance_advisory_(requirements|api)_traceability$"
    LABELS declaration-relevance-advisory-regional
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded regional declaration relevance advisories follow active subscriptions$"
    LABELS declaration-relevance-advisory-regional
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
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records accepted regional Send Interaction With Regions before interaction callback$"
    LABELS ordinary-regional-interaction-service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction$"
    LABELS ordinary-regional-interaction-service-report
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers accepted regional Send Interaction With Regions through MOM interaction$"
    LABELS ordinary-regional-interaction-service-report-mom-interaction
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
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed regional Send Interaction With Regions invocations$"
    LABELS ordinary-regional-interaction-failure-service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction$"
    LABELS ordinary-regional-interaction-failure
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed regional Send Interaction With Regions invocations through MOM interaction$"
    LABELS ordinary-regional-interaction-failure-mom-interaction
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
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records Provide Attribute Value Update before its callback$"
    LABELS provide-attribute-value-update-service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting preserves receive-order Update Attribute Values arguments$"
    LABELS update-attribute-values-service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting preserves receive-order Delete Object Instance arguments$"
    LABELS delete-object-instance-service-report-file
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed receive-order Delete Object Instance invocations$"
    LABELS delete-object-instance-failure-file-matrix
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers failed receive-order Delete Object Instance invocations through MOM interaction$"
    LABELS receive-order-delete-object-instance-failure-mom-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded regional provider response rechecks DDM eligibility at reflection delivery$"
    LABELS regional-provider-response-ddm-recheck
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded Request Attribute Value Update supports a 2025 provider response$"
    LABELS ordinary-attribute-value-update-response
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded ownership transfer clears the former owner's 2025 update-region association$"
    LABELS ownership-transfer-update-region
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting records failed regional Send Interaction With Regions invocations$"
    LABELS ordinary-regional-interaction-failure
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
    MATCH "receive_order_interaction_(requirements|api)_traceability$"
    LABELS receive-order-send-interaction-service-report-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "umbra\\.ieee1516_2025\\.catch2\\.Embedded service reporting delivers receive-order Send Interaction through MOM interaction$"
    LABELS receive-order-send-interaction-service-report-interaction
  )
  umbra_label_registered_ctest_tests(
    MATCH "mom_service_reporting_requirements_traceability$"
    LABELS receive-order-send-interaction-service-report-interaction
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
  # Region templates/specifications have their own lifecycle lane.  Keep the
  # official C++ create/commit/query/delete case separate from the broader DDM
  # routing matrix so a region-state edit does not require a full DDM run.
  umbra_label_registered_ctest_tests(
    MATCH "region_lifecycle_(requirements|api)_traceability$"
    LABELS region-lifecycle
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
  # Keep the internal filesystem/memory store seam independently runnable.
  # The service-report lifecycle contracts are its standards-facing boundary;
  # the Catch2 [service-report-store] cases exercise the implementation seam.
  umbra_label_registered_ctest_tests(
    MATCH "service_report_file_lifecycle_(requirements|api)_traceability$"
    LABELS service-report-store
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
      cpp/tests/fom_wire_codec_catch2.cpp
      cpp/tests/rpr_standard_isolation_catch2.cpp
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
      cpp/tests/ieee1516_2025_connection_catch2.cpp
      cpp/tests/ieee1516_2025_time_catch2.cpp
    )
    if(UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
      add_executable(umbra_process_federation_service_probe
        cpp/tests/process_federation_service_probe.cpp
      )
      target_compile_features(umbra_process_federation_service_probe PRIVATE cxx_std_20)
      target_compile_definitions(umbra_process_federation_service_probe PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_include_directories(umbra_process_federation_service_probe PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_link_libraries(umbra_process_federation_service_probe PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        umbra::fedtime
      )
      target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/libxml2_fom_composer_catch2.cpp
          cpp/tests/libxml2_fom_validator_catch2.cpp
          cpp/tests/fom_declaration_management_catch2.cpp
          cpp/tests/federation_registry_catch2.cpp
      )
      # Keep the federation-registry/state-image slice independently buildable.
      # The aggregate 2025 executable also contains the large federation-
      # management translation unit; this target gives the registry's durable
      # state and membership tests a bounded compile/run path while unrelated
      # aggregate cases are under repair.
      if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
        add_executable(umbra_federation_registry_catch2
          cpp/tests/federation_registry_catch2.cpp
        )
        target_compile_features(umbra_federation_registry_catch2 PRIVATE cxx_std_20)
        if(MSVC)
          target_compile_options(umbra_federation_registry_catch2 PRIVATE /bigobj)
        endif()
        target_include_directories(umbra_federation_registry_catch2 PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(umbra_federation_registry_catch2 PRIVATE
          UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
          UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(umbra_federation_registry_catch2 PRIVATE
          umbra_fom_validation_backend
          umbra::rti
          umbra::fedtime
          umbra::authorizer
          Catch2::Catch2WithMain
        )
      endif()
      if(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/external_siso_fom_catch2.cpp
        )
        target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
          UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY="${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}"
        )
      endif()
      if(UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY)
        add_executable(umbra_external_siso_fom_catch2
          cpp/tests/external_siso_fom_catch2.cpp
        )
        target_compile_features(umbra_external_siso_fom_catch2 PRIVATE cxx_std_20)
        target_compile_definitions(umbra_external_siso_fom_catch2 PRIVATE
          UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY="${UMBRA_EXTERNAL_SISO_FOM_CORPUS_DIRECTORY}"
          UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        )
        if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
          target_compile_definitions(umbra_external_siso_fom_catch2 PRIVATE
            UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY="${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
          )
        endif()
        if(UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH)
          target_compile_definitions(umbra_external_siso_fom_catch2 PRIVATE
            UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH="${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}"
          )
        endif()
        target_include_directories(umbra_external_siso_fom_catch2 PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_link_libraries(umbra_external_siso_fom_catch2 PRIVATE
          umbra_fom_validation_backend
          umbra::rti
          umbra::fedtime
          umbra::authorizer
          Catch2::Catch2WithMain
        )
        if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
          target_compile_definitions(umbra_external_siso_fom_catch2 PRIVATE
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
          )
        endif()
      endif()
      if(UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/external_2025_fom_catch2.cpp
        )
        target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
          UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY="${UMBRA_EXTERNAL_2025_FOM_CORPUS_DIRECTORY}"
        )
      endif()
      if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/external_2010_fom_catch2.cpp
        )
        target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
          UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY="${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
        )
        if(UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH)
          target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
            UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH="${UMBRA_EXTERNAL_TARGET_RADAR_FOM_PATH}"
          )
        endif()
      endif()
      if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
        target_sources(umbra_ieee1516_2025_catch2 PRIVATE
          cpp/tests/ieee1516_2025_federation_management_catch2.cpp
          cpp/tests/attribute_relevance_advisory_catch2.cpp
          cpp/tests/attribute_scope_advisory_catch2.cpp
          cpp/tests/auto_provide_baseline_catch2.cpp
          cpp/tests/attribute_value_update_request_baseline_catch2.cpp
          cpp/tests/object_class_attribute_value_update_request_baseline_catch2.cpp
          cpp/tests/object_class_attribute_value_update_service_report_catch2.cpp
          cpp/tests/request_attribute_value_update_service_report_catch2.cpp
          cpp/tests/provide_attribute_value_update_service_report_catch2.cpp
          cpp/tests/update_attribute_values_service_report_catch2.cpp
          cpp/tests/delete_object_instance_service_report_catch2.cpp
          cpp/tests/delete_object_instance_failure_service_report_catch2.cpp
          cpp/tests/delete_object_instance_failure_service_report_interaction_catch2.cpp
          cpp/tests/regional_attribute_value_update_provider_response_recheck_catch2.cpp
          cpp/tests/passive_regional_interaction_transition_catch2.cpp
          cpp/tests/attribute_value_update_response_catch2.cpp
          cpp/tests/ownership_transfer_update_region_catch2.cpp
          cpp/tests/regional_interaction_failure_service_report_file_catch2.cpp
          cpp/tests/regional_interaction_failure_service_report_interaction_catch2.cpp
          cpp/tests/regional_interaction_service_report_file_catch2.cpp
          cpp/tests/regional_interaction_service_report_interaction_catch2.cpp
          cpp/tests/connection_loss_directed_tso_catch2.cpp
          cpp/tests/connection_loss_attribute_update_tso_catch2.cpp
          cpp/tests/connection_loss_attribute_update_tso_multi_recipient_catch2.cpp
          cpp/tests/connection_loss_attribute_update_tso_late_tar_catch2.cpp
          cpp/tests/connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2.cpp
          cpp/tests/connection_loss_attribute_update_tso_immediate_catch2.cpp
          cpp/tests/connection_loss_object_deletion_tso_catch2.cpp
          cpp/tests/connection_loss_object_deletion_tso_multi_recipient_catch2.cpp
          cpp/tests/connection_loss_directed_selector_mutation_catch2.cpp
          cpp/tests/connection_loss_regional_selector_mutation_catch2.cpp
          cpp/tests/restore_live_tso_regional_attribute_update_catch2.cpp
          cpp/tests/live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp
          cpp/tests/restore_live_tso_regional_interaction_catch2.cpp
          cpp/tests/restore_live_tso_interaction_multi_recipient_catch2.cpp
          cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp
          cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp
          cpp/tests/timed_restore_live_tso_regional_attribute_update_multi_recipient_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_source_resignation_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_delete_then_divest_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_then_delete_then_divest_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_cancel_pending_ownership_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_if_available_cancel_pending_ownership_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_catch2.cpp
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_if_available_regular_candidate_continuation_after_restore_catch2.cpp
          cpp/tests/timed_restore_live_tso_default_region_interaction_catch2.cpp
          cpp/tests/timed_restore_live_tso_directed_interaction_catch2.cpp
          cpp/tests/timestamped_directed_interaction_alternate_advance_catch2.cpp
          cpp/tests/timed_restore_live_tso_default_region_attribute_update_catch2.cpp
          cpp/tests/restore_live_tso_default_region_attribute_update_catch2.cpp
          cpp/tests/restore_live_tso_attribute_update_catch2.cpp
          cpp/tests/auto_provide_service_report_file_catch2.cpp
          cpp/tests/joined_federate_mom_federate_state_save_restore_catch2.cpp
          cpp/tests/timestamped_delete_object_instance_failure_service_report_catch2.cpp
          cpp/tests/timestamped_delete_object_instance_service_report_catch2.cpp
          cpp/tests/time_regulated_timestamped_delete_object_instance_service_report_catch2.cpp
          cpp/tests/timestamped_delete_object_instance_service_report_interaction_catch2.cpp
          cpp/tests/timestamped_delete_object_instance_failure_service_report_interaction_catch2.cpp
          cpp/tests/timestamped_regional_interaction_service_report_file_catch2.cpp
          cpp/tests/timestamped_regional_interaction_service_report_interaction_catch2.cpp
          cpp/tests/timestamped_regional_interaction_failure_service_report_file_catch2.cpp
          cpp/tests/timestamped_regional_interaction_failure_service_report_interaction_catch2.cpp
          cpp/tests/connection_loss_directed_tso_late_tar_catch2.cpp
        )
      endif()
      target_link_libraries(umbra_ieee1516_2025_catch2 PRIVATE umbra_fom_validation_backend)
      target_compile_definitions(umbra_ieee1516_2025_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_PROCESS_SERVICE_PROBE_PATH="$<TARGET_FILE:umbra_process_federation_service_probe>"
      )
      add_dependencies(umbra_ieee1516_2025_catch2
        umbra_process_federation_service_probe
      )

      # Keep the process-boundary connection slice independently buildable.
      # The aggregate 2025 executable also contains the large federation-
      # management translation unit; this focused target lets the process
      # endpoint lane compile and run while that unrelated source is under
      # development.  It intentionally uses the same official headers,
      # validator backend, and production RTI library as the aggregate target.
      if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
        add_executable(umbra_ieee1516_2025_connection_catch2
          cpp/tests/ieee1516_2025_connection_catch2.cpp
        )
        target_compile_features(umbra_ieee1516_2025_connection_catch2 PRIVATE cxx_std_20)
        if(MSVC)
          target_compile_options(umbra_ieee1516_2025_connection_catch2 PRIVATE /bigobj)
        endif()
        target_include_directories(umbra_ieee1516_2025_connection_catch2 PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(umbra_ieee1516_2025_connection_catch2 PRIVATE
          UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
          UMBRA_PROCESS_SERVICE_PROBE_PATH="$<TARGET_FILE:umbra_process_federation_service_probe>"
          UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(umbra_ieee1516_2025_connection_catch2 PRIVATE
          umbra_fom_validation_backend
          umbra::rti
          umbra::fedtime
          umbra::authorizer
          Catch2::Catch2WithMain
        )
        add_dependencies(umbra_ieee1516_2025_connection_catch2
          umbra_process_federation_service_probe
        )

        # Keep the private process-boundary tests independently buildable as
        # well.  The aggregate 2025 executable also contains the damaged
        # federation-management translation unit, so its historical private
        # tests cannot be the reproducible source for the process JUnit lane.
        add_executable(umbra_process_boundary_private_catch2
          cpp/tests/process_transport_catch2.cpp
          cpp/tests/process_transport_service_catch2.cpp
          cpp/tests/process_federation_callback_bridge_catch2.cpp
          cpp/tests/transport_protocol_catch2.cpp
          cpp/tests/process_federation_service_catch2.cpp
          cpp/tests/process_federation_additional_fom_catch2.cpp
          cpp/tests/process_federation_service_process_catch2.cpp
        )
        target_compile_features(
          umbra_process_boundary_private_catch2
          PRIVATE cxx_std_20
        )
        if(MSVC)
          target_compile_options(umbra_process_boundary_private_catch2 PRIVATE /bigobj)
        endif()
        target_include_directories(
          umbra_process_boundary_private_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_process_boundary_private_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_PROCESS_SERVICE_PROBE_PATH="$<TARGET_FILE:umbra_process_federation_service_probe>"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_process_boundary_private_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )
        add_dependencies(
          umbra_process_boundary_private_catch2
          umbra_process_federation_service_probe
        )

        # Keep the federation-wide MOM HLAsetSwitches Auto Provide case
        # independently buildable.  This narrow management slice remains
        # queryable while the large aggregate federation-management source is
        # under repair.
        add_executable(umbra_auto_provide_mom_catch2
          cpp/tests/auto_provide_mom_catch2.cpp
        )
        target_compile_features(
          umbra_auto_provide_mom_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_auto_provide_mom_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_auto_provide_mom_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_auto_provide_mom_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM HLAsetSwitches subset independently
        # buildable. This focused case covers predefined and extension
        # parameters, sender-only mutation, malformed wire values, and the
        # service-reporting interlock without reopening the aggregate test.
        add_executable(umbra_mom_federate_set_switches_catch2
          cpp/tests/mom_federate_set_switches_catch2.cpp
        )
        target_compile_features(
          umbra_mom_federate_set_switches_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_mom_federate_set_switches_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_mom_federate_set_switches_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_mom_federate_set_switches_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the injected writer-creation failure contract independently
        # buildable. This is an internal/test-only seam that proves a failed
        # joined-federate report setup rolls membership back without exposing
        # a memory-backed fallback through the production profile.
        add_executable(umbra_service_report_writer_failure_catch2
          cpp/tests/service_report_writer_failure_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_writer_failure_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_writer_failure_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_writer_failure_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_writer_failure_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the disabled Auto Provide discovery-only baseline independently
        # buildable. It is the negative control for the richer provider
        # admission and switch-mutation slices.
        add_executable(umbra_auto_provide_disabled_discovery_only_catch2
          cpp/tests/auto_provide_disabled_discovery_only_catch2.cpp
        )
        target_compile_features(
          umbra_auto_provide_disabled_discovery_only_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_auto_provide_disabled_discovery_only_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_auto_provide_disabled_discovery_only_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_auto_provide_disabled_discovery_only_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep object-instance-name reservation state and callback outcomes
        # independently buildable. The aggregate case remains historical
        # evidence while its focused replacement owns the executable source.
        add_executable(umbra_object_instance_name_reservation_catch2
          cpp/tests/object_instance_name_reservation_catch2.cpp
        )
        target_compile_features(
          umbra_object_instance_name_reservation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_object_instance_name_reservation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_object_instance_name_reservation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_object_instance_name_reservation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep named object-instance registration independently buildable. It
        # consumes the reservation state above and exercises the regional
        # registration surface without the damaged aggregate translation unit.
        add_executable(umbra_object_instance_named_registration_catch2
          cpp/tests/object_instance_named_registration_catch2.cpp
        )
        target_compile_features(
          umbra_object_instance_named_registration_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_object_instance_named_registration_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_object_instance_named_registration_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_object_instance_named_registration_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep unnamed object-instance registration/discovery independently
        # buildable. This focused lifecycle case covers publication,
        # superclass promotion, known-instance lookup, and callback gating
        # without depending on the damaged aggregate translation unit.
        add_executable(umbra_object_instance_registration_discovery_catch2
          cpp/tests/object_instance_registration_discovery_catch2.cpp
        )
        target_compile_features(
          umbra_object_instance_registration_discovery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_object_instance_registration_discovery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_object_instance_registration_discovery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_object_instance_registration_discovery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the no-time regional object-attribute routing slice independently
        # buildable. It is the focused replacement for the matching aggregate
        # declaration and owns the 2025 §§9.5-9.9 overlap evidence.
        add_executable(umbra_regional_object_attribute_routing_catch2
          cpp/tests/regional_object_attribute_routing_catch2.cpp
        )
        target_compile_features(
          umbra_regional_object_attribute_routing_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_object_attribute_routing_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_object_attribute_routing_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_object_attribute_routing_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the zero-dimensional regional-interaction DDM slice separately
        # buildable.  Its explicit lane proves the §9.1.3.2 no-overlap rule
        # without coupling the interaction case to the larger object-routing
        # translation unit.
        add_executable(umbra_zero_dimensional_regional_interaction_catch2
          cpp/tests/zero_dimensional_regional_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_zero_dimensional_regional_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_zero_dimensional_regional_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_zero_dimensional_regional_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_zero_dimensional_regional_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the positive dimensional/default-region interaction matrix
        # separately runnable from the aggregate federation-management target.
        add_executable(umbra_default_region_interaction_routing_catch2
          cpp/tests/default_region_interaction_routing_catch2.cpp
        )
        target_compile_features(
          umbra_default_region_interaction_routing_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_default_region_interaction_routing_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_default_region_interaction_routing_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_default_region_interaction_routing_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the multi-region interaction routing matrix independently
        # runnable. It exercises the official RegionHandleSet union semantics
        # without coupling the next DDM slice to the aggregate translation
        # unit.
        add_executable(umbra_multi_region_interaction_routing_catch2
          cpp/tests/multi_region_interaction_routing_catch2.cpp
        )
        target_compile_features(
          umbra_multi_region_interaction_routing_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_multi_region_interaction_routing_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_multi_region_interaction_routing_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_multi_region_interaction_routing_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep mixed-dimensional Send Interaction With Regions validation
        # independently runnable. It proves the all-or-nothing region-set
        # check without coupling this boundary to the aggregate DDM target.
        add_executable(umbra_mixed_region_interaction_validation_catch2
          cpp/tests/mixed_region_interaction_validation_catch2.cpp
        )
        target_compile_features(
          umbra_mixed_region_interaction_validation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_mixed_region_interaction_validation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_mixed_region_interaction_validation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_mixed_region_interaction_validation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order object-deletion lifecycle independently
        # buildable. It owns the no-time §6.16/§6.16.4/§6.17.1 evidence rather
        # than depending on the damaged aggregate translation unit.
        add_executable(umbra_object_instance_deletion_catch2
          cpp/tests/object_instance_deletion_catch2.cpp
        )
        target_compile_features(
          umbra_object_instance_deletion_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_object_instance_deletion_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_object_instance_deletion_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_object_instance_deletion_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order attribute-update/passel lifecycle independently
        # buildable. It owns the non-region §§6.1.12, 6.10, and 6.11.1 evidence
        # rather than depending on the damaged aggregate translation unit.
        add_executable(umbra_receive_order_attribute_update_catch2
          cpp/tests/receive_order_attribute_update_catch2.cpp
        )
        target_compile_features(
          umbra_receive_order_attribute_update_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_receive_order_attribute_update_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_receive_order_attribute_update_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_receive_order_attribute_update_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the Delay Subscription Evaluation timestamped directed-
        # interaction case independently buildable. It covers creation-time
        # switch enablement/defaulting under both callback models.
        add_executable(umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
          cpp/tests/delay_subscription_evaluation_timestamped_directed_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order Delay Subscription Evaluation directed-
        # interaction case independently buildable. It covers switch
        # enablement/defaulting and callback-time selector rechecking.
        add_executable(umbra_delay_subscription_evaluation_directed_interaction_catch2
          cpp/tests/delay_subscription_evaluation_directed_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_delay_subscription_evaluation_directed_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_delay_subscription_evaluation_directed_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_delay_subscription_evaluation_directed_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_delay_subscription_evaluation_directed_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the filesystem-backed Confirm Synchronization Point
        # Registration result lane independently buildable. It verifies that
        # both callback-result forms are durable before HLA_EVOKED delivery.
        add_executable(umbra_service_report_file_confirm_synchronization_point_registration_catch2
          cpp/tests/service_report_file_confirm_synchronization_point_registration_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_file_confirm_synchronization_point_registration_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_file_confirm_synchronization_point_registration_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_file_confirm_synchronization_point_registration_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_file_confirm_synchronization_point_registration_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the filesystem-backed Announce Synchronization Point recipient
        # lane independently buildable. It checks late-join routing and both
        # service-report switch gates without rotating the selected file.
        add_executable(umbra_service_report_file_announce_synchronization_point_catch2
          cpp/tests/service_report_file_announce_synchronization_point_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_file_announce_synchronization_point_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_file_announce_synchronization_point_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_file_announce_synchronization_point_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_file_announce_synchronization_point_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the filesystem-backed Federation Synchronized recipient lane
        # independently buildable. It verifies per-recipient serial ordering,
        # failed-set encoding, and append-before-callback behavior.
        add_executable(umbra_service_report_file_federation_synchronized_catch2
          cpp/tests/service_report_file_federation_synchronized_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_file_federation_synchronized_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_file_federation_synchronized_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_file_federation_synchronized_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_file_federation_synchronized_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the filesystem-backed Synchronization Point Achieved argument
        # lane independently buildable. It isolates §4.17.1 successful-void
        # record forms from the §4.18 completion record.
        add_executable(umbra_service_report_file_synchronization_point_achieved_catch2
          cpp/tests/service_report_file_synchronization_point_achieved_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_file_synchronization_point_achieved_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_file_synchronization_point_achieved_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_file_synchronization_point_achieved_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_file_synchronization_point_achieved_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the resignation-completion service-report lane independently
        # buildable. It isolates removal of an unachieved sync-set member.
        add_executable(umbra_service_report_file_federation_synchronized_resignation_catch2
          cpp/tests/service_report_file_federation_synchronized_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_service_report_file_federation_synchronized_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_service_report_file_federation_synchronized_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_service_report_file_federation_synchronized_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_service_report_file_federation_synchronized_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the local-delete/timestamped-attribute recipient-isolation case
        # independently buildable.  This focused time-management lane proves
        # one recipient can forget queued work without affecting its survivor.
        add_executable(umbra_local_delete_timestamped_attribute_catch2
          cpp/tests/local_delete_timestamped_attribute_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_timestamped_attribute_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_timestamped_attribute_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_timestamped_attribute_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_timestamped_attribute_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the paired local-delete/timestamped-object-removal case
        # independently buildable.  It isolates a queued TSO removal per
        # recipient and retains the official removal metadata assertions.
        add_executable(umbra_local_delete_timestamped_object_removal_catch2
          cpp/tests/local_delete_timestamped_object_removal_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_timestamped_object_removal_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_timestamped_object_removal_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_timestamped_object_removal_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_timestamped_object_removal_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped source-resignation/ownership-recovery case
        # independently buildable.  It proves an accepted TSO passel remains
        # deliverable after its source resigns and another federate acquires.
        add_executable(umbra_timestamped_attribute_source_resignation_catch2
          cpp/tests/timestamped_attribute_source_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_source_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_source_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_source_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_source_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the four-member timestamped source-resignation fanout case
        # independently buildable.  It releases each recipient queue in turn
        # and verifies the copied TSO metadata remains recipient-local.
        add_executable(umbra_timestamped_attribute_source_resignation_fanout_catch2
          cpp/tests/timestamped_attribute_source_resignation_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_source_resignation_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_source_resignation_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_source_resignation_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_source_resignation_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the accepted-TSO ownership-transfer case independently
        # buildable. It verifies that an in-flight timestamped update retains
        # its original producer, payload, time, and retraction after
        # unconditional divestiture and If Available acquisition.
        add_executable(umbra_timestamped_attribute_update_ownership_transfer_catch2
          cpp/tests/timestamped_attribute_update_ownership_transfer_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_ownership_transfer_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_ownership_transfer_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_ownership_transfer_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_ownership_transfer_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the focused attribute-transportation control contract
        # independently buildable. It isolates FOM defaults, per-instance
        # changes, and public query/confirmation callbacks from the aggregate
        # federation-management executable.
        add_executable(umbra_custom_transportation_type_control_catch2
          cpp/tests/custom_transportation_type_control_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_type_control_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_type_control_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_type_control_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_type_control_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation ordinary-delivery case
        # independently buildable. It carries the composed FOM transport
        # through receive-order interaction and attribute callbacks.
        add_executable(umbra_custom_transportation_delivery_catch2
          cpp/tests/custom_transportation_delivery_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_delivery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_delivery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_delivery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_delivery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation ordinary-regional case
        # independently buildable. It isolates regional interaction overlap
        # and callback designator propagation from the aggregate executable.
        add_executable(umbra_custom_transportation_ordinary_regional_interaction_catch2
          cpp/tests/custom_transportation_ordinary_regional_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_ordinary_regional_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_ordinary_regional_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_ordinary_regional_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_ordinary_regional_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation ordinary-regional-attribute
        # case independently buildable. It isolates regional object routing
        # and conveyed designators from the aggregate executable.
        add_executable(umbra_custom_transportation_ordinary_regional_attribute_catch2
          cpp/tests/custom_transportation_ordinary_regional_attribute_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_ordinary_regional_attribute_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_ordinary_regional_attribute_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_ordinary_regional_attribute_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_ordinary_regional_attribute_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation timestamped-delivery case
        # independently buildable. It composes the official interaction and
        # attribute callback surfaces with a user-declared FOM transportation.
        add_executable(umbra_custom_transportation_timestamped_delivery_catch2
          cpp/tests/custom_transportation_timestamped_delivery_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_timestamped_delivery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_timestamped_delivery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_timestamped_delivery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_timestamped_delivery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation timestamped-directed case
        # independently buildable. It isolates target routing and retraction
        # metadata from the aggregate federation-management executable.
        add_executable(umbra_custom_transportation_timestamped_directed_delivery_catch2
          cpp/tests/custom_transportation_timestamped_directed_delivery_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_timestamped_directed_delivery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_timestamped_directed_delivery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_timestamped_directed_delivery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_timestamped_directed_delivery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the declared custom-transportation timestamped regional
        # attribute case independently buildable. It isolates DDM region
        # designator propagation from the aggregate federation-management
        # executable while retaining the official C++ callback surface.
        add_executable(umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
          cpp/tests/custom_transportation_timestamped_regional_attribute_delivery_catch2.cpp
        )
        target_compile_features(
          umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the MOM transportation-type request interaction case
        # independently buildable. It exercises the official MIM payload
        # encoding, callback-gated public changes, and both callback models.
        add_executable(umbra_mom_transportation_type_change_request_catch2
          cpp/tests/mom_transportation_type_change_request_catch2.cpp
        )
        target_compile_features(
          umbra_mom_transportation_type_change_request_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_mom_transportation_type_change_request_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_mom_transportation_type_change_request_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_mom_transportation_type_change_request_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the multi-recipient timestamped directed-interaction resignation
        # case independently buildable. It isolates queued TSO delivery and
        # per-recipient grant ordering from the damaged aggregate target.
        add_executable(umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
          cpp/tests/timestamped_directed_interaction_multi_recipient_source_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the direct TAR/NMR timestamped directed-interaction frontier
        # independently buildable. It is the narrow two-recipient companion
        # to the broader alternate-advance and source-resignation cases.
        add_executable(umbra_timestamped_directed_interaction_tar_nmr_catch2
          cpp/tests/timestamped_directed_interaction_tar_nmr_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_tar_nmr_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_tar_nmr_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_tar_nmr_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_tar_nmr_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep federation-scoped MOM save-conditionals independently
        # buildable. This isolates the pending/admitted/completed conditional
        # attribute contract from the damaged aggregate federation-management
        # translation unit.
        add_executable(umbra_federation_mom_save_conditionals_catch2
          cpp/tests/federation_mom_save_conditionals_catch2.cpp
        )
        target_compile_features(
          umbra_federation_mom_save_conditionals_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_federation_mom_save_conditionals_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_federation_mom_save_conditionals_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_federation_mom_save_conditionals_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the federation-scoped HLAcurrentFDD projection independently
        # buildable. This isolates the composed-FDD request and conditional
        # refresh contract from the damaged aggregate federation-management
        # translation unit.
        add_executable(umbra_federation_mom_current_fdd_catch2
          cpp/tests/federation_mom_current_fdd_catch2.cpp
        )
        target_compile_features(
          umbra_federation_mom_current_fdd_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_federation_mom_current_fdd_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_federation_mom_current_fdd_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_federation_mom_current_fdd_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM GALT/LITS projection independently
        # buildable. This isolates Query GALT/Query LITS and periodic
        # HLAsetTiming reflections from the aggregate federation-management
        # translation unit.
        add_executable(umbra_joined_federate_mom_galt_lits_periodic_catch2
          cpp/tests/joined_federate_mom_galt_lits_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_galt_lits_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_galt_lits_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_galt_lits_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_galt_lits_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM TSO-length projection independently
        # buildable. This isolates the recipient-scoped queued TSO ledger and
        # its HLAsetTiming projection from the aggregate test translation unit.
        add_executable(umbra_joined_federate_mom_tso_length_periodic_catch2
          cpp/tests/joined_federate_mom_tso_length_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_tso_length_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_tso_length_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_tso_length_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_tso_length_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM time-state duration projection
        # independently buildable. This isolates the direct and periodic
        # HLAtimeGrantedTime/HLAtimeAdvancingTime contract from the aggregate
        # federation-management translation unit.
        add_executable(umbra_joined_federate_mom_time_state_duration_catch2
          cpp/tests/joined_federate_mom_time_state_duration_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_time_state_duration_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_time_state_duration_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_time_state_duration_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_time_state_duration_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM reflection-count projection
        # independently buildable. This isolates the distinct-object versus
        # callback-invocation ledger from the aggregate federation-management
        # translation unit.
        add_executable(umbra_joined_federate_mom_reflection_count_catch2
          cpp/tests/joined_federate_mom_reflection_count_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_reflection_count_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_reflection_count_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_reflection_count_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_reflection_count_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM updates-sent projection independently
        # buildable. This isolates accepted Update Attribute Values ledger
        # accounting, transportation buckets, and nested class-count reports.
        add_executable(umbra_joined_federate_mom_updates_sent_catch2
          cpp/tests/joined_federate_mom_updates_sent_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_updates_sent_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_updates_sent_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_updates_sent_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_updates_sent_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM reflections-received projection
        # independently buildable. This isolates accepted application
        # reflection accounting, transportation buckets, and nested
        # class-count reports.
        add_executable(umbra_joined_federate_mom_reflections_received_catch2
          cpp/tests/joined_federate_mom_reflections_received_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_reflections_received_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_reflections_received_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_reflections_received_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_reflections_received_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM interactions-received projection
        # independently buildable. This isolates accepted application receive
        # callbacks, transportation buckets, and nested class-count reports.
        add_executable(umbra_joined_federate_mom_interactions_received_catch2
          cpp/tests/joined_federate_mom_interactions_received_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_interactions_received_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_interactions_received_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_interactions_received_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_interactions_received_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM directed-interactions-received
        # projection independently buildable. This isolates the directed
        # receive subset, ordinary-receive exclusion, and nested count report.
        add_executable(umbra_joined_federate_mom_directed_interactions_received_catch2
          cpp/tests/joined_federate_mom_directed_interactions_received_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_directed_interactions_received_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_directed_interactions_received_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_directed_interactions_received_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_directed_interactions_received_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM interactions-sent projection
        # independently buildable. This isolates accepted Send Interaction
        # accounting, regional inclusion, transportation buckets, and nested
        # interaction-count reports.
        add_executable(umbra_joined_federate_mom_interactions_sent_catch2
          cpp/tests/joined_federate_mom_interactions_sent_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_interactions_sent_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_interactions_sent_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_interactions_sent_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_interactions_sent_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the cross-family MOM NULL-bucket projection independently
        # buildable. This isolates empty sender ledgers for updates, ordinary
        # interactions, and directed interactions.
        add_executable(umbra_mom_sender_count_reports_null_buckets_catch2
          cpp/tests/mom_sender_count_reports_null_buckets_catch2.cpp
        )
        target_compile_features(
          umbra_mom_sender_count_reports_null_buckets_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_mom_sender_count_reports_null_buckets_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_mom_sender_count_reports_null_buckets_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_mom_sender_count_reports_null_buckets_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM removed-object history projection
        # independently buildable. This combines the public deletion/retraction
        # boundary with the receiving federate's historical MOM counter.
        add_executable(umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
          cpp/tests/joined_federate_mom_removed_object_count_tso_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the Allow Relaxed DDM object-attribute boundary case
        # independently buildable.  It runs the enabled and strict profiles
        # without depending on the damaged aggregate translation unit.
        add_executable(umbra_allow_relaxed_ddm_object_attribute_catch2
          cpp/tests/allow_relaxed_ddm_object_attribute_catch2.cpp
        )
        target_compile_features(
          umbra_allow_relaxed_ddm_object_attribute_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_allow_relaxed_ddm_object_attribute_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_allow_relaxed_ddm_object_attribute_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_allow_relaxed_ddm_object_attribute_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped directed-interaction re-enable case
        # independently buildable.  It isolates one queued directed TSO passel
        # across a disable/enable cycle and checks callback/grant ordering.
        add_executable(umbra_timestamped_directed_interaction_reenable_catch2
          cpp/tests/timestamped_directed_interaction_reenable_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_reenable_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_reenable_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_reenable_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_reenable_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the queued directed-TSO source-resignation case independently
        # buildable.  A surviving target owner and regulator release a passel
        # after the separate interaction producer leaves the federation.
        add_executable(umbra_timestamped_directed_interaction_source_resignation_catch2
          cpp/tests/timestamped_directed_interaction_source_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_source_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_source_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_source_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_source_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the directed timestamped queue/retraction case independently
        # buildable. It isolates pre-grant retract, then a later eligible TSO
        # delivery with the official directed-interaction metadata.
        add_executable(umbra_timestamped_directed_interaction_retraction_catch2
          cpp/tests/timestamped_directed_interaction_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the immediate-recipient directed TSO resignation case
        # independently buildable. It verifies that source resignation does
        # not discard a callback already queued on an unconstrained receiver.
        add_executable(umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
          cpp/tests/timestamped_directed_interaction_immediate_source_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the terminal timestamped Send Interaction/retraction case
        # independently buildable.  It exercises pre-grant retraction,
        # terminal post-delivery classification, and Request Retraction.
        add_executable(umbra_timestamped_send_interaction_retraction_catch2
          cpp/tests/timestamped_send_interaction_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_send_interaction_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_send_interaction_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_send_interaction_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_send_interaction_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the no-recipient timestamped Send Interaction designator case
        # independently buildable.  It exercises the retraction ledger without
        # conflating designator validity with recipient fanout.
        add_executable(umbra_timestamped_send_interaction_no_fanout_catch2
          cpp/tests/timestamped_send_interaction_no_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_send_interaction_no_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_send_interaction_no_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_send_interaction_no_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_send_interaction_no_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the no-recipient timestamped Update Attribute Values designator
        # case independently buildable. It verifies that the public retraction
        # contract is not contingent on retaining recipient fanout state.
        add_executable(umbra_timestamped_attribute_update_no_fanout_catch2
          cpp/tests/timestamped_attribute_update_no_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_no_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_no_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_no_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_no_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the m53 timestamped attribute-update queue/retraction case
        # independently buildable. It isolates the pre-grant retract boundary
        # and the later mixed-transportation delivery cohort.
        add_executable(umbra_timestamped_attribute_update_queued_passel_retraction_catch2
          cpp/tests/timestamped_attribute_update_queued_passel_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_queued_passel_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_queued_passel_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_queued_passel_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_queued_passel_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the five-form timestamped designator-terminalization case
        # independently buildable. It isolates the producer-side advance
        # boundary that makes each designator no longer retractable.
        add_executable(umbra_timestamped_tso_designator_terminalization_catch2
          cpp/tests/timestamped_tso_designator_terminalization_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_tso_designator_terminalization_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_tso_designator_terminalization_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_tso_designator_terminalization_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_tso_designator_terminalization_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the Time Regulation disable/re-enable TSO case independently
        # buildable.  It proves the original recipient queue survives role
        # mutation and retains its timestamped metadata.
        add_executable(umbra_timestamped_interaction_regulation_reenable_catch2
          cpp/tests/timestamped_interaction_regulation_reenable_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_regulation_reenable_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_regulation_reenable_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_regulation_reenable_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_regulation_reenable_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the mixed regional timestamped attribute-update advance case
        # independently buildable. It exercises one shared TSO frontier while
        # FQR, TARA, and NMRA use their distinct official request surfaces.
        add_executable(umbra_timestamped_regional_attribute_update_mixed_advance_catch2
          cpp/tests/timestamped_regional_attribute_update_mixed_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_regional_attribute_update_mixed_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_regional_attribute_update_mixed_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_regional_attribute_update_mixed_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_regional_attribute_update_mixed_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-source/default-region mixed advance case
        # independently buildable. It complements the explicit regional
        # source lane while retaining the same FQR/TARA/NMRA frontier.
        add_executable(umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
          cpp/tests/timestamped_default_region_attribute_update_mixed_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-source/default-region timestamped-retraction case
        # independently buildable. It pairs TARA and NMRA alternate advances
        # and proves the queued default-region passel is removed before either
        # recipient crosses its callback boundary.
        add_executable(umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
          cpp/tests/timestamped_default_region_attribute_update_retract_alternate_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional timestamped-retraction/mixed-advance case
        # independently buildable. It proves all three alternate requests
        # complete after one explicit-source passel is retracted.
        add_executable(umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
          cpp/tests/timestamped_regional_attribute_update_retract_mixed_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the ordinary regional TAR/NMR timestamped-update case
        # independently buildable. It is the focused two-recipient counterpart
        # to the broader FQR/TARA/NMRA frontier tests.
        add_executable(umbra_timestamped_regional_attribute_update_tar_nmr_catch2
          cpp/tests/timestamped_regional_attribute_update_tar_nmr_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_regional_attribute_update_tar_nmr_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_regional_attribute_update_tar_nmr_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_regional_attribute_update_tar_nmr_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_regional_attribute_update_tar_nmr_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-source regional timestamped-update resignation
        # case independently buildable. It isolates the queued passel's
        # lifetime across source resignation and its surviving regulator.
        add_executable(umbra_timestamped_regional_attribute_update_resignation_catch2
          cpp/tests/timestamped_regional_attribute_update_resignation_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_regional_attribute_update_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_regional_attribute_update_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_regional_attribute_update_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_regional_attribute_update_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the mixed-fanout regional TSO/retraction case independently
        # buildable. It pairs an immediate recipient with an evoked,
        # time-constrained recipient and checks region metadata in both modes.
        add_executable(umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
          cpp/tests/timestamped_regional_attribute_update_mixed_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional best-effort timestamped-rate case independently
        # buildable. It exercises an FDD Low update-rate gate on a regional
        # subscription while the recipient still crosses each TSO boundary.
        add_executable(umbra_regional_best_effort_timestamped_attribute_rate_catch2
          cpp/tests/regional_best_effort_timestamped_attribute_rate_catch2.cpp
        )
        target_compile_features(
          umbra_regional_best_effort_timestamped_attribute_rate_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_best_effort_timestamped_attribute_rate_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_best_effort_timestamped_attribute_rate_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_best_effort_timestamped_attribute_rate_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order regional best-effort-rate case independently
        # buildable. It exercises the FDD Low update-rate gate and subscription
        # generation reset without depending on the aggregate test binary.
        add_executable(umbra_regional_best_effort_attribute_rate_catch2
          cpp/tests/regional_best_effort_attribute_rate_catch2.cpp
        )
        target_compile_features(
          umbra_regional_best_effort_attribute_rate_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_best_effort_attribute_rate_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_best_effort_attribute_rate_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_best_effort_attribute_rate_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the nonregional TARA/NMRA timestamped update case independently
        # buildable. It is the compact available-advance frontier for the
        # attribute-update service.
        add_executable(umbra_timestamped_attribute_update_available_advance_catch2
          cpp/tests/timestamped_attribute_update_available_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_available_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_available_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_available_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_available_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped update-rate reduction case independently
        # buildable. It isolates the FDD-defined Low-rate boundary while
        # retaining reliable attributes and closing suppressed TSO retractions.
        add_executable(umbra_timestamped_attribute_update_rate_reduction_catch2
          cpp/tests/timestamped_attribute_update_rate_reduction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_rate_reduction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_rate_reduction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_rate_reduction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_rate_reduction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the ordinary timestamped attribute re-enable lifecycle case
        # independently buildable. It verifies that a queued passel survives
        # disable/re-enable of Time Constrained without duplication.
        add_executable(umbra_timestamped_attribute_update_reenable_catch2
          cpp/tests/timestamped_attribute_update_reenable_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_reenable_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_reenable_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_reenable_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_reenable_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the alternate-advance timestamped attribute-ordering case
        # independently buildable. It exercises equal-timestamp cohorts for
        # both TARA and NMRA before admitting the later queued update.
        add_executable(umbra_timestamped_attribute_update_alternate_advance_catch2
          cpp/tests/timestamped_attribute_update_alternate_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_alternate_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_alternate_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_alternate_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_alternate_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the different-timestamp attribute-order cohort independently
        # buildable. It submits timestamp 7 before the timestamp-5 cohort and
        # checks recipient-local ordering through the ordinary TSO advance.
        add_executable(umbra_timestamped_attribute_order_cohort_catch2
          cpp/tests/timestamped_attribute_order_cohort_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_order_cohort_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_order_cohort_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_order_cohort_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_order_cohort_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-region interaction retraction/alternate-advance
        # case independently buildable. It isolates private default-source
        # overlap with regional subscribers and grant completion after retract.
        add_executable(umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
          cpp/tests/timestamped_default_region_interaction_retract_alternate_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped interaction changed-lookahead lifecycle case
        # independently buildable. It verifies that disabling and re-enabling
        # regulation retains the queued message and retract identity.
        add_executable(umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
          cpp/tests/timestamped_interaction_regulation_reenable_changed_lookahead_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped directed-interaction changed-lookahead
        # lifecycle case independently buildable. It verifies that a target-
        # qualified queued interaction retains its retraction identity when
        # regulation is disabled and re-enabled with a new lookahead.
        add_executable(umbra_tso_directed_reenable_changed_lookahead_catch2
          cpp/tests/timestamped_directed_interaction_regulation_reenable_changed_lookahead_catch2.cpp
        )
        target_compile_features(
          umbra_tso_directed_reenable_changed_lookahead_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_directed_reenable_changed_lookahead_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_directed_reenable_changed_lookahead_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_directed_reenable_changed_lookahead_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped attribute changed-lookahead lifecycle case
        # independently buildable. It verifies that disabling and re-enabling
        # regulation retains the queued update and retraction identity.
        add_executable(umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
          cpp/tests/timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed multi-recipient regional save/restore ownership case
        # independently buildable. This focused matrix preserves the saved TSO
        # source while cancelling a retained negotiated confirmation.
        add_executable(umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
          cpp/tests/timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2.cpp
        )
        target_compile_features(
          umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed multi-recipient regional negotiated-cancellation
        # matrix independently buildable. It restores a queued timestamped
        # update, enters negotiated divestiture, and cancels the requester's
        # pending ownership acquisition before confirmation delivery.
        add_executable(umbra_tso_regional_negotiated_cancel_restore_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_regional_negotiated_cancel_restore_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_regional_negotiated_cancel_restore_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_regional_negotiated_cancel_restore_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_regional_negotiated_cancel_restore_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed multi-recipient regional negotiated-continuation
        # matrix independently buildable. It restores a queued timestamped
        # update, retains an If Available candidate after requester
        # cancellation, and completes the surviving negotiated transfer.
        add_executable(umbra_tso_regional_negotiated_continuation_restore_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_regional_negotiated_continuation_restore_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_regional_negotiated_continuation_restore_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_regional_negotiated_continuation_restore_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_regional_negotiated_continuation_restore_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed regular-candidate continuation case independently
        # buildable. The wrapper exposes the macro-based fixture as a direct
        # TEST_CASE for source indexing and a focused CTest handle.
        add_executable(umbra_tso_regional_regular_continuation_restore_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore_lane_catch2.cpp
        )
        target_compile_features(
          umbra_tso_regional_regular_continuation_restore_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_regional_regular_continuation_restore_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_regional_regular_continuation_restore_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_regional_regular_continuation_restore_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed multi-recipient mixed-candidate continuation case
        # independently buildable. It reuses the regular-candidate fixture
        # while selecting an If Available candidate for the surviving clock.
        add_executable(umbra_tso_regional_mixed_continuation_restore_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_regional_mixed_continuation_restore_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_regional_mixed_continuation_restore_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_regional_mixed_continuation_restore_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_regional_mixed_continuation_restore_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed mixed retained-regular pre-delivery cancellation case
        # independently buildable. It reuses the regional save/restore
        # fixture while cancelling before Request Divestiture Confirmation.
        add_executable(umbra_tso_mixed_pre_delivery_cancel_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_mixed_pre_delivery_cancel_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_mixed_pre_delivery_cancel_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_mixed_pre_delivery_cancel_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_mixed_pre_delivery_cancel_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed mixed retained-regular confirmation cancellation case
        # independently buildable. It reuses the regional save/restore
        # fixture while cancelling after Request Divestiture Confirmation.
        add_executable(umbra_tso_mixed_confirmation_cancel_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_mixed_confirmation_cancel_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_mixed_confirmation_cancel_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_mixed_confirmation_cancel_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_mixed_confirmation_cancel_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timed multi-recipient regional negotiated-confirmation
        # cancellation case independently buildable. It restores a queued
        # timestamped update, delivers the retained owner's confirmation, and
        # verifies cancellation rejects stale confirmation without transferring
        # ownership.
        add_executable(umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
          cpp/tests/timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore_catch2.cpp
        )
        target_compile_features(
          umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the nonregional Flush Queue timestamped-update case
        # independently buildable. It checks actual-versus-optimistic grant
        # times and passel ordering without depending on the aggregate source.
        add_executable(umbra_timestamped_attribute_update_flush_queue_catch2
          cpp/tests/timestamped_attribute_update_flush_queue_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_attribute_update_flush_queue_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_attribute_update_flush_queue_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_attribute_update_flush_queue_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_attribute_update_flush_queue_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the future-input Flush Queue interaction case independently
        # buildable. It checks that TSO messages submitted after FQR admission
        # still precede its grant in the evoked callback queue.
        add_executable(umbra_flush_queue_future_input_catch2
          cpp/tests/flush_queue_future_input_catch2.cpp
        )
        target_compile_features(
          umbra_flush_queue_future_input_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_flush_queue_future_input_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_flush_queue_future_input_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_flush_queue_future_input_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the no-recipient timestamped deletion/retraction case
        # independently buildable. It isolates the execution-owned
        # reconstitution boundary before joined-recipient fanout is added.
        add_executable(umbra_request_retraction_timestamped_deletion_no_fanout_catch2
          cpp/tests/request_retraction_timestamped_deletion_no_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_request_retraction_timestamped_deletion_no_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_request_retraction_timestamped_deletion_no_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_request_retraction_timestamped_deletion_no_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_request_retraction_timestamped_deletion_no_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the delivered-recipient timestamped object-deletion retraction
        # case independently buildable. It isolates object reconstitution,
        # Request Retraction fanout, and suppression of the pending recipient.
        add_executable(umbra_timestamped_object_deletion_retraction_catch2
          cpp/tests/timestamped_object_deletion_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_object_deletion_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_object_deletion_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_object_deletion_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_object_deletion_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-owner timestamped-deletion retraction case
        # independently buildable. It isolates resignation filtering during
        # object reconstitution and the resulting ownership query boundary.
        add_executable(umbra_timestamped_object_deletion_joined_owner_retraction_catch2
          cpp/tests/timestamped_object_deletion_joined_owner_retraction_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_object_deletion_joined_owner_retraction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_object_deletion_joined_owner_retraction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_object_deletion_joined_owner_retraction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_object_deletion_joined_owner_retraction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the optimistic-time Flush Queue Request slice independently
        # buildable. It isolates queued TSO FIFO delivery and grant timing.
        add_executable(umbra_flush_queue_request_optimistic_time_catch2
          cpp/tests/flush_queue_request_optimistic_time_catch2.cpp
        )
        target_compile_features(
          umbra_flush_queue_request_optimistic_time_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_flush_queue_request_optimistic_time_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_flush_queue_request_optimistic_time_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_flush_queue_request_optimistic_time_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the terminal timestamped-deletion tombstone case independently
        # buildable. It verifies strict Retract finalization and object-name
        # reuse after the retained deletion state is released.
        add_executable(umbra_terminal_timestamped_deletion_tombstone_catch2
          cpp/tests/terminal_timestamped_deletion_tombstone_catch2.cpp
        )
        target_compile_features(
          umbra_terminal_timestamped_deletion_tombstone_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_terminal_timestamped_deletion_tombstone_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_terminal_timestamped_deletion_tombstone_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_terminal_timestamped_deletion_tombstone_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the source-resignation timestamped-deletion fanout case
        # independently buildable. It verifies that each recipient-local
        # removal queue survives the producing owner's departure.
        add_executable(umbra_timestamped_object_deletion_source_resignation_fanout_catch2
          cpp/tests/timestamped_object_deletion_source_resignation_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_object_deletion_source_resignation_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_object_deletion_source_resignation_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_object_deletion_source_resignation_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_object_deletion_source_resignation_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the compact queued-TSO Next Message Request case independently
        # buildable. It selects the next message timestamp after GALT opens
        # without pulling in the larger alternate-advance matrix.
        add_executable(umbra_next_message_request_queued_tso_catch2
          cpp/tests/next_message_request_queued_tso_catch2.cpp
        )
        target_compile_features(
          umbra_next_message_request_queued_tso_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_next_message_request_queued_tso_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_next_message_request_queued_tso_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_next_message_request_queued_tso_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the paired Available-time TARA/NMRA interaction case
        # independently buildable. It exercises inclusive GALT at two queued
        # timestamp frontiers without introducing a new transport seam.
        add_executable(umbra_available_time_advance_inclusive_galt_catch2
          cpp/tests/available_time_advance_inclusive_galt_catch2.cpp
        )
        target_compile_features(
          umbra_available_time_advance_inclusive_galt_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_available_time_advance_inclusive_galt_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_available_time_advance_inclusive_galt_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_available_time_advance_inclusive_galt_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the bounded connection-loss/TSO declaration-recheck case
        # independently buildable.  The aggregate federation-management
        # translation unit currently carries unrelated source drift; this
        # target lets the exact planned case compile and run without reopening
        # that aggregate file.
        add_executable(umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
          cpp/tests/connection_loss_attribute_update_tso_suppressed_cleanup_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the multi-recipient automatic-cleanup connection-loss case
        # independently buildable. It is already a mapped C++ plan row, but
        # the aggregate federation-management translation unit is not a safe
        # execution boundary while its unrelated source artifact is being
        # reconciled.
        add_executable(umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
          cpp/tests/connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the single-survivor automatic-cleanup connection-loss case
        # independently buildable.  This is the exact planned cutoff slice;
        # the multi-recipient variant above remains a separate evidence row.
        add_executable(umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
          cpp/tests/connection_loss_attribute_update_tso_automatic_cleanup_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the directed-selector connection-loss variants independently
        # buildable.  This target contains both the ownership-selector and
        # unsubscribe-at-callback-boundary cases without the aggregate test
        # translation unit.
        add_executable(umbra_connection_loss_directed_selector_mutation_catch2
          cpp/tests/connection_loss_directed_selector_mutation_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_directed_selector_mutation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_directed_selector_mutation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_directed_selector_mutation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_directed_selector_mutation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the HLA_EVOKED asynchronous-delivery cutoff case independently
        # buildable alongside the existing HLA_IMMEDIATE counterpart.
        add_executable(umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
          cpp/tests/connection_loss_attribute_update_tso_immediate_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the single-recipient timestamped-interaction resignation case
        # independently buildable beside the existing fan-out variant.
        add_executable(umbra_timestamped_interaction_source_resignation_catch2
          cpp/tests/timestamped_interaction_source_resignation_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_source_resignation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_source_resignation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_source_resignation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_source_resignation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-region timestamped attribute callback contract
        # independently buildable.  This is the narrow DDM/time slice behind
        # the next roadmap pointer and does not depend on the damaged
        # federation-management aggregate translation unit.
        add_executable(umbra_timestamped_default_region_attribute_update_catch2
          cpp/tests/timestamped_default_region_attribute_update_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_attribute_update_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_attribute_update_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_attribute_update_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_attribute_update_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-region timestamped re-enable case independently
        # buildable.  It exercises the callback-gated Time Constrained role
        # transition without relying on the aggregate federation-management
        # translation unit.
        add_executable(umbra_timestamped_default_region_attribute_reenable_catch2
          cpp/tests/timestamped_default_region_attribute_reenable_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_attribute_reenable_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_attribute_reenable_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_attribute_reenable_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_attribute_reenable_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-region mixed-fanout/retraction case independently
        # buildable.  It covers one immediate recipient and one constrained
        # recipient while the aggregate source remains under recovery.
        add_executable(umbra_timestamped_default_region_attribute_mixed_fanout_catch2
          cpp/tests/timestamped_default_region_attribute_mixed_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_default_region_attribute_mixed_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_default_region_attribute_mixed_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_default_region_attribute_mixed_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_default_region_attribute_mixed_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the mixed regular/If Available Divestiture If Wanted case
        # independently buildable. It isolates serial multi-federate callback
        # ordering and the selected owner's follow-up release request.
        add_executable(umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
          cpp/tests/attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the supplied-set Divestiture If Wanted case independently
        # buildable. It isolates the empty-result path, mixed pending-acquirer
        # transfer, and acquisition-notification tag/ownership boundaries.
        add_executable(umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
          cpp/tests/attribute_ownership_divestiture_if_wanted_pending_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the negotiated-divestiture transition independently buildable.
        # It isolates owner Waiting/confirmation state, cancellation rollback,
        # and the final acquisition-notification tag boundary.
        add_executable(umbra_negotiated_attribute_ownership_divestiture_pending_catch2
          cpp/tests/negotiated_attribute_ownership_divestiture_pending_catch2.cpp
        )
        target_compile_features(
          umbra_negotiated_attribute_ownership_divestiture_pending_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_negotiated_attribute_ownership_divestiture_pending_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_negotiated_attribute_ownership_divestiture_pending_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_negotiated_attribute_ownership_divestiture_pending_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the MOM Service Reporting subscription interlock independently
        # buildable. It isolates ordinary/regional active and passive
        # subscription admission from the broader MOM service matrix.
        add_executable(umbra_mom_service_reporting_interlock_catch2
          cpp/tests/mom_service_reporting_interlock_catch2.cpp
        )
        target_compile_features(
          umbra_mom_service_reporting_interlock_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_mom_service_reporting_interlock_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_mom_service_reporting_interlock_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_mom_service_reporting_interlock_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM deletable-object projection independently
        # buildable. It isolates the live implicit-privilege ownership ledger,
        # direct AVU request, and HLAsetTiming periodic reflection paths.
        add_executable(umbra_joined_federate_mom_deletable_object_count_catch2
          cpp/tests/joined_federate_mom_deletable_object_count_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_deletable_object_count_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_deletable_object_count_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_deletable_object_count_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_deletable_object_count_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM receive-order length projection
        # independently buildable. It isolates the callback-route queue
        # ledger, direct MOM request, and HLAsetTiming periodic reflection.
        add_executable(umbra_joined_federate_mom_ro_length_periodic_catch2
          cpp/tests/joined_federate_mom_ro_length_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_ro_length_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_ro_length_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_ro_length_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_ro_length_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM object-instances-updated report
        # independently buildable. It isolates the Subscribe-only request,
        # accepted update ledger, nested class-count encoding, and
        # RTI-originated callback route.
        add_executable(umbra_joined_federate_mom_object_instances_updated_report_catch2
          cpp/tests/joined_federate_mom_object_instances_updated_report_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_object_instances_updated_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_object_instances_updated_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_object_instances_updated_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_object_instances_updated_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM deletable-object report independently
        # buildable. It isolates the Subscribe-only request, live
        # HLAprivilegeToDeleteObject ownership ledger, nested class-count
        # encoding, and RTI-originated callback route.
        add_executable(umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
          cpp/tests/joined_federate_mom_object_instances_that_can_be_deleted_report_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM reflected-object report independently
        # buildable. It isolates the accepted application reflection ledger,
        # nested class-count encoding, and RTI-originated callback route.
        add_executable(umbra_joined_federate_mom_object_instances_reflected_report_catch2
          cpp/tests/joined_federate_mom_object_instances_reflected_report_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_object_instances_reflected_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_object_instances_reflected_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_object_instances_reflected_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_object_instances_reflected_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM object-information report independently
        # buildable. It isolates known/NULL object snapshots, nested owned
        # attribute lists, and the requesting federate's callback route.
        add_executable(umbra_joined_federate_mom_object_instance_information_report_catch2
          cpp/tests/joined_federate_mom_object_instance_information_report_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_object_instance_information_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_object_instance_information_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_object_instance_information_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_object_instance_information_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM successful-update projection independently
        # buildable. It isolates the accepted Update Attribute Values ledger,
        # direct MOM request, and HLAsetTiming periodic reflection paths.
        add_executable(umbra_joined_federate_mom_updates_sent_periodic_catch2
          cpp/tests/joined_federate_mom_updates_sent_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_updates_sent_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_updates_sent_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_updates_sent_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_updates_sent_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM updated-object projection independently
        # buildable. It isolates the distinct-object Update Attribute Values
        # ledger, direct MOM request, and HLAsetTiming periodic reflection.
        add_executable(umbra_joined_federate_mom_updated_object_count_periodic_catch2
          cpp/tests/joined_federate_mom_updated_object_count_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_updated_object_count_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_updated_object_count_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_updated_object_count_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_updated_object_count_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM registered-object projection
        # independently buildable. It isolates the registration counter and
        # its relationship to the accepted update/object ledgers.
        add_executable(umbra_joined_federate_mom_registered_object_count_periodic_catch2
          cpp/tests/joined_federate_mom_registered_object_count_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_registered_object_count_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_registered_object_count_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_registered_object_count_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_registered_object_count_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the joined-federate MOM deleted-object projection independently
        # buildable. It isolates accepted deletion history from live object
        # membership and periodic MOM reflection.
        add_executable(umbra_joined_federate_mom_deleted_object_count_periodic_catch2
          cpp/tests/joined_federate_mom_deleted_object_count_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_deleted_object_count_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_deleted_object_count_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_deleted_object_count_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_deleted_object_count_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receiving-federate MOM removed-object projection
        # independently buildable. It isolates committed Remove Object
        # Instance callbacks from the sender's deletion history and periodic
        # MOM reflection.
        add_executable(umbra_joined_federate_mom_removed_object_count_periodic_catch2
          cpp/tests/joined_federate_mom_removed_object_count_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_removed_object_count_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_removed_object_count_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_removed_object_count_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_removed_object_count_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receiving-federate MOM discovered-object projection
        # independently buildable. It isolates committed Discover Object
        # Instance callbacks, local-delete rediscovery, and periodic MOM
        # reflection.
        add_executable(umbra_joined_federate_mom_discovered_object_count_periodic_catch2
          cpp/tests/joined_federate_mom_discovered_object_count_periodic_catch2.cpp
        )
        target_compile_features(
          umbra_joined_federate_mom_discovered_object_count_periodic_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_joined_federate_mom_discovered_object_count_periodic_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_joined_federate_mom_discovered_object_count_periodic_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_joined_federate_mom_discovered_object_count_periodic_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the official handle-normalization support-services surface
        # independently buildable. It isolates connection/member fences,
        # typed invalid-designator errors, and execution-scoped DDM
        # point-coordinate stability from the broader DDM suites.
        add_executable(umbra_handle_normalization_catch2
          cpp/tests/handle_normalization_catch2.cpp
        )
        target_compile_features(
          umbra_handle_normalization_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_handle_normalization_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_handle_normalization_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_handle_normalization_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the Willing-to-Acquire candidate selection case independently
        # buildable. It isolates the private If Available reservation and its
        # confirmation/notification handoff.
        add_executable(umbra_negotiated_willing_to_acquire_candidate_catch2
          cpp/tests/negotiated_willing_to_acquire_candidate_catch2.cpp
        )
        target_compile_features(
          umbra_negotiated_willing_to_acquire_candidate_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_negotiated_willing_to_acquire_candidate_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_negotiated_willing_to_acquire_candidate_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_negotiated_willing_to_acquire_candidate_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order interaction matrix independently buildable.
        # It isolates active/passive promotion, sender suppression, parameter
        # validation, and callback-time unsubscribe behavior.
        add_executable(umbra_receive_order_interaction_catch2
          cpp/tests/receive_order_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_receive_order_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_receive_order_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_receive_order_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_receive_order_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the receive-order directed-interaction case independently
        # buildable. It isolates target-known routing, declaration fences,
        # sender exclusion, and target-departure callback suppression.
        add_executable(umbra_directed_interaction_known_target_catch2
          cpp/tests/directed_interaction_known_target_catch2.cpp
        )
        target_compile_features(
          umbra_directed_interaction_known_target_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_directed_interaction_known_target_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_directed_interaction_known_target_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_directed_interaction_known_target_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the directed subscription selector matrix independently
        # buildable. It isolates default by-ownership, universal, empty-set,
        # and supplied-class mode changes.
        add_executable(umbra_directed_interaction_subscription_kind_catch2
          cpp/tests/directed_interaction_subscription_kind_catch2.cpp
        )
        target_compile_features(
          umbra_directed_interaction_subscription_kind_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_directed_interaction_subscription_kind_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_directed_interaction_subscription_kind_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_directed_interaction_subscription_kind_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the strict three-dimensional object-attribute overlap case
        # independently buildable. It isolates complete-overlap routing and
        # one-dimension disjoint suppression under HLA_EVOKED.
        add_executable(umbra_three_dimensional_regional_object_attribute_overlap_catch2
          cpp/tests/three_dimensional_regional_object_attribute_overlap_catch2.cpp
        )
        target_compile_features(
          umbra_three_dimensional_regional_object_attribute_overlap_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_three_dimensional_regional_object_attribute_overlap_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_three_dimensional_regional_object_attribute_overlap_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_three_dimensional_regional_object_attribute_overlap_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the partial multi-attribute negotiated-cancellation case
        # independently buildable. It isolates the retained-versus-cancelled
        # member reservation and the single confirmation/notification path.
        add_executable(umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
          cpp/tests/negotiated_divestiture_partial_acquisition_cancellation_catch2.cpp
        )
        target_compile_features(
          umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the pre-delivery cancellation race independently buildable. It
        # isolates stale confirmation suppression and negotiated-state cleanup.
        add_executable(umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
          cpp/tests/negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2.cpp
        )
        target_compile_features(
          umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the negotiated Willing-to-Acquire continuation case
        # independently buildable. It isolates superseding regular acquisition
        # plus cancellation before the next candidate's confirmation.
        add_executable(umbra_negotiated_willing_to_acquire_continuation_catch2
          cpp/tests/negotiated_willing_to_acquire_continuation_catch2.cpp
        )
        target_compile_features(
          umbra_negotiated_willing_to_acquire_continuation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_negotiated_willing_to_acquire_continuation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_negotiated_willing_to_acquire_continuation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_negotiated_willing_to_acquire_continuation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the two-dimensional multi-attribute DDM source-isolation case
        # independently buildable. It proves per-attribute source-region
        # overlap, conjunctive range filtering, and restoration.
        add_executable(umbra_regional_multi_attribute_ddm_catch2
          cpp/tests/regional_multi_attribute_ddm_catch2.cpp
        )
        target_compile_features(
          umbra_regional_multi_attribute_ddm_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_multi_attribute_ddm_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_multi_attribute_ddm_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_multi_attribute_ddm_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the default-region routing case independently buildable. It
        # isolates ordinary/default source fallback from explicit regional
        # association and subscriber declarations.
        add_executable(umbra_default_region_object_routing_catch2
          cpp/tests/default_region_object_routing_catch2.cpp
        )
        target_compile_features(
          umbra_default_region_object_routing_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_default_region_object_routing_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_default_region_object_routing_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_default_region_object_routing_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 region-template lifecycle independently buildable.
        # This isolates official create/commit/delete/range-bound semantics
        # from the large federation-management aggregate, whose unrelated
        # source-integrity failures must not hide this focused evidence.
        add_executable(umbra_region_lifecycle_catch2
          cpp/tests/region_lifecycle_catch2.cpp
        )
        target_compile_features(
          umbra_region_lifecycle_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_region_lifecycle_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_region_lifecycle_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_region_lifecycle_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 Query Attribute Ownership grouping case independently
        # buildable. It isolates owner/unowned result callbacks and queued
        # report nullification from the large federation-management aggregate.
        add_executable(umbra_attribute_ownership_query_catch2
          cpp/tests/attribute_ownership_query_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_query_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_query_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_query_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_query_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the ownership-transfer/update-region cleanup case independently
        # buildable. It isolates the former-owner association boundary from
        # the large federation-management aggregate.
        add_executable(umbra_ownership_transfer_update_region_catch2
          cpp/tests/ownership_transfer_update_region_catch2.cpp
        )
        target_compile_features(
          umbra_ownership_transfer_update_region_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_ownership_transfer_update_region_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_ownership_transfer_update_region_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_ownership_transfer_update_region_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 If Available acquisition path independently buildable.
        # It isolates Willing-to-Acquire state and terminal callbacks from the
        # large federation-management aggregate.
        add_executable(umbra_attribute_ownership_acquisition_if_available_catch2
          cpp/tests/attribute_ownership_acquisition_if_available_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_acquisition_if_available_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_acquisition_if_available_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_acquisition_if_available_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_acquisition_if_available_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 regular acquisition/release-denial path independently
        # buildable. It isolates the owner-release handshake from the aggregate.
        add_executable(umbra_attribute_ownership_acquisition_catch2
          cpp/tests/attribute_ownership_acquisition_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_acquisition_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_acquisition_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_acquisition_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_acquisition_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 unconditional-divestiture/assumption path independently
        # buildable. It isolates ownership release and candidate filtering from
        # the large federation-management aggregate.
        add_executable(umbra_unconditional_attribute_ownership_divestiture_catch2
          cpp/tests/unconditional_attribute_ownership_divestiture_catch2.cpp
        )
        target_compile_features(
          umbra_unconditional_attribute_ownership_divestiture_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_unconditional_attribute_ownership_divestiture_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_unconditional_attribute_ownership_divestiture_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_unconditional_attribute_ownership_divestiture_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 resign-action unconditional-divestiture path
        # independently buildable from the aggregate federation-management test.
        add_executable(umbra_resign_action_unconditional_divestiture_catch2
          cpp/tests/resign_action_unconditional_divestiture_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_unconditional_divestiture_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_unconditional_divestiture_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_unconditional_divestiture_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_unconditional_divestiture_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 Connection Lost automatic-unconditional-divestiture
        # path independently buildable from the aggregate test. It isolates
        # the configured forced-resignation disposition and survivor offer.
        add_executable(umbra_connection_loss_automatic_unconditional_divestiture_catch2
          cpp/tests/connection_loss_automatic_unconditional_divestiture_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_automatic_unconditional_divestiture_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_automatic_unconditional_divestiture_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_automatic_unconditional_divestiture_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_automatic_unconditional_divestiture_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 Connection Lost pending-acquisition cancellation path
        # independently buildable from the aggregate test. It isolates stale
        # owner-release suppression and subsequent survivor eligibility.
        add_executable(umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
          cpp/tests/connection_loss_automatic_cancel_pending_acquisition_catch2.cpp
        )
        target_compile_features(
          umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 resign-action pending-acquisition rejection boundary
        # independently buildable from the aggregate test.
        add_executable(umbra_resign_action_pending_acquisition_rejection_catch2
          cpp/tests/resign_action_pending_acquisition_rejection_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_pending_acquisition_rejection_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_pending_acquisition_rejection_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_pending_acquisition_rejection_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_pending_acquisition_rejection_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 voluntary resign cancellation path independently
        # buildable. It isolates stale regular-acquisition callbacks from the
        # aggregate federation-management test.
        add_executable(umbra_resign_action_cancel_pending_acquisition_catch2
          cpp/tests/resign_action_cancel_pending_acquisition_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_cancel_pending_acquisition_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_cancel_pending_acquisition_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_cancel_pending_acquisition_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_cancel_pending_acquisition_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 negotiated cancellation boundary independently
        # buildable. It covers the stale release and confirmation callbacks
        # selected by a pending negotiated transfer.
        add_executable(umbra_resign_action_cancel_negotiated_pending_catch2
          cpp/tests/resign_action_cancel_negotiated_pending_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_cancel_negotiated_pending_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_cancel_negotiated_pending_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_cancel_negotiated_pending_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_cancel_negotiated_pending_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 If Available cancellation boundary independently
        # buildable. It isolates Willing-to-Acquire cleanup and callback
        # suppression from the aggregate federation-management test.
        add_executable(umbra_resign_action_cancel_if_available_pending_catch2
          cpp/tests/resign_action_cancel_if_available_pending_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_cancel_if_available_pending_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_cancel_if_available_pending_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_cancel_if_available_pending_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_cancel_if_available_pending_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 voluntary DELETE_OBJECTS resignation case
        # independently buildable. It isolates the delete-privilege guard and
        # receive-order Remove Object Instance callback.
        add_executable(umbra_resign_action_delete_objects_catch2
          cpp/tests/resign_action_delete_objects_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_delete_objects_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_delete_objects_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_delete_objects_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_delete_objects_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 final-federate resignation rule independently
        # buildable. It isolates the §4.12.4 directive-two override and the
        # reusable object-name boundary from the aggregate test.
        add_executable(umbra_resign_action_final_federate_catch2
          cpp/tests/resign_action_final_federate_catch2.cpp
        )
        target_compile_features(
          umbra_resign_action_final_federate_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_resign_action_final_federate_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_resign_action_final_federate_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_resign_action_final_federate_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the 2025 FDD update-rate metadata lookup independently
        # buildable. It isolates named-rate/default lookup and subscription
        # state projection from the aggregate federation-management source.
        add_executable(umbra_update_rate_value_catch2
          cpp/tests/update_rate_value_catch2.cpp
        )
        target_compile_features(
          umbra_update_rate_value_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_update_rate_value_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_update_rate_value_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_update_rate_value_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the passive object-attribute subscription case independently
        # buildable. It isolates the 2025 passive-declaration rule from the
        # ordinary and regional discovery/reflection paths.
        add_executable(umbra_passive_object_attribute_subscription_catch2
          cpp/tests/passive_object_attribute_subscription_catch2.cpp
        )
        target_compile_features(
          umbra_passive_object_attribute_subscription_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_passive_object_attribute_subscription_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_passive_object_attribute_subscription_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_passive_object_attribute_subscription_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the non-timestamped Local Delete Object Instance case
        # independently buildable. It isolates local knowledge removal and
        # the FederateOwnsAttributes/OwnershipAcquisitionPending guards.
        add_executable(umbra_local_delete_object_instance_catch2
          cpp/tests/local_delete_object_instance_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_object_instance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_object_instance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_object_instance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_object_instance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the Local Delete Object Instance service-report regression
        # independently buildable. It verifies the filesystem-backed MOM
        # record shape and switch gating for the accepted transition.
        add_executable(umbra_local_delete_object_instance_service_report_catch2
          cpp/tests/local_delete_object_instance_service_report_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_object_instance_service_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_object_instance_service_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_object_instance_service_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_object_instance_service_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the failed Local Delete Object Instance service-report matrix
        # independently buildable. It checks serial ordering and exact failure
        # vocabulary around the accepted local-forget transition.
        add_executable(umbra_local_delete_object_instance_failure_service_report_catch2
          cpp/tests/local_delete_object_instance_failure_service_report_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_object_instance_failure_service_report_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_object_instance_failure_service_report_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_object_instance_failure_service_report_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_object_instance_failure_service_report_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the failed Local Delete Object Instance MOM-interaction matrix
        # independently buildable. It decodes the official report contract
        # under HLA_IMMEDIATE without coupling to the aggregate test binary.
        add_executable(umbra_local_delete_object_instance_failure_service_report_interaction_catch2
          cpp/tests/local_delete_object_instance_failure_service_report_interaction_catch2.cpp
        )
        target_compile_features(
          umbra_local_delete_object_instance_failure_service_report_interaction_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_local_delete_object_instance_failure_service_report_interaction_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_local_delete_object_instance_failure_service_report_interaction_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_local_delete_object_instance_failure_service_report_interaction_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regular acquisition-cancellation ownership slice
        # independently buildable. It isolates the confirmation boundary and
        # stale callback suppression from the broader ownership matrix.
        add_executable(umbra_attribute_ownership_acquisition_cancellation_catch2
          cpp/tests/attribute_ownership_acquisition_cancellation_catch2.cpp
        )
        target_compile_features(
          umbra_attribute_ownership_acquisition_cancellation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_attribute_ownership_acquisition_cancellation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_attribute_ownership_acquisition_cancellation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_attribute_ownership_acquisition_cancellation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the owner-denial cancellation race independently buildable. It
        # exercises the callback-boundary arbitration without broad ownership
        # matrix setup.
        add_executable(umbra_ownership_acquisition_cancellation_race_catch2
          cpp/tests/ownership_acquisition_cancellation_race_catch2.cpp
        )
        target_compile_features(
          umbra_ownership_acquisition_cancellation_race_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_ownership_acquisition_cancellation_race_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_ownership_acquisition_cancellation_race_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_ownership_acquisition_cancellation_race_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the competing transfer cancellation race independently
        # buildable. It proves Divestiture If Wanted wins over a queued
        # cancellation confirmation at the callback boundary.
        add_executable(umbra_ownership_acquisition_cancellation_transfer_race_catch2
          cpp/tests/ownership_acquisition_cancellation_transfer_race_catch2.cpp
        )
        target_compile_features(
          umbra_ownership_acquisition_cancellation_transfer_race_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_ownership_acquisition_cancellation_transfer_race_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_ownership_acquisition_cancellation_transfer_race_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_ownership_acquisition_cancellation_transfer_race_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the ordinary mixed TSO/FQR/TARA/NMRA interaction case
        # independently buildable.  This preserves a focused alternate-time
        # grant lane while the aggregate federation-management source is under
        # recovery.
        add_executable(umbra_timestamped_interaction_mixed_advance_catch2
          cpp/tests/timestamped_interaction_mixed_advance_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_mixed_advance_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_mixed_advance_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_mixed_advance_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_mixed_advance_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the cross-producer timestamp-order case independently
        # buildable. It exercises two publisher queues and two constrained
        # recipient queues without depending on the damaged aggregate source.
        add_executable(umbra_timestamped_interaction_cross_producer_order_catch2
          cpp/tests/timestamped_interaction_cross_producer_order_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_cross_producer_order_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_cross_producer_order_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_cross_producer_order_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_cross_producer_order_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the source-resignation fanout case independently buildable. It
        # proves that already-admitted TSO payloads survive producer departure
        # for each recipient-local queue.
        add_executable(umbra_timestamped_interaction_source_resignation_fanout_catch2
          cpp/tests/timestamped_interaction_source_resignation_fanout_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_source_resignation_fanout_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_source_resignation_fanout_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_source_resignation_fanout_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_source_resignation_fanout_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the ordinary interaction re-enable case independently
        # buildable. It checks that a queued TSO passel belongs to the joined
        # federate lifetime rather than one Time Constrained enablement.
        add_executable(umbra_timestamped_interaction_reenable_catch2
          cpp/tests/timestamped_interaction_reenable_catch2.cpp
        )
        target_compile_features(
          umbra_timestamped_interaction_reenable_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_timestamped_interaction_reenable_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_timestamped_interaction_reenable_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_timestamped_interaction_reenable_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional Auto Provide overlap/suppression case independently
        # buildable. It is a recovered source slice from the aggregate test
        # while that translation unit remains under repair.
        add_executable(umbra_regional_auto_provide_overlap_catch2
          cpp/tests/regional_auto_provide_overlap_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_overlap_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_overlap_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_overlap_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_overlap_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional Auto Provide provider-response case independently
        # buildable. It covers both callback models and the reflected scoped
        # value produced by the owner callback.
        add_executable(umbra_regional_auto_provide_response_catch2
          cpp/tests/regional_auto_provide_response_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_response_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_response_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_response_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_response_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional multi-provider Auto Provide transfer case
        # independently buildable. It verifies ownership handoff followed by
        # one request/reflection from each eligible provider in both callback
        # models.
        add_executable(umbra_regional_auto_provide_multi_provider_catch2
          cpp/tests/regional_auto_provide_multi_provider_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_multi_provider_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_multi_provider_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_multi_provider_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_multi_provider_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the multi-source-region Auto Provide response case independently
        # buildable. It checks source-region separation and one provider request
        # per eligible source attribute in both callback models.
        add_executable(umbra_regional_auto_provide_multi_source_catch2
          cpp/tests/regional_auto_provide_multi_source_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_multi_source_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_multi_source_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_multi_source_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_multi_source_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped regional Auto Provide response case
        # independently buildable. It checks one TSO reflection and retraction
        # lifecycle in both callback models.
        add_executable(umbra_regional_auto_provide_timestamped_response_catch2
          cpp/tests/regional_auto_provide_timestamped_response_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_timestamped_response_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_timestamped_response_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_timestamped_response_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_timestamped_response_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped Auto Provide admission-fence case independently
        # buildable. It verifies a discovery-time MOM switch mutation suppresses
        # provider admission and that re-enabling permits a fresh TSO discovery.
        add_executable(umbra_regional_auto_provide_timestamped_switch_admission_catch2
          cpp/tests/regional_auto_provide_timestamped_switch_admission_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_timestamped_switch_admission_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_timestamped_switch_admission_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_timestamped_switch_admission_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_timestamped_switch_admission_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the timestamped Auto Provide switch-mutation case independently
        # buildable. It verifies an admitted TSO response survives a later
        # switch disable while a re-enabled execution admits fresh discovery.
        add_executable(umbra_regional_auto_provide_timestamped_switch_mutation_catch2
          cpp/tests/regional_auto_provide_timestamped_switch_mutation_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_timestamped_switch_mutation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_timestamped_switch_mutation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_timestamped_switch_mutation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_timestamped_switch_mutation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the non-timestamped Auto Provide admission-fence case
        # independently buildable. It verifies a discovery callback can turn
        # the switch off before provider planning, then re-enable fresh work.
        add_executable(umbra_regional_auto_provide_switch_admission_catch2
          cpp/tests/regional_auto_provide_switch_admission_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_switch_admission_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_switch_admission_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_switch_admission_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_switch_admission_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the regional Auto Provide switch-mutation case independently
        # buildable. It covers MOM switch admission, ownership transfer, and
        # fresh discovery after re-enabling the switch.
        add_executable(umbra_regional_auto_provide_switch_mutation_catch2
          cpp/tests/regional_auto_provide_switch_mutation_catch2.cpp
        )
        target_compile_features(
          umbra_regional_auto_provide_switch_mutation_catch2
          PRIVATE cxx_std_20
        )
        target_include_directories(
          umbra_regional_auto_provide_switch_mutation_catch2
          PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(
          umbra_regional_auto_provide_switch_mutation_catch2
          PRIVATE
            UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
            UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(
          umbra_regional_auto_provide_switch_mutation_catch2
          PRIVATE
            umbra_fom_validation_backend
            umbra::rti
            umbra::fedtime
            umbra::authorizer
            Catch2::Catch2WithMain
        )

        # Keep the small FOM declaration-management slice independently
        # buildable. The aggregate target also contains the large federation-
        # management translation unit, so this target gives the bounded
        # §4.5.5 case a focused compile/run path while that source is under
        # development.
        add_executable(umbra_fom_declaration_management_catch2
          cpp/tests/fom_declaration_management_catch2.cpp
        )
        target_compile_features(umbra_fom_declaration_management_catch2 PRIVATE cxx_std_20)
        target_include_directories(umbra_fom_declaration_management_catch2 PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(umbra_fom_declaration_management_catch2 PRIVATE
          UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
          UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(umbra_fom_declaration_management_catch2 PRIVATE
          umbra_fom_validation_backend
          umbra::rti
          umbra::fedtime
          umbra::authorizer
          Catch2::Catch2WithMain
        )

        # Keep FOM-composition validation independently buildable. The
        # aggregate 2025 executable also contains the large federation-
        # management translation unit; this target lets Annex C and schema
        # reconciliation lanes run without waiting on unrelated service work.
        add_executable(umbra_fom_composer_catch2
          cpp/tests/libxml2_fom_composer_catch2.cpp
        )
        target_compile_features(umbra_fom_composer_catch2 PRIVATE cxx_std_20)
        target_include_directories(umbra_fom_composer_catch2 PRIVATE
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
        )
        target_compile_definitions(umbra_fom_composer_catch2 PRIVATE
          UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
          UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
        )
        target_link_libraries(umbra_fom_composer_catch2 PRIVATE
          umbra_fom_validation_backend
          umbra::rti
          umbra::fedtime
          umbra::authorizer
          Catch2::Catch2WithMain
        )
      endif()
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
    if(TARGET umbra_external_siso_fom_catch2)
      catch_discover_tests(umbra_external_siso_fom_catch2
        TEST_PREFIX "umbra.external_siso_fom_catch2.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    catch_discover_tests(umbra_ieee1516_2025_catch2
      TEST_PREFIX "umbra.ieee1516_2025.catch2."
      ADD_TAGS_AS_LABELS
      # Keep CTest invocation read-only with respect to the discovery manifest.
      # PRE_TEST discovery lets concurrent focused lanes race while they both
      # try to generate the same include file. POST_BUILD generates it once
      # when the Catch2 executable changes, before any lane runs.
      DISCOVERY_MODE POST_BUILD
    )
    if(TARGET umbra_ieee1516_2025_connection_catch2)
      catch_discover_tests(umbra_ieee1516_2025_connection_catch2
        TEST_PREFIX "umbra.ieee1516_2025.connection_catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_process_boundary_private_catch2)
      catch_discover_tests(umbra_process_boundary_private_catch2
        TEST_PREFIX "umbra.process_boundary_private.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_auto_provide_mom_catch2)
      catch_discover_tests(umbra_auto_provide_mom_catch2
        TEST_PREFIX "umbra.auto_provide_mom.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_mom_federate_set_switches_catch2)
      catch_discover_tests(umbra_mom_federate_set_switches_catch2
        TEST_PREFIX "umbra.mom_federate_set_switches.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_writer_failure_catch2)
      catch_discover_tests(umbra_service_report_writer_failure_catch2
        TEST_PREFIX "umbra.service_report_writer_failure.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_auto_provide_disabled_discovery_only_catch2)
      catch_discover_tests(umbra_auto_provide_disabled_discovery_only_catch2
        TEST_PREFIX "umbra.auto_provide_disabled_discovery_only.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_object_instance_name_reservation_catch2)
      catch_discover_tests(umbra_object_instance_name_reservation_catch2
        TEST_PREFIX "umbra.object_instance_name_reservation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_object_instance_named_registration_catch2)
      catch_discover_tests(umbra_object_instance_named_registration_catch2
        TEST_PREFIX "umbra.object_instance_named_registration.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_object_instance_registration_discovery_catch2)
      catch_discover_tests(umbra_object_instance_registration_discovery_catch2
        TEST_PREFIX "umbra.object_instance_registration_discovery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_object_attribute_routing_catch2)
      catch_discover_tests(umbra_regional_object_attribute_routing_catch2
        TEST_PREFIX "umbra.regional_object_attribute_routing.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_zero_dimensional_regional_interaction_catch2)
      catch_discover_tests(umbra_zero_dimensional_regional_interaction_catch2
        TEST_PREFIX "umbra.zero_dimensional_regional_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_default_region_interaction_routing_catch2)
      catch_discover_tests(umbra_default_region_interaction_routing_catch2
        TEST_PREFIX "umbra.default_region_interaction_routing.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_multi_region_interaction_routing_catch2)
      catch_discover_tests(umbra_multi_region_interaction_routing_catch2
        TEST_PREFIX "umbra.multi_region_interaction_routing.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_mixed_region_interaction_validation_catch2)
      catch_discover_tests(umbra_mixed_region_interaction_validation_catch2
        TEST_PREFIX "umbra.mixed_region_interaction_validation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_object_instance_deletion_catch2)
      catch_discover_tests(umbra_object_instance_deletion_catch2
        TEST_PREFIX "umbra.object_instance_deletion.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_receive_order_attribute_update_catch2)
      catch_discover_tests(umbra_receive_order_attribute_update_catch2
        TEST_PREFIX "umbra.receive_order_attribute_update.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2)
      catch_discover_tests(umbra_delay_subscription_evaluation_timestamped_directed_interaction_catch2
        TEST_PREFIX "umbra.delay_subscription_evaluation_timestamped_directed_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_delay_subscription_evaluation_directed_interaction_catch2)
      catch_discover_tests(umbra_delay_subscription_evaluation_directed_interaction_catch2
        TEST_PREFIX "umbra.delay_subscription_evaluation_directed_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_file_confirm_synchronization_point_registration_catch2)
      catch_discover_tests(umbra_service_report_file_confirm_synchronization_point_registration_catch2
        TEST_PREFIX "umbra.service_report_file_confirm_synchronization_point_registration.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_file_announce_synchronization_point_catch2)
      catch_discover_tests(umbra_service_report_file_announce_synchronization_point_catch2
        TEST_PREFIX "umbra.service_report_file_announce_synchronization_point.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_file_federation_synchronized_catch2)
      catch_discover_tests(umbra_service_report_file_federation_synchronized_catch2
        TEST_PREFIX "umbra.service_report_file_federation_synchronized.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_file_synchronization_point_achieved_catch2)
      catch_discover_tests(umbra_service_report_file_synchronization_point_achieved_catch2
        TEST_PREFIX "umbra.service_report_file_synchronization_point_achieved.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_service_report_file_federation_synchronized_resignation_catch2)
      catch_discover_tests(umbra_service_report_file_federation_synchronized_resignation_catch2
        TEST_PREFIX "umbra.service_report_file_federation_synchronized_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_timestamped_attribute_catch2)
      catch_discover_tests(umbra_local_delete_timestamped_attribute_catch2
        TEST_PREFIX "umbra.local_delete_timestamped_attribute.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_timestamped_object_removal_catch2)
      catch_discover_tests(umbra_local_delete_timestamped_object_removal_catch2
        TEST_PREFIX "umbra.local_delete_timestamped_object_removal.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_source_resignation_catch2)
      catch_discover_tests(umbra_timestamped_attribute_source_resignation_catch2
        TEST_PREFIX "umbra.timestamped_attribute_source_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_source_resignation_fanout_catch2)
      catch_discover_tests(umbra_timestamped_attribute_source_resignation_fanout_catch2
        TEST_PREFIX "umbra.timestamped_attribute_source_resignation_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_ownership_transfer_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_ownership_transfer_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_ownership_transfer.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_type_control_catch2)
      catch_discover_tests(umbra_custom_transportation_type_control_catch2
        TEST_PREFIX "umbra.custom_transportation_type_control.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_delivery_catch2)
      catch_discover_tests(umbra_custom_transportation_delivery_catch2
        TEST_PREFIX "umbra.custom_transportation_delivery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_ordinary_regional_interaction_catch2)
      catch_discover_tests(umbra_custom_transportation_ordinary_regional_interaction_catch2
        TEST_PREFIX "umbra.custom_transportation_ordinary_regional_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_ordinary_regional_attribute_catch2)
      catch_discover_tests(umbra_custom_transportation_ordinary_regional_attribute_catch2
        TEST_PREFIX "umbra.custom_transportation_ordinary_regional_attribute.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_timestamped_delivery_catch2)
      catch_discover_tests(umbra_custom_transportation_timestamped_delivery_catch2
        TEST_PREFIX "umbra.custom_transportation_timestamped_delivery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_timestamped_directed_delivery_catch2)
      catch_discover_tests(umbra_custom_transportation_timestamped_directed_delivery_catch2
        TEST_PREFIX "umbra.custom_transportation_timestamped_directed_delivery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2)
      catch_discover_tests(umbra_custom_transportation_timestamped_regional_attribute_delivery_catch2
        TEST_PREFIX "umbra.custom_transportation_timestamped_regional_attribute_delivery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_mom_transportation_type_change_request_catch2)
      catch_discover_tests(umbra_mom_transportation_type_change_request_catch2
        TEST_PREFIX "umbra.mom_transportation_type_change_request.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_multi_recipient_source_resignation_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_multi_recipient_source_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_tar_nmr_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_tar_nmr_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_tar_nmr.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_federation_mom_save_conditionals_catch2)
      catch_discover_tests(umbra_federation_mom_save_conditionals_catch2
        TEST_PREFIX "umbra.federation_mom_save_conditionals.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_federation_mom_current_fdd_catch2)
      catch_discover_tests(umbra_federation_mom_current_fdd_catch2
        TEST_PREFIX "umbra.federation_mom_current_fdd.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_galt_lits_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_galt_lits_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_galt_lits_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_tso_length_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_tso_length_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_tso_length_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_time_state_duration_catch2)
      catch_discover_tests(umbra_joined_federate_mom_time_state_duration_catch2
        TEST_PREFIX "umbra.joined_federate_mom_time_state_duration.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_reflection_count_catch2)
      catch_discover_tests(umbra_joined_federate_mom_reflection_count_catch2
        TEST_PREFIX "umbra.joined_federate_mom_reflection_count.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_updates_sent_catch2)
      catch_discover_tests(umbra_joined_federate_mom_updates_sent_catch2
        TEST_PREFIX "umbra.joined_federate_mom_updates_sent.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_object_instances_updated_report_catch2)
      catch_discover_tests(umbra_joined_federate_mom_object_instances_updated_report_catch2
        TEST_PREFIX "umbra.joined_federate_mom_object_instances_updated_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2)
      catch_discover_tests(umbra_joined_federate_mom_object_instances_that_can_be_deleted_report_catch2
        TEST_PREFIX "umbra.joined_federate_mom_object_instances_that_can_be_deleted_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_object_instances_reflected_report_catch2)
      catch_discover_tests(umbra_joined_federate_mom_object_instances_reflected_report_catch2
        TEST_PREFIX "umbra.joined_federate_mom_object_instances_reflected_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_object_instance_information_report_catch2)
      catch_discover_tests(umbra_joined_federate_mom_object_instance_information_report_catch2
        TEST_PREFIX "umbra.joined_federate_mom_object_instance_information_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_reflections_received_catch2)
      catch_discover_tests(umbra_joined_federate_mom_reflections_received_catch2
        TEST_PREFIX "umbra.joined_federate_mom_reflections_received.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_interactions_received_catch2)
      catch_discover_tests(umbra_joined_federate_mom_interactions_received_catch2
        TEST_PREFIX "umbra.joined_federate_mom_interactions_received.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_directed_interactions_received_catch2)
      catch_discover_tests(umbra_joined_federate_mom_directed_interactions_received_catch2
        TEST_PREFIX "umbra.joined_federate_mom_directed_interactions_received.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_interactions_sent_catch2)
      catch_discover_tests(umbra_joined_federate_mom_interactions_sent_catch2
        TEST_PREFIX "umbra.joined_federate_mom_interactions_sent.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_mom_sender_count_reports_null_buckets_catch2)
      catch_discover_tests(umbra_mom_sender_count_reports_null_buckets_catch2
        TEST_PREFIX "umbra.mom_sender_count_reports_null_buckets.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2)
      catch_discover_tests(umbra_joined_federate_mom_removed_object_count_tso_retraction_catch2
        TEST_PREFIX "umbra.joined_federate_mom_removed_object_count_tso_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_allow_relaxed_ddm_object_attribute_catch2)
      catch_discover_tests(umbra_allow_relaxed_ddm_object_attribute_catch2
        TEST_PREFIX "umbra.allow_relaxed_ddm_object_attribute.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_reenable_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_reenable_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_reenable.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_source_resignation_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_source_resignation_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_source_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_retraction_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_retraction_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_directed_interaction_immediate_source_resignation_catch2)
      catch_discover_tests(umbra_timestamped_directed_interaction_immediate_source_resignation_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_immediate_source_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_send_interaction_retraction_catch2)
      catch_discover_tests(umbra_timestamped_send_interaction_retraction_catch2
        TEST_PREFIX "umbra.timestamped_send_interaction_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_send_interaction_no_fanout_catch2)
      catch_discover_tests(umbra_timestamped_send_interaction_no_fanout_catch2
        TEST_PREFIX "umbra.timestamped_send_interaction_no_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_no_fanout_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_no_fanout_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_no_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_queued_passel_retraction_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_queued_passel_retraction_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_queued_passel_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_tso_designator_terminalization_catch2)
      catch_discover_tests(umbra_timestamped_tso_designator_terminalization_catch2
        TEST_PREFIX "umbra.timestamped_tso_designator_terminalization.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_regulation_reenable_catch2)
      catch_discover_tests(umbra_timestamped_interaction_regulation_reenable_catch2
        TEST_PREFIX "umbra.timestamped_interaction_regulation_reenable.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_regional_attribute_update_mixed_advance_catch2)
      catch_discover_tests(umbra_timestamped_regional_attribute_update_mixed_advance_catch2
        TEST_PREFIX "umbra.timestamped_regional_attribute_update_mixed_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_attribute_update_mixed_advance_catch2)
      catch_discover_tests(umbra_timestamped_default_region_attribute_update_mixed_advance_catch2
        TEST_PREFIX "umbra.timestamped_default_region_attribute_update_mixed_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2)
      catch_discover_tests(umbra_timestamped_default_region_attribute_update_retract_alternate_advance_catch2
        TEST_PREFIX "umbra.timestamped_default_region_attribute_update_retract_alternate_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2)
      catch_discover_tests(umbra_timestamped_regional_attribute_update_retract_mixed_advance_catch2
        TEST_PREFIX "umbra.timestamped_regional_attribute_update_retract_mixed_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_regional_attribute_update_tar_nmr_catch2)
      catch_discover_tests(umbra_timestamped_regional_attribute_update_tar_nmr_catch2
        TEST_PREFIX "umbra.timestamped_regional_attribute_update_tar_nmr.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_regional_attribute_update_resignation_catch2)
      catch_discover_tests(umbra_timestamped_regional_attribute_update_resignation_catch2
        TEST_PREFIX "umbra.timestamped_regional_attribute_update_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_regional_attribute_update_mixed_fanout_catch2)
      catch_discover_tests(umbra_timestamped_regional_attribute_update_mixed_fanout_catch2
        TEST_PREFIX "umbra.timestamped_regional_attribute_update_mixed_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_best_effort_timestamped_attribute_rate_catch2)
      catch_discover_tests(umbra_regional_best_effort_timestamped_attribute_rate_catch2
        TEST_PREFIX "umbra.regional_best_effort_timestamped_attribute_rate.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_best_effort_attribute_rate_catch2)
      catch_discover_tests(umbra_regional_best_effort_attribute_rate_catch2
        TEST_PREFIX "umbra.regional_best_effort_attribute_rate.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_available_advance_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_available_advance_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_available_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_rate_reduction_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_rate_reduction_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_rate_reduction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_reenable_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_reenable_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_reenable.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_alternate_advance_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_alternate_advance_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_alternate_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_order_cohort_catch2)
      catch_discover_tests(umbra_timestamped_attribute_order_cohort_catch2
        TEST_PREFIX "umbra.timestamped_attribute_order_cohort.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2)
      catch_discover_tests(umbra_timestamped_default_region_interaction_retract_alternate_advance_catch2
        TEST_PREFIX "umbra.timestamped_default_region_interaction_retract_alternate_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2)
      catch_discover_tests(umbra_timestamped_interaction_regulation_reenable_changed_lookahead_catch2
        TEST_PREFIX "umbra.timestamped_interaction_regulation_reenable_changed_lookahead.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_directed_reenable_changed_lookahead_catch2)
      catch_discover_tests(umbra_tso_directed_reenable_changed_lookahead_catch2
        TEST_PREFIX "umbra.timestamped_directed_interaction_regulation_reenable_changed_lookahead.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_regulation_reenable_changed_lookahead_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_regulation_reenable_changed_lookahead.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2)
      catch_discover_tests(umbra_timed_multi_recipient_regional_attribute_restore_negotiated_cancel_catch2
        TEST_PREFIX "umbra.timed_multi_recipient_regional_attribute_restore_negotiated_cancel.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_regional_negotiated_cancel_restore_catch2)
      catch_discover_tests(umbra_tso_regional_negotiated_cancel_restore_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_cancel_pending_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_regional_negotiated_continuation_restore_catch2)
      catch_discover_tests(umbra_tso_regional_negotiated_continuation_restore_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_continuation_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_regional_regular_continuation_restore_catch2)
      catch_discover_tests(umbra_tso_regional_regular_continuation_restore_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_regular_candidate_continuation_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_regional_mixed_continuation_restore_catch2)
      catch_discover_tests(umbra_tso_regional_mixed_continuation_restore_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_mixed_candidate_continuation_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_mixed_pre_delivery_cancel_catch2)
      catch_discover_tests(umbra_tso_mixed_pre_delivery_cancel_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_pre_delivery_cancel_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_mixed_confirmation_cancel_catch2)
      catch_discover_tests(umbra_tso_mixed_confirmation_cancel_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_retained_regular_confirmation_cancel_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2)
      catch_discover_tests(umbra_tso_regional_negotiated_confirmation_cancel_restore_catch2
        TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_multi_recipient_negotiated_confirmation_cancel_after_restore.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_attribute_update_flush_queue_catch2)
      catch_discover_tests(umbra_timestamped_attribute_update_flush_queue_catch2
        TEST_PREFIX "umbra.timestamped_attribute_update_flush_queue.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_flush_queue_future_input_catch2)
      catch_discover_tests(umbra_flush_queue_future_input_catch2
        TEST_PREFIX "umbra.flush_queue_future_input.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_request_retraction_timestamped_deletion_no_fanout_catch2)
      catch_discover_tests(umbra_request_retraction_timestamped_deletion_no_fanout_catch2
        TEST_PREFIX "umbra.request_retraction_timestamped_deletion_no_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_object_deletion_retraction_catch2)
      catch_discover_tests(umbra_timestamped_object_deletion_retraction_catch2
        TEST_PREFIX "umbra.timestamped_object_deletion_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_object_deletion_joined_owner_retraction_catch2)
      catch_discover_tests(umbra_timestamped_object_deletion_joined_owner_retraction_catch2
        TEST_PREFIX "umbra.timestamped_object_deletion_joined_owner_retraction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_flush_queue_request_optimistic_time_catch2)
      catch_discover_tests(umbra_flush_queue_request_optimistic_time_catch2
        TEST_PREFIX "umbra.flush_queue_request_optimistic_time.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_terminal_timestamped_deletion_tombstone_catch2)
      catch_discover_tests(umbra_terminal_timestamped_deletion_tombstone_catch2
        TEST_PREFIX "umbra.terminal_timestamped_deletion_tombstone.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_object_deletion_source_resignation_fanout_catch2)
      catch_discover_tests(umbra_timestamped_object_deletion_source_resignation_fanout_catch2
        TEST_PREFIX "umbra.timestamped_object_deletion_source_resignation_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_next_message_request_queued_tso_catch2)
      catch_discover_tests(umbra_next_message_request_queued_tso_catch2
        TEST_PREFIX "umbra.next_message_request_queued_tso.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_available_time_advance_inclusive_galt_catch2)
      catch_discover_tests(umbra_available_time_advance_inclusive_galt_catch2
        TEST_PREFIX "umbra.available_time_advance_inclusive_galt.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2)
      catch_discover_tests(umbra_connection_loss_attribute_update_tso_suppressed_cleanup_catch2
        TEST_PREFIX "umbra.connection_loss_attribute_update_tso_suppressed_cleanup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2)
      catch_discover_tests(umbra_connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient_catch2
        TEST_PREFIX "umbra.connection_loss_attribute_update_tso_automatic_cleanup_multi_recipient.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2)
      catch_discover_tests(umbra_connection_loss_attribute_update_tso_automatic_cleanup_catch2
        TEST_PREFIX "umbra.connection_loss_attribute_update_tso_automatic_cleanup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_directed_selector_mutation_catch2)
      catch_discover_tests(umbra_connection_loss_directed_selector_mutation_catch2
        TEST_PREFIX "umbra.connection_loss_directed_selector_mutation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2)
      catch_discover_tests(umbra_connection_loss_attribute_update_tso_asynchronous_delivery_catch2
        TEST_PREFIX "umbra.connection_loss_attribute_update_tso_asynchronous_delivery.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_source_resignation_catch2)
      catch_discover_tests(umbra_timestamped_interaction_source_resignation_catch2
        TEST_PREFIX "umbra.timestamped_interaction_source_resignation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_attribute_update_catch2)
      catch_discover_tests(umbra_timestamped_default_region_attribute_update_catch2
        TEST_PREFIX "umbra.timestamped_default_region_attribute_update.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_attribute_reenable_catch2)
      catch_discover_tests(umbra_timestamped_default_region_attribute_reenable_catch2
        TEST_PREFIX "umbra.timestamped_default_region_attribute_reenable.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_default_region_attribute_mixed_fanout_catch2)
      catch_discover_tests(umbra_timestamped_default_region_attribute_mixed_fanout_catch2
        TEST_PREFIX "umbra.timestamped_default_region_attribute_mixed_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2)
      catch_discover_tests(umbra_attribute_ownership_divestiture_if_wanted_mixed_acquirer_catch2
        TEST_PREFIX "umbra.attribute_ownership_divestiture_if_wanted_mixed_acquirer.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_divestiture_if_wanted_pending_catch2)
      catch_discover_tests(umbra_attribute_ownership_divestiture_if_wanted_pending_catch2
        TEST_PREFIX "umbra.attribute_ownership_divestiture_if_wanted_pending.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_negotiated_attribute_ownership_divestiture_pending_catch2)
      catch_discover_tests(umbra_negotiated_attribute_ownership_divestiture_pending_catch2
        TEST_PREFIX "umbra.negotiated_attribute_ownership_divestiture_pending.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_mom_service_reporting_interlock_catch2)
      catch_discover_tests(umbra_mom_service_reporting_interlock_catch2
        TEST_PREFIX "umbra.mom_service_reporting_interlock.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_deletable_object_count_catch2)
      catch_discover_tests(umbra_joined_federate_mom_deletable_object_count_catch2
        TEST_PREFIX "umbra.joined_federate_mom_deletable_object_count.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_ro_length_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_ro_length_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_ro_length_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_updates_sent_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_updates_sent_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_updates_sent_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_updated_object_count_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_updated_object_count_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_updated_object_count_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_registered_object_count_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_registered_object_count_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_registered_object_count_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_deleted_object_count_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_deleted_object_count_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_deleted_object_count_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_removed_object_count_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_removed_object_count_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_removed_object_count_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_joined_federate_mom_discovered_object_count_periodic_catch2)
      catch_discover_tests(umbra_joined_federate_mom_discovered_object_count_periodic_catch2
        TEST_PREFIX "umbra.joined_federate_mom_discovered_object_count_periodic.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_handle_normalization_catch2)
      catch_discover_tests(umbra_handle_normalization_catch2
        TEST_PREFIX "umbra.handle_normalization.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_negotiated_willing_to_acquire_candidate_catch2)
      catch_discover_tests(umbra_negotiated_willing_to_acquire_candidate_catch2
        TEST_PREFIX "umbra.negotiated_willing_to_acquire_candidate.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_receive_order_interaction_catch2)
      catch_discover_tests(umbra_receive_order_interaction_catch2
        TEST_PREFIX "umbra.receive_order_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_directed_interaction_known_target_catch2)
      catch_discover_tests(umbra_directed_interaction_known_target_catch2
        TEST_PREFIX "umbra.directed_interaction_known_target.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_directed_interaction_subscription_kind_catch2)
      catch_discover_tests(umbra_directed_interaction_subscription_kind_catch2
        TEST_PREFIX "umbra.directed_interaction_subscription_kind.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_three_dimensional_regional_object_attribute_overlap_catch2)
      catch_discover_tests(umbra_three_dimensional_regional_object_attribute_overlap_catch2
        TEST_PREFIX "umbra.three_dimensional_regional_object_attribute_overlap.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2)
      catch_discover_tests(umbra_negotiated_divestiture_partial_acquisition_cancellation_catch2
        TEST_PREFIX "umbra.negotiated_divestiture_partial_acquisition_cancellation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2)
      catch_discover_tests(umbra_negotiated_divestiture_pre_delivery_acquisition_cancellation_catch2
        TEST_PREFIX "umbra.negotiated_divestiture_pre_delivery_acquisition_cancellation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_negotiated_willing_to_acquire_continuation_catch2)
      catch_discover_tests(umbra_negotiated_willing_to_acquire_continuation_catch2
        TEST_PREFIX "umbra.negotiated_willing_to_acquire_continuation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_multi_attribute_ddm_catch2)
      catch_discover_tests(umbra_regional_multi_attribute_ddm_catch2
        TEST_PREFIX "umbra.regional_multi_attribute_ddm.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_default_region_object_routing_catch2)
      catch_discover_tests(umbra_default_region_object_routing_catch2
        TEST_PREFIX "umbra.default_region_object_routing.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_region_lifecycle_catch2)
      catch_discover_tests(umbra_region_lifecycle_catch2
        TEST_PREFIX "umbra.region_lifecycle.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_query_catch2)
      catch_discover_tests(umbra_attribute_ownership_query_catch2
        TEST_PREFIX "umbra.attribute_ownership_query.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_ownership_transfer_update_region_catch2)
      catch_discover_tests(umbra_ownership_transfer_update_region_catch2
        TEST_PREFIX "umbra.ownership_transfer_update_region.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_acquisition_if_available_catch2)
      catch_discover_tests(umbra_attribute_ownership_acquisition_if_available_catch2
        TEST_PREFIX "umbra.attribute_ownership_acquisition_if_available.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_acquisition_catch2)
      catch_discover_tests(umbra_attribute_ownership_acquisition_catch2
        TEST_PREFIX "umbra.attribute_ownership_acquisition.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_unconditional_attribute_ownership_divestiture_catch2)
      catch_discover_tests(umbra_unconditional_attribute_ownership_divestiture_catch2
        TEST_PREFIX "umbra.unconditional_attribute_ownership_divestiture.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_unconditional_divestiture_catch2)
      catch_discover_tests(umbra_resign_action_unconditional_divestiture_catch2
        TEST_PREFIX "umbra.resign_action_unconditional_divestiture.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_automatic_unconditional_divestiture_catch2)
      catch_discover_tests(umbra_connection_loss_automatic_unconditional_divestiture_catch2
        TEST_PREFIX "umbra.connection_loss_automatic_unconditional_divestiture.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_connection_loss_automatic_cancel_pending_acquisition_catch2)
      catch_discover_tests(umbra_connection_loss_automatic_cancel_pending_acquisition_catch2
        TEST_PREFIX "umbra.connection_loss_automatic_cancel_pending_acquisition.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_pending_acquisition_rejection_catch2)
      catch_discover_tests(umbra_resign_action_pending_acquisition_rejection_catch2
        TEST_PREFIX "umbra.resign_action_pending_acquisition_rejection.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_cancel_pending_acquisition_catch2)
      catch_discover_tests(umbra_resign_action_cancel_pending_acquisition_catch2
        TEST_PREFIX "umbra.resign_action_cancel_pending_acquisition.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_cancel_negotiated_pending_catch2)
      catch_discover_tests(umbra_resign_action_cancel_negotiated_pending_catch2
        TEST_PREFIX "umbra.resign_action_cancel_negotiated_pending.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_cancel_if_available_pending_catch2)
      catch_discover_tests(umbra_resign_action_cancel_if_available_pending_catch2
        TEST_PREFIX "umbra.resign_action_cancel_if_available_pending.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_delete_objects_catch2)
      catch_discover_tests(umbra_resign_action_delete_objects_catch2
        TEST_PREFIX "umbra.resign_action_delete_objects.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_resign_action_final_federate_catch2)
      catch_discover_tests(umbra_resign_action_final_federate_catch2
        TEST_PREFIX "umbra.resign_action_final_federate.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_update_rate_value_catch2)
      catch_discover_tests(umbra_update_rate_value_catch2
        TEST_PREFIX "umbra.update_rate_value.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_passive_object_attribute_subscription_catch2)
      catch_discover_tests(umbra_passive_object_attribute_subscription_catch2
        TEST_PREFIX "umbra.passive_object_attribute_subscription.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_object_instance_catch2)
      catch_discover_tests(umbra_local_delete_object_instance_catch2
        TEST_PREFIX "umbra.local_delete_object_instance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_object_instance_service_report_catch2)
      catch_discover_tests(umbra_local_delete_object_instance_service_report_catch2
        TEST_PREFIX "umbra.local_delete_object_instance_service_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_object_instance_failure_service_report_catch2)
      catch_discover_tests(umbra_local_delete_object_instance_failure_service_report_catch2
        TEST_PREFIX "umbra.local_delete_object_instance_failure_service_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_local_delete_object_instance_failure_service_report_interaction_catch2)
      catch_discover_tests(umbra_local_delete_object_instance_failure_service_report_interaction_catch2
        TEST_PREFIX "umbra.local_delete_object_instance_failure_service_report_interaction.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_attribute_ownership_acquisition_cancellation_catch2)
      catch_discover_tests(umbra_attribute_ownership_acquisition_cancellation_catch2
        TEST_PREFIX "umbra.attribute_ownership_acquisition_cancellation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_ownership_acquisition_cancellation_race_catch2)
      catch_discover_tests(umbra_ownership_acquisition_cancellation_race_catch2
        TEST_PREFIX "umbra.ownership_acquisition_cancellation_race.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_ownership_acquisition_cancellation_transfer_race_catch2)
      catch_discover_tests(umbra_ownership_acquisition_cancellation_transfer_race_catch2
        TEST_PREFIX "umbra.ownership_acquisition_cancellation_transfer_race.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_mixed_advance_catch2)
      catch_discover_tests(umbra_timestamped_interaction_mixed_advance_catch2
        TEST_PREFIX "umbra.timestamped_interaction_mixed_advance.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_cross_producer_order_catch2)
      catch_discover_tests(umbra_timestamped_interaction_cross_producer_order_catch2
        TEST_PREFIX "umbra.timestamped_interaction_cross_producer_order.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_source_resignation_fanout_catch2)
      catch_discover_tests(umbra_timestamped_interaction_source_resignation_fanout_catch2
        TEST_PREFIX "umbra.timestamped_interaction_source_resignation_fanout.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_timestamped_interaction_reenable_catch2)
      catch_discover_tests(umbra_timestamped_interaction_reenable_catch2
        TEST_PREFIX "umbra.timestamped_interaction_reenable.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_overlap_catch2)
      catch_discover_tests(umbra_regional_auto_provide_overlap_catch2
        TEST_PREFIX "umbra.regional_auto_provide_overlap.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_response_catch2)
      catch_discover_tests(umbra_regional_auto_provide_response_catch2
        TEST_PREFIX "umbra.regional_auto_provide_response.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_multi_provider_catch2)
      catch_discover_tests(umbra_regional_auto_provide_multi_provider_catch2
        TEST_PREFIX "umbra.regional_auto_provide_multi_provider.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_multi_source_catch2)
      catch_discover_tests(umbra_regional_auto_provide_multi_source_catch2
        TEST_PREFIX "umbra.regional_auto_provide_multi_source.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_timestamped_response_catch2)
      catch_discover_tests(umbra_regional_auto_provide_timestamped_response_catch2
        TEST_PREFIX "umbra.regional_auto_provide_timestamped_response.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_timestamped_switch_admission_catch2)
      catch_discover_tests(umbra_regional_auto_provide_timestamped_switch_admission_catch2
        TEST_PREFIX "umbra.regional_auto_provide_timestamped_switch_admission.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_timestamped_switch_mutation_catch2)
      catch_discover_tests(umbra_regional_auto_provide_timestamped_switch_mutation_catch2
        TEST_PREFIX "umbra.regional_auto_provide_timestamped_switch_mutation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_switch_admission_catch2)
      catch_discover_tests(umbra_regional_auto_provide_switch_admission_catch2
        TEST_PREFIX "umbra.regional_auto_provide_switch_admission.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_regional_auto_provide_switch_mutation_catch2)
      catch_discover_tests(umbra_regional_auto_provide_switch_mutation_catch2
        TEST_PREFIX "umbra.regional_auto_provide_switch_mutation.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_fom_declaration_management_catch2)
      catch_discover_tests(umbra_fom_declaration_management_catch2
        TEST_PREFIX "umbra.fom_declaration_management.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_fom_composer_catch2)
      catch_discover_tests(umbra_fom_composer_catch2
        TEST_PREFIX "umbra.fom_composer.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()
    if(TARGET umbra_federation_registry_catch2)
      catch_discover_tests(umbra_federation_registry_catch2
        TEST_PREFIX "umbra.federation_registry.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the legacy wire boundary runnable even when a large unrelated
    # development translation unit is being edited. The same source remains
    # in the aggregate Catch2 target so the normal regression executable still
    # covers it.
    add_executable(umbra_fom_wire_codec_catch2
      cpp/tests/fom_wire_codec_catch2.cpp
    )
    target_compile_features(umbra_fom_wire_codec_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_fom_wire_codec_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_link_libraries(umbra_fom_wire_codec_catch2 PRIVATE
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_fom_wire_codec_catch2
      TEST_PREFIX "umbra.fom_wire_codec.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # This target intentionally includes only the neutral FOM descriptor and
    # the public 2025 RTI. It is the compile/link/run guard that the standard
    # RTI path cannot accidentally depend on the RPR codec adapter.
    add_executable(umbra_rpr_standard_isolation_catch2
      cpp/tests/rpr_standard_isolation_catch2.cpp
    )
    target_compile_features(umbra_rpr_standard_isolation_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_rpr_standard_isolation_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_link_libraries(umbra_rpr_standard_isolation_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_rpr_standard_isolation_catch2
      TEST_PREFIX "umbra.rpr.standard_isolation.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the durable service-report store independently buildable.  The
    # aggregate 2025 Catch2 executable also contains the large federation-
    # management translation unit; an isolated target preserves a fast,
    # standards-facing filesystem-store loop when an unrelated aggregate test
    # is being edited.
    add_executable(umbra_service_report_store_catch2
      cpp/tests/service_report_store_catch2.cpp
    )
    target_compile_features(umbra_service_report_store_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_service_report_store_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_service_report_store_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_service_report_store_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_service_report_store_catch2
      TEST_PREFIX "umbra.service_report_store.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )
    add_custom_target(umbra_service_report_store_junit
      COMMAND ${CMAKE_COMMAND} -E make_directory
        "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/service-report-store"
      COMMAND $<TARGET_FILE:umbra_service_report_store_catch2>
        "[service-report-store]"
        --allow-running-no-tests
        --reporter junit
        --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/service-report-store/service-report-store.xml"
      DEPENDS umbra_service_report_store_catch2
      COMMENT "Writing service-report-store Catch2 JUnit artifact"
      VERBATIM
    )

    # Keep the public joined-federate MOM report-file projection independently
    # buildable.  This is a deliberately small object-management lane: it
    # proves the official HLAreportServiceFile attribute through discovery,
    # reflection, and a requested-value replay without depending on the large
    # federation-management aggregate translation unit.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_joined_federate_mom_report_file_catch2
        cpp/tests/joined_federate_mom_report_file_catch2.cpp
      )
      target_compile_features(umbra_joined_federate_mom_report_file_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_joined_federate_mom_report_file_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_joined_federate_mom_report_file_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_joined_federate_mom_report_file_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_joined_federate_mom_report_file_catch2
        TEST_PREFIX "umbra.joined_federate_mom_report_file.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the public durable application-value restart contract independently
    # buildable.  The aggregate federation-management source contains the
    # historical scenario, while this focused target provides a clean source
    # pointer, exact CTest filter, and fast filesystem/re-hydration loop.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_public_process_restart_application_value_catch2
        cpp/tests/public_process_restart_application_value_catch2.cpp
      )
      target_compile_features(umbra_public_process_restart_application_value_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_public_process_restart_application_value_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_public_process_restart_application_value_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_public_process_restart_application_value_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_public_process_restart_application_value_catch2
        TEST_PREFIX "umbra.public_process_restart_application_value.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the object-class handle/name lookup contract independently
    # buildable.  The aggregate federation-management translation unit also
    # contains this case, but the focused target preserves a fast lookup lane
    # while unrelated aggregate tests are under development.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_object_class_lookup_catch2
        cpp/tests/object_class_lookup_catch2.cpp
      )
      target_compile_features(umbra_object_class_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_object_class_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_object_class_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_object_class_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_object_class_lookup_catch2
        TEST_PREFIX "umbra.object_class_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the interaction-class handle/name lookup contract independently
    # buildable alongside the object-class lookup lane.  This prevents a
    # focused Support Services check from depending on the aggregate
    # federation-management translation unit.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_interaction_class_lookup_catch2
        cpp/tests/interaction_class_lookup_catch2.cpp
      )
      target_compile_features(umbra_interaction_class_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_interaction_class_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_interaction_class_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_interaction_class_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_interaction_class_lookup_catch2
        TEST_PREFIX "umbra.interaction_class_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the inherited attribute handle/name lookup contract independently
    # buildable with the same official-header and embedded-profile boundary.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_attribute_lookup_catch2
        cpp/tests/attribute_lookup_catch2.cpp
      )
      target_compile_features(umbra_attribute_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_attribute_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_attribute_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_attribute_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_attribute_lookup_catch2
        TEST_PREFIX "umbra.attribute_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the inherited parameter handle/name lookup contract independently
    # buildable with the same official-header and embedded-profile boundary.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_parameter_lookup_catch2
        cpp/tests/parameter_lookup_catch2.cpp
      )
      target_compile_features(umbra_parameter_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_parameter_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_parameter_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_parameter_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_parameter_lookup_catch2
        TEST_PREFIX "umbra.parameter_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the dimension metadata/lookup contract independently buildable.
    # This focused DDM-foundation lane preserves fast feedback for FOM
    # hierarchy, stable handles, available-dimension sets, and upper bounds
    # without requiring the aggregate federation-management executable.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_dimension_lookup_catch2
        cpp/tests/dimension_lookup_catch2.cpp
      )
      target_compile_features(umbra_dimension_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_dimension_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_dimension_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_dimension_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_dimension_lookup_catch2
        TEST_PREFIX "umbra.dimension_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the mandatory transportation-type lookup pair independently
    # buildable.  This Support Services lane verifies the official
    # HLAreliable/HLAbestEffort handles and names without compiling the
    # aggregate federation-management translation unit.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_transportation_type_lookup_catch2
        cpp/tests/transportation_type_lookup_catch2.cpp
      )
      target_compile_features(umbra_transportation_type_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_transportation_type_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_transportation_type_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_transportation_type_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_transportation_type_lookup_catch2
        TEST_PREFIX "umbra.transportation_type_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the later-join transportation-handle stability case independently
    # buildable.  This FOM-composition slice exercises immutable execution
    # directory identity without compiling the aggregate federation-management
    # translation unit.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_custom_transportation_handle_stability_catch2
        cpp/tests/custom_transportation_handle_stability_catch2.cpp
      )
      target_compile_features(umbra_custom_transportation_handle_stability_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_custom_transportation_handle_stability_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_custom_transportation_handle_stability_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_custom_transportation_handle_stability_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_custom_transportation_handle_stability_catch2
        TEST_PREFIX "umbra.custom_transportation_handle_stability.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the mandatory Receive/TimeStamp order-type lookup pair independently
    # buildable.  The aggregate federation-management translation unit also
    # contains this case, but the focused target keeps Support Services lookup
    # feedback isolated from order-control and delivery behavior.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_order_type_lookup_catch2
        cpp/tests/order_type_lookup_catch2.cpp
      )
      target_compile_features(umbra_order_type_lookup_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_order_type_lookup_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_order_type_lookup_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_order_type_lookup_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_order_type_lookup_catch2
        TEST_PREFIX "umbra.order_type_lookup.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the order-control state transition case independently buildable.
    # This isolates class defaults, per-instance overrides, and publisher-
    # scoped interaction order from the aggregate time-management executable.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_order_type_control_catch2
        cpp/tests/order_type_control_catch2.cpp
      )
      target_compile_features(umbra_order_type_control_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_order_type_control_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_order_type_control_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_order_type_control_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_order_type_control_catch2
        TEST_PREFIX "umbra.order_type_control.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep ordinary declaration-relevance advisories independently buildable.
    # This isolates active/passive publication and subscription transitions
    # from regional declaration and service-report delivery cases.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_declaration_relevance_advisory_catch2
        cpp/tests/declaration_relevance_advisory_catch2.cpp
      )
      target_compile_features(umbra_declaration_relevance_advisory_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_declaration_relevance_advisory_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_declaration_relevance_advisory_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_declaration_relevance_advisory_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_declaration_relevance_advisory_catch2
        TEST_PREFIX "umbra.declaration_relevance_advisory.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep regional declaration-relevance advisories independently buildable.
    # This isolates active/passive region-scoped declarations from ordinary
    # relevance and from full regional DDM delivery.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_regional_declaration_relevance_advisory_catch2
        cpp/tests/regional_declaration_relevance_advisory_catch2.cpp
      )
      target_compile_features(umbra_regional_declaration_relevance_advisory_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_regional_declaration_relevance_advisory_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_regional_declaration_relevance_advisory_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_regional_declaration_relevance_advisory_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_regional_declaration_relevance_advisory_catch2
        TEST_PREFIX "umbra.regional_declaration_relevance_advisory.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep federation-list callback reports independently runnable so the
    # Support Services listing boundary does not require the aggregate suite.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_federation_listing_catch2
        cpp/tests/federation_listing_catch2.cpp
      )
      target_compile_features(umbra_federation_listing_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_federation_listing_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_federation_listing_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_federation_listing_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_federation_listing_catch2
        TEST_PREFIX "umbra.federation_listing.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep declaration-relevance service-report ordering independently
    # buildable. This preserves the filesystem-backed report seam while
    # separating durability-before-callback assertions from advisory routing.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_declaration_relevance_service_report_catch2
        cpp/tests/declaration_relevance_service_report_catch2.cpp
      )
      target_compile_features(umbra_declaration_relevance_service_report_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_declaration_relevance_service_report_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_declaration_relevance_service_report_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
        UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      )
      target_link_libraries(umbra_declaration_relevance_service_report_catch2 PRIVATE
        umbra_fom_validation_backend
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_declaration_relevance_service_report_catch2
        TEST_PREFIX "umbra.declaration_relevance_service_report.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the embedded regional Attribute Relevance Advisory regression
    # independently buildable.  The aggregate 2025 Catch2 executable also
    # contains the large federation-management translation unit; this focused
    # target preserves a fast regional advisory lane while that file is under
    # unrelated development.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_attribute_relevance_advisory_catch2
        cpp/tests/attribute_relevance_advisory_catch2.cpp
      )
      target_compile_features(umbra_attribute_relevance_advisory_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_attribute_relevance_advisory_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_attribute_relevance_advisory_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_attribute_relevance_advisory_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_attribute_relevance_advisory_catch2
        TEST_PREFIX "umbra.attribute_relevance_advisory.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
      add_custom_target(umbra_attribute_relevance_advisory_junit
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/regional-attribute-relevance"
        COMMAND $<TARGET_FILE:umbra_attribute_relevance_advisory_catch2>
          "[regional-attribute-relevance]"
          --allow-running-no-tests
          --reporter junit
          --out "${UMBRA_COMPLIANCE_ARTIFACT_DIRECTORY}/regional-attribute-relevance/regional-attribute-relevance.xml"
        DEPENDS umbra_attribute_relevance_advisory_catch2
        COMMENT "Writing regional-attribute-relevance Catch2 JUnit artifact"
        VERBATIM
      )
    endif()

    # Keep the ordinary Attribute Relevance Advisory rate-reissue contract
    # independently buildable.  This lane isolates active-maximum changes
    # from the broader regional and public-process advisory suites.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_attribute_relevance_rate_reissue_catch2
        cpp/tests/attribute_relevance_rate_reissue_catch2.cpp
      )
      target_compile_features(umbra_attribute_relevance_rate_reissue_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_attribute_relevance_rate_reissue_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_attribute_relevance_rate_reissue_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_attribute_relevance_rate_reissue_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_attribute_relevance_rate_reissue_catch2
        TEST_PREFIX "umbra.attribute_relevance_rate_reissue.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the regional Attribute Relevance Advisory rate-designator contract
    # independently buildable.  This lane isolates source-region overlap and
    # restoration from the ordinary active-maximum case.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_regional_attribute_relevance_rate_designator_catch2
        cpp/tests/regional_attribute_relevance_rate_designator_catch2.cpp
      )
      target_compile_features(umbra_regional_attribute_relevance_rate_designator_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_regional_attribute_relevance_rate_designator_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_regional_attribute_relevance_rate_designator_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_regional_attribute_relevance_rate_designator_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_regional_attribute_relevance_rate_designator_catch2
        TEST_PREFIX "umbra.regional_attribute_relevance_rate_designator.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the known-class-disabled Attribute Relevance Advisory contract
    # independently buildable.  This lane isolates the static policy's
    # registered-class subscription walk from its enabled counterpart.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_attribute_relevance_known_class_disabled_subscription_catch2
        cpp/tests/attribute_relevance_known_class_disabled_subscription_catch2.cpp
      )
      target_compile_features(umbra_attribute_relevance_known_class_disabled_subscription_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_attribute_relevance_known_class_disabled_subscription_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_attribute_relevance_known_class_disabled_subscription_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_attribute_relevance_known_class_disabled_subscription_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_attribute_relevance_known_class_disabled_subscription_catch2
        TEST_PREFIX "umbra.attribute_relevance_known_class_disabled_subscription.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep the known-class-enabled Attribute Relevance Advisory contract
    # independently buildable.  This lane isolates the static policy's
    # known-class subscription walk from its disabled counterpart.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_attribute_relevance_known_class_enabled_subscription_catch2
        cpp/tests/attribute_relevance_known_class_enabled_subscription_catch2.cpp
      )
      target_compile_features(umbra_attribute_relevance_known_class_enabled_subscription_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_attribute_relevance_known_class_enabled_subscription_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_attribute_relevance_known_class_enabled_subscription_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_attribute_relevance_known_class_enabled_subscription_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_attribute_relevance_known_class_enabled_subscription_catch2
        TEST_PREFIX "umbra.attribute_relevance_known_class_enabled_subscription.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep passive regional update-rate lookup independently buildable.  This
    # lane isolates active-rate reduction from update delivery and advisory
    # transition evidence.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_update_rate_passive_regional_subscription_catch2
        cpp/tests/update_rate_passive_regional_subscription_catch2.cpp
      )
      target_compile_features(umbra_update_rate_passive_regional_subscription_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_update_rate_passive_regional_subscription_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_update_rate_passive_regional_subscription_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_update_rate_passive_regional_subscription_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_update_rate_passive_regional_subscription_catch2
        TEST_PREFIX "umbra.update_rate_passive_regional_subscription.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

    # Keep mixed ordinary update-rate subscription gating independently
    # buildable. This lane proves per-attribute reduction without widening
    # into timestamped, regional, or advisory evidence.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    add_executable(umbra_mixed_update_rate_subscriptions_catch2
      cpp/tests/mixed_update_rate_subscriptions_catch2.cpp
    )
      target_compile_features(umbra_mixed_update_rate_subscriptions_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_mixed_update_rate_subscriptions_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_mixed_update_rate_subscriptions_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_mixed_update_rate_subscriptions_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_mixed_update_rate_subscriptions_catch2
        TEST_PREFIX "umbra.mixed_update_rate_subscriptions.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )

    # Keep federation-teardown update-rate-history isolation independently
    # buildable. This is the next bounded row after mixed per-attribute gating.
    add_executable(umbra_federation_teardown_update_rate_history_catch2
      cpp/tests/federation_teardown_update_rate_history_catch2.cpp
    )
    target_compile_features(umbra_federation_teardown_update_rate_history_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_federation_teardown_update_rate_history_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_federation_teardown_update_rate_history_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_federation_teardown_update_rate_history_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_federation_teardown_update_rate_history_catch2
      TEST_PREFIX "umbra.federation_teardown_update_rate_history.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the multi-recipient default-region save/restore regression
    # independently buildable. The aggregate federation-management source is
    # intentionally left intact for historical coverage, while this focused
    # target keeps the restore/retraction ledger lane runnable in isolation.
    add_executable(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2
      cpp/tests/restore_live_tso_default_region_attribute_update_multi_recipient_catch2.cpp
    )
    target_compile_features(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_restore_live_tso_default_region_attribute_update_multi_recipient_catch2
      TEST_PREFIX "umbra.restore_live_tso_default_region_attribute_update_multi_recipient.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the timed explicit-source regional-interaction restore case
    # independently buildable.  Its aggregate declaration remains useful for
    # historical coverage, while this target keeps the timed save-boundary
    # queue/retraction lane runnable without the aggregate translation unit.
    add_executable(umbra_timed_restore_live_tso_regional_interaction_catch2
      cpp/tests/timed_restore_live_tso_regional_interaction_catch2.cpp
    )
    target_compile_features(umbra_timed_restore_live_tso_regional_interaction_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_timed_restore_live_tso_regional_interaction_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_timed_restore_live_tso_regional_interaction_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_timed_restore_live_tso_regional_interaction_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_timed_restore_live_tso_regional_interaction_catch2
      TEST_PREFIX "umbra.timed_restore_live_tso_regional_interaction.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the timestamped regional-interaction source-resignation regression
    # independently buildable.  The aggregate declaration remains useful for
    # historical coverage, while this focused target keeps the membership and
    # queued-delivery lane runnable without the aggregate translation unit.
    add_executable(umbra_timed_live_tso_regional_interaction_source_resignation_catch2
      cpp/tests/timed_live_tso_regional_interaction_source_resignation_catch2.cpp
    )
    target_compile_features(umbra_timed_live_tso_regional_interaction_source_resignation_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_timed_live_tso_regional_interaction_source_resignation_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_timed_live_tso_regional_interaction_source_resignation_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_timed_live_tso_regional_interaction_source_resignation_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_timed_live_tso_regional_interaction_source_resignation_catch2
      TEST_PREFIX "umbra.timed_live_tso_regional_interaction_source_resignation.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the live regional multi-recipient restore regression independently
    # buildable.  The aggregate declaration remains historical coverage, while
    # this target makes the fan-out/retraction ledger slice directly runnable.
    add_executable(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2
      cpp/tests/restore_live_tso_regional_interaction_multi_recipient_catch2.cpp
    )
    target_compile_features(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_restore_live_tso_regional_interaction_multi_recipient_catch2
      TEST_PREFIX "umbra.restore_live_tso_regional_interaction_multi_recipient.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the timed regional-attribute source-resignation-after-restore case
    # independently buildable. The aggregate declaration remains historical
    # coverage; this target gives the saved source-region/queue lifetime slice
    # an exact compile, run, and CTest handle.
    add_executable(umbra_timed_regional_attr_source_resign_restore_catch2
      cpp/tests/timed_live_tso_regional_attribute_update_source_resignation_after_restore_catch2.cpp
    )
    target_compile_features(umbra_timed_regional_attr_source_resign_restore_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_timed_regional_attr_source_resign_restore_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_timed_regional_attr_source_resign_restore_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_timed_regional_attr_source_resign_restore_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_timed_regional_attr_source_resign_restore_catch2
      TEST_PREFIX "umbra.timed_live_tso_regional_attribute_update_source_resignation_after_restore.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )

    # Keep the timed explicit-source regional-attribute restore baseline
    # independently buildable. This is the save-boundary companion to the
    # source-resignation-after-restore lane above.
    add_executable(umbra_timed_restore_regional_attr_catch2
      cpp/tests/timed_restore_live_tso_regional_attribute_update_catch2.cpp
    )
    target_compile_features(umbra_timed_restore_regional_attr_catch2 PRIVATE cxx_std_20)
    target_include_directories(umbra_timed_restore_regional_attr_catch2 PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
    )
    target_compile_definitions(umbra_timed_restore_regional_attr_catch2 PRIVATE
      UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(umbra_timed_restore_regional_attr_catch2 PRIVATE
      umbra::rti
      Catch2::Catch2WithMain
    )
    catch_discover_tests(umbra_timed_restore_regional_attr_catch2
      TEST_PREFIX "umbra.timed_restore_live_tso_regional_attribute_update.catch2."
      ADD_TAGS_AS_LABELS
      DISCOVERY_MODE POST_BUILD
    )
    endif()

    # Keep synchronization-point service coverage independently buildable.
    # The aggregate federation-management translation unit is intentionally
    # kept separate while unrelated development cases are repaired.
    if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
      add_executable(umbra_synchronization_point_catch2
        cpp/tests/synchronization_point_catch2.cpp
      )
      target_compile_features(umbra_synchronization_point_catch2 PRIVATE cxx_std_20)
      target_include_directories(umbra_synchronization_point_catch2 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
      )
      target_compile_definitions(umbra_synchronization_point_catch2 PRIVATE
        UMBRA_SOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}"
      )
      target_link_libraries(umbra_synchronization_point_catch2 PRIVATE
        umbra::rti
        Catch2::Catch2WithMain
      )
      catch_discover_tests(umbra_synchronization_point_catch2
        TEST_PREFIX "umbra.synchronization_point.catch2."
        ADD_TAGS_AS_LABELS
        DISCOVERY_MODE POST_BUILD
      )
    endif()

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

    # Emit a bounded JUnit artifact for the active process-boundary lane.  It
    # intentionally uses the exact Catch2 tag rather than the broad
    # compliance selector, so the artifact remains aligned with the roadmap's
    # mapped process-boundary edit/build/test slice.
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
  else()
    message(STATUS "Catch2 v3 not found; configure with -DUMBRA_FETCH_CATCH2=ON or provide Catch2::Catch2WithMain")
  endif()
endif()

install(TARGETS ieee1516_2025_headers ieee1516_2010_headers
  EXPORT umbra_rti_targets
)
install(TARGETS umbra_rti umbra_rti_2010 umbra_fedtime umbra_authorizer
  EXPORT umbra_rti_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  install(TARGETS umbra_fom_validation_backend
    EXPORT umbra_rti_targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endif()
install(DIRECTORY "${UMBRA_IEEE1516_2025_INCLUDE_DIR}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cpp/include/umbra/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/umbra
)
install(FILES third_party/ieee1516.1-2025/NOTICE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/umbra_rti
)
install(
  DIRECTORY "${UMBRA_IEEE1516_2010_INCLUDE_DIR}/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/umbra/ieee1516.1-2010
)
install(FILES third_party/ieee1516.1-2010/NOTICE
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
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/umbra_rti-profile.json"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/umbra_rti
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
