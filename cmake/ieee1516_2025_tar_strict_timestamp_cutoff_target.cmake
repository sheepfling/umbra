if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_tar_strict_timestamp_cutoff_catch2
    SOURCES
    cpp/tests/ieee1516_2025_tar_strict_timestamp_cutoff_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_tar_strict_timestamp_cutoff_catch2
    TEST_PREFIX "umbra.ieee1516_2025_tar_strict_timestamp_cutoff.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
