#define UMBRA_REUSE_TIMED_RESTORE_INTERACTION_HELPERS
#include "timed_restore_live_tso_default_region_interaction_catch2.cpp"
#undef UMBRA_REUSE_TIMED_RESTORE_INTERACTION_HELPERS

void suppressDeclarationRelevanceAdvisories(RTIambassador& rti) {
  if (rti.getObjectClassRelevanceAdvisorySwitch()) {
    rti.setObjectClassRelevanceAdvisorySwitch(false);
  }
  if (rti.getInteractionRelevanceAdvisorySwitch()) {
    rti.setInteractionRelevanceAdvisorySwitch(false);
  }
}

TEST_CASE(
    "Embedded queued timestamped regional interaction survives source resignation",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][explicit-source][tso][resignation]"
    "[multi-federate-callback-ordering]"
    "[rti.service.send-interaction-with-regions][rti.service.resign-federation-execution]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador clockReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x52, 0x45, 0x47, 0x53};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x53, 0x49, 0x47};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      sender->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"timestamped-regional-resignation-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-resignation-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timestamped-regional-resignation-clock", L"publisher", federationName));
  suppressDeclarationRelevanceAdvisories(*sender);

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = sender->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegion = sender->createRegion(
      rti1516_2025::DimensionHandleSet{serverId});
  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(sender->setRangeBounds(sourceRegion, serverId, RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(receiverRegion, serverId, RangeBounds(8UL, 9UL)));
  REQUIRE_NOTHROW(sender->commitRegionModifications(RegionHandleSet{sourceRegion}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(sender->evokeCallback(0.0));
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(clock->evokeCallback(0.0));

  auto const retraction = sender->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{sourceRegion},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractions.empty());

  // Admit the recipient's frontier while the sender is still present, then
  // resign the source. The accepted regional passel must remain in the
  // recipient-local queue, retaining its explicit source-region realization.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE(receiverReports.timeAdvanceGrants.empty());
  REQUIRE_NOTHROW(sender->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE(receiverReports.timestampedInteractions.empty());

  // An independent regulator supplies the temporal frontier after the source
  // federate has left the execution.
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (clockReports.timeAdvanceGrants.empty() &&
         clock->evokeCallback(0.0)) {
  }
  REQUIRE(clockReports.timeAdvanceGrants.size() == 1U);

  while (receiverReports.timeAdvanceGrants.empty() &&
         receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timestampedInteractions.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrants.size() == 1U);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractions.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(
              parameterBytes, parameterBytes + sizeof(parameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == senderHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegion});
  REQUIRE(receiverReports.timeAdvanceGrants.front().value == L"6");
  // Membership is checked before retraction classification once the source
  // ambassador has resigned.
  REQUIRE_THROWS_AS(
      sender->retract(retraction),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(receiver->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
}
