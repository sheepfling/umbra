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
#error "The regional Auto Provide multi-provider test requires the Umbra source directory."
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
  return L"regional-auto-provide-multi-provider-" +
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

  struct AttributeOwnershipAssumptionReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
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
  std::vector<AttributeOwnershipAcquisitionReport>
      attributeOwnershipAcquisitionReports;
  std::vector<AttributeOwnershipAssumptionReport>
      attributeOwnershipAssumptionReports;
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
    "Embedded regional Auto Provide fans out one request across two provider owners under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ownership-management][ddm][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-multi-provider]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
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
  auto runScenario = [](CallbackModel const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador secondProviderReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto secondProvider = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml")
                               .wstring();
    std::vector<unsigned char> const divestitureTag{0xD1U, 0x4DU, 0x25U};
    std::vector<unsigned char> const acquisitionTag{0xA1U, 0x4DU, 0x25U};
    std::vector<unsigned char> const providerAValue{
        0x41U, 0x2DU, 0x52U, 0x45U, 0x47U};
    std::vector<unsigned char> const providerBValue{
        0x42U, 0x2DU, 0x52U, 0x45U, 0x47U};
    std::vector<unsigned char> const providerATag{0x54U, 0x41U};
    std::vector<unsigned char> const providerBTag{0x54U, 0x42U};
    VariableLengthData const divestitureTagData(
        divestitureTag.data(),
        divestitureTag.size());
    VariableLengthData const acquisitionTagData(
        acquisitionTag.data(),
        acquisitionTag.size());
    VariableLengthData const providerATagData(
        providerATag.data(),
        providerATag.size());
    VariableLengthData const providerBTagData(
        providerBTag.data(),
        providerBTag.size());
    FederateHandle ownerHandle;
    FederateHandle secondProviderHandle;
    std::vector<AttributeHandleSet> ownerProviderRequests;
    std::vector<AttributeHandleSet> secondProviderRequests;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(secondProvider->connect(secondProviderReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-multi-provider-owner",
        L"provider-a",
        federationName));
    REQUIRE_NOTHROW(secondProviderHandle = secondProvider->joinFederationExecution(
        L"regional-auto-provide-multi-provider-second-provider",
        L"provider-b",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-multi-provider-requester",
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
    auto const secondProviderObjectClass = secondProvider->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const secondProviderBAttribute = secondProvider->getAttributeHandle(
        secondProviderObjectClass,
        L"ProviderBValue");
    auto const secondProviderDimension = secondProvider->getDimensionHandle(
        L"UmbraRegionX");
    auto const requesterObjectClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    auto const requesterAAttribute = requester->getAttributeHandle(
        requesterObjectClass,
        L"ProviderAValue");
    auto const requesterBAttribute = requester->getAttributeHandle(
        requesterObjectClass,
        L"ProviderBValue");
    auto const requesterDimension = requester->getDimensionHandle(L"UmbraRegionX");
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
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(secondProvider->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());

    AttributeHandleSet const ownerAttributes{ownerAAttribute, ownerBAttribute};
    AttributeHandleSet const secondProviderAttributes{secondProviderBAttribute};
    AttributeHandleSet const requesterAttributes{
        requesterAAttribute,
        requesterBAttribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerObjectClass,
        ownerAttributes));
    REQUIRE_NOTHROW(secondProvider->publishObjectClassAttributes(
        secondProviderObjectClass,
        secondProviderAttributes));

    auto const ownerRegion = owner->createRegion(
        DimensionHandleSet{ownerDimension});
    auto const secondProviderRegion = secondProvider->createRegion(
        DimensionHandleSet{secondProviderDimension});
    auto const requesterRegion = requester->createRegion(
        DimensionHandleSet{requesterDimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(
        ownerRegion,
        ownerDimension,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(secondProvider->setRangeBounds(
        secondProviderRegion,
        secondProviderDimension,
        RangeBounds(0UL, 10UL)));
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(
        RegionHandleSet{ownerRegion}));
    REQUIRE_NOTHROW(secondProvider->commitRegionModifications(
        RegionHandleSet{secondProviderRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));

    AttributeHandleSetRegionHandleSetPairVector const ownerPair{
        {ownerAttributes, RegionHandleSet{ownerRegion}}};
    AttributeHandleSetRegionHandleSetPairVector const secondProviderPair{
        {secondProviderAttributes, RegionHandleSet{secondProviderRegion}}};
    AttributeHandleSetRegionHandleSetPairVector const requesterPair{
        {requesterAttributes, RegionHandleSet{requesterRegion}}};
    AttributeHandleSetRegionHandleSetPairVector const ownerARegionPair{
        {AttributeHandleSet{ownerAAttribute}, RegionHandleSet{ownerRegion}}};
    REQUIRE_NOTHROW(secondProvider->subscribeObjectClassAttributesWithRegions(
        secondProviderObjectClass,
        secondProviderPair));
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    ownerReports.provideAttributeValueUpdateHandler =
        [&ownerProviderRequests](ObjectInstanceHandle const&,
                                 AttributeHandleSet const& callbackAttributes,
                                 VariableLengthData const&) {
          ownerProviderRequests.push_back(callbackAttributes);
        };

    auto drain = [&]() {
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        return;
      }
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
        static_cast<void>(secondProvider->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
      }
    };

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        ownerPair));
    REQUIRE(objectInstance.isValid());
    drain();
    REQUIRE(secondProviderReports.objectDiscoveryReports.size() == 1U);
    // The second provider's regional subscription is already eligible while
    // owner A still owns both attributes. Consume that initial solicitation;
    // the assertions below concern only the post-transfer two-owner fan-out.
    ownerReports.attributeValueUpdateRequestReports.clear();
    ownerProviderRequests.clear();

    REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
        objectInstance,
        AttributeHandleSet{ownerBAttribute},
        divestitureTagData));
    REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, ownerAAttribute));
    REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, ownerBAttribute));
    drain();
    REQUIRE(secondProviderReports.attributeOwnershipAssumptionReports.size() == 1U);
    auto const& assumption =
        secondProviderReports.attributeOwnershipAssumptionReports.front();
    REQUIRE(assumption.objectInstance == objectInstance);
    REQUIRE(assumption.attributes == AttributeHandleSet{secondProviderBAttribute});
    REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) == divestitureTag);

    REQUIRE_NOTHROW(secondProvider->attributeOwnershipAcquisitionIfAvailable(
        objectInstance,
        AttributeHandleSet{secondProviderBAttribute},
        acquisitionTagData));
    drain();
    REQUIRE(secondProviderReports.attributeOwnershipAcquisitionReports.size() == 1U);
    auto const& acquisition =
        secondProviderReports.attributeOwnershipAcquisitionReports.front();
    REQUIRE(acquisition.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
    REQUIRE(acquisition.objectInstance == objectInstance);
    REQUIRE(acquisition.attributes == AttributeHandleSet{secondProviderBAttribute});
    REQUIRE(variableLengthDataBytes(acquisition.userSuppliedTag) == acquisitionTag);
    REQUIRE(secondProvider->isAttributeOwnedByFederate(
        objectInstance,
        secondProviderBAttribute));

    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &ownerProviderRequests,
        ownerAAttribute,
        providerAValue,
        providerATagData](ObjectInstanceHandle const& callbackObject,
                          AttributeHandleSet const& callbackAttributes,
                          VariableLengthData const& callbackTag) {
      ownerProviderRequests.push_back(callbackAttributes);
      REQUIRE(callbackAttributes == AttributeHandleSet{ownerAAttribute});
      REQUIRE(variableLengthDataBytes(callbackTag).empty());
      AttributeHandleValueMap values;
      values.emplace(
          ownerAAttribute,
          VariableLengthData(providerAValue.data(), providerAValue.size()));
      owner->updateAttributeValues(callbackObject, values, providerATagData);
    };
    secondProviderReports.provideAttributeValueUpdateHandler = [
        &secondProvider,
        &secondProviderRequests,
        secondProviderBAttribute,
        providerBValue,
        providerBTagData](ObjectInstanceHandle const& callbackObject,
                          AttributeHandleSet const& callbackAttributes,
                          VariableLengthData const& callbackTag) {
      secondProviderRequests.push_back(callbackAttributes);
      REQUIRE(callbackAttributes ==
              AttributeHandleSet{secondProviderBAttribute});
      REQUIRE(variableLengthDataBytes(callbackTag).empty());
      AttributeHandleValueMap values;
      values.emplace(
          secondProviderBAttribute,
          VariableLengthData(providerBValue.data(), providerBValue.size()));
      secondProvider->updateAttributeValues(callbackObject, values, providerBTagData);
    };

    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    drain();
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(ownerProviderRequests.size() == 1U);
    REQUIRE(secondProviderRequests.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(secondProviderReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().objectInstance ==
            objectInstance);
    REQUIRE(secondProviderReports.attributeValueUpdateRequestReports.front().objectInstance ==
            objectInstance);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.front().attributes ==
            AttributeHandleSet{ownerAAttribute});
    REQUIRE(secondProviderReports.attributeValueUpdateRequestReports.front().attributes ==
            AttributeHandleSet{secondProviderBAttribute});
    REQUIRE(variableLengthDataBytes(
                ownerReports.attributeValueUpdateRequestReports.front().userSuppliedTag)
                .empty());
    REQUIRE(variableLengthDataBytes(
                secondProviderReports.attributeValueUpdateRequestReports.front().userSuppliedTag)
                .empty());

    REQUIRE(requesterReports.attributeReflectionReports.size() == 2U);
    bool sawOwnerAReflection = false;
    bool sawSecondProviderBReflection = false;
    for (auto const& reflection : requesterReports.attributeReflectionReports) {
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1U);
      REQUIRE(reflection.transportationType ==
              requester->getTransportationTypeHandle(L"HLAreliable"));
      if (reflection.producingFederate == ownerHandle) {
        REQUIRE_FALSE(sawOwnerAReflection);
        sawOwnerAReflection = true;
        REQUIRE(reflection.attributeValues.contains(requesterAAttribute));
        REQUIRE(variableLengthDataBytes(
                    reflection.attributeValues.at(requesterAAttribute)) ==
                providerAValue);
        REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == providerATag);
        REQUIRE(reflection.sentRegionsSupplied);
        REQUIRE(reflection.sentRegions == RegionHandleSet{ownerRegion});
      } else if (reflection.producingFederate == secondProviderHandle) {
        REQUIRE_FALSE(sawSecondProviderBReflection);
        sawSecondProviderBReflection = true;
        REQUIRE(reflection.attributeValues.contains(requesterBAttribute));
        REQUIRE(variableLengthDataBytes(
                    reflection.attributeValues.at(requesterBAttribute)) ==
                providerBValue);
        REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == providerBTag);
        // Divestiture clears the old source association. The new owner is
        // therefore delivered with the default source projection until it
        // explicitly associates a replacement update region.
        REQUIRE(reflection.sentRegionsSupplied);
        REQUIRE(reflection.sentRegions.empty());
      } else {
        FAIL("regional Auto Provide reflection came from an unexpected federate");
      }
    }
    REQUIRE(sawOwnerAReflection);
    REQUIRE(sawSecondProviderBReflection);

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(secondProvider->unsubscribeObjectClassAttributesWithRegions(
        secondProviderObjectClass,
        secondProviderPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
        objectInstance,
        ownerARegionPair));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
    REQUIRE_NOTHROW(secondProvider->deleteRegion(secondProviderRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::NO_ACTION));
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
