#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {

TEST_CASE(
    "Embedded timestamped regional interaction subscription replacement does not retarget a queued passel",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][explicit-source][tso][subscription-replacement]"
    "[timestamped-regional-interaction-subscription-replacement]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.send-interaction-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]"
    "[federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x49, 0x52, 0x31};
  unsigned char const secondParameterBytes[] = {0x49, 0x52, 0x32};
  unsigned char const firstTagBytes[] = {0x53, 0x55, 0x42, 0x31};
  unsigned char const secondTagBytes[] = {0x53, 0x55, 0x42, 0x32};
  ParameterHandleValueMap firstParameters;
  ParameterHandleValueMap secondParameters;
  VariableLengthData const firstTag(firstTagBytes, sizeof(firstTagBytes));
  VariableLengthData const secondTag(secondTagBytes, sizeof(secondTagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, L"HLAinteger64Time"));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-subscription-replacement-publisher",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-subscription-replacement-receiver",
      L"subscriber",
      federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = publisher->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  auto const serverId = publisher->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  firstParameters.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  secondParameters.emplace(
      temperatureOk,
      VariableLengthData(secondParameterBytes, sizeof(secondParameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto const sourceRegionA = publisher->createRegion(DimensionHandleSet{serverId});
  auto const sourceRegionB = publisher->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegionA = receiver->createRegion(DimensionHandleSet{serverId});
  auto const receiverRegionB = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionA,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      sourceRegionB,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegionA,
      serverId,
      RangeBounds(0UL, 1UL)));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegionB,
      serverId,
      RangeBounds(2UL, 3UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(
      RegionHandleSet{sourceRegionA, sourceRegionB}));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(
      RegionHandleSet{receiverRegionA, receiverRegionB}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionA}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.timeRegulationEnabledReports.size() == 1U);

  // The first passel captures receiverRegionA at acceptance. Replacing the
  // subscription before its callback boundary must suppress it rather than
  // retarget it to receiverRegionB or synthesize Request Retraction.
  auto const firstRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      firstParameters,
      RegionHandleSet{sourceRegionA},
      firstTag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstRetraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionA}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionB}));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());

  // Later sends use the replacement subscription and source region. This
  // passel must deliver exactly once with receiverRegionB's source realization.
  auto const secondRetraction = publisher->sendInteractionWithRegions(
      interactionClass,
      secondParameters,
      RegionHandleSet{sourceRegionB},
      secondTag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondRetraction.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2U);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1U);
  REQUIRE(report.parameterValues.contains(temperatureOk));
  REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
          std::vector<unsigned char>(secondParameterBytes,
                                     secondParameterBytes + sizeof(secondParameterBytes)));
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(secondTagBytes,
                                     secondTagBytes + sizeof(secondTagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == L"HLAinteger64Time");
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions == RegionHandleSet{sourceRegionB});
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "interaction", "grant"});
  REQUIRE(receiverReports.requestRetractionReports.empty());

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegionB}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegionB));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegionA));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionB));
  REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegionA));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

} // namespace
