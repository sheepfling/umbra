#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded passive interaction subscriptions do not arrange ordinary or regional delivery",
    "[integration][development-profile][interaction-management][ddm][passive-subscription]"
    "[rti.service.subscribe-interaction-class]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[federate.callback.receive-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x50, 0x41, 0x53};
  unsigned char const tagBytes[] = {0x53, 0x55, 0x42};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap parameterValues;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(
      publisherHandle = publisher->joinFederationExecution(
          L"passive-subscription-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"passive-subscription-subscriber", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));

  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  // An ordinary passive subscription is retained as declaration state but is
  // not eligible for a Receive Interaction callback. Replacing it with an
  // active subscription makes the next send eligible.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, false));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.empty());

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(interactionClass, true));
  REQUIRE_NOTHROW(publisher->sendInteraction(interactionClass, parameterValues, tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);
  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClass(interactionClass));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));

  // The same eligibility rule applies to a regional pair: the region remains
  // subscribed and in use while passive, but its overlap cannot arrange
  // delivery until the pair is made active.
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      false));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1);

  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion},
      true));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag));
  static_cast<void>(subscriber->evokeMultipleCallbacks(0.0, 0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2);
  REQUIRE(subscriberReports.interactionReports.back().producingFederate == publisherHandle);
  REQUIRE_FALSE(subscriberReports.interactionReports.back().sentRegionsSupplied);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}  // namespace
