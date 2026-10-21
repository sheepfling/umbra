# Focused public API evidence for conditional HLAautomaticResignAction MOM
# reflections with both omitted and explicit FDD initial values.
if(TARGET Catch2::Catch2WithMain AND
   COMMAND umbra_add_embedded_2025_catch2_target)
  umbra_add_embedded_2025_catch2_target(
    umbra_ieee1516_2025_automatic_resign_mom_attribute_catch2
    SOURCES
    cpp/tests/ieee1516_2025_automatic_resign_mom_attribute_catch2.cpp
  )
  add_test(
    NAME umbra.ieee1516_2025.automatic_resign_mom_attribute.catch2
    COMMAND $<TARGET_FILE:umbra_ieee1516_2025_automatic_resign_mom_attribute_catch2>
      "[automatic-resign-mom-reflection]"
  )
  set_tests_properties(
    umbra.ieee1516_2025.automatic_resign_mom_attribute.catch2
    PROPERTIES LABELS "integration;federation-management;mom;2025"
  )
endif()
