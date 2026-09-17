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
#error "The immediate callback connection-loss tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::HLA_IMMEDIATE;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TIMESTAMP;
using rti1516_2025::TransportationTypeHandle;
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
  struct Reflection final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap values;
    VariableLengthData tag;
    TransportationTypeHandle transportationType;
    FederateHandle producer;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct Removal final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData tag;
    FederateHandle producer;
  };

  void connectionLost(std::wstring const& description) override {
    faultDescriptions.push_back(description);
  }

  void timeAdvanceGrant(rti1516_2025::LogicalTime const& time) override {
    grants.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeConstrainedEnabled(rti1516_2025::LogicalTime const& time) override {
    constrainedTimes.push_back({time.implementationName(), time.toString()});
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const&,
      ObjectClassHandle const&,
      std::wstring const&,
      FederateHandle const&) override {
    ++discoveryCount;
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& values,
      VariableLengthData const& tag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producer,
      rti1516_2025::RegionHandleSet const*,
      rti1516_2025::LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    reflections.push_back({
        objectInstance,
        values,
        tag,
        transportationType,
        producer,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& tag,
      FederateHandle const& producer) override {
    removals.push_back({objectInstance, tag, producer});
    callbackOrder.push_back("remove");
  }

  struct TimeReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  std::vector<std::wstring> faultDescriptions;
  std::vector<TimeReport> grants;
  std::vector<TimeReport> constrainedTimes;
  std::vector<Reflection> reflections;
  std::vector<Removal> removals;
  std::vector<std::string> callbackOrder;
  std::size_t discoveryCount = 0U;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded immediate transport loss releases automatic cleanup after cutoff reflection under asynchronous delivery",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-attribute-update]"
    "[asynchronous-delivery][connection-lost-tso-cutoff][callback-immediate]"
    "[rti.service.connection-lost][rti.service.update-attribute-values]"
    "[rti.service.enable-asynchronous-delivery][rti.service.time-advance-request]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.reflect-attribute-values]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-attribute-tso-immediate"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x49, 0x4D, 0x4D, 0x45, 0x44};
  unsigned char const tagBytes[] = {0x49, 0x4D, 0x4D, 0x45, 0x44, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_IMMEDIATE));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"immediate-cutoff-lost-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"immediate-cutoff-surviving-attribute-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  auto const reliableTransport = lost->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(reliableTransport.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap const values{{attribute, value}};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  REQUIRE(survivingReports.discoveryCount == 1U);
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(surviving->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.grants.empty());

  auto const retraction = lost->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  while (lost->evokeCallback(0.0)) {
  }
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");

  std::wstring const faultDescription =
      L"immediate asynchronous timestamped attribute automatic-delete cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE(survivingReports.removals.empty());

  // HLA_IMMEDIATE dispatches the eligible TSO callbacks on the TAR call.  The
  // receive-order automatic removal follows the reflection and its grant in
  // that same advancement cycle once asynchronous delivery is enabled.
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(survivingReports.reflections.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"reflect", "grant", "remove"});
  auto const& reflection = survivingReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.values.size() == 1U);
  REQUIRE(reflection.values.contains(attribute));
  REQUIRE(variableLengthDataBytes(reflection.values.at(attribute)) ==
          variableLengthDataBytes(value));
  REQUIRE(variableLengthDataBytes(reflection.tag) == variableLengthDataBytes(tag));
  REQUIRE(reflection.transportationType == reliableTransport);
  REQUIRE(reflection.producer == lostFederate);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"6");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(survivingReports.grants.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.grants.front().value == L"6");
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag).empty());
  REQUIRE(removal.producer == lostFederate);
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
    "Embedded transport loss releases automatic cleanup after cutoff reflection under asynchronous delivery",
    "[integration][development-profile][federation-management][transport]"
    "[object-management][time-management][timestamped-attribute-update]"
    "[asynchronous-delivery][connection-lost-tso-cutoff]"
    "[rti.service.connection-lost][rti.service.update-attribute-values]"
    "[rti.service.enable-asynchronous-delivery][rti.service.time-advance-request]"
    "[rti.service.set-automatic-resign-directive]"
    "[federate.callback.connection-lost][federate.callback.reflect-attribute-values]"
    "[federate.callback.remove-object-instance][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador lostReports;
  ReportingFederateAmbassador survivingReports;
  auto lost = makeRti();
  auto surviving = makeRti();
  auto const federationName = std::wstring{L"connection-loss-attribute-tso-asynchronous"};
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "third_party" / "ieee1516.2-2025" / "resources" /
                          "examples" / "RestaurantFOMmodule-2025.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x41, 0x53, 0x59, 0x4E, 0x43};
  unsigned char const tagBytes[] = {0x41, 0x53, 0x59, 0x4E, 0x43, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const value(valueBytes, sizeof(valueBytes));
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle lostFederate;
  REQUIRE_NOTHROW(lostFederate = lost->joinFederationExecution(
      L"asynchronous-cutoff-lost-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"asynchronous-cutoff-surviving-attribute-subscriber", L"subscriber", federationName));

  auto const objectClass = lost->getObjectClassHandle(L"HLAobjectRoot.Employee.Server");
  auto const attribute = lost->getAttributeHandle(objectClass, L"Efficiency");
  auto const reliableTransport = lost->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(reliableTransport.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap const values{{attribute, value}};
  REQUIRE_NOTHROW(lost->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(lost->changeDefaultAttributeOrderType(
      objectClass,
      attributes,
      TIMESTAMP));
  REQUIRE_NOTHROW(lost->setAutomaticResignDirective(rti1516_2025::DELETE_OBJECTS));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = lost->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  auto const objectName = lost->getObjectInstanceName(objectInstance);
  drain(*surviving);
  REQUIRE(survivingReports.discoveryCount == 1U);
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drain(*surviving);
  REQUIRE(survivingReports.constrainedTimes.size() == 1U);
  REQUIRE_NOTHROW(surviving->enableAsynchronousDelivery());
  REQUIRE_NOTHROW(lost->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drain(*lost);
  REQUIRE(lostReports.grants.empty());

  auto const retraction = lost->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE_NOTHROW(lost->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  drain(*lost);
  REQUIRE(lostReports.grants.size() == 1U);
  REQUIRE(lostReports.grants.front().value == L"6");

  std::wstring const faultDescription =
      L"asynchronous timestamped attribute automatic-delete cutoff transport fault";
  REQUIRE(umbra::detail::failEmbeddedTransportConnectionForTesting(
      *lost,
      faultDescription));
  REQUIRE(survivingReports.reflections.empty());
  REQUIRE(survivingReports.removals.empty());

  // With asynchronous delivery enabled, the cutoff reflection and its grant
  // are dispatched before the independent receive-order cleanup in the same
  // advancement cycle.
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  drain(*surviving);
  REQUIRE(survivingReports.reflections.size() == 1U);
  REQUIRE(survivingReports.grants.size() == 1U);
  REQUIRE(survivingReports.removals.size() == 1U);
  REQUIRE(survivingReports.callbackOrder ==
          std::vector<std::string>{"reflect", "grant", "remove"});
  auto const& reflection = survivingReports.reflections.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.values.size() == 1U);
  REQUIRE(reflection.values.contains(attribute));
  REQUIRE(variableLengthDataBytes(reflection.values.at(attribute)) ==
          variableLengthDataBytes(value));
  REQUIRE(variableLengthDataBytes(reflection.tag) == variableLengthDataBytes(tag));
  REQUIRE(reflection.transportationType == reliableTransport);
  REQUIRE(reflection.producer == lostFederate);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"6");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE(survivingReports.grants.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(survivingReports.grants.front().value == L"6");
  auto const& removal = survivingReports.removals.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(variableLengthDataBytes(removal.tag).empty());
  REQUIRE(removal.producer == lostFederate);
  REQUIRE_THROWS_AS(
      surviving->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  drain(*lost);
  REQUIRE(lostReports.faultDescriptions ==
          std::vector<std::wstring>{faultDescription});
  REQUIRE_THROWS_AS(lost->disconnect(), rti1516_2025::NotConnected);
  REQUIRE_NOTHROW(lost->connect(lostReports, HLA_EVOKED));
  REQUIRE_NOTHROW(lost->disconnect());
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(surviving->disconnect());
}
