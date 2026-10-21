cmake_minimum_required(VERSION 3.23)

project(umbra_rti_package_smoke LANGUAGES CXX)

find_package(umbra_rti CONFIG REQUIRED)
include("${CMAKE_CURRENT_LIST_DIR}/abi_policy.cmake")

if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES)
  foreach(_umbra_target IN ITEMS umbra::rti umbra::fedtime umbra::authorizer)
    get_target_property(_umbra_target_type ${_umbra_target} TYPE)
    if(NOT _umbra_target_type STREQUAL "SHARED_LIBRARY")
      message(FATAL_ERROR
        "The shared-library package profile requires ${_umbra_target} to be an imported shared library; "
        "got '${_umbra_target_type}'."
      )
    endif()
  endforeach()
endif()

add_executable(umbra_rti_package_smoke main.cpp)
target_compile_features(umbra_rti_package_smoke PRIVATE cxx_std_20)
target_link_libraries(umbra_rti_package_smoke PRIVATE
  umbra::rti
  umbra::fedtime
  umbra::authorizer
)

get_filename_component(UMBRA_PACKAGE_RESTAURANT_FOM
  "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml"
  ABSOLUTE)
get_filename_component(UMBRA_PACKAGE_SWITCH_FOM
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/data/switch-support-enabled-fom.xml"
  ABSOLUTE)
get_filename_component(UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/data/attribute-update-passel-fom.xml"
  ABSOLUTE)
add_executable(umbra_rti_package_federation_restart_smoke federation_restart_smoke.cpp)
target_compile_features(umbra_rti_package_federation_restart_smoke PRIVATE cxx_std_20)
target_compile_definitions(umbra_rti_package_federation_restart_smoke PRIVATE
  UMBRA_PACKAGE_RESTAURANT_FOM="${UMBRA_PACKAGE_RESTAURANT_FOM}"
  UMBRA_PACKAGE_SWITCH_FOM="${UMBRA_PACKAGE_SWITCH_FOM}"
)
target_link_libraries(umbra_rti_package_federation_restart_smoke PRIVATE
  umbra::rti
  umbra::fedtime
)

if(UMBRA_PACKAGE_PROCESS_SMOKE)
  if(NOT DEFINED UMBRA_PROCESS_SERVICE_PROBE_PATH OR
     "${UMBRA_PROCESS_SERVICE_PROBE_PATH}" STREQUAL "")
    message(FATAL_ERROR
      "UMBRA_PROCESS_SERVICE_PROBE_PATH is required for the package process smoke")
  endif()
  add_executable(umbra_rti_package_process_smoke process_smoke.cpp)
  target_compile_features(umbra_rti_package_process_smoke PRIVATE cxx_std_20)
  target_compile_definitions(umbra_rti_package_process_smoke PRIVATE
    UMBRA_PROCESS_SERVICE_PROBE_PATH="${UMBRA_PROCESS_SERVICE_PROBE_PATH}"
  )
  target_link_libraries(umbra_rti_package_process_smoke PRIVATE
    umbra::rti
    umbra::fedtime
    umbra::authorizer
  )
endif()

if(DEFINED UMBRA_PACKAGE_CATCH2_SOURCE_DIRECTORY AND
   EXISTS "${UMBRA_PACKAGE_CATCH2_SOURCE_DIRECTORY}/CMakeLists.txt")
  set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
  add_subdirectory(
    "${UMBRA_PACKAGE_CATCH2_SOURCE_DIRECTORY}"
    "${CMAKE_CURRENT_BINARY_DIR}/catch2"
    EXCLUDE_FROM_ALL
  )
elseif(DEFINED UMBRA_PACKAGE_CATCH2_DIR AND
       NOT "${UMBRA_PACKAGE_CATCH2_DIR}" STREQUAL "")
  set(Catch2_DIR "${UMBRA_PACKAGE_CATCH2_DIR}" CACHE PATH "" FORCE)
  find_package(Catch2 3 CONFIG REQUIRED)
endif()

if(UMBRA_PACKAGE_SUBSCRIPTION_DISCOVERY_SMOKE AND
   TARGET Catch2::Catch2WithMain)
  add_executable(umbra_rti_package_subscribe_discovery_catch2
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/package_discovery_catch2.cpp
  )
  target_compile_features(umbra_rti_package_subscribe_discovery_catch2 PRIVATE cxx_std_20)
  target_compile_definitions(umbra_rti_package_subscribe_discovery_catch2 PRIVATE
    UMBRA_PACKAGE_RESTAURANT_FOM="${UMBRA_PACKAGE_RESTAURANT_FOM}"
  )
  target_link_libraries(umbra_rti_package_subscribe_discovery_catch2 PRIVATE
    umbra::rti
    Catch2::Catch2WithMain
  )
  add_executable(umbra_rti_package_attribute_filter_catch2
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/package_attribute_filter_catch2.cpp
  )
  target_compile_features(umbra_rti_package_attribute_filter_catch2 PRIVATE cxx_std_20)
  target_compile_definitions(umbra_rti_package_attribute_filter_catch2 PRIVATE
    UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM="${UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM}"
  )
  target_link_libraries(umbra_rti_package_attribute_filter_catch2 PRIVATE
    umbra::rti
    Catch2::Catch2WithMain
  )
  add_executable(umbra_rti_package_subscription_validity_catch2
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/package_subscription_validity_catch2.cpp
  )
  target_compile_features(umbra_rti_package_subscription_validity_catch2 PRIVATE cxx_std_20)
  target_compile_definitions(umbra_rti_package_subscription_validity_catch2 PRIVATE
    UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM="${UMBRA_PACKAGE_ATTRIBUTE_FILTER_FOM}"
  )
  target_link_libraries(umbra_rti_package_subscription_validity_catch2 PRIVATE
    umbra::rti
    Catch2::Catch2WithMain
  )
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME umbra_rti_package_consumer COMMAND umbra_rti_package_smoke)
  if(TARGET umbra_rti_package_subscribe_discovery_catch2)
    add_test(
      NAME umbra_rti_package_subscribe_discovery_consumer
      COMMAND umbra_rti_package_subscribe_discovery_catch2
    )
    set_tests_properties(umbra_rti_package_subscribe_discovery_consumer
      PROPERTIES LABELS "installable-package;subscribe-object-class-attributes-discovery"
    )
  endif()
  if(TARGET umbra_rti_package_attribute_filter_catch2)
    add_test(
      NAME umbra_rti_package_attribute_filter_consumer
      COMMAND umbra_rti_package_attribute_filter_catch2
    )
    set_tests_properties(umbra_rti_package_attribute_filter_consumer
      PROPERTIES LABELS "installable-package;subscribe-object-class-attributes-value-filter"
    )
  endif()
  if(TARGET umbra_rti_package_subscription_validity_catch2)
    add_test(
      NAME umbra_rti_package_subscription_validity_consumer
      COMMAND umbra_rti_package_subscription_validity_catch2
    )
    set_tests_properties(umbra_rti_package_subscription_validity_consumer
      PROPERTIES LABELS "installable-package;subscribe-object-class-attributes-attribute-set-validity"
    )
  endif()
  add_test(
    NAME umbra_rti_package_federation_restart_consumer
    COMMAND umbra_rti_package_federation_restart_smoke
  )
  set_tests_properties(umbra_rti_package_federation_restart_consumer
    PROPERTIES LABELS "package-save-restore-restart;public-process-restart-attribute-transportation-type-change;public-process-restart-interaction-transportation-type-change"
  )
  if(UMBRA_PACKAGE_PROCESS_SMOKE)
    add_test(
      NAME umbra_rti_package_process_consumer
      COMMAND umbra_rti_package_process_smoke
    )
    set_tests_properties(umbra_rti_package_process_consumer
      PROPERTIES LABELS package-process
    )
    add_test(
      NAME umbra_rti_package_process_timestamped_consumer
      COMMAND umbra_rti_package_process_smoke timestamped
    )
    set_tests_properties(umbra_rti_package_process_timestamped_consumer
      PROPERTIES LABELS package-process-timestamped
    )
    add_test(
      NAME umbra_rti_package_process_parameterized_consumer
      COMMAND umbra_rti_package_process_smoke parameterized
    )
    set_tests_properties(umbra_rti_package_process_parameterized_consumer
      PROPERTIES LABELS package-process-parameterized
    )
    add_test(
      NAME umbra_rti_package_process_connection_loss_consumer
      COMMAND umbra_rti_package_process_smoke connection-loss
    )
    set_tests_properties(umbra_rti_package_process_connection_loss_consumer
      PROPERTIES LABELS package-process-connection-loss
    )
    add_test(
      NAME umbra_rti_package_process_connection_loss_delete_objects_consumer
      COMMAND umbra_rti_package_process_smoke connection-loss-delete-objects
    )
    set_tests_properties(umbra_rti_package_process_connection_loss_delete_objects_consumer
      PROPERTIES LABELS package-process-connection-loss-delete-objects
    )
    add_test(
      NAME umbra_rti_package_process_object_registration_consumer
      COMMAND umbra_rti_package_process_smoke object-registration
    )
    set_tests_properties(umbra_rti_package_process_object_registration_consumer
      PROPERTIES LABELS package-process-object-registration
    )
    add_test(
      NAME umbra_rti_package_process_named_registration_consumer
      COMMAND umbra_rti_package_process_smoke named-registration
    )
    set_tests_properties(umbra_rti_package_process_named_registration_consumer
      PROPERTIES LABELS package-process-named-registration
    )
    add_test(
      NAME umbra_rti_package_process_attribute_update_consumer
      COMMAND umbra_rti_package_process_smoke attribute-update
    )
    set_tests_properties(umbra_rti_package_process_attribute_update_consumer
      PROPERTIES LABELS package-process-attribute-update
    )
    add_test(
      NAME umbra_rti_package_process_directed_retraction_consumer
      COMMAND umbra_rti_package_process_smoke directed-retraction
    )
    set_tests_properties(umbra_rti_package_process_directed_retraction_consumer
      PROPERTIES LABELS package-process-directed-retraction
    )
    add_test(
      NAME umbra_rti_package_process_federation_save_restore_consumer
      COMMAND umbra_rti_package_process_smoke federation-save-restore
    )
    set_tests_properties(umbra_rti_package_process_federation_save_restore_consumer
      PROPERTIES LABELS package-process-federation-save-restore
    )
    add_test(
      NAME umbra_rti_package_process_federation_save_restore_failure_consumer
      COMMAND umbra_rti_package_process_smoke federation-save-restore-failure
    )
    set_tests_properties(umbra_rti_package_process_federation_save_restore_failure_consumer
      PROPERTIES LABELS package-process-federation-save-restore-failure
    )
    add_test(
      NAME umbra_rti_package_process_federation_save_restore_abort_consumer
      COMMAND umbra_rti_package_process_smoke federation-save-restore-abort
    )
    set_tests_properties(umbra_rti_package_process_federation_save_restore_abort_consumer
      PROPERTIES LABELS package-process-federation-save-restore-abort
    )
    add_test(
      NAME umbra_rti_package_process_federation_save_restore_status_consumer
      COMMAND umbra_rti_package_process_smoke federation-save-restore-status
    )
    set_tests_properties(umbra_rti_package_process_federation_save_restore_status_consumer
      PROPERTIES LABELS package-process-federation-save-restore-status
    )
  endif()
endif()

if(UMBRA_PACKAGE_SUBSCRIPTION_DISCOVERY_SMOKE)
  include("${CMAKE_CURRENT_LIST_DIR}/delayed_regional_attribute_immediate.cmake")
endif()
