# Focused configured-process evidence for receive-order regional interaction
# and timestamped attribute-reflection conveyed-region callback snapshots.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_ddm_process_region_callback_snapshot_catch2
    SOURCES
    cpp/tests/ieee1516_2025_connection_process_ddm_region_callback_snapshot_catch2.cpp
  )
  catch_discover_tests(
    umbra_ieee1516_2025_ddm_process_region_callback_snapshot_catch2
    TEST_PREFIX "umbra.ieee1516_2025.ddm_process_region_callback_snapshot.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
