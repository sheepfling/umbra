#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped regional Auto Provide response test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
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
  return L"regional-auto-provide-timestamped-response-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / relativePath;
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct ObjectDiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
    FederateHandle producingFederate;
  };

  struct AttributeValueUpdateRequestReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeReflectionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
    std::wstring timeImplementationName;
    std::wstring timeValue;
    OrderType sentOrderType = RECEIVE;
    OrderType receivedOrderType = RECEIVE;
    bool retractionSupplied = false;
    bool retractionValid = false;
  };

  struct TimeReport final {
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
    if (onDiscoverObjectInstance) {
      onDiscoverObjectInstance();
    }
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeValueUpdateRequestReports.push_back({
        objectInstance,
        attributes,
        userSuppliedTag,
    });
    if (provideAttributeValueUpdateHandler) {
      provideAttributeValueUpdateHandler(objectInstance, attributes, userSuppliedTag);
    }
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions,
      LogicalTime const& time,
      OrderType sentOrderType,
      OrderType receivedOrderType,
      MessageRetractionHandle const* optionalRetraction) override {
    attributeReflectionReports.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
        time.implementationName(),
        time.toString(),
        sentOrderType,
        receivedOrderType,
        optionalRetraction != nullptr,
        optionalRetraction != nullptr && optionalRetraction->isValid(),
    });
    callbackOrder.push_back("reflect");
    if (onAttributeReflection) {
      onAttributeReflection();
    }
  }

  void timeConstrainedEnabled(LogicalTime const& time) override {
    timeConstrainedEnabledReports.push_back({
        time.implementationName(),
        time.toString(),
    });
  }

  void timeRegulationEnabled(LogicalTime const& time) override {
    timeRegulationEnabledReports.push_back({
        time.implementationName(),
        time.toString(),
    });
  }

  void timeAdvanceGrant(LogicalTime const& time) override {
    timeAdvanceGrantReports.push_back({
        time.implementationName(),
        time.toString(),
    });
    callbackOrder.push_back("grant");
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<TimeReport> timeConstrainedEnabledReports;
  std::vector<TimeReport> timeRegulationEnabledReports;
  std::vector<TimeReport> timeAdvanceGrantReports;
  std::vector<std::string> callbackOrder;
  std::function<void()> onDiscoverObjectInstance;
  std::function<void()> onAttributeReflection;
  std::function<void(
      ObjectInstanceHandle const&,
      AttributeHandleSet const&,
      VariableLengthData const&)> provideAttributeValueUpdateHandler;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded regional Auto Provide timestamped response reflects once and exposes valid retraction under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][time-management][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-timestamped-response]"
    "[auto-provide-regional][timestamped-regional-attribute-update][tso][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[rti.service.update-attribute-values][rti.service.retract]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
    unsigned char const requestTagBytes[] = {0x54, 0x53, 0x4F};
    unsigned char const responseTagBytes[] = {0x52, 0x41, 0x54, 0x53};
    std::vector<unsigned char> const responseValue{0x52, 0x45, 0x47};
    VariableLengthData const requestTag(requestTagBytes, sizeof(requestTagBytes));
    VariableLengthData const responseTag(responseTagBytes, sizeof(responseTagBytes));
    std::vector<unsigned char> observedRequestTag;
    std::vector<std::string> callbackOrder;
    MessageRetractionHandle responseRetraction;
    FederateHandle ownerHandle;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-timestamped-owner", L"provider", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-timestamped-requester", L"subscriber", federationName));

    auto const ownerObjectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const ownerAttribute = owner->getAttributeHandle(
        ownerObjectClass,
        L"ProviderAValue");
    auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
    auto const requesterObjectClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const requesterAttribute = requester->getAttributeHandle(
        requesterObjectClass,
        L"ProviderAValue");
    auto const requesterDimension = requester->getDimensionHandle(L"UmbraRegionX");
    REQUIRE(ownerObjectClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE(ownerDimension.isValid());
    REQUIRE(requesterObjectClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE(requesterDimension.isValid());
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());

    AttributeHandleSet const ownerAttributes{ownerAttribute};
    AttributeHandleSet const requesterAttributes{requesterAttribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerObjectClass,
        ownerAttributes));
    REQUIRE_NOTHROW(owner->changeDefaultAttributeOrderType(
        ownerObjectClass,
        ownerAttributes,
        TIMESTAMP));
    auto const ownerRegion = owner->createRegion(DimensionHandleSet{ownerDimension});
    auto const requesterRegion = requester->createRegion(
        DimensionHandleSet{requesterDimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(
        ownerRegion,
        ownerDimension,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));
    AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
        ownerAttributes,
        RegionHandleSet{ownerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
        requesterAttributes,
        RegionHandleSet{requesterRegion},
    }};
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(requester->enableTimeConstrained());
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      while (requester->evokeCallback(0.0)) {
      }
      while (owner->evokeCallback(0.0)) {
      }
    }
    REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    ownerReports.provideAttributeValueUpdateHandler = [
        &callbackOrder,
        &observedRequestTag,
        &owner,
        &responseRetraction,
        ownerAttribute,
        responseValue,
        responseTag](ObjectInstanceHandle const& callbackObject,
                     AttributeHandleSet const& callbackAttributes,
                     VariableLengthData const& callbackTag) {
      callbackOrder.push_back("provide");
      observedRequestTag = variableLengthDataBytes(callbackTag);
      AttributeHandleValueMap values;
      for (auto const& attribute : callbackAttributes) {
        REQUIRE(attribute == ownerAttribute);
        values.emplace(
            attribute,
            VariableLengthData(responseValue.data(), responseValue.size()));
      }
      responseRetraction = owner->updateAttributeValues(
          callbackObject,
          values,
          responseTag,
          rti1516_2025::HLAinteger64Time(2));
    };
    requesterReports.onDiscoverObjectInstance = [&callbackOrder] {
      callbackOrder.push_back("discover");
    };
    requesterReports.onAttributeReflection = [&callbackOrder] {
      callbackOrder.push_back("reflect");
    };

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        ownerPair));
    REQUIRE(objectInstance.isValid());
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
      }
    }

    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});
    REQUIRE(responseRetraction.isValid());
    auto const& request = ownerReports.attributeValueUpdateRequestReports.front();
    REQUIRE(request.objectInstance == objectInstance);
    REQUIRE(request.attributes == ownerAttributes);
    REQUIRE(variableLengthDataBytes(request.userSuppliedTag).empty());
    REQUIRE(observedRequestTag.empty());

    REQUIRE_NOTHROW(requester->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      while (owner->evokeCallback(0.0)) {
      }
      while (requester->evokeCallback(0.0)) {
      }
    }

    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(requesterReports.callbackOrder ==
            std::vector<std::string>{"reflect", "grant"});
    REQUIRE(callbackOrder ==
            std::vector<std::string>{"discover", "provide", "reflect"});
    auto const& reflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.attributeValues.contains(requesterAttribute));
    REQUIRE(variableLengthDataBytes(
                reflection.attributeValues.at(requesterAttribute)) ==
            responseValue);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) ==
            std::vector<unsigned char>(
                responseTagBytes,
                responseTagBytes + sizeof(responseTagBytes)));
    REQUIRE(reflection.transportationType ==
            owner->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(reflection.producingFederate == ownerHandle);
    REQUIRE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.sentRegions == RegionHandleSet{ownerRegion});
    REQUIRE(reflection.timeValue == L"2");
    REQUIRE(reflection.sentOrderType == TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);

    REQUIRE_THROWS_AS(
        owner->retract(responseRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
