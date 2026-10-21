# Focused public callback checks for the lifetime of conveyed region copies.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ddm_conveyed_region_designator_snapshot_catch2
    SOURCES
    cpp/tests/ddm_conveyed_region_designator_snapshot_lifetime_catch2.cpp
    cpp/tests/ddm_conveyed_region_designator_attribute_reflection_snapshot_catch2.cpp
    cpp/tests/ddm_conveyed_region_designator_tso_snapshot_lifetime_catch2.cpp
    cpp/tests/ddm_conveyed_region_designator_tso_reflect_snapshot_lifetime_catch2.cpp
  )
  catch_discover_tests(umbra_ddm_conveyed_region_designator_snapshot_catch2
    TEST_PREFIX "umbra.ddm_conveyed_region_designator_snapshot.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
