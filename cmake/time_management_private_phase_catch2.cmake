# Keep the internal TSO queue/coordinator phase tests independently buildable.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_time_management_private_phase_catch2
    SOURCES
    cpp/tests/tso_message_queue_catch2.cpp
    cpp/tests/federation_time_coordinator_catch2.cpp
  )
  catch_discover_tests(umbra_time_management_private_phase_catch2
    TEST_PREFIX "umbra.time_management_private_phase.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
