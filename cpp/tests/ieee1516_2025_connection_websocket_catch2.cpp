#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "support/tls_test_certificate.hpp"
#include "tck_tls_websocket_transport.hpp"
#include "tck_websocket_transport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)

namespace {

class ScopedTlsTrustStore final {
 public:
  explicit ScopedTlsTrustStore(std::filesystem::path const& certificatePath) {
    constexpr char name[] = "UMBRA_RTI_TLS_TRUST_STORE_FILE";
    if (auto const* previous = std::getenv(name); previous != nullptr) {
      previous_ = previous;
    }
#if defined(_WIN32)
    auto const result = _putenv_s(name, certificatePath.string().c_str());
#else
    auto const result =
        ::setenv(name, certificatePath.string().c_str(), 1);
#endif
    if (result != 0) {
      throw std::runtime_error("Setting the secure WebSocket test trust store failed.");
    }
  }

  ScopedTlsTrustStore(ScopedTlsTrustStore const&) = delete;
  ScopedTlsTrustStore& operator=(ScopedTlsTrustStore const&) = delete;

  ~ScopedTlsTrustStore() {
#if defined(_WIN32)
    if (previous_) {
      static_cast<void>(
          _putenv_s("UMBRA_RTI_TLS_TRUST_STORE_FILE", previous_->c_str()));
    } else {
      static_cast<void>(_putenv_s("UMBRA_RTI_TLS_TRUST_STORE_FILE", ""));
    }
#else
    if (previous_) {
      static_cast<void>(::setenv(
          "UMBRA_RTI_TLS_TRUST_STORE_FILE", previous_->c_str(), 1));
    } else {
      static_cast<void>(::unsetenv("UMBRA_RTI_TLS_TRUST_STORE_FILE"));
    }
#endif
  }

 private:
  std::optional<std::string> previous_;
};

}  // namespace

TEST_CASE(
    "RTIambassador Connect completes a binary WebSocket process-session handshake",
    "[integration][foundation][connection][transport][process-boundary][public-endpoint][websocket][2025][websocket-public-rtiambassador-connect]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;

  EmbeddedFederationRegistry registry;
  ProcessTransportAddress address;
  address.host = "127.0.0.1";
  address.framing = ProcessTransportAddress::Framing::websocket;
  address.websocketResource = "/hla";
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      std::move(address),
      {"websocket-public-api-test-authority", 0U});
  host.start();
  auto const endpoint = host.address();
  REQUIRE(endpoint.port != 0U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"websocket-public-api-client")
                           .withRtiAddress(
                               L"ws://127.0.0.1:" +
                               std::to_wstring(endpoint.port) + L"/hla");

  std::optional<ConfigurationResult> result;
  std::exception_ptr clientFailure;
  try {
    result = rti->connect(federate, HLA_EVOKED, configuration);
    rti->disconnect();

    hla_rti_cpp_tck::detail::Options tckOptions;
    tckOptions.rtiAddress =
        "ws://127.0.0.1:" + std::to_string(endpoint.port) + "/hla";
    hla_rti_cpp_tck::detail::scenarioWebSocketTransportSupport(
        tckOptions, HLA_EVOKED);
    hla_rti_cpp_tck::detail::scenarioWebSocketTransportSupport(
        tckOptions, HLA_IMMEDIATE);
  } catch (...) {
    clientFailure = std::current_exception();
  }
  host.stop();

  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  host.rethrowIfFailed();
  REQUIRE(result.has_value());
  CHECK(result->configurationUsed);
  CHECK(result->addressUsed);
  CHECK(result->additionalSettingsResult == SETTINGS_IGNORED);
  CHECK(endpoint.framing == ProcessTransportAddress::Framing::websocket);
  CHECK(endpoint.websocketResource == "/hla");
}

TEST_CASE(
    "RTIambassador Connect completes a secure WebSocket TLS 1.3 process-session handshake",
    "[integration][foundation][connection][transport][process-boundary][public-endpoint][tls][websocket][secure-websocket][2025][secure-websocket-public-rtiambassador-connect]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;

  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  umbra::test::tls::writeTestCertificate(certificatePath, privateKeyPath);
  ScopedTlsTrustStore trustStore(certificatePath);

  EmbeddedFederationRegistry registry;
  ProcessTransportAddress address;
  address.host = "127.0.0.1";
  address.security = ProcessTransportAddress::Security::tls13;
  address.framing = ProcessTransportAddress::Framing::websocket;
  address.websocketResource = "/hla";
  ProcessFederationServiceHostOptions hostOptions;
  hostOptions.tlsServerConfiguration = platform::TlsServerConfiguration{
      certificatePath,
      privateKeyPath};
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      std::move(address),
      {"secure-websocket-public-api-test-authority", 0U},
      ProcessFederationServiceOptions{},
      std::move(hostOptions));
  host.start();
  auto const endpoint = host.address();
  REQUIRE(endpoint.port != 0U);

  hla_rti_cpp_tck::detail::Options tckOptions;
  tckOptions.rtiAddress =
      "wss://127.0.0.1:" + std::to_string(endpoint.port) + "/hla";
  tckOptions.additionalSettings = "tlsCertificateValidation=server";
  std::exception_ptr clientFailure;
  try {
    hla_rti_cpp_tck::detail::scenarioSecureWebSocketTransportSupport(
        tckOptions, HLA_EVOKED);
    hla_rti_cpp_tck::detail::scenarioSecureWebSocketTransportSupport(
        tckOptions, HLA_IMMEDIATE);
  } catch (...) {
    clientFailure = std::current_exception();
  }
  host.stop();

  if (clientFailure) {
    std::rethrow_exception(clientFailure);
  }
  host.rethrowIfFailed();
  CHECK(endpoint.security == ProcessTransportAddress::Security::tls13);
  CHECK(endpoint.framing == ProcessTransportAddress::Framing::websocket);
  CHECK(endpoint.websocketResource == "/hla");
}

#endif
