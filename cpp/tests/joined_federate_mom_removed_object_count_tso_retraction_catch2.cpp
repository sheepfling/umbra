#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The joined-federate MOM removed-object-count tests require the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
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

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"federation-joined-mom-removed-count-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
      "data" / relativePath;
}

class ReportingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct ObjectRemovalReport final {
    ObjectInstanceHandle objectInstance;
    VariableLengthData userSuppliedTag;
    FederateHandle producingFederate;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct RequestRetractionReport final {
    bool retractionValid = false;
    VariableLengthData encodedRetraction;
  };

  struct TimeAdvanceGrantReport final {
    std::wstring timeImplementationName;
    std::wstring value;
  };

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
      RegionHandleSet const* optionalSentRegions) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  void removeObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      VariableLengthData const& userSuppliedTag,
      FederateHandle const& producingFederate,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    objectRemovalReports.push_back({
        objectInstance,
        userSuppliedTag,
        producingFederate,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("remove");
  }

  void requestRetraction(MessageRetractionHandle const& retraction) override {
    requestRetractionReports.push_back({
        retraction.isValid(),
        retraction.encode(),
    });
    callbackOrder.push_back("request-retraction");
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({time.implementationName(), time.toString()});
  }

  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<ObjectRemovalReport> objectRemovalReports;
  std::vector<RequestRetractionReport> requestRetractionReports;
  std::vector<TimeAdvanceGrantReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

void drainCallbacks(RTIambassador& rti) {
  for (int pass = 0; pass != 256; ++pass) {
    if (!rti.evokeCallback(0.0)) {
      break;
    }
  }
}

}  // namespace

TEST_CASE(
    "Embedded joined-federate MOM HLAobjectInstancesRemoved preserves history after timestamped retraction",
    "[integration][development-profile][federation-management][mom]"
    "[object-management][time-management][timestamped-remove-object-instance]"
    "[retraction][joined-federate-mom-removed-object-count-tso-retraction]"
    "[rti.service.request-attribute-value-update][rti.service.delete-object-instance]"
    "[rti.service.retract][rti.service.time-advance-request]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.get-object-instance-name]"
    "[rti.service.get-object-instance-handle][rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.evoke-callback][federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction][federate.callback.time-advance-grant]"
    "[federate.callback.reflect-attribute-values]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("attribute-update-passel-fom.xml").wstring();
  unsigned char const tagBytes[] = {0xD5, 0x37, 0x2B};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  auto const immediateFederate = immediate->joinFederationExecution(
      L"removed-count-immediate", L"subscriber", federationName);

  auto const momClass = immediate->getObjectClassHandle(
      standard_hla::mom::federate_object_class);
  auto const federateHandleAttribute = immediate->getAttributeHandle(
      momClass, standard_hla::mom::federate_handle);
  auto const removedCountAttribute = immediate->getAttributeHandle(
      momClass, standard_hla::mom::object_instances_removed);
  auto const reliable = immediate->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  REQUIRE(momClass.isValid());
  REQUIRE(federateHandleAttribute.isValid());
  REQUIRE(removedCountAttribute.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(
      momClass,
      AttributeHandleSet{federateHandleAttribute, removedCountAttribute},
      true));

  auto const publisherFederate = publisher->joinFederationExecution(
      L"removed-count-publisher", L"publisher", federationName);
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"removed-count-constrained", L"subscriber", federationName));
  drainCallbacks(*immediate);

  auto const reflectedPublisher = std::find_if(
      immediateReports.attributeReflectionReports.begin(),
      immediateReports.attributeReflectionReports.end(),
      [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
        auto const value = report.attributeValues.find(federateHandleAttribute);
        return value != report.attributeValues.end() &&
            variableLengthDataBytes(value->second) ==
                variableLengthDataBytes(publisherFederate.encode());
      });
  REQUIRE(reflectedPublisher != immediateReports.attributeReflectionReports.end());
  auto const immediateMomObject = [&] {
    auto const reflectedImmediate = std::find_if(
        immediateReports.attributeReflectionReports.begin(),
        immediateReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          auto const value = report.attributeValues.find(federateHandleAttribute);
          return value != report.attributeValues.end() &&
              variableLengthDataBytes(value->second) ==
                  variableLengthDataBytes(immediateFederate.encode());
        });
    REQUIRE(reflectedImmediate != immediateReports.attributeReflectionReports.end());
    return reflectedImmediate->objectInstance;
  }();
  AttributeHandleSet const removedCountSet{removedCountAttribute};

  auto requestRemovedCount = [&] {
    auto const before = immediateReports.attributeReflectionReports.size();
    REQUIRE_NOTHROW(immediate->requestAttributeValueUpdate(
        immediateMomObject,
        removedCountSet,
        VariableLengthData{}));
    drainCallbacks(*immediate);
    auto const reflectionIterator = std::find_if(
        immediateReports.attributeReflectionReports.begin() +
            static_cast<std::ptrdiff_t>(before),
        immediateReports.attributeReflectionReports.end(),
        [&](ReportingFederateAmbassador::AttributeReflectionReport const& report) {
          return report.objectInstance == immediateMomObject &&
              report.attributeValues.contains(removedCountAttribute);
        });
    REQUIRE(reflectionIterator != immediateReports.attributeReflectionReports.end());
    auto const& reflection = *reflectionIterator;
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.transportationType == reliable);
    REQUIRE_FALSE(reflection.producingFederate.isValid());
    REQUIRE_FALSE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.userSuppliedTag.size() == 0U);
    rti1516_2025::HLAinteger32BE decoded;
    REQUIRE_NOTHROW(decoded.decode(reflection.attributeValues.at(removedCountAttribute)));
    return decoded.get();
  };

  auto const child = publisher->getObjectClassHandle(
      L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
  auto const reliableAttribute = publisher->getAttributeHandle(child, L"ReliableBaseA");
  auto const bestEffortAttribute = publisher->getAttributeHandle(child, L"BestEffortBase");
  REQUIRE(child.isValid());
  REQUIRE(reliableAttribute.isValid());
  REQUIRE(bestEffortAttribute.isValid());
  AttributeHandleSet const attributes{reliableAttribute, bestEffortAttribute};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(immediate->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(constrained->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(requestRemovedCount() == 0);

  AttributeHandleSet const immediateOwnedAttribute{bestEffortAttribute};
  REQUIRE_NOTHROW(immediate->publishObjectClassAttributes(child, immediateOwnedAttribute));
  REQUIRE_NOTHROW(immediate->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      immediateOwnedAttribute,
      tag));
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(publisher->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      immediateOwnedAttribute,
      tag,
      divestedAttributes));
  REQUIRE(divestedAttributes == immediateOwnedAttribute);
  drainCallbacks(*immediate);
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffortAttribute));
  REQUIRE(immediate->isAttributeOwnedByFederate(objectInstance, bestEffortAttribute));

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  drainCallbacks(*constrained);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  drainCallbacks(*publisher);

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  drainCallbacks(*immediate);
  REQUIRE(immediateReports.objectRemovalReports.size() == 1U);
  REQUIRE(immediateReports.objectRemovalReports.front().retractionSupplied);
  REQUIRE(immediateReports.objectRemovalReports.front().retractionValid);
  REQUIRE(constrainedReports.objectRemovalReports.empty());
  REQUIRE(requestRemovedCount() == 1);
  REQUIRE_THROWS_AS(
      publisher->getObjectInstanceName(objectInstance),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      immediate->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drainCallbacks(*immediate);
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder.size() >= 2U);
  REQUIRE(immediateReports.callbackOrder[immediateReports.callbackOrder.size() - 2] ==
          "remove");
  REQUIRE(immediateReports.callbackOrder.back() == "request-retraction");
  REQUIRE(requestRemovedCount() == 1);

  REQUIRE(publisher->getObjectInstanceName(objectInstance) == objectInstanceName);
  REQUIRE(immediate->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(constrained->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(publisher->isAttributeOwnedByFederate(objectInstance, reliableAttribute));
  REQUIRE_FALSE(publisher->isAttributeOwnedByFederate(objectInstance, bestEffortAttribute));
  REQUIRE(immediate->isAttributeOwnedByFederate(objectInstance, bestEffortAttribute));

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.objectRemovalReports.empty());

  REQUIRE_NOTHROW(constrained->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
