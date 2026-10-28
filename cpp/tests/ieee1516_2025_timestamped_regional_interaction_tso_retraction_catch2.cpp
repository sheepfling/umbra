#include "ieee1516_2025_federation_management_fixture_support.hpp"

#include <array>

TEST_CASE(
    "Embedded timestamped regional interaction queues TSO before the grant and supports retraction",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[rti.service.send-interaction-with-regions][rti.service.retract]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x4D, 0x0A};
  unsigned char const tagBytes[] = {0x6B, 0x22};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timestamped-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timestamped-regional-receiver", L"subscriber", federationName));
  suppressDeclarationRelevanceAdvisories(*publisher);
  REQUIRE_FALSE(receiver->getConveyRegionDesignatorSetsSwitch());

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
      publisherRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  REQUIRE_THROWS_AS(
      publisher->sendInteractionWithRegions(
          interactionClass,
          parameterValues,
          RegionHandleSet{publisherRegion},
          tag,
          rti1516_2025::HLAinteger64Time(4)),
      rti1516_2025::InvalidLogicalTime);

  auto const firstHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(firstHandle.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(publisher->retract(firstHandle));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  auto const secondHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(secondHandle.isValid());
  // The receiver's TAR(7) must remain strictly below GALT. Advancing the
  // producer to 3 with its lookahead of 5 establishes GALT 8.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(3)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 2);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"grant", "interaction", "grant"});

  auto const& report = receiverReports.timestampedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.parameterValues.size() == 1);
  REQUIRE(report.parameterValues.find(temperatureOk) != report.parameterValues.end());
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);
  REQUIRE_FALSE(report.sentRegionsSupplied);
  REQUIRE_THROWS_AS(
      publisher->retract(secondHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));
  auto const thirdHandle = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(thirdHandle.isValid());
  // Advance once more so TAR(8) is still strictly below GALT.
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 3);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 2);
  auto const& conveyedReport = receiverReports.timestampedInteractionReports.back();
  REQUIRE(conveyedReport.timeValue == L"8");
  REQUIRE(conveyedReport.sentRegionsSupplied);
  REQUIRE(conveyedReport.sentRegions.size() == 1U);
  REQUIRE(conveyedReport.sentRegions.begin()->isValid());
  REQUIRE_THROWS_AS(
      publisher->retract(thirdHandle),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded regional Send Interaction follows all eight Table 2 order-classification rows",
    "[integration][development-profile][interaction-management][ddm][time-management]"
    "[regional-interaction-order-classification-matrix]"
    "[rti.service.send-interaction-with-regions][rti.service.change-interaction-order-type]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][federate.callback.receive-interaction]"
    "[federate.callback.time-advance-grant][2025]") {
  struct Table2Row final {
    rti1516_2025::OrderType preferredOrder;
    bool senderRegulates;
    bool timestampPresent;
    rti1516_2025::OrderType expectedSentOrder;
  };
  constexpr std::array<Table2Row, 8> rows{{
      {rti1516_2025::RECEIVE, false, false, rti1516_2025::RECEIVE},
      {rti1516_2025::RECEIVE, false, true, rti1516_2025::RECEIVE},
      {rti1516_2025::TIMESTAMP, false, false, rti1516_2025::RECEIVE},
      {rti1516_2025::TIMESTAMP, false, true, rti1516_2025::RECEIVE},
      {rti1516_2025::RECEIVE, true, false, rti1516_2025::RECEIVE},
      {rti1516_2025::RECEIVE, true, true, rti1516_2025::RECEIVE},
      {rti1516_2025::TIMESTAMP, true, false, rti1516_2025::RECEIVE},
      {rti1516_2025::TIMESTAMP, true, true, rti1516_2025::TIMESTAMP},
  }};

  ReportingFederateAmbassador senderReports;
  ReportingFederateAmbassador receiverReports;
  ReportingFederateAmbassador regulatorReports;
  auto sender = makeRti();
  auto receiver = makeRti();
  auto regulator = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterByte = 0x4DU;
  unsigned char const tagByte = 0x52U;
  ParameterHandleValueMap parameters;
  VariableLengthData const parameterValue(&parameterByte, sizeof(parameterByte));
  VariableLengthData const tag(&tagByte, sizeof(tagByte));

  REQUIRE_NOTHROW(sender->connect(senderReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(regulator->connect(regulatorReports, HLA_EVOKED));
  REQUIRE_NOTHROW(sender->createFederationExecution(
      federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle senderHandle;
  REQUIRE_NOTHROW(senderHandle = sender->joinFederationExecution(
      L"regional-order-matrix-sender", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"regional-order-matrix-receiver", L"subscriber", federationName));
  REQUIRE_NOTHROW(regulator->joinFederationExecution(
      L"regional-order-matrix-regulator", L"regulator", federationName));

  auto const interactionClass = sender->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::main_course_served);
  auto const temperatureOk = sender->getParameterHandle(
      interactionClass, fixture_hla::fixture::temperature_ok);
  auto const serverId = sender->getDimensionHandle(fixture_hla::fixture::server_id);
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  REQUIRE(temperatureOk.isValid());
  REQUIRE(serverId.isValid());
  parameters.emplace(temperatureOk, parameterValue);
  REQUIRE_NOTHROW(sender->publishInteractionClass(interactionClass));

  auto const sourceRegion = sender->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(sender->setRangeBounds(sourceRegion, serverId, RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(sender->commitRegionModifications(RegionHandleSet{sourceRegion}));
  auto const subscriptionRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      subscriptionRegion, serverId, RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      receiverInteractionClass, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(regulator->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(regulator->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiver->enableAsynchronousDelivery());

  auto const pumpUntil = [&](auto ready) {
    for (unsigned attempt = 0U; attempt != 30U && !ready(); ++attempt) {
      static_cast<void>(receiver->evokeCallback(0.05));
    }
    REQUIRE(ready());
  };
  auto const firstByte = [](VariableLengthData const& value) {
    auto const* bytes = static_cast<unsigned char const*>(value.data());
    return bytes == nullptr || value.size() == 0U ? 0U : bytes[0];
  };

  bool senderRegulating = false;
  std::size_t expectedUntimestampedReports = 0U;
  std::size_t expectedTimestampedReports = 0U;
  rti1516_2025::MessageRetractionHandle tsoRetraction;
  for (std::size_t index = 0U; index < rows.size(); ++index) {
    auto const& row = rows[index];
    if (row.senderRegulates && !senderRegulating) {
      REQUIRE_NOTHROW(sender->enableTimeRegulation(
          rti1516_2025::HLAinteger64Interval(5)));
      for (unsigned attempt = 0U;
           attempt != 30U && senderReports.timeRegulationEnabledReports.empty();
           ++attempt) {
        static_cast<void>(sender->evokeCallback(0.05));
      }
      REQUIRE(senderReports.timeRegulationEnabledReports.size() == 1U);
      senderRegulating = true;
    }
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(
        interactionClass, row.preferredOrder));
    auto const valueByte = static_cast<unsigned char>(index + 1U);
    auto const caseTagByte = static_cast<unsigned char>(0x60U + index);
    ParameterHandleValueMap rowParameters;
    rowParameters.emplace(
        temperatureOk, VariableLengthData(&valueByte, sizeof(valueByte)));
    VariableLengthData const rowTag(&caseTagByte, sizeof(caseTagByte));

    if (row.timestampPresent) {
      auto const retraction = sender->sendInteractionWithRegions(
          interactionClass,
          rowParameters,
          RegionHandleSet{sourceRegion},
          rowTag,
          rti1516_2025::HLAinteger64Time(6));
      auto const expectsRetraction =
          row.senderRegulates && row.preferredOrder == rti1516_2025::TIMESTAMP;
      REQUIRE(retraction.isValid() == expectsRetraction);
      if (expectsRetraction) {
        tsoRetraction = retraction;
      } else {
        ++expectedTimestampedReports;
        pumpUntil([&] {
          return receiverReports.timestampedInteractionReports.size() ==
              expectedTimestampedReports;
        });
      }
    } else {
      sender->sendInteractionWithRegions(
          interactionClass,
          rowParameters,
          RegionHandleSet{sourceRegion},
          rowTag);
      ++expectedUntimestampedReports;
      pumpUntil([&] {
        return receiverReports.interactionReports.size() == expectedUntimestampedReports;
      });
    }
  }

  REQUIRE(expectedUntimestampedReports == 4U);
  REQUIRE(expectedTimestampedReports == 3U);
  REQUIRE(receiverReports.interactionReports.size() == expectedUntimestampedReports);
  REQUIRE(receiverReports.timestampedInteractionReports.size() == expectedTimestampedReports);
  REQUIRE(tsoRetraction.isValid());

  for (std::size_t index = 0U; index < receiverReports.interactionReports.size(); ++index) {
    auto const& report = receiverReports.interactionReports[index];
    auto const rowIndex = index * 2U;
    REQUIRE(rows[rowIndex].expectedSentOrder == rti1516_2025::RECEIVE);
    REQUIRE(report.interactionClass == receiverInteractionClass);
    REQUIRE(report.producingFederate == senderHandle);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(firstByte(report.parameterValues.at(temperatureOk)) == rowIndex + 1U);
    REQUIRE(firstByte(report.userSuppliedTag) == 0x60U + rowIndex);
    REQUIRE_FALSE(report.sentRegionsSupplied);
  }
  constexpr std::array<std::size_t, 3> timestampedReceiveOrderRows{1U, 3U, 5U};
  for (std::size_t index = 0U; index < timestampedReceiveOrderRows.size(); ++index) {
    auto const rowIndex = timestampedReceiveOrderRows[index];
    auto const& report = receiverReports.timestampedInteractionReports[index];
    REQUIRE(report.interactionClass == receiverInteractionClass);
    REQUIRE(report.producingFederate == senderHandle);
    REQUIRE(report.parameterValues.size() == 1U);
    REQUIRE(firstByte(report.parameterValues.at(temperatureOk)) == rowIndex + 1U);
    REQUIRE(firstByte(report.userSuppliedTag) == 0x60U + rowIndex);
    REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.timeValue == L"6");
    REQUIRE(rows[rowIndex].expectedSentOrder == report.sentOrderType);
    REQUIRE(report.sentOrderType == rti1516_2025::RECEIVE);
    REQUIRE(report.receivedOrderType == rti1516_2025::RECEIVE);
    REQUIRE_FALSE(report.retractionSupplied);
  }

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_NOTHROW(sender->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  REQUIRE_NOTHROW(regulator->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(2)));
  for (unsigned attempt = 0U;
       attempt != 30U && senderReports.timeAdvanceGrantReports.empty();
       ++attempt) {
    static_cast<void>(sender->evokeCallback(0.05));
  }
  REQUIRE(senderReports.timeAdvanceGrantReports.size() == 1U);
  for (unsigned attempt = 0U;
       attempt != 30U && regulatorReports.timeAdvanceGrantReports.empty();
       ++attempt) {
    static_cast<void>(regulator->evokeCallback(0.05));
  }
  REQUIRE(regulatorReports.timeAdvanceGrantReports.size() == 1U);
  pumpUntil([&] { return receiverReports.timeAdvanceGrantReports.size() == 1U; });
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 4U);
  auto const& tsoReport = receiverReports.timestampedInteractionReports.back();
  REQUIRE(tsoReport.interactionClass == receiverInteractionClass);
  REQUIRE(tsoReport.producingFederate == senderHandle);
  REQUIRE(tsoReport.parameterValues.size() == 1U);
  REQUIRE(firstByte(tsoReport.parameterValues.at(temperatureOk)) == 8U);
  REQUIRE(firstByte(tsoReport.userSuppliedTag) == 0x67U);
  REQUIRE(tsoReport.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(tsoReport.timeValue == L"6");
  REQUIRE(rows.back().expectedSentOrder == tsoReport.sentOrderType);
  REQUIRE(tsoReport.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(tsoReport.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(tsoReport.retractionSupplied);
  REQUIRE(tsoReport.retractionValid);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass, RegionHandleSet{subscriptionRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(subscriptionRegion));
  REQUIRE_NOTHROW(sender->deleteRegion(sourceRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(regulator->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(sender->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(regulator->disconnect());
  REQUIRE_NOTHROW(sender->disconnect());
}
