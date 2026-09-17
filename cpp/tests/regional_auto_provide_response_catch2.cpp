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
#error "The regional Auto Provide response test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::OrderType;
using rti1516_2025::RECEIVE;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;

std::vector<unsigned char> variableLengthDataBytes(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

std::wstring nextRelaxedDdmFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-auto-provide-relaxed-ddm-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path relaxedDdmResourcePath(
    std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "cpp" / "tests" / "data" / relativePath;
}

class RelaxedDdmReportingFederateAmbassador final
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

std::unique_ptr<RTIambassador> makeRelaxedDdmRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

TEST_CASE(
    "Embedded regional Auto Provide applies Allow Relaxed DDM to touching source projections and suppresses positive gaps under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][allow-relaxed-ddm][strict-relaxed-ddm-boundary][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-relaxed-ddm]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-allow-relaxed-ddm-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
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
  auto runScenario = [](auto const callbackModel,
                        bool const relaxedDdmEnabled) {
    RelaxedDdmReportingFederateAmbassador ownerReports;
    RelaxedDdmReportingFederateAmbassador requesterReports;
    auto owner = makeRelaxedDdmRti();
    auto requester = makeRelaxedDdmRti();
    auto const federationName = nextRelaxedDdmFederationName();
    std::vector<std::wstring> fomModules{
        relaxedDdmResourcePath("regional-ownership-fanout-fom.xml").wstring()};
    if (relaxedDdmEnabled) {
      fomModules.push_back(relaxedDdmResourcePath("allow-relaxed-ddm-enabled-fom.xml")
                               .wstring());
    }

    std::vector<unsigned char> const providedValue{0x52U, 0x44U, 0x44U, 0x4DU};
    std::vector<unsigned char> const providedTag{0x41U, 0x50U};
    std::vector<unsigned char> const strictValue{0x53U, 0x54U, 0x52U};
    VariableLengthData const responseTag(providedTag.data(), providedTag.size());
    FederateHandle ownerHandle;
    std::vector<AttributeHandleSet> providerRequestAttributes;
    std::size_t providerCallbackCount = 0U;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-relaxed-ddm-owner",
        L"provider",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-relaxed-ddm-requester",
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
    REQUIRE(ownerObjectClass.isValid());
    REQUIRE(ownerAttribute.isValid());
    REQUIRE(ownerDimension.isValid());
    REQUIRE(requesterObjectClass.isValid());
    REQUIRE(requesterAttribute.isValid());
    REQUIRE(requesterDimension.isValid());
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());
    REQUIRE(owner->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);
    REQUIRE(requester->getAllowRelaxedDDMSwitch() == relaxedDdmEnabled);

    AttributeHandleSet const ownerAttributes{ownerAttribute};
    AttributeHandleSet const requesterAttributes{requesterAttribute};
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        ownerObjectClass,
        ownerAttributes));

    auto const sourceRegion = owner->createRegion(
        DimensionHandleSet{ownerDimension});
    auto const requesterRegion = requester->createRegion(
        DimensionHandleSet{requesterDimension});
    REQUIRE_NOTHROW(owner->setRangeBounds(
        sourceRegion,
        ownerDimension,
        RangeBounds(0UL, 2UL)));
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(2UL, 3UL)));
    REQUIRE_NOTHROW(owner->commitRegionModifications(RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));
    AttributeHandleSetRegionHandleSetPairVector const sourcePair{{
        ownerAttributes,
        RegionHandleSet{sourceRegion},
    }};
    AttributeHandleSetRegionHandleSetPairVector const requesterPair{{
        requesterAttributes,
        RegionHandleSet{requesterRegion},
    }};
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    ownerReports.provideAttributeValueUpdateHandler = [
        &owner,
        &providerRequestAttributes,
        &providerCallbackCount,
        providedValue,
        responseTag,
        ownerAttribute](
        ObjectInstanceHandle const callbackObject,
        AttributeHandleSet const& callbackAttributes,
        VariableLengthData const& callbackTag) {
      providerRequestAttributes.push_back(callbackAttributes);
      ++providerCallbackCount;
      REQUIRE(variableLengthDataBytes(callbackTag).empty());
      REQUIRE(callbackAttributes == AttributeHandleSet{ownerAttribute});
      AttributeHandleValueMap values;
      values.emplace(
          ownerAttribute,
          VariableLengthData(providedValue.data(), providedValue.size()));
      owner->updateAttributeValues(callbackObject, values, responseTag);
    };

    ObjectInstanceHandle objectInstance;
    REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
        ownerObjectClass,
        sourcePair));
    REQUIRE(objectInstance.isValid());

    auto drain = [&]() {
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        return;
      }
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
      }
    };

    // Exact touching is admitted only by the enabled Relaxed DDM policy.
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    drain();
    std::size_t expectedDiscoveries = relaxedDdmEnabled ? 1U : 0U;
    std::size_t expectedReflections = relaxedDdmEnabled ? 1U : 0U;
    REQUIRE(requesterReports.objectDiscoveryReports.size() == expectedDiscoveries);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() ==
            (relaxedDdmEnabled ? 1U : 0U));
    REQUIRE(providerRequestAttributes.size() == (relaxedDdmEnabled ? 1U : 0U));
    REQUIRE(providerCallbackCount == (relaxedDdmEnabled ? 1U : 0U));
    REQUIRE(requesterReports.attributeReflectionReports.size() == expectedReflections);
    if (relaxedDdmEnabled) {
      auto const& reflection = requesterReports.attributeReflectionReports.front();
      REQUIRE(reflection.objectInstance == objectInstance);
      REQUIRE(reflection.attributeValues.size() == 1U);
      REQUIRE(reflection.attributeValues.contains(requesterAttribute));
      REQUIRE(variableLengthDataBytes(
                  reflection.attributeValues.at(requesterAttribute)) ==
              providedValue);
      REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == providedTag);
      REQUIRE(reflection.sentRegionsSupplied);
      REQUIRE(reflection.sentRegions == RegionHandleSet{sourceRegion});
      REQUIRE(reflection.producingFederate == ownerHandle);
    }

    // A positive gap must suppress the regional route even after a relaxed
    // discovery has made the object known to the requester.
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(3UL, 4UL)));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));
    std::vector<unsigned char> const gapValue{0x47U, 0x41U, 0x50U};
    AttributeHandleValueMap gapValues;
    gapValues.emplace(
        ownerAttribute,
        VariableLengthData(gapValue.data(), gapValue.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        objectInstance,
        gapValues,
        VariableLengthData()));
    drain();
    REQUIRE(requesterReports.attributeReflectionReports.size() == expectedReflections);

    // Restore a strict overlap. A disabled execution discovers the existing
    // instance now; an enabled execution keeps its original discovery and
    // must not solicit a second provider response.
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(1UL, 3UL)));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    drain();
    if (!relaxedDdmEnabled) {
      ++expectedDiscoveries;
      ++expectedReflections;
    }
    REQUIRE(requesterReports.objectDiscoveryReports.size() == expectedDiscoveries);
    REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
    REQUIRE(providerRequestAttributes.size() == 1U);
    REQUIRE(providerCallbackCount == 1U);
    REQUIRE(requesterReports.attributeReflectionReports.size() == expectedReflections);

    // Strict overlap remains a normal receive-order regional update after the
    // relaxed-only gap was rejected.
    AttributeHandleValueMap strictValues;
    strictValues.emplace(
        ownerAttribute,
        VariableLengthData(strictValue.data(), strictValue.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        objectInstance,
        strictValues,
        VariableLengthData()));
    drain();
    ++expectedReflections;
    REQUIRE(requesterReports.attributeReflectionReports.size() == expectedReflections);
    REQUIRE(requesterReports.attributeReflectionReports.back().sentRegionsSupplied);
    REQUIRE(requesterReports.attributeReflectionReports.back().sentRegions ==
            RegionHandleSet{sourceRegion});

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, sourcePair));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
    REQUIRE_NOTHROW(owner->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  };

  SECTION("HLA_EVOKED with Relaxed DDM enabled") {
    runScenario(HLA_EVOKED, true);
  }
  SECTION("HLA_EVOKED with Relaxed DDM disabled") {
    runScenario(HLA_EVOKED, false);
  }
  SECTION("HLA_IMMEDIATE with Relaxed DDM enabled") {
    runScenario(rti1516_2025::HLA_IMMEDIATE, true);
  }
  SECTION("HLA_IMMEDIATE with Relaxed DDM disabled") {
    runScenario(rti1516_2025::HLA_IMMEDIATE, false);
  }
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-auto-provide-response-" +
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
    if (onAttributeReflection) {
      onAttributeReflection();
    }
  }

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::vector<AttributeReflectionReport> attributeReflectionReports;
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
    "Embedded regional Auto Provide provider response reflects one scoped value under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-response]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
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
  auto runScenario = [](auto const callbackModel) {
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto const federationName = nextFederationName();
    auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
    std::vector<std::string> callbackOrder;
    std::vector<unsigned char> const responseValue{0x52U, 0x45U, 0x53U};
    std::vector<unsigned char> const responseTag{0x41U, 0x50U, 0x52U};
    std::vector<unsigned char> observedRequestTag;
    FederateHandle ownerHandle;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModule,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
        L"regional-auto-provide-response-owner", L"provider", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-response-requester", L"subscriber", federationName));

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

    ownerReports.provideAttributeValueUpdateHandler = [
        &callbackOrder,
        &observedRequestTag,
        &owner,
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
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          callbackObject,
          values,
          VariableLengthData(responseTag.data(), responseTag.size())));
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
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);
    REQUIRE(callbackOrder ==
            std::vector<std::string>{"discover", "provide", "reflect"});
    auto const& request = ownerReports.attributeValueUpdateRequestReports.front();
    REQUIRE(request.objectInstance == objectInstance);
    REQUIRE(request.attributes == ownerAttributes);
    REQUIRE(observedRequestTag.empty());

    auto const& reflection = requesterReports.attributeReflectionReports.front();
    REQUIRE(reflection.objectInstance == objectInstance);
    REQUIRE(reflection.attributeValues.size() == 1U);
    REQUIRE(reflection.attributeValues.contains(requesterAttribute));
    REQUIRE(variableLengthDataBytes(
                reflection.attributeValues.at(requesterAttribute)) ==
            responseValue);
    REQUIRE(variableLengthDataBytes(reflection.userSuppliedTag) == responseTag);
    REQUIRE(reflection.transportationType ==
            owner->getTransportationTypeHandle(L"HLAreliable"));
    REQUIRE(reflection.producingFederate == ownerHandle);
    REQUIRE(reflection.sentRegionsSupplied);
    REQUIRE(reflection.sentRegions == RegionHandleSet{ownerRegion});

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
