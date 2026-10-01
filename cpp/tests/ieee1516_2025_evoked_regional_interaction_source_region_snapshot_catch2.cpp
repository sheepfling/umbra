#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded evoked regional interaction retains its send-time source region",
    "[integration][development-profile][interaction-management][ddm]"
    "[regional-interaction][source-region-mutation][federate.callback.receive-interaction]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador subscriberReports;
  auto publisher = makeRti();
  auto subscriber = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x2C, 0x01};
  unsigned char const tagBytes[] = {0x53, 0x4E, 0x50};
  ParameterHandleValueMap parameters;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(subscriber->connect(subscriberReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-source-snapshot-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(subscriber->joinFederationExecution(
      L"regional-source-snapshot-subscriber", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const subscriberRegion = subscriber->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(subscriber->setRangeBounds(
      subscriberRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(subscriber->commitRegionModifications(RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->setConveyRegionDesignatorSetsSwitch(true));

  // The send is accepted while [0, 10) overlaps the subscriber's [5, 15)
  // range, but HLA_EVOKED leaves the callback pending. Mutating the source
  // region to [20, 30) must not reinterpret that already-admitted message.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  REQUIRE(subscriberReports.interactionReports.empty());
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(16UL, 20UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1U);
  auto const& firstReport = subscriberReports.interactionReports.front();
  REQUIRE(firstReport.interactionClass == interactionClass);
  REQUIRE(firstReport.producingFederate == publisherHandle);
  REQUIRE(firstReport.sentRegionsSupplied);
  REQUIRE(firstReport.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(variableLengthDataBytes(firstReport.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // New sends use the new source realization. Resetting the region and sending
  // again proves the immutable snapshot was scoped to the first message.
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(publisher->sendInteractionWithRegions(
      interactionClass,
      parameters,
      RegionHandleSet{sourceRegion},
      tag));
  REQUIRE_FALSE(subscriber->evokeCallback(0.0));
  REQUIRE(subscriberReports.interactionReports.size() == 2U);

  REQUIRE_NOTHROW(subscriber->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{subscriberRegion}));
  REQUIRE_NOTHROW(subscriber->deleteRegion(subscriberRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(subscriber->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
