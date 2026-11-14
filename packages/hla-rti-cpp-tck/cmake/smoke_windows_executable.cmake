if(NOT WIN32)
  message(FATAL_ERROR "Windows executable smoke checks can only run on Windows")
endif()

foreach(_required_variable IN ITEMS UMBRA_SMOKE_EXECUTABLE)
  if(NOT DEFINED ${_required_variable})
    message(FATAL_ERROR "${_required_variable} must be provided")
  endif()
endforeach()

if(NOT DEFINED UMBRA_SMOKE_ARGUMENTS)
  set(UMBRA_SMOKE_ARGUMENTS "--help")
endif()
string(REPLACE "@" ";" _umbra_runtime_directories
  "${UMBRA_SMOKE_RUNTIME_DIRECTORIES}")

set(_umbra_system_root "$ENV{SystemRoot}")
if(NOT _umbra_system_root)
  set(_umbra_system_root "C:/Windows")
endif()
set(_umbra_path_entries
  ${_umbra_runtime_directories}
  "${_umbra_system_root}/System32"
  "${_umbra_system_root}"
)
list(REMOVE_DUPLICATES _umbra_path_entries)
list(JOIN _umbra_path_entries ";" _umbra_clean_path)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "PATH=${_umbra_clean_path}"
    "${UMBRA_SMOKE_EXECUTABLE}" "${UMBRA_SMOKE_ARGUMENTS}"
  RESULT_VARIABLE _umbra_smoke_result
  OUTPUT_VARIABLE _umbra_smoke_stdout
  ERROR_VARIABLE _umbra_smoke_stderr
)
if(NOT _umbra_smoke_result STREQUAL "0")
  message(FATAL_ERROR
    "Windows runtime smoke failed for ${UMBRA_SMOKE_EXECUTABLE} ${UMBRA_SMOKE_ARGUMENTS} (result ${_umbra_smoke_result}).\n"
    "stdout:\n${_umbra_smoke_stdout}\n"
    "stderr:\n${_umbra_smoke_stderr}")
endif()
