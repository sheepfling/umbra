# Focused public-API Catch2 target for rebinding a pending regular ownership
# transaction to same-type members in a fresh registry.
if(NOT TARGET umbra_ieee1516_2025_restore_ownership_transaction_rebinding_catch2 AND
   TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_restore_ownership_transaction_rebinding_catch2
    SOURCES
    cpp/tests/ieee1516_2025_restore_ownership_transaction_rebinding_catch2.cpp
  )
  set(_umbra_restore_ownership_rebinding_temp_directory "${CMAKE_BINARY_DIR}/t")
  file(MAKE_DIRECTORY "${_umbra_restore_ownership_rebinding_temp_directory}")
  catch_discover_tests(umbra_ieee1516_2025_restore_ownership_transaction_rebinding_catch2
    TEST_PREFIX "umbra.ieee1516_2025_restore_ownership_transaction_rebinding.catch2."
    ADD_TAGS_AS_LABELS
    DISCOVERY_MODE POST_BUILD
    PROPERTIES ENVIRONMENT "TMP=${_umbra_restore_ownership_rebinding_temp_directory}"
  )
endif()
