#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
   "Embedded public fresh-registry restore fans out mixed regular and If Available Confirm Divestiture notifications through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture][confirm-divestiture]"
    "[process-restart-confirm-divestiture][process-restart-confirm-divestiture-fanout]"
    "[process-restart-confirm-divestiture-mixed-fanout]"
    "[public-process-restart-confirm-divestiture][public-process-restart-confirm-divestiture-fanout]"
    "[public-process-restart-confirm-divestiture-mixed-fanout]"
    "[callback-immediate][mixed-regular-if-available]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.negotiated-attribute-ownership-divestiture][rti.service.confirm-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.request-divestiture-confirmation][federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]") {
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
        ? std::wstring{L"public-confirm-divestiture-mixed-fanout-evoked"}
        : std::wstring{L"public-confirm-divestiture-mixed-fanout-immediate"};
    std::vector<unsigned char> const valueBytes{0x4DU, 0x49, 0x58U};
    std::vector<unsigned char> const regularAcquisitionTagBytes{
        0x52U, 0x45U, 0x47U};
    std::vector<unsigned char> const ifAvailableAcquisitionTagBytes{
        0x57U, 0x54U, 0x41U};
    std::vector<unsigned char> const divestitureTagBytes{
        0x44U, 0x49U, 0x56U};
    std::vector<unsigned char> const confirmationTagBytes{
        0x43U, 0x46U, 0x4DU};
    VariableLengthData const regularAcquisitionTag(
        regularAcquisitionTagBytes.data(), regularAcquisitionTagBytes.size());
    VariableLengthData const ifAvailableAcquisitionTag(
        ifAvailableAcquisitionTagBytes.data(),
        ifAvailableAcquisitionTagBytes.size());
    VariableLengthData const divestitureTag(
        divestitureTagBytes.data(), divestitureTagBytes.size());
    VariableLengthData const confirmationTag(
        confirmationTagBytes.data(), confirmationTagBytes.size());

    auto const drainAll = [](RTIambassador& owner,
                             RTIambassador& regularRequester,
                             RTIambassador& ifAvailableRequester) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(owner.evokeCallback(0.0));
        static_cast<void>(regularRequester.evokeCallback(0.0));
        static_cast<void>(ifAvailableRequester.evokeCallback(0.0));
      }
    };

    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceRegularAttribute;
    AttributeHandle sourceIfAvailableAttribute;
    FederateHandle sourceOwnerHandle;
    FederateHandle sourceRegularRequesterHandle;
    FederateHandle sourceIfAvailableRequesterHandle;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador regularRequesterReports;
      ReportingFederateAmbassador ifAvailableRequesterReports;
      auto owner = makeRti();
      auto regularRequester = makeRti();
      auto ifAvailableRequester = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(
          regularRequester->connect(regularRequesterReports, callbackModel));
      REQUIRE_NOTHROW(ifAvailableRequester->connect(
          ifAvailableRequesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-mixed-fanout-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(sourceRegularRequesterHandle =
                           regularRequester->joinFederationExecution(
                               L"public-confirm-divestiture-mixed-fanout-regular-requester",
                               L"subscriber",
                               federationName));
      REQUIRE_NOTHROW(sourceIfAvailableRequesterHandle =
                           ifAvailableRequester->joinFederationExecution(
                               L"public-confirm-divestiture-mixed-fanout-if-available-requester",
                               L"subscriber",
                               federationName));

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceRegularAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_a);
      sourceIfAvailableAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_b);
      auto const regularRequesterClass = regularRequester->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const ifAvailableRequesterClass =
          ifAvailableRequester->getObjectClassHandle(
              fixture_hla::fom::attribute_fixture_child);
      auto const regularRequesterAttribute = regularRequester->getAttributeHandle(
          regularRequesterClass, fixture_hla::fixture::reliable_base_a);
      auto const ifAvailableRequesterAttribute =
          ifAvailableRequester->getAttributeHandle(
              ifAvailableRequesterClass, fixture_hla::fixture::reliable_base_b);
      REQUIRE(sourceObjectClass.isValid());
      REQUIRE(sourceRegularAttribute.isValid());
      REQUIRE(sourceIfAvailableAttribute.isValid());
      REQUIRE(regularRequesterAttribute == sourceRegularAttribute);
      REQUIRE(ifAvailableRequesterAttribute == sourceIfAvailableAttribute);

      AttributeHandleSet const sourceAttributes{
          sourceRegularAttribute, sourceIfAvailableAttribute};
      AttributeHandleSet const regularAttributes{regularRequesterAttribute};
      AttributeHandleSet const ifAvailableAttributes{
          ifAvailableRequesterAttribute};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceObjectClass, sourceAttributes));
      REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(
          regularRequesterClass, regularAttributes));
      REQUIRE_NOTHROW(ifAvailableRequester->subscribeObjectClassAttributes(
          ifAvailableRequesterClass, ifAvailableAttributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceObjectClass));
      REQUIRE(sourceObjectInstance.isValid());
      drainAll(*owner, *regularRequester, *ifAvailableRequester);
      REQUIRE(regularRequesterReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(ifAvailableRequesterReports.objectDiscoveryReports.size() == 1U);

      // Publishing after discovery establishes Willing to Acquire for both
      // candidate forms before the owner selects them by negotiated
      // divestiture.
      REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(
          regularRequesterClass, regularAttributes));
      REQUIRE_NOTHROW(ifAvailableRequester->publishObjectClassAttributes(
          ifAvailableRequesterClass, ifAvailableAttributes));
      AttributeHandleValueMap values;
      values.emplace(
          sourceRegularAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      values.emplace(
          sourceIfAvailableAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyValueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyValueTag));
      drainAll(*owner, *regularRequester, *ifAvailableRequester);
      REQUIRE(regularRequesterReports.attributeReflectionReports.size() == 1U);
      REQUIRE(ifAvailableRequesterReports.attributeReflectionReports.size() == 1U);

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(regularRequester->disableCallbacks());
        REQUIRE_NOTHROW(ifAvailableRequester->disableCallbacks());
      }
      REQUIRE_NOTHROW(regularRequester->attributeOwnershipAcquisition(
          sourceObjectInstance, regularAttributes, regularAcquisitionTag));
      REQUIRE_NOTHROW(ifAvailableRequester->attributeOwnershipAcquisitionIfAvailable(
          sourceObjectInstance,
          ifAvailableAttributes,
          ifAvailableAcquisitionTag));
      REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
          sourceObjectInstance, sourceAttributes, divestitureTag));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
      } else {
        for (int pass = 0; pass != 32 &&
             ownerReports.divestitureConfirmationReports.size() != 2U; ++pass) {
          static_cast<void>(owner->evokeCallback(0.0));
        }
      }
      REQUIRE(ownerReports.divestitureConfirmationReports.size() == 2U);
      auto const verifyOwnerConfirmation =
          [&](AttributeHandleSet const& expectedAttributes,
              std::vector<unsigned char> const& expectedTag) {
            auto const match = std::find_if(
                ownerReports.divestitureConfirmationReports.begin(),
                ownerReports.divestitureConfirmationReports.end(),
                [&](ReportingFederateAmbassador::DivestitureConfirmationReport const& report) {
                  return report.attributes == expectedAttributes;
                });
            REQUIRE(match != ownerReports.divestitureConfirmationReports.end());
            REQUIRE(match->objectInstance == sourceObjectInstance);
            REQUIRE(variableLengthDataBytes(match->userSuppliedTag) == expectedTag);
          };
      verifyOwnerConfirmation(regularAttributes, regularAcquisitionTagBytes);
      verifyOwnerConfirmation(
          ifAvailableAttributes, ifAvailableAcquisitionTagBytes);

      REQUIRE_NOTHROW(owner->confirmDivestiture(
          sourceObjectInstance, sourceAttributes, confirmationTag));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceRegularAttribute));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceIfAvailableAttribute));
      REQUIRE(regularRequester->isAttributeOwnedByFederate(
          sourceObjectInstance, regularRequesterAttribute));
      REQUIRE(ifAvailableRequester->isAttributeOwnedByFederate(
          sourceObjectInstance, ifAvailableRequesterAttribute));
      REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.empty());

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(regularRequester->federateSaveBegun());
      REQUIRE_NOTHROW(ifAvailableRequester->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(regularRequester->federateSaveComplete());
      REQUIRE_NOTHROW(ifAvailableRequester->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.objects.size() == 1U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
      REQUIRE(savedObject.pendingNegotiatedAttributeOwnershipDivestitures.empty());
      REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 2U);
      REQUIRE(savedObject.pendingOperationCount == 2U);
      auto const sourceRegularRequesterValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRegularRequesterHandle);
      auto const sourceIfAvailableRequesterValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceIfAvailableRequesterHandle);
      auto const sourceRegularAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceRegularAttribute);
      auto const sourceIfAvailableAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceIfAvailableAttribute);
      REQUIRE(sourceRegularRequesterValue.has_value());
      REQUIRE(sourceIfAvailableRequesterValue.has_value());
      REQUIRE(sourceRegularAttributeValue.has_value());
      REQUIRE(sourceIfAvailableAttributeValue.has_value());
      auto const verifySavedNotification =
          [&](std::uint64_t expectedFederateId,
              std::uint64_t expectedAttributeHandle) {
            auto const match = std::find_if(
                savedObject.pendingConfirmDivestitureNotifications.begin(),
                savedObject.pendingConfirmDivestitureNotifications.end(),
                [&](umbra::detail::FederationStateImagePendingConfirmDivestiture const& notification) {
                  return notification.receivingFederateId == expectedFederateId;
                });
            REQUIRE(match !=
                    savedObject.pendingConfirmDivestitureNotifications.end());
            REQUIRE(match->attributeHandles ==
                    std::vector<std::uint64_t>{expectedAttributeHandle});
            REQUIRE(match->userSuppliedTag ==
                    std::string(reinterpret_cast<char const*>(confirmationTagBytes.data()),
                                confirmationTagBytes.size()));
          };
      verifySavedNotification(
          *sourceRegularRequesterValue, *sourceRegularAttributeValue);
      verifySavedNotification(
          *sourceIfAvailableRequesterValue, *sourceIfAvailableAttributeValue);

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(regularRequester->disconnect());
      REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador regularRequesterReports;
      ReportingFederateAmbassador ifAvailableRequesterReports;
      auto owner = makeRti();
      auto regularRequester = makeRti();
      auto ifAvailableRequester = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(
          regularRequester->connect(regularRequesterReports, callbackModel));
      REQUIRE_NOTHROW(ifAvailableRequester->connect(
          ifAvailableRequesterReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshRegularRequesterHandle;
      FederateHandle freshIfAvailableRequesterHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-mixed-fanout-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(freshRegularRequesterHandle =
                           regularRequester->joinFederationExecution(
                               L"public-confirm-divestiture-mixed-fanout-regular-requester",
                               L"subscriber",
                               federationName));
      REQUIRE_NOTHROW(freshIfAvailableRequesterHandle =
                           ifAvailableRequester->joinFederationExecution(
                               L"public-confirm-divestiture-mixed-fanout-if-available-requester",
                               L"subscriber",
                               federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshRegularRequesterHandle == sourceRegularRequesterHandle);
      REQUIRE(freshIfAvailableRequesterHandle == sourceIfAvailableRequesterHandle);

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshRegularAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_a);
      auto const freshIfAvailableAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_b);
      auto const freshRegularRequesterClass =
          regularRequester->getObjectClassHandle(
              fixture_hla::fom::attribute_fixture_child);
      auto const freshIfAvailableRequesterClass =
          ifAvailableRequester->getObjectClassHandle(
              fixture_hla::fom::attribute_fixture_child);
      auto const freshRegularRequesterAttribute =
          regularRequester->getAttributeHandle(
              freshRegularRequesterClass, fixture_hla::fixture::reliable_base_a);
      auto const freshIfAvailableRequesterAttribute =
          ifAvailableRequester->getAttributeHandle(
              freshIfAvailableRequesterClass, fixture_hla::fixture::reliable_base_b);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshRegularAttribute == sourceRegularAttribute);
      REQUIRE(freshIfAvailableAttribute == sourceIfAvailableAttribute);
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshRegularAttribute, freshIfAvailableAttribute}));
      REQUIRE_NOTHROW(regularRequester->subscribeObjectClassAttributes(
          freshRegularRequesterClass,
          AttributeHandleSet{freshRegularRequesterAttribute}));
      REQUIRE_NOTHROW(ifAvailableRequester->subscribeObjectClassAttributes(
          freshIfAvailableRequesterClass,
          AttributeHandleSet{freshIfAvailableRequesterAttribute}));
      REQUIRE_NOTHROW(regularRequester->publishObjectClassAttributes(
          freshRegularRequesterClass,
          AttributeHandleSet{freshRegularRequesterAttribute}));
      REQUIRE_NOTHROW(ifAvailableRequester->publishObjectClassAttributes(
          freshIfAvailableRequesterClass,
          AttributeHandleSet{freshIfAvailableRequesterAttribute}));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(regularRequester->disableCallbacks());
        REQUIRE_NOTHROW(ifAvailableRequester->disableCallbacks());
      }
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainAll(*owner, *regularRequester, *ifAvailableRequester);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(regularRequesterReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(ifAvailableRequesterReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.empty());
        REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.empty());
      }
      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(regularRequester->federateRestoreComplete());
      REQUIRE_NOTHROW(ifAvailableRequester->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(regularRequester->enableCallbacks());
        REQUIRE_NOTHROW(ifAvailableRequester->enableCallbacks());
      } else {
        drainAll(*owner, *regularRequester, *ifAvailableRequester);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(regularRequesterReports.federationRestoredReportCount == 1U);
      REQUIRE(ifAvailableRequesterReports.federationRestoredReportCount == 1U);
      REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
      REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
      auto const verifyRequesterNotification =
          [&](ReportingFederateAmbassador const& reports,
              AttributeHandleSet const& expectedAttributes) {
            auto const& notification =
                reports.attributeOwnershipAcquisitionReports.front();
            REQUIRE(notification.kind ==
                    ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
            REQUIRE(notification.objectInstance == sourceObjectInstance);
            REQUIRE(notification.attributes == expectedAttributes);
            REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
                    confirmationTagBytes);
          };
      verifyRequesterNotification(
          regularRequesterReports,
          AttributeHandleSet{freshRegularRequesterAttribute});
      verifyRequesterNotification(
          ifAvailableRequesterReports,
          AttributeHandleSet{freshIfAvailableRequesterAttribute});
      if (callbackModel == HLA_EVOKED) {
        drainAll(*owner, *regularRequester, *ifAvailableRequester);
      }
      REQUIRE(regularRequesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
      REQUIRE(ifAvailableRequesterReports.attributeOwnershipAcquisitionReports.size() == 1U);

      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshRegularAttribute, freshIfAvailableAttribute}));
      REQUIRE_NOTHROW(regularRequester->unpublishObjectClassAttributes(
          freshRegularRequesterClass,
          AttributeHandleSet{freshRegularRequesterAttribute}));
      REQUIRE_NOTHROW(ifAvailableRequester->unpublishObjectClassAttributes(
          freshIfAvailableRequesterClass,
          AttributeHandleSet{freshIfAvailableRequesterAttribute}));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(regularRequester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(ifAvailableRequester->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(regularRequester->disconnect());
      REQUIRE_NOTHROW(ifAvailableRequester->disconnect());
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
