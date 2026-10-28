# Include after the installed-package Catch2 dependency is available. This
# downstream consumer links the exported RTI and fedtime targets.
if(TARGET Catch2::Catch2WithMain)
  get_filename_component(UMBRA_PACKAGE_DELAYED_REGIONAL_MIM_FOM
    "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml"
    ABSOLUTE)
  get_filename_component(UMBRA_PACKAGE_DELAYED_REGIONAL_RESTAURANT_FOM
    "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml"
    ABSOLUTE)
  get_filename_component(UMBRA_PACKAGE_DELAYED_REGIONAL_SWITCH_FOM
    "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/data/delay-subscription-evaluation-enabled-fom.xml"
    ABSOLUTE)

  add_executable(umbra_rti_package_delayed_regional_attribute_catch2
    "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/tests/package_delayed_regional_attribute_immediate_catch2.cpp")
  target_compile_features(umbra_rti_package_delayed_regional_attribute_catch2 PRIVATE cxx_std_20)
  target_compile_definitions(umbra_rti_package_delayed_regional_attribute_catch2 PRIVATE
    UMBRA_PACKAGE_DELAYED_REGIONAL_MIM_FOM="${UMBRA_PACKAGE_DELAYED_REGIONAL_MIM_FOM}"
    UMBRA_PACKAGE_DELAYED_REGIONAL_RESTAURANT_FOM="${UMBRA_PACKAGE_DELAYED_REGIONAL_RESTAURANT_FOM}"
    UMBRA_PACKAGE_DELAYED_REGIONAL_SWITCH_FOM="${UMBRA_PACKAGE_DELAYED_REGIONAL_SWITCH_FOM}"
  )
  target_link_libraries(umbra_rti_package_delayed_regional_attribute_catch2 PRIVATE
    umbra::rti
    umbra::fedtime
    Catch2::Catch2WithMain
  )

  if(BUILD_TESTING)
    add_test(
      NAME umbra_rti_package_delayed_regional_attribute_dse_consumer
      COMMAND umbra_rti_package_delayed_regional_attribute_catch2
        "Installed 2025 package admits a delayed regional reflection after overlapping resubscription"
    )
    add_test(
      NAME umbra_rti_package_delayed_regional_attribute_resign_fence_consumer
      COMMAND umbra_rti_package_delayed_regional_attribute_catch2
        "Installed 2025 package fences an active callback before resign returns"
    )
    set_tests_properties(umbra_rti_package_delayed_regional_attribute_dse_consumer
      PROPERTIES LABELS
        "installable-package;package-delayed-regional-attribute-immediate")
    set_tests_properties(umbra_rti_package_delayed_regional_attribute_resign_fence_consumer
      PROPERTIES LABELS
        "installable-package;package-resign-callback-fence;internal")
  endif()
endif()
