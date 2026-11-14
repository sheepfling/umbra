#include "process_federation_service_runtime_probe.hpp"

#include "internal/federation/process_federation_service_runtime.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>

namespace {

struct RuntimeProbeContext final {
  std::filesystem::path directory;
  bool callbackFailed = false;
};

void publishAddress(
    std::string_view host,
    std::uint16_t port,
    void* userData) noexcept {
  auto& context = *static_cast<RuntimeProbeContext*>(userData);
  try {
    auto const temporaryPath = context.directory / "coordinator.address.tmp";
    auto const addressPath = context.directory / "coordinator.address";
    std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
    output << host << ' ' << port << '\n';
    output.close();
    if (!output) {
      context.callbackFailed = true;
      return;
    }
    std::filesystem::rename(temporaryPath, addressPath);
  } catch (...) {
    context.callbackFailed = true;
  }
}

bool stopRequested(void* userData) noexcept {
  auto& context = *static_cast<RuntimeProbeContext*>(userData);
  if (context.callbackFailed) {
    return true;
  }
  std::error_code error;
  auto const requested = std::filesystem::exists(
      context.directory / "coordinator.stop",
      error);
  return requested || static_cast<bool>(error);
}

}  // namespace

int runProcessFederationServiceRuntimeProbe(
    std::filesystem::path const& directory) noexcept {
  RuntimeProbeContext context{directory};
  return umbra::detail::runProcessFederationService(
      "127.0.0.1",
      0U,
      directory / "service-reports",
      publishAddress,
      stopRequested,
      &context);
}
