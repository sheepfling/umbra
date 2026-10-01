#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded mixed regional TSO advances deliver before FQR TARA and NMRA grants",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][tso][flush-queue-request]"
    "[time-advance-request-available][next-message-request-available]"
    "[rti.service.send-interaction-with-regions][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.time-advance-request]"
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
  unsigned char const parameterBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x52};
  unsigned char const tagBytes[] = {0x4D, 0x49, 0x58, 0x2D, 0x46};
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
      L"mixed-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(fqr->joinFederationExecution(
      L"mixed-regional-fqr", L"subscriber", federationName));
  REQUIRE_NOTHROW(tara->joinFederationExecution(
      L"mixed-regional-tara", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmra->joinFederationExecution(
      L"mixed-regional-nmra", L"subscriber", federationName));
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
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));

  auto prepareReceiver = [&](auto& rti) {
    auto const region = rti->createRegion(DimensionHandleSet{serverId});
    REQUIRE_NOTHROW(rti->setRangeBounds(region, serverId, RangeBounds(5UL, 15UL)));
    REQUIRE_NOTHROW(rti->commitRegionModifications(RegionHandleSet{region}));
    REQUIRE_NOTHROW(rti->subscribeInteractionClassWithRegions(
        interactionClass,
        RegionHandleSet{region}));
    REQUIRE_FALSE(rti->getConveyRegionDesignatorSetsSwitch());
    REQUIRE_NOTHROW(rti->setConveyRegionDesignatorSetsSwitch(true));
    REQUIRE_NOTHROW(rti->enableTimeConstrained());
    REQUIRE_FALSE(rti->evokeCallback(0.0));
    return region;
  };
  auto const fqrRegion = prepareReceiver(fqr);
  auto const taraRegion = prepareReceiver(tara);
  auto const nmraRegion = prepareReceiver(nmra);

  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const message = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(message.isValid());
  REQUIRE(fqrReports.timestampedInteractionReports.empty());
  REQUIRE(taraReports.timestampedInteractionReports.empty());
  REQUIRE(nmraReports.timestampedInteractionReports.empty());

  // All three constrained members see the same regional TSO frontier. FQR
  // uses request 10, TARA reaches 7 inclusively, and NMRA selects the queued
  // timestamp 7; the regulator's request to 2 establishes GALT 7 for all.
  REQUIRE_NOTHROW(fqr->flushQueueRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
      rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto requireRegionalReport = [&](auto const& report) {
    REQUIRE(report.interactionClass == interactionClass);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(report.parameterValues.contains(temperatureOk));
    REQUIRE(report.producingFederate == publisherHandle);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"7");
    REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(report.sentRegionsSupplied);
    REQUIRE(report.sentRegions.contains(publisherRegion));
    REQUIRE(report.retractionSupplied);
    REQUIRE(report.retractionValid);
  };

  REQUIRE_FALSE(fqr->evokeCallback(0.0));
  REQUIRE_FALSE(tara->evokeCallback(0.0));
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
  requireRegionalReport(fqrReports.timestampedInteractionReports.front());
  requireRegionalReport(taraReports.timestampedInteractionReports.front());
  requireRegionalReport(nmraReports.timestampedInteractionReports.front());
  REQUIRE(variableLengthDataBytes(fqrReports.timestampedInteractionReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

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
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
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
