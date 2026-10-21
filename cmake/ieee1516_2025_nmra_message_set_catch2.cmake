# Optional focused public-API Catch2 target for the NMRA message-set rule.
# Pass this file as CMAKE_PROJECT_umbra_rti_INCLUDE during a one-off configure.
get_property(_nmra_message_set_hook_deferred GLOBAL PROPERTY
  UMBRA_NMRA_MESSAGE_SET_HOOK_DEFERRED)
if(NOT _nmra_message_set_hook_deferred)
  set_property(GLOBAL PROPERTY UMBRA_NMRA_MESSAGE_SET_HOOK_DEFERRED TRUE)
  cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL include
      "${CMAKE_SOURCE_DIR}/cmake/ieee1516_2025_nmra_message_set_catch2.cmake")
  return()
endif()

if(NOT TARGET umbra_ieee1516_2025_nmra_message_set_catch2 AND
   TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_nmra_message_set_catch2
    SOURCES
    cpp/tests/ieee1516_2025_nmra_message_set_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_nmra_message_set_catch2
    TEST_PREFIX "umbra.ieee1516_2025_nmra_message_set.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
