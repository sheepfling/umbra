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
#error "The regional Auto Provide admission-fence test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;

std::vector<unsigned char> variableLengthDataBytes(
    rti1516_2025::VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  if (bytes == nullptr) {
    return {};
  }
  return {bytes, bytes + value.size()};
}

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::AttributeHandleValueMap;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandle;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::ParameterHandleValueMap;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::RangeBounds;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::VariableLengthData;

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"regional-auto-provide-switch-admission-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

class ReportingFederateAmbassador final
    : public rti1516_2025::NullFederateAmbassador {
 public:
  struct DiscoveryReport final {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
    std::wstring objectInstanceName;
  };

  struct ProvideReport final {
    ObjectInstanceHandle objectInstance;
    AttributeHandleSet attributes;
    VariableLengthData userSuppliedTag;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const& objectInstanceName,
      rti1516_2025::FederateHandle const&) override {
    discoveries.push_back({objectInstance, objectClass, objectInstanceName});
    if (onDiscoverObjectInstance) {
      onDiscoverObjectInstance();
    }
  }

  void provideAttributeValueUpdate(
      ObjectInstanceHandle const& objectInstance,
      AttributeHandleSet const& attributes,
      VariableLengthData const& userSuppliedTag) override {
    provides.push_back({objectInstance, attributes, userSuppliedTag});
    if (onProvideAttributeValueUpdate) {
      onProvideAttributeValueUpdate();
    }
  }

  std::vector<DiscoveryReport> discoveries;
  std::vector<ProvideReport> provides;
  std::function<void()> onDiscoverObjectInstance;
  std::function<void()> onProvideAttributeValueUpdate;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

}  // namespace

TEST_CASE(
    "Embedded regional Auto Provide consumes queued discovery work when switch changes before provider admission and re-enables fresh discovery under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][mom][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-switch-admission-mutation]"
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
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
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
    std::size_t provideCount = 0U;
    bool disableOnFirstDiscovery = true;

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(federationName, fomModule));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"regional-auto-provide-switch-admission-owner",
        L"provider",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"regional-auto-provide-switch-admission-requester",
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
    auto drain = [&] {
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        return;
      }
      for (int pass = 0; pass != 64; ++pass) {
        static_cast<void>(requester->evokeCallback(0.0));
        static_cast<void>(owner->evokeCallback(0.0));
      }
    };

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
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(requester->setConveyRegionDesignatorSetsSwitch(true));

    requesterReports.onDiscoverObjectInstance = [&] {
      if (disableOnFirstDiscovery) {
        disableOnFirstDiscovery = false;
        // This mutation occurs inside discovery, before queueObjectInstance-
        // Discovery plans any Auto Provide recipients for the new object.
        setAutoProvide(false);
      }
    };
    ownerReports.onProvideAttributeValueUpdate = [&] {
      ++provideCount;
    };

    ObjectInstanceHandle const firstObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(firstObject.isValid());
    drain();
    REQUIRE(requesterReports.discoveries.size() == 1U);
    REQUIRE(ownerReports.provides.empty());
    REQUIRE(provideCount == 0U);
    REQUIRE_FALSE(owner->getAutoProvideSwitch());
    REQUIRE_FALSE(requester->getAutoProvideSwitch());

    REQUIRE_NOTHROW(setAutoProvide(true));
    REQUIRE(owner->getAutoProvideSwitch());
    REQUIRE(requester->getAutoProvideSwitch());

    ObjectInstanceHandle const secondObject =
        owner->registerObjectInstanceWithRegions(ownerObjectClass, ownerPair);
    REQUIRE(secondObject.isValid());
    drain();
    REQUIRE(requesterReports.discoveries.size() == 2U);
    REQUIRE(ownerReports.provides.size() == 1U);
    REQUIRE(ownerReports.provides.front().objectInstance == secondObject);
    REQUIRE(ownerReports.provides.front().attributes == ownerAttributes);
    REQUIRE(ownerReports.provides.front().userSuppliedTag.size() == 0U);
    REQUIRE(provideCount == 1U);

    REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
        requesterObjectClass,
        requesterPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(firstObject, ownerPair));
    REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(secondObject, ownerPair));
    REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
    REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
    REQUIRE_NOTHROW(requester->resignFederationExecution(
        rti1516_2025::NO_ACTION));
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

TEST_CASE(
    "Embedded regional Auto Provide preserves synchronous discovery-before-provider ordering under HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][ddm][auto-provide]"
    "[regional-automatic-provision][regional-automatic-provision-immediate]"
    "[auto-provide-regional][callback-immediate]"
    "[rti.service.get-auto-provide-switch]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.provide-attribute-value-update]") {
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

  REQUIRE_NOTHROW(owner->connect(ownerReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(requester->connect(requesterReports, rti1516_2025::HLA_IMMEDIATE));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"regional-auto-provide-immediate-owner",
      L"provider",
      federationName));
  REQUIRE_NOTHROW(requester->joinFederationExecution(
      L"regional-auto-provide-immediate-requester",
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

  ownerReports.onProvideAttributeValueUpdate = [&callbackOrder] {
    callbackOrder.push_back("provide");
  };
  requesterReports.onDiscoverObjectInstance = [&callbackOrder] {
    callbackOrder.push_back("discover");
  };

  REQUIRE_NOTHROW(requester->disableCallbacks());
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstanceWithRegions(
      ownerObjectClass,
      ownerPair));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(requester->subscribeObjectClassAttributesWithRegions(
      requesterObjectClass,
      requesterPair));
  REQUIRE(requesterReports.discoveries.empty());
  REQUIRE(ownerReports.provides.empty());
  REQUIRE(callbackOrder.empty());

  // HLA_IMMEDIATE dispatches retained discovery synchronously when callbacks
  // are re-enabled; the provider solicitation must follow that discovery.
  REQUIRE_NOTHROW(requester->enableCallbacks());
  REQUIRE(requesterReports.discoveries.size() == 1U);
  REQUIRE(ownerReports.provides.size() == 1U);
  REQUIRE(callbackOrder == std::vector<std::string>{"discover", "provide"});
  auto const& request = ownerReports.provides.front();
  REQUIRE(request.objectInstance == objectInstance);
  REQUIRE(request.attributes == ownerAttributes);
  REQUIRE(variableLengthDataBytes(request.userSuppliedTag).empty());

  REQUIRE_NOTHROW(requester->unsubscribeObjectClassAttributesWithRegions(
      requesterObjectClass,
      requesterPair));
  REQUIRE_NOTHROW(owner->unassociateRegionsForUpdates(objectInstance, ownerPair));
  REQUIRE_NOTHROW(requester->deleteRegion(requesterRegion));
  REQUIRE_NOTHROW(owner->deleteRegion(ownerRegion));
  REQUIRE_NOTHROW(requester->resignFederationExecution(
      rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(owner->disconnect());
  REQUIRE_NOTHROW(requester->disconnect());
}
