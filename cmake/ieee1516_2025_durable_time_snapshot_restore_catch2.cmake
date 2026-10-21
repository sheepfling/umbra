# Focused public-API Catch2 target for one durable 2025 time snapshot slice.
# The top-level Catch2 configuration includes this fragment after its helpers.
if(NOT TARGET umbra_ieee1516_2025_durable_time_snapshot_restore_catch2 AND
   TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_durable_time_snapshot_restore_catch2
    SOURCES
    cpp/tests/ieee1516_2025_durable_time_snapshot_restore_catch2.cpp
  )
  set(_umbra_durable_time_snapshot_temp_directory "${CMAKE_BINARY_DIR}/t")
  file(MAKE_DIRECTORY "${_umbra_durable_time_snapshot_temp_directory}")
  catch_discover_tests(umbra_ieee1516_2025_durable_time_snapshot_restore_catch2
    TEST_PREFIX "umbra.ieee1516_2025_durable_time_snapshot_restore.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
    PROPERTIES ENVIRONMENT "TMP=${_umbra_durable_time_snapshot_temp_directory}"
  )
endif()
