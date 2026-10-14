#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending attribute and interaction transportation-type changes and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][interaction-management]"
    "[transportation-management][save-restore][durable-save][filesystem][process-restart][restore]"
    "[attribute-transportation-type-change][interaction-transportation-type-change]"
    "[process-restart-attribute-transportation-type-change]"
    "[process-restart-interaction-transportation-type-change]"
    "[public-process-restart-attribute-transportation-type-change]"
    "[public-process-restart-interaction-transportation-type-change]"
    "[rti.service.get-interaction-class-handle][rti.service.get-object-class-handle]"
    "[rti.service.get-attribute-handle][rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.query-attribute-transportation-type]"
    "[rti.service.publish-interaction-class][rti.service.subscribe-interaction-class]"
    "[rti.service.send-interaction][rti.service.request-interaction-transportation-type-change]"
    "[rti.service.query-interaction-transportation-type]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.receive-interaction]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.report-attribute-transportation-type]"
    "[federate.callback.confirm-interaction-transportation-type-change]"
    "[federate.callback.report-interaction-transportation-type]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
    "[callback-immediate]") {
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
  auto const restaurantFom = resourcePath("examples/RestaurantFOMmodule-2025.xml").wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{restaurantFom, switchFom};
  std::wstring const saveLabel =
      L"public-interaction-transportation-process-restart";
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObject;
  ObjectClassHandle sourceServer;
  AttributeHandle sourceEfficiency;
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
        L"public-interaction-transportation-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-interaction-transportation-restore-peer",
        L"subscriber",
        federationName));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      // Keep the publisher's transportation-type confirmation pending for the
      // durable image while leaving the subscriber route enabled.
      REQUIRE_NOTHROW(owner->disableCallbacks());
    }
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
    sourceServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const peerServer = peer->getObjectClassHandle(fixture_hla::fom::employee_server);
    sourceEfficiency = owner->getAttributeHandle(
        sourceServer,
        fixture_hla::fixture::efficiency);
    auto const peerEfficiency = peer->getAttributeHandle(
        peerServer,
        fixture_hla::fixture::efficiency);
    auto const reliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const bestEffort = owner->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(sourceTakeOrder.isValid());
    REQUIRE(peerTakeOrder.isValid());
    REQUIRE(sourceServer.isValid());
    REQUIRE(peerServer.isValid());
    REQUIRE(sourceEfficiency.isValid());
    REQUIRE(peerEfficiency.isValid());
    REQUIRE(reliable.isValid());
    REQUIRE(bestEffort.isValid());
    AttributeHandleSet const ownerEfficiencyOnly{sourceEfficiency};
    AttributeHandleSet const peerEfficiencyOnly{peerEfficiency};
    REQUIRE_NOTHROW(peer->subscribeObjectClassAttributes(
        peerServer,
        peerEfficiencyOnly));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        sourceServer,
        ownerEfficiencyOnly));
    REQUIRE_NOTHROW(sourceObject = owner->registerObjectInstance(sourceServer));
    REQUIRE(sourceObject.isValid());
    REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(peerReports.objectDiscoveryReports.front().objectInstance == sourceObject);
    std::vector<unsigned char> const initialValueBytes{0x4aU, 0x06U};
    AttributeHandleValueMap initialValues;
    initialValues.emplace(
        sourceEfficiency,
        VariableLengthData(initialValueBytes.data(), initialValueBytes.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObject,
        initialValues,
        VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.attributeReflectionReports.size() == 1U);
    REQUIRE(peerReports.attributeReflectionReports.front().transportationType == reliable);
    peerReports.attributeReflectionReports.clear();
    REQUIRE_NOTHROW(owner->publishInteractionClass(sourceTakeOrder));
    REQUIRE_NOTHROW(peer->subscribeInteractionClass(peerTakeOrder));

    REQUIRE_NOTHROW(owner->sendInteraction(
        sourceTakeOrder,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(peerReports.interactionReports.size() == 1U);
    REQUIRE(peerReports.interactionReports.front().interactionClass ==
            peerTakeOrder);
    REQUIRE(peerReports.interactionReports.front().transportationType == reliable);
    peerReports.interactionReports.clear();

    // Leave the accepted change callback pending while the durable image is
    // committed. The saved operation must be rebound to the fresh route.
    REQUIRE_NOTHROW(owner->requestAttributeTransportationTypeChange(
        sourceObject,
        ownerEfficiencyOnly,
        bestEffort));
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.empty());
    REQUIRE_NOTHROW(owner->requestInteractionTransportationTypeChange(
        sourceTakeOrder,
        bestEffort));
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.empty());

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
    REQUIRE(ownerDeclaration->pendingInteractionTransportationTypeChanges.size() == 1U);
    auto const& savedChange =
        ownerDeclaration->pendingInteractionTransportationTypeChanges.front();
    REQUIRE(savedChange.interactionClassHandle == *sourceTakeOrderValue);
    REQUIRE(savedChange.value == "HLAbestEffort");
    REQUIRE(durableImage.objects.size() == 1U);
    REQUIRE(durableImage.objects.front().pendingAttributeTransportationTypeChanges.size() ==
            1U);

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

    // Only after the image is durable may the source route consume its
    // confirmation; the fresh registry below must deliver it again once.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
    }
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 1U);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().interactionClass ==
            sourceTakeOrder);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.front().transportationType ==
            bestEffort);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1U);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().objectInstance ==
            sourceObject);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().attributes ==
            ownerEfficiencyOnly);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().transportationType ==
            bestEffort);

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
        L"public-interaction-transportation-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-interaction-transportation-restore-peer",
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
    auto const freshServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const freshEfficiency = owner->getAttributeHandle(
        freshServer,
        fixture_hla::fixture::efficiency);
    auto const freshReliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const freshBestEffort = owner->getTransportationTypeHandle(standard_hla::mom::best_effort);
    REQUIRE(freshTakeOrder.isValid());
    REQUIRE(freshServer.isValid());
    REQUIRE(freshEfficiency.isValid());
    REQUIRE(freshReliable.isValid());
    REQUIRE(freshBestEffort.isValid());

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(peerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(peerReports.initiateFederateRestoreReports.size() == 1U);

    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(peer->federateRestoreComplete());
    drainAll(*owner, *peer);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(peerReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.interactionTransportationTypeChangeReports.size() == 1U);
    auto const& restoredChange =
        ownerReports.interactionTransportationTypeChangeReports.front();
    REQUIRE(restoredChange.interactionClass == freshTakeOrder);
    REQUIRE(restoredChange.transportationType == freshBestEffort);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1U);
    auto const& restoredAttributeChange =
        ownerReports.attributeTransportationTypeChangeReports.front();
    REQUIRE(restoredAttributeChange.objectInstance == sourceObject);
    REQUIRE(restoredAttributeChange.attributes == AttributeHandleSet{freshEfficiency});
    REQUIRE(restoredAttributeChange.transportationType == freshBestEffort);

    REQUIRE_NOTHROW(owner->queryAttributeTransportationType(
        sourceObject,
        freshEfficiency));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.attributeTransportationTypeReports.size() == 1U);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().objectInstance ==
            sourceObject);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().attribute ==
            freshEfficiency);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().transportationType ==
            freshBestEffort);

    FederateHandle const restoredOwnerHandle = freshOwnerHandle;
    REQUIRE_NOTHROW(peer->queryInteractionTransportationType(
        restoredOwnerHandle,
        freshTakeOrder));
    REQUIRE_FALSE(peer->evokeCallback(0.0));
    REQUIRE(peerReports.interactionTransportationTypeReports.size() == 1U);
    REQUIRE(peerReports.interactionTransportationTypeReports.front().federate ==
            restoredOwnerHandle);
    REQUIRE(peerReports.interactionTransportationTypeReports.front().interactionClass ==
            freshTakeOrder);
    REQUIRE(peerReports.interactionTransportationTypeReports.front().transportationType ==
            freshBestEffort);

    peerReports.interactionReports.clear();
    REQUIRE_NOTHROW(owner->sendInteraction(
        freshTakeOrder,
        ParameterHandleValueMap{},
        VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.interactionReports.size() == 1U);
    REQUIRE(peerReports.interactionReports.front().interactionClass == freshTakeOrder);
    REQUIRE(peerReports.interactionReports.front().transportationType == freshBestEffort);

    std::vector<unsigned char> const restoredValueBytes{0x4bU, 0x07U};
    AttributeHandleValueMap restoredValues;
    restoredValues.emplace(
        freshEfficiency,
        VariableLengthData(restoredValueBytes.data(), restoredValueBytes.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObject,
        restoredValues,
        VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.attributeReflectionReports.size() == 1U);
    REQUIRE(peerReports.attributeReflectionReports.front().objectInstance == sourceObject);
    REQUIRE(peerReports.attributeReflectionReports.front().attributeValues.contains(
        freshEfficiency));
    REQUIRE(peerReports.attributeReflectionReports.front().transportationType ==
            freshBestEffort);
    REQUIRE(variableLengthDataBytes(
                peerReports.attributeReflectionReports.front().attributeValues.at(
                    freshEfficiency)) == restoredValueBytes);

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
  SECTION("HLA_EVOKED") {
    runScenario(rti1516_2025::HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
}
