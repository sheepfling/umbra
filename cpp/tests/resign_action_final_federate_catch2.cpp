#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The final-federate resignation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"resign-action-final-federate-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  void objectInstanceNameReservationSucceeded(
      std::wstring const& objectInstanceName) override {
    reservationSuccesses.push_back(objectInstanceName);
  }

  std::vector<std::wstring> reservationSuccesses;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 128; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Standalone final-federate resignation applies directive two regardless of the supplied action",
    "[integration][development-profile][federation-management][object-management]"
    "[resign-action-final-federate][standalone][2025]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.reserve-object-instance-name]"
    "[rti.service.register-object-instance]"
    "[federate.callback.object-instance-name-reservation-succeeded]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(rti->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"last-federate", L"publisher", federationName));

  auto const server = rti->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency =
      rti->getAttributeHandle(server, fixture_hla::fixture::efficiency);
  AttributeHandleSet const efficiencyOnly{efficiency};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle originalObject;
  REQUIRE_NOTHROW(originalObject = rti->registerObjectInstance(server));
  auto const reusableObjectName = rti->getObjectInstanceName(originalObject);
  REQUIRE(originalObject.isValid());
  REQUIRE_FALSE(reusableObjectName.empty());

  // IEEE 1516.1-2025 §4.12.4 forces directive two for the final joined
  // federate, even when the caller supplies NO_ACTION.  The deleted name
  // must be available to a fresh joined lifetime in the same execution.
  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"rejoined-federate", L"publisher", federationName));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(reusableObjectName));
  REQUIRE(reports.reservationSuccesses.empty());
  drainCallbacks(*rti);
  REQUIRE(reports.reservationSuccesses.size() == 1U);
  REQUIRE(reports.reservationSuccesses.front() == reusableObjectName);

  ObjectInstanceHandle replacementObject;
  REQUIRE_NOTHROW(
      replacementObject = rti->registerObjectInstance(server, reusableObjectName));
  REQUIRE(replacementObject.isValid());
  REQUIRE(replacementObject != originalObject);

  REQUIRE_NOTHROW(rti->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}
