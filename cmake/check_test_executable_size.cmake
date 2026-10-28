# Shared post-link budget for routine MinGW test executables.
if(NOT DEFINED UMBRA_TEST_EXECUTABLE OR
   NOT EXISTS "${UMBRA_TEST_EXECUTABLE}")
  message(FATAL_ERROR "MinGW test image budget: executable is missing")
endif()
if(NOT DEFINED UMBRA_TEST_OBJDUMP OR NOT EXISTS "${UMBRA_TEST_OBJDUMP}")
  message(FATAL_ERROR "MinGW test image budget: objdump is missing")
endif()

# Routine test images are deliberately small; full-symbol investigation uses
# the separate mingw-focused-full-debug-static preset.
set(_umbra_max_image_bytes 134217728) # 128 MiB
set(_umbra_max_debug_bytes 16777216) # 16 MiB
file(SIZE "${UMBRA_TEST_EXECUTABLE}" _umbra_image_bytes)

execute_process(
  COMMAND "${UMBRA_TEST_OBJDUMP}" -h "${UMBRA_TEST_EXECUTABLE}"
  RESULT_VARIABLE _umbra_objdump_result
  OUTPUT_VARIABLE _umbra_sections
  ERROR_VARIABLE _umbra_objdump_error
)
if(NOT _umbra_objdump_result EQUAL 0)
  message(FATAL_ERROR
    "MinGW test image budget could not inspect sections: ${_umbra_objdump_error}")
endif()

set(_umbra_debug_bytes 0)
string(REPLACE "\r" "" _umbra_sections "${_umbra_sections}")
string(REPLACE "\n" ";" _umbra_section_lines "${_umbra_sections}")
foreach(_umbra_line IN LISTS _umbra_section_lines)
  if(_umbra_line MATCHES "^[ \t]*[0-9]+[ \t]+\\.debug_[^ \t]+[ \t]+([0-9A-Fa-f]+)[ \t]")
    math(EXPR _umbra_debug_bytes "${_umbra_debug_bytes} + 0x${CMAKE_MATCH_1}")
  endif()
endforeach()

if(_umbra_image_bytes GREATER _umbra_max_image_bytes OR
   _umbra_debug_bytes GREATER _umbra_max_debug_bytes)
  message(FATAL_ERROR
    "MinGW test executable exceeds routine budget: ${UMBRA_TEST_EXECUTABLE} "
    "(${_umbra_image_bytes} image bytes; ${_umbra_debug_bytes} DWARF bytes). "
    "Limits: ${_umbra_max_image_bytes} image bytes and "
    "${_umbra_max_debug_bytes} DWARF bytes. Use the explicit "
    "mingw-focused-full-debug-static preset for full-symbol investigation.")
endif()
message(STATUS
  "MinGW test image budget: ${_umbra_image_bytes} bytes total, "
  "${_umbra_debug_bytes} bytes DWARF")
