cmake_minimum_required(VERSION 3.23)

project(umbra_rti_package_smoke LANGUAGES CXX)

find_package(umbra_rti CONFIG REQUIRED)

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

include(CTest)
if(BUILD_TESTING)
  add_test(NAME umbra_rti_package_consumer COMMAND umbra_rti_package_smoke)
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
