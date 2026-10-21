# Focused IEEE 1516.1-2025 directed-interaction non-delivery regression.
# Included after the embedded 2025 Catch2 helper is defined.

if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_directed_interaction_no_delivery_catch2
    SOURCES
    cpp/tests/ieee1516_2025_directed_interaction_unsubscribed_no_delivery_catch2.cpp
  )
  catch_discover_tests(umbra_directed_interaction_no_delivery_catch2
    TEST_PREFIX "umbra.ieee1516_2025.directed_interaction_no_delivery.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
