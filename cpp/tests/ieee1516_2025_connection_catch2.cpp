#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstring>
#include <memory>

#include <RTI/RTI1516.h>
#include <RTI/NullFederateAmbassador.h>

namespace {

using rti1516_2025::CallbackModel;
using rti1516_2025::ConfigurationResult;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::HLAnoCredentials;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RtiConfiguration;
using rti1516_2025::SETTINGS_IGNORED;
using rti1516_2025::VariableLengthData;

using TestFederateAmbassador = NullFederateAmbassador;

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void requireIgnoredConfiguration(ConfigurationResult const& result) {
  REQUIRE_FALSE(result.configurationUsed);
  REQUIRE_FALSE(result.addressUsed);
  REQUIRE(result.additionalSettingsResult == SETTINGS_IGNORED);
  REQUIRE(result.message.empty());
}

}  // namespace

TEST_CASE("IEEE 1516.1-2025 connection support types have usable value semantics", "[baseline][support-types]") {
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

TEST_CASE("RTIambassador Connect exposes all four official C++ overloads", "[integration][connection]") {
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

TEST_CASE("RTIambassador Connect rejects unsupported callback models without connecting", "[integration][connection]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_THROWS_AS(
      rti->connect(federate, static_cast<CallbackModel>(-1)),
      rti1516_2025::UnsupportedCallbackModel);
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE("RTIambassador Disconnect rejects an absent connection", "[integration][connection]") {
  auto rti = makeRti();
  REQUIRE_THROWS_AS(rti->disconnect(), rti1516_2025::NotConnected);
}

TEST_CASE("RTIambassador callback controls honor both models with an empty embedded queue", "[integration][callbacks]") {
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
    "[integration][compliance][rti.service.disconnect]") {
  TestFederateAmbassador federate;
  auto rti = makeRti();

  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
  REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->disconnect());
}
