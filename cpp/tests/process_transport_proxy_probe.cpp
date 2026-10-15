#include "internal/federation/process_transport_proxy_host.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void writeMarker(
    std::filesystem::path const& path,
    std::string const& content) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output || !(output << content)) {
    throw std::runtime_error("Could not write process proxy marker: " +
                             path.string());
  }
}

void publishPortMarker(
    std::filesystem::path const& path,
    unsigned short port) {
  auto temporary = path;
  temporary += ".tmp";
  writeMarker(temporary, std::to_string(port));
  // Readers must not observe a partially written (but parseable) port.
  std::filesystem::rename(temporary, path);
}

unsigned short readUpstreamPort(std::filesystem::path const& path) {
  std::ifstream input(path, std::ios::binary);
  unsigned int port = 0U;
  if (!(input >> port) || port == 0U || port > 65535U) {
    throw std::runtime_error("The authority port marker is invalid.");
  }
  return static_cast<unsigned short>(port);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }

  std::filesystem::path const directory(argv[1]);
  std::string const role(argv[2]);
  std::string const markerPrefix = role + ".";
  try {
    std::uint16_t identity = 0U;
    if (role == "sender") {
      identity = 0xB821U;
    } else if (role == "receiver") {
      identity = 0xB822U;
    } else {
      throw std::invalid_argument("Unknown process proxy role.");
    }

    umbra::detail::ProcessTransportProxyHost proxy(
        {"127.0.0.1", 0U},
        {"127.0.0.1", readUpstreamPort(directory / "authority.port")},
        {"process-tso-subprocess-" + role, identity});
    proxy.start();
    publishPortMarker(
        directory / (markerPrefix + "port"), proxy.address().port);

    auto const deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(20);
    auto const stopPath = directory / (markerPrefix + "stop");
    while (std::chrono::steady_clock::now() < deadline &&
           !std::filesystem::exists(stopPath)) {
      if (!proxy.running()) {
        proxy.rethrowIfFailed();
        throw std::runtime_error("The process proxy stopped unexpectedly.");
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!std::filesystem::exists(stopPath)) {
      throw std::runtime_error("The process proxy exceeded its lifetime bound.");
    }

    proxy.stop();
    proxy.rethrowIfFailed();
    writeMarker(directory / (markerPrefix + "ok"), "stopped");
    return 0;
  } catch (std::exception const& error) {
    try {
      writeMarker(
          directory / (markerPrefix + "error"),
          std::string(error.what()));
    } catch (...) {
    }
    return 1;
  }
}
