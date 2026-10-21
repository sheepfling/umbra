#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <umbra/embedded_profile_configuration.hpp>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The service-reporting switch default test requires the Umbra source directory."
#endif

namespace {

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0U};
  return L"umbra-service-reporting-fdd-default-" +
      std::to_wstring(++counter);
}

std::filesystem::path restaurantFomPath() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / "examples" /
      "RestaurantFOMmodule-2025.xml";
}

std::filesystem::path switchSupportFomPath() {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / "switch-support-enabled-fom.xml";
}

void requireInitialServiceReportingSwitch(
    std::vector<std::wstring> const& fomModules,
    bool const expectedValue) {
  using namespace rti1516_2025;

  NullFederateAmbassador federate;
  RTIambassadorFactory factory;
  auto rti = factory.createRTIambassador();
  auto const federationName = nextFederationName();
  auto configuration = RtiConfiguration::createConfiguration()
                           .withRtiAddress(L"in-process");
  bool connected = false;
  bool federationCreated = false;
  bool joined = false;

  try {
    REQUIRE_NOTHROW(rti->connect(federate, HLA_EVOKED, configuration));
    connected = true;
    REQUIRE_NOTHROW(rti->createFederationExecution(
        federationName, fomModules, L"HLAinteger64Time"));
    federationCreated = true;
    auto const federateHandle = rti->joinFederationExecution(
        L"service-reporting-initial-setting-federate", federationName);
    joined = true;
    REQUIRE(federateHandle.isValid());
    REQUIRE(rti->getServiceReportingSwitch() == expectedValue);

    rti->resignFederationExecution(NO_ACTION);
    joined = false;
    rti->destroyFederationExecution(federationName);
    federationCreated = false;
    rti->disconnect();
    connected = false;
  } catch (...) {
    if (joined) {
      try {
        rti->resignFederationExecution(NO_ACTION);
      } catch (...) {
      }
    }
    if (federationCreated) {
      try {
        rti->destroyFederationExecution(federationName);
      } catch (...) {
      }
    }
    if (connected) {
      try {
        rti->disconnect();
      } catch (...) {
      }
    }
    throw;
  }
}

}  // namespace

TEST_CASE(
    "RTIambassador uses the standard default service-reporting switch when the FDD omits it",
    "[integration][federation-management][mom][service-reporting-switch-fdd-default][2025][rti.service.get-service-reporting-switch]") {
  auto const fomModules = std::vector<std::wstring>{restaurantFomPath().wstring()};
  // The Restaurant FOM sets other switches but omits serviceReporting.
  // The 1516.2-2025 MIM default for HLAserviceReporting is HLAfalse.
  requireInitialServiceReportingSwitch(fomModules, false);
}

TEST_CASE(
    "RTIambassador honors the explicit service-reporting switch value from the FDD",
    "[integration][federation-management][mom][service-reporting-switch-fdd-default][2025][rti.service.get-service-reporting-switch]") {
  auto const fomModules = std::vector<std::wstring>{
      restaurantFomPath().wstring(), switchSupportFomPath().wstring()};
  // The support-switch FOM explicitly sets serviceReporting to HLAtrue.
  requireInitialServiceReportingSwitch(fomModules, true);
}
