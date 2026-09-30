if(NOT WIN32)
  message(FATAL_ERROR "The mingw-scoop toolchain is for native Windows builds only.")
endif()

set(_umbra_scoop_root "$ENV{SCOOP}")
if(_umbra_scoop_root STREQUAL "")
  set(_umbra_scoop_root "$ENV{USERPROFILE}/scoop")
endif()

set(_umbra_scoop_gcc_bin "${_umbra_scoop_root}/apps/gcc/current/bin")
set(CMAKE_C_COMPILER "${_umbra_scoop_gcc_bin}/gcc.exe" CACHE FILEPATH "Scoop MinGW C compiler")
set(CMAKE_CXX_COMPILER "${_umbra_scoop_gcc_bin}/g++.exe" CACHE FILEPATH "Scoop MinGW C++ compiler")
if(NOT DEFINED CMAKE_RC_COMPILER OR CMAKE_RC_COMPILER STREQUAL "windres")
  set(CMAKE_RC_COMPILER
    "${_umbra_scoop_gcc_bin}/windres.exe"
    CACHE FILEPATH "Scoop MinGW resource compiler" FORCE
  )
endif()
set(CMAKE_AR "${_umbra_scoop_gcc_bin}/ar.exe" CACHE FILEPATH "Scoop MinGW archiver")
set(CMAKE_RANLIB "${_umbra_scoop_gcc_bin}/ranlib.exe" CACHE FILEPATH "Scoop MinGW archive indexer")

if(NOT EXISTS "${CMAKE_C_COMPILER}" OR
   NOT EXISTS "${CMAKE_CXX_COMPILER}" OR
   NOT EXISTS "${CMAKE_RC_COMPILER}" OR
   NOT EXISTS "${CMAKE_AR}" OR
   NOT EXISTS "${CMAKE_RANLIB}")
  message(FATAL_ERROR
    "Scoop MinGW GCC was not found under '${_umbra_scoop_gcc_bin}'. "
    "Install it with 'scoop install gcc' or set CMAKE_C_COMPILER and CMAKE_CXX_COMPILER explicitly."
  )
endif()

unset(_umbra_scoop_root)
unset(_umbra_scoop_gcc_bin)
