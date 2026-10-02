#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore suppresses one post-confirmation resigned Confirm Divestiture recipient while preserving two surviving recipients",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[attribute-ownership-acquisition][negotiated-attribute-ownership-divestiture][confirm-divestiture]"
    "[process-restart-confirm-divestiture][process-restart-confirm-divestiture-fanout]"
    "[process-restart-confirm-divestiture-post-confirmation-resignation]"
    "[public-process-restart-confirm-divestiture][public-process-restart-confirm-divestiture-fanout]"
    "[public-process-restart-confirm-divestiture-post-confirmation-resignation]"
    "[callback-immediate][multi-survivor-post-confirmation-fanout][post-confirmation-resignation]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
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
        ? std::wstring{L"public-confirm-divestiture-post-resignation-fanout-evoked"}
        : std::wstring{L"public-confirm-divestiture-post-resignation-fanout-immediate"};
    std::vector<unsigned char> const valueBytes{0x46U, 0x41U, 0x4EU};
    std::vector<unsigned char> const acquisitionTagABytes{
        0x41U, 0x2DU, 0x41U};
    std::vector<unsigned char> const acquisitionTagBBytes{
        0x41U, 0x2DU, 0x42U};
    std::vector<unsigned char> const acquisitionTagCBytes{
        0x41U, 0x2DU, 0x43U};
    std::vector<unsigned char> const divestitureTagBytes{
        0x44U, 0x2DU, 0x46U};
    std::vector<unsigned char> const confirmationTagBytes{
        0x43U, 0x2DU, 0x46U};
    VariableLengthData const acquisitionTagA(
        acquisitionTagABytes.data(), acquisitionTagABytes.size());
    VariableLengthData const acquisitionTagB(
        acquisitionTagBBytes.data(), acquisitionTagBBytes.size());
    VariableLengthData const acquisitionTagC(
        acquisitionTagCBytes.data(), acquisitionTagCBytes.size());
    VariableLengthData const divestitureTag(
        divestitureTagBytes.data(), divestitureTagBytes.size());
    VariableLengthData const confirmationTag(
        confirmationTagBytes.data(), confirmationTagBytes.size());

    auto const drainAll = [](RTIambassador& owner,
                             RTIambassador& requesterA,
                             RTIambassador& requesterB,
                             RTIambassador& requesterC) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(owner.evokeCallback(0.0));
        static_cast<void>(requesterA.evokeCallback(0.0));
        static_cast<void>(requesterB.evokeCallback(0.0));
        static_cast<void>(requesterC.evokeCallback(0.0));
      }
    };

    auto const drainLive = [](RTIambassador& owner,
                              RTIambassador& requesterA,
                              RTIambassador& requesterB) {
      for (int pass = 0; pass != 32; ++pass) {
        static_cast<void>(owner.evokeCallback(0.0));
        static_cast<void>(requesterA.evokeCallback(0.0));
        static_cast<void>(requesterB.evokeCallback(0.0));
      }
    };

    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceAttributeA;
    AttributeHandle sourceAttributeB;
    AttributeHandle sourceAttributeC;
    FederateHandle sourceOwnerHandle;
    FederateHandle sourceRequesterAHandle;
    FederateHandle sourceRequesterBHandle;
    FederateHandle sourceRequesterCHandle;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador requesterAReports;
      ReportingFederateAmbassador requesterBReports;
      ReportingFederateAmbassador requesterCReports;
      auto owner = makeRti();
      auto requesterA = makeRti();
      auto requesterB = makeRti();
      auto requesterC = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requesterA->connect(requesterAReports, callbackModel));
      REQUIRE_NOTHROW(requesterB->connect(requesterBReports, callbackModel));
      REQUIRE_NOTHROW(requesterC->connect(requesterCReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-fanout-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(sourceRequesterAHandle = requesterA->joinFederationExecution(
          L"public-confirm-divestiture-fanout-requester-a", L"subscriber", federationName));
      REQUIRE_NOTHROW(sourceRequesterBHandle = requesterB->joinFederationExecution(
          L"public-confirm-divestiture-fanout-requester-b", L"subscriber", federationName));
      REQUIRE_NOTHROW(sourceRequesterCHandle = requesterC->joinFederationExecution(
          L"public-confirm-divestiture-fanout-requester-c", L"subscriber", federationName));

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceAttributeA = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_a);
      sourceAttributeB = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_base_b);
      sourceAttributeC = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_child);
      auto const requesterAClass = requesterA->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const requesterBClass = requesterB->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const requesterCClass = requesterC->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const requesterAAttribute = requesterA->getAttributeHandle(
          requesterAClass, fixture_hla::fixture::reliable_base_a);
      auto const requesterBAttribute = requesterB->getAttributeHandle(
          requesterBClass, fixture_hla::fixture::reliable_base_b);
      auto const requesterCAttribute = requesterC->getAttributeHandle(
          requesterCClass, fixture_hla::fixture::reliable_child);
      REQUIRE(sourceObjectClass.isValid());
      REQUIRE(sourceAttributeA.isValid());
      REQUIRE(sourceAttributeB.isValid());
      REQUIRE(sourceAttributeC.isValid());
      REQUIRE(requesterAAttribute == sourceAttributeA);
      REQUIRE(requesterBAttribute == sourceAttributeB);
      REQUIRE(requesterCAttribute == sourceAttributeC);

      AttributeHandleSet const sourceAttributes{
          sourceAttributeA, sourceAttributeB, sourceAttributeC};
      AttributeHandleSet const requesterAAttributes{requesterAAttribute};
      AttributeHandleSet const requesterBAttributes{requesterBAttribute};
      AttributeHandleSet const requesterCAttributes{requesterCAttribute};
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceObjectClass, sourceAttributes));
      REQUIRE_NOTHROW(requesterA->subscribeObjectClassAttributes(
          requesterAClass, requesterAAttributes));
      REQUIRE_NOTHROW(requesterB->subscribeObjectClassAttributes(
          requesterBClass, requesterBAttributes));
      REQUIRE_NOTHROW(requesterC->subscribeObjectClassAttributes(
          requesterCClass, requesterCAttributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceObjectClass));
      REQUIRE(sourceObjectInstance.isValid());
      drainAll(*owner, *requesterA, *requesterB, *requesterC);
      REQUIRE(requesterAReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(requesterBReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(requesterCReports.objectDiscoveryReports.size() == 1U);

      // Publishing after discovery satisfies the 2025 Willing-to-Acquire
      // precondition used by Confirm Divestiture for each candidate.
      REQUIRE_NOTHROW(requesterA->publishObjectClassAttributes(
          requesterAClass, requesterAAttributes));
      REQUIRE_NOTHROW(requesterB->publishObjectClassAttributes(
          requesterBClass, requesterBAttributes));
      REQUIRE_NOTHROW(requesterC->publishObjectClassAttributes(
          requesterCClass, requesterCAttributes));

      AttributeHandleValueMap values;
      values.emplace(
          sourceAttributeA,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      values.emplace(
          sourceAttributeB,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      values.emplace(
          sourceAttributeC,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyValueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyValueTag));
      drainAll(*owner, *requesterA, *requesterB, *requesterC);
      REQUIRE(requesterAReports.attributeReflectionReports.size() == 1U);
      REQUIRE(requesterBReports.attributeReflectionReports.size() == 1U);
      REQUIRE(requesterCReports.attributeReflectionReports.size() == 1U);

      // Keep every requester notification outside user code until after the
      // fresh-registry restore boundary.  The owner confirmation callback is
      // delivered before saving so Confirm Divestiture can create all three
      // durable notification reservations.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requesterA->disableCallbacks());
        REQUIRE_NOTHROW(requesterB->disableCallbacks());
        REQUIRE_NOTHROW(requesterC->disableCallbacks());
      }
      REQUIRE_NOTHROW(requesterA->attributeOwnershipAcquisition(
          sourceObjectInstance, requesterAAttributes, acquisitionTagA));
      REQUIRE_NOTHROW(requesterB->attributeOwnershipAcquisition(
          sourceObjectInstance, requesterBAttributes, acquisitionTagB));
      REQUIRE_NOTHROW(requesterC->attributeOwnershipAcquisition(
          sourceObjectInstance, requesterCAttributes, acquisitionTagC));
      REQUIRE_NOTHROW(owner->negotiatedAttributeOwnershipDivestiture(
          sourceObjectInstance, sourceAttributes, divestitureTag));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
      } else {
        for (int pass = 0; pass != 32 &&
             ownerReports.divestitureConfirmationReports.size() != 3U; ++pass) {
          static_cast<void>(owner->evokeCallback(0.0));
        }
      }
      REQUIRE(ownerReports.divestitureConfirmationReports.size() == 3U);
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
      verifyOwnerConfirmation(requesterAAttributes, acquisitionTagABytes);
      verifyOwnerConfirmation(requesterBAttributes, acquisitionTagBBytes);
      verifyOwnerConfirmation(requesterCAttributes, acquisitionTagCBytes);

      REQUIRE_NOTHROW(owner->confirmDivestiture(
          sourceObjectInstance, sourceAttributes, confirmationTag));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceAttributeA));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceAttributeB));
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceAttributeC));
      REQUIRE(requesterA->isAttributeOwnedByFederate(
          sourceObjectInstance, requesterAAttribute));
      REQUIRE(requesterB->isAttributeOwnedByFederate(
          sourceObjectInstance, requesterBAttribute));
      REQUIRE(requesterC->isAttributeOwnedByFederate(
          sourceObjectInstance, requesterCAttribute));
      REQUIRE(requesterAReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(requesterBReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(requesterCReports.attributeOwnershipAcquisitionReports.empty());

      // Remove one recipient after confirmation. Its pending notification is
      // suppressed, while the two surviving recipient reservations remain
      // durable. Withdraw the owner's publication for the departing
      // attribute so no unrelated assumption candidate is created.
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          sourceObjectClass, AttributeHandleSet{sourceAttributeC}));
      REQUIRE_NOTHROW(requesterC->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE(requesterCReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE_NOTHROW(requesterC->disconnect());

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(requesterA->federateSaveBegun());
      REQUIRE_NOTHROW(requesterB->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(requesterA->federateSaveComplete());
      REQUIRE_NOTHROW(requesterB->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.objects.size() == 1U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 2U);
      REQUIRE(savedObject.pendingOperationCount == 4U);
      auto const sourceRequesterAValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterAHandle);
      auto const sourceRequesterBValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterBHandle);
      auto const sourceRequesterCValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterCHandle);
      auto const sourceAttributeAValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeA);
      auto const sourceAttributeBValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeB);
      auto const sourceAttributeCValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeC);
      REQUIRE(sourceRequesterAValue.has_value());
      REQUIRE(sourceRequesterBValue.has_value());
      REQUIRE(sourceRequesterCValue.has_value());
      REQUIRE(sourceAttributeAValue.has_value());
      REQUIRE(sourceAttributeBValue.has_value());
      REQUIRE(sourceAttributeCValue.has_value());
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
      verifySavedNotification(*sourceRequesterAValue, *sourceAttributeAValue);
      verifySavedNotification(*sourceRequesterBValue, *sourceAttributeBValue);

      REQUIRE(durableImage.members.size() == 3U);
      REQUIRE(std::none_of(
          durableImage.members.begin(),
          durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceRequesterCValue;
          }));
      REQUIRE(std::count_if(
          durableImage.members.begin(),
          durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceRequesterAValue ||
                member.id == *sourceRequesterBValue;
          }) == 2U);
      REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.size() == 1U);
      auto const& savedAssumption =
          savedObject.ownershipAssumptionRecipientsByAttribute.front();
      REQUIRE(savedAssumption.attributeHandle == *sourceAttributeCValue);
      REQUIRE(savedAssumption.recipientFederateIds.empty());
      REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() ==
              1U);
      auto const& savedAssumptionTag =
          savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front();
      REQUIRE(savedAssumptionTag.attributeHandle == *sourceAttributeCValue);
      REQUIRE(savedAssumptionTag.userSuppliedTag.empty());

      // The departing member is already resigned and disconnected.
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          sourceObjectClass,
          AttributeHandleSet{sourceAttributeA, sourceAttributeB}));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(requesterA->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(requesterB->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requesterA->disconnect());
      REQUIRE_NOTHROW(requesterB->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador requesterAReports;
      ReportingFederateAmbassador requesterBReports;
      auto owner = makeRti();
      auto requesterA = makeRti();
      auto requesterB = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requesterA->connect(requesterAReports, callbackModel));
      REQUIRE_NOTHROW(requesterB->connect(requesterBReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshRequesterAHandle;
      FederateHandle freshRequesterBHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-fanout-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(freshRequesterAHandle = requesterA->joinFederationExecution(
          L"public-confirm-divestiture-fanout-requester-a", L"subscriber", federationName));
      REQUIRE_NOTHROW(freshRequesterBHandle = requesterB->joinFederationExecution(
          L"public-confirm-divestiture-fanout-requester-b", L"subscriber", federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshRequesterAHandle == sourceRequesterAHandle);
      REQUIRE(freshRequesterBHandle == sourceRequesterBHandle);

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshAttributeA = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_a);
      auto const freshAttributeB = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_base_b);
      auto const freshAttributeC = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_child);
      auto const freshRequesterAClass = requesterA->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshRequesterBClass = requesterB->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshRequesterAAttribute = requesterA->getAttributeHandle(
          freshRequesterAClass, fixture_hla::fixture::reliable_base_a);
      auto const freshRequesterBAttribute = requesterB->getAttributeHandle(
          freshRequesterBClass, fixture_hla::fixture::reliable_base_b);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshAttributeA == sourceAttributeA);
      REQUIRE(freshAttributeB == sourceAttributeB);
      REQUIRE(freshAttributeC == sourceAttributeC);
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshAttributeA, freshAttributeB}));
      REQUIRE_NOTHROW(requesterA->subscribeObjectClassAttributes(
          freshRequesterAClass, AttributeHandleSet{freshRequesterAAttribute}));
      REQUIRE_NOTHROW(requesterB->subscribeObjectClassAttributes(
          freshRequesterBClass, AttributeHandleSet{freshRequesterBAttribute}));
      REQUIRE_NOTHROW(requesterA->publishObjectClassAttributes(
          freshRequesterAClass, AttributeHandleSet{freshRequesterAAttribute}));
      REQUIRE_NOTHROW(requesterB->publishObjectClassAttributes(
          freshRequesterBClass, AttributeHandleSet{freshRequesterBAttribute}));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(requesterA->disableCallbacks());
        REQUIRE_NOTHROW(requesterB->disableCallbacks());
      }
      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainLive(*owner, *requesterA, *requesterB);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterAReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterBReports.federationRestoreBegunReportCount == 1U);
        REQUIRE(requesterAReports.attributeOwnershipAcquisitionReports.empty());
        REQUIRE(requesterBReports.attributeOwnershipAcquisitionReports.empty());
      }

      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(requesterA->federateRestoreComplete());
      REQUIRE_NOTHROW(requesterB->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(requesterA->enableCallbacks());
        REQUIRE_NOTHROW(requesterB->enableCallbacks());
      } else {
        drainLive(*owner, *requesterA, *requesterB);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(requesterAReports.federationRestoredReportCount == 1U);
      REQUIRE(requesterBReports.federationRestoredReportCount == 1U);
      REQUIRE(ownerReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(requesterAReports.attributeOwnershipAcquisitionReports.size() == 1U);
      REQUIRE(requesterBReports.attributeOwnershipAcquisitionReports.size() == 1U);
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
          requesterAReports, AttributeHandleSet{freshRequesterAAttribute});
      verifyRequesterNotification(
          requesterBReports, AttributeHandleSet{freshRequesterBAttribute});

      // Both restored reservations are one-shot.  A later evoke pass
      // must not replay any recipient's notification.
      if (callbackModel == HLA_EVOKED) {
        drainLive(*owner, *requesterA, *requesterB);
      }
      REQUIRE(requesterAReports.attributeOwnershipAcquisitionReports.size() == 1U);
      REQUIRE(requesterBReports.attributeOwnershipAcquisitionReports.size() == 1U);

      REQUIRE_NOTHROW(requesterA->unpublishObjectClassAttributes(
          freshRequesterAClass, AttributeHandleSet{freshRequesterAAttribute}));
      REQUIRE_NOTHROW(requesterB->unpublishObjectClassAttributes(
          freshRequesterBClass, AttributeHandleSet{freshRequesterBAttribute}));
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshAttributeA, freshAttributeB}));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(requesterA->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(requesterB->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(requesterA->disconnect());
      REQUIRE_NOTHROW(requesterB->disconnect());
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
