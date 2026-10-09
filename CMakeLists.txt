cmake_minimum_required(VERSION 3.23)

project(
  umbra_rti
  VERSION 0.1.0
  DESCRIPTION "A native C++ HLA RTI"
  LANGUAGES CXX
)

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # Match MSVC's /bigobj allowance for the standards-shaped, high-fanout
  # dispatch translation units when emitting MinGW COFF object files.
  add_compile_options(-Wa,-mbig-obj)
endif()

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
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # The vendored 1516.1-2010 headers use Microsoft's _int64 spelling under
  # _WIN32; MinGW needs the equivalent standard type for this header boundary.
  target_compile_definitions(ieee1516_2010_headers INTERFACE "_int64=long long")
endif()

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
  cpp/src/internal/federation/process_federation_service_region_management.cpp
  cpp/src/internal/federation/process_federation_service_regional_object_association.cpp
  cpp/src/internal/federation/process_federation_service_transportation.cpp
  cpp/src/internal/federation/process_federation_service_declarations.cpp
  cpp/src/internal/federation/process_federation_service_ddm_switches.cpp
  cpp/src/internal/federation/process_federation_service_request_handlers.cpp
  cpp/src/internal/federation/process_federation_service_object_instance_deletion.cpp
  cpp/src/internal/federation/process_federation_service_dimension_queries.cpp
  cpp/src/internal/federation/process_federation_service_dispatch.cpp
  cpp/src/internal/federation/process_federation_service_connection_loss.cpp
  cpp/src/internal/federation/process_federation_service_codecs.cpp
  cpp/src/internal/federation/process_federation_service_control_codecs.cpp
  cpp/src/internal/federation/process_federation_service_time_codecs.cpp
  cpp/src/internal/federation/process_federation_service_reporting_codecs.cpp
  cpp/src/internal/federation/process_federation_service_ownership_codecs.cpp
  cpp/src/internal/federation/process_federation_service_declaration_codecs.cpp
  cpp/src/internal/federation/process_federation_service_lookup_codecs.cpp
  cpp/src/internal/federation/process_federation_service_object_instance_codecs.cpp
  cpp/src/internal/federation/process_federation_service_dimension_region_codecs.cpp
  cpp/src/internal/federation/process_federation_service_interaction_event_codecs.cpp
  cpp/src/internal/federation/process_federation_service_retraction.cpp
  cpp/src/internal/federation/process_federation_service_receive.cpp
  cpp/src/internal/federation/process_federation_service_reporting.cpp
  cpp/src/internal/federation/process_federation_service_interaction_send.cpp
  cpp/src/internal/federation/process_federation_service_handle_lookup.cpp
  cpp/src/internal/federation/process_federation_service_lifecycle.cpp
  cpp/src/internal/federation/process_federation_service_save_restore.cpp
  cpp/src/internal/federation/process_federation_service_time_management.cpp
  cpp/src/internal/federation/process_federation_service_tso_delivery.cpp
  cpp/src/internal/federation/process_federation_service_notifications.cpp
  cpp/src/internal/federation/process_federation_service_attribute_relevance_advisories.cpp
  cpp/src/internal/federation/process_federation_service_attribute_ownership_delivery.cpp
  cpp/src/internal/federation/process_federation_service_attribute_value_updates.cpp
  cpp/src/internal/federation/process_federation_service_attribute_update_requests.cpp
  cpp/src/internal/federation/process_federation_service_ownership_handlers.cpp
  cpp/src/internal/federation/process_federation_service_tso_payload_dispatch.cpp
  cpp/src/internal/federation/process_federation_service_object_discovery.cpp
  cpp/src/internal/federation/process_federation_service_object_instance_removal.cpp
  cpp/src/internal/federation/process_federation_callback_bridge.cpp
  cpp/src/internal/federation/process_federation_client.cpp
  cpp/src/internal/federation/process_federation_client_federation_management.cpp
  cpp/src/internal/federation/process_federation_client_time_services.cpp
  cpp/src/internal/federation/process_federation_client_declaration_services.cpp
  cpp/src/internal/federation/process_federation_client_object_services.cpp
  cpp/src/internal/federation/process_federation_client_region_services.cpp
  cpp/src/internal/federation/process_federation_client_value_services.cpp
  cpp/src/internal/federation/process_federation_client_callback_dispatch.cpp
  cpp/src/internal/federation/process_federation_client_handle_lookup.cpp
  cpp/src/internal/federation/process_federation_client_ownership.cpp
  cpp/src/internal/federation/process_transport_session.cpp
  cpp/src/internal/federation/transport_protocol.cpp
  cpp/src/internal/federation/transport_service_protocol.cpp
  cpp/src/internal/handles/attribute_handle_directory.cpp
  cpp/src/internal/handles/dimension_handle_directory.cpp
  cpp/src/internal/federation/federate_lifecycle.cpp
  cpp/src/internal/time/federate_time_state.cpp
    cpp/src/internal/federation/federation_management_coordinator.cpp
    cpp/src/internal/federation/federation_registry.cpp cpp/src/internal/federation/federation_registry_control_state.cpp cpp/src/internal/federation/federation_registry_time_tso_enqueue.cpp cpp/src/internal/federation/federation_registry_tso_delivery.cpp cpp/src/internal/federation/federation_registry_regions.cpp cpp/src/internal/federation/federation_registry_catalog_queries.cpp cpp/src/internal/federation/federation_registry_attribute_relevance.cpp cpp/src/internal/federation/federation_registry_directed_interactions.cpp cpp/src/internal/federation/federation_registry_object_attribute_declarations.cpp cpp/src/internal/federation/federation_registry_object_instance_registration.cpp cpp/src/internal/federation/federation_registry_update_region_associations.cpp cpp/src/internal/federation/federation_registry_object_instance_deletion.cpp cpp/src/internal/federation/federation_registry_object_instance_discovery_planning.cpp cpp/src/internal/federation/federation_registry_object_discovery_routes.cpp cpp/src/internal/federation/federation_registry_object_instance_removal.cpp cpp/src/internal/federation/federation_registry_object_instance_discovery_state.cpp cpp/src/internal/federation/federation_registry_attribute_value_update_requests.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition_if_available.cpp cpp/src/internal/federation/federation_registry_negotiated_attribute_ownership_divestiture.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_pending_work.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_acquisition.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_assumptions.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_divestiture_if_wanted.cpp cpp/src/internal/federation/federation_registry_attribute_ownership_cancellation.cpp cpp/src/internal/federation/federation_registry_receive_order_interactions.cpp cpp/src/internal/federation/federation_registry_mom_service_reporting.cpp cpp/src/internal/federation/federation_registry_federate_lost_reporting.cpp cpp/src/internal/federation/federation_registry_mom_exception_reporting.cpp cpp/src/internal/federation/federation_registry_mom_object_instances_updated_reporting.cpp cpp/src/internal/federation/federation_registry_mom_object_instances_that_can_be_deleted_reporting.cpp cpp/src/internal/federation/federation_registry_mom_object_instances_reflected_reporting.cpp cpp/src/internal/federation/federation_registry_mom_object_instance_information_reporting.cpp cpp/src/internal/federation/federation_registry_mom_fom_module_data_reporting.cpp cpp/src/internal/federation/federation_registry_mom_federation_content_reporting.cpp cpp/src/internal/federation/federation_registry_mom_synchronization_reporting.cpp cpp/src/internal/federation/federation_registry_mom_publications_reporting.cpp cpp/src/internal/federation/federation_registry_mom_subscriptions_reporting.cpp cpp/src/internal/federation/federation_registry_mom_updates_sent_reporting.cpp cpp/src/internal/federation/federation_registry_mom_interactions_sent_reporting.cpp cpp/src/internal/federation/federation_registry_mom_directed_interactions_sent_reporting.cpp cpp/src/internal/federation/federation_registry_mom_interactions_received_reporting.cpp cpp/src/internal/federation/federation_registry_mom_directed_interactions_received_reporting.cpp cpp/src/internal/federation/federation_registry_mom_reflections_received_reporting.cpp cpp/src/internal/federation/federation_registry_attribute_transportation_type_change_planning.cpp cpp/src/internal/federation/federation_registry_transportation_order_services.cpp
    cpp/src/internal/federation/federation_state_image.cpp
    cpp/src/internal/federation/federation_state_image_decode.cpp
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
  cpp/src/internal/time/tso_message_queue.cpp cpp/src/internal/federation/federation_registry_state_image_validation.cpp cpp/src/internal/federation/federation_registry_membership_lifecycle.cpp cpp/src/internal/federation/federation_registry_resign_lifecycle.cpp cpp/src/internal/federation/federation_registry_state_image_capture.cpp cpp/src/internal/federation/federation_registry_state_image_restore.cpp cpp/src/internal/federation/federation_registry_restore_declarations.cpp cpp/src/internal/federation/federation_registry_restore_ownership_ledgers.cpp cpp/src/internal/federation/federation_registry_restore_tso_payloads.cpp cpp/src/internal/federation/federation_registry_state_image_restore_helpers.cpp cpp/src/internal/federation/federation_registry_value_helpers.cpp
  cpp/src/internal/runtime/umbra_rti_ambassador.cpp
  cpp/src/internal/runtime/umbra_rti_ambassador_service_failure_translation.cpp
  cpp/src/internal/runtime/umbra_rti_ambassador_connect_services.cpp
  cpp/src/internal/runtime/umbra_rti_ambassador_callback_control_services.cpp
  cpp/src/internal/runtime/ambassador_connection_support.cpp
  cpp/src/internal/runtime/rti_initialization_data.cpp
  cpp/src/internal/runtime/utf8_string.cpp
)
target_sources(umbra_rti PRIVATE
  cpp/src/internal/federation/federation_registry_save_control.cpp
  cpp/src/internal/federation/federation_registry_restore_control.cpp
  cpp/src/internal/federation/federation_registry_mom_object_state.cpp
  cpp/src/internal/federation/federation_registry_attribute_update_recipients.cpp
  cpp/src/internal/federation/federation_registry_state_image_interaction_validation.cpp
  cpp/src/internal/federation/federation_registry_state_image_ownership_validation.cpp
)
include(cmake/umbra_rti_ambassador_embedded_sources.cmake)
add_library(umbra::rti ALIAS umbra_rti)
set_target_properties(umbra_rti PROPERTIES EXPORT_NAME rti)
target_compile_features(umbra_rti PUBLIC cxx_std_20)
target_compile_definitions(umbra_rti PUBLIC STATIC_RTI)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # The unmodified IEEE 1516.1-2025 encoding header uses Microsoft's _int64
  # keyword for every _WIN32 compiler. Keep the licensed header baseline exact
  # and supply its MinGW spelling through the public CMake target instead.
  target_compile_definitions(umbra_rti PUBLIC "_int64=long long")
endif()
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
  target_link_libraries(umbra_rti PRIVATE ws2_32 advapi32)
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
set_target_properties(umbra_rti_2010 PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)
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

include(cmake/fom_validation_backend.cmake)

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
  target_include_directories(umbra_ieee1516_2010_composite_encoding_smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
  )
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

  set_target_properties(
    umbra_ieee1516_2010_binding_shell_smoke
    umbra_ieee1516_2010_headers_smoke
    umbra_ieee1516_2010_encoding_smoke
    umbra_ieee1516_2010_composite_encoding_smoke
    umbra_ieee1516_2010_integer_time_smoke
    umbra_ieee1516_2010_float_time_smoke
    umbra_ieee1516_2010_time_marshal_smoke
    PROPERTIES
      CXX_STANDARD 14
      CXX_STANDARD_REQUIRED YES
      CXX_EXTENSIONS NO
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
    # 2025 Requirements Lab contract tests; 2010 registrations remain separate.
    include(cmake/ieee1516_2025_requirements_lab_ctests.cmake)
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
    include(cmake/catch2_executable_targets.cmake)
    include(cmake/catch2_ctest_lanes.cmake)
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
