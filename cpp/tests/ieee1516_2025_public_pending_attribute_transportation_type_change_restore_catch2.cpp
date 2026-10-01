#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending attribute transportation-type change and preserves report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][object-management]"
    "[transportation-management][save-restore][durable-save][filesystem][process-restart][restore]"
    "[attribute-transportation-type-change][process-restart-attribute-transportation-type-change]"
    "[public-process-restart-attribute-transportation-type-change]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-transportation-type-handle]"
    "[rti.service.publish-object-class-attributes][rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.request-attribute-transportation-type-change]"
    "[rti.service.query-attribute-transportation-type]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.confirm-attribute-transportation-type-change]"
    "[federate.callback.report-attribute-transportation-type]"
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
      L"public-attribute-transportation-process-restart";
  std::vector<unsigned char> const initialValueBytes{0x4aU, 0x06U};
  std::vector<unsigned char> const restoredValueBytes{0x4bU, 0x07U};
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceServer;
  AttributeHandle sourceEfficiency;
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
        L"public-attribute-transportation-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-attribute-transportation-restore-peer",
        L"subscriber",
        federationName));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      // Stage the owner-side transport confirmation so it remains pending in
      // the durable image; keep the peer route enabled for discovery/reflect.
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
    REQUIRE_NOTHROW(sourceObjectInstance = owner->registerObjectInstance(sourceServer));
    REQUIRE(sourceObjectInstance.isValid());
    REQUIRE_FALSE(peer->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(peerReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(peerReports.objectDiscoveryReports.front().objectInstance ==
            sourceObjectInstance);

    AttributeHandleValueMap initialValues;
    initialValues.emplace(
        sourceEfficiency,
        VariableLengthData(initialValueBytes.data(), initialValueBytes.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance,
        initialValues,
        VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.attributeReflectionReports.size() == 1U);
    REQUIRE(peerReports.attributeReflectionReports.back().transportationType == reliable);
    peerReports.attributeReflectionReports.clear();

    // Leave the accepted change callback pending while the durable image is
    // committed. The saved operation must be rebound to the fresh route.
    REQUIRE_NOTHROW(owner->requestAttributeTransportationTypeChange(
        sourceObjectInstance,
        ownerEfficiencyOnly,
        bestEffort));
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(peer->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(peer->federateSaveComplete());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.objects.size() == 1U);
    auto const& savedObject = durableImage.objects.front();
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceObjectInstance);
    auto const sourceAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(sourceEfficiency);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(sourceAttributeValue.has_value());
    REQUIRE(savedObject.handle == *sourceObjectValue);
    REQUIRE(savedObject.attributeValuesPresent);
    REQUIRE(savedObject.attributeValues.size() == 1U);
    REQUIRE(savedObject.attributeValues.front().attributeHandle == *sourceAttributeValue);
    REQUIRE(savedObject.attributeValues.front().value ==
            std::string(reinterpret_cast<char const*>(initialValueBytes.data()),
                        initialValueBytes.size()));
    REQUIRE(savedObject.pendingAttributeTransportationTypeChanges.size() == 1U);
    auto const& savedChange =
        savedObject.pendingAttributeTransportationTypeChanges.front();
    REQUIRE(savedChange.requestId != 0U);
    REQUIRE(savedChange.requestingFederateId != 0U);
    REQUIRE(savedChange.attributeHandles ==
            std::vector<std::uint64_t>{*sourceAttributeValue});
    REQUIRE(savedChange.transportationName == "HLAbestEffort");
    REQUIRE(savedObject.pendingOperationCount == 1U);

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
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1U);
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.front().objectInstance ==
            sourceObjectInstance);
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
        L"public-attribute-transportation-restore-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(peer->joinFederationExecution(
        L"public-attribute-transportation-restore-peer",
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

    auto const freshServer = owner->getObjectClassHandle(fixture_hla::fom::employee_server);
    auto const freshEfficiency = owner->getAttributeHandle(
        freshServer,
        fixture_hla::fixture::efficiency);
    auto const freshReliable = owner->getTransportationTypeHandle(standard_hla::mom::reliable);
    auto const freshBestEffort = owner->getTransportationTypeHandle(standard_hla::mom::best_effort);
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
    REQUIRE(ownerReports.attributeTransportationTypeChangeReports.size() == 1U);
    auto const& restoredChange = ownerReports.attributeTransportationTypeChangeReports.front();
    REQUIRE(restoredChange.objectInstance == sourceObjectInstance);
    REQUIRE(restoredChange.attributes == AttributeHandleSet{freshEfficiency});
    REQUIRE(restoredChange.transportationType == freshBestEffort);

    REQUIRE_NOTHROW(owner->queryAttributeTransportationType(
        sourceObjectInstance,
        freshEfficiency));
    REQUIRE_FALSE(owner->evokeCallback(0.0));
    REQUIRE(ownerReports.attributeTransportationTypeReports.size() == 1U);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().objectInstance ==
            sourceObjectInstance);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().attribute ==
            freshEfficiency);
    REQUIRE(ownerReports.attributeTransportationTypeReports.front().transportationType ==
            freshBestEffort);

    AttributeHandleSet const freshEfficiencyOnly{freshEfficiency};
    AttributeHandleValueMap restoredValues;
    restoredValues.emplace(
        freshEfficiency,
        VariableLengthData(restoredValueBytes.data(), restoredValueBytes.size()));
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance,
        restoredValues,
        VariableLengthData{}));
    drainAll(*owner, *peer);
    REQUIRE(peerReports.attributeReflectionReports.size() == 1U);
    REQUIRE(peerReports.attributeReflectionReports.front().objectInstance ==
            sourceObjectInstance);
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
