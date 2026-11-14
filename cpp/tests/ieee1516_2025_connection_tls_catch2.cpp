#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_service_host.hpp"
#include "support/tls_test_certificate.hpp"

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

class ScopedEnvironmentVariable final {
 public:
  ScopedEnvironmentVariable(std::string name, std::string value)
      : name_(std::move(name)) {
    if (auto const* previous = std::getenv(name_.c_str());
        previous != nullptr) {
      previous_ = previous;
    }
#if defined(_WIN32)
    auto const result = _putenv_s(name_.c_str(), value.c_str());
#else
    auto const result = ::setenv(name_.c_str(), value.c_str(), 1);
#endif
    if (result != 0) {
      throw std::runtime_error(
          "Setting the TLS test environment variable failed.");
    }
  }

  ScopedEnvironmentVariable(ScopedEnvironmentVariable const&) = delete;
  ScopedEnvironmentVariable& operator=(ScopedEnvironmentVariable const&) = delete;

  ~ScopedEnvironmentVariable() {
#if defined(_WIN32)
    if (previous_) {
      static_cast<void>(_putenv_s(name_.c_str(), previous_->c_str()));
    } else {
      static_cast<void>(_putenv_s(name_.c_str(), ""));
    }
#else
    if (previous_) {
      static_cast<void>(::setenv(name_.c_str(), previous_->c_str(), 1));
    } else {
      static_cast<void>(::unsetenv(name_.c_str()));
    }
#endif
  }

 private:
  std::string name_;
  std::optional<std::string> previous_;
};

}  // namespace

TEST_CASE(
    "RTIambassador Connect uses the configured TLS 1.3 process endpoint",
    "[integration][connection][transport][process-boundary][tls][2025][rtiambassador.connect]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  umbra::test::tls::writeTestCertificate(certificatePath, privateKeyPath);
  ScopedEnvironmentVariable trustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", certificatePath.string());
  ScopedEnvironmentVariable certificateChain(
      "UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE", certificatePath.string());
  ScopedEnvironmentVariable privateKey(
      "UMBRA_RTI_TLS_PRIVATE_KEY_FILE", privateKeyPath.string());

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.serviceReportDirectory =
      temporaryDirectory.path() / "service-reports";
  std::filesystem::create_directories(serviceOptions.serviceReportDirectory);
  auto hostOptions = processFederationServiceHostOptionsFromRtiInitializationData();
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      ProcessTransportAddress{
          "127.0.0.1",
          0U,
          ProcessTransportAddress::Security::tls13,
          {}},
      {"tls-public-api-test-authority", 0U},
      std::move(serviceOptions),
      std::move(hostOptions));
  host.start();
  auto const address = host.address();
  REQUIRE(address.port != 0U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"tls-public-api-client")
                           .withRtiAddress(
                               L"tls://127.0.0.1:" +
                               std::to_wstring(address.port))
                           .withAdditionalSettings(
                               L"tlsCertificateValidation=server");
  std::optional<ConfigurationResult> result;
  std::exception_ptr clientFailure;
  try {
    result = rti->connect(federate, HLA_EVOKED, configuration);
    rti->disconnect();
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
  CHECK(result->additionalSettingsResult == SETTINGS_APPLIED);
  CHECK(host.address().security == ProcessTransportAddress::Security::tls13);
}

TEST_CASE(
    "RTIambassador Connect rejects an untrusted TLS peer and remains reusable",
    "[integration][connection][transport][process-boundary][tls][2025][rtiambassador.connect][tls-peer-validation]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  auto const untrustedCertificatePath =
      temporaryDirectory.path() / "untrusted.crt";
  auto const untrustedPrivateKeyPath =
      temporaryDirectory.path() / "untrusted.key";
  umbra::test::tls::writeTestCertificate(certificatePath, privateKeyPath);
  umbra::test::tls::writeTestCertificate(
      untrustedCertificatePath, untrustedPrivateKeyPath);

  ScopedEnvironmentVariable untrustedTrustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", untrustedCertificatePath.string());
  ScopedEnvironmentVariable certificateChain(
      "UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE", certificatePath.string());
  ScopedEnvironmentVariable privateKey(
      "UMBRA_RTI_TLS_PRIVATE_KEY_FILE", privateKeyPath.string());

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.serviceReportDirectory =
      temporaryDirectory.path() / "service-reports";
  std::filesystem::create_directories(serviceOptions.serviceReportDirectory);
  auto hostOptions = processFederationServiceHostOptionsFromRtiInitializationData();
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      ProcessTransportAddress{
          "127.0.0.1",
          0U,
          ProcessTransportAddress::Security::tls13,
          {}},
      {"tls-untrusted-peer-test-authority", 0U},
      std::move(serviceOptions),
      std::move(hostOptions));
  host.start();
  auto const address = host.address();
  REQUIRE(address.port != 0U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withConfigurationName(L"tls-untrusted-peer-client")
                           .withRtiAddress(
                               L"tls://127.0.0.1:" +
                               std::to_wstring(address.port));

  REQUIRE_THROWS_AS(
      rti->connect(federate, HLA_EVOKED, configuration), ConnectionFailed);
  CHECK(host.running());

  ScopedEnvironmentVariable trustedTrustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", certificatePath.string());
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  CHECK(result.configurationUsed);
  CHECK(result.addressUsed);
  rti->disconnect();
  host.stop();
  host.rethrowIfFailed();
}

TEST_CASE(
    "RTIambassador Connect rejects a trusted TLS peer with a hostname mismatch and remains reusable",
    "[integration][connection][transport][process-boundary][tls][2025][rtiambassador.connect][tls-peer-validation]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost-only.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost-only.key";
  umbra::test::tls::writeTestCertificate(
      certificatePath, privateKeyPath, "DNS:localhost");
  ScopedEnvironmentVariable trustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", certificatePath.string());
  ScopedEnvironmentVariable certificateChain(
      "UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE", certificatePath.string());
  ScopedEnvironmentVariable privateKey(
      "UMBRA_RTI_TLS_PRIVATE_KEY_FILE", privateKeyPath.string());

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.serviceReportDirectory =
      temporaryDirectory.path() / "service-reports";
  std::filesystem::create_directories(serviceOptions.serviceReportDirectory);
  auto hostOptions = processFederationServiceHostOptionsFromRtiInitializationData();
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      ProcessTransportAddress{
          "127.0.0.1",
          0U,
          ProcessTransportAddress::Security::tls13,
          {}},
      {"tls-hostname-mismatch-test-authority", 0U},
      std::move(serviceOptions),
      std::move(hostOptions));
  host.start();
  auto const address = host.address();
  REQUIRE(address.port != 0U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto const mismatchedConfiguration = RtiConfiguration::createConfiguration()
                                           .withConfigurationName(
                                               L"tls-hostname-mismatch-client")
                                           .withRtiAddress(
                                               L"tls://127.0.0.1:" +
                                               std::to_wstring(address.port));
  REQUIRE_THROWS_AS(
      rti->connect(federate, HLA_EVOKED, mismatchedConfiguration),
      ConnectionFailed);
  CHECK(host.running());

  auto const matchingConfiguration = RtiConfiguration::createConfiguration()
                                         .withConfigurationName(
                                             L"tls-hostname-match-client")
                                         .withRtiAddress(
                                             L"tls://localhost:" +
                                             std::to_wstring(address.port));
  auto const result = rti->connect(federate, HLA_EVOKED, matchingConfiguration);
  CHECK(result.configurationUsed);
  CHECK(result.addressUsed);
  rti->disconnect();
  host.stop();
  host.rethrowIfFailed();
}

TEST_CASE(
    "RTIambassador Connect supports TLS encryption-only mode without certificate validation",
    "[integration][connection][transport][process-boundary][tls][2025][rtiambassador.connect][tls-encryption-only]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost-only.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost-only.key";
  auto const unavailableTrustStore =
      temporaryDirectory.path() / "must-not-be-opened.pem";
  umbra::test::tls::writeTestCertificate(
      certificatePath, privateKeyPath, "DNS:localhost");
  ScopedEnvironmentVariable trustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", unavailableTrustStore.string());
  ScopedEnvironmentVariable certificateChain(
      "UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE", certificatePath.string());
  ScopedEnvironmentVariable privateKey(
      "UMBRA_RTI_TLS_PRIVATE_KEY_FILE", privateKeyPath.string());

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.serviceReportDirectory =
      temporaryDirectory.path() / "service-reports";
  std::filesystem::create_directories(serviceOptions.serviceReportDirectory);
  auto hostOptions = processFederationServiceHostOptionsFromRtiInitializationData();
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      ProcessTransportAddress{
          "127.0.0.1",
          0U,
          ProcessTransportAddress::Security::tls13,
          {}},
      {"tls-encryption-only-test-authority", 0U},
      std::move(serviceOptions),
      std::move(hostOptions));
  host.start();
  auto const address = host.address();
  REQUIRE(address.port != 0U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto const invalidModeConfiguration = RtiConfiguration::createConfiguration()
                                           .withConfigurationName(
                                               L"tls-invalid-mode-client")
                                           .withRtiAddress(
                                               L"tls://127.0.0.1:" +
                                               std::to_wstring(address.port))
                                           .withAdditionalSettings(
                                               L"tlsCertificateValidation=disabled");
  REQUIRE_THROWS_AS(
      rti->connect(federate, HLA_EVOKED, invalidModeConfiguration),
      ConnectionFailed);
  CHECK(host.running());

  auto const configuration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"tls-encryption-only-client")
                                 .withRtiAddress(
                                     L"tls://127.0.0.1:" +
                                     std::to_wstring(address.port))
                                 .withAdditionalSettings(
                                     L"tlsCertificateValidation=none");
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  CHECK(result.configurationUsed);
  CHECK(result.addressUsed);
  CHECK(result.additionalSettingsResult == SETTINGS_APPLIED);
  CHECK(host.running());
  rti->disconnect();
  host.stop();
  host.rethrowIfFailed();
}

TEST_CASE(
    "RTIambassador Connect uses the normative default TLS 1.3 port",
    "[integration][connection][transport][process-boundary][tls][2025][rtiambassador.connect][tls-default-port]") {
  using namespace rti1516_2025;
  using namespace umbra::detail;
  using namespace umbra::detail::platform;

  initializeSocketRuntime();
  umbra::test::tls::TemporaryDirectory temporaryDirectory;
  auto const certificatePath = temporaryDirectory.path() / "localhost.crt";
  auto const privateKeyPath = temporaryDirectory.path() / "localhost.key";
  umbra::test::tls::writeTestCertificate(certificatePath, privateKeyPath);
  ScopedEnvironmentVariable trustStore(
      "UMBRA_RTI_TLS_TRUST_STORE_FILE", certificatePath.string());
  ScopedEnvironmentVariable certificateChain(
      "UMBRA_RTI_TLS_CERTIFICATE_CHAIN_FILE", certificatePath.string());
  ScopedEnvironmentVariable privateKey(
      "UMBRA_RTI_TLS_PRIVATE_KEY_FILE", privateKeyPath.string());

  EmbeddedFederationRegistry registry;
  ProcessFederationServiceOptions serviceOptions;
  serviceOptions.serviceReportDirectory =
      temporaryDirectory.path() / "service-reports";
  std::filesystem::create_directories(serviceOptions.serviceReportDirectory);
  auto hostOptions = processFederationServiceHostOptionsFromRtiInitializationData();
  ProcessFederationServiceHost host(
      registry,
      FederationDefinition{},
      ProcessTransportAddress{
          "127.0.0.1",
          15165U,
          ProcessTransportAddress::Security::tls13,
          {}},
      {"tls-default-port-test-authority", 0U},
      std::move(serviceOptions),
      std::move(hostOptions));
  host.start();
  REQUIRE(host.address().port == 15165U);

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto const configuration = RtiConfiguration::createConfiguration()
                                 .withConfigurationName(
                                     L"tls-default-port-client")
                                 .withRtiAddress(L"tls://127.0.0.1")
                                 .withAdditionalSettings(
                                     L"tlsCertificateValidation=server");
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  CHECK(result.configurationUsed);
  CHECK(result.addressUsed);
  CHECK(result.additionalSettingsResult == SETTINGS_APPLIED);
  CHECK(host.address().port == 15165U);
  rti->disconnect();
  host.stop();
  host.rethrowIfFailed();
}
#endif
