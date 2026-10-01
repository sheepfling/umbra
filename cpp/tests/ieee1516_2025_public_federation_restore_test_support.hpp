#pragma once

#include "ieee1516_2025_federation_management_test_support.hpp"

namespace public_federation_restore_test_support {

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path)
      : path_(std::move(path)) {}
  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;
  ~ScopedTemporaryDirectory() {
    std::error_code ignored;
    std::filesystem::remove_all(path_, ignored);
  }
  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

inline ScopedTemporaryDirectory reserveDirectory(std::string_view prefix) {
  static std::atomic_uint64_t counter{0};
  auto const parent = std::filesystem::temp_directory_path();
  auto const tick = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  auto const process = static_cast<std::uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
  for (std::size_t attempt = 0U; attempt != 1024U; ++attempt) {
    auto const path = parent /
        (std::string(prefix) + "-" + std::to_string(tick) + "-" +
         std::to_string(process) + "-" + std::to_string(++counter) + "-" +
         std::to_string(attempt));
    std::error_code error;
    if (std::filesystem::create_directory(path, error)) {
      return ScopedTemporaryDirectory(path);
    }
    if (error && error != std::errc::file_exists) {
      throw std::filesystem::filesystem_error(
          "Unable to reserve temporary directory", path, error);
    }
  }
  throw std::runtime_error("Unable to reserve a unique temporary directory.");
}

inline ScopedTemporaryDirectory temporaryServiceReportDirectory() {
  return reserveDirectory("umbra-public-directed-tso-report");
}

inline ScopedTemporaryDirectory temporaryFederationSaveDirectory() {
  return reserveDirectory("umbra-public-directed-tso-save");
}

class ScopedEmbeddedFederationRegistry final {
 public:
  explicit ScopedEmbeddedFederationRegistry(
      std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> replacement)
      : previous_(
            rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
                std::move(replacement))) {}
  ScopedEmbeddedFederationRegistry(ScopedEmbeddedFederationRegistry const&) = delete;
  ScopedEmbeddedFederationRegistry& operator=(ScopedEmbeddedFederationRegistry const&) = delete;
  ~ScopedEmbeddedFederationRegistry() noexcept {
    if (previous_) {
      static_cast<void>(
          rti1516_2025::umbra_binding_detail::replaceEmbeddedFederationRegistryForTesting(
              std::move(previous_)));
    }
  }

 private:
  std::shared_ptr<umbra::detail::EmbeddedFederationRegistry> previous_;
};

[[nodiscard]] inline RtiConfiguration configurationForServiceReportDirectory(
    std::filesystem::path const& directory) {
  return umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
}

inline std::vector<std::filesystem::path> serviceReportFiles(
    std::filesystem::path const& directory) {
  std::vector<std::filesystem::path> files;
  for (auto const& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

inline std::string readTextFile(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  REQUIRE(input.good());
  return {
      std::istreambuf_iterator<char>(input),
      std::istreambuf_iterator<char>()};
}

}  // namespace public_federation_restore_test_support
