#include <catch2/catch_test_macros.hpp>

#include "internal/fom/hla_names.hpp"
#include "hla_test_names.hpp"

#include <RTI/NullFederateAmbassador.h>
#include <RTI/RTI1516.h>
#include <RTI/time/HLAinteger64Time.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef UMBRA_SOURCE_DIRECTORY
#error "The regional declaration-relevance focused test requires the Umbra source directory."
#endif

namespace {

namespace standard_hla = umbra::detail::hla::wide;
namespace fixture_hla = umbra::test::hla::wide;

using rti1516_2025::AttributeHandleSet;
using rti1516_2025::AttributeHandleSetRegionHandleSetPairVector;
using rti1516_2025::CallbackModel;
using rti1516_2025::DimensionHandleSet;
using rti1516_2025::HLA_EVOKED;
using rti1516_2025::InteractionClassHandle;
using rti1516_2025::NO_ACTION;
using rti1516_2025::NullFederateAmbassador;
using rti1516_2025::ObjectClassHandle;
using rti1516_2025::RTIambassador;
using rti1516_2025::RTIambassadorFactory;
using rti1516_2025::RangeBounds;
using rti1516_2025::RegionHandleSet;

struct ObjectClassRelevanceReport final {
  ObjectClassHandle objectClass;
};

struct InteractionRelevanceReport final {
  InteractionClassHandle interactionClass;
};

class RegionalDeclarationFederateAmbassador final : public NullFederateAmbassador {
 public:
  void startRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    startRegistrationForObjectClassReports.push_back({objectClass});
  }

  void stopRegistrationForObjectClass(
      ObjectClassHandle const& objectClass) override {
    stopRegistrationForObjectClassReports.push_back({objectClass});
  }

  void turnInteractionsOn(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOnReports.push_back({interactionClass});
  }

  void turnInteractionsOff(
      InteractionClassHandle const& interactionClass) override {
    turnInteractionsOffReports.push_back({interactionClass});
  }

  std::vector<ObjectClassRelevanceReport> startRegistrationForObjectClassReports;
  std::vector<ObjectClassRelevanceReport> stopRegistrationForObjectClassReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOnReports;
  std::vector<InteractionRelevanceReport> turnInteractionsOffReports;
};

std::unique_ptr<RTIambassador> makeRti() {
  RTIambassadorFactory factory;
  return factory.createRTIambassador();
}

std::filesystem::path resourcePath(std::filesystem::path const& relativePath) {
  return std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "third_party" /
      "ieee1516.2-2025" / "resources" / relativePath;
}

std::wstring nextFederationName() {
  static std::atomic_uint64_t counter{0};
  return L"umbra-regional-declaration-relevance-" +
      std::to_wstring(++counter);
}

}  // namespace

TEST_CASE(
    "Embedded regional declaration relevance advisories follow active subscriptions",
    "[integration][development-profile][federation-management][ddm]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.start-registration-for-object-class]"
    "[rti.service.stop-registration-for-object-class]"
    "[rti.service.turn-interactions-on][rti.service.turn-interactions-off]"
    "[federate.callback.start-registration-for-object-class]"
    "[federate.callback.stop-registration-for-object-class]"
    "[federate.callback.turn-interactions-on][federate.callback.turn-interactions-off]"
    "[declaration-relevance-advisory-regional][2025]") {
  RegionalDeclarationFederateAmbassador publisherReports;
  RegionalDeclarationFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{restaurantFom, switchFom},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-relevance-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-relevance-subscriber", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::food_drink_soda);
  auto const attribute = publisher->getAttributeHandle(
      objectClass, fixture_hla::fixture::flavor);
  auto const objectDimension =
      publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const interactionDimension =
      publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  REQUIRE(objectDimension.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(interactionDimension.isValid());
  REQUIRE(publisher->getObjectClassRelevanceAdvisorySwitch());
  REQUIRE(publisher->getInteractionRelevanceAdvisorySwitch());

  REQUIRE_NOTHROW(
      publisher->publishObjectClassAttributes(objectClass, {attribute}));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const objectRegion =
      subscriber->createRegion(DimensionHandleSet{objectDimension});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      objectRegion, objectDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(
      subscriber->commitRegionModifications(RegionHandleSet{objectRegion}));
  AttributeHandleSetRegionHandleSetPairVector const objectSubscription{{
      AttributeHandleSet{attribute},
      RegionHandleSet{objectRegion},
  }};

  // A passive regional declaration is retained but does not establish
  // relevance. Switching the same pair to active creates Start Registration.
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.size() == 1U);
  REQUIRE(publisherReports.startRegistrationForObjectClassReports.front().objectClass ==
          objectClass);

  REQUIRE_NOTHROW(subscriber->unsubscribeObjectClassAttributesWithRegions(
      objectClass, objectSubscription));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.size() == 1U);
  REQUIRE(publisherReports.stopRegistrationForObjectClassReports.front().objectClass ==
          objectClass);

  auto const interactionRegion =
      subscriber->createRegion(DimensionHandleSet{interactionDimension});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      interactionRegion, interactionDimension, RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(
      RegionHandleSet{interactionRegion}));

  // Regional interaction declarations use the same active/passive boundary.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}, false));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.empty());
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}, true));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOnReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOnReports.front().interactionClass ==
          interactionClass);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{interactionRegion}));
  REQUIRE_FALSE(publisher->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(publisherReports.turnInteractionsOffReports.size() == 1U);
  REQUIRE(publisherReports.turnInteractionsOffReports.front().interactionClass ==
          interactionClass);

  REQUIRE_NOTHROW(subscriber->deleteRegion(objectRegion));
  REQUIRE_NOTHROW(subscriber->deleteRegion(interactionRegion));
  REQUIRE_NOTHROW(publisher->unpublishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->unpublishObjectClassAttributes(
      objectClass, {attribute}));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
