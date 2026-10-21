if(NOT DEFINED umbra_rti_ABI_MAJOR_VERSION OR
   "${umbra_rti_ABI_MAJOR_VERSION}" STREQUAL "")
  message(FATAL_ERROR "The installed package does not expose its ABI major version.")
endif()

foreach(_umbra_target IN ITEMS umbra::rti umbra::fedtime umbra::authorizer)
  if(_umbra_target STREQUAL "umbra::rti")
    set(_umbra_expected_library_name umbra_rti)
    set(_umbra_package_library_name "${umbra_rti_RTI_LIBRARY_NAME}")
  elseif(_umbra_target STREQUAL "umbra::fedtime")
    set(_umbra_expected_library_name umbra_fedtime)
    set(_umbra_package_library_name "${umbra_rti_FEDTIME_LIBRARY_NAME}")
  else()
    set(_umbra_expected_library_name umbra_authorizer)
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
