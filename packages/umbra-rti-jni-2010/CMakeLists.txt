cmake_minimum_required(VERSION 3.23)

project(umbra_rti_jni_2010 LANGUAGES CXX)

find_package(JNI REQUIRED)

# The product wrapper stages this bounded provider with the standard API JAR;
# this native target intentionally does not claim complete 1516e conformance.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/umbra-core" EXCLUDE_FROM_ALL)

set_target_properties(umbra_rti_2010 PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(umbra_rti_jni_2010 SHARED src/native/umbra_rti_jni_2010.cpp)
target_compile_features(umbra_rti_jni_2010 PRIVATE cxx_std_14)
target_include_directories(umbra_rti_jni_2010 PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ieee1516.1-2010/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/src"
)
target_link_libraries(umbra_rti_jni_2010 PRIVATE umbra::rti_2010 JNI::JNI)
set_target_properties(umbra_rti_jni_2010 PROPERTIES OUTPUT_NAME "umbra_rti_jni_2010")
