include_guard(GLOBAL)

function(umbra_stage_windows_runtime_dependencies target_name)
  if(NOT WIN32)
    return()
  endif()

  get_target_property(_target_type "${target_name}" TYPE)
  if(NOT _target_type MATCHES "^(EXECUTABLE|SHARED_LIBRARY|MODULE_LIBRARY)$")
    return()
  endif()

  get_target_property(_staging_applied "${target_name}"
    UMBRA_WINDOWS_RUNTIME_DEPENDENCY_STAGING_APPLIED)
  if(_staging_applied)
    return()
  endif()

  set(_search_directories ${ARGN})
  if(OPENSSL_ROOT_DIR)
    list(APPEND _search_directories
      "${OPENSSL_ROOT_DIR}/bin"
      "${OPENSSL_ROOT_DIR}/c/bin"
    )
  endif()
  list(REMOVE_DUPLICATES _search_directories)

  add_custom_command(TARGET "${target_name}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
      "-DUMBRA_RUNTIME_BINARY=$<TARGET_FILE:${target_name}>"
      "-DUMBRA_RUNTIME_DESTINATION=$<TARGET_FILE_DIR:${target_name}>"
      "-DUMBRA_RUNTIME_DLLS=$<JOIN:$<TARGET_RUNTIME_DLLS:${target_name}>,@>"
      "-DUMBRA_RUNTIME_SEARCH_DIRECTORIES=$<JOIN:${_search_directories},@>"
      -P "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/stage_windows_runtime_dependencies_script.cmake"
    COMMENT "Staging Windows runtime dependencies for ${target_name}"
    VERBATIM
  )
  set(_smoke_script
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../packages/hla-rti-cpp-tck/cmake/smoke_windows_executable.cmake")
  if(_target_type STREQUAL "EXECUTABLE" AND "${target_name}" MATCHES "_catch2$")
    add_custom_command(TARGET "${target_name}" POST_BUILD
      COMMAND "${CMAKE_COMMAND}"
        "-DUMBRA_SMOKE_EXECUTABLE=$<TARGET_FILE:${target_name}>"
        "-DUMBRA_SMOKE_ARGUMENTS=--help"
        "-DUMBRA_SMOKE_RUNTIME_DIRECTORIES="
        -P "${_smoke_script}"
      COMMENT "Checking ${target_name} with a restricted Windows DLL search path"
      VERBATIM
    )
  endif()
  set_property(TARGET "${target_name}" APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/stage_windows_runtime_dependencies_script.cmake")
  if(_target_type STREQUAL "EXECUTABLE" AND "${target_name}" MATCHES "_catch2$")
    set_property(TARGET "${target_name}" APPEND PROPERTY LINK_DEPENDS
      "${_smoke_script}")
  endif()
  set_property(TARGET "${target_name}" PROPERTY
    UMBRA_WINDOWS_RUNTIME_DEPENDENCY_STAGING_APPLIED TRUE)
endfunction()
