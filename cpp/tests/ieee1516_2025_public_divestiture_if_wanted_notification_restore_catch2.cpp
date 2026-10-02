#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending Divestiture If Wanted notification through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-divestiture-if-wanted]"
    "[process-restart-divestiture-if-wanted]"
    "[public-process-restart-divestiture-if-wanted]"
    "[callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
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
        ? std::wstring{L"public-divestiture-if-wanted-evoked"}
        : std::wstring{L"public-divestiture-if-wanted-immediate"};
    std::vector<unsigned char> const valueBytes{0x44U, 0x49U, 0x56U};
    std::vector<unsigned char> const acquisitionTagBytes{
        0x41U, 0x43U, 0x51U};
    std::vector<unsigned char> const divestitureTagBytes{
        0x44U, 0x49U, 0x57U};
    VariableLengthData const acquisitionTag(
        acquisitionTagBytes.data(), acquisitionTagBytes.size());
    VariableLengthData const divestitureTag(
        divestitureTagBytes.data(), divestitureTagBytes.size());

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
          L"public-divestiture-if-wanted-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
          L"public-divestiture-if-wanted-requester",
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
      // Publishing after discovery satisfies the 2025 Willing-to-Acquire
      // precondition used by Divestiture If Wanted.
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

      // Leave the notification outside user code for both callback models:
      // HLA_EVOKED keeps it queued, while HLA_IMMEDIATE is held by the
      // callback-disable switch until after the restore boundary.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requester->disableCallbacks());
      }
      REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
          sourceObjectInstance, attributes, acquisitionTag));
      AttributeHandleSet divestedAttributes;
      REQUIRE_NOTHROW(owner->attributeOwnershipDivestitureIfWanted(
          sourceObjectInstance, attributes, divestitureTag, divestedAttributes));
      REQUIRE(divestedAttributes == attributes);
      REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());
      REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());

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
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      auto const sourceAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttribute);
      auto const sourceRequesterValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterHandle);
      REQUIRE(sourceObjectValue.has_value());
      REQUIRE(sourceAttributeValue.has_value());
      REQUIRE(sourceRequesterValue.has_value());
      REQUIRE(savedObject.handle == *sourceObjectValue);
      REQUIRE(savedObject.attributeValuesPresent);
      REQUIRE(savedObject.attributeValues.size() == 1U);
      REQUIRE(savedObject.attributeValues.front().attributeHandle ==
              *sourceAttributeValue);
      REQUIRE(savedObject.attributeValues.front().value ==
              std::string(reinterpret_cast<char const*>(valueBytes.data()),
                          valueBytes.size()));
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.empty());
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty());
      REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionCancellations.empty());
      REQUIRE(savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.size() ==
              1U);
      auto const& savedNotification =
          savedObject.pendingAttributeOwnershipDivestitureIfWantedNotifications.front();
      REQUIRE(savedNotification.notificationId != 0U);
      REQUIRE(savedNotification.receivingFederateId == *sourceRequesterValue);
      REQUIRE(savedNotification.attributeHandles ==
              std::vector<std::uint64_t>{*sourceAttributeValue});
      REQUIRE(savedNotification.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(divestitureTagBytes.data()),
                          divestitureTagBytes.size()));
      REQUIRE(savedObject.pendingOperationCount == 1U);

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
          L"public-divestiture-if-wanted-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
          L"public-divestiture-if-wanted-requester",
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
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainBoth(*owner, *requester);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.empty());
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
      REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);
      auto const& notification =
          requesterReports.attributeOwnershipAcquisitionReports.front();
      REQUIRE(notification.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
      REQUIRE(notification.objectInstance == sourceObjectInstance);
      REQUIRE(notification.attributes == attributes);
      REQUIRE(variableLengthDataBytes(notification.userSuppliedTag) ==
              divestitureTagBytes);

      // The restored notification is consumed at callback entry and cannot
      // be replayed by a later evoke pass.
      drain(*requester);
      REQUIRE(requesterReports.attributeOwnershipAcquisitionReports.size() == 1U);

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
