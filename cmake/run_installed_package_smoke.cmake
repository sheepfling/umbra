foreach(required_variable IN ITEMS
    UMBRA_CMAKE_COMMAND
    UMBRA_CTEST_COMMAND
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
