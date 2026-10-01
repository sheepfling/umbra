#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded regional TSO advances deliver before TAR and NMR grants",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[timestamped-regional-interaction][tso][time-advance-request][next-message-request]"
    "[rti.service.send-interaction-with-regions][rti.service.time-advance-request]"
    "[rti.service.next-message-request]"
    "[federate.callback.receive-interaction][federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador tarReports;
  ReportingFederateAmbassador nmrReports;
  auto publisher = makeRti();
  auto tar = makeRti();
  auto nmr = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x54, 0x41, 0x52, 0x2D, 0x4E};
  unsigned char const tagBytes[] = {0x54, 0x41, 0x52, 0x2D, 0x4D};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(tar->connect(tarReports, HLA_EVOKED));
  REQUIRE_NOTHROW(nmr->connect(nmrReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"regional-tar-nmr-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(tar->joinFederationExecution(
      L"regional-tar-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(nmr->joinFederationExecution(
      L"regional-nmr-receiver", L"subscriber", federationName));
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
  auto const tarRegion = prepareReceiver(tar);
  auto const nmrRegion = prepareReceiver(nmr);

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
  REQUIRE(tarReports.timestampedInteractionReports.empty());
  REQUIRE(nmrReports.timestampedInteractionReports.empty());

  // Ordinary TAR and NMR both admit the inclusive timestamp-7 frontier. The
  // regulator's request to 2 plus lookahead 5 establishes GALT 7 for both
  // regional recipients, and each callback must precede its own grant.
  REQUIRE_NOTHROW(tar->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_NOTHROW(nmr->nextMessageRequest(rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(tar->evokeCallback(0.0));
  REQUIRE_FALSE(nmr->evokeCallback(0.0));

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

  REQUIRE(publisherReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(publisherReports.timeAdvanceGrantReports.front().value == L"2");
  REQUIRE(tarReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(tarReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(tarReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  REQUIRE(nmrReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(nmrReports.timeAdvanceGrantReports.front().value == L"7");
  REQUIRE(nmrReports.callbackOrder == std::vector<std::string>{"interaction", "grant"});
  requireRegionalReport(tarReports.timestampedInteractionReports.front());
  requireRegionalReport(nmrReports.timestampedInteractionReports.front());
  REQUIRE(variableLengthDataBytes(tarReports.timestampedInteractionReports.front().userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));

  REQUIRE_NOTHROW(tar->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{tarRegion}));
  REQUIRE_NOTHROW(nmr->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{nmrRegion}));
  REQUIRE_NOTHROW(tar->deleteRegion(tarRegion));
  REQUIRE_NOTHROW(nmr->deleteRegion(nmrRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(nmr->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(tar->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(nmr->disconnect());
  REQUIRE_NOTHROW(tar->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
