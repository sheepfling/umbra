#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore preserves delivered negotiated owner confirmation and report-file lifetimes under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management]"
    "[save-restore][durable-save][filesystem][process-restart][restore]"
    "[ownership-ledger-state][attribute-ownership-acquisition]"
    "[negotiated-attribute-ownership-divestiture]"
    "[process-restart-negotiated-confirmation-delivered]"
    "[public-process-restart-negotiated-confirmation-delivered]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.negotiated-attribute-ownership-divestiture]"
    "[rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.federation-restore-begun]"
     "[federate.callback.initiate-federate-restore]"
     "[federate.callback.federation-restored]"
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
  auto const ownershipFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{ownershipFom, switchFom};
  std::wstring const saveLabel =
      L"public-ownership-negotiated-delivered-process-restart";
  std::vector<unsigned char> const valueBytes{0x79U, 0x7AU};
  std::vector<unsigned char> const acquisitionTagBytes{
      0x52U, 0x45U, 0x47U, 0x44U};
  std::vector<unsigned char> const divestitureTagBytes{
      0x44U, 0x49U, 0x56U, 0x44U};
  std::vector<unsigned char> const confirmationTagBytes{
      0x43U, 0x4FU, 0x4EU, 0x44U};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  VariableLengthData const divestitureTag(
      divestitureTagBytes.data(), divestitureTagBytes.size());
  VariableLengthData const confirmationTag(
      confirmationTagBytes.data(), confirmationTagBytes.size());
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceObjectClass;
  AttributeHandle sourceAttribute;
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
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();

     REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, sourceConfiguration));
     REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
     REQUIRE_NOTHROW(owner->createFederationExecution(
         federationName, fomModules, standard_hla::mom::integer64_time));
     REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
         L"public-ownership-negotiated-delivered-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"public-ownership-negotiated-delivered-requester",
        L"subscriber",
        federationName));
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*requester);

    auto const filesAtJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAtJoin.size() == 1U);
    sourceReportFile = filesAtJoin.front();
    REQUIRE(std::filesystem::absolute(sourceReportFile).lexically_normal() ==
            sourceReportFile.lexically_normal());
    auto const initialReportText = readTextFile(sourceReportFile);

    sourceObjectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
    sourceAttribute = owner->getAttributeHandle(sourceObjectClass, L"ReliableBaseA");
    REQUIRE(sourceObjectClass.isValid());
    REQUIRE(sourceAttribute.isValid());
    AttributeHandleSet const attributes{sourceAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        sourceObjectClass, attributes));
    REQUIRE_NOTHROW(sourceObjectInstance =
                        owner->registerObjectInstance(sourceObjectClass));
    REQUIRE(sourceObjectInstance.isValid());
    REQUIRE_FALSE(requester->evokeMultipleCallbacks(0.0, 0.0));
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
            sourceObjectInstance);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        sourceObjectClass, attributes));
    AttributeHandleValueMap values;
    values.emplace(
        sourceAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData valueTag;
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance, values, valueTag));
    drainAll(*owner, *requester);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      // Stage the already-delivered confirmation boundary so the immediate
      // model exercises the same saved ledger as the evoked model.
      REQUIRE_NOTHROW(owner->disableCallbacks());
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        sourceObjectInstance, attributes, acquisitionTag));
    REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
        sourceObjectInstance, attributes, divestitureTag));
    REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
    REQUIRE(ownerReports.divestitureConfirmationReports.empty());
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(owner->enableCallbacks());
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }

    // The callback pass delivers the negotiated confirmation and consumes the
    // stale ordinary release request before the save boundary.
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
    REQUIRE(ownerReports.divestitureConfirmationReports.size() == 1U);
    auto const& sourceConfirmation =
        ownerReports.divestitureConfirmationReports.front();
    REQUIRE(sourceConfirmation.objectInstance == sourceObjectInstance);
    REQUIRE(sourceConfirmation.attributes == attributes);
    REQUIRE(variableLengthDataBytes(sourceConfirmation.userSuppliedTag) ==
            acquisitionTagBytes);
    REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const durableImage =
        umbra::detail::FederationStateImageCodec::decode(durable->stateImage);
    REQUIRE(durableImage.objects.size() == 1U);
    auto const& savedObject = durableImage.objects.front();
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceObjectInstance);
    auto const sourceOwnerValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(sourceOwnerHandle);
    auto const sourceAttributeValue =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(sourceAttribute);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(sourceOwnerValue.has_value());
    REQUIRE(sourceAttributeValue.has_value());
    REQUIRE(savedObject.handle == *sourceObjectValue);
    REQUIRE(savedObject.attributeValuesPresent);
    REQUIRE(savedObject.attributeValues.size() == 1U);
    REQUIRE(savedObject.attributeValues.front().attributeHandle ==
            *sourceAttributeValue);
    REQUIRE(savedObject.attributeValues.front().value ==
            std::string(reinterpret_cast<char const*>(valueBytes.data()),
                        valueBytes.size()));
    REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() ==
            1U);
    auto const& savedRequest =
        savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
    REQUIRE(savedRequest.requestingFederateId != 0U);
    REQUIRE(savedRequest.desiredAttributeHandles ==
            std::vector<std::uint64_t>{*sourceAttributeValue});
    REQUIRE(savedRequest.unavailableQueuedAttributeHandles.empty());
    REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.empty());
    REQUIRE(savedRequest.userSuppliedTag ==
            std::string(reinterpret_cast<char const*>(acquisitionTagBytes.data()),
                        acquisitionTagBytes.size()));
    REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.size() ==
            1U);
    auto const& savedDivestiture =
        savedObject.pendingNegotiatedAttributeOwnershipDivestitures.front();
    REQUIRE(savedDivestiture.attributeHandle == *sourceAttributeValue);
    REQUIRE(savedDivestiture.divestingFederateId == *sourceOwnerValue);
    REQUIRE(savedDivestiture.acquiringFederateId == savedRequest.requestingFederateId);
    REQUIRE(savedDivestiture.acquisitionRequestId == savedRequest.requestId);
    REQUIRE_FALSE(savedDivestiture.acquiringFederateIsIfAvailable);
    REQUIRE(savedDivestiture.confirmationQueued);
    REQUIRE(savedDivestiture.confirmationDelivered);
    REQUIRE(savedDivestiture.userSuppliedTag ==
            std::string(reinterpret_cast<char const*>(divestitureTagBytes.data()),
                        divestitureTagBytes.size()));

    auto const reportAfterSave = readTextFile(sourceReportFile);
    REQUIRE(reportAfterSave.size() > initialReportText.size());
    REQUIRE(reportAfterSave.find("RequestFederationSave") != std::string::npos);
    REQUIRE(reportAfterSave.find("FederateSaveComplete") != std::string::npos);

    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, saveStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador ownerReports;
    ReportingFederateAmbassador requesterReports;
    auto owner = makeRti();
    auto requester = makeRti();
    auto freshConfiguration =
        configurationForServiceReportDirectory(reportDirectory.path());
    freshConfiguration.withRtiAddress(L"in-process");

    REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel, freshConfiguration));
    REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName, fomModules, standard_hla::mom::integer64_time));
    FederateHandle freshOwnerHandle;
    REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
        L"public-ownership-negotiated-delivered-owner",
        L"publisher",
        federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"public-ownership-negotiated-delivered-requester",
        L"subscriber",
        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*requester);
    AttributeHandleSet const attributes{sourceAttribute};

    auto const filesAfterFreshJoin = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterFreshJoin.size() == 2U);
    REQUIRE(std::find(filesAfterFreshJoin.begin(), filesAfterFreshJoin.end(),
                      sourceReportFile) != filesAfterFreshJoin.end());
    auto const freshReportFile = filesAfterFreshJoin.front() == sourceReportFile
        ? filesAfterFreshJoin.back()
        : filesAfterFreshJoin.front();
    REQUIRE(freshReportFile != sourceReportFile);
    auto const freshInitialReportText = readTextFile(freshReportFile);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(requesterReports.federationRestoreBegunReportCount == 1U);
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);

    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
    REQUIRE(ownerReports.divestitureConfirmationReports.empty());
    REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

    // A delivered confirmation is not replayed after restore; the owner keeps
    // the pending negotiated transfer until Confirm Divestiture is invoked.
    REQUIRE(owner->isAttributeOwnedByFederate(sourceObjectInstance, sourceAttribute));
    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        sourceObjectInstance, sourceAttribute));
    REQUIRE_NOTHROW(owner->confirmDivestiture(
        sourceObjectInstance, attributes, confirmationTag));
    REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
        sourceObjectInstance, sourceAttribute));
    REQUIRE(requester->isAttributeOwnedByFederate(
        sourceObjectInstance, sourceAttribute));
    drainAll(*owner, *requester);
    REQUIRE(ownerReports.divestitureConfirmationReports.empty());
    REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
    auto const& notification =
        requesterReports.attributeOwnershipAcquisitionReports.front();
    REQUIRE(notification.kind ==
            ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
    REQUIRE(notification.objectInstance == sourceObjectInstance);
    REQUIRE(notification.attributes == attributes);
    REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
            confirmationTagBytes);

    auto const filesAfterRestore = serviceReportFiles(reportDirectory.path());
    REQUIRE(filesAfterRestore == filesAfterFreshJoin);
    auto const freshReportAfterRestore = readTextFile(freshReportFile);
    REQUIRE(freshReportAfterRestore.size() > freshInitialReportText.size());
    REQUIRE(freshReportAfterRestore.find("RequestFederationRestore") !=
            std::string::npos);
    REQUIRE(freshReportAfterRestore.find("FederateRestoreComplete") !=
            std::string::npos);

    REQUIRE_NOTHROW(owner->resignFederationExecution(CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
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
