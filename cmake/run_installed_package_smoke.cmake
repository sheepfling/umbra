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

# CTest does not build target dependencies before invoking a test.  The
# package install exports all of these static targets, so explicitly build
# the complete installed library set before calling `cmake --install`.  This
# keeps the package smoke reproducible whether it follows a full build or a
# focused Catch2 target build.
set(runtime_build_command
  "${UMBRA_CMAKE_COMMAND}"
  --build "${UMBRA_BINARY_DIRECTORY}"
  --target umbra_rti umbra_rti_2010 umbra_fedtime umbra_authorizer
)
set(process_probe_path "")
if(DEFINED UMBRA_EXPECT_PROCESS_PROFILE AND
   "${UMBRA_EXPECT_PROCESS_PROFILE}" STREQUAL "ON")
  # The process fixture is intentionally a source-tree test executable.  It
  # is built for the package smoke, but is not installed as public runtime.
  list(APPEND runtime_build_command umbra_process_federation_service_probe)
  if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
    set(process_probe_path
      "${UMBRA_BINARY_DIRECTORY}/${UMBRA_CONFIGURATION}/umbra_process_federation_service_probe.exe"
    )
  else()
    set(process_probe_path
      "${UMBRA_BINARY_DIRECTORY}/umbra_process_federation_service_probe.exe"
    )
  endif()
  if(NOT EXISTS "${process_probe_path}")
    # Single-config generators and non-Windows builds do not place the
    # executable under a configuration directory or use an .exe suffix.
    set(process_probe_path
      "${UMBRA_BINARY_DIRECTORY}/umbra_process_federation_service_probe"
    )
  endif()
  if(NOT EXISTS "${process_probe_path}")
    message(FATAL_ERROR
      "Embedded profile package smoke could not find its process fixture: ${process_probe_path}"
    )
  endif()
endif()
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND runtime_build_command --config "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${runtime_build_command})

set(install_prefix "${UMBRA_BINARY_DIRECTORY}/package-smoke-install")
set(consumer_binary_directory "${UMBRA_BINARY_DIRECTORY}/package-smoke-consumer")

# Keep the downstream check genuinely clean.  Both paths are deterministic
# subdirectories of the selected build tree; removing them prevents a stale
# installed export or consumer executable from masking a packaging change.
file(REMOVE_RECURSE "${install_prefix}" "${consumer_binary_directory}")

set(install_command
  "${UMBRA_CMAKE_COMMAND}"
  --install "${UMBRA_BINARY_DIRECTORY}"
  --prefix "${install_prefix}"
)
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND install_command --config "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${install_command})

# Validate the installed profile manifest before compiling the downstream
# consumer.  The manifest is the reproducible record of which optional runtime
# profile was built; it must not be inferred from whatever happens to be in the
# source checkout at test time.
set(installed_profile_manifest
  "${install_prefix}/share/umbra_rti/umbra_rti-profile.json"
)
if(NOT EXISTS "${installed_profile_manifest}")
  message(FATAL_ERROR
    "Installed-package smoke could not find the profile manifest: ${installed_profile_manifest}"
  )
endif()
file(READ "${installed_profile_manifest}" profile_json)
string(JSON profile_schema ERROR_VARIABLE profile_json_error GET "${profile_json}" schema)
if(profile_json_error OR NOT profile_schema STREQUAL "1")
  message(FATAL_ERROR "Installed Umbra profile manifest has no schema=1.")
endif()
string(JSON profile_provider ERROR_VARIABLE profile_json_error GET "${profile_json}" provider)
string(JSON profile_standard ERROR_VARIABLE profile_json_error GET "${profile_json}" standard)
string(JSON profile_name ERROR_VARIABLE profile_json_error GET "${profile_json}" profile)
string(JSON process_endpoint ERROR_VARIABLE profile_json_error GET
  "${profile_json}" public_configuration process_endpoint
)
string(JSON report_storage ERROR_VARIABLE profile_json_error GET
  "${profile_json}" public_configuration service_report_storage
)
if(profile_json_error OR
   NOT profile_provider STREQUAL "Umbra" OR
   NOT profile_standard STREQUAL "IEEE 1516.1-2025" OR
   "${profile_name}" STREQUAL "" OR
   NOT report_storage STREQUAL "filesystem")
  message(FATAL_ERROR "Installed Umbra profile manifest has invalid provider, standard, profile, or storage fields.")
endif()
if(profile_name STREQUAL "embedded-federation-management")
  if(NOT process_endpoint STREQUAL "rtiAddress=tcp://host:port")
    message(FATAL_ERROR "Embedded Umbra profile does not advertise the tcp RtiConfiguration endpoint contract.")
  endif()
elseif(profile_name STREQUAL "binding-foundation")
  if(NOT process_endpoint STREQUAL "not-enabled")
    message(FATAL_ERROR "Foundation-only Umbra profile must not advertise a process endpoint.")
  endif()
else()
  message(FATAL_ERROR "Installed Umbra profile manifest contains an unknown profile: ${profile_name}")
endif()

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
if(DEFINED UMBRA_EXPECT_PROCESS_PROFILE AND
   "${UMBRA_EXPECT_PROCESS_PROFILE}" STREQUAL "ON")
  list(APPEND configure_command
    "-DUMBRA_PACKAGE_PROCESS_SMOKE=ON"
    "-DUMBRA_PROCESS_SERVICE_PROBE_PATH=${process_probe_path}"
  )
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

# The embedded profile exposes eight independently runnable process-package
# projections.  Verify their exact CTest names and labels against the indexed
# roadmap before executing the consumer; this catches a lost/renamed lane or
# stale traceability handle without resyncing the Requirements Lab.
if(DEFINED UMBRA_EXPECT_PROCESS_PROFILE AND
   "${UMBRA_EXPECT_PROCESS_PROFILE}" STREQUAL "ON")
  set(package_lane_catalog_command
    "${UMBRA_PYTHON_EXECUTABLE}"
    "${UMBRA_SOURCE_DIRECTORY}/tools/verify_process_package_lanes.py"
    --ctest "${UMBRA_CTEST_COMMAND}"
    --test-dir "${consumer_binary_directory}"
    --index "${UMBRA_SOURCE_DIRECTORY}/docs/planning/ROADMAP-INDEX.json"
  )
  if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
    list(APPEND package_lane_catalog_command --config "${UMBRA_CONFIGURATION}")
  endif()
  umbra_run_checked(${package_lane_catalog_command})
endif()

set(ctest_command
  "${UMBRA_CTEST_COMMAND}"
  --test-dir "${consumer_binary_directory}"
  --output-on-failure
)
if(DEFINED UMBRA_CONFIGURATION AND NOT "${UMBRA_CONFIGURATION}" STREQUAL "")
  list(APPEND ctest_command -C "${UMBRA_CONFIGURATION}")
endif()
umbra_run_checked(${ctest_command})
