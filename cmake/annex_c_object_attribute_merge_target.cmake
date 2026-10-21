# Keep the page-110 l18 object-class attribute merge case independently buildable.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_annex_c_object_attribute_merge_catch2
    SOURCES
    cpp/tests/annex_c_object_attribute_merge_catch2.cpp
  )
  catch_discover_tests(umbra_annex_c_object_attribute_merge_catch2
    TEST_PREFIX "umbra.annex_c_object_attribute_merge.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()
