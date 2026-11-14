#include "process_tls_encryption_only_fixture.hpp"

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "support/tls_test_certificate.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace umbra::test {

namespace {

void writeText(
    std::filesystem::path const& path,
    std::string const& value) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output << value;
  if (!output) {
    throw std::runtime_error(
        "Writing the TLS encryption-only fixture marker failed.");
  }
}

void waitForTckCompletion(std::filesystem::path const& markerPath) {
  auto const deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(90);
  while (std::chrono::steady_clock::now() < deadline) {
    if (std::filesystem::exists(markerPath)) {
      std::ifstream input(markerPath, std::ios::binary);
      std::string value;
      input >> value;
      if (!input || value != "complete") {
        throw std::runtime_error(
            "The TLS TCK completion marker is invalid.");
      }
      return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
  }
  throw std::runtime_error(
      "The TLS TCK completion marker timed out.");
}

int runTlsServiceWithCredentials(
    std::filesystem::path const& directory,
    std::filesystem::path const& certificatePath,
    std::filesystem::path const& privateKeyPath,
    std::string const& endpointId,
    std::filesystem::path const& completionMarker,
    std::uint16_t listenPort = 0U,
    bool websocket = false) {
  detail::EmbeddedFederationRegistry registry;
  detail::ProcessTransportAddress address;
  address.host = "127.0.0.1";
  address.port = listenPort;
  address.security = detail::ProcessTransportAddress::Security::tls13;
  if (websocket) {
    address.framing = detail::ProcessTransportAddress::Framing::websocket;
    address.websocketResource = "/hla";
  }
  detail::ProcessFederationServiceHostOptions hostOptions;
  hostOptions.tlsServerConfiguration =
      detail::platform::TlsServerConfiguration{
          certificatePath, privateKeyPath};
  detail::ProcessFederationServiceHost host(
      registry,
      detail::FederationDefinition{},
      std::move(address),
      {endpointId, 0U},
      detail::ProcessFederationServiceOptions{},
      std::move(hostOptions));
  host.start();
  writeText(directory / "port.txt", std::to_string(host.address().port));
  waitForTckCompletion(completionMarker);
  host.stop();
  host.rethrowIfFailed();
  return 0;
}

}  // namespace

int runTlsEncryptionOnlyServiceFixture(
    std::filesystem::path const& directory) {
  auto const credentialsDirectory = directory / "tls-credentials";
  std::filesystem::create_directories(credentialsDirectory);
  auto const certificatePath = credentialsDirectory / "localhost-only.crt";
  auto const privateKeyPath = credentialsDirectory / "localhost-only.key";
  tls::writeTestCertificate(
      certificatePath, privateKeyPath, "DNS:localhost");
  return runTlsServiceWithCredentials(
      directory,
      certificatePath,
      privateKeyPath,
      "portable-tck-tls-encryption-only",
      directory / "tls-tck-complete.ok");
}

int runTlsServerValidationServiceFixture(
    std::filesystem::path const& directory) {
  auto const credentialsDirectory = directory / "tls-credentials";
  std::filesystem::create_directories(credentialsDirectory);
  auto const serverCertificatePath =
      credentialsDirectory / "server-ip.crt";
  auto const serverPrivateKeyPath =
      credentialsDirectory / "server-ip.key";
  auto const untrustedCertificatePath =
      credentialsDirectory / "untrusted-ip.crt";
  auto const untrustedPrivateKeyPath =
      credentialsDirectory / "untrusted-ip.key";
  tls::writeTestCertificate(
      serverCertificatePath, serverPrivateKeyPath, "IP:127.0.0.1");
  tls::writeTestCertificate(
      untrustedCertificatePath, untrustedPrivateKeyPath, "IP:127.0.0.1");
  return runTlsServiceWithCredentials(
      directory,
      serverCertificatePath,
      serverPrivateKeyPath,
      "portable-tck-tls-server-validation",
      directory / "tls-server-validation-tck-complete.ok");
}

int runTlsDefaultPortServiceFixture(
    std::filesystem::path const& directory) {
  auto const credentialsDirectory = directory / "tls-credentials";
  std::filesystem::create_directories(credentialsDirectory);
  auto const certificatePath = credentialsDirectory / "server-ip.crt";
  auto const privateKeyPath = credentialsDirectory / "server-ip.key";
  tls::writeTestCertificate(
      certificatePath, privateKeyPath, "IP:127.0.0.1");
  return runTlsServiceWithCredentials(
      directory,
      certificatePath,
      privateKeyPath,
      "portable-tck-tls-default-port",
      directory / "tls-default-port-tck-complete.ok",
      15165U);
}

int runTlsAlternatePortServiceFixture(
    std::filesystem::path const& directory) {
  auto const credentialsDirectory = directory / "tls-credentials";
  std::filesystem::create_directories(credentialsDirectory);
  auto const certificatePath = credentialsDirectory / "server-ip.crt";
  auto const privateKeyPath = credentialsDirectory / "server-ip.key";
  tls::writeTestCertificate(
      certificatePath, privateKeyPath, "IP:127.0.0.1");
  return runTlsServiceWithCredentials(
      directory,
      certificatePath,
      privateKeyPath,
      "portable-tck-tls-alternate-port",
      directory / "tls-alternate-port-tck-complete.ok",
      15166U);
}

int runTlsSecureWebSocketServiceFixture(
    std::filesystem::path const& directory) {
  auto const credentialsDirectory = directory / "tls-credentials";
  std::filesystem::create_directories(credentialsDirectory);
  auto const certificatePath = credentialsDirectory / "server-ip.crt";
  auto const privateKeyPath = credentialsDirectory / "server-ip.key";
  tls::writeTestCertificate(
      certificatePath, privateKeyPath, "IP:127.0.0.1");
  return runTlsServiceWithCredentials(
      directory,
      certificatePath,
      privateKeyPath,
      "portable-tck-secure-websocket",
      directory / "secure-websocket-tck-complete.ok",
      0U,
      true);
}

}  // namespace umbra::test
