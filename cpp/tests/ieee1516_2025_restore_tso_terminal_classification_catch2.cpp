#include "ieee1516_2025_federation_management_fixture_support.hpp"

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
