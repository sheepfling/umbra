#define UMBRA_REUSE_TIMED_RESTORE_INTERACTION_HELPERS
#include "timed_restore_live_tso_default_region_interaction_catch2.cpp"
#undef UMBRA_REUSE_TIMED_RESTORE_INTERACTION_HELPERS

TEST_CASE(
    "Embedded timed federation restore restores a live timestamped regional interaction at the save boundary",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][ddm][time-management][timed-save][tso]"
    "[timestamped-regional-interaction][explicit-source][timestamped-regional-interaction-timed-restore]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.send-interaction-with-regions][rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x54, 0x49, 0x4D, 0x45};
  unsigned char const tagBytes[] = {0x52, 0x45, 0x47};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timed-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timed-regional-receiver", L"subscriber", federationName));
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const parameter = publisher->getParameterHandle(
      interactionClass, fixture_hla::fixture::temperature_ok);
  auto const dimension = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(parameter.isValid());
  REQUIRE(dimension.isValid());
  parameterValues.emplace(parameter,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  auto const receiverRegion = receiver->createRegion(
      rti1516_2025::DimensionHandleSet{dimension});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion, dimension, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, dimension, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {}

  auto const retraction = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(retraction.isValid());
  REQUIRE_NOTHROW(publisher->requestFederationSave(
      L"timed-regional-save", rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  while (receiver->evokeCallback(0.0)) {}
  while (publisher->evokeCallback(0.0)) {}
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {}
  while (receiver->evokeCallback(0.0)) {}

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_THROWS_AS(publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_NOTHROW(publisher->requestFederationRestore(L"timed-regional-save"));
  while (publisher->evokeCallback(0.0)) {}
  while (receiver->evokeCallback(0.0)) {}
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {}
  while (receiver->evokeCallback(0.0)) {}

  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractions.size() == 1U);
  auto const& report = receiverReports.timestampedInteractions.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.contains(parameter));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeValue == L"8");
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{publisherRegion});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractions.size() == 1U);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(rti1516_2025::NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
