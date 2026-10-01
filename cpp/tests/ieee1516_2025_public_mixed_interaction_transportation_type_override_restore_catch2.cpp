#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore preserves mixed interaction override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[transportation-management][save-restore][durable-save][filesystem][process-restart][restore]"
    "[interaction-transportation-type-change][process-restart-interaction-mixed-override]"
    "[public-process-restart-interaction-mixed-override]"
    "[callback-immediate]"
    "[rti.service.get-interaction-class-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::readTextFile;
  using public_federation_restore_test_support::serviceReportFiles;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;
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
  auto const restaurantFom =
      resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  std::wstring const saveLabel =
      L"public-interaction-mixed-override-process-restart";
  FederateHandle sourcePublisherHandle;
  InteractionClassHandle sourcePublishedClass;
  std::filesystem::path sourceReportFile;

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
        L"public-interaction-mixed-override-restore-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"public-interaction-mixed-override-restore-subscriber",
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

    sourcePublishedClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::customer_seated);
    auto const sourceSubscribedClass = subscriber->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const reliable =
        publisher->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const bestEffort =
        publisher->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(sourcePublishedClass.isValid());
    REQUIRE(sourceSubscribedClass.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE(bestEffort.isValid());
    REQUIRE_NOTHROW(publisher->publishInteractionClass(sourcePublishedClass));
    REQUIRE_NOTHROW(subscriber->subscribeInteractionClass(sourceSubscribedClass));

    REQUIRE_NOTHROW(publisher->requestInteractionTransportationTypeChange(
        sourcePublishedClass, bestEffort));
    drainAll(*publisher, *subscriber);
    REQUIRE(publisherReports.interactionTransportationTypeChangeReports.size() == 1U);
    REQUIRE(publisherReports.interactionTransportationTypeChangeReports.front().interactionClass ==
            sourcePublishedClass);
    REQUIRE(publisherReports.interactionTransportationTypeChangeReports.front().transportationType ==
            bestEffort);

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
    auto const sourcePublisherValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourcePublisherHandle);
    auto const sourcePublishedClassValue =
        rti1516_2025::umbra_binding_detail::interactionClassHandleValue(sourcePublishedClass);
    REQUIRE(sourcePublisherValue.has_value());
    REQUIRE(sourcePublishedClassValue.has_value());
    auto const publisherDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == *sourcePublisherValue;
        });
    REQUIRE(publisherDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(publisherDeclaration->publishedInteractionClasses ==
            std::vector<std::uint64_t>{*sourcePublishedClassValue});
    REQUIRE(publisherDeclaration->interactionTransportationTypes.size() == 1U);
    REQUIRE(publisherDeclaration->interactionTransportationTypes.front().interactionClassHandle ==
            *sourcePublishedClassValue);
    REQUIRE(publisherDeclaration->interactionTransportationTypes.front().value ==
            "HLAbestEffort");
    REQUIRE(publisherDeclaration->pendingInteractionTransportationTypeChanges.empty());
    auto const subscriberDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId != *sourcePublisherValue;
        });
    REQUIRE(subscriberDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(subscriberDeclaration->subscribedInteractionClasses.size() == 1U);
    REQUIRE(subscriberDeclaration->subscribedInteractionClasses.front().active);

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
    REQUIRE_NOTHROW(freshPublisherHandle = publisher->joinFederationExecution(
        L"public-interaction-mixed-override-restore-publisher",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(subscriber->joinFederationExecution(
        L"public-interaction-mixed-override-restore-subscriber",
        L"subscriber",
        federationName));
    REQUIRE(freshPublisherHandle == sourcePublisherHandle);
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

    auto const freshPublishedClass = publisher->getInteractionClassHandle(
        fixture_hla::fom::customer_seated);
    auto const freshSubscribedClass = subscriber->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const freshReliable =
        publisher->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const freshBestEffort =
        publisher->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(freshPublishedClass.isValid());
    REQUIRE(freshSubscribedClass.isValid());
    REQUIRE(freshReliable.isValid());
    REQUIRE(freshBestEffort.isValid());

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
    REQUIRE(publisherReports.interactionTransportationTypeChangeReports.empty());

    REQUIRE_NOTHROW(subscriber->queryInteractionTransportationType(
        freshPublisherHandle, freshPublishedClass));
    REQUIRE_FALSE(subscriber->evokeCallback(0.0));
    REQUIRE(subscriberReports.interactionTransportationTypeReports.size() == 1U);
    REQUIRE(subscriberReports.interactionTransportationTypeReports.front().transportationType ==
            freshBestEffort);

    // Publish the other restored declaration after restart; delivery proves
    // that the subscriber's independent declaration survived the image.
    REQUIRE_NOTHROW(publisher->publishInteractionClass(freshSubscribedClass));
    REQUIRE_NOTHROW(publisher->sendInteraction(
        freshSubscribedClass, ParameterHandleValueMap{}, VariableLengthData{}));
    drainAll(*publisher, *subscriber);
    REQUIRE(subscriberReports.interactionReports.size() == 1U);
    REQUIRE(subscriberReports.interactionReports.front().interactionClass ==
            freshSubscribedClass);
    REQUIRE(subscriberReports.interactionReports.front().transportationType ==
            freshReliable);

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
}
