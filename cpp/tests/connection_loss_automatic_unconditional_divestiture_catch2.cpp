#include <catch2/catch_test_macros.hpp>

#include "internal/federation/embedded_transport.hpp"
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
#error "The connection-loss automatic-divestiture test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"connection-loss-automatic-unconditional-divestiture-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct OwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass});
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    ownershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  struct Discovery final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  std::vector<std::wstring> faultDescriptions;
  std::vector<Discovery> discoveries;
  std::vector<OwnershipAssumptionReport> ownershipAssumptionReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Standalone transport loss applies the configured automatic unconditional-divest directive",
    "[integration][development-profile][federation-management][transport]"
    "[ownership-management][connection-lost-automatic-unconditional-divestiture]"
    "[standalone][2025]"
    "[rti.service.connection-lost]"
    "[rti.service.get-automatic-resign-directive]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost]"
    "[federate.callback.request-attribute-ownership-assumption]") {
  ReportingFederateAmbassador lostCallbacks;
  ReportingFederateAmbassador survivorCallbacks;
  auto lost = makeRti();
  auto survivor = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(lost->connect(lostCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(survivor->connect(survivorCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));

  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-divest-lost",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(survivor->joinFederationExecution(
      L"automatic-divest-survivor",
      L"subscriber",
      federationName));

  auto const server = lost->getObjectClassHandle(fixture_hla::fom::employee_server);
  auto const efficiency = lost->getAttributeHandle(
      server, fixture_hla::fixture::efficiency);
  auto const privilegeToDelete = lost->getAttributeHandle(
      server, standard_hla::mom::privilege_to_delete_object);
  AttributeHandleSet const efficiencyOnly{efficiency};
  AttributeHandleSet const expectedAssumption{efficiency, privilegeToDelete};
  REQUIRE(server.isValid());
  REQUIRE(efficiency.isValid());
  REQUIRE(privilegeToDelete.isValid());
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(server, efficiencyOnly));
  REQUIRE_NOTHROW(survivor->publishObjectClassAttributes(server, efficiencyOnly));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(server));
  REQUIRE(objectInstance.isValid());
  auto const objectInstanceName = lost->getObjectInstanceName(objectInstance);
  REQUIRE_FALSE(survivor->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivorCallbacks.discoveries.size() == 1U);
  REQUIRE(survivorCallbacks.discoveries.front().objectInstance == objectInstance);
  REQUIRE(survivorCallbacks.discoveries.front().objectClass == server);

  // The official support service selects the member's automatic-resign
  // disposition.  Directive 1 retains the object while releasing owned
  // attributes for future ownership acquisition.
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));
  REQUIRE(
      lost->getAutomaticResignDirective() ==
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES);

  std::wstring const faultDescription =
      L"automatic unconditional-divest transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(lostCallbacks.faultDescriptions.empty());
  REQUIRE(survivorCallbacks.ownershipAssumptionReports.empty());

  REQUIRE_FALSE(lost->evokeCallback(0.0));
  REQUIRE(lostCallbacks.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_FALSE(survivor->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(survivorCallbacks.ownershipAssumptionReports.size() == 1U);
  auto const& assumption = survivorCallbacks.ownershipAssumptionReports.front();
  REQUIRE(assumption.objectInstance == objectInstance);
  REQUIRE(assumption.attributes == expectedAssumption);
  REQUIRE(assumption.userSuppliedTag.size() == 0U);
  REQUIRE(survivor->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_FALSE(survivor->isAttributeOwnedByFederate(objectInstance, efficiency));
  REQUIRE_FALSE(
      survivor->isAttributeOwnedByFederate(objectInstance, privilegeToDelete));
  REQUIRE(survivor->getFederateName(lostFederate) == L"automatic-divest-lost");

  REQUIRE_NOTHROW(survivor->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(survivor->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(lost->connect(lostCallbacks, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(survivor->disconnect());
}
