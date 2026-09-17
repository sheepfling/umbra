#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The passive regional update-rate test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandle;
using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::ObjectInstanceHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;
using rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;

class RecordingFederateAmbassador final : public NullFederateAmbassador {
 public:
  struct DiscoveryReport {
    ObjectInstanceHandle objectInstance;
    ObjectClassHandle objectClass;
  };

  void discoverObjectInstance(
      ObjectInstanceHandle const& objectInstance,
      ObjectClassHandle const& objectClass,
      std::wstring const&,
      rti1516_2025::FederateHandle const&) override {
    discoveryReports.push_back({objectInstance, objectClass});
  }

  std::vector<DiscoveryReport> discoveryReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t sequence{0U};
  return L"embedded-update-rate-passive-regional-" +
      std::to_wstring(sequence.fetch_add(1U, std::memory_order_relaxed));
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
      "third_party" / "ieee1516.2-2025" / "resources" / relativePath;
}

TEST_CASE(
    "Embedded update-rate lookup ignores passive regional subscriptions",
    "[integration][development-profile][federation-management][object-management][ddm]"
    "[callbacks][update-rate-reduction][update-rate-passive-regional-subscription]"
    "[regional-attribute-update][2025]"
    "[rti.service.get-update-rate-value-for-attribute]"
    "[rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.create-region]"
    "[rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.register-object-instance]"
    "[rti.service.evoke-callback]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.delete-region]"
    "[rti.service.unpublish-object-class-attributes]"
    "[federate.callback.discover-object-instance]") {
  RecordingFederateAmbassador ownerReports;
  RecordingFederateAmbassador subscriberReports;
  auto owner = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();

  REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(owner->createFederationExecution(
      federationName,
      fomModule,
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(owner->joinFederationExecution(
      L"passive-regional-rate-owner", L"owner", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-regional-rate-subscriber", L"subscriber", federationName));

  auto const objectClass =
      owner->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const attribute =
      owner->getAttributeHandle(objectClass, fixture_hla::fixture::flavor);
  auto const dimension =
      owner->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(dimension.isValid());
  REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
      objectClass,
      AttributeHandleSet{attribute}));

  auto const lowRegion = subscriber->createRegion(DimensionHandleSet{dimension});
  auto const highRegion = subscriber->createRegion(DimensionHandleSet{dimension});
  REQUIRE(lowRegion.isValid());
  REQUIRE(highRegion.isValid());
  REQUIRE(lowRegion != highRegion);
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      lowRegion,
      dimension,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{lowRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      highRegion,
      dimension,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{highRegion}));

  AttributeHandleSet const attributeSet{attribute};
  AttributeHandleSetRegionHandleSetPairVector const lowPair{{
      attributeSet,
      RegionHandleSet{lowRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const highPair{{
      attributeSet,
      RegionHandleSet{highRegion},
  }};
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass,
      lowPair,
      true,
      L"Low"));
  // A passive declaration remains stored, but contributes no active rate.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass,
      highPair,
      false,
      L"High"));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = owner->registerObjectInstance(objectClass));
  REQUIRE(objectInstance.isValid());
  REQUIRE_NOTHROW(subscriber->evokeCallback(0.0));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));
  REQUIRE(subscriberReports.discoveryReports.size() == 1U);
  REQUIRE(subscriberReports.discoveryReports.front().objectInstance == objectInstance);
  REQUIRE(subscriberReports.discoveryReports.front().objectClass == objectClass);

  REQUIRE(subscriber->getUpdateRateValueForAttribute(objectInstance, attribute) ==
          Catch::Approx(0.2));
  REQUIRE(owner->getUpdateRateValueForAttribute(objectInstance, attribute) ==
          Catch::Approx(0.0));

  // Activating the same High declaration makes it the active maximum over
  // the existing Low declaration.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass,
      highPair,
      true,
      L"High"));
  REQUIRE(subscriber->getUpdateRateValueForAttribute(objectInstance, attribute) ==
          Catch::Approx(30.0));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));

  // Removing High exposes the active Low declaration again.
  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass,
      highPair));
  REQUIRE(subscriber->getUpdateRateValueForAttribute(objectInstance, attribute) ==
          Catch::Approx(0.2));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass,
      lowPair));
  REQUIRE(subscriber->getUpdateRateValueForAttribute(objectInstance, attribute) ==
          Catch::Approx(0.0));

  REQUIRE_NOTHROW(subscriber->deleteRegion(lowRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(highRegion));
  REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
      objectClass,
      attributeSet));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(owner->disconnect());
}

}  // namespace
