# Focused public-API evidence that FQR is completed with Flush Queue Grant,
# not Time Advance Grant.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_fqr_grant_callback_kind_catch2
    SOURCES
    cpp/tests/ieee1516_2025_fqr_grant_callback_kind_catch2.cpp
  )
  add_test(
    NAME umbra.ieee1516_2025.fqr_grant_callback_kind.catch2
    COMMAND $<TARGET_FILE:umbra_ieee1516_2025_fqr_grant_callback_kind_catch2>
      "[flush-queue-request-grant-callback-kind]"
  )
  set_tests_properties(
    umbra.ieee1516_2025.fqr_grant_callback_kind.catch2
    PROPERTIES LABELS "integration;time-management;2025"
  )
endif()
