cmake_minimum_required(VERSION 3.23)

project(umbra_rti_package_smoke LANGUAGES CXX)

find_package(umbra_rti CONFIG REQUIRED)

add_executable(umbra_rti_package_smoke main.cpp)
target_compile_features(umbra_rti_package_smoke PRIVATE cxx_std_20)
target_link_libraries(umbra_rti_package_smoke PRIVATE umbra::rti umbra::fedtime)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME umbra_rti_package_consumer COMMAND umbra_rti_package_smoke)
endif()
