# Focused public-API receipt-class and parameter projection check for directed interactions.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_directed_interaction_class_projection_catch2
    SOURCES
    cpp/tests/directed_interaction_class_projection_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_directed_interaction_class_projection_catch2
    TEST_PREFIX "umbra.ieee1516_2025.directed_interaction_class_projection.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
