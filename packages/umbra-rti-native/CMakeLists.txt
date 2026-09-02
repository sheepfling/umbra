cmake_minimum_required(VERSION 3.23)

project(umbra_rti_native LANGUAGES CXX)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
find_package(pybind11 CONFIG REQUIRED)

# The first provider is intentionally built from the monorepo. When the C++
# SDK gains a standalone ABI/package contract, replace this with find_package
# (umbra_rti CONFIG REQUIRED) and build wheels against that published SDK.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/umbra-core" EXCLUDE_FROM_ALL)

# Static core libraries need PIC when linked into a POSIX extension module.
set_target_properties(umbra_rti umbra_rti_2010 umbra_fedtime PROPERTIES POSITION_INDEPENDENT_CODE ON)

pybind11_add_module(_native MODULE src/native_module.cpp)
target_compile_features(_native PRIVATE cxx_std_20)
target_link_libraries(_native PRIVATE umbra::rti)
# Umbra's opaque-handle codecs are deliberately provider-private.  The binding
# uses them only to reconstruct standard C++ handle values from the portable
# public Python encoding before invoking a real RTI service.
target_include_directories(_native PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/src")

install(TARGETS _native LIBRARY DESTINATION umbra/_native/rti1516_2025 RUNTIME DESTINATION umbra/_native/rti1516_2025)

# The 2010 target is intentionally a separate extension and Python namespace.
# It links the exact rti1516e C++ ABI and exposes only the bounded
# connect/disconnect, scalar/composite encoders, reference logical time, and a
# provider-owned federation/declaration/object callback slice currently
# implemented by the core; remaining RTI/MOM services stay explicit
# unsupported until their stateful 2010 implementations are added.
pybind11_add_module(_native_2010 MODULE src/native_module_2010.cpp)
target_compile_features(_native_2010 PRIVATE cxx_std_14)
target_link_libraries(_native_2010 PRIVATE umbra::rti_2010)
target_include_directories(_native_2010 PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ieee1516.1-2010/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cpp/src"
)
install(TARGETS _native_2010 LIBRARY DESTINATION umbra/_native/rti1516e RUNTIME DESTINATION umbra/_native/rti1516e)
