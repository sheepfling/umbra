#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace umbra::test_support {

class ScopedTemporaryDirectory final {
 public:
  explicit ScopedTemporaryDirectory(std::filesystem::path path)
      : path_(std::move(path)) {}

  ScopedTemporaryDirectory(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory const&) = delete;
  ScopedTemporaryDirectory(ScopedTemporaryDirectory&& other) noexcept
      : path_(std::move(other.path_)) {
    other.path_.clear();
  }
  ScopedTemporaryDirectory& operator=(ScopedTemporaryDirectory&&) = delete;

  ~ScopedTemporaryDirectory() {
    if (!path_.empty()) {
      std::error_code ignored;
      std::filesystem::remove_all(path_, ignored);
    }
  }

  [[nodiscard]] static ScopedTemporaryDirectory reserve(std::string_view prefix) {
    static std::atomic_uint64_t next{0U};
    auto const parent = std::filesystem::temp_directory_path();
    for (std::uint64_t attempt = 0U; attempt != 1024U; ++attempt) {
      auto const path = parent /
          (std::string(prefix) + "-" + std::to_string(++next) + "-" +
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

  [[nodiscard]] std::filesystem::path const& path() const noexcept {
    return path_;
  }

 private:
  std::filesystem::path path_;
};

}  // namespace umbra::test_support
