# Focused Catch2 target for durable restore of one pending 2025 time-role request.
# The top-level Catch2 configuration includes this fragment after its helpers.
if(NOT TARGET umbra_ieee1516_2025_durable_pending_time_constrained_restore_catch2 AND
   TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_durable_pending_time_constrained_restore_catch2
    SOURCES
    cpp/tests/ieee1516_2025_durable_pending_time_constrained_restore_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_durable_pending_time_constrained_restore_catch2
    TEST_PREFIX "umbra.ieee1516_2025_durable_pending_time_constrained_restore.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
