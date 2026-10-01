#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update cancels regular retained owner confirmation after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture][negotiated-willing-to-acquire-continuation]"
    "[willing-to-acquire][timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state]"
    "[tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-regular-candidate-state]"
    "[tso-regional-attribute-update-timed-negotiated-regular-retained-confirmation-cancel-state]"
    "[timed-multi-recipient-retained-regular-confirmation-cancel-after-restore]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]" ) {
  runTimedMultiRecipientRegionalResignationAfterRestore(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      false,
      true,
      true,
      true,
      true,
      false,
      true);
}

TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update cancels regular retained owner confirmation before delivery after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[cancel-pending-ownership-acquisitions][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture][negotiated-willing-to-acquire-continuation]"
    "[willing-to-acquire][timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-cancel-state]"
    "[tso-regional-attribute-update-timed-negotiated-state]"
    "[tso-regional-attribute-update-timed-negotiated-continuation-state]"
    "[tso-regional-attribute-update-timed-negotiated-regular-candidate-state]"
    "[tso-regional-attribute-update-timed-negotiated-regular-retained-pre-delivery-cancel-state]"
    "[timed-multi-recipient-retained-regular-confirmation-pre-delivery-cancel-after-restore]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.cancel-negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]" ) {
  runTimedMultiRecipientRegionalResignationAfterRestore(
      rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS,
      false,
      true,
      true,
      false,
      true,
      true,
      true);
}

#if 0
TEST_CASE(
    "Embedded timed multi-recipient regional timestamped attribute update survives no-action resignation after restore",
    "[integration][development-profile][federation-management][save-restore]"
    "[timed-save][object-management][ddm][time-management][tso][resignation]"
    "[no-action][timestamped-regional-attribute-update][explicit-source]"
    "[mixed-fanout][tso-regional-attribute-update-timed-no-action-state]"
    "[tso-regional-attribute-update-timed-resignation-matrix]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.update-attribute-values]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.flush-queue-request][rti.service.time-advance-request]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.reflect-attribute-values][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]" ) {
  runTimedMultiRecipientRegionalResignationAfterRestore(
      rti1516_2025::NO_ACTION);
}

#endif

TEST_CASE(
    "Embedded federation restore restores a saved live timestamped default-region interaction",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][ddm][time-management][tso]"
    "[timestamped-default-region-interaction][default-region]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant]"
    "[restore-live-tso-default-region-interaction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x53, 0x52};
  unsigned char const tagBytes[] = {0x44, 0x45, 0x46, 0x2D, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-default-region-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-default-region-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-default-region-receiver", L"subscriber", federationName));
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

  // No public source RegionHandle is supplied. The private default source
  // overlaps the receiver's committed region and must remain that same
  // realization after restore.
  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.front().value == L"5");
  REQUIRE(receiverReports.flushQueueGrantReports.front().optimisticValue == L"7");
  REQUIRE(
      std::vector<std::string>(
          receiverReports.callbackOrder.end() - 2,
          receiverReports.callbackOrder.end()) ==
      std::vector<std::string>{"interaction", "flush-grant"});

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
  REQUIRE(report.timeValue == L"7");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.sentRegionsSupplied);
  REQUIRE(report.sentRegions.empty());
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

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
#if 0
#endif

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped default-region interaction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][ddm][time-management][tso]"
    "[timestamped-default-region-interaction][default-region][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.publish-interaction-class][rti.service.change-interaction-order-type]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.unsubscribe-interaction-class-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.delete-region]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x54, 0x44, 0x2D, 0x53};
  unsigned char const tagBytes[] = {0x54, 0x44, 0x2D, 0x52};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-default-region-interaction-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-terminal-default-region-interaction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-terminal-default-region-interaction-receiver", L"subscriber", federationName));
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
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  // No public source RegionHandle is supplied. The private default source
  // overlaps the receiver's committed region; preserve only its terminal
  // classification through restore in this bounded case.
  auto const terminal = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(terminal.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(terminal));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Keep a distinct live default-source passel to admit the constrained
  // recipient; the terminal tombstone has no pending recipient state after
  // Retract.
  auto const admission = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(admission.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(admission));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  auto const postSave = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

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
    "Embedded timed federation restore restores a live timestamped object deletion at the save boundary",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][timed-save][tso][timestamped-object-deletion]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.delete-object-instance]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restored][federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x49, 0x4D, 0x45, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"timed-live-tso-deletion-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"timed-restore-live-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"timed-restore-live-deletion-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      child,
      fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(1)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  // The timestamped save boundary is four. The queued timestamp-six removal
  // must be captured in the saved image after the constrained member reaches
  // four, not delivered before save initiation.
  REQUIRE_NOTHROW(publisher->requestFederationSave(
      saveLabel,
      rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  REQUIRE_NOTHROW(publisher->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(4)));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});
  REQUIRE(receiverReports.timeAdvanceGrantReports.size() == 1U);
  REQUIRE(receiverReports.timeAdvanceGrantReports.back().value == L"4");
  REQUIRE(receiverReports.callbackOrder ==
          std::vector<std::string>{"save-initiate", "grant"});
  REQUIRE(publisherReports.initiateFederateSaveReports ==
          std::vector<std::wstring>{saveLabel});

  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  // Make the live designator terminal after saving. Restore must recover the
  // queued deletion and its retraction ledger, rather than only restoring an
  // allocator floor.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder.size() >= 2U);
  REQUIRE(
      std::vector<std::string>(
          receiverReports.callbackOrder.end() - 2,
          receiverReports.callbackOrder.end()) ==
      std::vector<std::string>{"remove", "flush-grant"});

  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores a saved live timestamped object deletion",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-object-deletion]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.delete-object-instance]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();
    auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x53, 0x41, 0x56, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-deletion-baseline";
  bool const immediate = callbackModel == rti1516_2025::HLA_IMMEDIATE;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, callbackModel));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-deletion-receiver", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(child, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(child, fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  // The constrained member must be advancing before the untimed save can
  // initiate. Its boundary stays below the queued deletion timestamp, so the
  // snapshot retains both the typed removal and its live retraction ledger.
  if (immediate) {
    // Preserve the constrained member's advancing boundary until the save
    // request is admitted; enabling callbacks then delivers the grant and the
    // resulting Initiate Federate Save notification in one immediate chain.
    REQUIRE_NOTHROW(receiver->disableCallbacks());
  }
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  if (immediate) {
    REQUIRE_NOTHROW(receiver->enableCallbacks());
  }
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  // Make the live designator terminal after saving. Restore must replace that
  // post-save state with the queued deletion and its object reconstitution
  // record, rather than merely retaining an allocator floor.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.empty());
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder.size() >= 2U);
  REQUIRE(std::vector<std::string>(
              receiverReports.callbackOrder.end() - 2,
              receiverReports.callbackOrder.end()) ==
          std::vector<std::string>{"remove", "flush-grant"});
  auto const& removal = receiverReports.objectRemovalReports.front();
  REQUIRE(removal.objectInstance == objectInstance);
  REQUIRE(removal.producingFederate == publisherHandle);
  REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(removal.timeValue == L"6");
  REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(removal.retractionSupplied);
  REQUIRE(removal.retractionValid);
  REQUIRE_THROWS_AS(
      receiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(receiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

TEST_CASE(
    "Embedded federation restore restores one queued timestamped object deletion to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-object-deletion]"
    "[mixed-fanout][multi-federate-callback-ordering]"
    "[timestamped-object-deletion-restore-multi-recipient]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.delete-object-instance]"
    "[rti.service.flush-queue-request][rti.service.retract]"
    "[rti.service.time-advance-request]"
    "[federate.callback.remove-object-instance][federate.callback.request-retraction]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador firstReceiverReports;
  ReportingFederateAmbassador secondReceiverReports;
  auto publisher = makeRti();
  auto firstReceiver = makeRti();
  auto secondReceiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" / "tests" / "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x4D, 0x55, 0x4C, 0x54, 0x49, 0x2D, 0x44};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"multi-recipient-tso-deletion-baseline";

  auto const drain = [](RTIambassador& ambassador) {
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(firstReceiver->connect(firstReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(secondReceiver->connect(secondReceiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"multi-recipient-tso-deletion-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(firstReceiver->joinFederationExecution(
      L"multi-recipient-tso-deletion-first", L"subscriber", federationName));
  REQUIRE_NOTHROW(secondReceiver->joinFederationExecution(
      L"multi-recipient-tso-deletion-second", L"subscriber", federationName));

  auto const child = publisher->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = publisher->getAttributeHandle(
      child, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = publisher->getAttributeHandle(
      child, fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(child.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(firstReceiver->subscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(secondReceiver->subscribeObjectClassAttributes(child, attributes));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstance(child));
  // Receiver subscriptions may enqueue Auto Provide setup callbacks on the
  // publisher. Drain that discovery-time work before time-role assertions so
  // this case isolates restore/deletion scheduling.
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectDiscoveryReports.size() == 1U);
  auto const objectInstanceName = publisher->getObjectInstanceName(objectInstance);
  REQUIRE(firstReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(secondReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  REQUIRE_NOTHROW(firstReceiver->enableTimeConstrained());
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(secondReceiver->enableTimeConstrained());
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE(secondReceiverReports.timeConstrainedEnabledReports.size() == 1U);
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  // Enabling the publisher's time role may enqueue its role callback after
  // the discovery-time setup drain. Consume that callback before the saved
  // deletion ledger is exercised.
  drain(*publisher);

  auto const retraction = publisher->deleteObjectInstance(
      objectInstance,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(firstReceiverReports.objectRemovalReports.empty());
  REQUIRE(secondReceiverReports.objectRemovalReports.empty());
  REQUIRE(firstReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(secondReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);

  // Keep both constrained recipients below the queued deletion timestamp so
  // the saved image contains two independent removal ledger entries.
  REQUIRE_NOTHROW(firstReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(secondReceiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(firstReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(secondReceiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(firstReceiver->federateSaveComplete());
  REQUIRE_NOTHROW(secondReceiver->federateSaveComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(firstReceiverReports.federationSavedReportCount == 1U);
  REQUIRE(secondReceiverReports.federationSavedReportCount == 1U);

  // Terminalize the post-save designator.  Restore must replace this state
  // with the queued removal and its object reconstitution record.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(firstReceiverReports.objectRemovalReports.empty());
  REQUIRE(secondReceiverReports.objectRemovalReports.empty());
  REQUIRE(firstReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(secondReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(firstReceiver->federateRestoreComplete());
  REQUIRE_NOTHROW(secondReceiver->federateRestoreComplete());
  drain(*publisher);
  drain(*firstReceiver);
  drain(*secondReceiver);
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(firstReceiverReports.federationRestoredReportCount == 1U);
  REQUIRE(secondReceiverReports.federationRestoredReportCount == 1U);

  firstReceiverReports.callbackOrder.clear();
  secondReceiverReports.callbackOrder.clear();

  auto const verifyRemoval = [&](ReportingFederateAmbassador const& reports) {
    REQUIRE(reports.objectRemovalReports.size() == 1U);
    REQUIRE(reports.flushQueueGrantReports.size() == 1U);
    auto const& removal = reports.objectRemovalReports.front();
    REQUIRE(removal.objectInstance == objectInstance);
    REQUIRE(removal.producingFederate == publisherHandle);
    REQUIRE(variableLengthDataBytes(removal.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(removal.timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(removal.timeValue == L"6");
    REQUIRE(removal.sentOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(removal.receivedOrderType == rti1516_2025::TIMESTAMP);
    REQUIRE(removal.retractionSupplied);
    REQUIRE(removal.retractionValid);
    REQUIRE(reports.callbackOrder ==
            std::vector<std::string>{"remove", "flush-grant"});
  };

  // Release each restored recipient independently.  The first removal must
  // not consume the second recipient's pending copy or retraction state.
  REQUIRE_NOTHROW(firstReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.objectRemovalReports.size() == 1U);
  REQUIRE(firstReceiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(secondReceiverReports.objectRemovalReports.empty());
  REQUIRE_NOTHROW(secondReceiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(10)));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  verifyRemoval(firstReceiverReports);
  verifyRemoval(secondReceiverReports);
  REQUIRE_THROWS_AS(
      firstReceiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);
  REQUIRE_THROWS_AS(
      secondReceiver->getObjectInstanceHandle(objectInstanceName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(firstReceiver->evokeCallback(0.0));
  REQUIRE_FALSE(secondReceiver->evokeCallback(0.0));
  REQUIRE(firstReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(secondReceiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(firstReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(secondReceiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              firstReceiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(variableLengthDataBytes(
              secondReceiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE(firstReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE(secondReceiver->getObjectInstanceHandle(objectInstanceName) == objectInstance);
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(firstReceiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(secondReceiver->unsubscribeObjectClassAttributes(child, attributes));
  REQUIRE_NOTHROW(firstReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(secondReceiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(firstReceiver->disconnect());
  REQUIRE_NOTHROW(secondReceiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores a saved live timestamped directed interaction",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][directed][time-management][tso]"
    "[timestamped-directed-interaction][timestamped-directed-interaction-restore]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.register-object-instance]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.send-directed-interaction][rti.service.flush-queue-request]"
    "[rti.service.retract][rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x53, 0x41, 0x56, 0x2D, 0x44, 0x49};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-directed-baseline";
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  FederateHandle publisherHandle;
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-live-directed-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-directed-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  directedClasses.insert(interactionClass);
  InteractionClassHandleSet const receiverDirectedClasses{receiverInteractionClass};

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const retraction = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());

  // Keep the constrained member below the queued timestamp while the
  // completed untimed snapshot captures the directed payload and its live
  // retraction ledger.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  // Terminalize the live record after saving. Restore must replace this
  // post-save state with the queued directed payload, not only its allocator
  // high-water mark.
  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  // FQR reaches the restored directed callback boundary without advancing the
  // producer. The original designator must remain live through delivery.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverReports.callbackOrder.size() >= 2U);
  REQUIRE(std::vector<std::string>(
              receiverReports.callbackOrder.end() - 2,
              receiverReports.callbackOrder.end()) ==
          std::vector<std::string>{"directed", "flush-grant"});

  auto const& report = receiverReports.directedInteractionReports.front();
  REQUIRE(report.interactionClass == interactionClass);
  REQUIRE(report.objectInstance == target);
  REQUIRE(report.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(report.producingFederate == publisherHandle);
  REQUIRE(report.transportationType ==
          publisher->getTransportationTypeHandle(standard_hla::mom::reliable));
  REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(report.timeValue == L"6");
  REQUIRE(report.sentOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.receivedOrderType == rti1516_2025::TIMESTAMP);
  REQUIRE(report.retractionSupplied);
  REQUIRE(report.retractionValid);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves TSO retraction-designator uniqueness",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][directed][time-management][tso]"
    "[tso-retraction-designator-uniqueness]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.send-directed-interaction]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x55, 0x4E, 0x49, 0x51};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"tso-retraction-designator-uniqueness";
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-unique-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-unique-retraction-receiver", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  directedClasses.insert(interactionClass);
  InteractionClassHandleSet const receiverDirectedClasses{receiverInteractionClass};

  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const saved = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(saved.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());

  // The constrained recipient must have a save-admission frontier, while its
  // target remains below the timestamp-six payload. This keeps the saved
  // passel queued so restore has to reconstruct its payload and ledger.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  // This designator is allocated after the saved boundary. Restore must make
  // it invalid without allowing the next post-restore allocation to reuse it.
  auto const postSave = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(saved.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  auto const fresh = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(fresh.isValid());
  REQUIRE(variableLengthDataBytes(postSave.encode()) !=
          variableLengthDataBytes(fresh.encode()));
  REQUIRE(variableLengthDataBytes(saved.encode()) !=
          variableLengthDataBytes(fresh.encode()));
  REQUIRE_NOTHROW(publisher->retract(fresh));
  REQUIRE_THROWS_AS(
      publisher->retract(fresh),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // The saved message remains independently queued. Crossing the callback
  // boundary proves that the fresh retraction did not alias or suppress it.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(6)));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverReports.flushQueueGrantReports.size() == 1U);
  auto const& delivered = receiverReports.directedInteractionReports.front();
  REQUIRE(delivered.interactionClass == interactionClass);
  REQUIRE(delivered.objectInstance == target);
  REQUIRE(delivered.parameterValues.empty());
  REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) ==
          std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
  REQUIRE(delivered.timeImplementationName == standard_hla::mom::integer64_time);
  REQUIRE(delivered.timeValue == L"6");
  REQUIRE(delivered.sentOrderType == TIMESTAMP);
  REQUIRE(delivered.receivedOrderType == TIMESTAMP);
  REQUIRE(delivered.retractionSupplied);
  REQUIRE(delivered.retractionValid);

  REQUIRE_NOTHROW(publisher->retract(saved));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(saved.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(saved),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores one queued timestamped directed interaction to multiple recipients",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][directed][time-management][tso]"
    "[timestamped-directed-interaction][mixed-fanout]"
    "[timestamped-directed-interaction-restore-multi-recipient]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.register-object-instance]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type][rti.service.enable-time-regulation]"
    "[rti.service.enable-time-constrained][rti.service.time-advance-request]"
    "[rti.service.send-directed-interaction][rti.service.flush-queue-request]"
    "[rti.service.retract][rti.service.unsubscribe-object-class-attributes]"
    "[rti.service.unsubscribe-object-class-directed-interactions]"
    "[federate.callback.receive-directed-interaction][federate.callback.request-retraction]"
    "[federate.callback.flush-queue-grant][federate.callback.federation-saved]"
    "[federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverAReports;
  ReportingFederateAmbassador receiverBReports;
  auto publisher = makeRti();
  auto receiverA = makeRti();
  auto receiverB = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x4D, 0x52, 0x46, 0x32};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"directed-tso-multi-recipient-restore";
  FederateHandle publisherHandle;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiverA->connect(receiverAReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiverB->connect(receiverBReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisherHandle = publisher->joinFederationExecution(
      L"restore-directed-multi-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiverA->joinFederationExecution(
      L"restore-directed-multi-receiver-a", L"subscriber", federationName));
  REQUIRE_NOTHROW(receiverB->joinFederationExecution(
      L"restore-directed-multi-receiver-b", L"subscriber", federationName));

  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(
      objectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const receiverAObjectClass = receiverA->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const receiverAMarker = receiverA->getAttributeHandle(
      receiverAObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const receiverAInteractionClass = receiverA->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const receiverBObjectClass = receiverB->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const receiverBMarker = receiverB->getAttributeHandle(
      receiverBObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const receiverBInteractionClass = receiverB->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverAObjectClass.isValid());
  REQUIRE(receiverAMarker.isValid());
  REQUIRE(receiverAInteractionClass.isValid());
  REQUIRE(receiverBObjectClass.isValid());
  REQUIRE(receiverBMarker.isValid());
  REQUIRE(receiverBInteractionClass.isValid());

  InteractionClassHandleSet const directedClasses{interactionClass};
  InteractionClassHandleSet const receiverADirectedClasses{receiverAInteractionClass};
  InteractionClassHandleSet const receiverBDirectedClasses{receiverBInteractionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiverA->subscribeObjectClassAttributes(
      receiverAObjectClass,
      {receiverAMarker}));
  REQUIRE_NOTHROW(receiverB->subscribeObjectClassAttributes(
      receiverBObjectClass,
      {receiverBMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(
      interactionClass,
      TIMESTAMP));
  REQUIRE_NOTHROW(receiverA->subscribeObjectClassDirectedInteractions(
      receiverAObjectClass,
      receiverADirectedClasses,
      true));
  REQUIRE_NOTHROW(receiverB->subscribeObjectClassDirectedInteractions(
      receiverBObjectClass,
      receiverBDirectedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(receiverAReports.objectDiscoveryReports.size() == 1U);
  REQUIRE(receiverBReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiverA->enableTimeConstrained());
  REQUIRE_FALSE(receiverA->evokeCallback(0.0));
  REQUIRE_NOTHROW(receiverB->enableTimeConstrained());
  REQUIRE_FALSE(receiverB->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const saved = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(saved.isValid());
  REQUIRE(receiverAReports.directedInteractionReports.empty());
  REQUIRE(receiverBReports.directedInteractionReports.empty());

  // Establish a constrained save frontier below the timestamp-eight passel.
  REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(
      rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiverA->federateSaveBegun());
  REQUIRE_NOTHROW(receiverB->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiverA->federateSaveComplete());
  REQUIRE_NOTHROW(receiverB->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverAReports.federationSavedReportCount == 1U);
  REQUIRE(receiverBReports.federationSavedReportCount == 1U);

  // A post-save designator is terminalized before restore. The restored image
  // must discard that terminal record while retaining both saved recipient
  // projections and the saved live retraction ledger.
  auto const postSave = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(8));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(saved.encode()) !=
          variableLengthDataBytes(postSave.encode()));
  REQUIRE_NOTHROW(publisher->retract(postSave));
  REQUIRE_FALSE(receiverA->evokeCallback(0.0));
  REQUIRE_FALSE(receiverB->evokeCallback(0.0));
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiverA->federateRestoreComplete());
  REQUIRE_NOTHROW(receiverB->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverAReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverBReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverAReports.directedInteractionReports.empty());
  REQUIRE(receiverBReports.directedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  receiverAReports.callbackOrder.clear();
  receiverBReports.callbackOrder.clear();
  REQUIRE_NOTHROW(receiverA->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(8)));
  while (receiverA->evokeCallback(0.0)) {
  }
  REQUIRE(receiverAReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverAReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverAReports.callbackOrder ==
          std::vector<std::string>{"directed", "flush-grant"});
  REQUIRE(receiverBReports.directedInteractionReports.empty());

  REQUIRE_NOTHROW(receiverB->flushQueueRequest(
      rti1516_2025::HLAinteger64Time(8)));
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(receiverBReports.directedInteractionReports.size() == 1U);
  REQUIRE(receiverBReports.flushQueueGrantReports.size() == 1U);
  REQUIRE(receiverBReports.callbackOrder ==
          std::vector<std::string>{"directed", "flush-grant"});

  auto const reliable = publisher->getTransportationTypeHandle(
      standard_hla::mom::reliable);
  for (auto const& report : {
           std::cref(receiverAReports.directedInteractionReports.front()),
           std::cref(receiverBReports.directedInteractionReports.front())}) {
    REQUIRE(report.get().interactionClass == interactionClass);
    REQUIRE(report.get().objectInstance == target);
    REQUIRE(report.get().parameterValues.empty());
    REQUIRE(variableLengthDataBytes(report.get().userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes, tagBytes + sizeof(tagBytes)));
    REQUIRE(report.get().transportationType == reliable);
    REQUIRE(report.get().producingFederate == publisherHandle);
    REQUIRE(report.get().timeImplementationName == standard_hla::mom::integer64_time);
    REQUIRE(report.get().timeValue == L"8");
    REQUIRE(report.get().sentOrderType == TIMESTAMP);
    REQUIRE(report.get().receivedOrderType == TIMESTAMP);
    REQUIRE(report.get().retractionSupplied);
    REQUIRE(report.get().retractionValid);
  }

  REQUIRE_NOTHROW(publisher->retract(saved));
  while (receiverA->evokeCallback(0.0)) {
  }
  while (receiverB->evokeCallback(0.0)) {
  }
  REQUIRE(receiverAReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverBReports.requestRetractionReports.size() == 1U);
  REQUIRE(receiverAReports.requestRetractionReports.front().retractionValid);
  REQUIRE(receiverBReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverAReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(saved.encode()));
  REQUIRE(variableLengthDataBytes(
              receiverBReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(saved.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(saved),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiverA->unsubscribeObjectClassDirectedInteractions(
      receiverAObjectClass,
      receiverADirectedClasses));
  REQUIRE_NOTHROW(receiverB->unsubscribeObjectClassDirectedInteractions(
      receiverBObjectClass,
      receiverBDirectedClasses));
  REQUIRE_NOTHROW(receiverA->unsubscribeObjectClassAttributes(
      receiverAObjectClass,
      {receiverAMarker}));
  REQUIRE_NOTHROW(receiverB->unsubscribeObjectClassAttributes(
      receiverBObjectClass,
      {receiverBMarker}));
  REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiverA->disconnect());
  REQUIRE_NOTHROW(receiverB->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore restores a saved live TSO retraction record",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][time-management][tso]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.retract][rti.service.flush-queue-request]"
    "[federate.callback.receive-interaction][federate.callback.request-retraction]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const parameterBytes[] = {0x5A, 0x52};
  unsigned char const tagBytes[] = {0x53, 0x4E, 0x50};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"live-tso-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-live-retraction-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-live-retraction-receiver", L"subscriber", federationName));

  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = publisher->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(publisher->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClass(interactionClass));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const retraction = publisher->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(retraction.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());

  // The constrained recipient must be Time Advancing before the untimed save
  // can initiate. Its target stays below the active regulator's GALT and well
  // below the live payload timestamp, so this does not deliver or alter the
  // payload that the snapshot must retain.
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));

  // The completed snapshot contains both the queued interaction and its
  // live ledger entry.  Retract it after saving to ensure restore must replace
  // terminal post-save state rather than merely preserve an allocator floor.
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1);
  REQUIRE(receiverReports.federationSavedReportCount == 1);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE(receiverReports.requestRetractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1);
  REQUIRE(receiverReports.federationRestoredReportCount == 1);

  // Flush Queue Request reaches the restored callback boundary without moving
  // the producer's strict retraction lower boundary. Both the original payload
  // and the same public designator must therefore be restored and remain
  // legally retractable.
  REQUIRE_NOTHROW(receiver->flushQueueRequest(rti1516_2025::HLAinteger64Time(6)));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.size() == 1);
  REQUIRE(receiverReports.timestampedInteractionReports.front().timeValue == L"6");
  REQUIRE(receiverReports.timestampedInteractionReports.front().retractionValid);

  REQUIRE_NOTHROW(publisher->retract(retraction));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.requestRetractionReports.size() == 1);
  REQUIRE(receiverReports.requestRetractionReports.front().retractionValid);
  REQUIRE(variableLengthDataBytes(
              receiverReports.requestRetractionReports.front().encodedRetraction) ==
          variableLengthDataBytes(retraction.encode()));
  REQUIRE_THROWS_AS(
      publisher->retract(retraction),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal TSO retraction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][time-management][tso][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction]"
    "[rti.service.retract]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "parameter-handle-provider-fom.xml")
                             .wstring();
  unsigned char const parameterBytes[] = {0x54, 0x4D};
  unsigned char const tagBytes[] = {0x54, 0x4F, 0x4D};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-tso-retraction-baseline";

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-terminal-retraction-publisher", L"publisher", federationName));

  auto const interactionClass = rti->getInteractionClassHandle(
      fixture_hla::fom::parameter_fixture_child_interaction);
  auto const identifier = rti->getParameterHandle(interactionClass, fixture_hla::fixture::identifier);
  REQUIRE(interactionClass.isValid());
  REQUIRE(identifier.isValid());
  ParameterHandleValueMap parameterValues;
  parameterValues.emplace(
      identifier,
      VariableLengthData(parameterBytes, sizeof(parameterBytes)));
  REQUIRE_NOTHROW(rti->publishInteractionClass(interactionClass));
  REQUIRE_NOTHROW(rti->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const terminal = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(terminal.isValid());
  REQUIRE_NOTHROW(rti->retract(terminal));
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Save the lightweight terminal record, then create later live traffic so
  // restore must replace the retraction index rather than merely accept the
  // same process-local handle again.
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1);

  auto const postSave = rti->sendInteraction(
      interactionClass,
      parameterValues,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1);

  // Terminal classification is state in the saved ledger, not an inference
  // from current queue contents. The post-save handle, by contrast, belongs
  // to discarded traffic and must not alias that tombstone.
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      rti->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped attribute retraction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-attribute-update][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const valueBytes[] = {0x54, 0x41, 0x2D, 0x54};
  unsigned char const tagBytes[] = {0x41, 0x54, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-attribute-retraction-baseline";

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-terminal-attribute-publisher", L"publisher", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const attribute = rti->getAttributeHandle(objectClass, fixture_hla::fixture::reliable_base_a);
  REQUIRE(objectClass.isValid());
  REQUIRE(attribute.isValid());
  AttributeHandleSet const attributes{attribute};
  AttributeHandleValueMap values;
  values.emplace(attribute, VariableLengthData(valueBytes, sizeof(valueBytes)));
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->changeDefaultAttributeOrderType(objectClass, attributes, TIMESTAMP));
  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = rti->registerObjectInstance(objectClass));
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const terminal = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(terminal.isValid());
  REQUIRE_NOTHROW(rti->retract(terminal));
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Save the lightweight terminal record, then issue fresh traffic so restore
  // must recover the saved classification rather than merely preserve an
  // allocator floor or accept a post-save designator.
  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1U);

  auto const postSave = rti->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      rti->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(rti->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped object deletion classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][time-management][tso][timestamped-object-deletion][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.publish-object-class-attributes]"
    "[rti.service.reserve-object-instance-name][rti.service.register-object-instance]"
    "[rti.service.delete-object-instance][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador reports;
  auto rti = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                          "cpp" /
                          "tests" /
                          "data" /
                          "attribute-update-passel-fom.xml")
                             .wstring();
  unsigned char const tagBytes[] = {0x54, 0x44, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const objectName = L"Umbra.RestoreTerminalTimestampedDeletion";
  std::wstring const saveLabel = L"terminal-deletion-retraction-baseline";

  REQUIRE_NOTHROW(rti->connect(reports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      rti->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(rti->joinFederationExecution(
      L"restore-terminal-deletion-owner", L"owner", federationName));

  auto const objectClass = rti->getObjectClassHandle(
      fixture_hla::fom::attribute_fixture_child);
  auto const reliable = rti->getAttributeHandle(objectClass, fixture_hla::fixture::reliable_base_a);
  auto const bestEffort = rti->getAttributeHandle(objectClass, fixture_hla::fixture::best_effort_base);
  AttributeHandleSet const attributes{reliable, bestEffort};
  REQUIRE(objectClass.isValid());
  REQUIRE(reliable.isValid());
  REQUIRE(bestEffort.isValid());
  REQUIRE_NOTHROW(rti->publishObjectClassAttributes(objectClass, attributes));
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  ObjectInstanceHandle original;
  REQUIRE_NOTHROW(original = rti->registerObjectInstance(objectClass, objectName));
  REQUIRE(original.isValid());
  REQUIRE_NOTHROW(rti->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(rti->evokeCallback(0.0));

  auto const terminal = rti->deleteObjectInstance(
      original,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(terminal.isValid());

  // With no surviving recipient the timestamped deletion still owns an
  // execution-wide designator. Crossing the strict timestamp/lookahead bound
  // terminalizes it and releases the deleted object's name before the save.
  REQUIRE_NOTHROW(rti->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(rti->requestFederationSave(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateSaveBegun());
  REQUIRE_NOTHROW(rti->federateSaveComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationSavedReportCount == 1U);

  // A new joined-lifetime object may reuse the released name. Its deletion
  // handle belongs to post-save state and must not survive the restore.
  REQUIRE_NOTHROW(rti->reserveObjectInstanceName(objectName));
  REQUIRE_FALSE(rti->evokeCallback(0.0));
  ObjectInstanceHandle postSaveObject;
  REQUIRE_NOTHROW(postSaveObject = rti->registerObjectInstance(objectClass, objectName));
  auto const postSave = rti->deleteObjectInstance(
      postSaveObject,
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(rti->requestFederationRestore(saveLabel));
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(rti->federateRestoreComplete());
  while (rti->evokeCallback(0.0)) {
  }
  REQUIRE(reports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      rti->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      rti->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);
  REQUIRE_THROWS_AS(
      rti->getObjectInstanceHandle(objectName),
      rti1516_2025::ObjectInstanceNotKnown);

  REQUIRE_NOTHROW(rti->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
  REQUIRE_NOTHROW(rti->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(rti->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped directed interaction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][directed][time-management][tso]"
    "[timestamped-directed-interaction][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.publish-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.register-object-instance][rti.service.enable-time-constrained]"
    "[rti.service.enable-time-regulation][rti.service.time-advance-request]"
    "[rti.service.send-directed-interaction][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const objectConsumer = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                               "cpp" / "tests" / "data" /
                               "directed-interaction-object-consumer-fom.xml")
                                  .wstring();
  auto const interactionProvider = (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) /
                                    "cpp" / "tests" / "data" /
                                    "directed-interaction-interaction-provider-fom.xml")
                                       .wstring();
  unsigned char const tagBytes[] = {0x54, 0x44, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-directed-retraction-baseline";
  InteractionClassHandleSet directedClasses;

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(publisher->createFederationExecution(
      federationName,
      std::vector<std::wstring>{objectConsumer, interactionProvider},
      standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-terminal-directed-publisher", L"publisher", federationName));

  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-terminal-directed-receiver", L"subscriber", federationName));
  auto const objectClass = publisher->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const marker = publisher->getAttributeHandle(objectClass, fixture_hla::fixture::directed_target_marker);
  auto const interactionClass = publisher->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  auto const receiverObjectClass = receiver->getObjectClassHandle(
      fixture_hla::fom::directed_fixture_object);
  auto const receiverMarker = receiver->getAttributeHandle(
      receiverObjectClass,
      fixture_hla::fixture::directed_target_marker);
  auto const receiverInteractionClass = receiver->getInteractionClassHandle(
      fixture_hla::fom::directed_fixture_interaction);
  REQUIRE(objectClass.isValid());
  REQUIRE(marker.isValid());
  REQUIRE(interactionClass.isValid());
  REQUIRE(receiverObjectClass.isValid());
  REQUIRE(receiverMarker.isValid());
  REQUIRE(receiverInteractionClass.isValid());
  directedClasses.insert(interactionClass);
  InteractionClassHandleSet receiverDirectedClasses{receiverInteractionClass};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(objectClass, {marker}));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
      objectClass,
      directedClasses));
  REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses,
      true));

  ObjectInstanceHandle target;
  REQUIRE_NOTHROW(target = publisher->registerObjectInstance(objectClass));
  REQUIRE(target.isValid());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->changeInteractionOrderType(interactionClass, TIMESTAMP));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  auto const terminal = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(terminal.isValid());
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(terminal));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.directedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Keep one separate live passel to provide the constrained recipient's
  // save-admission frontier. The terminal tombstone itself has no pending
  // recipient work after the successful Retract.
  auto const admission = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(admission.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));

  // Save the lightweight terminal record, then issue distinct directed
  // traffic. Restore must retain the saved classification without allowing
  // the discarded post-save designator to alias it.
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(admission));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  auto const postSave = publisher->sendDirectedInteraction(
      interactionClass,
      target,
      ParameterHandleValueMap{},
      tag,
      rti1516_2025::HLAinteger64Time(6));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassDirectedInteractions(
      receiverObjectClass,
      receiverDirectedClasses));
  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributes(
      receiverObjectClass,
      {receiverMarker}));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped regional interaction classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[interaction-management][ddm][time-management][tso]"
    "[timestamped-regional-interaction][explicit-source][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.send-interaction-with-regions]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.subscribe-interaction-class-with-regions]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const parameterBytes[] = {0x54, 0x52, 0x2D, 0x54};
  unsigned char const tagBytes[] = {0x52, 0x54, 0x2D, 0x54};
  ParameterHandleValueMap parameterValues;
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-regional-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-terminal-regional-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-terminal-regional-receiver", L"subscriber", federationName));
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
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{serverId});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      serverId,
      RangeBounds(0UL, 10UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      serverId,
      RangeBounds(5UL, 15UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->subscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  REQUIRE_FALSE(publisher->evokeCallback(0.0));

  auto const terminal = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(terminal.isValid());
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(terminal));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.timestampedInteractionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // A separate live regional passel provides the constrained save-admission
  // frontier; the terminal tombstone has no pending recipient state now.
  auto const admission = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(admission.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(admission));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));

  auto const postSave = publisher->sendInteractionWithRegions(
      interactionClass,
      parameterValues,
      RegionHandleSet{publisherRegion},
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(receiver->unsubscribeInteractionClassWithRegions(
      interactionClass,
      RegionHandleSet{receiverRegion}));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}

TEST_CASE(
    "Embedded federation restore preserves a terminal timestamped regional attribute classification",
    "[integration][development-profile][federation-management][save-restore]"
    "[object-management][ddm][time-management][tso]"
    "[timestamped-regional-attribute-update][explicit-source][tombstone]"
    "[rti.service.request-federation-save][rti.service.request-federation-restore]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.federate-restore-complete][rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.change-default-attribute-order-type]"
    "[rti.service.register-object-instance-with-regions]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unsubscribe-object-class-attributes-with-regions]"
    "[rti.service.unassociate-regions-for-updates]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications]"
    "[rti.service.delete-region]"
    "[rti.service.set-convey-region-designator-sets-switch]"
    "[rti.service.enable-time-constrained][rti.service.enable-time-regulation]"
    "[rti.service.time-advance-request][rti.service.retract]"
    "[federate.callback.federation-saved][federate.callback.federation-restored]") {
  ReportingFederateAmbassador publisherReports;
  ReportingFederateAmbassador receiverReports;
  auto publisher = makeRti();
  auto receiver = makeRti();
  auto const federationName = nextFederationName();
  auto const fomModule = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  unsigned char const valueBytes[] = {0x54, 0x52, 0x2D, 0x41};
  unsigned char const tagBytes[] = {0x52, 0x41, 0x2D, 0x54};
  VariableLengthData const tag(tagBytes, sizeof(tagBytes));
  std::wstring const saveLabel = L"terminal-regional-attribute-retraction-baseline";

  REQUIRE_NOTHROW(publisher->connect(publisherReports, HLA_EVOKED));
  REQUIRE_NOTHROW(receiver->connect(receiverReports, HLA_EVOKED));
  REQUIRE_NOTHROW(
      publisher->createFederationExecution(federationName, fomModule, standard_hla::mom::integer64_time));
  REQUIRE_NOTHROW(publisher->joinFederationExecution(
      L"restore-terminal-regional-attribute-publisher", L"publisher", federationName));
  REQUIRE_NOTHROW(receiver->joinFederationExecution(
      L"restore-terminal-regional-attribute-receiver", L"subscriber", federationName));

  auto const soda = publisher->getObjectClassHandle(fixture_hla::fom::food_drink_soda);
  auto const flavor = publisher->getAttributeHandle(soda, fixture_hla::fixture::flavor);
  auto const sodaFlavor = publisher->getDimensionHandle(fixture_hla::fixture::soda_flavor);
  REQUIRE(soda.isValid());
  REQUIRE(flavor.isValid());
  REQUIRE(sodaFlavor.isValid());
  AttributeHandleSet const flavorOnly{flavor};
  REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(soda, flavorOnly));
  REQUIRE_NOTHROW(publisher->changeDefaultAttributeOrderType(soda, flavorOnly, TIMESTAMP));

  auto const publisherRegion = publisher->createRegion(DimensionHandleSet{sodaFlavor});
  auto const receiverRegion = receiver->createRegion(DimensionHandleSet{sodaFlavor});
  REQUIRE_NOTHROW(publisher->setRangeBounds(
      publisherRegion,
      sodaFlavor,
      RangeBounds(0UL, 2UL)));
  REQUIRE_NOTHROW(publisher->commitRegionModifications(RegionHandleSet{publisherRegion}));
  REQUIRE_NOTHROW(receiver->setRangeBounds(
      receiverRegion,
      sodaFlavor,
      RangeBounds(1UL, 3UL)));
  REQUIRE_NOTHROW(receiver->commitRegionModifications(RegionHandleSet{receiverRegion}));
  AttributeHandleSetRegionHandleSetPairVector const publisherPair{{
      flavorOnly,
      RegionHandleSet{publisherRegion},
  }};
  AttributeHandleSetRegionHandleSetPairVector const receiverPair{{
      flavorOnly,
      RegionHandleSet{receiverRegion},
  }};
  REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(receiver->setConveyRegionDesignatorSetsSwitch(true));

  ObjectInstanceHandle objectInstance;
  REQUIRE_NOTHROW(objectInstance = publisher->registerObjectInstanceWithRegions(
      soda,
      publisherPair));
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
  REQUIRE_NOTHROW(receiver->enableTimeConstrained());
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE_NOTHROW(publisher->enableTimeRegulation(
      rti1516_2025::HLAinteger64Interval(5)));
  while (publisher->evokeCallback(0.0)) {
  }

  AttributeHandleValueMap values;
  values.emplace(flavor, VariableLengthData(valueBytes, sizeof(valueBytes)));
  auto const terminal = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(terminal.isValid());
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_NOTHROW(publisher->retract(terminal));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  REQUIRE(receiverReports.attributeReflectionReports.empty());
  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);

  // Keep a distinct live regional passel to admit the constrained recipient;
  // the terminal tombstone has no pending recipient state after Retract.
  auto const admission = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(admission.isValid());
  REQUIRE_NOTHROW(receiver->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(1)));
  REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
  while (receiver->evokeCallback(0.0)) {
  }
  while (publisher->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateSaveBegun());
  REQUIRE_NOTHROW(receiver->federateSaveBegun());
  REQUIRE_NOTHROW(publisher->federateSaveComplete());
  REQUIRE_NOTHROW(receiver->federateSaveComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationSavedReportCount == 1U);
  REQUIRE(receiverReports.federationSavedReportCount == 1U);

  REQUIRE_NOTHROW(publisher->retract(admission));
  REQUIRE_FALSE(receiver->evokeCallback(0.0));
  auto const postSave = publisher->updateAttributeValues(
      objectInstance,
      values,
      tag,
      rti1516_2025::HLAinteger64Time(7));
  REQUIRE(postSave.isValid());
  REQUIRE(variableLengthDataBytes(terminal.encode()) !=
          variableLengthDataBytes(postSave.encode()));

  REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE_NOTHROW(publisher->federateRestoreComplete());
  REQUIRE_NOTHROW(receiver->federateRestoreComplete());
  while (publisher->evokeCallback(0.0)) {
  }
  while (receiver->evokeCallback(0.0)) {
  }
  REQUIRE(publisherReports.federationRestoredReportCount == 1U);
  REQUIRE(receiverReports.federationRestoredReportCount == 1U);

  REQUIRE_THROWS_AS(
      publisher->retract(terminal),
      rti1516_2025::MessageCanNoLongerBeRetracted);
  REQUIRE_THROWS_AS(
      publisher->retract(postSave),
      rti1516_2025::InvalidMessageRetractionHandle);

  REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(soda, receiverPair));
  REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(objectInstance, publisherPair));
  REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
  REQUIRE_NOTHROW(publisher->deleteRegion(publisherRegion));
  REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
  REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
  REQUIRE_NOTHROW(receiver->disconnect());
  REQUIRE_NOTHROW(publisher->disconnect());
}
