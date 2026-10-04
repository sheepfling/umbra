#include "ieee1516_2025_federation_management_fixture_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rehydrates directed interaction declaration and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[directed-interaction][directed-declaration][declaration-management]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[process-restart-directed-interaction-declaration]"
    "[public-process-restart-directed-interaction-declaration]"
    "[callback-immediate]"
    "[multi-federate-callback-ordering]"
    "[rti.service.get-object-class-handle][rti.service.get-interaction-class-handle]"
    "[rti.service.publish-object-class-directed-interactions]"
    "[rti.service.subscribe-object-class-directed-interactions]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
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
      L"public-directed-interaction-declaration-process-restart";
  FederateHandle sourcePublisherHandle;
  FederateHandle sourceSubscriberHandle;
  ObjectClassHandle sourceObjectClass;
  InteractionClassHandle sourceInteractionClass;
  std::filesystem::path sourceReportFile;
  std::uint64_t sourcePublisherValue = 0;
  std::uint64_t sourceSubscriberValue = 0;
  std::uint64_t sourceInteractionClassValue = 0;

  auto const drainAll = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeMultipleCallbacks(0.0, 0.0));
      static_cast<void>(second.evokeMultipleCallbacks(0.0, 0.0));
    }
  };

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();

    REQUIRE_NOTHROW(
        publisher->connect(publisherReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourcePublisherHandle = publisher->joinFederationExecution(
        L"public-directed-interaction-declaration-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(sourceSubscriberHandle = subscriber->joinFederationExecution(
        L"public-directed-interaction-declaration-subscriber",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*publisher);
    suppressDeclarationRelevanceAdvisories(*subscriber);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    sourceObjectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    sourceInteractionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    InteractionClassHandleSet const directedClasses{sourceInteractionClass};
    REQUIRE(sourceObjectClass.isValid());
    REQUIRE(sourceInteractionClass.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassDirectedInteractions(
        sourceObjectClass, directedClasses));
    REQUIRE_NOTHROW(subscriber->subscribeObjectClassDirectedInteractions(
        sourceObjectClass, directedClasses, true));

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(subscriber->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(subscriber->federateSaveComplete());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.interactionDeclarationCount == 2U);
    REQUIRE(durableImage.interactionDeclarations.size() == 2U);
    auto const sourcePublisherValueResult =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourcePublisherHandle);
    auto const sourceSubscriberValueResult =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceSubscriberHandle);
    auto const sourceInteractionClassValueResult =
        rti1516_2025::umbra_binding_detail::interactionClassHandleValue(sourceInteractionClass);
    REQUIRE(sourcePublisherValueResult.has_value());
    REQUIRE(sourceSubscriberValueResult.has_value());
    REQUIRE(sourceInteractionClassValueResult.has_value());
    sourcePublisherValue = *sourcePublisherValueResult;
    sourceSubscriberValue = *sourceSubscriberValueResult;
    sourceInteractionClassValue = *sourceInteractionClassValueResult;
    auto const publisherDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == sourcePublisherValue;
        });
    auto const subscriberDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == sourceSubscriberValue;
        });
    REQUIRE(publisherDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(subscriberDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(publisherDeclaration->publishedObjectClassDirectedInteractions.size() == 1U);
    REQUIRE(publisherDeclaration->publishedObjectClassDirectedInteractions.front().interactionClassHandle ==
            sourceInteractionClassValue);
    REQUIRE(subscriberDeclaration->subscribedObjectClassDirectedInteractions.size() == 1U);
    REQUIRE(subscriberDeclaration->subscribedObjectClassDirectedInteractions.front().interactionClassHandle ==
            sourceInteractionClassValue);
    REQUIRE(subscriberDeclaration->subscribedObjectClassDirectedInteractions.front().active);
    REQUIRE(publisherDeclaration->interactionTransportationTypes.empty());
    REQUIRE(subscriberDeclaration->interactionTransportationTypes.empty());

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador subscriberReports;
    auto publisher = makeRti();
    auto subscriber = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(
        publisher->connect(publisherReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(subscriber->connect(subscriberReports, callbackModel));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshPublisherHandle;
    FederateHandle freshSubscriberHandle;
    REQUIRE_NOTHROW(freshPublisherHandle = publisher->joinFederationExecution(
        L"public-directed-interaction-declaration-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(freshSubscriberHandle = subscriber->joinFederationExecution(
        L"public-directed-interaction-declaration-subscriber",
        L"subscriber",
        federationName));
    REQUIRE(freshPublisherHandle == sourcePublisherHandle);
    REQUIRE(freshSubscriberHandle == sourceSubscriberHandle);
    suppressDeclarationRelevanceAdvisories(*publisher);
    suppressDeclarationRelevanceAdvisories(*subscriber);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshObjectClass = publisher->getObjectClassHandle(
        fixture_hla::fom::directed_fixture_object);
    auto const freshInteractionClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::directed_fixture_interaction);
    REQUIRE(freshObjectClass.isValid());
    REQUIRE(freshInteractionClass.isValid());
    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drainAll(*publisher, *subscriber);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(subscriberReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(subscriber->federateRestoreComplete());
    drainAll(*publisher, *subscriber);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(subscriberReports.federationRestoredReportCount == 1U);

    // A second public save proves that the restored directed maps serialize
    // back to the same pair rather than merely accepting the image.
    std::wstring const roundTripLabel =
        L"public-directed-interaction-declaration-round-trip";
    REQUIRE_NOTHROW(publisher->requestFederationSave(roundTripLabel));
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(subscriber->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(subscriber->federateSaveComplete());
    auto const roundTrip = saveStore->load(federationName, roundTripLabel);
    REQUIRE(roundTrip.has_value());
    auto const roundTripImage =
        umbra::detail::FederationStateImageCodec::decode(roundTrip->stateImage);
    REQUIRE(roundTripImage.interactionDeclarations.size() == 2U);
    auto const roundTripPublisher = std::find_if(
        roundTripImage.interactionDeclarations.begin(),
        roundTripImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == sourcePublisherValue;
        });
    auto const roundTripSubscriber = std::find_if(
        roundTripImage.interactionDeclarations.begin(),
        roundTripImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == sourceSubscriberValue;
        });
    REQUIRE(roundTripPublisher != roundTripImage.interactionDeclarations.end());
    REQUIRE(roundTripSubscriber != roundTripImage.interactionDeclarations.end());
    REQUIRE(roundTripPublisher->publishedObjectClassDirectedInteractions.size() == 1U);
    REQUIRE(roundTripPublisher->publishedObjectClassDirectedInteractions.front().interactionClassHandle ==
            sourceInteractionClassValue);
    REQUIRE(roundTripSubscriber->subscribedObjectClassDirectedInteractions.size() == 1U);
    REQUIRE(roundTripSubscriber->subscribedObjectClassDirectedInteractions.front().interactionClassHandle ==
            sourceInteractionClassValue);
    REQUIRE(roundTripSubscriber->subscribedObjectClassDirectedInteractions.front().active);

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(subscriber->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(subscriber->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
  };
  runScenario(HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
} // namespace
