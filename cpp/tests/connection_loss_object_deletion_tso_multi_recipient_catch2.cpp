#include <catch2/catch_test_macros.hpp>

#include "internal/federation/embedded_transport.hpp"
#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The multi-recipient object-deletion tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::VariableLengthData;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct RemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    grants.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    constrainedTimes.push_back({time.implementationName(), time.toString()});
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveryCount;
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    removals.push_back({
        objectInstance,
        tag,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeReport> grants;
  std::vector<TimeReport> constrainedTimes;
  std::vector<RemovalReport> removals;
  std::vector<std::string> callbackOrder;
  std::size_t discoveryCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void requireCutoffRemoval(
    ReportingFederateAmbassador const& reports,
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& tag,
    FederateHandle const& lostFederate) {
  REQUIRE(reports.grants.size() == 1U);
  REQUIRE(reports.removals.size() == 1U);
  REQUIRE(reports.callbackOrder == std::vector<std::string>{"remove", "grant"});
  auto const& removal = reports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag) == variableLengthDataBytes(tag));
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE(reports.grants.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(reports.grants.front().value == L"6");
}

}  // namespace

TEST_CASE(
    "Embedded transport loss drains a cutoff timestamped deletion to each pending survivor",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-object-deletion]"
    "[connection-lost-tso-cutoff][automatic-resign-delete][multi-recipient]"
    "[rti.service.connection-lost][rti.service.delete-object-instance]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  auto lost = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto const federationName =
      std::wstring{L"connection-loss-object-deletion-tso-multi-recipient"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"multi-cutoff-object-deletion-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"multi-cutoff-object-deletion-first-survivor", L"subscriber", federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"multi-cutoff-object-deletion-second-survivor", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(first->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(second->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.discoveryCount == 1U);
  REQUIRE(secondReports.discoveryCount == 1U);
  REQUIRE_NOTHROW(first->enableTimeConstrained());
  REQUIRE_NOTHROW(second->enableTimeConstrained());
  drain(*first);
  drain(*second);
  REQUIRE(firstReports.constrainedTimes.size() == 1U);
  REQUIRE(secondReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);

  auto const retraction = lost->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.removals.empty());
  REQUIRE(secondReports.removals.empty());

  // Only the first survivor has a pending TAR when the source fails.  The
  // second TAR is submitted after loss to prove independent queued copies.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(firstReports.grants.empty());
  REQUIRE(secondReports.grants.empty());

  std::wstring const faultDescription =
      L"timestamped object deletion multi-recipient cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(firstReports.removals.empty());
  REQUIRE(secondReports.removals.empty());

  drain(*first);
  requireCutoffRemoval(firstReports, objectInstance, tag, lostFederate);
  REQUIRE(secondReports.removals.empty());
  REQUIRE(secondReports.grants.empty());

  REQUIRE_NOTHROW(second->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*second);
  requireCutoffRemoval(secondReports, objectInstance, tag, lostFederate);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_THROWS_AS(
      first->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      second->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(first->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(second->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
}
