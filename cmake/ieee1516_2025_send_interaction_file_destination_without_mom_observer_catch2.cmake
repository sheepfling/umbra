# Isolated public API case for file-selected Send Interaction reporting with
# no federate subscribed to the MOM service-report interaction.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_send_interaction_file_destination_without_mom_observer_catch2
    SOURCES
    cpp/tests/ieee1516_2025_send_interaction_file_destination_without_mom_observer_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_send_interaction_file_destination_without_mom_observer_catch2
    TEST_PREFIX "umbra.ieee1516_2025_send_interaction_file_destination_without_mom_observer.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
