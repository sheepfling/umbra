cmake_minimum_required(VERSION 3.23)

foreach(required_variable IN ITEMS
    UMBRA_PACKAGE_CONSUMER_DIRECTORY
    UMBRA_PACKAGE_CONSUMER_FILENAME
    UMBRA_PACKAGE_PRIMARY_PROVIDER_PREFIX
    UMBRA_PACKAGE_PRIMARY_RTI_RUNTIME
    UMBRA_PACKAGE_PRIMARY_FEDTIME_RUNTIME
    UMBRA_PACKAGE_PRIMARY_AUTHORIZER_RUNTIME
    UMBRA_PACKAGE_INSTALL_BINDIR
    UMBRA_PACKAGE_INSTALL_LIBDIR)
  if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
    message(FATAL_ERROR "${required_variable} is required for the runtime-provider switch smoke")
  endif()
endforeach()

if(NOT DEFINED UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX OR
   "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}" STREQUAL "")
  message("SKIP: alternate provider install prefix is not configured; the two-provider ABI proof was not run.")
  return()
endif()

if(NOT IS_DIRECTORY "${UMBRA_PACKAGE_CONSUMER_DIRECTORY}")
  message(FATAL_ERROR "The package consumer build directory is missing: ${UMBRA_PACKAGE_CONSUMER_DIRECTORY}")
endif()
file(GLOB_RECURSE consumer_candidates
  LIST_DIRECTORIES FALSE
  "${UMBRA_PACKAGE_CONSUMER_DIRECTORY}/${UMBRA_PACKAGE_CONSUMER_FILENAME}"
)
list(LENGTH consumer_candidates consumer_candidate_count)
if(NOT consumer_candidate_count EQUAL 1)
  message(FATAL_ERROR
    "Expected one unchanged package consumer executable named ${UMBRA_PACKAGE_CONSUMER_FILENAME} "
    "beneath ${UMBRA_PACKAGE_CONSUMER_DIRECTORY}; found ${consumer_candidate_count}."
  )
endif()
list(GET consumer_candidates 0 UMBRA_PACKAGE_CONSUMER)

if(NOT IS_DIRECTORY "${UMBRA_PACKAGE_PRIMARY_PROVIDER_PREFIX}")
  message(FATAL_ERROR "Primary provider install prefix does not exist: ${UMBRA_PACKAGE_PRIMARY_PROVIDER_PREFIX}")
endif()
if(NOT IS_DIRECTORY "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}")
  message(FATAL_ERROR "Alternate provider install prefix does not exist: ${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}")
endif()
if(NOT EXISTS "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}/include/RTI/RTI1516.h")
  message(FATAL_ERROR
    "Alternate provider does not install the official 1516.1 C++ header tree at include/RTI: "
    "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}"
  )
endif()

function(umbra_provider_runtime_directories prefix output_variable)
  set(candidate_directories
    "${prefix}/${UMBRA_PACKAGE_INSTALL_BINDIR}"
    "${prefix}/${UMBRA_PACKAGE_INSTALL_LIBDIR}"
    "${prefix}/bin"
    "${prefix}/lib"
    "${prefix}/lib64"
  )
  set(runtime_directories)
  foreach(candidate_directory IN LISTS candidate_directories)
    if(IS_DIRECTORY "${candidate_directory}")
      list(APPEND runtime_directories "${candidate_directory}")
    endif()
  endforeach()
  if(NOT runtime_directories)
    message(FATAL_ERROR "No runtime library directory was found beneath provider prefix: ${prefix}")
  endif()
  set(${output_variable} "${runtime_directories}" PARENT_SCOPE)
endfunction()

umbra_provider_runtime_directories(
  "${UMBRA_PACKAGE_PRIMARY_PROVIDER_PREFIX}"
  primary_runtime_directories
)
umbra_provider_runtime_directories(
  "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}"
  alternate_runtime_directories
)

function(umbra_find_matching_provider_library prefix runtime_directories primary_library output_variable)
  if(NOT EXISTS "${primary_library}")
    message(FATAL_ERROR "Primary provider runtime library is missing: ${primary_library}")
  endif()
  get_filename_component(library_filename "${primary_library}" NAME)
  foreach(runtime_directory IN LISTS runtime_directories)
    if(EXISTS "${runtime_directory}/${library_filename}")
      set(${output_variable} "${runtime_directory}/${library_filename}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  message(FATAL_ERROR
    "Alternate provider is not a drop-in runtime replacement: it lacks ${library_filename} beneath ${prefix}. "
    "The unchanged consumer imports the primary provider's exact library name."
  )
endfunction()

umbra_find_matching_provider_library(
  "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}"
  "${alternate_runtime_directories}"
  "${UMBRA_PACKAGE_PRIMARY_RTI_RUNTIME}"
  alternate_rti_runtime
)
umbra_find_matching_provider_library(
  "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}"
  "${alternate_runtime_directories}"
  "${UMBRA_PACKAGE_PRIMARY_FEDTIME_RUNTIME}"
  alternate_fedtime_runtime
)
umbra_find_matching_provider_library(
  "${UMBRA_PACKAGE_ALTERNATE_PROVIDER_PREFIX}"
  "${alternate_runtime_directories}"
  "${UMBRA_PACKAGE_PRIMARY_AUTHORIZER_RUNTIME}"
  alternate_authorizer_runtime
)

file(SHA256 "${UMBRA_PACKAGE_PRIMARY_RTI_RUNTIME}" primary_rti_digest)
file(SHA256 "${alternate_rti_runtime}" alternate_rti_digest)

if(WIN32)
  set(runtime_environment_variable PATH)
  set(runtime_path_separator ";")
elseif(APPLE)
  set(runtime_environment_variable DYLD_LIBRARY_PATH)
  set(runtime_path_separator ":")
else()
  set(runtime_environment_variable LD_LIBRARY_PATH)
  set(runtime_path_separator ":")
endif()
set(original_runtime_path "$ENV{${runtime_environment_variable}}")

function(umbra_set_provider_runtime_path runtime_directories)
  string(JOIN "${runtime_path_separator}" provider_runtime_path ${runtime_directories})
  if(NOT "${original_runtime_path}" STREQUAL "")
    string(APPEND provider_runtime_path "${runtime_path_separator}${original_runtime_path}")
  endif()
  set(ENV{${runtime_environment_variable}} "${provider_runtime_path}")
endfunction()

function(umbra_capture_provider_identity label runtime_directories output_variable)
  umbra_set_provider_runtime_path("${runtime_directories}")
  execute_process(
    COMMAND "${UMBRA_PACKAGE_CONSUMER}" --provider-identity
    RESULT_VARIABLE result
    OUTPUT_VARIABLE identity
    ERROR_VARIABLE error
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT result EQUAL 0 OR "${identity}" STREQUAL "")
    message(FATAL_ERROR
      "Could not obtain ${label} runtime identity from the unchanged consumer (result=${result}).\n${error}"
    )
  endif()
  message(STATUS "${label} provider identity fingerprint: ${identity}")
  set(${output_variable} "${identity}" PARENT_SCOPE)
endfunction()

umbra_capture_provider_identity(
  "Primary"
  "${primary_runtime_directories}"
  primary_provider_identity
)
umbra_capture_provider_identity(
  "Alternate"
  "${alternate_runtime_directories}"
  alternate_provider_identity
)
if(primary_provider_identity STREQUAL alternate_provider_identity)
  message(FATAL_ERROR
    "The unchanged consumer reported the same rtiName()/rtiVersion() identity for both runtime paths. "
    "The alternate provider was not independently observed."
  )
endif()
if(primary_rti_digest STREQUAL alternate_rti_digest)
  message(FATAL_ERROR
    "The alternate provider's RTI runtime is byte-identical to the primary provider; "
    "this is not evidence of a distinct RTI implementation."
  )
endif()

# The same executable was compiled and linked once against the primary
# provider.  Running its complete package smoke with the alternate provider
# first in the loader path exercises imported symbols from RTI, fedtime, and
# authorizer without recompiling or relinking the consumer.
umbra_set_provider_runtime_path("${alternate_runtime_directories}")
execute_process(
  COMMAND "${UMBRA_PACKAGE_CONSUMER}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR
    "The unchanged consumer failed against the alternate provider (result=${result}).\n${output}\n${error}"
  )
endif()
message(STATUS
  "Unchanged consumer passed against alternate provider; RTI runtime digests differ "
  "(${primary_rti_digest} vs ${alternate_rti_digest})."
)
