foreach(required_variable IN ITEMS
    UMBRA_CMAKE_COMMAND
    UMBRA_CTEST_COMMAND
    UMBRA_PYTHON_EXECUTABLE
    UMBRA_SOURCE_DIRECTORY
    UMBRA_BINARY_DIRECTORY)
  if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${required_variable} is required for the installed-package smoke test")
  endif()
endforeach()

function(umbra_run_checked)
  execute_process(
    COMMAND ${ARGN}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
  )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR
      "Installed-package smoke command failed (${result}): ${ARGN}\n${output}\n${error}"
    )
  endif()
endfunction()

set(install_prefix "${UMBRA_BINARY_DIRECTORY}/package-smoke-install")
set(consumer_binary_directory "${UMBRA_BINARY_DIRECTORY}/package-smoke-consumer")

set(install_command
  "${UMBRA_CMAKE_COMMAND}"
  --install "${UMBRA_BINARY_DIRECTORY}"
  --prefix "${install_prefix}"
)
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND install_command --config "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${install_command})

# The installed package must carry the exact 1516.2 schemas, MIM, examples,
# and reviewed digest manifest that the source-tree validator uses.  Validate
# the staged files before compiling the consumer so a successful CMake export
# cannot mask a truncated or altered standards-resource payload.
set(installed_resource_root
  "${install_prefix}/share/umbra_rti/ieee1516.2-2025"
)
set(installed_resource_manifest
  "${install_prefix}/share/umbra_rti/ieee1516.2-2025/resource-digests.json"
)
umbra_run_checked(
  "${UMBRA_PYTHON_EXECUTABLE}"
  "${UMBRA_SOURCE_DIRECTORY}/tools/ieee_1516_2_resources.py"
  --check
  --root "${installed_resource_root}"
  --manifest "${installed_resource_manifest}"
)

set(configure_command
  "${UMBRA_CMAKE_COMMAND}"
  -S "${UMBRA_SOURCE_DIRECTORY}/cmake/package-smoke"
  -B "${consumer_binary_directory}"
  "-DCMAKE_PREFIX_PATH=${install_prefix}"
)
if(DEFINED UMBRA_GENERATOR AND NOT "${UMBRA_GENERATOR}" STREQUAL "")
  list(APPEND configure_command -G "${UMBRA_GENERATOR}")
endif()
if(DEFINED UMBRA_GENERATOR_PLATFORM AND NOT "${UMBRA_GENERATOR_PLATFORM}" STREQUAL "")
  list(APPEND configure_command -A "${UMBRA_GENERATOR_PLATFORM}")
endif()
umbra_run_checked(${configure_command})

set(build_command
  "${UMBRA_CMAKE_COMMAND}"
  --build "${consumer_binary_directory}"
)
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND build_command --config "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${build_command})

set(ctest_command
  "${UMBRA_CTEST_COMMAND}"
  --test-dir "${consumer_binary_directory}"
  --output-on-failure
)
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND ctest_command -C "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${ctest_command})
