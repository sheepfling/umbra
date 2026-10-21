# Isolated public API test of interaction-class FDD preferred-order defaults.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_interaction_fdd_preferred_order_default_catch2
    SOURCES
    cpp/tests/ieee1516_2025_interaction_fdd_preferred_order_default_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_interaction_fdd_preferred_order_default_catch2
    TEST_PREFIX "umbra.ieee1516_2025_interaction_fdd_preferred_order_default.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
