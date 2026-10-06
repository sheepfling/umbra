#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded timestamped default-region interaction preserves 2025 regional callback metadata",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[default-region][tso][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0xD3, 0x0F};
  unsigned char const tagBytes[] = {0x54, 0x53, 0x4F};
  ParameterHandleValueMap parameters;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-interaction-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
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

  // No source RegionHandle is supplied. For this dimensional interaction,
  // the 2025 RTI-provided default region overlaps the receiver's committed,
  // non-empty explicit subscription region and must retain that fact through
  // the federation-owned TSO queue.
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region interaction survives time-constrained re-enable",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region-interaction][default-region][tso][re-enable]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.disable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-constrained-enabled]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x52, 0x45, 0x45, 0x4E};
  unsigned char const tagBytes[] = {0x44, 0x44, 0x4D, 0x2D, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-reenable-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-reenable-receiver", L"subscriber", federationName));
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

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // The dimensional interaction uses the private default source region. Its
  // queued TSO identity must survive a receiver role transition; disabling
  // and re-enabling Time Constrained must not deliver, discard, duplicate, or
  // replace the pending passel.
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->disableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 2U);
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(
      receiverReports.callbackOrder ==
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
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region interaction uses available and next-message advances",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region][default-region][tso]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class-with-regions]"
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
  unsigned char const tagBytes[] = {0x41, 0x56, 0x41, 0x2D, 0x44};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"alternate-default-region-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"alternate-default-region-receiver", L"subscriber", federationName));
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

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
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

  auto const first = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(first.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // TARA reaches the exact defined-GALT boundary. The private default source
  // region overlaps the committed receiver region and is conveyed as an
  // explicit supplied-empty callback set before the matching grant.
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
  REQUIRE(firstReport.sentRegions.empty());
  REQUIRE(firstReport.retractionSupplied);
  REQUIRE(firstReport.retractionValid);

  auto const second = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(9));
  REQUIRE(second.isValid());

  // NMRA selects the next queued timestamp while the regulator advances from
  // 2 to 4, making GALT exactly 9. The second default-region callback again
  // precedes its grant and retains the supplied-empty region marker.
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
  REQUIRE(secondReport.sentRegions.empty());
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
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region interaction survives time-regulation disable and re-enable",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region-interaction][default-region][tso][re-enable][regulation-disable]"
    "[timestamped-default-region-interaction-regulation-reenable-changed-lookahead]"
    "[changed-lookahead]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.disable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-regulation-enabled]"
    "[federate.callback.time-advance-grant]") {
  auto runScenario = [](std::int64_t reenabledLookahead,
                        std::int64_t producerAdvance) {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x52, 0x45, 0x47, 0x44};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-regulation-reenable-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-default-region-regulation-reenable-receiver",
      L"subscriber",
      federationName));
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

  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The producer's private default source region is part of the accepted
  // recipient ledger. Turning regulation off must not discard that queued
  // regional passel; re-enabling with the configured lookahead must retain its
  // identity even when that lookahead changes.
  REQUIRE_NOTHROW(publisher->disableTimeRegulation());
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(reenabledLookahead)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 2U);

  rti1516_2025::HLAinteger64Interval queriedLookahead;
  REQUIRE_NOTHROW(publisher->queryLookahead(queriedLookahead));
  REQUIRE(queriedLookahead.getInterval() == reenabledLookahead);

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(producerAdvance)));
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
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  };

  // Exercise both the unchanged-lookahead lifecycle and the changed-lookahead
  // boundary.  With lookahead three, TAR(3) is the first producer position
  // whose GALT reaches the queued timestamp six.
  runScenario(5, 2);
  runScenario(3, 3);
}

TEST_CASE(
    "Embedded mixed default-region timestamped interactions deliver before FQR TARA and NMRA grants",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region-interaction][default-region][tso][multi-federate-callback-ordering]"
    "[flush-queue-request]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction][rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-range-bounds][rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request-available]"
    "[rti.service.next-message-request-available][rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador fqrReports;
  ReportingFederateAmbassador taraReports;
  ReportingFederateAmbassador nmraReports;
  auto publisher = makeRti();
  auto fqr = makeRti();
  auto tara = makeRti();
  auto nmra = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x49, 0x4E, 0x54, 0x2D, 0x44};
  unsigned char const tagBytes[] = {0x49, 0x4E, 0x54, 0x2D, 0x46};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(fqr->connect(fqrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tara->connect(taraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmra->connect(nmraReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"mixed-default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"mixed-default-region-interaction-fqr", L"subscriber", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"mixed-default-region-interaction-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"mixed-default-region-interaction-nmra", L"subscriber", federationName));

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

  auto prepareReceiver = [&](auto& rti) {
    auto const region = rti->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, serverId, RangeBounds(8UL, 9UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{region}));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    while (rti->evokeCallback(0.0)) {
    }
    return region;
  };
  auto const fqrRegion = prepareReceiver(fqr);
  auto const taraRegion = prepareReceiver(tara);
  auto const nmraRegion = prepareReceiver(nmra);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  fqrReports.callbackOrder.clear();
  taraReports.callbackOrder.clear();
  nmraReports.callbackOrder.clear();
  REQUIRE(fqrReports.timestampedInteractionReports.empty());
  REQUIRE(taraReports.timestampedInteractionReports.empty());
  REQUIRE(nmraReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto requireDefaultRegionInteraction = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(temperatureOk));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
            std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.empty());
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };

  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  // Draining one recipient must not consume or reorder callbacks queued for
  // the other two recipients.
  REQUIRE(taraReports.callbackOrder.empty());
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE_FALSE(tara->evokeCallback(0.0));
  REQUIRE(nmraReports.callbackOrder.empty());
  REQUIRE_FALSE(nmra->evokeCallback(0.0));
  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(fqrReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"7");
  REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(fqrReports.callbackOrder == std::vector<std::string>{"interaction", "flush-grant"});
  REQUIRE(taraReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(taraReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(nmraReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmraReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  requireDefaultRegionInteraction(fqrReports.timestampedInteractionReports.front());
  requireDefaultRegionInteraction(taraReports.timestampedInteractionReports.front());
  requireDefaultRegionInteraction(nmraReports.timestampedInteractionReports.front());

  REQUIRE_NOTHROW(fqr->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{fqrRegion}));
  REQUIRE_NOTHROW(tara->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{taraRegion}));
  REQUIRE_NOTHROW(nmra->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{nmraRegion}));
  REQUIRE_NOTHROW(fqr->deleteRegion(fqrRegion));
  REQUIRE_NOTHROW(tara->deleteRegion(taraRegion));
  REQUIRE_NOTHROW(nmra->deleteRegion(nmraRegion));
  REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmra->disconnect());
  REQUIRE_NOTHROW(tara->disconnect());
  REQUIRE_NOTHROW(fqr->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded timestamped default-region Send Interaction splits immediate delivery and pending retraction",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[default-region][tso][mixed-fanout][multi-federate-callback-ordering][retract]"
    "[rti.service.send-interaction][rti.service.retract]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador constrainedReports;
  ReportingFederateAmbassador immediateReports;
  auto publisher = makeRti();
  auto constrained = makeRti();
  auto immediate = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x44, 0x45, 0x46, 0x49};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x54, 0x53, 0x4F};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  ParameterHandleValueMap parameters;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(constrained->connect(constrainedReports, HLA_EVOKED));
  REQUIRE_NOTHROW(immediate->connect(immediateReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-default-region-mixed-interaction-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(constrained->joinFederationExecution(
      L"timestamped-default-region-mixed-interaction-constrained",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(immediate->joinFederationExecution(
      L"timestamped-default-region-mixed-interaction-immediate",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const constrainedRegion = constrained->createRegion(DimensionHandleSet{serverId});
  auto const immediateRegion = immediate->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(constrained->setRangeBounds(
      constrainedRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(immediate->setRangeBounds(
      immediateRegion,
      serverId,
      RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(constrained->commitRegionModifications(
      RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->commitRegionModifications(
      RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(immediate->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(constrained->enableTimeConstrained());
  while (constrained->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  // The ordinary timestamped invocation has no public source RegionHandle.
  // Its private default realization overlaps both committed regional
  // declarations, but only the time-constrained recipient enters the TSO queue.
  constrainedReports.callbackOrder.clear();
  immediateReports.callbackOrder.clear();
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameters,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());

  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.timestampedInteractionReports.size() == 1U);
  auto const& immediateReport = immediateReports.timestampedInteractionReports.front();
  REQUIRE(immediateReport.interactionClass == interactionClass);
  REQUIRE(immediateReport.parameterValues.size() == 1U);
  REQUIRE(immediateReport.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(immediateReport.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(immediateReport.producingFederate == publisherHandle);
  REQUIRE(immediateReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(immediateReport.timeValue == L"6");
  REQUIRE(immediateReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(immediateReport.receivedOrderType == rti1516_2025::RECEIVE);
  REQUIRE(immediateReport.retractionSupplied);
  REQUIRE(immediateReport.retractionValid);
  REQUIRE(immediateReport.sentRegionsSupplied);
  REQUIRE(immediateReport.sentRegions.empty());
  REQUIRE(variableLengthDataBytes(immediateReport.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  // Retract preserves the delivered recipient in the ledger while withdrawing
  // the constrained recipient's pending default-region interaction before it
  // can cross the grant boundary.
  REQUIRE_NOTHROW(constrained->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(retraction));
  // The constrained recipient must not observe a callback merely because the
  // immediate recipient received the interaction and its Request Retraction.
  REQUIRE(constrainedReports.callbackOrder.empty());
  while (immediate->evokeCallback(0.0)) {
  }
  REQUIRE(immediateReports.requestRetractionReports.size() == 1U);
  REQUIRE(immediateReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              immediateReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(immediateReports.callbackOrder ==
          std::vector<std::string>{"interaction", "request-retraction"});

  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (constrained->evokeCallback(0.0)) {
  }
  REQUIRE(constrainedReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().implementationName ==
          standard_hla::mom::integer64_time);
  REQUIRE(constrainedReports.timeAdvanceGrantReports.front().value == L"6");
  REQUIRE(constrainedReports.timestampedInteractionReports.empty());
  REQUIRE(constrainedReports.requestRetractionReports.empty());
  REQUIRE(constrainedReports.callbackOrder == std::vector<std::string>{"grant"});

  REQUIRE_NOTHROW(constrained->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{constrainedRegion}));
  REQUIRE_NOTHROW(immediate->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{immediateRegion}));
  REQUIRE_NOTHROW(constrained->deleteRegion(constrainedRegion));
  REQUIRE_NOTHROW(immediate->deleteRegion(immediateRegion));
  REQUIRE_NOTHROW(constrained->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(immediate->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(constrained->disconnect());
  REQUIRE_NOTHROW(immediate->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
}
