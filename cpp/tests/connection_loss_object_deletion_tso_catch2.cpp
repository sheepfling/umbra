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
#error "The object-deletion connection-loss tests require the Umbra source directory."
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

}  // namespace

TEST_CASE(
    "Embedded transport loss delivers timestamped object deletion through the lost federate's last-known time",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-object-deletion]"
    "[connection-lost-tso-cutoff][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.delete-object-instance]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-object-deletion-tso"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"cutoff-object-deletion-lost-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"cutoff-object-deletion-surviving-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.discoveryCount == 1U);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }

  auto const retraction = lost->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"timestamped object deletion cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.removals.empty());

  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"remove", "grant"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag) == variableLengthDataBytes(tag));
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE(survivingReports.grants.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.grants.front().value == L"6");
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss preserves a cutoff timestamped deletion across automatic delete cleanup",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-object-deletion]"
    "[connection-lost-tso-cutoff][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.delete-object-instance]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-object-deletion-tso-automatic"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x41, 0x55, 0x54, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"automatic-cutoff-object-deletion-lost-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"automatic-cutoff-object-deletion-surviving-subscriber",
      L"subscriber",
      federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.discoveryCount == 1U);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }

  auto const retraction = lost->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"timestamped object deletion automatic-delete cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.removals.empty());

  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"remove", "grant"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag) == variableLengthDataBytes(tag));
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  // Automatic DELETE_OBJECTS cleanup must not replace or duplicate the
  // already accepted timestamped removal on a later receive-order gate.
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.removals.size() == 1U);

  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}

TEST_CASE(
    "Embedded transport loss delivers timestamped object deletion before the lost federate's last-known time",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-object-deletion]"
    "[connection-lost-tso-cutoff][automatic-resign-delete]"
    "[rti.service.connection-lost][rti.service.delete-object-instance]"
    "[rti.service.time-advance-request][rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.remove-object-instance]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-object-deletion-tso-less-than"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x44, 0x45, 0x4C, 0x2D, 0x4C, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"less-than-cutoff-object-deletion-lost-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"less-than-cutoff-object-deletion-surviving-subscriber",
      L"subscriber",
      federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(
      rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.discoveryCount == 1U);
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }

  auto const retraction = lost->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.removals.empty());
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");
  REQUIRE(survivingReports.grants.empty());

  std::wstring const faultDescription =
      L"timestamped object deletion strict-less-than cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.removals.empty());

  while (surviving->evokeCallback(0.0)) {
  }
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"remove", "grant"});
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag) == variableLengthDataBytes(tag));
  REQUIRE(removal.producer == lostFederate);
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"5");
  REQUIRE(removal.sentOrderType == TIMESTAMP);
  REQUIRE(removal.receivedOrderType == TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE(survivingReports.grants.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.grants.front().value == L"6");
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}
