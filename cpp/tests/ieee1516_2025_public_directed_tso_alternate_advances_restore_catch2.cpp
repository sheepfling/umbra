#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry directed TSO restores through FQR TARA and NMRA under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[directed-interaction][directed-routing][time-management][tso][save-restore]"
    "[durable-save][filesystem][process-restart][restore][tso-directed-interaction-state]"
    "[tso-queue-state][tso-retraction-ledger-state][alternate-advance]"
    "[callback-immediate]"
    "[process-restart-directed-interaction-tso-alternate-advances]"
    "[public-process-restart-directed-interaction-tso-alternate-advances]"
    "[service-report-file][service-reporting]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.change-interaction-order-type][rti.service.register-object-instance]"
    "[rti.service.update-attribute-values][rti.service.send-directed-interaction]"
    "[rti.service.enable-time-regulation][rti.service.enable-time-constrained]"
    "[rti.service.time-advance-request][rti.service.flush-queue-request]"
    "[rti.service.time-advance-request-available][rti.service.next-message-request-available]"
    "[rti.service.retract][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-directed-interaction][federate.callback.flush-queue-grant]"
    "[federate.callback.time-advance-grant]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  auto runScenario = [](CallbackModel callbackModel) {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::readTextFile;
  using public_federation_restore_test_support::serviceReportFiles;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto sourceConfiguration =
      configurationForServiceReportDirectory(reportDirectory.path());
  sourceConfiguration.withRtiAddress(L"in-process");
  auto const federationName = nextFederationName();
  auto const objectConsumer =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "directed-interaction-object-consumer-fom.xml")
          .wstring();
  auto const interactionProvider =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "directed-interaction-interaction-provider-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{
      objectConsumer, interactionProvider, switchFom};
  std::wstring const saveLabel =
      L"public-directed-tso-alternate-advances";
  std::string const tagBytes{"alternate-advance-tso"};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceSenderHandle;
  FederateHandle sourceFqrHandle;
  FederateHandle sourceTaraHandle;
  FederateHandle sourceNmraHandle;
  ObjectInstanceHandle sourceTarget;
  rti1516_2025::MessageRetractionHandle sourceRetraction;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [](RTIambassador& owner,
                           RTIambassador& sender,
                           RTIambassador& fqr,
                           RTIambassador& tara,
                           RTIambassador& nmra) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(owner.evokeCallback(0.0));
      static_cast<void>(sender.evokeCallback(0.0));
      static_cast<void>(fqr.evokeCallback(0.0));
      static_cast<void>(tara.evokeCallback(0.0));
      static_cast<void>(nmra.evokeCallback(0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador fqrReports;
    ReportingFederateAmbassador taraReports;
    ReportingFederateAmbassador nmraReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto fqr = makeRti();
    auto tara = makeRti();
    auto nmra = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(fqr->connect(fqrReports, callbackModel));
    REQUIRE_NOTHROW(tara->connect(taraReports, callbackModel));
    REQUIRE_NOTHROW(nmra->connect(nmraReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-directed-alternate-owner", L"owner", federationName));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-directed-alternate-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceFqrHandle = fqr->joinFederationExecution(
        L"public-directed-alternate-fqr", L"subscriber", federationName));
    REQUIRE_NOTHROW(sourceTaraHandle = tara->joinFederationExecution(
        L"public-directed-alternate-tara", L"subscriber", federationName));
    REQUIRE_NOTHROW(sourceNmraHandle = nmra->joinFederationExecution(
        L"public-directed-alternate-nmra", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*fqr);
    suppressDeclarationRelevanceAdvisories(*tara);
    suppressDeclarationRelevanceAdvisories(*nmra);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);
    REQUIRE_NOTHROW(sender->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(sender->setSendServiceReportsToFileSwitch(true));

    auto const objectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const marker = owner->getAttributeHandle(
        objectClass,
        fixture_hla::fixture::directed_target_marker);
    auto const interactionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{interactionClass};
    REQUIRE(objectClass.isValid());
    REQUIRE(marker.isValid());
    REQUIRE(interactionClass.isValid());
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{marker}));
    for (auto* receiver : {sender.get(), fqr.get(), tara.get(), nmra.get()}) {
      REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(
          objectClass, AttributeHandleSet{marker}));
    }
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass, directedClasses));
    REQUIRE_NOTHROW(sender->changeInteractionOrderType(
        interactionClass, TIMESTAMP));
    for (auto* receiver : {fqr.get(), tara.get(), nmra.get()}) {
      REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
          objectClass, directedClasses, true));
    }

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(fqrReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(taraReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(nmraReports.objectDiscoveryReports.size() == 1U);

    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, tag);
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceTarget, markerValues, tag));
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    REQUIRE(fqrReports.attributeReflectionReports.size() == 1U);
    REQUIRE(taraReports.attributeReflectionReports.size() == 1U);
    REQUIRE(nmraReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    fqrReports.attributeReflectionReports.clear();
    taraReports.attributeReflectionReports.clear();
    nmraReports.attributeReflectionReports.clear();

    REQUIRE_NOTHROW(fqr->enableTimeConstrained());
    REQUIRE_FALSE(fqr->evokeCallback(0.0));
    REQUIRE_NOTHROW(tara->enableTimeConstrained());
    REQUIRE_FALSE(tara->evokeCallback(0.0));
    REQUIRE_NOTHROW(nmra->enableTimeConstrained());
    REQUIRE_FALSE(nmra->evokeCallback(0.0));
    REQUIRE_NOTHROW(sender->enableTimeRegulation(
        rti1516_2025::HLAinteger64Interval(1)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));

    REQUIRE_NOTHROW(sourceRetraction = sender->sendDirectedInteraction(
        interactionClass,
        sourceTarget,
        ParameterHandleValueMap{},
        tag,
        rti1516_2025::HLAinteger64Time(9)));
    REQUIRE(sourceRetraction.isValid());
    REQUIRE(fqrReports.directedInteractionReports.empty());
    REQUIRE(taraReports.directedInteractionReports.empty());
    REQUIRE(nmraReports.directedInteractionReports.empty());

    REQUIRE_NOTHROW(sender->requestFederationSave(
        saveLabel,
        rti1516_2025::HLAinteger64Time(7)));
    // HLA_IMMEDIATE executes a grant as soon as its own time-advance
    // request is submitted.  Timed-save admission intentionally requires
    // every constrained member to have a qualifying grant queued before the
    // first one crosses the boundary.  Hold immediate callbacks while
    // staging the five requests, then release them; HLA_EVOKED stages this
    // same set naturally before drainAll().
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(sender->disableCallbacks());
      REQUIRE_NOTHROW(fqr->disableCallbacks());
      REQUIRE_NOTHROW(tara->disableCallbacks());
      REQUIRE_NOTHROW(nmra->disableCallbacks());
    }
    REQUIRE_NOTHROW(owner->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(fqr->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(tara->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    REQUIRE_NOTHROW(nmra->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(7)));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(sender->enableCallbacks());
      REQUIRE_NOTHROW(fqr->enableCallbacks());
      REQUIRE_NOTHROW(tara->enableCallbacks());
      REQUIRE_NOTHROW(nmra->enableCallbacks());
    }
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(fqrReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(taraReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(nmraReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(fqr->federateSaveBegun());
    REQUIRE_NOTHROW(tara->federateSaveBegun());
    REQUIRE_NOTHROW(nmra->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    REQUIRE_NOTHROW(fqr->federateSaveComplete());
    REQUIRE_NOTHROW(tara->federateSaveComplete());
    REQUIRE_NOTHROW(nmra->federateSaveComplete());
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(senderReports.federationSavedReportCount == 1U);
    REQUIRE(fqrReports.federationSavedReportCount == 1U);
    REQUIRE(taraReports.federationSavedReportCount == 1U);
    REQUIRE(nmraReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(durableImage.tsoDirectedInteractionMessages.size() == 1U);
    REQUIRE(durableImage.tsoRequestRetractionRecords.size() == 1U);
    REQUIRE(durableImage.tsoQueueEntries.size() == 3U);
    auto const& savedMessage =
        durableImage.tsoDirectedInteractionMessages.front();
    auto const& savedRetraction =
        durableImage.tsoRequestRetractionRecords.front();
    // The embedded profile's default delayed-subscription switch keeps a
    // live, route-only projection for the joined target owner even though the
    // owner has no directed selector.  It is retained in the durable payload
    // and retraction ledger, while only the three time-constrained selectors
    // enter the temporal queue and receive a callback.
    REQUIRE(savedMessage.recipients.size() == 4U);
    REQUIRE(savedRetraction.recipientStates.size() == 4U);
    auto const fqrValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceFqrHandle);
    auto const taraValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceTaraHandle);
    auto const nmraValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceNmraHandle);
    REQUIRE(fqrValue.has_value());
    REQUIRE(taraValue.has_value());
    REQUIRE(nmraValue.has_value());
    for (auto const recipientId : {*fqrValue, *taraValue, *nmraValue}) {
      REQUIRE(std::any_of(
          savedMessage.recipients.begin(),
          savedMessage.recipients.end(),
          [&](umbra::detail::FederationStateImageTsoDirectedInteractionRecipient const& recipient) {
            return recipient.receivingFederateId == recipientId;
          }));
      REQUIRE(std::any_of(
          savedRetraction.recipientStates.begin(),
          savedRetraction.recipientStates.end(),
          [&](umbra::detail::FederationStateImageTsoRequestRetractionRecipient const& recipient) {
            return recipient.receivingFederateId == recipientId &&
                   recipient.state == 0U;
          }));
      REQUIRE(std::any_of(
          durableImage.tsoQueueEntries.begin(),
          durableImage.tsoQueueEntries.end(),
          [&](umbra::detail::FederationStateImageTsoQueueEntry const& entry) {
            return entry.recipientFederateId == recipientId && entry.phase == 0U;
          }));
    }
    REQUIRE(savedMessage.receivedOrderType ==
            static_cast<std::uint32_t>(TIMESTAMP));
    REQUIRE(readTextFile(sourceReportFile).size() > initialReportText.size());

    REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(fqr->disconnect());
    REQUIRE_NOTHROW(tara->disconnect());
    REQUIRE_NOTHROW(nmra->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador fqrReports;
    ReportingFederateAmbassador taraReports;
    ReportingFederateAmbassador nmraReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto fqr = makeRti();
    auto tara = makeRti();
    auto nmra = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(fqr->connect(fqrReports, callbackModel));
    REQUIRE_NOTHROW(tara->connect(taraReports, callbackModel));
    REQUIRE_NOTHROW(nmra->connect(nmraReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    FederateHandle freshSenderHandle;
    FederateHandle freshFqrHandle;
    FederateHandle freshTaraHandle;
    FederateHandle freshNmraHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-directed-alternate-owner", L"owner", federationName));
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-directed-alternate-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(freshFqrHandle = fqr->joinFederationExecution(
        L"public-directed-alternate-fqr", L"subscriber", federationName));
    REQUIRE_NOTHROW(freshTaraHandle = tara->joinFederationExecution(
        L"public-directed-alternate-tara", L"subscriber", federationName));
    REQUIRE_NOTHROW(freshNmraHandle = nmra->joinFederationExecution(
        L"public-directed-alternate-nmra", L"subscriber", federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    REQUIRE(freshFqrHandle == sourceFqrHandle);
    REQUIRE(freshTaraHandle == sourceTaraHandle);
    REQUIRE(freshNmraHandle == sourceNmraHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*fqr);
    suppressDeclarationRelevanceAdvisories(*tara);
    suppressDeclarationRelevanceAdvisories(*nmra);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile =
        filesAfterFreshJoin.front() == sourceReportFile
            ? filesAfterFreshJoin.back()
            : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);
    REQUIRE_NOTHROW(sender->setServiceReportingSwitch(true));
    REQUIRE_NOTHROW(sender->setSendServiceReportsToFileSwitch(true));

    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(fqrReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(taraReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(nmraReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    REQUIRE_NOTHROW(fqr->federateRestoreComplete());
    REQUIRE_NOTHROW(tara->federateRestoreComplete());
    REQUIRE_NOTHROW(nmra->federateRestoreComplete());
    drainAll(*owner, *sender, *fqr, *tara, *nmra);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(fqrReports.federationRestoredReportCount == 1U);
    REQUIRE(taraReports.federationRestoredReportCount == 1U);
    REQUIRE(nmraReports.federationRestoredReportCount == 1U);
    REQUIRE(fqrReports.directedInteractionReports.empty());
    REQUIRE(taraReports.directedInteractionReports.empty());
    REQUIRE(nmraReports.directedInteractionReports.empty());

    auto const freshInteractionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshInteractionClass.isValid());
    REQUIRE_NOTHROW(sender->timeAdvanceRequest(
        rti1516_2025::HLAinteger64Time(8)));
    REQUIRE_FALSE(sender->evokeCallback(0.0));
    fqrReports.callbackOrder.clear();
    taraReports.callbackOrder.clear();
    nmraReports.callbackOrder.clear();
    REQUIRE_NOTHROW(fqr->flushQueueRequest(
        rti1516_2025::HLAinteger64Time(10)));
    REQUIRE_NOTHROW(tara->timeAdvanceRequestAvailable(
        rti1516_2025::HLAinteger64Time(9)));
    REQUIRE_NOTHROW(nmra->nextMessageRequestAvailable(
        rti1516_2025::HLAinteger64Time(10)));
    drainAll(*owner, *sender, *fqr, *tara, *nmra);

    REQUIRE(fqrReports.directedInteractionReports.size() == 1U);
    REQUIRE(taraReports.directedInteractionReports.size() == 1U);
    REQUIRE(nmraReports.directedInteractionReports.size() == 1U);
    REQUIRE(fqrReports.flushQueueGrantReports.size() == 1U);
    REQUIRE(taraReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(nmraReports.timeAdvanceGrantReports.size() == 1U);
    REQUIRE(fqrReports.flushQueueGrantReports.front().value == L"9");
    REQUIRE(fqrReports.flushQueueGrantReports.front().optimisticValue == L"9");
    REQUIRE(taraReports.timeAdvanceGrantReports.front().value == L"9");
    REQUIRE(nmraReports.timeAdvanceGrantReports.front().value == L"9");
    REQUIRE(fqrReports.callbackOrder ==
            std::vector<std::string>{"directed", "flush-grant"});
    REQUIRE(taraReports.callbackOrder ==
            std::vector<std::string>{"directed", "grant"});
    REQUIRE(nmraReports.callbackOrder ==
            std::vector<std::string>{"directed", "grant"});

    auto requireDirected = [&](auto const& report) {
      REQUIRE(report.interactionClass == freshInteractionClass);
      REQUIRE(report.objectInstance == sourceTarget);
      REQUIRE(report.parameterValues.empty());
      REQUIRE(variableLengthDataBytes(report.userSuppliedTag) ==
              std::vector<unsigned char>(tagBytes.begin(), tagBytes.end()));
      REQUIRE(report.transportationType == sender->getTransportationTypeHandle(
          standard_hla::mom::reliable));
      REQUIRE(report.producingFederate == freshSenderHandle);
      REQUIRE(report.timeImplementationName == standard_hla::mom::integer64_time);
      REQUIRE(report.timeValue == L"9");
      REQUIRE(report.sentOrderType == TIMESTAMP);
      REQUIRE(report.receivedOrderType == TIMESTAMP);
      REQUIRE(report.retractionSupplied);
      REQUIRE(report.retractionValid);
    };
    requireDirected(fqrReports.directedInteractionReports.front());
    requireDirected(taraReports.directedInteractionReports.front());
    requireDirected(nmraReports.directedInteractionReports.front());
    REQUIRE_THROWS_AS(
        sender->retract(sourceRetraction),
        rti1516_2025::MessageCanNoLongerBeRetracted);
    REQUIRE(fqrReports.requestRetractionReports.empty());
    REQUIRE(taraReports.requestRetractionReports.empty());
    REQUIRE(nmraReports.requestRetractionReports.empty());
    REQUIRE(readTextFile(freshReportFile).size() > freshInitialReportText.size());
    REQUIRE(serviceReportFiles(reportDirectory.path()) == filesAfterFreshJoin);

    REQUIRE_NOTHROW(fqr->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(tara->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(nmra->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(fqr->disconnect());
    REQUIRE_NOTHROW(tara->disconnect());
    REQUIRE_NOTHROW(nmra->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}

}
