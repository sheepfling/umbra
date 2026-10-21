# Focused embedded public-API witness for accumulation across successive
# directed-interaction subscriptions on the same object class.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_directed_interaction_subscription_accumulation_catch2
    SOURCES
    cpp/tests/directed_interaction_subscription_accumulation_catch2.cpp
  )
  catch_discover_tests(
    umbra_ieee1516_2025_directed_interaction_subscription_accumulation_catch2
    TEST_PREFIX "umbra.ieee1516_2025.directed_interaction_subscription_accumulation.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
