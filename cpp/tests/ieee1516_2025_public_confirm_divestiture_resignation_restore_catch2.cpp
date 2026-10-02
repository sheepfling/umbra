#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore suppresses a resigned Confirm Divestiture candidate and preserves the surviving recipient",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture][confirm-divestiture][resign-action]"
    "[process-restart-confirm-divestiture][process-restart-confirm-divestiture-resignation]"
    "[public-process-restart-confirm-divestiture][public-process-restart-confirm-divestiture-resignation]"
    "[callback-immediate][resigned-candidate-suppression]"
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
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.attribute-ownership-acquisition-notification]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
    "[federate.callback.initiate-federate-restore][federate.callback.federation-restored][2025]") {
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
        ? std::wstring{L"public-confirm-divestiture-resignation-evoked"}
        : std::wstring{L"public-confirm-divestiture-resignation-immediate"};
    std::vector<unsigned char> const valueBytes{0x52U, 0x45U, 0x53U};
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
                             RTIambassador& survivor,
                             RTIambassador& departing) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(owner.evokeCallback(0.0));
        static_cast<void>(survivor.evokeCallback(0.0));
        static_cast<void>(departing.evokeCallback(0.0));
      }
    };
    auto const drainLive = [](RTIambassador& owner,
                              RTIambassador& survivor) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(owner.evokeCallback(0.0));
        static_cast<void>(survivor.evokeCallback(0.0));
      }
    };

    FederateHandle sourceOwnerHandle;
    FederateHandle sourceSurvivorHandle;
    FederateHandle sourceDepartingHandle;
    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceRegularAttribute;
    AttributeHandle sourceIfAvailableAttribute;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador survivorReports;
      ReportingFederateAmbassador departingReports;
      auto owner = makeRti();
      auto survivor = makeRti();
      auto departing = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(survivor->connect(survivorReports, callbackModel));
      REQUIRE_NOTHROW(departing->connect(departingReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));

      // Keep the surviving member's ID below the departing member's ID.  The
      // fresh registry can then join the two saved members with the same
      // stable IDs after the third member has resigned.
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-resignation-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(sourceSurvivorHandle = survivor->joinFederationExecution(
          L"public-confirm-divestiture-resignation-survivor",
          L"subscriber",
          federationName));
      REQUIRE_NOTHROW(sourceDepartingHandle = departing->joinFederationExecution(
          L"public-confirm-divestiture-resignation-departing",
          L"subscriber",
          federationName));
      REQUIRE(sourceOwnerHandle != sourceSurvivorHandle);
      REQUIRE(sourceSurvivorHandle != sourceDepartingHandle);

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceRegularAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_a);
      sourceIfAvailableAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_b);
      REQUIRE(sourceObjectClass.isValid());
      REQUIRE(sourceRegularAttribute.isValid());
      REQUIRE(sourceIfAvailableAttribute.isValid());
      AttributeHandleSet const sourceAttributes{
          sourceRegularAttribute, sourceIfAvailableAttribute};
      AttributeHandleSet const regularAttributes{sourceRegularAttribute};
      AttributeHandleSet const ifAvailableAttributes{sourceIfAvailableAttribute};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceObjectClass, sourceAttributes));
      REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(
          sourceObjectClass, ifAvailableAttributes));
      REQUIRE_NOTHROW(departing->subscribeObjectClassAttributes(
          sourceObjectClass, regularAttributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceObjectClass));
      REQUIRE(sourceObjectInstance.isValid());
      drainAll(*owner, *survivor, *departing);
      REQUIRE(survivorReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(departingReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(survivorReports.objectDiscoveryReports.front().objectInstance ==
              sourceObjectInstance);
      REQUIRE(departingReports.objectDiscoveryReports.front().objectInstance ==
              sourceObjectInstance);
      REQUIRE_NOTHROW(survivor->publishObjectClassAttributes(
          sourceObjectClass, ifAvailableAttributes));
      REQUIRE_NOTHROW(departing->publishObjectClassAttributes(
          sourceObjectClass, regularAttributes));

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
      drainAll(*owner, *survivor, *departing);
      REQUIRE(survivorReports.attributeReflectionReports.size() == 1U);
      REQUIRE(departingReports.attributeReflectionReports.size() == 1U);

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(survivor->disableCallbacks());
        REQUIRE_NOTHROW(departing->disableCallbacks());
      }
      REQUIRE_NOTHROW(departing->attributeOwnershipAcquisition(
          sourceObjectInstance, regularAttributes, regularAcquisitionTag));
      REQUIRE_NOTHROW(survivor->attributeOwnershipAcquisitionIfAvailable(
          sourceObjectInstance,
          ifAvailableAttributes,
          ifAvailableAcquisitionTag));
      REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
          sourceObjectInstance, sourceAttributes, divestitureTag));

      // Both candidates are selected before the save boundary.  Deliver the
      // owner's confirmation callbacks, then resign only the regular
      // candidate.  The resignation must cancel its acquisition and
      // negotiated-divestiture state without disturbing the surviving
      // candidate's selected If Available transfer.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
      } else {
        for (int pass = 0; pass != 32 &&
             ownerReports.divestitureConfirmationReports.size() != 2U;
             ++pass) {
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

      REQUIRE_NOTHROW(departing->resignFederationExecution(
          rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS));
      REQUIRE(departingReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE_NOTHROW(owner->confirmDivestiture(
          sourceObjectInstance, ifAvailableAttributes, confirmationTag));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceIfAvailableAttribute));
      REQUIRE(survivor->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceIfAvailableAttribute));
      REQUIRE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceRegularAttribute));
      REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.empty());

      REQUIRE_NOTHROW(departing->disconnect());
      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(survivor->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(survivor->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.members.size() == 2U);
      auto const sourceOwnerValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceOwnerHandle);
      auto const sourceSurvivorValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceSurvivorHandle);
      auto const sourceDepartingValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceDepartingHandle);
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      auto const sourceIfAvailableAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceIfAvailableAttribute);
      REQUIRE(sourceOwnerValue.has_value());
      REQUIRE(sourceSurvivorValue.has_value());
      REQUIRE(sourceDepartingValue.has_value());
      REQUIRE(sourceObjectValue.has_value());
      REQUIRE(sourceIfAvailableAttributeValue.has_value());
      REQUIRE(std::none_of(
          durableImage.members.begin(),
          durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceDepartingValue;
          }));
      REQUIRE(std::any_of(
          durableImage.members.begin(),
          durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceOwnerValue;
          }));
      REQUIRE(std::any_of(
          durableImage.members.begin(),
          durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceSurvivorValue;
          }));
      REQUIRE(durableImage.objects.size() == 1U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.handle == *sourceObjectValue);
      REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 1U);
      auto const& savedNotification =
          savedObject.pendingConfirmDivestitureNotifications.front();
      REQUIRE(savedNotification.receivingFederateId == *sourceSurvivorValue);
      REQUIRE(savedNotification.attributeHandles ==
              std::vector<std::uint64_t>{*sourceIfAvailableAttributeValue});
      REQUIRE(savedNotification.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(confirmationTagBytes.data()),
                          confirmationTagBytes.size()));
      // Resigning the regular candidate cancels its negotiated transfer and
      // clears that candidate's assumption-search interval.  The surviving
      // If Available Confirm Divestiture notification is the only durable
      // operation left on this object.
      REQUIRE(savedObject.pendingOperationCount == 1U);

      REQUIRE_NOTHROW(survivor->resignFederationExecution(
          rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(survivor->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador survivorReports;
      auto owner = makeRti();
      auto survivor = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(survivor->connect(survivorReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshSurvivorHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-resignation-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(freshSurvivorHandle = survivor->joinFederationExecution(
          L"public-confirm-divestiture-resignation-survivor",
          L"subscriber",
          federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshSurvivorHandle == sourceSurvivorHandle);

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshRegularAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_a);
      auto const freshIfAvailableAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_b);
      auto const freshSurvivorClass = survivor->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshSurvivorAttribute = survivor->getAttributeHandle(
          freshSurvivorClass, fixture_hla::fixture::reliable_base_b);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshRegularAttribute == sourceRegularAttribute);
      REQUIRE(freshIfAvailableAttribute == sourceIfAvailableAttribute);
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshRegularAttribute, freshIfAvailableAttribute}));
      REQUIRE_NOTHROW(survivor->subscribeObjectClassAttributes(
          freshSurvivorClass, AttributeHandleSet{freshSurvivorAttribute}));
      REQUIRE_NOTHROW(survivor->publishObjectClassAttributes(
          freshSurvivorClass, AttributeHandleSet{freshSurvivorAttribute}));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(survivor->disableCallbacks());
      }
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainLive(*owner, *survivor);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(survivorReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.empty());
      }

      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(survivor->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(survivor->enableCallbacks());
      } else {
        drainLive(*owner, *survivor);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(survivorReports.federationRestoredReportCount == 1U);
      REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.size() == 1U);
      auto const& restoredNotification =
          survivorReports.attributeOwnershipAcquisitionReports.front();
      REQUIRE(restoredNotification.kind ==
              ReportingFederateAmbassador::AttributeOwnershipAcquisitionReport::Kind::notification);
      REQUIRE(restoredNotification.objectInstance == sourceObjectInstance);
      REQUIRE(restoredNotification.attributes ==
              AttributeHandleSet{freshSurvivorAttribute});
      REQUIRE(variableLengthDataBytes(restoredNotification.userSuppliedTag) ==
              confirmationTagBytes);
      REQUIRE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, freshRegularAttribute));
      REQUIRE(survivor->isAttributeOwnedByFederate(
          sourceObjectInstance, freshSurvivorAttribute));
      REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.size() == 1U);
      if (callbackModel == HLA_EVOKED) {
        drainLive(*owner, *survivor);
      }
      REQUIRE(survivorReports.attributeOwnershipAcquisitionReports.size() == 1U);

      REQUIRE_NOTHROW(survivor->unpublishObjectClassAttributes(
          freshSurvivorClass, AttributeHandleSet{freshSurvivorAttribute}));
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshRegularAttribute, freshIfAvailableAttribute}));
      REQUIRE_NOTHROW(survivor->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(survivor->disconnect());
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
