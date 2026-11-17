cmake_minimum_required(VERSION 3.20)

project(hla_rti_cpp_tck_installed_package_consumer LANGUAGES CXX)

# The adapter is intentionally driven only by its explicit prefix. Do not
# fall back to either CMake package registry while resolving the installed API.
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY FALSE)
find_package(umbra_rti CONFIG REQUIRED)

foreach(_required_target IN ITEMS
    umbra::ieee1516_2025_headers
    umbra::rti
    umbra::fedtime
    umbra::authorizer)
  if(NOT TARGET "${_required_target}")
    message(FATAL_ERROR
      "The installed provider package did not export ${_required_target}")
  endif()
endforeach()

get_target_property(_installed_api_include
  umbra::ieee1516_2025_headers INTERFACE_INCLUDE_DIRECTORIES)
if(NOT _installed_api_include OR _installed_api_include MATCHES "-NOTFOUND")
  message(FATAL_ERROR
    "The installed provider package did not export the official API include directory")
endif()
list(GET _installed_api_include 0 _installed_api_include)

get_filename_component(_repository_root
  "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
get_target_property(_current_package_rti_location
  umbra::rti IMPORTED_LOCATION_DEBUG)
if(NOT _current_package_rti_location)
  get_target_property(_current_package_rti_location
    umbra::rti IMPORTED_LOCATION)
endif()
get_target_property(_current_package_rti_type umbra::rti TYPE)
if(_current_package_rti_type STREQUAL "SHARED_LIBRARY")
  set(UMBRA_PACKAGE_EXPECT_SHARED_LIBRARIES ON)
  include("${_repository_root}/cmake/package-smoke/abi_policy.cmake")
endif()
set(_custom_tradt_provider_enabled FALSE)
set(_custom_authorizer_provider_enabled FALSE)
if(_current_package_rti_location MATCHES "\\.(dll|so|dylib)$")
  add_library(umbra_cpp_tck_custom_tradt_provider SHARED
    custom_tradt_provider.cpp)
  target_include_directories(umbra_cpp_tck_custom_tradt_provider PRIVATE
    "${_repository_root}/cpp/src")
  target_link_libraries(umbra_cpp_tck_custom_tradt_provider PRIVATE umbra::rti)
  target_compile_features(umbra_cpp_tck_custom_tradt_provider PRIVATE cxx_std_20)
  if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(umbra_cpp_tck_custom_tradt_provider PRIVATE
      "_int64=long long")
  endif()
  set(_custom_tradt_provider_enabled TRUE)
  set(HLA_RTI_TCK_CUSTOM_TRADT_PROVIDER_TARGET
    umbra_cpp_tck_custom_tradt_provider)
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/portable-tck-fedtime-provider-$<CONFIG>.txt"
    CONTENT "$<TARGET_FILE:${HLA_RTI_TCK_CUSTOM_TRADT_PROVIDER_TARGET}>\n")

  add_library(umbra_cpp_tck_custom_authorizer_provider SHARED
    custom_authorizer_provider.cpp)
  target_include_directories(umbra_cpp_tck_custom_authorizer_provider PRIVATE
    "${_repository_root}/cpp/src")
  target_link_libraries(umbra_cpp_tck_custom_authorizer_provider PRIVATE
    umbra::rti)
  target_compile_features(umbra_cpp_tck_custom_authorizer_provider PRIVATE
    cxx_std_20)
  if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(umbra_cpp_tck_custom_authorizer_provider PRIVATE
      "_int64=long long")
  endif()
  set(_custom_authorizer_provider_enabled TRUE)
  set(HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET
    umbra_cpp_tck_custom_authorizer_provider)
  file(GENERATE
    OUTPUT "${CMAKE_BINARY_DIR}/portable-tck-authorizer-provider-$<CONFIG>.txt"
    CONTENT "$<TARGET_FILE:${HLA_RTI_TCK_CUSTOM_AUTHORIZER_PROVIDER_TARGET}>\n")
endif()
set(HLA_RTI_TCK_API_INCLUDE_DIR "${_installed_api_include}" CACHE PATH
  "Official API headers exported by the installed provider package" FORCE)
set(HLA_RTI_TCK_PROVIDER_LIBRARIES
  "umbra::rti;umbra::fedtime;umbra::authorizer" CACHE STRING
  "Provider targets exported by the installed package" FORCE)
set(HLA_RTI_TCK_PROVIDER_COMPILE_DEFINITIONS "" CACHE STRING
  "Compile definitions are carried by the imported provider targets" FORCE)
set(HLA_RTI_TCK_PROVIDER_LINK_OPTIONS "" CACHE STRING
  "Link options are carried by the imported provider targets" FORCE)

set(HLA_RTI_TCK_ADAPTER_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml"
  CACHE FILEPATH "FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_SCHEMA_VALID_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/schema-valid-minimal-2025-tck.xml"
  CACHE FILEPATH "Official-XSD-valid 2025 FDD for Annex F TCK coverage")
set(HLA_RTI_TCK_ADAPTER_SCHEMA_INVALID_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/schema-invalid-reliable-enumeration-2025-tck.xml"
  CACHE FILEPATH "Well-formed, correctly namespaced XSD-invalid 2025 FDD for Annex F TCK coverage")
set(HLA_RTI_TCK_ADAPTER_MODEL_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-model-tck.xml"
  CACHE FILEPATH "Rich FOM model supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_DDM_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-tck.xml"
  CACHE FILEPATH "Dimensional FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-multi-attribute-tck.xml"
  CACHE FILEPATH "Multi-attribute dimensional FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-three-dimensional-tck.xml"
  CACHE FILEPATH "Three-dimensional DDM FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_MIM_FOM
  "${_repository_root}/third_party/ieee1516.2-2025/resources/mim/HLAstandardMIM-2025.xml"
  CACHE FILEPATH "Standard MIM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_SWITCHES_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/switches-tck.xml"
  CACHE FILEPATH "Switch-declaration FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/auto-provide-tck.xml"
  CACHE FILEPATH "Auto Provide FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONSUMER_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/data-type-merge-consumer-tck.xml"
  CACHE FILEPATH "Annex C.3 forward-reference consumer FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_PROVIDER_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/data-type-merge-provider-tck.xml"
  CACHE FILEPATH "Annex C.3 named data-type provider FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONFLICTING_PROVIDER_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/data-type-merge-conflicting-provider-tck.xml"
  CACHE FILEPATH "Annex C.3 conflicting named data-type provider FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_BASE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-base-tck.xml"
  CACHE FILEPATH "Annex C.3 extendable-variant base FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_DISJOINT_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-disjoint-tck.xml"
  CACHE FILEPATH "Annex C.3 disjoint variant-alternative FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_OVERLAP_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-overlap-tck.xml"
  CACHE FILEPATH "Annex C.3 overlapping variant-alternative FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_BASE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-parameter-merge-base-tck.xml"
  CACHE FILEPATH "Annex C.3 interaction-parameter base FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_DISJOINT_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-parameter-merge-disjoint-tck.xml"
  CACHE FILEPATH "Annex C.3 disjoint interaction-parameter FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_CONFLICT_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-parameter-merge-conflict-tck.xml"
  CACHE FILEPATH "Annex C.3 conflicting interaction-parameter FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_BASE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-merge-base-tck.xml"
  CACHE FILEPATH "Annex C.3 object-attribute base FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_DISJOINT_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-merge-disjoint-tck.xml"
  CACHE FILEPATH "Annex C.3 disjoint object-attribute FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_CONFLICT_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-merge-conflict-tck.xml"
  CACHE FILEPATH "Annex C.3 conflicting object-attribute FOM supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
  "object-attribute-subscription-independent-class=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-subscription-independent-class-tck.xml"
  "attribute-table-empty-semantics=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-table-empty-semantics-tck.xml"
  "empty-domain-tables=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-empty-domain-content-tck.xml"
  "official-restaurant-example=${_repository_root}/third_party/ieee1516.2-2025/resources/examples/RestaurantFOMmodule-2025.xml"
  "default-logical-time=${_repository_root}/packages/hla-rti-cpp-tck/fom/default-logical-time-tck.xml"
  "time-representation-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/time-representation-valid-tck.xml"
  "time-representation-valid-float64=${_repository_root}/packages/hla-rti-cpp-tck/fom/time-representation-float64-tck.xml"
  "delay-subscription-evaluation=${_repository_root}/packages/hla-rti-cpp-tck/fom/delay-subscription-evaluation-tck.xml"
  "support-switch-getter-lifecycle=${_repository_root}/packages/hla-rti-cpp-tck/fom/support-switch-getter-lifecycle-tck.xml"
  "service-usage-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/service-usage-first-tck.xml"
  "service-usage-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/service-usage-second-tck.xml"
  "reference-data-type-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-data-type-invalid-name-tck.xml"
  "reference-data-type-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-data-type-valid-name-tck.xml"
  "model-identification-name-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/model-identification-name-valid-tck.xml"
  "model-modification-date-timezone=${_repository_root}/packages/hla-rti-cpp-tck/fom/model-modification-date-timezone-tck.xml"
  "model-modification-date-exact=${_repository_root}/packages/hla-rti-cpp-tck/fom/model-modification-date-exact-tck.xml"
  "simple-data-type-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/simple-data-type-valid-name-tck.xml"
  "simple-data-type-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/simple-data-type-invalid-name-tck.xml"
  "simple-data-type-reserved-prefix-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/simple-data-type-reserved-prefix-name-tck.xml"
  "simple-data-type-reserved-na-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/simple-data-type-reserved-na-name-tck.xml"
  "tag-data-type-category-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/tag-data-type-category-valid-tck.xml"
  "tag-data-type-basic-user=${_repository_root}/packages/hla-rti-cpp-tck/fom/tag-data-type-basic-user-tck.xml"
  "tag-data-type-basic-synchronization=${_repository_root}/packages/hla-rti-cpp-tck/fom/tag-data-type-basic-synchronization-tck.xml"
  "tag-synchronization-capability-register=${_repository_root}/packages/hla-rti-cpp-tck/fom/tag-synchronization-capability-register-tck.xml"
  "enumerated-data-type-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-data-type-valid-name-tck.xml"
  "enumerated-data-type-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-data-type-invalid-name-tck.xml"
  "enumerator-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerator-valid-name-tck.xml"
  "enumerator-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerator-invalid-name-tck.xml"
  "variant-record-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-record-valid-name-tck.xml"
  "variant-record-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-record-invalid-name-tck.xml"
  "variant-discriminant-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-valid-name-tck.xml"
  "variant-discriminant-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-invalid-name-tck.xml"
  "variant-alternative-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-valid-name-tck.xml"
  "variant-alternative-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-invalid-name-tck.xml"
  "variant-alternative-type-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-type-valid-tck.xml"
  "variant-alternative-type-invalid=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-alternative-type-invalid-tck.xml"
  "variant-record-encoding-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-record-encoding-valid-tck.xml"
  "variant-record-encoding-invalid=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-record-encoding-invalid-tck.xml"
  "interaction-parameter-unresolved-data-type=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-parameter-unresolved-data-type-tck.xml"
  "interaction-transportation-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-transportation-conflict-tck.xml"
  "interaction-order-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-order-conflict-tck.xml"
  "interaction-order-invalid-value=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-order-invalid-value-tck.xml"
  "object-attribute-transportation-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-transportation-conflict-tck.xml"
  "object-attribute-order-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-attribute-order-conflict-tck.xml"
  "fom-root-classes-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-root-classes-omitted-tck.xml"
  "mom-table-attribute-transportation-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-table-attribute-transportation-conflict-tck.xml"
  "mom-category-c-interaction=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-category-c-interaction-tck.xml"
  "mom-report-service-invocation-extension-parameter=${_repository_root}/packages/hla-rti-cpp-tck/fom/service-report-invocation-extension-parameter-tck.xml"
  "mom-report-added-subclass-not-sent=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-report-added-subclass-tck.xml"
  "mom-federate-subclass-instance=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-federate-subclass-instance-tck.xml"
  "mom-federate-added-attribute-same-instance=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-federate-added-attribute-same-instance-tck.xml"
  "mom-set-switches-extra-parameter=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-set-switches-extra-parameter-tck.xml"
  "additional-join-duplicate=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-additional-duplicate-tck.xml"
  "additional-join-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-additional-order-conflict-tck.xml"
  "additional-join-interaction-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-additional-interaction-order-conflict-tck.xml"
  "additional-join-transport-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-additional-transportation-conflict-tck.xml"
  "fom-handle-uniqueness=${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-handle-uniqueness-tck.xml"
  "object-class-semantics-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-class-semantics-conflict-tck.xml"
  "matching-data-types-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/matching-data-types-first-tck.xml"
  "matching-data-types-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/matching-data-types-second-tck.xml"
  "object-dimension-merge-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-dimension-merge-first-tck.xml"
  "object-dimension-merge-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/object-dimension-merge-second-tck.xml"
  "interaction-dimension-merge-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-dimension-merge-first-tck.xml"
  "interaction-dimension-merge-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/interaction-dimension-merge-second-tck.xml"
  "named-variant-alternative-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/named-variant-alternative-first-tck.xml"
  "named-variant-alternative-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/named-variant-alternative-second-tck.xml"
  "extendable-hlaother-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/named-variant-alternative-first-tck.xml"
  "extendable-hlaother-invalid=${_repository_root}/packages/hla-rti-cpp-tck/fom/extendable-variant-hlaother-tck.xml"
  "enumerated-value-collision-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-value-collision-first-tck.xml"
  "enumerated-value-collision-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-value-collision-second-tck.xml"
  "enumerated-value-merge-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-value-merge-first-tck.xml"
  "enumerated-value-merge-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-value-merge-second-tck.xml"
  "enumerated-representation-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-representation-omitted-tck.xml"
  "enumerated-representation-nonbasic=${_repository_root}/packages/hla-rti-cpp-tck/fom/enumerated-representation-nonbasic-tck.xml"
  "variant-discriminant-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-valid-tck.xml"
  "variant-discriminant-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-omitted-tck.xml"
  "variant-discriminant-basic=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-basic-tck.xml"
  "variant-discriminant-simple=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-simple-tck.xml"
  "variant-discriminant-na=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-discriminant-na-tck.xml"
  "variant-enumerator-range=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-range-tck.xml"
  "variant-enumerator-hlaother=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-hlaother-tck.xml"
  "variant-enumerator-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-omitted-tck.xml"
  "variant-enumerator-malformed-range=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-malformed-range-tck.xml"
  "variant-enumerator-composite-hlaother=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-composite-hlaother-tck.xml"
  "variant-member-unknown=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-unknown-member-tck.xml"
  "variant-member-unknown-endpoint=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-unknown-endpoint-tck.xml"
  "variant-member-incomplete=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-incomplete-members-tck.xml"
  "variant-member-later-consumer=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-later-consumer-tck.xml"
  "variant-member-later-provider=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-enumerator-later-provider-tck.xml"
  "variant-range-order-base=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-range-order-base-tck.xml"
  "variant-range-order-valid-extension=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-range-order-valid-extension-tck.xml"
  "variant-range-order-conflict-extension=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-range-order-conflict-extension-tck.xml"
  "variant-range-other-complement=${_repository_root}/packages/hla-rti-cpp-tck/fom/variant-range-other-complement-tck.xml"
  "nonextendable-variant-first=${_repository_root}/packages/hla-rti-cpp-tck/fom/nonextendable-variant-first-tck.xml"
  "nonextendable-variant-second=${_repository_root}/packages/hla-rti-cpp-tck/fom/nonextendable-variant-second-tck.xml"
  "array-cardinality-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/valid-array-cardinality-forms-tck.xml"
  "array-data-type-valid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-data-type-valid-name-tck.xml"
  "array-data-type-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-data-type-invalid-name-tck.xml"
  "array-element-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-valid-tck.xml"
  "array-element-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-omitted-tck.xml"
  "array-element-basic=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-basic-tck.xml"
  "array-element-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-invalid-name-tck.xml"
  "array-element-type-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-type-valid-tck.xml"
  "array-element-type-invalid=${_repository_root}/packages/hla-rti-cpp-tck/fom/array-element-type-invalid-tck.xml"
  "array-cardinality-reversed-range=${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-array-cardinality-tck.xml"
  "array-cardinality-fixed-encoding-mismatch=${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-fixed-array-encoding-tck.xml"
  "array-cardinality-variable-encoding-mismatch=${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-variable-array-encoding-tck.xml"
  "record-member-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-valid-tck.xml"
  "record-member-encoding-mismatch=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-encoding-mismatch-tck.xml"
  "record-member-reserved-hla-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-reserved-hla-name-tck.xml"
  "record-member-reserved-na-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-reserved-na-name-tck.xml"
  "record-member-field-types=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-field-types-tck.xml"
  "record-member-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-omitted-tck.xml"
  "record-member-basic-field=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-basic-field-tck.xml"
  "record-member-basic-alternative=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-basic-alternative-tck.xml"
  "record-member-reserved-hla-field-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-reserved-hla-field-name-tck.xml"
  "record-member-reserved-na-field-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/record-member-reserved-na-field-name-tck.xml"
  "dimension-duplicate-base=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-duplicate-base-tck.xml"
  "dimension-duplicate-within-module=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-duplicate-within-module-tck.xml"
  "dimension-duplicate-equal-unique=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-duplicate-equal-unique-tck.xml"
  "dimension-duplicate-conflicting-upper-bound=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-duplicate-conflicting-upper-bound-tck.xml"
  "dimension-duplicate-unspecified-upper-bound=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-duplicate-unspecified-upper-bound-tck.xml"
  "dimension-default-range-outside=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-default-range-outside-tck.xml"
  "dimension-default-range-bounded=${_repository_root}/packages/hla-rti-cpp-tck/fom/dimension-default-range-bounded-tck.xml"
  "synchronization-duplicate-base=${_repository_root}/packages/hla-rti-cpp-tck/fom/synchronization-duplicate-base-tck.xml"
  "synchronization-duplicate-equal=${_repository_root}/packages/hla-rti-cpp-tck/fom/synchronization-duplicate-equal-tck.xml"
  "synchronization-duplicate-note-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/synchronization-duplicate-note-conflict-tck.xml"
  "transportation-duplicate-base=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-duplicate-base-tck.xml"
  "transportation-duplicate-equal=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-duplicate-equal-tck.xml"
  "transportation-description-different=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-description-different-tck.xml"
  "transportation-duplicate-note-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-duplicate-note-conflict-tck.xml"
  "transportation-duplicate-reliability-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-duplicate-reliability-conflict-tck.xml"
  "transportation-table-invalid-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-table-invalid-name-tck.xml"
  "transportation-table-invalid-reliability=${_repository_root}/packages/hla-rti-cpp-tck/fom/transportation-table-invalid-reliability-tck.xml"
  "update-rate-duplicate-base=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-duplicate-base-tck.xml"
  "update-rate-duplicate-equal=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-duplicate-equal-tck.xml"
  "update-rate-duplicate-note-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-duplicate-note-conflict-tck.xml"
  "update-rate-duplicate-rate-conflict=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-duplicate-rate-conflict-tck.xml"
  "update-rate-unique-merge=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-unique-merge-tck.xml"
  "update-rate-invalid-name-preflight=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-invalid-name-preflight-tck.xml"
  "update-rate-zero-preflight=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-zero-preflight-tck.xml"
  "update-rate-positive-preflight=${_repository_root}/packages/hla-rti-cpp-tck/fom/update-rate-positive-preflight-tck.xml"
  "member-data-type-valid=${_repository_root}/packages/hla-rti-cpp-tck/fom/member-data-type-valid-tck.xml"
  "member-data-type-attribute-basic=${_repository_root}/packages/hla-rti-cpp-tck/fom/member-data-type-attribute-basic-tck.xml"
  "member-data-type-parameter-basic=${_repository_root}/packages/hla-rti-cpp-tck/fom/member-data-type-parameter-basic-tck.xml"
  "attribute-na-valid-companions=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-valid-companions-tck.xml"
  "attribute-na-omitted-companions=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-omitted-companions-tck.xml"
  "attribute-na-omitted-companions-extension=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-omitted-companions-extension-tck.xml"
  "attribute-na-invalid-update-type=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-invalid-update-type-tck.xml"
  "attribute-na-invalid-update-condition=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-invalid-update-condition-tck.xml"
  "attribute-na-invalid-transportation=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-na-invalid-transportation-tck.xml"
  "reference-attribute-consumer=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-attribute-consumer-tck.xml"
  "reference-attribute-provider=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-attribute-provider-tck.xml"
  "reference-attribute-missing=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-attribute-missing-tck.xml"
  "reference-attribute-mismatched=${_repository_root}/packages/hla-rti-cpp-tck/fom/reference-attribute-mismatched-tck.xml"
  "representation-special=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-special-identifiers-tck.xml"
  "basic-data-representation-endian-big=${_repository_root}/packages/hla-rti-cpp-tck/fom/basic-data-representation-endian-big-tck.xml"
  "basic-data-representation-endian-little=${_repository_root}/packages/hla-rti-cpp-tck/fom/basic-data-representation-endian-little-tck.xml"
  "representation-missing-simple=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-missing-simple-tck.xml"
  "representation-missing-enumerated=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-missing-enumerated-tck.xml"
  "representation-invalid-special=${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-reference-object-name-tck.xml"
  "representation-invalid-basic=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-invalid-basic-reference-tck.xml"
  "representation-invalid-basic-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-invalid-basic-name-tck.xml"
  "representation-invalid-reference-name=${_repository_root}/packages/hla-rti-cpp-tck/fom/representation-invalid-reference-name-tck.xml"
  "switch-duplicate-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/switch-duplicate-disabled-tck.xml"
  "switch-duplicate-equal=${_repository_root}/packages/hla-rti-cpp-tck/fom/switch-duplicate-equal-tck.xml"
  "switch-duplicate-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/switch-duplicate-enabled-tck.xml"
  "switch-duplicate-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/switch-duplicate-omitted-tck.xml"
  "service-reporting-initial-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/service-reporting-initial-omitted-tck.xml"
  "service-reporting-initial-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/service-reporting-initial-enabled-tck.xml"
  "attribute-fdd-preferred-order-default=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-fdd-preferred-order-tck.xml"
  "timestamped-update-before-delete=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-fdd-preferred-order-tck.xml"
  "transport-order-fdd-default=${_repository_root}/packages/hla-rti-cpp-tck/fom/transport-default-best-effort-tck.xml"
  "directed-interaction-subscription-accumulation=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-interaction-subscription-accumulation-tck.xml"
  "directed-reference-object=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-reference-object-tck.xml"
  "directed-reference-interaction=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-reference-interaction-tck.xml"
  "directed-reference-missing=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-reference-missing-tck.xml"
  "directed-reference-invalid-sharing=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-reference-invalid-sharing-tck.xml"
  "ddm-region-realization-default-range=${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-region-realization-default-range-tck.xml"
  "ddm-region-realization-excluded-dimensions=${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-region-realization-excluded-dimensions-tck.xml"
  "advisory-switch-duplicate-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/advisory-switch-duplicate-disabled-tck.xml"
  "advisory-switch-duplicate-equal=${_repository_root}/packages/hla-rti-cpp-tck/fom/advisory-switch-duplicate-equal-tck.xml"
  "advisory-switch-duplicate-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/advisory-switch-duplicate-enabled-tck.xml"
  "advisory-switch-duplicate-omitted=${_repository_root}/packages/hla-rti-cpp-tck/fom/advisory-switch-duplicate-omitted-tck.xml"
  "restore-pending-time-advance=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml"
  "restore-pending-flush-queue-request=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml"
  "nrg-regulator-release=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml"
  CACHE STRING "Semicolon-separated NAME=FILE FOM bindings supplied by this adapter")
foreach(_name_convention_fom_binding IN ITEMS
    "object-class-name-period|object-class-name-period-tck.xml"
    "object-class-name-reserved-hla|object-class-name-reserved-hla-tck.xml"
    "object-class-name-reserved-na|object-class-name-reserved-na-tck.xml"
    "object-class-name-allowed-punctuation|object-class-name-allowed-punctuation-tck.xml"
    "enumerated-data-type-valid-name|enumerated-data-type-valid-name-tck.xml"
    "enumerated-data-type-invalid-name|enumerated-data-type-invalid-name-tck.xml"
    "enumerator-valid-name|enumerator-valid-name-tck.xml"
    "enumerator-invalid-name|enumerator-invalid-name-tck.xml"
    "variant-record-valid-name|variant-record-valid-name-tck.xml"
    "variant-record-invalid-name|variant-record-invalid-name-tck.xml"
    "variant-discriminant-valid-name|variant-discriminant-valid-name-tck.xml"
    "variant-discriminant-invalid-name|variant-discriminant-invalid-name-tck.xml"
    "variant-alternative-valid-name|variant-alternative-valid-name-tck.xml"
    "variant-alternative-invalid-name|variant-alternative-invalid-name-tck.xml"
    "variant-alternative-type-valid|variant-alternative-type-valid-tck.xml"
    "variant-alternative-type-invalid|variant-alternative-type-invalid-tck.xml"
    "variant-record-encoding-valid|variant-record-encoding-valid-tck.xml"
    "variant-record-encoding-invalid|variant-record-encoding-invalid-tck.xml"
    "array-data-type-valid-name|array-data-type-valid-name-tck.xml"
    "array-data-type-invalid-name|array-data-type-invalid-name-tck.xml"
    "array-element-type-valid|array-element-type-valid-tck.xml"
    "array-element-type-invalid|array-element-type-invalid-tck.xml")
  string(REPLACE "|" ";" _name_convention_fom_parts "${_name_convention_fom_binding}")
  list(GET _name_convention_fom_parts 0 _name_convention_fom_key)
  list(GET _name_convention_fom_parts 1 _name_convention_fom_file)
  set(_name_convention_fom_present FALSE)
  foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
    if(_scenario_fom MATCHES "^${_name_convention_fom_key}=")
      set(_name_convention_fom_present TRUE)
      break()
    endif()
  endforeach()
  if(NOT _name_convention_fom_present)
    list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
      "${_name_convention_fom_key}=${_repository_root}/packages/hla-rti-cpp-tck/fom/${_name_convention_fom_file}")
  endif()
endforeach()
set(_transport_order_fdd_default_fom_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^transport-order-fdd-default=")
    set(_transport_order_fdd_default_fom_present TRUE)
    break()
  endif()
endforeach()
if(NOT _transport_order_fdd_default_fom_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "transport-order-fdd-default=${_repository_root}/packages/hla-rti-cpp-tck/fom/transport-default-best-effort-tck.xml")
endif()
set(_custom_tradt_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^federation-supplied-custom-tradt=")
    set(_custom_tradt_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _custom_tradt_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "federation-supplied-custom-tradt=${_repository_root}/packages/hla-rti-cpp-tck/fom/time-representation-custom-tradt-tck.xml")
endif()
set(_noncompliant_update_rate_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^noncompliant-update-rate=")
    set(_noncompliant_update_rate_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _noncompliant_update_rate_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "noncompliant-update-rate=${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-zero-update-rate-tck.xml")
endif()
set(_mom_federate_subclass_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^mom-federate-subclass-instance=")
    set(_mom_federate_subclass_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _mom_federate_subclass_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "mom-federate-subclass-instance=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-federate-subclass-instance-tck.xml")
endif()
set(_mom_federate_added_attribute_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^mom-federate-added-attribute-same-instance=")
    set(_mom_federate_added_attribute_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _mom_federate_added_attribute_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "mom-federate-added-attribute-same-instance=${_repository_root}/packages/hla-rti-cpp-tck/fom/mom-federate-added-attribute-same-instance-tck.xml")
endif()
set(_noncompliant_module_atomicity_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^noncompliant-module-atomicity=")
    set(_noncompliant_module_atomicity_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _noncompliant_module_atomicity_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "noncompliant-module-atomicity=${_repository_root}/packages/hla-rti-cpp-tck/fom/noncompliant-module-atomicity-tck.xml")
endif()
foreach(_scaffold_fom_binding IN ITEMS
    "incomplete-object-class-scaffold|incomplete-object-class-scaffold-tck.xml"
    "complete-object-class-scaffold-provider|complete-object-class-scaffold-provider-tck.xml"
    "incomplete-interaction-class-scaffold|incomplete-interaction-class-scaffold-tck.xml"
    "complete-interaction-class-scaffold-provider|complete-interaction-class-scaffold-provider-tck.xml")
  string(REPLACE "|" ";" _scaffold_fom_parts "${_scaffold_fom_binding}")
  list(GET _scaffold_fom_parts 0 _scaffold_fom_key)
  list(GET _scaffold_fom_parts 1 _scaffold_fom_file)
  set(_scaffold_fom_key_present FALSE)
  foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
    if(_scenario_fom MATCHES "^${_scaffold_fom_key}=")
      set(_scaffold_fom_key_present TRUE)
      break()
    endif()
  endforeach()
  if(NOT _scaffold_fom_key_present)
    list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
      "${_scaffold_fom_key}=${_repository_root}/packages/hla-rti-cpp-tck/fom/${_scaffold_fom_file}")
  endif()
endforeach()
set(_directed_interaction_accumulation_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^directed-interaction-subscription-accumulation=")
    set(_directed_interaction_accumulation_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _directed_interaction_accumulation_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "directed-interaction-subscription-accumulation=${_repository_root}/packages/hla-rti-cpp-tck/fom/directed-interaction-subscription-accumulation-tck.xml")
endif()
set(_attribute_fdd_preferred_order_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^attribute-fdd-preferred-order-default=")
    set(_attribute_fdd_preferred_order_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _attribute_fdd_preferred_order_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "attribute-fdd-preferred-order-default=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-fdd-preferred-order-tck.xml")
endif()
set(_timestamped_update_before_delete_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^timestamped-update-before-delete=")
    set(_timestamped_update_before_delete_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _timestamped_update_before_delete_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "timestamped-update-before-delete=${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-fdd-preferred-order-tck.xml")
endif()
set(_ddm_region_realization_default_range_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^ddm-region-realization-default-range=")
    set(_ddm_region_realization_default_range_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _ddm_region_realization_default_range_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "ddm-region-realization-default-range=${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-region-realization-default-range-tck.xml")
endif()
set(_ddm_region_realization_excluded_dimensions_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^ddm-region-realization-excluded-dimensions=")
    set(_ddm_region_realization_excluded_dimensions_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _ddm_region_realization_excluded_dimensions_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "ddm-region-realization-excluded-dimensions=${_repository_root}/packages/hla-rti-cpp-tck/fom/ddm-region-realization-excluded-dimensions-tck.xml")
endif()
set(_restore_pending_fqr_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^restore-pending-flush-queue-request=")
    set(_restore_pending_fqr_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _restore_pending_fqr_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "restore-pending-flush-queue-request=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml")
endif()
set(_pending_time_advance_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^restore-pending-time-advance=")
    set(_pending_time_advance_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _pending_time_advance_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "restore-pending-time-advance=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml")
endif()
set(_nrg_regulator_release_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^nrg-regulator-release=")
    set(_nrg_regulator_release_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _nrg_regulator_release_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "nrg-regulator-release=${_repository_root}/packages/hla-rti-cpp-tck/fom/non-regulated-grant-enabled.xml")
endif()
set(_fqr_undefined_galt_nrg_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^fqr-undefined-galt-nrg-disabled=")
    set(_fqr_undefined_galt_nrg_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _fqr_undefined_galt_nrg_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "fqr-undefined-galt-nrg-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml")
endif()
set(_fqr_undefined_galt_nrg_enabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^fqr-undefined-galt-nrg-enabled=")
    set(_fqr_undefined_galt_nrg_enabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _fqr_undefined_galt_nrg_enabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "fqr-undefined-galt-nrg-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/nrg-interaction-tck.xml")
endif()
set(_nmr_undefined_galt_nrg_disabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^nmr-undefined-galt-nrg-disabled=")
    set(_nmr_undefined_galt_nrg_disabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _nmr_undefined_galt_nrg_disabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "nmr-undefined-galt-nrg-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml")
endif()
set(_nmr_undefined_galt_nrg_enabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^nmr-undefined-galt-nrg-enabled=")
    set(_nmr_undefined_galt_nrg_enabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _nmr_undefined_galt_nrg_enabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "nmr-undefined-galt-nrg-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/nrg-interaction-tck.xml")
endif()
set(_nmra_undefined_galt_nrg_disabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^nmra-undefined-galt-nrg-disabled=")
    set(_nmra_undefined_galt_nrg_disabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _nmra_undefined_galt_nrg_disabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "nmra-undefined-galt-nrg-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml")
endif()
set(_nmra_undefined_galt_nrg_enabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^nmra-undefined-galt-nrg-enabled=")
    set(_nmra_undefined_galt_nrg_enabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _nmra_undefined_galt_nrg_enabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "nmra-undefined-galt-nrg-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/nrg-interaction-tck.xml")
endif()
set(_tara_undefined_galt_nrg_disabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^tara-undefined-galt-nrg-disabled=")
    set(_tara_undefined_galt_nrg_disabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _tara_undefined_galt_nrg_disabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "tara-undefined-galt-nrg-disabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/p0-tck.xml")
endif()
set(_tara_undefined_galt_nrg_enabled_fom_key_present FALSE)
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  if(_scenario_fom MATCHES "^tara-undefined-galt-nrg-enabled=")
    set(_tara_undefined_galt_nrg_enabled_fom_key_present TRUE)
    break()
  endif()
endforeach()
if(NOT _tara_undefined_galt_nrg_enabled_fom_key_present)
  list(APPEND HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS
    "tara-undefined-galt-nrg-enabled=${_repository_root}/packages/hla-rti-cpp-tck/fom/nrg-interaction-tck.xml")
endif()
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/known-class-advisory-tck.xml"
  CACHE FILEPATH "Known-class advisory FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/known-class-advisory-disabled-tck.xml"
  CACHE FILEPATH "Known-class-disabled advisory FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_OWNERSHIP_EXTENSION_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/ownership-post-join-extension-tck.xml"
  CACHE FILEPATH "FOM extending an already registered object class for ownership testing")
set(HLA_RTI_TCK_ADAPTER_OWNERSHIP_ADDED_ATTRIBUTE "AddedAfterRegistration"
  CACHE STRING "Attribute added by the ownership extension FOM")
set(HLA_RTI_TCK_ADAPTER_RATE_FOM
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/attribute-relevance-rate-tck.xml"
  CACHE FILEPATH "Attribute relevance rate advisory FOM supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_RATE_OBJECT_CLASS
  "HLAobjectRoot.TckRateObject"
  CACHE STRING "Update-rate advisory object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_RATE_BEST_EFFORT_ATTRIBUTE "BestEffortValue"
  CACHE STRING "Update-rate advisory attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_FOM_UPDATE_RATE "TckFast"
  CACHE STRING "Higher update-rate designator supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_RATE_DESIGNATOR "TckSlow"
  CACHE STRING "Lower update-rate designator supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS "TckDimensionX;TckDimensionY"
  CACHE STRING "Semicolon-separated DDM dimensions supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_DDM_UNRELATED_DIMENSION "TckUnrelatedDimension"
  CACHE STRING "Unrelated DDM dimension supplied for InvalidRegionContext cases")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS
  "TckDimensionX;TckDimensionY;TckDimensionZ"
  CACHE STRING "Semicolon-separated three-dimensional DDM dimensions supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS
  "HLAobjectRoot.TckThreeDimensionalObject"
  CACHE STRING "Three-dimensional DDM object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE "Value"
  CACHE STRING "Three-dimensional DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS
  "HLAobjectRoot.TckTransportRegionalObject"
  CACHE STRING "Timestamped regional custom-transport object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE "RegionalValue"
  CACHE STRING "Timestamped regional custom-transport attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS
  "HLAinteractionRoot.TckTransportRegionalInteraction"
  CACHE STRING "Timestamped regional custom-transport interaction class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER "RegionalPayload"
  CACHE STRING "Timestamped regional custom-transport parameter supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_FOM_CUSTOM_TRANSPORTATION "TckBestEffort"
  CACHE STRING "Custom transportation name in the adapter rich FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_TRANSPORTATION "TckAdditionalTransport"
  CACHE STRING "Transportation name in the adapter additional FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_DIMENSION "TckDimension"
  CACHE STRING "Base dimension name in the adapter rich FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION "TckAdditionalDimension"
  CACHE STRING "Dimension name in the adapter additional FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_OBJECT_CLASS
  "HLAobjectRoot.TckExtensionDimensionalObject"
  CACHE STRING "Dimensional object class in the adapter additional FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_ATTRIBUTE "AdditionalValue"
  CACHE STRING "Dimensional attribute in the adapter additional FOM")
set(HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_UPDATE_RATE "TckAdditionalRate"
  CACHE STRING "Update-rate designator in the adapter additional FOM")
set(HLA_RTI_TCK_ADAPTER_OBJECT_CLASS "HLAobjectRoot.TckObject"
  CACHE STRING "Ordinary object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ATTRIBUTE "Value"
  CACHE STRING "Ordinary object attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_SECONDARY_ATTRIBUTE "Name"
  CACHE STRING "Second ordinary object attribute used by partial-unsubscription tests")
set(HLA_RTI_TCK_ADAPTER_OBJECT_CLASS_THIRD_ATTRIBUTE "PayRate"
  CACHE STRING "Third ordinary object attribute in the adapter's complete object-class subscription test")
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_OBJECT_CLASS
  "HLAobjectRoot.TckKnownClassObject"
  CACHE STRING "Known-class advisory base object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_OBJECT_CLASS
  "HLAobjectRoot.TckKnownClassObject.TckKnownClassDerivedObject"
  CACHE STRING "Known-class advisory derived object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_KNOWN_ATTRIBUTE "Name"
  CACHE STRING "Known-class advisory inherited attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_ATTRIBUTE "Efficiency"
  CACHE STRING "Known-class advisory derived-only attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_OBJECT_CLASS
  "HLAobjectRoot.TckAutoProvideObject"
  CACHE STRING "Auto Provide object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FIRST "ProviderAValue"
  CACHE STRING "First Auto Provide attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_SECOND "ProviderBValue"
  CACHE STRING "Second Auto Provide attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS
  "HLAobjectRoot.TckMultiAttributeObject"
  CACHE STRING "Multi-attribute DDM object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST "FirstValue"
  CACHE STRING "First multi-attribute DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND "SecondValue"
  CACHE STRING "Second multi-attribute DDM attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_OBJECT_CLASS
  "HLAobjectRoot.TckTimestampedAttributePasselObject"
  CACHE STRING "Timestamped multi-transport attribute object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_RELIABLE_ATTRIBUTE
  "ReliableValue"
  CACHE STRING "Reliable timestamped passel attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_BEST_EFFORT_ATTRIBUTE
  "BestEffortValue"
  CACHE STRING "Best-effort timestamped passel attribute supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS "HLAinteractionRoot.TckInteraction"
  CACHE STRING "Ordinary interaction class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_PREFERRED_TIMESTAMP_INTERACTION_CLASS
  "HLAinteractionRoot.TckPreferredTimestampInteraction"
  CACHE STRING "Interaction class with FDD-preferred timestamp order")
set(HLA_RTI_TCK_ADAPTER_PREFERRED_RECEIVE_INTERACTION_CLASS
  "HLAinteractionRoot.TckPreferredReceiveInteraction"
  CACHE STRING "Interaction class with FDD-preferred receive order")
set(HLA_RTI_TCK_ADAPTER_SECONDARY_DIRECTED_OBJECT_CLASS
  "HLAobjectRoot.TckObject.TckSecondaryObject"
  CACHE STRING "Second directed-interaction object class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_SECONDARY_DIRECTED_INTERACTION_CLASS
  "HLAinteractionRoot.TckSecondaryDirectedInteraction"
  CACHE STRING "Second directed-interaction class supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_PARAMETER "Payload"
  CACHE STRING "Ordinary interaction parameter supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/fom-extension-tck.xml"
  CACHE STRING "Semicolon-separated extension FOM modules supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-malformed-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-namespace-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-duplicate-tck.xml"
  CACHE STRING "Semicolon-separated invalid FOM modules supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_STANDARD_INVALID_FOM_MODULES
  "${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-zero-update-rate-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-dimension-default-value-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-array-cardinality-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-variable-array-encoding-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-fixed-array-encoding-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-reference-object-name-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-reference-basic-representation-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-data-type-representation-name-collision-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-model-identification-blank-version-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-basic-data-representation-reserved-hla-name-tck.xml;${_repository_root}/packages/hla-rti-cpp-tck/fom/invalid-basic-data-representation-endian-enumeration-tck.xml"
  CACHE STRING "Semicolon-separated standard-invalid FOM modules supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL "both"
  CACHE STRING "Callback model supplied by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_TIMEOUT_MS "10000"
  CACHE STRING "Per-scenario timeout passed to the portable TCK")
if(WIN32 AND DEFINED ENV{TEMP} AND NOT "$ENV{TEMP}" STREQUAL "")
  set(_tck_default_service_report_directory
    "$ENV{TEMP}/umbra-cpp-tck-service-reports")
else()
  set(_tck_default_service_report_directory
    "${CMAKE_CURRENT_BINARY_DIR}/service-reports")
endif()
set(HLA_RTI_TCK_ADAPTER_SERVICE_REPORT_DIRECTORY
  "${_tck_default_service_report_directory}"
  CACHE PATH "Embedded-profile service-report directory for package scenarios")
set(HLA_RTI_TCK_ADAPTER_SCENARIO_SET "verified"
  CACHE STRING "Scenario set for the CTest matrix: verified or all")
set(HLA_RTI_TCK_ADAPTER_PROVIDER_ID "installed-package"
  CACHE STRING "Provider identifier recorded in TCK evidence")
set(HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION "HLAinteger64Time"
  CACHE STRING "Logical time implementation selected by this provider adapter")
set(HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TIME_IMPLEMENTATION ""
  CACHE STRING "Logical-time factory name this provider adapter knows is unavailable")
set(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS ""
  CACHE STRING "Optional standard RtiConfiguration address supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME ""
  CACHE STRING "Optional standard RtiConfiguration name supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS ""
  CACHE STRING "Optional standard RtiConfiguration settings supplied by this adapter")
set(HLA_RTI_TCK_ADAPTER_SETTINGS_PARSE_FAILURE_INPUT "fomEdition=202x"
  CACHE STRING "Provider-specific settings text known to fail parsing")
set(HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TRANSPORT_SETTINGS
  "unsupportedTransportationTypes=${HLA_RTI_TCK_ADAPTER_FOM_CUSTOM_TRANSPORTATION},${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_TRANSPORTATION}"
  CACHE STRING "Adapter-specific additional settings that make the custom transport FOM types unavailable")
set(HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE ""
  CACHE FILEPATH "Optional Umbra process-service fixture for forced resignation TCK")
set(HLA_RTI_TCK_ADAPTER_PROVIDER_BIN ""
  CACHE PATH "Provider runtime directory used by process fixtures")
set(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER ""
  CACHE FILEPATH "Optional marker written after the adapter observes connection loss")
set(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED OFF
  CACHE BOOL "Whether the provider adapter manages connection loss out of band")
set(HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS ""
  CACHE STRING "Semicolon-separated scenarios excluded from the adapter CTest matrix")
set(HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER ""
  CACHE STRING "Optional semicolon-separated scenario IDs selected for the adapter CTest matrix")

if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_FOM}")
  message(FATAL_ERROR "Adapter FOM does not exist: ${HLA_RTI_TCK_ADAPTER_FOM}")
endif()
if(HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE AND
   (NOT EXISTS "${HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE}" OR
    IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE}"))
  message(FATAL_ERROR
    "Forced-resignation adapter fixture does not exist: ${HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE}")
endif()
if(HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE AND
   (NOT HLA_RTI_TCK_ADAPTER_PROVIDER_BIN OR
    NOT IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_PROVIDER_BIN}"))
  message(FATAL_ERROR
    "Forced-resignation adapter requires the provider runtime directory")
endif()
foreach(_schema_fom IN ITEMS
    HLA_RTI_TCK_ADAPTER_SCHEMA_VALID_FOM
    HLA_RTI_TCK_ADAPTER_SCHEMA_INVALID_FOM)
  if(NOT EXISTS "${${_schema_fom}}" OR IS_DIRECTORY "${${_schema_fom}}")
    message(FATAL_ERROR "Adapter Annex F FOM does not exist: ${${_schema_fom}}")
  endif()
endforeach()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}")
  message(FATAL_ERROR
    "Adapter rich FOM model does not exist: ${HLA_RTI_TCK_ADAPTER_MODEL_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_DDM_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_DDM_FOM}")
  message(FATAL_ERROR
    "Adapter DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_DDM_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}")
  message(FATAL_ERROR
    "Adapter multi-attribute DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}")
  message(FATAL_ERROR
    "Adapter three-dimensional DDM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_MIM_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_MIM_FOM}")
  message(FATAL_ERROR
    "Adapter MIM FOM does not exist: ${HLA_RTI_TCK_ADAPTER_MIM_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}")
  message(FATAL_ERROR
    "Adapter switch-declaration FOM does not exist: ${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}")
  message(FATAL_ERROR
    "Adapter Auto Provide FOM does not exist: ${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}")
endif()
foreach(_merge_fom IN ITEMS
    HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONSUMER_FOM
    HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_PROVIDER_FOM
    HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONFLICTING_PROVIDER_FOM)
  if(NOT EXISTS "${${_merge_fom}}" OR IS_DIRECTORY "${${_merge_fom}}")
    message(FATAL_ERROR "Adapter data-type merge FOM does not exist: ${${_merge_fom}}")
  endif()
endforeach()
foreach(_variant_fom IN ITEMS
    HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_BASE_FOM
    HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_DISJOINT_FOM
    HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_OVERLAP_FOM)
  if(NOT EXISTS "${${_variant_fom}}" OR IS_DIRECTORY "${${_variant_fom}}")
    message(FATAL_ERROR "Adapter variant-alternative FOM does not exist: ${${_variant_fom}}")
  endif()
endforeach()
foreach(_parameter_fom IN ITEMS
    HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_BASE_FOM
    HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_DISJOINT_FOM
    HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_CONFLICT_FOM)
  if(NOT EXISTS "${${_parameter_fom}}" OR IS_DIRECTORY "${${_parameter_fom}}")
    message(FATAL_ERROR "Adapter interaction-parameter FOM does not exist: ${${_parameter_fom}}")
  endif()
endforeach()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM}")
  message(FATAL_ERROR
    "Adapter known-class advisory FOM does not exist: ${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM}")
  message(FATAL_ERROR
    "Adapter known-class-disabled advisory FOM does not exist: ${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_OWNERSHIP_EXTENSION_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_OWNERSHIP_EXTENSION_FOM}")
  message(FATAL_ERROR
    "Adapter ownership extension FOM does not exist: ${HLA_RTI_TCK_ADAPTER_OWNERSHIP_EXTENSION_FOM}")
endif()
if(NOT EXISTS "${HLA_RTI_TCK_ADAPTER_RATE_FOM}" OR
   IS_DIRECTORY "${HLA_RTI_TCK_ADAPTER_RATE_FOM}")
  message(FATAL_ERROR
    "Adapter attribute relevance rate advisory FOM does not exist: ${HLA_RTI_TCK_ADAPTER_RATE_FOM}")
endif()
list(LENGTH HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS _ddm_dimension_count)
if(_ddm_dimension_count LESS 2)
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS must supply at least two dimensions")
endif()
list(LENGTH HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS
  _three_dimensional_dimension_count)
if(_three_dimensional_dimension_count LESS 3)
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS must supply at least three dimensions")
endif()
foreach(_additional_fom IN LISTS HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES)
  if(NOT EXISTS "${_additional_fom}" OR IS_DIRECTORY "${_additional_fom}")
    message(FATAL_ERROR "Adapter extension FOM does not exist: ${_additional_fom}")
  endif()
endforeach()
foreach(_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES)
  if(NOT EXISTS "${_invalid_fom}" OR IS_DIRECTORY "${_invalid_fom}")
    message(FATAL_ERROR "Adapter invalid FOM does not exist: ${_invalid_fom}")
  endif()
endforeach()
foreach(_standard_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_STANDARD_INVALID_FOM_MODULES)
  if(NOT EXISTS "${_standard_invalid_fom}" OR IS_DIRECTORY "${_standard_invalid_fom}")
    message(FATAL_ERROR
      "Adapter standard-invalid FOM does not exist: ${_standard_invalid_fom}")
  endif()
endforeach()
if(NOT HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL MATCHES "^(both|evoked|immediate)$")
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL must be both, evoked, or immediate")
endif()
if(NOT HLA_RTI_TCK_ADAPTER_SCENARIO_SET MATCHES "^(verified|all)$")
  message(FATAL_ERROR
    "HLA_RTI_TCK_ADAPTER_SCENARIO_SET must be verified or all")
endif()

set(_tck_catalog_path
  "${_repository_root}/compliance/catalogs/cpp-tck-scenario-catalog.json")
if(NOT EXISTS "${_tck_catalog_path}")
  message(FATAL_ERROR "Portable TCK scenario catalog does not exist: ${_tck_catalog_path}")
endif()
file(READ "${_tck_catalog_path}" _tck_catalog_json)
string(JSON _tck_scenario_count LENGTH "${_tck_catalog_json}" scenarios)
if(_tck_scenario_count LESS 1)
  message(FATAL_ERROR "Portable TCK scenario catalog contains no scenarios")
endif()
math(EXPR _tck_last_scenario_index "${_tck_scenario_count} - 1")
set(_tck_test_scenarios)
set(_tck_candidate_scenarios)
set(_tck_evoked_only_scenarios)
set(_tck_immediate_only_scenarios)
foreach(_tck_scenario_index RANGE 0 ${_tck_last_scenario_index})
  string(JSON _tck_entry GET "${_tck_catalog_json}" scenarios
    ${_tck_scenario_index})
  string(JSON _tck_scenario GET "${_tck_entry}" id)
  if(_tck_scenario STREQUAL
       "cpp-tck.unsupported-fom-transportation-fallback" AND
     NOT HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TRANSPORT_SETTINGS)
    continue()
  endif()
  if(_tck_scenario STREQUAL
       "cpp-tck.federate-resigned-forced-departure" AND
     (NOT HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE OR
      NOT HLA_RTI_TCK_ADAPTER_PROVIDER_BIN))
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.websocket-transport-support" AND
     NOT HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES "^ws://")
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.tls13-transport-support" AND
     NOT HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES "^tls://")
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.tls13-default-port-support" AND
     NOT HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES "^tls://[^:/]+$")
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.tls13-alternate-address-port-configuration" AND
     (NOT HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES "^tls://.+:[0-9]+$" OR
      HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES ":15165$"))
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.secure-websocket-transport-support" AND
     NOT HLA_RTI_TCK_ADAPTER_RTI_ADDRESS MATCHES "^wss://")
    continue()
  endif()
  if(_tck_scenario STREQUAL "cpp-tck.federation-supplied-custom-tradt" AND
     NOT _custom_tradt_provider_enabled)
    continue()
  endif()
  if((_tck_scenario STREQUAL "cpp-tck.authorizer-federation-service-decision" OR
      _tck_scenario STREQUAL "cpp-tck.authorizer-destroy-service-decision" OR
      _tck_scenario STREQUAL "cpp-tck.authorizer-join-service-decision" OR
      _tck_scenario STREQUAL "cpp-tck.authorizer-no-credentials-connect" OR
      _tck_scenario STREQUAL "cpp-tck.authorizer-rid-configuration-decision" OR
      _tck_scenario STREQUAL "cpp-tck.authorizer-single-service-per-rid-profile") AND
     NOT _custom_authorizer_provider_enabled)
    continue()
  endif()
  if(HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER AND
     NOT _tck_scenario IN_LIST HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER)
    continue()
  endif()
  if(_tck_scenario IN_LIST HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS)
    continue()
  endif()
  string(JSON _tck_status GET "${_tck_entry}" default_status)
  string(JSON _tck_promotion GET "${_tck_entry}" promotion)
  if(_tck_promotion STREQUAL "candidate")
    list(APPEND _tck_candidate_scenarios "${_tck_scenario}")
  endif()
  if(NOT _tck_status STREQUAL "unsupported" AND
    (HLA_RTI_TCK_ADAPTER_SCENARIO_SET STREQUAL "all" OR
      _tck_promotion STREQUAL "promoted"))
    list(APPEND _tck_test_scenarios "${_tck_scenario}")
    string(JSON _tck_callback_model_count
      ERROR_VARIABLE _tck_callback_model_parse_error
      LENGTH "${_tck_entry}" callback_models)
    if(NOT _tck_callback_model_parse_error AND
       _tck_callback_model_count EQUAL 1)
      string(JSON _tck_callback_model GET "${_tck_entry}" callback_models 0)
      if(_tck_callback_model STREQUAL "evoked")
        list(APPEND _tck_evoked_only_scenarios "${_tck_scenario}")
      elseif(_tck_callback_model STREQUAL "immediate")
        list(APPEND _tck_immediate_only_scenarios "${_tck_scenario}")
      else()
        message(FATAL_ERROR
          "Unsupported single callback model '${_tck_callback_model}' for ${_tck_scenario}")
      endif()
    endif()
  endif()
endforeach()
if(NOT _tck_test_scenarios)
  set(_tck_connection_loss_only FALSE)
  if(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED AND
     HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER)
    set(_tck_connection_loss_only TRUE)
    foreach(_tck_filtered_scenario IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FILTER)
      if(NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-cleanup" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-cleanup-contract" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.federate-lost-mom-report" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.federate-lost-mom-report-contract" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-unconditional-divestiture" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-unconditional-divestiture-contract" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-delete-then-divest" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-delete-then-divest-contract" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-cancel-pending-acquisition" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.connection-loss-automatic-cancel-pending-acquisition-contract" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.automatic-resign-directive-delete-objects" AND
         NOT _tck_filtered_scenario STREQUAL "cpp-tck.automatic-resign-directive-delete-objects-contract")
        set(_tck_connection_loss_only FALSE)
      elseif(NOT _tck_filtered_scenario IN_LIST HLA_RTI_TCK_ADAPTER_EXCLUDED_SCENARIOS)
        set(_tck_connection_loss_only FALSE)
      endif()
    endforeach()
  endif()
  if(NOT _tck_connection_loss_only)
    message(FATAL_ERROR
      "The selected portable TCK scenario set contains no runnable scenarios")
  endif()
endif()

if(HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL STREQUAL "both")
  set(_tck_test_callback_models evoked immediate)
else()
  set(_tck_test_callback_models "${HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL}")
endif()

set(_tck_common_arguments
  --fom "${HLA_RTI_TCK_ADAPTER_FOM}"
  --schema-valid-fom "${HLA_RTI_TCK_ADAPTER_SCHEMA_VALID_FOM}"
  --schema-invalid-fom "${HLA_RTI_TCK_ADAPTER_SCHEMA_INVALID_FOM}"
  --model-fom "${HLA_RTI_TCK_ADAPTER_MODEL_FOM}"
  --data-type-merge-consumer-fom "${HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONSUMER_FOM}"
  --data-type-merge-provider-fom "${HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_PROVIDER_FOM}"
  --data-type-merge-conflicting-provider-fom
    "${HLA_RTI_TCK_ADAPTER_DATA_TYPE_MERGE_CONFLICTING_PROVIDER_FOM}"
  --variant-alternative-base-fom "${HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_BASE_FOM}"
  --variant-alternative-disjoint-fom "${HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_DISJOINT_FOM}"
  --variant-alternative-overlap-fom "${HLA_RTI_TCK_ADAPTER_VARIANT_ALTERNATIVE_OVERLAP_FOM}"
  --interaction-parameter-base-fom "${HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_BASE_FOM}"
  --interaction-parameter-disjoint-fom "${HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_DISJOINT_FOM}"
  --interaction-parameter-conflict-fom "${HLA_RTI_TCK_ADAPTER_INTERACTION_PARAMETER_CONFLICT_FOM}"
  --object-attribute-base-fom "${HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_BASE_FOM}"
  --object-attribute-disjoint-fom "${HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_DISJOINT_FOM}"
  --object-attribute-conflict-fom "${HLA_RTI_TCK_ADAPTER_OBJECT_ATTRIBUTE_CONFLICT_FOM}"
  --fom-custom-transportation "${HLA_RTI_TCK_ADAPTER_FOM_CUSTOM_TRANSPORTATION}"
  --fom-additional-transportation
    "${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_TRANSPORTATION}"
  --fom-dimension "${HLA_RTI_TCK_ADAPTER_FOM_DIMENSION}"
  --fom-additional-dimension "${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION}"
  --fom-additional-dimension-object-class
    "${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_OBJECT_CLASS}"
  --fom-additional-dimension-attribute
    "${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_DIMENSION_ATTRIBUTE}"
  --fom-additional-update-rate
    "${HLA_RTI_TCK_ADAPTER_FOM_ADDITIONAL_UPDATE_RATE}"
  --ddm-fom "${HLA_RTI_TCK_ADAPTER_DDM_FOM}"
  --multi-attribute-fom "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FOM}"
  --three-dimensional-fom "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_FOM}"
  --three-dimensional-object-class "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_OBJECT_CLASS}"
  --three-dimensional-attribute "${HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_ATTRIBUTE}"
  --custom-regional-object-class "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_OBJECT_CLASS}"
  --custom-regional-attribute "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_ATTRIBUTE}"
  --custom-regional-interaction-class "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_INTERACTION_CLASS}"
  --custom-regional-parameter "${HLA_RTI_TCK_ADAPTER_CUSTOM_REGIONAL_PARAMETER}"
  --object-class "${HLA_RTI_TCK_ADAPTER_OBJECT_CLASS}"
  --attribute "${HLA_RTI_TCK_ADAPTER_ATTRIBUTE}"
   --secondary-attribute "${HLA_RTI_TCK_ADAPTER_SECONDARY_ATTRIBUTE}"
   --object-class-third-attribute
     "${HLA_RTI_TCK_ADAPTER_OBJECT_CLASS_THIRD_ATTRIBUTE}"
  --known-class-object-class
    "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_OBJECT_CLASS}"
  --known-class-derived-object-class
    "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_OBJECT_CLASS}"
  --known-class-known-attribute
    "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_KNOWN_ATTRIBUTE}"
  --known-class-derived-attribute
    "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DERIVED_ATTRIBUTE}"
  --auto-provide-object-class "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_OBJECT_CLASS}"
  --auto-provide-first "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FIRST}"
  --auto-provide-second "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_SECOND}"
  --multi-attribute-object-class
    "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_OBJECT_CLASS}"
  --multi-attribute-first "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_FIRST}"
  --multi-attribute-second "${HLA_RTI_TCK_ADAPTER_MULTI_ATTRIBUTE_SECOND}"
  --timestamped-attribute-passel-object-class
    "${HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_OBJECT_CLASS}"
  --timestamped-attribute-passel-reliable-attribute
    "${HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_RELIABLE_ATTRIBUTE}"
  --timestamped-attribute-passel-best-effort-attribute
    "${HLA_RTI_TCK_ADAPTER_TIMESTAMPED_ATTRIBUTE_PASSEL_BEST_EFFORT_ATTRIBUTE}"
  --interaction-class "${HLA_RTI_TCK_ADAPTER_INTERACTION_CLASS}"
  --preferred-timestamp-interaction-class
    "${HLA_RTI_TCK_ADAPTER_PREFERRED_TIMESTAMP_INTERACTION_CLASS}"
  --preferred-receive-interaction-class
    "${HLA_RTI_TCK_ADAPTER_PREFERRED_RECEIVE_INTERACTION_CLASS}"
  --secondary-directed-object-class "${HLA_RTI_TCK_ADAPTER_SECONDARY_DIRECTED_OBJECT_CLASS}"
  --secondary-directed-interaction-class "${HLA_RTI_TCK_ADAPTER_SECONDARY_DIRECTED_INTERACTION_CLASS}"
  --parameter "${HLA_RTI_TCK_ADAPTER_PARAMETER}"
  --mim-fom "${HLA_RTI_TCK_ADAPTER_MIM_FOM}"
  --switches-fom "${HLA_RTI_TCK_ADAPTER_SWITCHES_FOM}"
  --auto-provide-fom "${HLA_RTI_TCK_ADAPTER_AUTO_PROVIDE_FOM}"
  --known-class-fom "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_FOM}"
  --known-class-disabled-fom
    "${HLA_RTI_TCK_ADAPTER_KNOWN_CLASS_DISABLED_FOM}"
  --ownership-extension-fom "${HLA_RTI_TCK_ADAPTER_OWNERSHIP_EXTENSION_FOM}"
  --ownership-added-attribute "${HLA_RTI_TCK_ADAPTER_OWNERSHIP_ADDED_ATTRIBUTE}"
  --rate-fom "${HLA_RTI_TCK_ADAPTER_RATE_FOM}"
  --rate-object-class "${HLA_RTI_TCK_ADAPTER_RATE_OBJECT_CLASS}"
  --rate-best-effort-attribute
    "${HLA_RTI_TCK_ADAPTER_RATE_BEST_EFFORT_ATTRIBUTE}"
  --fom-update-rate "${HLA_RTI_TCK_ADAPTER_FOM_UPDATE_RATE}"
  --rate-designator "${HLA_RTI_TCK_ADAPTER_RATE_DESIGNATOR}"
  --time-implementation "${HLA_RTI_TCK_ADAPTER_TIME_IMPLEMENTATION}"
  --provider-id "${HLA_RTI_TCK_ADAPTER_PROVIDER_ID}"
  --timeout-ms "${HLA_RTI_TCK_ADAPTER_TIMEOUT_MS}")
if(NOT HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TIME_IMPLEMENTATION STREQUAL "")
  list(APPEND _tck_common_arguments
    --unsupported-time-implementation
    "${HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TIME_IMPLEMENTATION}")
endif()
list(APPEND _tck_common_arguments
  --service-report-directory "${HLA_RTI_TCK_ADAPTER_SERVICE_REPORT_DIRECTORY}")
foreach(_additional_fom IN LISTS HLA_RTI_TCK_ADAPTER_ADDITIONAL_FOM_MODULES)
  list(APPEND _tck_common_arguments --additional-fom "${_additional_fom}")
endforeach()
foreach(_scenario_fom IN LISTS HLA_RTI_TCK_ADAPTER_SCENARIO_FOMS)
  list(APPEND _tck_common_arguments --scenario-fom "${_scenario_fom}")
endforeach()
foreach(_ddm_dimension IN LISTS HLA_RTI_TCK_ADAPTER_DDM_DIMENSIONS)
  list(APPEND _tck_common_arguments --ddm-dimension "${_ddm_dimension}")
endforeach()
list(APPEND _tck_common_arguments
  --ddm-unrelated-dimension "${HLA_RTI_TCK_ADAPTER_DDM_UNRELATED_DIMENSION}")
foreach(_three_dimensional_dimension IN LISTS HLA_RTI_TCK_ADAPTER_THREE_DIMENSIONAL_DIMENSIONS)
  list(APPEND _tck_common_arguments
    --three-dimensional-dimension "${_three_dimensional_dimension}")
endforeach()
foreach(_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_INVALID_FOM_MODULES)
  list(APPEND _tck_common_arguments --invalid-fom "${_invalid_fom}")
endforeach()
foreach(_standard_invalid_fom IN LISTS HLA_RTI_TCK_ADAPTER_STANDARD_INVALID_FOM_MODULES)
  list(APPEND _tck_common_arguments
    --standard-invalid-fom "${_standard_invalid_fom}")
endforeach()
if(HLA_RTI_TCK_ADAPTER_RTI_ADDRESS)
  list(APPEND _tck_common_arguments
    --rti-address "${HLA_RTI_TCK_ADAPTER_RTI_ADDRESS}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME)
  list(APPEND _tck_common_arguments
    --configuration-name "${HLA_RTI_TCK_ADAPTER_CONFIGURATION_NAME}")
endif()
if(HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS)
  list(APPEND _tck_common_arguments
    --additional-settings "${HLA_RTI_TCK_ADAPTER_ADDITIONAL_SETTINGS}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER)
  list(APPEND _tck_common_arguments
    --connection-loss-marker "${HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_MARKER}")
endif()
if(HLA_RTI_TCK_ADAPTER_CONNECTION_LOSS_SERVER_MANAGED)
  list(APPEND _tck_common_arguments --connection-loss-server-managed)
endif()

set(HLA_RTI_TCK_TEST_SCENARIOS "${_tck_test_scenarios}" CACHE STRING
  "Portable TCK scenarios registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_CANDIDATE_SCENARIOS "${_tck_candidate_scenarios}" CACHE STRING
  "Portable TCK candidate scenarios registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_CALLBACK_MODELS "${_tck_test_callback_models}" CACHE STRING
  "Portable TCK callback models registered by this adapter" FORCE)
set(HLA_RTI_TCK_TEST_EVOKED_ONLY_SCENARIOS "${_tck_evoked_only_scenarios}")
set(HLA_RTI_TCK_TEST_IMMEDIATE_ONLY_SCENARIOS "${_tck_immediate_only_scenarios}")
set(HLA_RTI_TCK_TEST_COMMON_ARGUMENTS "${_tck_common_arguments}" CACHE STRING
  "Adapter arguments shared by each portable TCK scenario" FORCE)
set(HLA_RTI_TCK_TEST_SETTINGS_PARSE_FAILURE_INPUT
  "${HLA_RTI_TCK_ADAPTER_SETTINGS_PARSE_FAILURE_INPUT}" CACHE STRING
  "Adapter-provided malformed settings value for the focused Connect TCK" FORCE)
set(HLA_RTI_TCK_TEST_UNSUPPORTED_TRANSPORT_SETTINGS
  "${HLA_RTI_TCK_ADAPTER_UNSUPPORTED_TRANSPORT_SETTINGS}" CACHE STRING
  "Adapter-provided unavailable-transport setup for the focused fallback TCK" FORCE)
set(HLA_RTI_TCK_FEDERATE_RESIGNATION_FIXTURE
  "${HLA_RTI_TCK_ADAPTER_FEDERATE_RESIGNATION_FIXTURE}" CACHE FILEPATH
  "Umbra process-service fixture for the forced-resignation TCK" FORCE)
set(HLA_RTI_TCK_FEDERATE_RESIGNATION_PROVIDER_BIN
  "${HLA_RTI_TCK_ADAPTER_PROVIDER_BIN}" CACHE PATH
  "Provider runtime directory for the forced-resignation TCK" FORCE)
set(HLA_RTI_TCK_TEST_TIMEOUT_SECONDS 120 CACHE STRING
  "CTest timeout for each portable TCK scenario" FORCE)

set(HLA_RTI_TCK_TEST_BUDGET_SCRIPT
  "${_repository_root}/cmake/test_executable_budget.cmake")
add_subdirectory(
  "${_repository_root}/packages/hla-rti-cpp-tck"
  "${CMAKE_CURRENT_BINARY_DIR}/portable-tck")

if(TARGET hla_rti_cpp_tck AND
   _current_package_rti_type STREQUAL "SHARED_LIBRARY")
  target_compile_definitions(hla_rti_cpp_tck PRIVATE
    "HLA_TCK_PACKAGE_COMPILER_MARKER=\"${umbra_rti_HLA_COMPILER_VERSION_MARKER}\""
    "HLA_TCK_PACKAGE_32BIT_MARKER=\"${umbra_rti_HLA_LIBRARY_32BIT_MARKER}\""
    "HLA_TCK_PACKAGE_DEBUG_BUILD=$<IF:$<CONFIG:Debug>,1,0>"
    "HLA_TCK_PACKAGE_RTI_LIBRARY_PATH=\"$<TARGET_FILE:umbra::rti>\""
    "HLA_TCK_PACKAGE_FEDTIME_LIBRARY_PATH=\"$<TARGET_FILE:umbra::fedtime>\""
    "HLA_TCK_PACKAGE_AUTHORIZER_LIBRARY_PATH=\"$<TARGET_FILE:umbra::authorizer>\""
  )
endif()

include(CTest)
if(BUILD_TESTING)
  set(_tck_command
    $<TARGET_FILE:hla_rti_cpp_tck>
    ${_tck_common_arguments}
    --callback-model "${HLA_RTI_TCK_ADAPTER_CALLBACK_MODEL}"
    )
  add_test(NAME hla_rti_cpp_tck_installed COMMAND ${_tck_command})
  set_tests_properties(hla_rti_cpp_tck_installed PROPERTIES
    LABELS "portable-cpp-tck-suite;installed-package")

  if(WIN32)
    get_target_property(_installed_rti_location umbra::rti IMPORTED_LOCATION_DEBUG)
    if(NOT _installed_rti_location)
      get_target_property(_installed_rti_location umbra::rti IMPORTED_LOCATION)
    endif()
    if(_installed_rti_location MATCHES "\\.dll$")
      get_filename_component(_installed_runtime_directory
        "${_installed_rti_location}" DIRECTORY)
      get_property(_portable_tck_tests
        DIRECTORY "${_repository_root}/packages/hla-rti-cpp-tck" PROPERTY TESTS)
      foreach(_portable_tck_test IN LISTS _portable_tck_tests)
        set_property(TEST "${_portable_tck_test}"
          DIRECTORY "${_repository_root}/packages/hla-rti-cpp-tck"
          APPEND PROPERTY ENVIRONMENT_MODIFICATION
          "PATH=path_list_prepend:${_installed_runtime_directory}")
      endforeach()
      set_property(TEST hla_rti_cpp_tck_installed APPEND PROPERTY
        ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${_installed_runtime_directory}")
    endif()
  endif()
endif()
