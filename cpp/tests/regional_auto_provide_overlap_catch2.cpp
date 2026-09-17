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
#error "The regional Auto Provide test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::FederateHandle;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;

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
  return L"regional-auto-provide-overlap-" +
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

  std::vector<ObjectDiscoveryReport> objectDiscoveryReports;
  std::vector<AttributeValueUpdateRequestReport> attributeValueUpdateRequestReports;
  std::function<void()> onDiscoverObjectInstance;
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
    "Embedded regional Auto Provide solicits overlap-qualified owners and suppresses stale disjoint work",
    "[integration][development-profile][federation-management][object-management][ddm][auto-provide]"
    "[regional-automatic-provision][auto-provide-regional]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
  ReportingFederateAmbassador ownerReports;
  ReportingFederateAmbassador requesterReports;
  auto owner = makeRti();
  auto requester = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("regional-ownership-fanout-fom.xml").wstring();
  std::vector<std::string> callbackOrder;
  bool moveDiscoveryOutOfScope = false;

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  FederateHandle ownerHandle;
  REQUIRE_NOTHROW(ownerHandle = owner->joinFederationExecution(
      L"regional-auto-provide-baseline-owner", L"provider", federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-auto-provide-baseline-requester", L"subscriber", federationName));

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

  ownerReports.provideAttributeValueUpdateHandler =
      [&callbackOrder](ObjectInstanceHandle const&,
                       AttributeHandleSet const&,
                       VariableLengthData const&) {
    callbackOrder.push_back("provide");
  };
  requesterReports.onDiscoverObjectInstance = [
      &callbackOrder,
      &requester,
      requesterRegion,
      requesterDimension,
      &moveDiscoveryOutOfScope]() {
    callbackOrder.push_back("discover");
    if (!moveDiscoveryOutOfScope) {
      return;
    }
    REQUIRE_NOTHROW(requester->setRangeBounds(
        requesterRegion,
        requesterDimension,
        RangeBounds(4UL, 5UL)));
    REQUIRE_NOTHROW(requester->commitRegionModifications(
        RegionHandleSet{requesterRegion}));
  };

  ObjectInstanceHandle firstObject;
  REQUIRE_NOTHROW(firstObject = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(firstObject.isValid());
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
      requesterObjectClass,
      requesterPair));
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(requester->evokeCallback(0.0));
    static_cast<void>(owner->evokeCallback(0.0));
  }
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
  REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});
  auto const& firstRequest = ownerReports.attributeValueUpdateRequestReports.front();
  REQUIRE(firstRequest.objectInstance == firstObject);
  REQUIRE(firstRequest.attributes == ownerAttributes);
  REQUIRE(variableLengthDataBytes(firstRequest.userSuppliedTag).empty());

  // The second discovery is delivered, but moving the subscribed region out
  // of overlap during discovery suppresses stale Auto Provide admission.
  moveDiscoveryOutOfScope = true;
  ObjectInstanceHandle secondObject;
  REQUIRE_NOTHROW(secondObject = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(secondObject.isValid());
  for (int pass = 0; pass != 64; ++pass) {
    static_cast<void>(requester->evokeCallback(0.0));
    static_cast<void>(owner->evokeCallback(0.0));
  }
  REQUIRE(requesterReports.objectDiscoveryReports.size() == 2U);
  REQUIRE(ownerReports.attributeValueUpdateRequestReports.size() == 1U);
  REQUIRE(callbackOrder ==
          std::vector<std::string>{"discover", "provide", "discover"});

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
      requesterObjectClass,
      requesterPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(firstObject, ownerPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(secondObject, ownerPair));
  REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
