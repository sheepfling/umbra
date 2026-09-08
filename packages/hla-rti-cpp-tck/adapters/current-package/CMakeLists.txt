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
set(HLA_RTI_TCK_ADAPTER_MODEL_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-model-tck.xml"
  CACHE FILEPATH "Rich FOM model supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_DDM_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-tck.xml"
  CACHE FILEPATH "Dimensional FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-multi-attribute-tck.xml"
  CACHE FILEPATH "Multi-attribute dimensional FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-three-dimensional-tck.xml"
  CACHE FILEPATH "Three-dimensional DDM FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_MIM_FOM
  "${_repository_root}/third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml"
  CACHE FILEPATH "Standard MIM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_SWITCHES_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/switches-tck.xml"
  CACHE FILEPATH "Switch-declaration FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/auto-provide-tck.xml"
  CACHE FILEPATH "Auto Provide FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS "TckDimensionX;TckDimensionY"
  CACHE STRING "Semicolon-separated DDM dimensions supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS
  "TckDimensionX;TckDimensionY;TckDimensionZ"
  CACHE STRING "Semicolon-separated three-dimensional DDM dimensions supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS
  "HLAobjectRoot.TckThreeDimensionalObject"
  CACHE STRING "Three-dimensional DDM object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE "Value"
  CACHE STRING "Three-dimensional DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS
  "HLAobjectRoot.TckTransportRegionalObject"
  CACHE STRING "Timestamped regional custom-transport object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE "RegionalValue"
  CACHE STRING "Timestamped regional custom-transport attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS
  "HLAinteractionRoot.TckTransportRegionalInteraction"
  CACHE STRING "Timestamped regional custom-transport interaction class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER "RegionalPayload"
  CACHE STRING "Timestamped regional custom-transport parameter supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_OBJECT_CLASS "HLAobjectRoot.TckObject"
  CACHE STRING "Ordinary object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ATTRIBUTE "Value"
  CACHE STRING "Ordinary object attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_OBJECT_CLASS
  "HLAobjectRoot.TckAutoProvideObject"
  CACHE STRING "Auto Provide object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FIRST "ProviderAValue"
  CACHE STRING "First Auto Provide attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_SECOND "ProviderBValue"
  CACHE STRING "Second Auto Provide attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS
  "HLAobjectRoot.TckMultiAttributeObject"
  CACHE STRING "Multi-attribute DDM object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST "FirstValue"
  CACHE STRING "First multi-attribute DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND "SecondValue"
  CACHE STRING "Second multi-attribute DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS "HLAinteractionRoot.TckInteraction"
  CACHE STRING "Ordinary interaction class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_PARAMETER "Payload"
  CACHE STRING "Ordinary interaction parameter supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-extension-tck.xml"
  CACHE STRING "Semicolon-separated extension FOM modules supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-malformed-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-namespace-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-duplicate-tck.xml"
  CACHE STRING "Semicolon-separated invalid FOM modules supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL "both"
  CACHE STRING "Callback model supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_TIMEOUT_MS "10000"
  CACHE STRING "Per-scenario timeout passed to the portable TCK")
set(HLA_RTI_TCK_ADAPTER_SCENARIO_SET "verified"
  CACHE STRING "Scenario set for the CTest matrix: verified or all")
set(HLA_RTI_TCK_ADAPTER_PROVIDER_ID "installed-package"
  CACHE STRING "Provider identifier recorded in TCK evidence")
set(HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION "HLAinteger64Time"
  CACHE STRING "Logical time implementation selected by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS ""
  CACHE STRING "Optional standard RtiConfiguration address supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME ""
  CACHE STRING "Optional standard RtiConfiguration name supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS ""
  CACHE STRING "Optional standard RtiConfiguration settings supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER ""
  CACHE FILEPATH "Optional marker written after the adapter observes connection loss")
set(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED OFF
  CACHE BOOL "Whether the provider adapter manages connection loss out of band")
set(HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS ""
  CACHE STRING "Semicolon-separated scenarios excluded from the adapter CTest matrix")

if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_FOM}")
  message(FATAL_ERROR "Adapter FOM does not exist: ${HLA_RTI_TCK_ADAPTER_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}")
  message(FATAL_ERROR
    "Adapter rich FOM model does not exist: ${HLA_RTI_TCK_ADAPTER_MODEL_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_DDM_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_DDM_FOM}")
  message(FATAL_ERROR
    "Adapter DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_DDM_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}")
  message(FATAL_ERROR
    "Adapter multi-attribute DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}")
  message(FATAL_ERROR
    "Adapter three-dimensional DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MIM_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MIM_FOM}")
  message(FATAL_ERROR
    "Adapter MIM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_MIM_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}")
  message(FATAL_ERROR
    "Adapter switch-declaration FOM does not exist: ${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}")
  message(FATAL_ERROR
    "Adapter Auto Provide FOM does not exist: ${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}")
endif()
list(LENGTH HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS _ddm_dimension_count)
if(_ddm_dimension_count LESS 2)
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS must supply at least two dimensions")
endif()
list(LENGTH HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS
  _three_dimensional_dimension_count)
if(_three_dimensional_dimension_count LESS 3)
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS must supply at least three dimensions")
endif()
foreach(_additional_fom IN LISTS HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES)
  if(NOT EXISTS "${_additional_fom}" OR IS_DIRECTORY "${_additional_fom}")
    message(FATAL_ERROR "Adapter extension FOM does not exist: ${_additional_fom}")
  endif()
endforeach()
foreach(_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES)
  if(NOT EXISTS "${_invalid_fom}" OR IS_DIRECTORY "${_invalid_fom}")
    message(FATAL_ERROR "Adapter invalid FOM does not exist: ${_invalid_fom}")
  endif()
endforeach()
if(NOT HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL MATCHES "^(both|evoked|immediate)$")
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL must be both, evoked, or immediate")
endif()
if(NOT HLA_RTI_TCK_ADAPTER_SCENARIO_SET MATCHES "^(verified|all)$")
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_SCENARIO_SET must be verified or all")
endif()

set(_tck_catalog_path
  "${_repository_root}/compliance/catalogs/cpp-tck-scenario-catalog.json")
if(NOT EXISTS "${_tck_catalog_path}")
  message(FATAL_ERROR "Portable TCK scenario catalog does not exist: ${_tck_catalog_path}")
endif()
file(READ "${_tck_catalog_path}" _tck_catalog_json)
string(JSON _tck_scenario_count LENGTH "${_tck_catalog_json}" scenarios)
if(_tck_scenario_count LESS 1)
  message(FATAL_ERROR "Portable TCK scenario catalog contains no scenarios")
endif()
math(EXPR _tck_last_scenario_index "${_tck_scenario_count} - 1")
set(_tck_test_scenarios)
set(_tck_candidate_scenarios)
foreach(_tck_scenario_index RANGE 0 ${_tck_last_scenario_index})
  string(JSON _tck_scenario GET "${_tck_catalog_json}" scenarios
    ${_tck_scenario_index} id)
  if(_tck_scenario IN_LIST HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS)
    continue()
  endif()
  string(JSON _tck_status GET "${_tck_catalog_json}" scenarios
    ${_tck_scenario_index} default_status)
  string(JSON _tck_promotion GET "${_tck_catalog_json}" scenarios
    ${_tck_scenario_index} promotion)
  if(_tck_promotion STREQUAL "candidate")
    list(APPEND _tck_candidate_scenarios "${_tck_scenario}")
  endif()
  if(NOT _tck_status STREQUAL "unsupported" AND
    (HLA_RTI_TCK_ADAPTER_SCENARIO_SET STREQUAL "all" OR
      _tck_promotion STREQUAL "promoted"))
    list(APPEND _tck_test_scenarios "${_tck_scenario}")
  endif()
endforeach()
if(NOT _tck_test_scenarios)
  message(FATAL_ERROR
    "The selected portable TCK scenario set contains no runnable scenarios")
endif()

if(HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL STREQUAL "both")
  set(_tck_test_callback_models evoked immediate)
else()
  set(_tck_test_callback_models "${HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL}")
endif()

set(_tck_common_arguments
  --fom "${HLA_RTI_TCK_ADAPTER_FOM}"
  --model-fom "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}"
  --ddm-fom "${HLA_RTI_TCK_ADAPTER_DDM_FOM}"
  --multi-attribute-fom "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}"
  --three-dimensional-fom "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}"
  --three-dimensional-object-class "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS}"
  --three-dimensional-attribute "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE}"
  --custom-regional-object-class "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS}"
  --custom-regional-attribute "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE}"
  --custom-regional-interaction-class "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS}"
  --custom-regional-parameter "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER}"
  --object-class "${HLA_RTI_TCK_ADAPTER_OBJECT_CLASS}"
  --attribute "${HLA_RTI_TCK_ADAPTER_ATTRIBUTE}"
  --auto-provide-object-class "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_OBJECT_CLASS}"
  --auto-provide-first "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FIRST}"
  --auto-provide-second "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_SECOND}"
  --multi-attribute-object-class
    "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS}"
  --multi-attribute-first "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST}"
  --multi-attribute-second "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND}"
  --interaction-class "${HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS}"
  --parameter "${HLA_RTI_TCK_ADAPTER_PARAMETER}"
  --mim-fom "${HLA_RTI_TCK_ADAPTER_MIM_FOM}"
  --switches-fom "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}"
  --auto-provide-fom "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}"
  --time-implementation "${HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION}"
  --provider-id "${HLA_RTI_TCK_ADAPTER_PROVIDER_ID}"
  --timeout-ms "${HLA_RTI_TCK_ADAPTER_TIMEOUT_MS}")
foreach(_additional_fom IN LISTS HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES)
  list(APPEND _tck_common_arguments --additional-fom "${_additional_fom}")
endforeach()
foreach(_ddm_dimension IN LISTS HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS)
  list(APPEND _tck_common_arguments --ddm-dimension "${_ddm_dimension}")
endforeach()
foreach(_three_dimensional_dimension IN LISTS HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS)
  list(APPEND _tck_common_arguments
    --three-dimensional-dimension "${_three_dimensional_dimension}")
endforeach()
foreach(_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES)
  list(APPEND _tck_common_arguments --invalid-fom "${_invalid_fom}")
endforeach()
if(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS)
  list(APPEND _tck_common_arguments
    --rti-address "${HLA_RTI_TCK_ADAPTER_RTI_ADDRESS}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME)
  list(APPEND _tck_common_arguments
    --configuration-name "${HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME}")
endif()
if(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS)
  list(APPEND _tck_common_arguments
    --additional-settings "${HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER)
  list(APPEND _tck_common_arguments
    --connection-loss-marker "${HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED)
  list(APPEND _tck_common_arguments --connection-loss-server-managed)
endif()

set(HLA_RTI_TCK_TEST_SCENARIOS "${_tck_test_scenarios}" CACHE STRING
  "Portable TCK scenarios registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS "${_tck_candidate_scenarios}" CACHE STRING
  "Portable TCK candidate scenarios registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_CALLBACK_MODELS "${_tck_test_callback_models}" CACHE STRING
  "Portable TCK callback models registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_COMMON_ARGUMENTS "${_tck_common_arguments}" CACHE STRING
  "Adapter arguments shared by each portable TCK scenario" FORCE)
set(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS 120 CACHE STRING
  "CTest timeout for each portable TCK scenario" FORCE)

add_subdirectory(
  "${_repository_root}/packages/hla-rti-cpp-tck"
  "${CMAKE_CURRENT_BINARY_DIR}/portable-tck")

include(CTest)
if(BUILD_TESTING)
  set(_tck_command
    $<TARGET_FILE:hla_rti_cpp_tck>
    ${_tck_common_arguments}
    --callback-model "${HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL}"
    )
  add_test(NAME hla_rti_cpp_tck_installed COMMAND ${_tck_command})
  set_tests_properties(hla_rti_cpp_tck_installed PROPERTIES
    LABELS "portable-cpp-tck-suite;installed-package")
endif()
