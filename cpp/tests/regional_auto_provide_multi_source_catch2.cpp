#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional Auto Provide multi-source test requires the Umbra source directory."
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
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
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
  return L"regional-auto-provide-multi-source-" +
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

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
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
    "Embedded regional Auto Provide separates independent source regions and provider callbacks under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-multi-source-region]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.update-attribute-values]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]"
    "[federate.callback.reflect-attribute-values]") {
  auto runScenario = [](rti1516_2025::CallbackModel const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador flavorReports;
    ReportingFederateAmbassador organicReports;
    auto owner = makeRti();
    auto flavor = makeRti();
    auto organic = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
    unsigned char const flavorValueBytes[] = {0x4D, 0x53, 0x41};
    unsigned char const organicValueBytes[] = {0x4D, 0x53, 0x42};
    unsigned char const flavorTagBytes[] = {0x54, 0x41};
    unsigned char const organicTagBytes[] = {0x54, 0x42};
    std::vector<unsigned char> const flavorValue(
        flavorValueBytes,
        flavorValueBytes + sizeof(flavorValueBytes));
    std::vector<unsigned char> const organicValue(
        organicValueBytes,
        organicValueBytes + sizeof(organicValueBytes));
    VariableLengthData const flavorTag(flavorTagBytes, sizeof(flavorTagBytes));
    VariableLengthData const organicTag(organicTagBytes, sizeof(organicTagBytes));
    FederateHandle ownerHandle;
    std::vector<AttributeHandleSet> providerRequests;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(flavor->connect(flavorReports, callbackModel));
    REQUIRE_NOTHROW(organic->connect(organicReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-multi-source-owner", L"provider", federationName));
    REQUIRE_NOTHROW(flavor->joinFederationExecution(
        L"regional-auto-provide-multi-source-flavor", L"subscriber", federationName));
    REQUIRE_NOTHROW(organic->joinFederationExecution(
        L"regional-auto-provide-multi-source-organic", L"subscriber", federationName));

    auto const ownerObjectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const ownerAAttribute = owner->getAttributeHandle(
        ownerObjectClass,
        L"ProviderAValue");
    auto const ownerBAttribute = owner->getAttributeHandle(
        ownerObjectClass,
        L"ProviderBValue");
    auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
    auto const flavorObjectClass = flavor->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const flavorAttribute = flavor->getAttributeHandle(
        flavorObjectClass,
        L"ProviderAValue");
    auto const flavorDimension = flavor->getDimensionHandle(L"UmbraRegionX");
    auto const organicObjectClass = organic->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const organicAttribute = organic->getAttributeHandle(
        organicObjectClass,
        L"ProviderBValue");
    auto const organicDimension = organic->getDimensionHandle(L"UmbraRegionX");
    REQUIRE(ownerObjectClass.isValid());
    REQUIRE(ownerAAttribute.isValid());
    REQUIRE(ownerBAttribute.isValid());
    REQUIRE(ownerDimension.isValid());
    REQUIRE(flavorObjectClass.isValid());
    REQUIRE(flavorAttribute.isValid());
    REQUIRE(flavorDimension.isValid());
    REQUIRE(organicObjectClass.isValid());
    REQUIRE(organicAttribute.isValid());
    REQUIRE(organicDimension.isValid());
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(flavor->getAutoProvideSwitch());
    REQUIRE(organic->getAutoProvideSwitch());

    AttributeHandleSet const ownerAttributes{ownerAAttribute, ownerBAttribute};
    AttributeHandleSet const flavorAttributes{flavorAttribute};
    AttributeHandleSet const organicAttributes{organicAttribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerObjectClass,
        ownerAttributes));

    auto const ownerFlavorRegion = owner->createRegion(
        DimensionHandleSet{ownerDimension});
    auto const ownerOrganicRegion = owner->createRegion(
        DimensionHandleSet{ownerDimension});
    auto const flavorRegion = flavor->createRegion(
        DimensionHandleSet{flavorDimension});
    auto const organicRegion = organic->createRegion(
        DimensionHandleSet{organicDimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(
        ownerFlavorRegion,
        ownerDimension,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(owner->setRangeBounds(
        ownerOrganicRegion,
        ownerDimension,
        RangeBounds(5UL, 7UL)));
    REQUIRE_NOTHROW(flavor->setRangeBounds(
        flavorRegion,
        flavorDimension,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(organic->setRangeBounds(
        organicRegion,
        organicDimension,
        RangeBounds(6UL, 8UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(
        RegionHandleSet{ownerFlavorRegion, ownerOrganicRegion}));
    REQUIRE_NOTHROW(flavor->commitRegionModifications(RegionHandleSet{flavorRegion}));
    REQUIRE_NOTHROW(organic->commitRegionModifications(RegionHandleSet{organicRegion}));

    AttributeHandleSetRegionHandleSetPairVector const ownerPairs{
        {AttributeHandleSet{ownerAAttribute}, RegionHandleSet{ownerFlavorRegion}},
        {AttributeHandleSet{ownerBAttribute}, RegionHandleSet{ownerOrganicRegion}}};
    AttributeHandleSetRegionHandleSetPairVector const flavorPair{{
        flavorAttributes,
        RegionHandleSet{flavorRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const organicPair{{
        organicAttributes,
        RegionHandleSet{organicRegion},
    }};
    REQUIRE_NOTHROW(flavor->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(organic->setConveyRegionDesignatorSetsSwitch(true));

    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &providerRequests,
        ownerAAttribute,
        ownerBAttribute,
        flavorValue,
        organicValue,
        flavorTag,
        organicTag](ObjectInstanceHandle const& callbackObject,
                    AttributeHandleSet const& callbackAttributes,
                    VariableLengthData const&) {
      providerRequests.push_back(callbackAttributes);
      AttributeHandleValueMap values;
      VariableLengthData responseTag;
      if (callbackAttributes.contains(ownerAAttribute)) {
        values.emplace(
            ownerAAttribute,
            VariableLengthData(flavorValue.data(), flavorValue.size()));
        responseTag = flavorTag;
      }
      if (callbackAttributes.contains(ownerBAttribute)) {
        values.emplace(
            ownerBAttribute,
            VariableLengthData(organicValue.data(), organicValue.size()));
        responseTag = organicTag;
      }
      if (!values.empty()) {
        REQUIRE_NOTHROW(owner->updateAttributeValues(
            callbackObject,
            values,
            responseTag));
      }
    };

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        ownerPairs));
    REQUIRE(objectInstance.isValid());

    auto drain = [&]() {
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        return;
      }
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(flavor->evokeCallback(0.0));
        static_cast<void>(organic->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
      }
    };

    REQUIRE_NOTHROW(flavor->subscribeObjectClassAttributesWithRegions(
        flavorObjectClass,
        flavorPair));
    REQUIRE_NOTHROW(organic->subscribeObjectClassAttributesWithRegions(
        organicObjectClass,
        organicPair));
    drain();

    REQUIRE(flavorReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(organicReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(providerRequests.size() == 2U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 2U);
    REQUIRE(flavorReports.attributeReflectionReports.size() == 1U);
    REQUIRE(organicReports.attributeReflectionReports.size() == 1U);

    bool sawFlavorRequest = false;
    bool sawOrganicRequest = false;
    for (auto const& request : providerRequests) {
      sawFlavorRequest = sawFlavorRequest ||
          request == AttributeHandleSet{ownerAAttribute};
      sawOrganicRequest = sawOrganicRequest ||
          request == AttributeHandleSet{ownerBAttribute};
    }
    REQUIRE(sawFlavorRequest);
    REQUIRE(sawOrganicRequest);

    auto const& flavorReflection = flavorReports.attributeReflectionReports.front();
    REQUIRE(flavorReflection.objectInstance == objectInstance);
    REQUIRE(flavorReflection.attributeValues.size() == 1U);
    REQUIRE(flavorReflection.attributeValues.contains(flavorAttribute));
    REQUIRE(variableLengthDataBytes(
                flavorReflection.attributeValues.at(flavorAttribute)) ==
            flavorValue);
    REQUIRE(variableLengthDataBytes(flavorReflection.userSuppliedTag) ==
            std::vector<unsigned char>{flavorTagBytes,
                                       flavorTagBytes + sizeof(flavorTagBytes)});
    REQUIRE(flavorReflection.sentRegionsSupplied);
    REQUIRE(flavorReflection.sentRegions == RegionHandleSet{ownerFlavorRegion});
    REQUIRE(flavorReflection.producingFederate == ownerHandle);

    auto const& organicReflection = organicReports.attributeReflectionReports.front();
    REQUIRE(organicReflection.objectInstance == objectInstance);
    REQUIRE(organicReflection.attributeValues.size() == 1U);
    REQUIRE(organicReflection.attributeValues.contains(organicAttribute));
    REQUIRE(variableLengthDataBytes(
                organicReflection.attributeValues.at(organicAttribute)) ==
            organicValue);
    REQUIRE(variableLengthDataBytes(organicReflection.userSuppliedTag) ==
            std::vector<unsigned char>{organicTagBytes,
                                       organicTagBytes + sizeof(organicTagBytes)});
    REQUIRE(organicReflection.sentRegionsSupplied);
    REQUIRE(organicReflection.sentRegions == RegionHandleSet{ownerOrganicRegion});
    REQUIRE(organicReflection.producingFederate == ownerHandle);

    REQUIRE_NOTHROW(flavor->unsubscribeObjectClassAttributesWithRegions(
        flavorObjectClass,
        flavorPair));
    REQUIRE_NOTHROW(organic->unsubscribeObjectClassAttributesWithRegions(
        organicObjectClass,
        organicPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPairs));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerFlavorRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerOrganicRegion));
    REQUIRE_NOTHROW(flavor->deleteRegion(flavorRegion));
    REQUIRE_NOTHROW(organic->deleteRegion(organicRegion));
    REQUIRE_NOTHROW(flavor->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(organic->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(flavor->disconnect());
    REQUIRE_NOTHROW(organic->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
