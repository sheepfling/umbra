#include "internal/federation/process_federation_service_runtime.hpp"

#include <charconv>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

volatile std::sig_atomic_t shutdownRequested = 0;

void requestShutdown(int) {
  shutdownRequested = 1;
}

bool shouldStop(void*) noexcept {
  return shutdownRequested != 0;
}

void reportListening(
    std::string_view host,
    std::uint16_t port,
    void*) noexcept {
  std::cout << "LISTENING " << host << ' ' << port << std::endl;
}

void printUsage(char const* executable) {
  std::cerr << "Usage: " << executable
            << " <bind-host> <port> <service-report-directory> [--websocket|--tls|--wss]\n"
            << "--websocket serves RFC 6455 at resource /hla; TCP framing is the default.\n"
            << "--tls serves TLS 1.3 over TCP; set the RTI TLS certificate and private-key environment variables.\n"
            << "--wss serves WebSocket over TLS 1.3 at resource /hla; set the same TLS credentials.\n"
            << "Use port 0 to request an OS-assigned port. Send SIGINT or SIGTERM to stop.\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::string_view(argv[1]) == "--help") {
    printUsage(argv[0]);
    return 0;
  }
  auto const transportOption = argc == 5 ? std::string_view(argv[4])
                                         : std::string_view{};
  if ((argc != 4 && argc != 5) ||
      (argc == 5 && transportOption != "--websocket" &&
       transportOption != "--tls" && transportOption != "--wss")) {
    printUsage(argv[0]);
    return 2;
  }

  unsigned int port = 0U;
  auto const* portBegin = argv[2];
  auto const* portEnd = portBegin + std::char_traits<char>::length(portBegin);
  auto const parseResult = std::from_chars(portBegin, portEnd, port);
  if (parseResult.ec != std::errc{} || parseResult.ptr != portEnd ||
      port > UINT16_MAX) {
    std::cerr << "The process coordinator port must be an integer in [0, 65535].\n";
    return 2;
  }

  static_cast<void>(std::signal(SIGINT, requestShutdown));
  static_cast<void>(std::signal(SIGTERM, requestShutdown));
  return umbra::detail::runProcessFederationService(
      argv[1],
      static_cast<std::uint16_t>(port),
      std::filesystem::path(argv[3]),
      reportListening,
      shouldStop,
      nullptr,
      transportOption == "--websocket",
      transportOption == "--tls",
      transportOption == "--wss");
}
