#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The local-delete timestamped attribute test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
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

std::wstring nextLocalDeleteSuppressionFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"local-delete-timestamped-suppression-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path localDeleteSuppressionResourcePath(
    std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class LocalDeleteSuppressionReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct TimeRoleReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    bool sentRegionsSupplied = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
  };

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({time.implementationName(), time.toString()});
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalSentRegions != nullptr,
    });
    callbackOrder.push_back("reflect");
  }

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid()});
    callbackOrder.push_back("retraction");
  }

  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TimeRoleReport> timeRegulationEnabledReports;
  std::vector<TimeRoleReport> timeConstrainedEnabledReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeLocalDeleteSuppressionRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

TEST_CASE(
    "Embedded local deletion suppresses a queued timestamped attribute reflection",
    "[integration][development-profile][object-management][time-management]"
    "[timestamped-attribute-update][local-delete-object-instance][local-delete-timestamped-attribute-suppression][tso]"
    "[rti.service.update-attribute-values][rti.service.local-delete-object-instance]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  LocalDeleteSuppressionReportingFederateAmbassador publisherReports;
  LocalDeleteSuppressionReportingFederateAmbassador receiverReports;
  auto publisher = makeLocalDeleteSuppressionRti();
  auto receiver = makeLocalDeleteSuppressionRti();
  auto const federationName = nextLocalDeleteSuppressionFederationName();
  auto const fomModule = localDeleteSuppressionResourcePath("attribute-update-passel-fom.xml")
                             .wstring();
  std::vector<unsigned char> const valueBytes{0x5AU, 0x16U};
  std::vector<unsigned char> const tagBytes{0xD2U, 0x25U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"local-delete-timestamped-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"local-delete-timestamped-receiver",
      L"subscriber",
      federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(
      child,
      L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  AttributeHandleSet const attributes{reliable};
  AttributeHandleValueMap values;
  values.emplace(
      reliable,
      VariableLengthData(valueBytes.data(), valueBytes.size()));

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  // The fixture's FOM default is Receive order. This scenario deliberately
  // opts the published attribute into the timestamped queue.
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child,
      attributes,
      TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const firstHandle = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());

  // Local deletion removes only the receiver's known-instance state. The
  // queued TSO message reaches its time boundary, but its callback-time
  // projection is suppressed rather than reflecting stale data.
  REQUIRE_NOTHROW(receiver->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      receiver->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"grant"});

  // A later eligible subscription creates a fresh discovery without changing
  // the federation-wide object. A new timestamped update then reflects once.
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(receiver->getKnownObjectClassHandle(objectInstance) == child);

  auto const secondHandle = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "reflect", "grant"});
  auto const& reflection = receiverReports.attributeReflectionReports.front();
  REQUIRE(reflection.objectInstance == objectInstance);
  REQUIRE(reflection.producingFederate == publisherHandle);
  REQUIRE(reflection.attributeValues.size() == 1U);
  REQUIRE(reflection.attributeValues.contains(reliable));
  REQUIRE(variableLengthDataBytes(reflection.attributeValues.at(reliable)) ==
          valueBytes);
  REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == tagBytes);
  REQUIRE(reflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(reflection.timeValue == L"7");
  REQUIRE(reflection.sentOrderType == TIMESTAMP);
  REQUIRE(reflection.receivedOrderType == TIMESTAMP);
  REQUIRE(reflection.retractionSupplied);
  REQUIRE(reflection.retractionValid);
  REQUIRE_FALSE(reflection.sentRegionsSupplied);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"local-delete-timestamped-attribute-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct TimeAdvanceGrantReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct TimeRoleReport final {
    std::wstring implementationName;
    std::wstring value;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
    bool sentRegionsSupplied = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
  };

  void requestRetraction(
      rti1516_2025::MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({retraction.isValid()});
    callbackOrder.push_back("retraction");
  };

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({
        time.implementationName(),
        time.toString(),
    });
    callbackOrder.push_back("grant");
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({
        time.implementationName(),
        time.toString(),
    });
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({
        time.implementationName(),
        time.toString(),
    });
  }

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      rti1516_2025::RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      rti1516_2025::MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
        optionalSentRegions != nullptr,
    });
    callbackOrder.push_back("reflect");
  }

  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<TimeRoleReport> timeRegulationEnabledReports;
  std::vector<TimeRoleReport> timeConstrainedEnabledReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<std::string> callbackOrder;
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
    "Embedded local deletion isolates queued timestamped attribute deliveries per recipient",
    "[integration][development-profile][federation-management][object-management]"
    "[time-management][timestamped-attribute-update][local-delete-object-instance]"
    "[multi-federate-callback-ordering][tso]"
    "[rti.service.update-attribute-values][rti.service.local-delete-object-instance]"
    "[rti.service.time-advance-request][federate.callback.reflect-attribute-values]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador localReports;
  ReportingFederateAmbassador survivingReports;
  auto publisher = makeRti();
  auto local = makeRti();
  auto surviving = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  std::vector<unsigned char> const valueBytes{0x5AU, 0x31U};
  std::vector<unsigned char> const tagBytes{0xD2U, 0x31U};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(local->connect(localReports, HLA_EVOKED));
  REQUIRE_NOTHROW(surviving->connect(survivingReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"local-delete-timestamped-attribute-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(local->joinFederationExecution(
      L"local-delete-timestamped-attribute-local",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(surviving->joinFederationExecution(
      L"local-delete-timestamped-attribute-surviving",
      L"subscriber",
      federationName));

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliable = publisher->getAttributeHandle(child, L"ReliableBaseA");
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  AttributeHandleSet const attributes{reliable};
  AttributeHandleValueMap values;
  values.emplace(
      reliable,
      VariableLengthData(valueBytes.data(), valueBytes.size()));
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(local->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(surviving->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(
      child,
      attributes,
      TIMESTAMP));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE(objectInstance.isValid());
  auto const objectName = publisher->getObjectInstanceName(objectInstance);
  drainCallbacks(*local);
  drainCallbacks(*surviving);
  REQUIRE(localReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(survivingReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(local->enableTimeConstrained());
  REQUIRE_NOTHROW(surviving->enableTimeConstrained());
  drainCallbacks(*local);
  drainCallbacks(*surviving);
  REQUIRE(localReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(survivingReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const firstRetraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstRetraction.isValid());
  REQUIRE_NOTHROW(local->localDeleteObjectInstance(objectInstance));
  REQUIRE_THROWS_AS(
      local->getKnownObjectClassHandle(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_NOTHROW(local->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*local);
  drainCallbacks(*surviving);

  REQUIRE(localReports.attributeReflectionReports.empty());
  REQUIRE(localReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(survivingReports.attributeReflectionReports.size() == 1U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 1U);
  auto const& firstReflection = survivingReports.attributeReflectionReports.front();
  REQUIRE(firstReflection.objectInstance == objectInstance);
  REQUIRE(firstReflection.producingFederate == publisherHandle);
  REQUIRE(firstReflection.attributeValues.size() == 1U);
  REQUIRE(firstReflection.attributeValues.contains(reliable));
  REQUIRE(variableLengthDataBytes(firstReflection.attributeValues.at(reliable)) ==
          valueBytes);
  REQUIRE(variableLengthDataBytes(firstReflection.userSuppliedTag) == tagBytes);
  REQUIRE(firstReflection.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(firstReflection.timeValue == L"6");
  REQUIRE(firstReflection.sentOrderType == TIMESTAMP);
  REQUIRE(firstReflection.receivedOrderType == TIMESTAMP);
  REQUIRE(firstReflection.retractionSupplied);
  REQUIRE(firstReflection.retractionValid);
  REQUIRE_THROWS_AS(
      local->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE(surviving->getObjectInstanceHandle(objectName) == objectInstance);

  // Re-subscribing the locally-forgotten receiver creates a fresh discovery;
  // the next queued update is independently delivered to both recipients.
  REQUIRE_NOTHROW(local->subscribeObjectClassAttributes(child, attributes));
  drainCallbacks(*local);
  REQUIRE(localReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(local->getObjectInstanceHandle(objectName) == objectInstance);
  auto const secondRetraction = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondRetraction.isValid());
  REQUIRE_NOTHROW(local->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(surviving->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  drainCallbacks(*publisher);
  drainCallbacks(*local);
  drainCallbacks(*surviving);
  REQUIRE(localReports.attributeReflectionReports.size() == 1U);
  REQUIRE(localReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(survivingReports.attributeReflectionReports.size() == 2U);
  REQUIRE(survivingReports.timeAdvanceGrantReports.size() == 2U);
  auto const& localReflection = localReports.attributeReflectionReports.front();
  REQUIRE(localReflection.objectInstance == objectInstance);
  REQUIRE(localReflection.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(localReflection.attributeValues.at(reliable)) ==
          valueBytes);
  REQUIRE(variableLengthDataBytes(localReflection.userSuppliedTag) == tagBytes);
  REQUIRE(localReflection.timeValue == L"7");
  REQUIRE(localReflection.sentOrderType == TIMESTAMP);
  REQUIRE(localReflection.receivedOrderType == TIMESTAMP);
  REQUIRE(localReflection.retractionSupplied);
  REQUIRE(localReflection.retractionValid);
  auto const& secondReflection = survivingReports.attributeReflectionReports.back();
  REQUIRE(secondReflection.objectInstance == objectInstance);
  REQUIRE(secondReflection.producingFederate == publisherHandle);
  REQUIRE(secondReflection.timeValue == L"7");
  REQUIRE(secondReflection.sentOrderType == TIMESTAMP);
  REQUIRE(secondReflection.receivedOrderType == TIMESTAMP);
  REQUIRE(secondReflection.retractionSupplied);
  REQUIRE(secondReflection.retractionValid);

  REQUIRE_NOTHROW(local->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(surviving->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(local->disconnect());
  REQUIRE_NOTHROW(surviving->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
