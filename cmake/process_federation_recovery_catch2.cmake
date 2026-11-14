# Keep socket-recovery integration tests in a small runner so this lane does
# not need to rebuild the entire grouped IEEE 1516.1-2025 connection suite.
if(NOT TARGET umbra_ieee1516_2025_catch2)
  return()
endif()

set(_umbra_process_federation_recovery_target
  umbra_process_federation_recovery_catch2)
add_executable(${_umbra_process_federation_recovery_target}
  cpp/tests/ieee1516_2025_process_federation_recovery_catch2.cpp
)
foreach(_umbra_property IN ITEMS
    COMPILE_DEFINITIONS
    COMPILE_FEATURES
    COMPILE_OPTIONS
    INCLUDE_DIRECTORIES
    LINK_LIBRARIES
    LINK_OPTIONS)
  get_target_property(_umbra_property_value umbra_ieee1516_2025_catch2
    ${_umbra_property})
  if(_umbra_property_value)
    set_property(TARGET ${_umbra_process_federation_recovery_target}
      PROPERTY ${_umbra_property} "${_umbra_property_value}")
  endif()
endforeach()
get_target_property(_umbra_test_dependencies umbra_ieee1516_2025_catch2
  MANUALLY_ADDED_DEPENDENCIES)
if(_umbra_test_dependencies)
  add_dependencies(${_umbra_process_federation_recovery_target}
    ${_umbra_test_dependencies})
endif()

include(Catch)
# These loopback integration cases are lightweight; let the caller's CTest
# parallel limit schedule all available recovery cases concurrently.
set(_umbra_process_federation_recovery_discovery_properties
  PROPERTIES PROCESSORS 1)
if(WIN32)
  set(_umbra_process_federation_recovery_temp_directory
    "${CMAKE_BINARY_DIR}/t")
  file(MAKE_DIRECTORY "${_umbra_process_federation_recovery_temp_directory}")
  set(_umbra_process_federation_recovery_discovery_properties
    PROPERTIES
      ENVIRONMENT
        "TMP=${_umbra_process_federation_recovery_temp_directory}"
      PROCESSORS 1)
endif()
catch_discover_tests(${_umbra_process_federation_recovery_target}
  TEST_PREFIX "umbra.process_federation_recovery.catch2."
  ADD_TAGS_AS_LABELS
  ${_umbra_process_federation_recovery_discovery_properties}
  DISCOVERY_MODE POST_BUILD
)

list(APPEND UMBRA_IEEE1516_2025_GROUPED_CATCH2_RUNNERS
  ${_umbra_process_federation_recovery_target})
