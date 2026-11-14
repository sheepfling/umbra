if(NOT DEFINED umbra_rti_ABI_MAJOR_VERSION OR
   "${umbra_rti_ABI_MAJOR_VERSION}" STREQUAL "")
  message(FATAL_ERROR "The installed package does not expose its ABI major version.")
endif()

if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES AND
   CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   NOT "${umbra_rti_HLA_COMPILER_VERSION_MARKER_SOURCE}" STREQUAL "user-configured")
  string(REGEX MATCH "^[0-9]+" _umbra_consumer_gcc_major
    "${CMAKE_CXX_COMPILER_VERSION}")
  if(_umbra_consumer_gcc_major GREATER_EQUAL 12)
    set(_umbra_expected_gcc_marker "gcc12")
    set(_umbra_expected_gcc_marker_source "SISO-REF-080-2023")
  elseif(_umbra_consumer_gcc_major GREATER_EQUAL 5 AND
         _umbra_consumer_gcc_major LESS_EQUAL 11)
    set(_umbra_expected_gcc_marker "gcc${_umbra_consumer_gcc_major}")
    set(_umbra_expected_gcc_marker_source "SISO-REF-080-2023")
  else()
    set(_umbra_expected_gcc_marker "gcc${_umbra_consumer_gcc_major}")
    set(_umbra_expected_gcc_marker_source "umbra-local-extension")
  endif()

  if(NOT "${umbra_rti_HLA_COMPILER_VERSION_MARKER}" STREQUAL
         "${_umbra_expected_gcc_marker}" OR
     NOT "${umbra_rti_HLA_COMPILER_VERSION_MARKER_SOURCE}" STREQUAL
         "${_umbra_expected_gcc_marker_source}")
    message(FATAL_ERROR
      "The shared package GCC marker '${umbra_rti_HLA_COMPILER_VERSION_MARKER}' "
      "(${umbra_rti_HLA_COMPILER_VERSION_MARKER_SOURCE}) does not match "
      "${_umbra_expected_gcc_marker} (${_umbra_expected_gcc_marker_source}) "
      "for GCC ${CMAKE_CXX_COMPILER_VERSION}.")
  endif()
endif()

foreach(_umbra_target IN ITEMS umbra::rti umbra::fedtime umbra::authorizer)
  if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES)
    if(NOT DEFINED umbra_rti_HLA_COMPILER_VERSION_MARKER OR
       "${umbra_rti_HLA_COMPILER_VERSION_MARKER}" STREQUAL "")
      message(FATAL_ERROR
        "The shared HLA package must expose its compiler-version marker.")
    endif()
    set(_umbra_hla_library_suffix
      "1516_2023${umbra_rti_HLA_COMPILER_VERSION_MARKER}${umbra_rti_HLA_LIBRARY_32BIT_MARKER}")
  else()
    set(_umbra_hla_library_suffix "")
  endif()

  if(_umbra_target STREQUAL "umbra::rti")
    if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES)
      set(_umbra_expected_library_name "rti${_umbra_hla_library_suffix}")
    else()
      set(_umbra_expected_library_name "umbra_rti")
    endif()
    set(_umbra_package_library_name "${umbra_rti_RTI_LIBRARY_NAME}")
  elseif(_umbra_target STREQUAL "umbra::fedtime")
    if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES)
      set(_umbra_expected_library_name "fedtime${_umbra_hla_library_suffix}")
    else()
      set(_umbra_expected_library_name "umbra_fedtime")
    endif()
    set(_umbra_package_library_name "${umbra_rti_FEDTIME_LIBRARY_NAME}")
  else()
    if(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES)
      set(_umbra_expected_library_name "authorizer${_umbra_hla_library_suffix}")
    else()
      set(_umbra_expected_library_name "umbra_authorizer")
    endif()
    set(_umbra_package_library_name "${umbra_rti_AUTHORIZER_LIBRARY_NAME}")
  endif()

  get_target_property(_umbra_abi_major
    ${_umbra_target} INTERFACE_UMBRA_ABI_MAJOR_VERSION
  )
  if(NOT "${_umbra_package_library_name}" STREQUAL "${_umbra_expected_library_name}")
    message(FATAL_ERROR
      "${_umbra_target} has unstable library-name metadata '${_umbra_package_library_name}'."
    )
  endif()
  if(NOT "${_umbra_abi_major}" STREQUAL "${umbra_rti_ABI_MAJOR_VERSION}")
    message(FATAL_ERROR
      "${_umbra_target} ABI metadata '${_umbra_abi_major}' does not match package ABI major '${umbra_rti_ABI_MAJOR_VERSION}'."
    )
  endif()
endforeach()
