#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded queued timestamped regional interaction survives time-regulation disable and re-enable with changed lookahead",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][tso][re-enable][regulation-disable][changed-lookahead]"
    "[timestamped-regional-interaction-regulation-reenable-changed-lookahead]"
    "[rti.service.send-interaction-with-regions][rti.service.enable-time-regulation]"
    "[rti.service.disable-time-regulation][rti.service.query-lookahead]"
    "[rti.service.time-advance-request][rti.service.enable-time-constrained]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[federate.callback.time-regulation-enabled]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xC7, 0x19};
  unsigned char const tagBytes[] = {0x52, 0x47, 0x43, 0x48};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-changed-lookahead-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-changed-lookahead-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegion = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{sourceRegion},
      tag,
      rti1516_2025::HLAinteger64Time(5));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The accepted TSO message keeps the source region realization from send
  // time.  A later source mutation must not make the queued recipient lose
  // an interaction that already overlapped at admission.
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegion,
      serverId,
      RangeBounds(16UL, 20UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{sourceRegion}));

  // The accepted regional passel retains its invocation-time source-region
  // snapshot while the producer changes time-regulation role and lookahead.
  // Re-enabling with lookahead three must establish the new GALT boundary,
  // not replace or re-route the queued passel.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(3)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);
  rti1516_2025::HLAinteger64Interval changedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(changedLookahead));
  REQUIRE(changedLookahead.getInterval() == 3);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(5)));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"5");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

} // namespace
