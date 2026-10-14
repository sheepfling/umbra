#pragma once

#include <filesystem>
#include <optional>
#include <system_error>
#include <utility>

namespace umbra::detail {

// A report file is allocated before a Join's MOM object and routes are
// committed. Remove that unadvertised file if the joined lifetime rolls back.
class PendingServiceReportFileCleanup final {
 public:
  PendingServiceReportFileCleanup() = default;
  PendingServiceReportFileCleanup(PendingServiceReportFileCleanup const&) = delete;
  PendingServiceReportFileCleanup& operator=(PendingServiceReportFileCleanup const&) = delete;

  ~PendingServiceReportFileCleanup() {
    if (!location_) {
      return;
    }
    std::error_code ignored;
    std::filesystem::remove(*location_, ignored);
  }

  void arm(std::filesystem::path location) {
    if (!location.empty()) {
      location_ = std::move(location);
    }
  }

  void commit() noexcept { location_.reset(); }

 private:
  std::optional<std::filesystem::path> location_;
};

}  // namespace umbra::detail
