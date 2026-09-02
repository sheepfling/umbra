cmake_minimum_required(VERSION 3.20)

project(hla_rti_cpp_tck_installed_package_consumer LANGUAGES CXX)

# The adapter is intentionally driven only by its explicit prefix. Do not
# fall back to either CMake package registry while resolving the installed API.
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
find_package(umbra_rti CONFIG REQUIRED)

foreach(_required_target IN ITEMS
    umbra::ieee1516_2025_headers
    umbra::rti
    umbra::fedtime
    umbra::authorizer)
  if(NOT TARGET "${_required_target}")
    message(FATAL_ERROR
      "The installed provider package did not export ${_required_target}")
  endif()
endforeach()

get_target_property(_installed_api_include
  umbra::ieee1516_2025_headers INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _installed_api_include OR _installed_api_include MATCHES "-NOTFOUND")
  message(FATAL_ERROR
    "The installed provider package did not export the official API include directory")
endif()
list(GET _installed_api_include 0 _installed_api_include)

get_filename_component(_repository_root
  "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(HLA_RTI_TCK_API_INCLUDE_DIR "${_installed_api_include}" CACHE PATH
  "Official API headers exported by the installed provider package" FORCE)
set(HLA_RTI_TCK_PROVIDER_LIBRARIES
  "umbra::rti;umbra::fedtime;umbra::authorizer" CACHE STRING
  "Provider targets exported by the installed package" FORCE)
set(HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS "" CACHE STRING
  "Compile definitions are carried by the imported provider targets" FORCE)
set(HLA_RTI_TCK_PROVIDER_LINK_OPTIONS "" CACHE STRING
  "Link options are carried by the imported provider targets" FORCE)

set(HLA_RTI_TCK_ADAPTER_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml"
  CACHE FILEPATH "FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL "both"
  CACHE STRING "Callback model supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_PROVIDER_ID "installed-package"
  CACHE STRING "Provider identifier recorded in TCK evidence")
set(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS ""
  CACHE STRING "Optional standard RtiConfiguration address supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME ""
  CACHE STRING "Optional standard RtiConfiguration name supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS ""
  CACHE STRING "Optional standard RtiConfiguration settings supplied by this adapter")

if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_FOM}")
  message(FATAL_ERROR "Adapter FOM does not exist: ${HLA_RTI_TCK_ADAPTER_FOM}")
endif()
if(NOT HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL MATCHES "^(both|evoked|immediate)$")
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL must be both, evoked, or immediate")
endif()

add_subdirectory(
  "${_repository_root}/packages/hla-rti-cpp-tck"
  "${CMAKE_CURRENT_BINARY_DIR}/portable-tck")

include(CTest)
if(BUILD_TESTING)
  set(_tck_command
    $<TARGET_FILE:hla_rti_cpp_tck>
    --fom "${HLA_RTI_TCK_ADAPTER_FOM}"
    --callback-model "${HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL}"
    --provider-id "${HLA_RTI_TCK_ADAPTER_PROVIDER_ID}")
  if(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS)
    list(APPEND _tck_command
      --rti-address "${HLA_RTI_TCK_ADAPTER_RTI_ADDRESS}")
  endif()
  if(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME)
    list(APPEND _tck_command
      --configuration-name "${HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME}")
  endif()
  if(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS)
    list(APPEND _tck_command
      --additional-settings "${HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS}")
  endif()
  add_test(NAME hla_rti_cpp_tck_installed_p0 COMMAND ${_tck_command})
  set_tests_properties(hla_rti_cpp_tck_installed_p0 PROPERTIES
    LABELS "portable-cpp-tck;installed-package")
endif()
