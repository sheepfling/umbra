# Focused configured-process public callback evidence for the
# HLAreportServiceInvocation update-region designator in §11.4.2.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_process_mom_service_report_region_callback_catch2
    SOURCES
    cpp/tests/ieee1516_2025_connection_process_mom_service_report_region_callback_catch2.cpp
  )
  catch_discover_tests(
    umbra_ieee1516_2025_process_mom_service_report_region_callback_catch2
    TEST_PREFIX "umbra.ieee1516_2025.process_mom_service_report_region_callback.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
