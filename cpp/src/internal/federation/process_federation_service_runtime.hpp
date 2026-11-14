#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

#if defined(_WIN32) && defined(BUILDING_RTI)
#define UMBRA_PROCESS_FEDERATION_RUNTIME_API __declspec(dllexport)
#elif defined(_WIN32) && defined(UMBRA_PROCESS_FEDERATION_RUNTIME_IMPORT)
#define UMBRA_PROCESS_FEDERATION_RUNTIME_API __declspec(dllimport)
#elif defined(__GNUC__)
#define UMBRA_PROCESS_FEDERATION_RUNTIME_API __attribute__((visibility("default")))
#else
#define UMBRA_PROCESS_FEDERATION_RUNTIME_API
#endif

namespace umbra::detail {

using ProcessFederationServiceListeningCallback = void (*)(
    std::string_view host,
    std::uint16_t port,
    void* userData) noexcept;
using ProcessFederationServiceStopPredicate = bool (*)(void* userData) noexcept;

// Runtime entry point shared by the installed coordinator executable and
// process-level integration probes. The service owns one authoritative
// registry for its lifetime; callers provide only deployment configuration
// and lifecycle callbacks.
[[nodiscard]] UMBRA_PROCESS_FEDERATION_RUNTIME_API int
runProcessFederationService(
    std::string_view listenHost,
    std::uint16_t listenPort,
    std::filesystem::path const& serviceReportDirectory,
    ProcessFederationServiceListeningCallback onListening,
    ProcessFederationServiceStopPredicate stopRequested,
    void* userData,
    bool websocketTransport = false,
    bool tlsTransport = false,
    bool secureWebSocketTransport = false) noexcept;

}  // namespace umbra::detail
