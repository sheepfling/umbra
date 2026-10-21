# Build the receive-order Send Interaction MOM/file-switch boundary without
# rebuilding the aggregate 2025 Catch2 executable.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_send_interaction_mom_report_catch2
    SOURCES
    cpp/tests/ieee1516_2025_service_report_receive_order_send_interaction_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_send_interaction_mom_report_catch2
    TEST_PREFIX "umbra.ieee1516_2025_send_interaction_mom_report.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
