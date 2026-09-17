#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The timestamped regional Auto Provide switch-mutation test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::LogicalTime;
using rti1516_2025::MessageRetractionHandle;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::ParameterHandleValueMap;
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
  return L"regional-auto-provide-timestamped-switch-mutation-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
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
    if (onProvideAttributeValueUpdate) {
      onProvideAttributeValueUpdate();
    }
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
  std::function<void()> onProvideAttributeValueUpdate;
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
    "Embedded regional Auto Provide preserves a queued timestamped response across switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][mom][time-management][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-timestamped-switch-mutation]"
    "[auto-provide-regional][timestamped-regional-attribute-update][tso][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.send-interaction]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.update-attribute-values]"
    "[rti.service.retract]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.time-constrained-enabled]"
    "[federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](CallbackModel const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                            "cpp" / "tests" / "data" /
                            "regional-ownership-fanout-fom.xml")
                               .wstring();
    auto const ownerValue = std::vector<unsigned char>{0x54, 0x53, 0x4D, 0x31};
    auto const secondValue = std::vector<unsigned char>{0x54, 0x53, 0x4D, 0x32};
    auto const ownerTagBytes = std::vector<unsigned char>{0x54, 0x53, 0x4D, 0x31};
    auto const secondTagBytes = std::vector<unsigned char>{0x54, 0x53, 0x4D, 0x32};
    VariableLengthData const ownerTag(ownerTagBytes.data(), ownerTagBytes.size());
    VariableLengthData const secondTag(secondTagBytes.data(), secondTagBytes.size());
    std::vector<std::string> callbackOrder;
    std::vector<rti1516_2025::MessageRetractionHandle> responseRetractions;
    std::size_t provideCount = 0;
    FederateHandle ownerHandle;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-timestamped-switch-owner",
        L"provider",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-timestamped-switch-requester",
        L"subscriber",
        federationName));

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
    auto const setSwitches = owner->getInteractionClassHandle(
        standard_hla::mom::set_switches_federation);
    auto const autoProvideParameter = owner->getParameterHandle(
        setSwitches,
        standard_hla::mom::auto_provide);
    REQUIRE(ownerObjectClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE(ownerDimension.isValid());
    REQUIRE(requesterObjectClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE(requesterDimension.isValid());
    REQUIRE(setSwitches.isValid());
    REQUIRE(autoProvideParameter.isValid());
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());

    auto setAutoProvide = [&](bool const enabled) {
      auto const encoded = rti1516_2025::HLAinteger32BE(
          enabled ? 1 : 0).encode();
      ParameterHandleValueMap switchParameters;
      switchParameters.emplace(autoProvideParameter, encoded);
      REQUIRE_NOTHROW(owner->sendInteraction(
          setSwitches,
          switchParameters,
          VariableLengthData{}));
    };
    auto pumpCallbacks = [&] {
      if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
        for (int pass = 0; pass != 64; ++pass) {
          static_cast<void>(requester->evokeCallback(0.0));
          static_cast<void>(owner->evokeCallback(0.0));
        }
      }
    };

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
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    REQUIRE_NOTHROW(requester->enableTimeConstrained());
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    pumpCallbacks();
    REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    requesterReports.onDiscoverObjectInstance = [&] {
      callbackOrder.push_back("discover");
    };
    ownerReports.onProvideAttributeValueUpdate = [&] {
      callbackOrder.push_back("provide");
      ++provideCount;
    };
    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &setAutoProvide,
        &responseRetractions,
        &provideCount,
        &ownerValue,
        &secondValue,
        &ownerTag,
        &secondTag,
        ownerAttribute](
        ObjectInstanceHandle const& callbackObject,
        AttributeHandleSet const& callbackAttributes,
        VariableLengthData const&) {
      REQUIRE(callbackAttributes == AttributeHandleSet{ownerAttribute});
      REQUIRE(provideCount == responseRetractions.size() + 1U);
      auto const firstResponse = provideCount == 1U;
      auto const& value = firstResponse ? ownerValue : secondValue;
      auto const& tag = firstResponse ? ownerTag : secondTag;
      auto const responseTime = firstResponse ? 2 : 3;
      AttributeHandleValueMap values;
      values.emplace(
          ownerAttribute,
          VariableLengthData(value.data(), value.size()));
      responseRetractions.push_back(owner->updateAttributeValues(
          callbackObject,
          values,
          tag,
          rti1516_2025::HLAinteger64Time(responseTime)));
      REQUIRE(responseRetractions.back().isValid());
      if (firstResponse) {
        // This mutation occurs after the timestamped response is accepted.
        // It must fence only later Auto Provide work, not this queued message.
        setAutoProvide(false);
      }
    };

    ObjectInstanceHandle const firstObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(firstObject.isValid());
    pumpCallbacks();
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(provideCount == 1U);
    REQUIRE_FALSE(owner->getAutoProvideSwitch());
    REQUIRE_FALSE(requester->getAutoProvideSwitch());
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    REQUIRE(responseRetractions.size() == 1U);
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});

    REQUIRE_NOTHROW(requester->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(2)));
    pumpCallbacks();
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 1U);
    auto const& firstReflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(firstReflection.objectInstance == firstObject);
    REQUIRE(firstReflection.attributeValues.size() == 1U);
    REQUIRE(firstReflection.attributeValues.contains(requesterAttribute));
    REQUIRE(variableLengthDataBytes(
                firstReflection.attributeValues.at(requesterAttribute)) == ownerValue);
    REQUIRE(variableLengthDataBytes(firstReflection.userSuppliedTag) == ownerTagBytes);
    REQUIRE(firstReflection.transportationType ==
            owner->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(firstReflection.producingFederate == ownerHandle);
    REQUIRE(firstReflection.sentRegionsSupplied);
    REQUIRE(firstReflection.sentRegions == RegionHandleSet{ownerRegion});
    REQUIRE(firstReflection.timeValue == L"2");
    REQUIRE(firstReflection.sentOrderType == TIMESTAMP);
    REQUIRE(firstReflection.receivedOrderType == TIMESTAMP);
    REQUIRE(firstReflection.retractionSupplied);
    REQUIRE(firstReflection.retractionValid);
    REQUIRE_THROWS_AS(
        owner->retract(responseRetractions.front()),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    callbackOrder.clear();
    ownerReports.attributeValueUpdateRequestReports.clear();
    REQUIRE_NOTHROW(setAutoProvide(true));
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());

    ObjectInstanceHandle const secondObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(secondObject.isValid());
    pumpCallbacks();
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance ==
            secondObject);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes ==
            ownerAttributes);
    REQUIRE(provideCount == 2U);
    REQUIRE(responseRetractions.size() == 2U);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});

    REQUIRE_NOTHROW(requester->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(3)));
    pumpCallbacks();
    REQUIRE(requesterReports.attributeReflectionReports.size() == 2U);
    REQUIRE(requesterReports.timeAdvanceGrantReports.size() == 2U);
    auto const& secondReflection = requesterReports.attributeReflectionReports.back();
    REQUIRE(secondReflection.objectInstance == secondObject);
    REQUIRE(secondReflection.attributeValues.size() == 1U);
    REQUIRE(secondReflection.attributeValues.contains(requesterAttribute));
    REQUIRE(variableLengthDataBytes(
                secondReflection.attributeValues.at(requesterAttribute)) == secondValue);
    REQUIRE(variableLengthDataBytes(secondReflection.userSuppliedTag) == secondTagBytes);
    REQUIRE(secondReflection.transportationType ==
            owner->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(secondReflection.producingFederate == ownerHandle);
    REQUIRE(secondReflection.sentRegionsSupplied);
    REQUIRE(secondReflection.sentRegions == RegionHandleSet{ownerRegion});
    REQUIRE(secondReflection.timeValue == L"3");
    REQUIRE(secondReflection.sentOrderType == TIMESTAMP);
    REQUIRE(secondReflection.receivedOrderType == TIMESTAMP);
    REQUIRE(secondReflection.retractionSupplied);
    REQUIRE(secondReflection.retractionValid);
    REQUIRE_THROWS_AS(
        owner->retract(responseRetractions.back()),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(firstObject, ownerPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(secondObject, ownerPair));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
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
