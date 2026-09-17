#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/encoding/BasicDataElements.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional Auto Provide switch-mutation test requires the Umbra source directory."
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
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
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
  return L"regional-auto-provide-switch-mutation-" +
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
    rti1516_2025::TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct AttributeOwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  struct AttributeOwnershipAcquisitionReport final {
    enum class Kind {
      notification,
      unavailable,
    };

    Kind kind = Kind::unavailable;
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    if (onDiscoverObjectInstance) {
      onDiscoverObjectInstance();
    }
    objectDiscoveryReports.push_back({
        objectInstance,
        objectClass,
        objectInstanceName,
        producingFederate,
    });
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    if (onProvideAttributeValueUpdate) {
      onProvideAttributeValueUpdate();
    }
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
      rti1516_2025::TransportationTypeHandle const& transportationType,
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
    if (onAttributeReflection) {
      onAttributeReflection();
    }
  }

  void requestAttributeOwnershipAssumption(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& offeredAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAssumptionReports.push_back({
        objectInstance,
        offeredAttributes,
        userSuppliedTag,
    });
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    attributeOwnershipAcquisitionReports.push_back({
        AttributeOwnershipAcquisitionReport::Kind::notification,
        objectInstance,
        securedAttributes,
        userSuppliedTag,
    });
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
  std::vector<AttributeOwnershipAssumptionReport> attributeOwnershipAssumptionReports;
  std::vector<AttributeOwnershipAcquisitionReport> attributeOwnershipAcquisitionReports;
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
    "Embedded regional Auto Provide suppresses queued work after switch mutation and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ownership-management][ddm][mom][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-switch-mutation]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.get-interaction-class-handle]"
    "[rti.service.get-parameter-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.send-interaction]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.update-attribute-values]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.request-attribute-ownership-assumption]"
    "[federate.callback.attribute-ownership-acquisition-notification]") {
  auto runScenario = [](rti1516_2025::CallbackModel const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador secondProviderReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto secondProvider = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
    unsigned char const divestitureTagBytes[] = {0xD1, 0x4D, 0x53};
    unsigned char const acquisitionTagBytes[] = {0xA1, 0x4D, 0x53};
    unsigned char const providerAValueBytes[] = {0x41, 0x2D, 0x53, 0x57};
    unsigned char const providerBValueBytes[] = {0x42, 0x2D, 0x53, 0x57};
    unsigned char const providerATagBytes[] = {0x54, 0x41, 0x53};
    std::vector<unsigned char> const providerAValue(
        providerAValueBytes,
        providerAValueBytes + sizeof(providerAValueBytes));
    std::vector<unsigned char> const providerBValue(
        providerBValueBytes,
        providerBValueBytes + sizeof(providerBValueBytes));
    VariableLengthData const divestitureTag(
        divestitureTagBytes,
        sizeof(divestitureTagBytes));
    VariableLengthData const acquisitionTag(
        acquisitionTagBytes,
        sizeof(acquisitionTagBytes));
    VariableLengthData const providerATag(
        providerATagBytes,
        sizeof(providerATagBytes));
    FederateHandle ownerHandle;
    FederateHandle secondProviderHandle;
    std::vector<std::string> callbackOrder;
    std::size_t ownerProviderCount = 0U;
    std::size_t secondProviderCount = 0U;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(secondProvider->connect(secondProviderReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-switch-owner", L"provider-a", federationName));
    REQUIRE_NOTHROW(secondProviderHandle = secondProvider->joinFederationExecution(
        L"regional-auto-provide-switch-second-provider", L"provider-b", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-switch-requester", L"subscriber", federationName));

    auto const ownerObjectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const ownerAAttribute = owner->getAttributeHandle(ownerObjectClass, L"ProviderAValue");
    auto const ownerBAttribute = owner->getAttributeHandle(ownerObjectClass, L"ProviderBValue");
    auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
    auto const secondProviderObjectClass = secondProvider->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const secondProviderBAttribute = secondProvider->getAttributeHandle(
        secondProviderObjectClass,
        L"ProviderBValue");
    auto const secondProviderDimension = secondProvider->getDimensionHandle(L"UmbraRegionX");
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
    REQUIRE(secondProviderObjectClass.isValid());
    REQUIRE(secondProviderBAttribute.isValid());
    REQUIRE(secondProviderDimension.isValid());
    REQUIRE(requesterObjectClass.isValid());
    REQUIRE(requesterAAttribute.isValid());
    REQUIRE(requesterBAttribute.isValid());
    REQUIRE(requesterDimension.isValid());
    REQUIRE(setSwitches.isValid());
    REQUIRE(autoProvideParameter.isValid());
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(secondProvider->getAutoProvideSwitch());
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
    auto drain = [&] {
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        return;
      }
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
        static_cast<void>(secondProvider->evokeCallback(0.0));
      }
    };

    AttributeHandleSet const ownerAttributes{ownerAAttribute, ownerBAttribute};
    AttributeHandleSet const secondProviderAttributes{secondProviderBAttribute};
    AttributeHandleSet const requesterAttributes{requesterAAttribute, requesterBAttribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(ownerObjectClass, ownerAttributes));
    REQUIRE_NOTHROW(secondProvider->publishObjectClassAttributes(
        secondProviderObjectClass,
        secondProviderAttributes));
    auto const ownerRegion = owner->createRegion(DimensionHandleSet{ownerDimension});
    auto const secondProviderRegion = secondProvider->createRegion(
        DimensionHandleSet{secondProviderDimension});
    auto const requesterRegion = requester->createRegion(
        DimensionHandleSet{requesterDimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(ownerRegion, ownerDimension, RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(secondProvider->setRangeBounds(
        secondProviderRegion,
        secondProviderDimension,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{ownerRegion}));
    REQUIRE_NOTHROW(secondProvider->commitRegionModifications(
        RegionHandleSet{secondProviderRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(RegionHandleSet{requesterRegion}));
    AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
        ownerAttributes,
        RegionHandleSet{ownerRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const secondProviderPair{{
        secondProviderAttributes,
        RegionHandleSet{secondProviderRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
        requesterAttributes,
        RegionHandleSet{requesterRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const ownerBRegionPair{{
        AttributeHandleSet{ownerBAttribute},
        RegionHandleSet{ownerRegion},
    }};
    REQUIRE_NOTHROW(secondProvider->subscribeObjectClassAttributesWithRegions(
        secondProviderObjectClass,
        secondProviderPair));
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    ObjectInstanceHandle const firstObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(firstObject.isValid());
    drain();
    REQUIRE(secondProviderReports.objectDiscoveryReports.size() == 1U);
    ownerReports.attributeValueUpdateRequestReports.clear();
    secondProviderReports.attributeValueUpdateRequestReports.clear();

    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        firstObject,
        AttributeHandleSet{ownerBAttribute},
        divestitureTag));
    drain();
    REQUIRE(secondProviderReports.attributeOwnershipAssumptionReports.size() == 1U);
    REQUIRE(secondProviderReports.attributeOwnershipAssumptionReports.front().objectInstance ==
            firstObject);
    REQUIRE(secondProviderReports.attributeOwnershipAssumptionReports.front().attributes ==
            AttributeHandleSet{secondProviderBAttribute});
    REQUIRE(variableLengthDataBytes(
                secondProviderReports.attributeOwnershipAssumptionReports.front().userSuppliedTag) ==
            std::vector<unsigned char>(
                divestitureTagBytes,
                divestitureTagBytes + sizeof(divestitureTagBytes)));
    REQUIRE_NOTHROW(secondProvider->attributeOwnershipAcquisitionIfAvailable(
        firstObject,
        AttributeHandleSet{secondProviderBAttribute},
        acquisitionTag));
    drain();
    REQUIRE(secondProviderReports.attributeOwnershipAcquisitionReports.size() == 1U);
    REQUIRE(secondProviderReports.attributeOwnershipAcquisitionReports.front().kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
    REQUIRE(secondProviderReports.attributeOwnershipAcquisitionReports.front().objectInstance ==
            firstObject);
    REQUIRE(secondProvider->isAttributeOwnedByFederate(
        firstObject,
        secondProviderBAttribute));

    requesterReports.onDiscoverObjectInstance = [&callbackOrder] {
      callbackOrder.push_back("discover");
    };
    ownerReports.onProvideAttributeValueUpdate = [&callbackOrder, &ownerProviderCount] {
      callbackOrder.push_back("provide");
      ++ownerProviderCount;
    };
    secondProviderReports.onProvideAttributeValueUpdate = [&secondProviderCount] {
      ++secondProviderCount;
    };
    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &setAutoProvide,
        &ownerProviderCount,
        ownerAAttribute,
        ownerBAttribute,
        providerAValue,
        providerBValue,
        providerATag](ObjectInstanceHandle const& callbackObject,
                      AttributeHandleSet const& callbackAttributes,
                      VariableLengthData const& callbackTag) {
      REQUIRE(variableLengthDataBytes(callbackTag).empty());
      AttributeHandleValueMap values;
      if (ownerProviderCount == 1U) {
        REQUIRE(callbackAttributes == AttributeHandleSet{ownerAAttribute});
        values.emplace(
            ownerAAttribute,
            VariableLengthData(providerAValue.data(), providerAValue.size()));
      } else {
        REQUIRE(ownerProviderCount == 2U);
        REQUIRE(callbackAttributes == AttributeHandleSet{ownerAAttribute, ownerBAttribute});
        values.emplace(
            ownerAAttribute,
            VariableLengthData(providerAValue.data(), providerAValue.size()));
        values.emplace(
            ownerBAttribute,
            VariableLengthData(providerBValue.data(), providerBValue.size()));
      }
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          callbackObject,
          values,
          providerATag));
      if (ownerProviderCount == 1U) {
        // The first response is admitted before the MOM switch mutation; the
        // queued second solicitation must be fenced at callback entry.
        setAutoProvide(false);
      }
    };

    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    if (callbackModel == rti1516_2025::HLA_EVOKED) {
      // The subscription queues discovery and both provider solicitations.
      // Evoke the requester first, then keep evoking the owner until its
      // first solicitation has entered user code and disabled Auto Provide.
      // Only then drain the second provider: this makes the callback-time
      // switch fence the queued solicitation instead of testing incidental
      // cross-federate queue order.
      for (int pass = 0;
           pass != 64 && requesterReports.objectDiscoveryReports.empty();
           ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
      }
      REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
      for (int pass = 0; pass != 64 && ownerProviderCount == 0U; ++pass) {
        static_cast<void>(owner->evokeCallback(0.0));
      }
      REQUIRE(ownerProviderCount == 1U);
      drain();
    } else {
      drain();
    }
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(ownerProviderCount == 1U);
    REQUIRE(secondProviderCount == 0U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(secondProviderReports.attributeValueUpdateRequestReports.empty());
    REQUIRE_FALSE(owner->getAutoProvideSwitch());
    REQUIRE_FALSE(secondProvider->getAutoProvideSwitch());
    REQUIRE_FALSE(requester->getAutoProvideSwitch());
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    auto const& firstReflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(firstReflection.objectInstance == firstObject);
    REQUIRE(firstReflection.attributeValues.size() == 1U);
    REQUIRE(firstReflection.attributeValues.contains(requesterAAttribute));
    REQUIRE(variableLengthDataBytes(
                firstReflection.attributeValues.at(requesterAAttribute)) ==
            providerAValue);
    REQUIRE(variableLengthDataBytes(firstReflection.userSuppliedTag) ==
            std::vector<unsigned char>(
                providerATagBytes,
                providerATagBytes + sizeof(providerATagBytes)));
    REQUIRE(firstReflection.transportationType ==
            requester->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(firstReflection.producingFederate == ownerHandle);
    REQUIRE(firstReflection.sentRegionsSupplied);
    REQUIRE(firstReflection.sentRegions == RegionHandleSet{ownerRegion});
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});

    REQUIRE_NOTHROW(secondProvider->unsubscribeObjectClassAttributesWithRegions(
        secondProviderObjectClass,
        secondProviderPair));
    REQUIRE_NOTHROW(setAutoProvide(true));
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(secondProvider->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());
    callbackOrder.clear();
    ownerReports.attributeValueUpdateRequestReports.clear();

    ObjectInstanceHandle const secondObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(secondObject.isValid());
    drain();
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
    REQUIRE(ownerProviderCount == 2U);
    REQUIRE(secondProviderCount == 0U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance ==
            secondObject);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes == ownerAttributes);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 2U);
    auto const& secondReflection = requesterReports.attributeReflectionReports.back();
    REQUIRE(secondReflection.objectInstance == secondObject);
    REQUIRE(secondReflection.attributeValues.size() == 2U);
    REQUIRE(secondReflection.attributeValues.contains(requesterAAttribute));
    REQUIRE(secondReflection.attributeValues.contains(requesterBAttribute));
    REQUIRE(variableLengthDataBytes(
                secondReflection.attributeValues.at(requesterAAttribute)) ==
            providerAValue);
    REQUIRE(variableLengthDataBytes(
                secondReflection.attributeValues.at(requesterBAttribute)) ==
            providerBValue);
    REQUIRE(variableLengthDataBytes(secondReflection.userSuppliedTag) ==
            std::vector<unsigned char>(
                providerATagBytes,
                providerATagBytes + sizeof(providerATagBytes)));
    REQUIRE(secondReflection.transportationType ==
            requester->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(secondReflection.producingFederate == ownerHandle);
    REQUIRE(secondReflection.sentRegionsSupplied);
    REQUIRE(secondReflection.sentRegions == RegionHandleSet{ownerRegion});
    REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(firstObject, ownerPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(secondObject, ownerPair));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
    REQUIRE_NOTHROW(secondProvider->deleteRegion(secondProviderRegion));
    REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(secondProvider->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(secondProvider->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
