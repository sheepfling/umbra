#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace restore_support = public_federation_restore_test_support;

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore retracts delivered timestamped object deletion for one recipient under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][time-management][tso]"
    "[object-deletion][tso-retraction-state][request-retraction]"
    "[process-restart-object-deletion-retraction]"
    "[public-process-restart-object-deletion-retraction]"
    "[public-object-deletion-retraction-restore]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.delete-object-instance][rti.service.flush-queue-request]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.request-federation-save][rti.service.time-advance-request]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.retract][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.remove-object-instance]"
    "[federate.callback.request-retraction][federate.callback.flush-queue-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[federate.callback.time-advance-grant]"
    "[rti.service.disable-callbacks][rti.service.enable-callbacks]"
    "[callback-immediate]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = restore_support::temporaryFederationSaveDirectory();
  auto const saveStore = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      saveDirectory.path());
  auto const reportDirectory = restore_support::temporaryServiceReportDirectory();
  auto sourceConfiguration = restore_support::configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  std::wstring const saveLabel = L"public-fresh-object-deletion-retraction";
  unsigned char const baselineBytes[] = {0x42, 0x41, 0x53, 0x45};
  unsigned char const deleteTagBytes[] = {0x52, 0x45, 0x54, 0x52};
  VariableLengthData const baselineTag(baselineBytes, sizeof(baselineBytes));
  VariableLengthData const deleteTag(deleteTagBytes, sizeof(deleteTagBytes));
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObjectInstance;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [&](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
      static_cast<void>(third.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-receiver-b",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAtJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());

    auto const sourceServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const sourcePrivilegeToDelete = owner->getAttributeHandle(
        sourceServer,
        standard_hla::mom::privilege_to_delete_object);
    auto const receiverAServer = receiverA->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const receiverAPrivilegeToDelete = receiverA->getAttributeHandle(
        receiverAServer,
        standard_hla::mom::privilege_to_delete_object);
    auto const receiverBServer = receiverB->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const receiverBPrivilegeToDelete = receiverB->getAttributeHandle(
        receiverBServer,
        standard_hla::mom::privilege_to_delete_object);
    REQUIRE(sourceServer.isValid());
    REQUIRE(sourcePrivilegeToDelete.isValid());
    REQUIRE(receiverAServer.isValid());
    REQUIRE(receiverAPrivilegeToDelete.isValid());
    REQUIRE(receiverBServer.isValid());
    REQUIRE(receiverBPrivilegeToDelete.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        sourceServer,
        AttributeHandleSet{sourcePrivilegeToDelete}));
    REQUIRE_NOTHROW(receiverA->subscribeObjectClassAttributes(
        receiverAServer,
        AttributeHandleSet{receiverAPrivilegeToDelete},
        true));
    REQUIRE_NOTHROW(receiverB->subscribeObjectClassAttributes(
        receiverBServer,
        AttributeHandleSet{receiverBPrivilegeToDelete},
        true));

    REQUIRE_NOTHROW(sourceObjectInstance = owner->registerObjectInstance(sourceServer));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverBReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap baselineValues;
    baselineValues.emplace(
        sourcePrivilegeToDelete,
        VariableLengthData(baselineBytes, sizeof(baselineBytes)));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance,
        baselineValues,
        baselineTag));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverAReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverBReports.attributeReflectionReports.size() == 1U);
    receiverAReports.attributeReflectionReports.clear();
    receiverBReports.attributeReflectionReports.clear();
    receiverAReports.callbackOrder.clear();
    receiverBReports.callbackOrder.clear();

    REQUIRE_NOTHROW(receiverA->enableTimeConstrained());
    REQUIRE_FALSE(receiverA->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverB->enableTimeConstrained());
    REQUIRE_FALSE(receiverB->evokeCallback(0.0));
    REQUIRE_NOTHROW(owner->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));

    REQUIRE_NOTHROW(sourceRetraction = owner->deleteObjectInstance(
        sourceObjectInstance,
        deleteTag,
        rti1516_2025::HLAinteger64Time(9)));
    REQUIRE(sourceRetraction.isValid());
    REQUIRE_NOTHROW(owner->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes an admitted grant synchronously. Hold every
    // route while the three members cross the timed-save boundary so the
    // durable retraction image has the same membership and queue frontier as
    // HLA_EVOKED.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(receiverA->disableCallbacks());
      REQUIRE_NOTHROW(receiverB->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverA->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(receiverA->enableCallbacks());
      REQUIRE_NOTHROW(receiverB->enableCallbacks());
    }
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverAReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverBReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(receiverA->federateSaveBegun());
    REQUIRE_NOTHROW(receiverB->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(receiverA->federateSaveComplete());
    REQUIRE_NOTHROW(receiverB->federateSaveComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(receiverAReports.federationSavedReportCount == 1U);
    REQUIRE(receiverBReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(durableImage.objects.size() == 1U);
    REQUIRE(durableImage.objects.front().pendingTimestampedDeletionMessageId.has_value());
    REQUIRE(durableImage.tsoObjectDeletionMessages.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.front().recipientStates.size() == 2U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 2U);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry = std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
        nullptr,
        saveStore);
    restore_support::ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador receiverAReports;
    ReportingFederateAmbassador receiverBReports;
    auto owner = makeRti();
    auto receiverA = makeRti();
    auto receiverB = makeRti();
    auto freshConfiguration = restore_support::configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(receiverA->connect(receiverAReports, callbackModel));
    REQUIRE_NOTHROW(receiverB->connect(receiverBReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(receiverA->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-receiver-a",
        L"subscriber",
        federationName));
    REQUIRE_NOTHROW(receiverB->joinFederationExecution(
        L"public-fresh-object-deletion-retraction-receiver-b",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*receiverA);
    suppressDeclarationRelevanceAdvisories(*receiverB);

    auto const filesAfterFreshJoin = restore_support::serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(), sourceReportFile) !=
            filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);

    auto const freshServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const freshPrivilegeToDelete = owner->getAttributeHandle(
        freshServer,
        standard_hla::mom::privilege_to_delete_object);
    REQUIRE(freshServer.isValid());
    REQUIRE(freshPrivilegeToDelete.isValid());
    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverA->federateRestoreComplete());
    REQUIRE_NOTHROW(receiverB->federateRestoreComplete());
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverBReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverAReports.objectRemovalReports.empty());
    REQUIRE(receiverBReports.objectRemovalReports.empty());

    receiverAReports.callbackOrder.clear();
    receiverBReports.callbackOrder.clear();
    REQUIRE_NOTHROW(receiverA->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(9)));
    while (receiverA->evokeCallback(0.0)) {
    }
    REQUIRE(receiverAReports.objectRemovalReports.size() == 1U);
    REQUIRE(receiverAReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(receiverAReports.callbackOrder ==
            std::vector<std::string>{"remove", "flush-grant"});
    REQUIRE(receiverBReports.objectRemovalReports.empty());
    REQUIRE_THROWS_AS(
        receiverA->getObjectInstanceHandle(
            owner->getObjectInstanceName(sourceObjectInstance)),
        rti1516_2025::ObjectInstanceNotKnown);

    REQUIRE_NOTHROW(owner->retract(sourceRetraction));
    while (receiverA->evokeCallback(0.0)) {
    }
    while (receiverB->evokeCallback(0.0)) {
    }
    REQUIRE(receiverAReports.requestRetractionReports.size() == 1U);
    REQUIRE(receiverAReports.requestRetractionReports.front().retractionValid);
    REQUIRE(variableLengthDataBytes(
                receiverAReports.requestRetractionReports.front().encodedRetraction) ==
            variableLengthDataBytes(sourceRetraction.encode()));
    REQUIRE(receiverBReports.requestRetractionReports.empty());
    REQUIRE(receiverAReports.objectRemovalReports.size() == 1U);
    REQUIRE(receiverBReports.objectRemovalReports.empty());

    auto const objectInstanceName = owner->getObjectInstanceName(sourceObjectInstance);
    REQUIRE(receiverA->getObjectInstanceHandle(objectInstanceName) == sourceObjectInstance);
    REQUIRE(receiverB->getObjectInstanceHandle(objectInstanceName) == sourceObjectInstance);
    REQUIRE(owner->isAttributeOwnedByFederate(sourceObjectInstance, freshPrivilegeToDelete));

    REQUIRE_NOTHROW(owner->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(8)));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE_NOTHROW(receiverB->timeAdvanceRequest(rti1516_2025::HLAinteger64Time(9)));
    drainAll(*owner, *receiverA, *receiverB);
    REQUIRE(receiverBReports.objectRemovalReports.empty());
    REQUIRE(receiverBReports.requestRetractionReports.empty());
    REQUIRE(restore_support::serviceReportFiles(reportDirectory.path()) == filesAfterFreshJoin);

    REQUIRE_NOTHROW(receiverA->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiverB->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiverA->disconnect());
    REQUIRE_NOTHROW(receiverB->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(rti1516_2025::HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
}
