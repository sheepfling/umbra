# Dedicated public API order-conversion test for preferred-TSO interactions
# sent before and after the sender enables time regulation.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_timestamped_interaction_sender_regulation_transition_catch2
    SOURCES
    cpp/tests/ieee1516_2025_timestamped_interaction_order_sender_regulation_transition_catch2.cpp
  )
  catch_discover_tests(umbra_timestamped_interaction_sender_regulation_transition_catch2
    TEST_PREFIX "umbra.timestamped_interaction_sender_regulation_transition.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
