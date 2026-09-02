cmake_minimum_required(VERSION 3.23)

project(umbra_rti_jni LANGUAGES CXX)

find_package(JNI REQUIRED)

# This is a bounded provider bridge.  It builds the exact C++ checkout in the
# same embedded profile as the pybind development provider; the product
# wrapper supplies the Java/API/native staging boundary, but neither target
# claims a stable binary SDK ABI or full RTI conformance.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR ON CACHE BOOL "" FORCE)
set(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT ON CACHE BOOL "" FORCE)
set(UMBRA_FETCH_LIBXML2 ON CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/umbra-core" EXCLUDE_FROM_ALL)

# The static C++ RTI is linked into this dynamic JNI library.
set_target_properties(umbra_rti PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(umbra_rti_jni SHARED src/native/umbra_rti_jni.cpp)
target_compile_features(umbra_rti_jni PRIVATE cxx_std_20)
target_compile_definitions(umbra_rti_jni PRIVATE
  UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
)
target_include_directories(umbra_rti_jni PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/src"
)
target_link_libraries(umbra_rti_jni PRIVATE umbra::rti JNI::JNI)
set_target_properties(umbra_rti_jni PROPERTIES OUTPUT_NAME "umbra_rti_jni")
