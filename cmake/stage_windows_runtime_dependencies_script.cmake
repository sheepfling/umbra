if(NOT WIN32)
  message(FATAL_ERROR "Windows runtime dependency staging can only run on Windows")
endif()
if(POLICY CMP0207)
  cmake_policy(SET CMP0207 NEW)
endif()
foreach(_required_variable IN ITEMS
    UMBRA_RUNTIME_BINARY
    UMBRA_RUNTIME_DESTINATION)
  if(NOT DEFINED ${_required_variable})
    message(FATAL_ERROR "${_required_variable} must be provided")
  endif()
endforeach()

string(REPLACE "@" ";" _umbra_runtime_dlls "${UMBRA_RUNTIME_DLLS}")
string(REPLACE "@" ";" _umbra_runtime_search_directories
  "${UMBRA_RUNTIME_SEARCH_DIRECTORIES}")
set(_umbra_search_directories
  "${UMBRA_RUNTIME_DESTINATION}"
  ${_umbra_runtime_search_directories}
)
foreach(_umbra_runtime_dll IN LISTS _umbra_runtime_dlls)
  if(EXISTS "${_umbra_runtime_dll}")
    get_filename_component(_umbra_runtime_dll_directory
      "${_umbra_runtime_dll}" DIRECTORY)
    list(APPEND _umbra_search_directories "${_umbra_runtime_dll_directory}")
  endif()
endforeach()
list(REMOVE_DUPLICATES _umbra_search_directories)

get_filename_component(_umbra_runtime_extension "${UMBRA_RUNTIME_BINARY}" EXT)
if(_umbra_runtime_extension STREQUAL ".dll")
  set(_umbra_runtime_scan_inputs MODULES "${UMBRA_RUNTIME_BINARY}")
else()
  set(_umbra_runtime_scan_inputs EXECUTABLES "${UMBRA_RUNTIME_BINARY}")
endif()

file(GET_RUNTIME_DEPENDENCIES
  ${_umbra_runtime_scan_inputs}
  DIRECTORIES ${_umbra_search_directories}
  RESOLVED_DEPENDENCIES_VAR _umbra_resolved_dependencies
  UNRESOLVED_DEPENDENCIES_VAR _umbra_unresolved_dependencies
  CONFLICTING_DEPENDENCIES_PREFIX _umbra_conflicting_dependencies
  PRE_EXCLUDE_REGEXES
    "[Aa][Pp][Ii]-[Mm][Ss]-[Ww][Ii][Nn]-.*"
    "[Ee][Xx][Tt]-[Mm][Ss]-[Ww][Ii][Nn]-.*"
  POST_EXCLUDE_REGEXES
    ".*[/\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\].*"
    ".*[/\\][Ww][Ii][Nn][Ss][Xx][Ss][/\\].*"
)

if(_umbra_unresolved_dependencies)
  list(JOIN _umbra_unresolved_dependencies "\n  " _umbra_unresolved_display)
  message(FATAL_ERROR
    "Could not resolve Windows runtime dependencies of ${UMBRA_RUNTIME_BINARY}:\n  ${_umbra_unresolved_display}\n"
    "Add their directories to the target's runtime dependency search paths.")
endif()
if(_umbra_conflicting_dependencies_FILENAMES)
  list(JOIN _umbra_conflicting_dependencies_FILENAMES ", "
    _umbra_conflicting_display)
  message(FATAL_ERROR
    "Conflicting Windows runtime dependencies of ${UMBRA_RUNTIME_BINARY}: ${_umbra_conflicting_display}")
endif()

foreach(_umbra_dependency IN LISTS _umbra_resolved_dependencies)
  get_filename_component(_umbra_dependency_name "${_umbra_dependency}" NAME)
  set(_umbra_staged_dependency
    "${UMBRA_RUNTIME_DESTINATION}/${_umbra_dependency_name}")
  file(REAL_PATH "${_umbra_dependency}" _umbra_dependency_real_path)
  if(EXISTS "${_umbra_staged_dependency}")
    file(REAL_PATH "${_umbra_staged_dependency}" _umbra_staged_dependency_real_path)
    if(_umbra_dependency_real_path STREQUAL _umbra_staged_dependency_real_path)
      continue()
    endif()
  endif()
  file(COPY_FILE "${_umbra_dependency}" "${_umbra_staged_dependency}"
    ONLY_IF_DIFFERENT)
endforeach()
