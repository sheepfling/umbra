#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded queued timestamped default-region interaction survives source resignation for each recipient",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-default-region-interaction][default-region][tso][resignation]"
    "[multi-federate-callback-ordering]"
    "[rti.service.send-interaction][rti.service.resign-federation-execution]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.next-message-request]"
    "[rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador firstReports;
  ReportingFederateAmbassador secondReports;
  ReportingFederateAmbassador clockReports;
  auto sender = makeRti();
  auto first = makeRti();
  auto second = makeRti();
  auto clock = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x46};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x52, 0x53};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(first->connect(firstReports, HLA_EVOKED));
  REQUIRE_NOTHROW(second->connect(secondReports, HLA_EVOKED));
  REQUIRE_NOTHROW(clock->connect(clockReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      sender->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"timestamped-default-region-interaction-resignation-sender",
      L"publisher",
      federationName));
  REQUIRE_NOTHROW(first->joinFederationExecution(
      L"timestamped-default-region-interaction-resignation-first",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(second->joinFederationExecution(
      L"timestamped-default-region-interaction-resignation-second",
      L"subscriber",
      federationName));
  REQUIRE_NOTHROW(clock->joinFederationExecution(
      L"timestamped-default-region-interaction-resignation-clock",
      L"publisher",
      federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = sender->getParameterHandle(
      interactionClass,
      fixture_hla::fixture::temperature_ok);
  REQUIRE(interactionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  parameterValues.emplace(
      temperatureOk,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(sender->changeInteractionOrderType(interactionClass, TIMESTAMP));

  auto prepareReceiver = [&](auto& rti, auto& reports) {
    auto const serverId = rti->getDimensionHandle(fixture_hla::fixture::server_id);
    REQUIRE(serverId.isValid());
    auto const region = rti->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, serverId, RangeBounds(8UL, 9UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{region}));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    REQUIRE(reports.timeConstrainedEnabledReports.size() == 1U);
    return region;
  };
  auto const firstRegion = prepareReceiver(first, firstReports);
  auto const secondRegion = prepareReceiver(second, secondReports);

  REQUIRE_NOTHROW(sender->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (sender->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(clock->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (clock->evokeCallback(0.0)) {
  }

  auto const retraction = sender->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timestampedInteractionReports.empty());

  // Admit both recipient-local queues while the sender's private default
  // source region is live, then remove the source before callback
  // reconstruction. Each accepted passel must retain its invocation-time
  // default-region association and producer identity.
  REQUIRE_NOTHROW(first->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(second->nextMessageRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE(firstReports.timeAdvanceGrantReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE(firstReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timestampedInteractionReports.empty());

  // The independent regulator supplies the federation-wide frontier after
  // the producing federate has left the execution.
  REQUIRE_NOTHROW(clock->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(2)));
  while (clockReports.timeAdvanceGrantReports.empty() &&
         clock->evokeCallback(0.0)) {
  }
  REQUIRE(clockReports.timeAdvanceGrantReports.size() == 1U);

  while (firstReports.timeAdvanceGrantReports.empty() &&
         first->evokeCallback(0.0)) {
  }
  REQUIRE(firstReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(firstReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(firstReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});
  REQUIRE(secondReports.timestampedInteractionReports.empty());
  REQUIRE(secondReports.timeAdvanceGrantReports.empty());

  while (secondReports.timeAdvanceGrantReports.empty() &&
         second->evokeCallback(0.0)) {
  }
  REQUIRE(secondReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(secondReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(secondReports.callbackOrder ==
          std::vector<std::string>{"interaction", "grant"});

  auto requireDefaultRegionInteraction = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(temperatureOk));
    REQUIRE(variableLengthDataBytes(report.parameterValues.at(temperatureOk)) ==
            std::vector<unsigned char>(
                parameterBytes,
                parameterBytes + sizeof(parameterBytes)));
    REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.producingFederate == senderHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.empty());
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };
  requireDefaultRegionInteraction(firstReports.timestampedInteractionReports.front());
  requireDefaultRegionInteraction(secondReports.timestampedInteractionReports.front());
  REQUIRE(firstReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(secondReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE_THROWS_AS(
      sender->retract(retraction),
      rti1516_2025::FederateNotExecutionMember);

  REQUIRE_NOTHROW(first->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{firstRegion}));
  REQUIRE_NOTHROW(second->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{secondRegion}));
  REQUIRE_NOTHROW(first->deleteRegion(firstRegion));
  REQUIRE_NOTHROW(second->deleteRegion(secondRegion));
  REQUIRE_NOTHROW(second->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(clock->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(first->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(sender->disconnect());
  REQUIRE_NOTHROW(first->disconnect());
  REQUIRE_NOTHROW(second->disconnect());
  REQUIRE_NOTHROW(clock->disconnect());
}
}
