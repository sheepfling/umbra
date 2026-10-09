if(UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR)
  find_package(LibXml2 2.15 CONFIG QUIET)
  if(NOT TARGET LibXml2::LibXml2 AND UMBRA_FETCH_LIBXML2)
    include(FetchContent)
    # Keep the private validator self-contained for its test target. The
    # completed SDK packaging decision is deferred until public RTI services
    # link this backend.
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_CATALOG OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_DOCS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_HTML OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_HTTP OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_LEGACY OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_MODULES OFF CACHE BOOL "" FORCE)
    # The private materializer serializes a schema-validated FDD into an
    # immutable runtime artifact. This does not enable XML network loading or
    # external entity resolution; those stay disabled in the document loader.
    set(LIBXML2_WITH_OUTPUT ON CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_VALID OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_XINCLUDE OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_XPATH OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "" FORCE)
    set(LIBXML2_WITH_SCHEMAS ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
      LibXml2
      URL https://download.gnome.org/sources/libxml2/2.15/libxml2-2.15.3.tar.xz
      URL_HASH SHA256=78262a6e7ac170d6528ebfe2efccdf220191a5af6a6cd61ea4a9a9a5042c7a07
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(LibXml2)
  endif()
  if(NOT TARGET LibXml2::LibXml2)
    message(FATAL_ERROR
      "UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR requires LibXml2 2.15+; configure with -DUMBRA_FETCH_LIBXML2=ON to fetch the pinned source."
    )
  endif()

  add_library(umbra_fom_validation_backend STATIC
    cpp/src/internal/fom/libxml2_fom_composer.cpp
    cpp/src/internal/fom/libxml2_fom_composer_validators.cpp
    cpp/src/internal/fom/libxml2_fom_document.cpp
    cpp/src/internal/fom/libxml2_fom_validator.cpp
  )
  target_compile_features(umbra_fom_validation_backend PRIVATE cxx_std_20)
  target_include_directories(umbra_fom_validation_backend PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpp/src"
  )
  target_link_libraries(umbra_fom_validation_backend PRIVATE LibXml2::LibXml2)
  set_target_properties(umbra_fom_validation_backend PROPERTIES
    EXPORT_NAME fom_validation_backend
  )

  if(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
    # Keep both locations in the binary. The source-tree location makes the
    # development profile deterministic; the installed location lets the
    # same binary use the reviewed 1516.2 resources after installation. The
    # package config declares the matching LibXml2 dependency below.
    target_compile_definitions(umbra_rti PRIVATE
      UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT=1
      UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY=L"${UMBRA_IEEE1516_2_2025_RESOURCE_DIR}"
      UMBRA_EMBEDDED_FOM_RESOURCE_DIRECTORY_INSTALL=L"${CMAKE_INSTALL_FULL_DATADIR}/umbra_rti/ieee1516.2-2025"
    )
    if(UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY)
      target_compile_definitions(umbra_rti PRIVATE
        UMBRA_EMBEDDED_FOM_2010_RESOURCE_DIRECTORY=L"${UMBRA_EXTERNAL_2010_FOM_RESOURCE_DIRECTORY}"
      )
    endif()
    target_link_libraries(umbra_rti PRIVATE umbra_fom_validation_backend)
  endif()
elseif(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  message(FATAL_ERROR
    "UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT requires UMBRA_ENABLE_LIBXML2_FOM_VALIDATOR=ON."
  )
endif()
