if(NOT DEFINED UMBRACATCH2_EXECUTABLE OR
   NOT EXISTS "${UMBRACATCH2_EXECUTABLE}")
  message(FATAL_ERROR "The Catch2 executable for the MinGW size guard is missing.")
endif()
if(NOT DEFINED UMBRACATCH2_STRIP OR
   NOT EXISTS "${UMBRACATCH2_STRIP}")
  message(FATAL_ERROR "The MinGW strip executable for the Catch2 size guard is missing.")
endif()

file(SIZE "${UMBRACATCH2_EXECUTABLE}" _umbra_catch2_image_size)
set(_umbra_catch2_debug_strip_threshold 1610612736)
if(_umbra_catch2_image_size LESS_EQUAL _umbra_catch2_debug_strip_threshold)
  return()
endif()

execute_process(
  COMMAND "${UMBRACATCH2_STRIP}" --strip-debug "${UMBRACATCH2_EXECUTABLE}"
  RESULT_VARIABLE _umbra_catch2_strip_result
  OUTPUT_VARIABLE _umbra_catch2_strip_output
  ERROR_VARIABLE _umbra_catch2_strip_error
)
if(NOT _umbra_catch2_strip_result EQUAL 0)
  message(FATAL_ERROR
    "Could not strip DWARF from oversized Catch2 executable: "
    "${_umbra_catch2_strip_output}${_umbra_catch2_strip_error}"
  )
endif()

file(SIZE "${UMBRACATCH2_EXECUTABLE}" _umbra_catch2_stripped_size)
if(_umbra_catch2_stripped_size GREATER _umbra_catch2_debug_strip_threshold)
  message(FATAL_ERROR
    "Catch2 executable remains too large after DWARF removal: "
    "${_umbra_catch2_stripped_size} bytes."
  )
endif()
message(STATUS
  "Removed oversized Catch2 DWARF (${_umbra_catch2_image_size} -> "
  "${_umbra_catch2_stripped_size} bytes)."
)
