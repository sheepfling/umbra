#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds directed target routing and receive-order delivery under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[directed-interaction][directed-routing][object-visibility-state][object-lifecycle-state]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[process-restart-directed-interaction-routing]"
    "[public-process-restart-directed-interaction-routing]"
    "[callback-immediate]"
    "[multi-federate-callback-ordering]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-interaction-class-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.send-directed-interaction]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.receive-directed-interaction]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[service-report-file][service-reporting]") {
  auto runScenario = [](CallbackModel const callbackModel) {
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
      L"public-directed-interaction-routing-process-restart";
  std::string const tagBytes{"directed-routing"};
  VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  FederateHandle sourceSenderHandle;
  ObjectInstanceHandle sourceTarget;
  std::filesystem::path sourceReportFile;

  auto const drainAll = [](RTIambassador& first,
                            RTIambassador& second,
                            RTIambassador& third) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(second.evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(third.evokeMultipleCallbacks(0.0, 0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador senderReports;
    ReportingFederateAmbassador receiverReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-directed-routing-owner", L"owner", federationName));
    REQUIRE_NOTHROW(sourceSenderHandle = sender->joinFederationExecution(
        L"public-directed-routing-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"public-directed-routing-receiver", L"subscriber", federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*receiver);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

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
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(sender->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributes(objectClass, {marker}));
    REQUIRE_NOTHROW(sender->publishObjectClassDirectedInteractions(
        objectClass, directedClasses));
    REQUIRE_NOTHROW(receiver->subscribeObjectClassDirectedInteractions(
        objectClass, directedClasses, true));

    REQUIRE_NOTHROW(sourceTarget = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceTarget.isValid());
    drainAll(*owner, *sender, *receiver);
    REQUIRE(senderReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance ==
            sourceTarget);

    AttributeHandleValueMap markerValues;
    markerValues.emplace(marker, tag);
    REQUIRE_NOTHROW(owner->updateAttributeValues(sourceTarget, markerValues, tag));
    drainAll(*owner, *sender, *receiver);
    REQUIRE(senderReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
    senderReports.attributeReflectionReports.clear();
    receiverReports.attributeReflectionReports.clear();
    senderReports.callbackOrder.clear();
    receiverReports.callbackOrder.clear();

    REQUIRE_NOTHROW(sender->requestFederationSave(saveLabel));
    drainAll(*owner, *sender, *receiver);
    REQUIRE(ownerReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(senderReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(sender->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(sender->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drainAll(*owner, *sender, *receiver);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(senderReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.objects.size() == 1U);
    REQUIRE(durableImage.objects.front().attributeValuesPresent);
    REQUIRE(durableImage.interactionDeclarations.size() == 2U);
    auto const publishedDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [](auto const& declaration) {
          return declaration.publishedObjectClassDirectedInteractions.size() == 1U;
        });
    auto const subscribedDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [](auto const& declaration) {
          return declaration.subscribedObjectClassDirectedInteractions.size() == 1U;
        });
    REQUIRE(publishedDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(subscribedDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(subscribedDeclaration->subscribedObjectClassDirectedInteractions.front().active);

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
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
    ReportingFederateAmbassador receiverReports;
    auto owner = makeRti();
    auto sender = makeRti();
    auto receiver = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
    REQUIRE_NOTHROW(sender->connect(senderReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(receiver->connect(receiverReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"public-directed-routing-owner", L"owner", federationName));
    FederateHandle freshSenderHandle;
    REQUIRE_NOTHROW(freshSenderHandle = sender->joinFederationExecution(
        L"public-directed-routing-sender", L"publisher", federationName));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"public-directed-routing-receiver", L"subscriber", federationName));
    REQUIRE(freshSenderHandle == sourceSenderHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*sender);
    suppressDeclarationRelevanceAdvisories(*receiver);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshObjectClass = owner->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const freshInteractionClass = sender->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshObjectClass.isValid());
    REQUIRE(freshInteractionClass.isValid());
    REQUIRE_NOTHROW(sender->requestFederationRestore(saveLabel));
    drainAll(*owner, *sender, *receiver);
    REQUIRE(senderReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(receiverReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(senderReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);

    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(sender->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drainAll(*owner, *sender, *receiver);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(senderReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    ownerReports.directedInteractionReports.clear();
    senderReports.directedInteractionReports.clear();
    receiverReports.directedInteractionReports.clear();
    receiverReports.callbackOrder.clear();
    REQUIRE_NOTHROW(sender->sendDirectedInteraction(
        freshInteractionClass,
        sourceTarget,
        ParameterHandleValueMap{},
        tag));
    drainAll(*owner, *sender, *receiver);
    REQUIRE(ownerReports.directedInteractionReports.empty());
    REQUIRE(senderReports.directedInteractionReports.empty());
    REQUIRE(receiverReports.directedInteractionReports.size() == 1U);
    auto const& delivered = receiverReports.directedInteractionReports.front();
    REQUIRE(delivered.interactionClass == freshInteractionClass);
    REQUIRE(delivered.objectInstance == sourceTarget);
    REQUIRE(delivered.parameterValues.empty());
    REQUIRE(variableLengthDataBytes(delivered.userSuppliedTag) ==
            std::vector<unsigned char>(tagBytes.begin(), tagBytes.end()));
    REQUIRE(delivered.transportationType ==
            sender->getTransportationTypeHandle(standard_hla::mom::reliable));
    REQUIRE(delivered.producingFederate == freshSenderHandle);
    REQUIRE(delivered.sentOrderType == RECEIVE);
    REQUIRE(delivered.receivedOrderType == RECEIVE);

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(sender->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(sender->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
} // namespace
