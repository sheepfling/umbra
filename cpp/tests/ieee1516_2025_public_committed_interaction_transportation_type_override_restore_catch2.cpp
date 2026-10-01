#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore preserves committed interaction transportation-type override and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[transportation-management][save-restore][durable-save][filesystem][process-restart][restore]"
    "[interaction-transportation-type-change][process-restart-interaction-transportation-type-override]"
    "[public-process-restart-interaction-transportation-type-override]"
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
      L"public-interaction-transportation-override-process-restart";
  FederateHandle sourceOwnerHandle;
  InteractionClassHandle sourceTakeOrder;
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
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador peerReports;
    auto owner = makeRti();
    auto peer = makeRti();

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, sourceConfiguration));
    REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"public-interaction-transportation-override-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-interaction-transportation-override-restore-peer",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*peer);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    sourceTakeOrder = owner->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const peerTakeOrder = peer->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const reliable =
        owner->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const bestEffort =
        owner->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(sourceTakeOrder.isValid());
    REQUIRE(peerTakeOrder.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE(bestEffort.isValid());
    REQUIRE_NOTHROW(owner->publishInteractionClass(sourceTakeOrder));
    REQUIRE_NOTHROW(peer->subscribeInteractionClass(peerTakeOrder));

    REQUIRE_NOTHROW(owner->sendInteraction(
        sourceTakeOrder, ParameterHandleValueMap{}, VariableLengthData{}));
    REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(peerReports.interactionReports.size() == 1U);
    REQUIRE(peerReports.interactionReports.front().transportationType == reliable);
    peerReports.interactionReports.clear();

    // Commit the override before saving. Restore must retain the effective
    // type without replaying a confirmation callback.
    REQUIRE_NOTHROW(owner->requestInteractionTransportationTypeChange(
        sourceTakeOrder, bestEffort));
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 1U);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().interactionClass ==
            sourceTakeOrder);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().transportationType ==
            bestEffort);

    REQUIRE_NOTHROW(owner->sendInteraction(
        sourceTakeOrder, ParameterHandleValueMap{}, VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.interactionReports.size() == 1U);
    REQUIRE(peerReports.interactionReports.front().transportationType == bestEffort);
    peerReports.interactionReports.clear();

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(peer->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(peer->federateSaveComplete());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.interactionDeclarationCount == 2U);
    REQUIRE(durableImage.interactionDeclarations.size() == 2U);
    auto const sourceOwnerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceOwnerHandle);
    auto const sourceTakeOrderValue =
        rti1516_2025::umbra_binding_detail::interactionClassHandleValue(sourceTakeOrder);
    REQUIRE(sourceOwnerValue.has_value());
    REQUIRE(sourceTakeOrderValue.has_value());
    auto const ownerDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId == *sourceOwnerValue;
        });
    REQUIRE(ownerDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(ownerDeclaration->publishedInteractionClasses ==
            std::vector<std::uint64_t>{*sourceTakeOrderValue});
    REQUIRE(ownerDeclaration->interactionTransportationTypes.size() == 1U);
    REQUIRE(ownerDeclaration->interactionTransportationTypes.front().interactionClassHandle ==
            *sourceTakeOrderValue);
    REQUIRE(ownerDeclaration->interactionTransportationTypes.front().value ==
            "HLAbestEffort");
    REQUIRE(ownerDeclaration->pendingInteractionTransportationTypeChanges.empty());
    auto const peerDeclaration = std::find_if(
        durableImage.interactionDeclarations.begin(),
        durableImage.interactionDeclarations.end(),
        [&](auto const& declaration) {
          return declaration.federateId != *sourceOwnerValue;
        });
    REQUIRE(peerDeclaration != durableImage.interactionDeclarations.end());
    REQUIRE(peerDeclaration->subscribedInteractionClasses.size() == 1U);
    REQUIRE(peerDeclaration->subscribedInteractionClasses.front().active);

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(peer->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador peerReports;
    auto owner = makeRti();
    auto peer = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(peer->connect(peerReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-interaction-transportation-override-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-interaction-transportation-override-restore-peer",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*peer);

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    auto const freshTakeOrder = owner->getInteractionClassHandle(
        fixture_hla::fom::server_take_order);
    auto const freshReliable =
        owner->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const freshBestEffort =
        owner->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(freshTakeOrder.isValid());
    REQUIRE(freshReliable.isValid());
    REQUIRE(freshBestEffort.isValid());
    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(peerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(peer->federateRestoreComplete());
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(peerReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.empty());

    REQUIRE_NOTHROW(peer->queryInteractionTransportationType(
        freshOwnerHandle, freshTakeOrder));
    REQUIRE_FALSE(peer->evokeCallback(0.0));
    REQUIRE(peerReports.interactionTransportationTypeReports.size() == 1U);
    REQUIRE(peerReports.interactionTransportationTypeReports.front().transportationType ==
            freshBestEffort);

    REQUIRE_NOTHROW(owner->sendInteraction(
        freshTakeOrder, ParameterHandleValueMap{}, VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.interactionReports.size() == 1U);
    REQUIRE(peerReports.interactionReports.front().interactionClass == freshTakeOrder);
    REQUIRE(peerReports.interactionReports.front().transportationType == freshBestEffort);

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(peer->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(peer->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };
  runScenario(HLA_EVOKED);
  runScenario(rti1516_2025::HLA_IMMEDIATE);
}
}
