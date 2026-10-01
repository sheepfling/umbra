#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded timestamped regional interaction uses available and next-message advances",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][tso]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction-with-regions][rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x41, 0x56, 0x41, 0x2D, 0x52};
  unsigned char const tagBytes[] = {0x52, 0x41, 0x56, 0x2D, 0x44};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"alternate-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"alternate-regional-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

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

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const first = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(first.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // TARA reaches the exact defined-GALT boundary and carries the explicit
  // source RegionHandle through the callback before the matching grant.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.front().value == L"7");

  auto const& firstReport = receiverReports.timestampedInteractionReports.front();
  REQUIRE(firstReport.interactionClass == interactionClass);
  REQUIRE(firstReport.parameterValues.size() == 1U);
  REQUIRE(firstReport.parameterValues.contains(temperatureOk));
  REQUIRE(firstReport.producingFederate == publisherHandle);
  REQUIRE(firstReport.timeValue == L"7");
  REQUIRE(firstReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(firstReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(firstReport.sentRegionsSupplied);
  REQUIRE(firstReport.sentRegions.contains(publisherRegion));
  REQUIRE(firstReport.retractionSupplied);
  REQUIRE(firstReport.retractionValid);

  auto const second = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(second.isValid());

  // NMRA selects the next queued timestamp while the regulator advances from
  // 2 to 4, making GALT exactly 9. The explicit source region is preserved.
  REQUIRE_NOTHROW(receiver->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(
      receiverReports.callbackOrder ==
      std::vector<std::string>{"interaction", "grant", "interaction", "grant"});
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"9");

  auto const& secondReport = receiverReports.timestampedInteractionReports.back();
  REQUIRE(secondReport.interactionClass == interactionClass);
  REQUIRE(secondReport.parameterValues.size() == 1U);
  REQUIRE(secondReport.parameterValues.contains(temperatureOk));
  REQUIRE(secondReport.producingFederate == publisherHandle);
  REQUIRE(secondReport.timeValue == L"9");
  REQUIRE(secondReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(secondReport.sentRegionsSupplied);
  REQUIRE(secondReport.sentRegions.contains(publisherRegion));
  REQUIRE(secondReport.retractionSupplied);
  REQUIRE(secondReport.retractionValid);
  REQUIRE(variableLengthDataBytes(secondReport.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  rti1516_2025::HLAinteger64Time queriedTime;
  REQUIRE_NOTHROW(receiver->queryLogicalTime(queriedTime));
  REQUIRE(queriedTime.getTime() == 9);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
