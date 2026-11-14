#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#ifndef UMBRA_PACKAGE_COMPILER_MARKER
#error "The installed package must publish its HLA compiler marker"
#endif
#ifndef UMBRA_PACKAGE_COMPILER_MARKER_SOURCE
#error "The installed package must identify its compiler marker source"
#endif
#ifndef UMBRA_PACKAGE_32BIT_MARKER
#error "The installed package must publish its bitness marker"
#endif
#ifndef UMBRA_PACKAGE_DEBUG_BUILD
#error "The package smoke configuration must identify the build mode"
#endif
#ifndef UMBRA_PACKAGE_RTI_LIBRARY_PATH
#error "The installed RTI library path is required"
#endif
#ifndef UMBRA_PACKAGE_FEDTIME_LIBRARY_PATH
#error "The installed logical-time library path is required"
#endif
#ifndef UMBRA_PACKAGE_AUTHORIZER_LIBRARY_PATH
#error "The installed authorizer library path is required"
#endif

namespace {

struct Library final {
  std::string_view base_name;
  std::filesystem::path path;
};

bool hasHlaSharedLibraryName(std::string_view filename,
                             std::string_view expected_prefix) {
#if defined(_WIN32)
  return filename == std::string(expected_prefix) + ".dll";
#elif defined(__APPLE__)
  std::string const exact = std::string(expected_prefix) + ".dylib";
  std::string const versioned_prefix = std::string(expected_prefix) + ".";
  return filename == exact ||
         (filename.starts_with(versioned_prefix) && filename.ends_with(".dylib"));
#else
  std::string const exact = std::string(expected_prefix) + ".so";
  std::string const versioned_prefix = exact + ".";
  return filename == exact || filename.starts_with(versioned_prefix);
#endif
}

}  // namespace

int main() {
  std::string expected_prefix{"1516_2023"};
  expected_prefix += UMBRA_PACKAGE_COMPILER_MARKER;
  expected_prefix += UMBRA_PACKAGE_32BIT_MARKER;
#if UMBRA_PACKAGE_DEBUG_BUILD
  expected_prefix += 'd';
#endif

  std::array<Library, 3> const libraries{{
      {"rti", UMBRA_PACKAGE_RTI_LIBRARY_PATH},
      {"fedtime", UMBRA_PACKAGE_FEDTIME_LIBRARY_PATH},
      {"authorizer", UMBRA_PACKAGE_AUTHORIZER_LIBRARY_PATH},
  }};

  for (Library const& library : libraries) {
    if (!std::filesystem::exists(library.path)) {
      std::cerr << "Installed " << library.base_name << " library is missing: "
                << library.path << '\n';
      return 1;
    }

    std::string expected_name{"lib"};
    expected_name += library.base_name;
    expected_name += expected_prefix;
    if (!hasHlaSharedLibraryName(library.path.filename().string(), expected_name)) {
      std::cerr << "Installed " << library.base_name << " library name does not "
                << "match the HLA 1516_2023 nomenclature: "
                << library.path.filename() << " (marker="
                << UMBRA_PACKAGE_COMPILER_MARKER << ", marker-source="
                << UMBRA_PACKAGE_COMPILER_MARKER_SOURCE << ")\n";
      return 1;
    }
  }

  std::cout << "Installed HLA shared-library basenames match 1516_2023 format; "
            << "compiler marker=" << UMBRA_PACKAGE_COMPILER_MARKER
            << " (" << UMBRA_PACKAGE_COMPILER_MARKER_SOURCE << ")\n";
  return 0;
}
