cmake_minimum_required(VERSION 3.20)

project(hla_rti_cpp_tck LANGUAGES CXX)

set(HLA_RTI_TCK_API_INCLUDE_DIR "" CACHE PATH
  "Directory containing the official IEEE 1516.1-2025 C++ API headers")
set(HLA_RTI_TCK_PROVIDER_LIBRARIES "" CACHE STRING
  "Semicolon-separated provider libraries and link targets")
set(HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS "" CACHE STRING
  "Semicolon-separated provider compile definitions")
set(HLA_RTI_TCK_PROVIDER_LINK_OPTIONS "" CACHE STRING
  "Semicolon-separated provider link options")
set(HLA_RTI_TCK_TEST_SCENARIOS "" CACHE STRING
  "Semicolon-separated portable TCK scenario IDs to register with CTest")
set(HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS "" CACHE STRING
  "Semicolon-separated candidate scenario IDs to label separately in CTest")
set(HLA_RTI_TCK_TEST_CALLBACK_MODELS "" CACHE STRING
  "Semicolon-separated callback models to register with CTest")
set(HLA_RTI_TCK_TEST_COMMON_ARGUMENTS "" CACHE STRING
  "Semicolon-separated adapter arguments shared by each CTest scenario")
set(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS 0 CACHE STRING
  "CTest timeout for each portable TCK scenario, or 0 for no CTest timeout")

if(NOT HLA_RTI_TCK_API_INCLUDE_DIR)
  message(FATAL_ERROR
    "HLA_RTI_TCK_API_INCLUDE_DIR must point at the official RTI headers")
endif()

add_executable(hla_rti_cpp_tck src/main.cpp)
target_compile_features(hla_rti_cpp_tck PRIVATE cxx_std_17)
if(MSVC)
  target_compile_options(hla_rti_cpp_tck PRIVATE /bigobj)
endif()
target_include_directories(hla_rti_cpp_tck PRIVATE
  "${HLA_RTI_TCK_API_INCLUDE_DIR}"
)
target_compile_definitions(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS}
)
target_link_options(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_LINK_OPTIONS}
)
target_link_libraries(hla_rti_cpp_tck PRIVATE
  ${HLA_RTI_TCK_PROVIDER_LIBRARIES}
)

install(TARGETS hla_rti_cpp_tck RUNTIME DESTINATION bin)
install(DIRECTORY fom/
  DESTINATION share/hla-rti-cpp-tck/fom
  FILES_MATCHING PATTERN "*.xml")

include(CTest)
if(BUILD_TESTING)
  add_test(NAME hla_rti_cpp_tck_help COMMAND hla_rti_cpp_tck --help)

  if(HLA_RTI_TCK_TEST_SCENARIOS AND HLA_RTI_TCK_TEST_CALLBACK_MODELS)
    foreach(_tck_scenario IN LISTS HLA_RTI_TCK_TEST_SCENARIOS)
      string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _tck_scenario_slug
        "${_tck_scenario}")
      if(_tck_scenario IN_LIST HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS)
        set(_tck_promotion_label candidate)
      else()
        set(_tck_promotion_label promoted)
      endif()
      foreach(_tck_callback_model IN LISTS HLA_RTI_TCK_TEST_CALLBACK_MODELS)
        set(_tck_test_name
          "hla_rti_cpp_tck.${_tck_scenario_slug}.${_tck_callback_model}")
        add_test(
          NAME "${_tck_test_name}"
          COMMAND hla_rti_cpp_tck
            ${HLA_RTI_TCK_TEST_COMMON_ARGUMENTS}
            --scenario "${_tck_scenario}"
            --callback-model "${_tck_callback_model}"
        )
        set_tests_properties("${_tck_test_name}" PROPERTIES
          LABELS "portable-cpp-tck;scenario;${_tck_callback_model};${_tck_promotion_label}"
          SKIP_REGULAR_EXPRESSION "skipped ")
        if(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS GREATER 0)
          set_tests_properties("${_tck_test_name}" PROPERTIES
            TIMEOUT "${HLA_RTI_TCK_TEST_TIMEOUT_SECONDS}")
        endif()
      endforeach()
    endforeach()
  endif()
endif()
