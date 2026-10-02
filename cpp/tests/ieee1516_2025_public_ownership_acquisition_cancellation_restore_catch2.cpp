#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending ownership-acquisition cancellation through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-cancellation]"
    "[process-restart-ownership-acquisition-cancellation]"
    "[public-process-restart-ownership-acquisition-cancellation]"
    "[callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.cancel-attribute-ownership-acquisition]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.confirm-attribute-ownership-acquisition-cancellation]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::readTextFile;
  using public_federation_restore_test_support::serviceReportFiles;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;
  auto runScenario = [](CallbackModel callbackModel) {
    auto const saveDirectory = temporaryFederationSaveDirectory();
    auto const saveStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const federationName = nextFederationName();
    auto const fomModule =
        (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
         "data" / "attribute-update-passel-fom.xml")
            .wstring();
    auto const saveLabel = callbackModel == HLA_EVOKED
        ? std::wstring{L"public-ownership-cancellation-evoked"}
        : std::wstring{L"public-ownership-cancellation-immediate"};
    std::vector<unsigned char> const valueBytes{0x43U, 0x41U, 0x4EU};
    std::vector<unsigned char> const acquisitionTagBytes{
        0x43U, 0x41U, 0x4EU, 0x43U};
    VariableLengthData const acquisitionTag(
        acquisitionTagBytes.data(), acquisitionTagBytes.size());

    auto const drain = [](RTIambassador& ambassador) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(ambassador.evokeCallback(0.0));
      }
    };
    auto const drainBoth = [&](RTIambassador& first, RTIambassador& second) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(first.evokeCallback(0.0));
        static_cast<void>(second.evokeCallback(0.0));
      }
    };

    FederateHandle sourceOwnerHandle;
    FederateHandle sourceRequesterHandle;
    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceAttribute;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador requesterReports;
      auto owner = makeRti();
      auto requester = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-ownership-cancellation-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
          L"public-ownership-cancellation-requester",
          L"subscriber",
          federationName));

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_child);
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
      drain(*requester);
      REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(requesterReports.objectDiscoveryReports.front().objectInstance ==
              sourceObjectInstance);
      REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
          sourceObjectClass, attributes));

      AttributeHandleValueMap values;
      values.emplace(
          sourceAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyValueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyValueTag));
      drain(*requester);
      REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);

      // Keep both callback routes at the same pre-callback boundary for the
      // two execution models. The cancellation and its confirmation must be
      // durable before either route is allowed to enter user code.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
          sourceObjectInstance, attributes, acquisitionTag));
      REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
      REQUIRE_NOTHROW(requester->cancelAttributeOwnershipAcquisition(
          sourceObjectInstance, attributes));
      REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(requester->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(requester->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.objects.size() == 1U);
      auto const& savedObject = durableImage.objects.front();
      auto const sourceRequesterValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterHandle);
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      auto const sourceAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttribute);
      REQUIRE(sourceRequesterValue.has_value());
      REQUIRE(sourceObjectValue.has_value());
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
      REQUIRE(savedRequest.requestingFederateId == *sourceRequesterValue);
      REQUIRE(savedRequest.desiredAttributeHandles ==
              std::vector<std::uint64_t>{*sourceAttributeValue});
      REQUIRE(savedRequest.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(acquisitionTagBytes.data()),
                          acquisitionTagBytes.size()));
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.size() ==
              1U);
      auto const& savedCancellation =
          savedObject.pendingAttributeOwnershipAcquisitionCancellations.front();
      REQUIRE(savedCancellation.requestingFederateId == *sourceRequesterValue);
      REQUIRE(savedCancellation.attributeHandles ==
              std::vector<std::uint64_t>{*sourceAttributeValue});
      REQUIRE(savedObject.pendingOperationCount == 2U);

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
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

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requester->connect(requesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshRequesterHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-ownership-cancellation-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
          L"public-ownership-cancellation-requester",
          L"subscriber",
          federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshRequesterHandle == sourceRequesterHandle);

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_child);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshAttribute == sourceAttribute);
      AttributeHandleSet const attributes{freshAttribute};
      REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
          freshObjectClass, attributes));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      requesterReports.onConfirmAttributeOwnershipAcquisitionCancellation = [&] {
        requesterReports.callbackOrder.push_back("ownership-cancellation");
      };

      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainBoth(*owner, *requester);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(ownerReports.attributeOwnershipAcquisitionCancellationReports.empty());
        REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.empty());
      }

      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(requester->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(requester->enableCallbacks());
      } else {
        drainBoth(*owner, *requester);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(requesterReports.federationRestoredReportCount == 1U);
      REQUIRE(ownerReports.attributeOwnershipAcquisitionCancellationReports.empty());
      REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() ==
              1U);
      auto const& confirmation =
          requesterReports.attributeOwnershipAcquisitionCancellationReports.front();
      REQUIRE(confirmation.objectInstance == sourceObjectInstance);
      REQUIRE(confirmation.attributes == attributes);

      auto const restoredComplete = std::find(
          requesterReports.callbackOrder.begin(),
          requesterReports.callbackOrder.end(),
          "restore-complete");
      auto const cancellationCallback = std::find(
          requesterReports.callbackOrder.begin(),
          requesterReports.callbackOrder.end(),
          "ownership-cancellation");
      REQUIRE(restoredComplete != requesterReports.callbackOrder.end());
      REQUIRE(cancellationCallback != requesterReports.callbackOrder.end());
      REQUIRE(restoredComplete < cancellationCallback);

      // The restored cancellation reservation is consumed at callback entry;
      // a later evoke pass must not replay the confirmation.
      drain(*requester);
      REQUIRE(requesterReports.attributeOwnershipAcquisitionCancellationReports.size() ==
              1U);

      REQUIRE_NOTHROW(requester->unpublishObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requester->disconnect());
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
