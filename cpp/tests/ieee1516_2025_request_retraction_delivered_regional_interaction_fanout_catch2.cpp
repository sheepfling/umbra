#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded Request Retraction notifies delivered regional-interaction recipients and suppresses queued fanout",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.request-retraction]"
    "[request-retraction-delivered-regional-interaction-fanout][2025]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador immediateReports;
  ReportingFederateAmbassador constrainedReports;
  auto publisher = makeRti();
  auto immediate = makeRti();
  auto constrained = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x52, 0x52};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x49};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"regional-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"regional-retraction-immediate", L"subscriber", federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"regional-retraction-constrained", L"subscriber", federationName));
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
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{serverId});
  auto const constrainedRegion = constrained->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(constrained->setRangeBounds(
      constrainedRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->commitRegionModifications(RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(2));
  REQUIRE(retraction.isValid());

  // The immediate recipient's overlap-qualified callback begins before the
  // constrained recipient can reach its grant boundary. The recipient ledger
  // still owns both states even though only one entered the temporal queue.
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1);
  auto const& first = immediateReports.timestampedInteractionReports.front();
  REQUIRE(first.interactionClass == interactionClass);
  REQUIRE(first.parameterValues.size() == 1);
  REQUIRE(first.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(first.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(first.timeValue == L"2");
  REQUIRE(first.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(first.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(first.retractionSupplied);
  REQUIRE(first.retractionValid);
  REQUIRE_FALSE(first.sentRegionsSupplied);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 1);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(constrained->evokeCallback(0.0));
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());

  // A regional send with no constrained recipient still receives an official
  // designator and retains the delivered immediate recipient for a later
  // Request Retraction callback.
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  auto const immediateOnlyRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(4));
  REQUIRE(immediateOnlyRetraction.isValid());
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 2);
  REQUIRE(immediateReports.timestampedInteractionReports.back().retractionValid);
  REQUIRE_NOTHROW(publisher->retract(immediateOnlyRetraction));
  REQUIRE_FALSE(immediate->evokeCallback(0.0));
  REQUIRE(immediateReports.requestRetractionReports.size() == 2);
  REQUIRE(immediateReports.requestRetractionReports.back().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.back().encodedRetraction) ==
          variableLengthDataBytes(immediateOnlyRetraction.encode()));

  REQUIRE_NOTHROW(immediate->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
