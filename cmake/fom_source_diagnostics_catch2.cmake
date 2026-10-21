# Keep public FOM source classification cases independently buildable.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_fom_source_diagnostics_catch2
    SOURCES
    cpp/tests/ieee1516_2025_fom_source_diagnostics_catch2.cpp
  )
  catch_discover_tests(umbra_fom_source_diagnostics_catch2
    TEST_PREFIX "umbra.fom_source_diagnostics.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
