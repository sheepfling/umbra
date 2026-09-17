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
#error "The timestamped regional Auto Provide admission-fence test requires the Umbra source directory."
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
  return L"regional-auto-provide-timestamped-switch-admission-" +
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
    "Embedded regional Auto Provide fences timestamped discovery work before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][mom][time-management][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-timestamped-switch-admission-mutation]"
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
    std::vector<std::string> callbackOrder;
    unsigned char const valueBytes[] = {0x54, 0x53, 0x41};
    unsigned char const tagBytes[] = {0x54, 0x47, 0x41};
    std::vector<unsigned char> const value(
        valueBytes,
        valueBytes + sizeof(valueBytes));
    std::vector<unsigned char> const tagValue(
        tagBytes,
        tagBytes + sizeof(tagBytes));
    VariableLengthData const tag(tagValue.data(), tagValue.size());
    rti1516_2025::MessageRetractionHandle responseRetraction;
    FederateHandle ownerHandle;
    bool disableOnFirstDiscovery = true;
    bool captureCallbacks = false;
    std::size_t provideCount = 0;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-timestamped-admission-owner",
        L"provider",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-timestamped-admission-requester",
        L"subscriber",
        federationName));

    auto const ownerObjectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const ownerAAttribute = owner->getAttributeHandle(
        ownerObjectClass,
        L"ProviderAValue");
    auto const ownerBAttribute = owner->getAttributeHandle(
        ownerObjectClass,
        L"ProviderBValue");
    auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
    auto const requesterObjectClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const requesterAAttribute = requester->getAttributeHandle(
        requesterObjectClass,
        L"ProviderAValue");
    auto const requesterBAttribute = requester->getAttributeHandle(
        requesterObjectClass,
        L"ProviderBValue");
    auto const requesterDimension = requester->getDimensionHandle(L"UmbraRegionX");
    auto const setSwitches = owner->getInteractionClassHandle(
        standard_hla::mom::set_switches_federation);
    auto const autoProvideParameter = owner->getParameterHandle(
        setSwitches,
        standard_hla::mom::auto_provide);
    REQUIRE(ownerObjectClass.isValid());
    REQUIRE(ownerAAttribute.isValid());
    REQUIRE(ownerBAttribute.isValid());
    REQUIRE(ownerDimension.isValid());
    REQUIRE(requesterObjectClass.isValid());
    REQUIRE(requesterAAttribute.isValid());
    REQUIRE(requesterBAttribute.isValid());
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

    AttributeHandleSet const ownerAttributes{ownerAAttribute, ownerBAttribute};
    AttributeHandleSet const requesterAttributes{
        requesterAAttribute,
        requesterBAttribute};
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
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(0UL, 1UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requesterRegion}));
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
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_FALSE(requester->evokeCallback(0.0));
    }
    REQUIRE(requesterReports.timeConstrainedEnabledReports.size() == 1U);
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      while (owner->evokeCallback(0.0)) {
      }
    }
    REQUIRE(ownerReports.timeRegulationEnabledReports.size() == 1U);

    requesterReports.onDiscoverObjectInstance = [&] {
      if (captureCallbacks) {
        callbackOrder.push_back("discover");
      }
      if (disableOnFirstDiscovery) {
        disableOnFirstDiscovery = false;
        // The switch changes before queueObjectInstanceDiscovery admits any
        // provider recipients. Discovery remains valid, but no timestamped
        // provider response may be solicited for this object.
        setAutoProvide(false);
      }
    };
    ownerReports.onProvideAttributeValueUpdate = [&] {
      if (captureCallbacks) {
        callbackOrder.push_back("provide");
      }
      ++provideCount;
    };
    requesterReports.onAttributeReflection = [&] {
      if (captureCallbacks) {
        callbackOrder.push_back("reflect");
      }
    };

    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &responseRetraction,
        &provideCount,
        value,
        tag,
        ownerAAttribute,
        ownerBAttribute](
        ObjectInstanceHandle const& callbackObject,
        AttributeHandleSet const& callbackAttributes,
        VariableLengthData const&) {
      AttributeHandleValueMap values;
      for (AttributeHandle const& attribute : callbackAttributes) {
        if (attribute == ownerAAttribute || attribute == ownerBAttribute) {
          values.emplace(
              attribute,
              VariableLengthData(value.data(), value.size()));
        }
      }
      if (!values.empty()) {
        responseRetraction = owner->updateAttributeValues(
            callbackObject,
            values,
            tag,
            rti1516_2025::HLAinteger64Time(2));
      }
      REQUIRE(provideCount == 1U);
    };

    captureCallbacks = true;
    ObjectInstanceHandle firstObject;
    REQUIRE_NOTHROW(firstObject = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        ownerPair));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      while (requester->evokeCallback(0.0)) {
      }
      while (owner->evokeCallback(0.0)) {
      }
    }
    REQUIRE(firstObject.isValid());
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.empty());
    REQUIRE(provideCount == 0U);
    REQUIRE_FALSE(owner->getAutoProvideSwitch());
    REQUIRE_FALSE(requester->getAutoProvideSwitch());
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    REQUIRE(callbackOrder == std::vector<std::string>{"discover"});

    REQUIRE_NOTHROW(setAutoProvide(true));
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());
    callbackOrder.clear();
    requesterReports.attributeReflectionReports.clear();
    requesterReports.callbackOrder.clear();
    ownerReports.attributeValueUpdateRequestReports.clear();

    ObjectInstanceHandle secondObject;
    REQUIRE_NOTHROW(secondObject = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        ownerPair));
    if (callbackModel != rti1516_2025::HLA_IMMEDIATE) {
      while (requester->evokeCallback(0.0)) {
      }
      while (owner->evokeCallback(0.0)) {
      }
    }
    REQUIRE(secondObject.isValid());
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance ==
            secondObject);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes ==
            ownerAttributes);
    REQUIRE(provideCount == 1U);
    REQUIRE(responseRetraction.isValid());
    REQUIRE(requesterReports.attributeReflectionReports.empty());
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});

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
    REQUIRE(reflection.objectInstance == secondObject);
    REQUIRE(reflection.attributeValues.size() == 2U);
    REQUIRE(reflection.attributeValues.contains(requesterAAttribute));
    REQUIRE(reflection.attributeValues.contains(requesterBAttribute));
    REQUIRE(variableLengthDataBytes(
                reflection.attributeValues.at(requesterAAttribute)) ==
            value);
    REQUIRE(variableLengthDataBytes(
                reflection.attributeValues.at(requesterBAttribute)) ==
            value);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == tagValue);
    REQUIRE(reflection.transportationType ==
            owner->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(reflection.producingFederate == ownerHandle);
    REQUIRE(reflection.timeValue == L"2");
    REQUIRE(reflection.sentOrderType == TIMESTAMP);
    REQUIRE(reflection.receivedOrderType == TIMESTAMP);
    REQUIRE(reflection.retractionSupplied);
    REQUIRE(reflection.retractionValid);
    REQUIRE_THROWS_AS(
        owner->retract(responseRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);

    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(owner->evokeCallback(0.0));
      static_cast<void>(requester->evokeCallback(0.0));
    }
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(provideCount == 1U);

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
