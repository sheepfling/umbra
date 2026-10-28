include_guard(GLOBAL)

function(umbra_guard_test_executable target_name)
  if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
     UMBRA_FULL_DEBUG_TEST_EXECUTABLES)
    return()
  endif()
  get_target_property(_umbra_budget_applied "${target_name}"
    UMBRA_TEST_IMAGE_BUDGET_APPLIED)
  if(_umbra_budget_applied)
    return()
  endif()
  if(NOT CMAKE_OBJDUMP OR NOT EXISTS "${CMAKE_OBJDUMP}")
    message(FATAL_ERROR "MinGW test image budget requires CMAKE_OBJDUMP")
  endif()

  # Strip at link time: a post-link strip alone can run out of space while
  # producing the temporary, statically linked image.
  target_link_options("${target_name}" PRIVATE "LINKER:--strip-debug")
  add_custom_command(TARGET "${target_name}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DUMBRA_TEST_EXECUTABLE=$<TARGET_FILE:${target_name}>"
      "-DUMBRA_TEST_OBJDUMP=${CMAKE_OBJDUMP}"
      -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/check_test_executable_size.cmake"
    COMMENT "Checking MinGW test executable image and DWARF budgets"
    VERBATIM
  )
  set_property(TARGET "${target_name}" PROPERTY
    UMBRA_TEST_IMAGE_BUDGET_APPLIED TRUE)
endfunction()
