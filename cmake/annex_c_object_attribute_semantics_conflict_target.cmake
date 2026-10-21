# Keep the page-110 l18 same-named attribute mismatch case independently buildable.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_annex_c_object_attribute_semantics_conflict_catch2
    SOURCES
    cpp/tests/annex_c_object_attribute_semantics_conflict_catch2.cpp
  )
  catch_discover_tests(umbra_annex_c_object_attribute_semantics_conflict_catch2
    TEST_PREFIX "umbra.annex_c_object_attribute_semantics_conflict.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
  )
endif()

# Keep the neighboring internal-only transportation mismatch branch isolated too.
include(cmake/annex_c_object_attribute_transportation_conflict_target.cmake)
