#include <catch2/catch_test_macros.hpp>

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The ownership/update-region test requires the Umbra source directory."
#endif

namespace {

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RegionHandle;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::TransportationTypeHandle;
using rti1516_2025::VariableLengthData;

class OwnershipRegionAmbassador final : public NullFederateAmbassador {
 public:
  struct Reflection final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleValueMap attributeValues;
    VariableLengthData userSuppliedTag;
    TransportationTypeHandle transportationType;
    FederateHandle producingFederate;
    bool sentRegionsSupplied = false;
    RegionHandleSet sentRegions;
  };

  struct Acquisition final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      FederateHandle const& producingFederate) override {
    static_cast<void>(objectClass);
    static_cast<void>(objectInstanceName);
    static_cast<void>(producingFederate);
    discoveries.push_back(objectInstance);
  }

  void attributeOwnershipAcquisitionNotification(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& securedAttributes,
      VariableLengthData const& userSuppliedTag) override {
    acquisitions.push_back({objectInstance, securedAttributes, userSuppliedTag});
  }

  void reflectAttributeValues(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleValueMap const& attributeValues,
      VariableLengthData const& userSuppliedTag,
      TransportationTypeHandle const& transportationType,
      FederateHandle const& producingFederate,
      RegionHandleSet const* optionalSentRegions) override {
    reflections.push_back({
        objectInstance,
        attributeValues,
        userSuppliedTag,
        transportationType,
        producingFederate,
        optionalSentRegions != nullptr,
        optionalSentRegions == nullptr ? RegionHandleSet{} : *optionalSentRegions,
    });
  }

  std::vector<ObjectInstanceHandle> discoveries;
  std::vector<Acquisition> acquisitions;
  std::vector<Reflection> reflections;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
      relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"ownership-transfer-update-region-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

void drain(
    RTIambassador& first,
    RTIambassador& second,
    RTIambassador& third) {
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(first.evokeCallback(0.0));
    static_cast<void>(second.evokeCallback(0.0));
    static_cast<void>(third.evokeCallback(0.0));
  }
}

void setRegionBounds(
    RTIambassador& rti,
    RegionHandle const& region,
    rti1516_2025::DimensionHandle const& dimension,
    unsigned long lower,
    unsigned long upper) {
  REQUIRE_NOTHROW(rti.setRangeBounds(region, dimension, rti1516_2025::RangeBounds(lower, upper)));
  REQUIRE_NOTHROW(rti.commitRegionModifications(RegionHandleSet{region}));
}

std::vector<unsigned char> bytes(VariableLengthData const& value) {
  auto const* data = static_cast<unsigned char const*>(value.data());
  if (data == nullptr || value.size() == 0U) {
    return {};
  }
  return {data, data + value.size()};
}

}  // namespace

TEST_CASE(
    "Embedded ownership transfer clears the former owner's 2025 update-region association",
    "[integration][development-profile][federation-management][object-management][ddm][ownership-management]"
    "[ownership-transfer-update-region][ownership-transfer-update-region-association][ownership-transfer-update-region-reacquisition][callback-evoked]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values]") {
  OwnershipRegionAmbassador ownerReports;
  OwnershipRegionAmbassador newOwnerReports;
  OwnershipRegionAmbassador receiverReports;
  auto owner = makeRti();
  auto newOwner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
  unsigned char const acquisitionTagBytes[] = {0x41, 0x43, 0x51, 0x25};
  unsigned char const divestitureTagBytes[] = {0x44, 0x49, 0x56, 0x25};
  unsigned char const ownerValueBytes[] = {0x4F, 0x4C, 0x44, 0x25};
  unsigned char const defaultValueBytes[] = {0x44, 0x45, 0x46, 0x25};
  unsigned char const replacementValueBytes[] = {0x4E, 0x45, 0x57, 0x25};
  unsigned char const reacquisitionTagBytes[] = {0x52, 0x41, 0x43, 0x25};
  unsigned char const reacquiredValueBytes[] = {0x52, 0x45, 0x41, 0x25};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(newOwner->connect(newOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ownership-region-owner",
      L"provider-a",
      federationName));
  REQUIRE_NOTHROW(newOwner->joinFederationExecution(
      L"ownership-region-new-owner",
      L"provider-b",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"ownership-region-receiver",
      L"subscriber",
      federationName));

  auto const ownerObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerObjectClass,
      L"ProviderBValue");
  auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
  auto const newOwnerObjectClass = newOwner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const newOwnerAttribute = newOwner->getAttributeHandle(
      newOwnerObjectClass,
      L"ProviderBValue");
  auto const newOwnerDimension = newOwner->getDimensionHandle(L"UmbraRegionX");
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverObjectClass,
      L"ProviderBValue");
  auto const receiverDimension = receiver->getDimensionHandle(L"UmbraRegionX");
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(ownerDimension.isValid());
  REQUIRE(newOwnerObjectClass.isValid());
  REQUIRE(newOwnerAttribute.isValid());
  REQUIRE(newOwnerDimension.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverDimension.isValid());

  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const newOwnerAttributes{newOwnerAttribute};
  AttributeHandleSet const receiverAttributes{receiverAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes));
  REQUIRE_NOTHROW(newOwner->publishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{ownerDimension});
  auto const newOwnerRegion = newOwner->createRegion(DimensionHandleSet{newOwnerDimension});
  auto const receiverOwnerRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  auto const receiverReplacementRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  setRegionBounds(*owner, ownerRegion, ownerDimension, 0UL, 2UL);
  setRegionBounds(*newOwner, newOwnerRegion, newOwnerDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverOwnerRegion, receiverDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverReplacementRegion, receiverDimension, 5UL, 7UL);

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const newOwnerPair{{
      newOwnerAttributes,
      RegionHandleSet{newOwnerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      RegionHandleSet{receiverOwnerRegion, receiverReplacementRegion},
  }};
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(newOwner->subscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      newOwnerPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(objectInstance.isValid());
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});
  REQUIRE(receiverReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});
  REQUIRE(receiverReports.reflections.empty());

  AttributeHandleValueMap ownerValues;
  ownerValues.emplace(
      ownerAttribute,
      VariableLengthData(ownerValueBytes, sizeof(ownerValueBytes)));
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      ownerValues,
      divestitureTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& ownerReflection = receiverReports.reflections.back();
  REQUIRE(ownerReflection.objectInstance == objectInstance);
  REQUIRE(ownerReflection.attributeValues.size() == 1U);
  REQUIRE(ownerReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(bytes(ownerReflection.attributeValues.at(receiverAttribute)) ==
          std::vector<unsigned char>(
              ownerValueBytes,
              ownerValueBytes + sizeof(ownerValueBytes)));
  REQUIRE(ownerReflection.sentRegionsSupplied);
  REQUIRE(ownerReflection.sentRegions == RegionHandleSet{ownerRegion});
  receiverReports.reflections.clear();

  // The If Available request is pending while the former owner still owns the
  // attribute.  Divestiture If Wanted then transfers the attribute and must
  // return the offered set exactly once.
  REQUIRE_NOTHROW(newOwner->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      newOwnerAttributes,
      acquisitionTag));
  REQUIRE(newOwnerReports.acquisitions.empty());
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownerAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownerAttributes);
  REQUIRE_FALSE(owner->isAttributeOwnedByFederate(objectInstance, ownerAttribute));
  REQUIRE(newOwner->isAttributeOwnedByFederate(objectInstance, newOwnerAttribute));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.acquisitions.size() == 1U);
  auto const& acquisition = newOwnerReports.acquisitions.front();
  REQUIRE(acquisition.objectInstance == objectInstance);
  REQUIRE(acquisition.attributes == newOwnerAttributes);
  REQUIRE(bytes(acquisition.userSuppliedTag) ==
          std::vector<unsigned char>(
              divestitureTagBytes,
              divestitureTagBytes + sizeof(divestitureTagBytes)));

  // The former source association no longer belongs to the new owner.  The
  // first post-transfer update therefore uses the default source realization.
  AttributeHandleValueMap defaultValues;
  defaultValues.emplace(
      newOwnerAttribute,
      VariableLengthData(defaultValueBytes, sizeof(defaultValueBytes)));
  REQUIRE_THROWS_AS(
      owner->updateAttributeValues(objectInstance, ownerValues, divestitureTag),
      rti1516_2025::AttributeNotOwned);
  REQUIRE_NOTHROW(newOwner->updateAttributeValues(
      objectInstance,
      defaultValues,
      divestitureTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& defaultReflection = receiverReports.reflections.back();
  REQUIRE(defaultReflection.objectInstance == objectInstance);
  REQUIRE(defaultReflection.attributeValues.size() == 1U);
  REQUIRE(defaultReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(bytes(defaultReflection.attributeValues.at(receiverAttribute)) ==
          std::vector<unsigned char>(
              defaultValueBytes,
              defaultValueBytes + sizeof(defaultValueBytes)));
  REQUIRE(defaultReflection.producingFederate != FederateHandle{});
  REQUIRE(defaultReflection.sentRegionsSupplied);
  REQUIRE(defaultReflection.sentRegions.empty());
  receiverReports.reflections.clear();

  // The new owner can establish a replacement source realization without
  // reviving the former owner's region identity.
  setRegionBounds(*newOwner, newOwnerRegion, newOwnerDimension, 5UL, 7UL);
  REQUIRE_NOTHROW(newOwner->associateRegionsForUpdates(
      objectInstance,
      newOwnerPair));
  AttributeHandleValueMap replacementValues;
  replacementValues.emplace(
      newOwnerAttribute,
      VariableLengthData(replacementValueBytes, sizeof(replacementValueBytes)));
  REQUIRE_NOTHROW(newOwner->updateAttributeValues(
      objectInstance,
      replacementValues,
      acquisitionTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& replacementReflection = receiverReports.reflections.back();
  REQUIRE(replacementReflection.objectInstance == objectInstance);
  REQUIRE(replacementReflection.attributeValues.size() == 1U);
  REQUIRE(replacementReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(bytes(replacementReflection.attributeValues.at(receiverAttribute)) ==
          std::vector<unsigned char>(
              replacementValueBytes,
              replacementValueBytes + sizeof(replacementValueBytes)));
  REQUIRE(replacementReflection.sentRegionsSupplied);
  REQUIRE(replacementReflection.sentRegions == RegionHandleSet{newOwnerRegion});

  // The original owner must be able to reacquire the attribute, but its lost
  // association is not restored implicitly.  Recreating the association is
  // an explicit operation on the new ownership lifetime.
  receiverReports.reflections.clear();
  VariableLengthData const reacquisitionTag(
      reacquisitionTagBytes,
      sizeof(reacquisitionTagBytes));
  REQUIRE_NOTHROW(owner->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      ownerAttributes,
      reacquisitionTag));
  REQUIRE(ownerReports.acquisitions.empty());
  AttributeHandleSet reacquiredAttributes;
  REQUIRE_NOTHROW(newOwner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      newOwnerAttributes,
      reacquisitionTag,
      reacquiredAttributes));
  REQUIRE(reacquiredAttributes == newOwnerAttributes);
  REQUIRE_FALSE(newOwner->isAttributeOwnedByFederate(objectInstance, newOwnerAttribute));
  REQUIRE(owner->isAttributeOwnedByFederate(objectInstance, ownerAttribute));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(ownerReports.acquisitions.size() == 1U);
  auto const& reacquisition = ownerReports.acquisitions.front();
  REQUIRE(reacquisition.objectInstance == objectInstance);
  REQUIRE(reacquisition.attributes == ownerAttributes);
  REQUIRE(bytes(reacquisition.userSuppliedTag) ==
          std::vector<unsigned char>(
              reacquisitionTagBytes,
              reacquisitionTagBytes + sizeof(reacquisitionTagBytes)));

  AttributeHandleValueMap reacquiredValues;
  reacquiredValues.emplace(
      ownerAttribute,
      VariableLengthData(reacquiredValueBytes, sizeof(reacquiredValueBytes)));
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      reacquiredValues,
      reacquisitionTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& reacquiredReflection = receiverReports.reflections.back();
  REQUIRE(reacquiredReflection.objectInstance == objectInstance);
  REQUIRE(reacquiredReflection.attributeValues.size() == 1U);
  REQUIRE(reacquiredReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(bytes(reacquiredReflection.attributeValues.at(receiverAttribute)) ==
          std::vector<unsigned char>(
              reacquiredValueBytes,
              reacquiredValueBytes + sizeof(reacquiredValueBytes)));
  REQUIRE(reacquiredReflection.sentRegionsSupplied);
  REQUIRE(reacquiredReflection.sentRegions.empty());
  receiverReports.reflections.clear();

  setRegionBounds(*owner, ownerRegion, ownerDimension, 0UL, 2UL);
  REQUIRE_NOTHROW(owner->associateRegionsForUpdates(
      objectInstance,
      ownerPair));
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      reacquiredValues,
      reacquisitionTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& recreatedReflection = receiverReports.reflections.back();
  REQUIRE(recreatedReflection.sentRegionsSupplied);
  REQUIRE(recreatedReflection.sentRegions == RegionHandleSet{ownerRegion});

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));
  REQUIRE_NOTHROW(newOwner->unsubscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      newOwnerPair));
  REQUIRE_NOTHROW(newOwner->unassociateRegionsForUpdates(
      objectInstance,
      newOwnerPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(
      objectInstance,
      ownerPair));
  REQUIRE_NOTHROW(newOwner->unpublishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverOwnerRegion));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverReplacementRegion));
  REQUIRE_NOTHROW(newOwner->deleteRegion(newOwnerRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(newOwner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(newOwner->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded ownership transfer activates a non-owner's deferred 2025 update-region association",
    "[integration][development-profile][federation-management][object-management][ddm][ownership-management]"
    "[ownership-transfer-update-region][ownership-transfer-update-region-deferred][callback-evoked]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values]") {
  OwnershipRegionAmbassador ownerReports;
  OwnershipRegionAmbassador newOwnerReports;
  OwnershipRegionAmbassador receiverReports;
  auto owner = makeRti();
  auto newOwner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
  unsigned char const acquisitionTagBytes[] = {0x41, 0x43, 0x51, 0x26};
  unsigned char const divestitureTagBytes[] = {0x44, 0x49, 0x56, 0x26};
  unsigned char const ownerValueBytes[] = {0x4F, 0x4C, 0x44, 0x26};
  unsigned char const newOwnerValueBytes[] = {0x4E, 0x45, 0x57, 0x26};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(newOwner->connect(newOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ownership-region-deferred-owner",
      L"provider-a",
      federationName));
  REQUIRE_NOTHROW(newOwner->joinFederationExecution(
      L"ownership-region-deferred-new-owner",
      L"provider-b",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"ownership-region-deferred-receiver",
      L"subscriber",
      federationName));

  auto const ownerObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerObjectClass,
      L"ProviderBValue");
  auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
  auto const newOwnerObjectClass = newOwner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const newOwnerAttribute = newOwner->getAttributeHandle(
      newOwnerObjectClass,
      L"ProviderBValue");
  auto const newOwnerDimension = newOwner->getDimensionHandle(L"UmbraRegionX");
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverObjectClass,
      L"ProviderBValue");
  auto const receiverDimension = receiver->getDimensionHandle(L"UmbraRegionX");
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(ownerDimension.isValid());
  REQUIRE(newOwnerObjectClass.isValid());
  REQUIRE(newOwnerAttribute.isValid());
  REQUIRE(newOwnerDimension.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverDimension.isValid());

  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const newOwnerAttributes{newOwnerAttribute};
  AttributeHandleSet const receiverAttributes{receiverAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes));
  REQUIRE_NOTHROW(newOwner->publishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{ownerDimension});
  auto const deferredRegion = newOwner->createRegion(DimensionHandleSet{newOwnerDimension});
  auto const receiverOwnerRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  auto const receiverDeferredRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  setRegionBounds(*owner, ownerRegion, ownerDimension, 0UL, 2UL);
  setRegionBounds(*newOwner, deferredRegion, newOwnerDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverOwnerRegion, receiverDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverDeferredRegion, receiverDimension, 0UL, 2UL);

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const deferredPair{{
      newOwnerAttributes,
      RegionHandleSet{deferredRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      RegionHandleSet{receiverOwnerRegion, receiverDeferredRegion},
  }};
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(newOwner->subscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      deferredPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(objectInstance.isValid());
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});
  REQUIRE(receiverReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});

  // A non-owner may establish its update-region association before it owns
  // the attribute.  The association is deferred: it must not retarget an
  // update emitted by the current owner.
  REQUIRE_NOTHROW(newOwner->associateRegionsForUpdates(
      objectInstance,
      deferredPair));
  AttributeHandleValueMap ownerValues;
  ownerValues.emplace(
      ownerAttribute,
      VariableLengthData(ownerValueBytes, sizeof(ownerValueBytes)));
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      ownerValues,
      divestitureTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& ownerReflection = receiverReports.reflections.back();
  REQUIRE(ownerReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(ownerReflection.sentRegionsSupplied);
  REQUIRE(ownerReflection.sentRegions == RegionHandleSet{ownerRegion});
  receiverReports.reflections.clear();

  REQUIRE_NOTHROW(newOwner->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      newOwnerAttributes,
      acquisitionTag));
  REQUIRE(newOwnerReports.acquisitions.empty());
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownerAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownerAttributes);
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.acquisitions.size() == 1U);
  REQUIRE(newOwnerReports.acquisitions.back().objectInstance == objectInstance);
  REQUIRE(newOwner->isAttributeOwnedByFederate(objectInstance, newOwnerAttribute));

  // Ownership acquisition promotes the deferred association atomically.  No
  // second Associate Regions For Updates call is needed before this update.
  AttributeHandleValueMap newOwnerValues;
  newOwnerValues.emplace(
      newOwnerAttribute,
      VariableLengthData(newOwnerValueBytes, sizeof(newOwnerValueBytes)));
  REQUIRE_NOTHROW(newOwner->updateAttributeValues(
      objectInstance,
      newOwnerValues,
      acquisitionTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& newOwnerReflection = receiverReports.reflections.back();
  REQUIRE(newOwnerReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(newOwnerReflection.sentRegionsSupplied);
  REQUIRE(newOwnerReflection.sentRegions == RegionHandleSet{deferredRegion});

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));
  REQUIRE_NOTHROW(newOwner->unsubscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      deferredPair));
  REQUIRE_NOTHROW(newOwner->unassociateRegionsForUpdates(
      objectInstance,
      deferredPair));
  REQUIRE_NOTHROW(newOwner->unpublishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverOwnerRegion));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverDeferredRegion));
  REQUIRE_NOTHROW(newOwner->deleteRegion(deferredRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(newOwner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(newOwner->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

TEST_CASE(
    "Embedded ownership transfer drops a non-owner's cancelled 2025 update-region association",
    "[integration][development-profile][federation-management][object-management][ddm][ownership-management]"
    "[ownership-transfer-update-region]"
    "[ownership-transfer-update-region-deferred-cancel][callback-evoked]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.associate-regions-for-updates]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.update-attribute-values]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values]") {
  OwnershipRegionAmbassador ownerReports;
  OwnershipRegionAmbassador newOwnerReports;
  OwnershipRegionAmbassador receiverReports;
  auto owner = makeRti();
  auto newOwner = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
  unsigned char const acquisitionTagBytes[] = {0x41, 0x43, 0x51, 0x27};
  unsigned char const divestitureTagBytes[] = {0x44, 0x49, 0x56, 0x27};
  unsigned char const ownerValueBytes[] = {0x4F, 0x4C, 0x44, 0x27};
  unsigned char const newOwnerValueBytes[] = {0x4E, 0x45, 0x57, 0x27};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes,
      sizeof(acquisitionTagBytes));
  VariableLengthData const divestitureTag(
      divestitureTagBytes,
      sizeof(divestitureTagBytes));

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(newOwner->connect(newOwnerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      L"HLAinteger64Time"));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"ownership-region-cancel-owner",
      L"provider-a",
      federationName));
  REQUIRE_NOTHROW(newOwner->joinFederationExecution(
      L"ownership-region-cancel-new-owner",
      L"provider-b",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"ownership-region-cancel-receiver",
      L"subscriber",
      federationName));

  auto const ownerObjectClass = owner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const ownerAttribute = owner->getAttributeHandle(
      ownerObjectClass,
      L"ProviderBValue");
  auto const ownerDimension = owner->getDimensionHandle(L"UmbraRegionX");
  auto const newOwnerObjectClass = newOwner->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const newOwnerAttribute = newOwner->getAttributeHandle(
      newOwnerObjectClass,
      L"ProviderBValue");
  auto const newOwnerDimension = newOwner->getDimensionHandle(L"UmbraRegionX");
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const receiverAttribute = receiver->getAttributeHandle(
      receiverObjectClass,
      L"ProviderBValue");
  auto const receiverDimension = receiver->getDimensionHandle(L"UmbraRegionX");
  REQUIRE(ownerObjectClass.isValid());
  REQUIRE(ownerAttribute.isValid());
  REQUIRE(ownerDimension.isValid());
  REQUIRE(newOwnerObjectClass.isValid());
  REQUIRE(newOwnerAttribute.isValid());
  REQUIRE(newOwnerDimension.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverAttribute.isValid());
  REQUIRE(receiverDimension.isValid());

  AttributeHandleSet const ownerAttributes{ownerAttribute};
  AttributeHandleSet const newOwnerAttributes{newOwnerAttribute};
  AttributeHandleSet const receiverAttributes{receiverAttribute};
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes));
  REQUIRE_NOTHROW(newOwner->publishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));

  auto const ownerRegion = owner->createRegion(DimensionHandleSet{ownerDimension});
  auto const cancelledRegion = newOwner->createRegion(
      DimensionHandleSet{newOwnerDimension});
  auto const receiverOwnerRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  auto const receiverCancelledRegion = receiver->createRegion(
      DimensionHandleSet{receiverDimension});
  setRegionBounds(*owner, ownerRegion, ownerDimension, 0UL, 2UL);
  setRegionBounds(*newOwner, cancelledRegion, newOwnerDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverOwnerRegion, receiverDimension, 0UL, 2UL);
  setRegionBounds(*receiver, receiverCancelledRegion, receiverDimension, 0UL, 2UL);

  AttributeHandleSetRegionHandleSetPairVector const ownerPair{{
      ownerAttributes,
      RegionHandleSet{ownerRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const cancelledPair{{
      newOwnerAttributes,
      RegionHandleSet{cancelledRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      receiverAttributes,
      RegionHandleSet{receiverOwnerRegion, receiverCancelledRegion},
  }};
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(newOwner->subscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      cancelledPair));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(objectInstance.isValid());
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});
  REQUIRE(receiverReports.discoveries ==
          std::vector<ObjectInstanceHandle>{objectInstance});

  // The non-owner's association is recorded provisionally, then cancelled
  // before acquisition.  It must not affect the current owner's update and
  // must not be promoted when ownership later changes.
  REQUIRE_NOTHROW(newOwner->associateRegionsForUpdates(
      objectInstance,
      cancelledPair));
  REQUIRE_NOTHROW(newOwner->unassociateRegionsForUpdates(
      objectInstance,
      cancelledPair));
  AttributeHandleValueMap ownerValues;
  ownerValues.emplace(
      ownerAttribute,
      VariableLengthData(ownerValueBytes, sizeof(ownerValueBytes)));
  REQUIRE_NOTHROW(owner->updateAttributeValues(
      objectInstance,
      ownerValues,
      divestitureTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& ownerReflection = receiverReports.reflections.back();
  REQUIRE(ownerReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(ownerReflection.sentRegionsSupplied);
  REQUIRE(ownerReflection.sentRegions == RegionHandleSet{ownerRegion});
  receiverReports.reflections.clear();

  REQUIRE_NOTHROW(newOwner->attributeOwnershipAcquisitionIfAvailable(
      objectInstance,
      newOwnerAttributes,
      acquisitionTag));
  REQUIRE(newOwnerReports.acquisitions.empty());
  AttributeHandleSet divestedAttributes;
  REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
      objectInstance,
      ownerAttributes,
      divestitureTag,
      divestedAttributes));
  REQUIRE(divestedAttributes == ownerAttributes);
  drain(*owner, *newOwner, *receiver);
  REQUIRE(newOwnerReports.acquisitions.size() == 1U);
  REQUIRE(newOwnerReports.acquisitions.back().objectInstance == objectInstance);
  REQUIRE(newOwner->isAttributeOwnedByFederate(objectInstance, newOwnerAttribute));

  AttributeHandleValueMap newOwnerValues;
  newOwnerValues.emplace(
      newOwnerAttribute,
      VariableLengthData(newOwnerValueBytes, sizeof(newOwnerValueBytes)));
  REQUIRE_NOTHROW(newOwner->updateAttributeValues(
      objectInstance,
      newOwnerValues,
      acquisitionTag));
  drain(*owner, *newOwner, *receiver);
  REQUIRE(receiverReports.reflections.size() == 1U);
  auto const& newOwnerReflection = receiverReports.reflections.back();
  REQUIRE(newOwnerReflection.attributeValues.contains(receiverAttribute));
  REQUIRE(newOwnerReflection.sentRegionsSupplied);
  REQUIRE(newOwnerReflection.sentRegions.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
      receiverObjectClass,
      receiverPair));
  REQUIRE_NOTHROW(newOwner->unsubscribeObjectClassAttributesWithRegions(
      newOwnerObjectClass,
      cancelledPair));
  REQUIRE_NOTHROW(newOwner->unpublishObjectClassAttributes(
      newOwnerObjectClass,
      newOwnerAttributes));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverOwnerRegion));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverCancelledRegion));
  REQUIRE_NOTHROW(newOwner->deleteRegion(cancelledRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(newOwner->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(newOwner->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}
