# Focused public-API Catch2 target for the 2025 MOM report region rule.
# The top-level Catch2 configuration includes this fragment after its helpers.
get_property(_mom_report_single_region_hook_deferred GLOBAL PROPERTY
  UMBRA_MOM_REPORT_SINGLE_REGION_HOOK_DEFERRED)
if(NOT _mom_report_single_region_hook_deferred)
  set_property(GLOBAL PROPERTY UMBRA_MOM_REPORT_SINGLE_REGION_HOOK_DEFERRED TRUE)
  cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
    CALL include
      "${CMAKE_SOURCE_DIR}/cmake/ieee1516_2025_mom_report_single_region_catch2.cmake")
  return()
endif()

if(NOT TARGET umbra_ieee1516_2025_mom_report_single_region_catch2 AND
   TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_mom_report_single_region_catch2
    SOURCES
    cpp/tests/ieee1516_2025_mom_report_single_region_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_mom_report_single_region_catch2
    TEST_PREFIX "umbra.ieee1516_2025_mom_report_single_region.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
