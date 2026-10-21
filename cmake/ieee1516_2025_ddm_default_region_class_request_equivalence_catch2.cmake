# Focused public-API check of class-form Request Attribute Value Update
# equivalence between an omitted region and the default region.
if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_ddm_default_region_class_request_equivalence_catch2
    SOURCES
    cpp/tests/ieee1516_2025_ddm_default_region_class_request_equivalence_catch2.cpp
  )
  catch_discover_tests(umbra_ieee1516_2025_ddm_default_region_class_request_equivalence_catch2
    TEST_PREFIX "umbra.ieee1516_2025.ddm_default_region_class_request_equivalence.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
