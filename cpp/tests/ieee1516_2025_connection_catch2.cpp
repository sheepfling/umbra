#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>

#include <umbra/embedded_profile_configuration.hpp>

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::ConfigurationResult;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::HLAplainTextPassword;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RTIinternalError;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::SETTINGS_APPLIED;
using rti1516_2025::SETTINGS_IGNORED;
using rti1516_2025::Unauthorized;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path temporaryServiceReportDirectory() {
  static std::atomic_uint64_t next{0};
  return std::filesystem::temp_directory_path() /
      ("umbra-service-report-configuration-" + std::to_string(++next));
}

void requireIgnoredConfiguration(ConfigurationResult const& result) {
  REQUIRE_FALSE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_IGNORED);
  REQUIRE(result.message.empty());
}

}  // namespace

TEST_CASE("IEEE 1516.1-2025 connection support types have usable value semantics", "[baseline][support-types][unit][foundation]") {
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"callbacks=evoked");
  REQUIRE(configuration.configurationName() == L"embedded");
  REQUIRE(configuration.rtiAddress() == L"in-process");
  REQUIRE(configuration.additionalSettings() == L"callbacks=evoked");

  ConfigurationResult defaultResult;
  requireIgnoredConfiguration(defaultResult);

  std::array<unsigned char, 3> source{0x01, 0x02, 0x03};
  VariableLengthData value(source.data(), source.size());
  source[0] = 0xFF;
  REQUIRE(value.size() == 3);
  REQUIRE(std::memcmp(value.data(), "\x01\x02\x03", value.size()) == 0);

  VariableLengthData copied(value);
  REQUIRE(copied.size() == value.size());
  REQUIRE(std::memcmp(copied.data(), value.data(), value.size()) == 0);
}

TEST_CASE("RTIambassador Connect exposes all four official C++ overloads", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  HLAnoCredentials credentials;
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process")
                                     .withAdditionalSettings(L"ignored-by-initial-slice");

  SECTION("base overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_IMMEDIATE), rti1516_2025::AlreadyConnected);
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    requireIgnoredConfiguration(rti->connect(federate, HLA_EVOKED, configuration, credentials));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect rejects supplied credentials while authorization is disabled",
    "[integration][connection][authorization][credentials][federation-management]") {
  TestFederateAmbassador federate;
  HLAplainTextPassword password(L"test-password");
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withConfigurationName(L"embedded")
                                     .withRtiAddress(L"in-process");

  SECTION("credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, password), Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("configuration and credentials overload") {
    auto rti = makeRti();
    REQUIRE_THROWS_AS(
        rti->connect(federate, HLA_EVOKED, configuration, password),
        Unauthorized);
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "Embedded Connect accepts only its filesystem service-report directory setting",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const directory = temporaryServiceReportDirectory();
  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + directory.wstring());
  auto rti = makeRti();

  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Umbra embedded profile configuration exposes a typed service-report directory",
    "[integration][connection][mom][service-report-store][service-reporting][configuration]") {
  auto const directory = temporaryServiceReportDirectory();
  auto configuration = umbra::embedded::makeEmbeddedRtiConfiguration(
      umbra::embedded::ServiceReportConfiguration{directory});
  configuration.withConfigurationName(L"typed-service-report")
      .withRtiAddress(L"in-process");

  REQUIRE(configuration.additionalSettings() ==
          L"serviceReportDirectory=" + directory.wstring());
  REQUIRE(configuration.configurationName() == L"typed-service-report");
  REQUIRE(configuration.rtiAddress() == L"in-process");

  TestFederateAmbassador federate;
  auto rti = makeRti();
  auto const result = rti->connect(federate, HLA_EVOKED, configuration);
  REQUIRE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_APPLIED);
  REQUIRE(std::filesystem::is_directory(directory));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
}

TEST_CASE(
    "Embedded Connect fails deterministically for an unusable service-report directory",
    "[integration][connection][mom][service-reporting]") {
  TestFederateAmbassador federate;
  auto const parent = temporaryServiceReportDirectory();
  std::filesystem::create_directories(parent);
  auto const regularFile = parent / "not-a-directory";
  std::ofstream output(regularFile, std::ios::binary | std::ios::trunc);
  REQUIRE(output.good());
  output.close();

  RtiConfiguration configuration = RtiConfiguration::createConfiguration()
                                     .withAdditionalSettings(
                                         L"serviceReportDirectory=" + regularFile.wstring());
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->connect(federate, HLA_EVOKED, configuration), RTIinternalError);
  // A rejected configuration is not a partial connection and never falls
  // back to the in-memory test store.
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());

  std::error_code ignored;
  std::filesystem::remove_all(parent, ignored);
}

TEST_CASE("RTIambassador Connect rejects unsupported callback models without connecting", "[integration][connection][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_THROWS_AS(
      rti->connect(federate, static_cast<CallbackModel>(-1)),
      rti1516_2025::UnsupportedCallbackModel);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE("RTIambassador Disconnect rejects an absent connection", "[integration][connection][federation-management]") {
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);
}

TEST_CASE("RTIambassador callback controls honor both models with an empty embedded queue", "[integration][callbacks][federation-management]") {
  TestFederateAmbassador federate;

  SECTION("immediate callbacks make Evoke services a no-op") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_IMMEDIATE));
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_NOTHROW(rti->disconnect());
  }

  SECTION("evoked callbacks report no pending work after controls are toggled") {
    auto rti = makeRti();
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
    REQUIRE_NOTHROW(rti->disableCallbacks());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE_NOTHROW(rti->enableCallbacks());
    REQUIRE_FALSE(rti->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE_NOTHROW(rti->disconnect());
  }
}

TEST_CASE(
    "RTIambassador disconnect terminates an unjoined connection",
    "[integration][compliance][rti.service.disconnect][federation-management]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}
