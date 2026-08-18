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
set_target_properties(umbra_rti umbra_fedtime PROPERTIES POSITION_INDEPENDENT_CODE ON)

pybind11_add_module(_native MODULE src/native_module.cpp)
target_compile_features(_native PRIVATE cxx_std_20)
target_link_libraries(_native PRIVATE umbra::rti)

install(TARGETS _native LIBRARY DESTINATION umbra/_native/rti1516_2025 RUNTIME DESTINATION umbra/_native/rti1516_2025)
