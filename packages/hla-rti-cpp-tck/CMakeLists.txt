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

if(NOT HLA_RTI_TCK_API_INCLUDE_DIR)
  message(FATAL_ERROR
    "HLA_RTI_TCK_API_INCLUDE_DIR must point at the official RTI headers")
endif()

add_executable(hla_rti_cpp_tck src/main.cpp)
target_compile_features(hla_rti_cpp_tck PRIVATE cxx_std_17)
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
install(FILES fom/p0-tck.xml DESTINATION share/hla-rti-cpp-tck/fom)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME hla_rti_cpp_tck_help COMMAND hla_rti_cpp_tck --help)
endif()
