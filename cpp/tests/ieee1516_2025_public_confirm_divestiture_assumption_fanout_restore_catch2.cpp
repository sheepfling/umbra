#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds an eligible ownership-assumption recipient beside two Confirm Divestiture notifications",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[process-restart-confirm-divestiture-post-confirmation-resignation]"
    "[public-process-restart-confirm-divestiture-post-confirmation-resignation]"
    "[public-process-restart-confirm-divestiture-assumption-fanout]"
    "[callback-immediate][mixed-post-confirmation-fanout]"
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
    "[federate.callback.request-divestiture-confirmation]"
    "[federate.callback.request-attribute-ownership-assumption]"
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
        ? std::wstring{L"public-confirm-divestiture-assumption-fanout-evoked"}
        : std::wstring{L"public-confirm-divestiture-assumption-fanout-immediate"};
    std::vector<unsigned char> const valueBytes{0x41U, 0x53U, 0x53U};
    std::vector<unsigned char> const acquisitionTagABytes{
        0x41U, 0x53U, 0x2DU, 0x41U};
    std::vector<unsigned char> const acquisitionTagBBytes{
        0x41U, 0x53U, 0x2DU, 0x42U};
    std::vector<unsigned char> const acquisitionTagCBytes{
        0x41U, 0x53U, 0x2DU, 0x43U};
    std::vector<unsigned char> const divestitureTagBytes{
        0x44U, 0x49U, 0x56U, 0x2DU, 0x41U};
    std::vector<unsigned char> const confirmationTagBytes{
        0x43U, 0x46U, 0x2DU, 0x41U};
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
          L"public-confirm-divestiture-assumption-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(sourceRequesterAHandle = requesterA->joinFederationExecution(
          L"public-confirm-divestiture-assumption-requester-a", L"subscriber", federationName));
      REQUIRE_NOTHROW(sourceRequesterBHandle = requesterB->joinFederationExecution(
          L"public-confirm-divestiture-assumption-requester-b", L"subscriber", federationName));
      REQUIRE_NOTHROW(sourceRequesterCHandle = requesterC->joinFederationExecution(
          L"public-confirm-divestiture-assumption-requester-c", L"subscriber", federationName));

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
      drainAll(*owner, *requesterA, *requesterB, *requesterC);
      REQUIRE(requesterAReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(requesterBReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(requesterCReports.objectDiscoveryReports.size() == 1U);
      REQUIRE_NOTHROW(requesterA->publishObjectClassAttributes(
          requesterAClass, requesterAAttributes));
      REQUIRE_NOTHROW(requesterB->publishObjectClassAttributes(
          requesterBClass, requesterBAttributes));
      REQUIRE_NOTHROW(requesterC->publishObjectClassAttributes(
          requesterCClass, requesterCAttributes));

      AttributeHandleValueMap values;
      values.emplace(sourceAttributeA,
                     VariableLengthData(valueBytes.data(), valueBytes.size()));
      values.emplace(sourceAttributeB,
                     VariableLengthData(valueBytes.data(), valueBytes.size()));
      values.emplace(sourceAttributeC,
                     VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyValueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyValueTag));
      drainAll(*owner, *requesterA, *requesterB, *requesterC);
      REQUIRE(requesterAReports.attributeReflectionReports.size() == 1U);
      REQUIRE(requesterBReports.attributeReflectionReports.size() == 1U);
      REQUIRE(requesterCReports.attributeReflectionReports.size() == 1U);

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

      // Keep the owner's publication for C. When C resigns, owner is the
      // eligible continuing assumption recipient while A and B retain their
      // already-confirmed notification reservations.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
      }
      REQUIRE_NOTHROW(requesterC->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE(requesterCReports.attributeOwnershipAcquisitionReports.empty());
      REQUIRE(ownerReports.attributeOwnershipAssumptionReports.empty());
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
      REQUIRE(durableImage.members.size() == 3U);
      REQUIRE(durableImage.objects.size() == 1U);
      auto const& savedObject = durableImage.objects.front();
      REQUIRE(savedObject.pendingConfirmDivestitureNotifications.size() == 2U);
      REQUIRE(savedObject.ownershipAssumptionRecipientsByAttribute.size() == 1U);
      REQUIRE(savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.size() == 1U);
      REQUIRE(savedObject.pendingOperationCount == 5U);
      REQUIRE(durableImage.pendingAttributeOwnershipAssumptionsPresent);
      REQUIRE(durableImage.pendingAttributeOwnershipAssumptions.size() == 1U);
      auto const sourceOwnerValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceOwnerHandle);
      auto const sourceRequesterAValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterAHandle);
      auto const sourceRequesterBValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterBHandle);
      auto const sourceRequesterCValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceRequesterCHandle);
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      auto const sourceAttributeAValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeA);
      auto const sourceAttributeBValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeB);
      auto const sourceAttributeCValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttributeC);
      REQUIRE(sourceOwnerValue.has_value());
      REQUIRE(sourceRequesterAValue.has_value());
      REQUIRE(sourceRequesterBValue.has_value());
      REQUIRE(sourceRequesterCValue.has_value());
      REQUIRE(sourceObjectValue.has_value());
      REQUIRE(sourceAttributeAValue.has_value());
      REQUIRE(sourceAttributeBValue.has_value());
      REQUIRE(sourceAttributeCValue.has_value());
      REQUIRE(std::none_of(
          durableImage.members.begin(), durableImage.members.end(),
          [&](umbra::detail::FederationStateImageMember const& member) {
            return member.id == *sourceRequesterCValue;
          }));
      auto const verifySavedNotification =
          [&](std::uint64_t expectedFederateId,
              std::uint64_t expectedAttributeHandle) {
            auto const match = std::find_if(
                savedObject.pendingConfirmDivestitureNotifications.begin(),
                savedObject.pendingConfirmDivestitureNotifications.end(),
                [&](umbra::detail::FederationStateImagePendingConfirmDivestiture const& notification) {
                  return notification.receivingFederateId == expectedFederateId;
                });
            REQUIRE(match != savedObject.pendingConfirmDivestitureNotifications.end());
            REQUIRE(match->attributeHandles ==
                    std::vector<std::uint64_t>{expectedAttributeHandle});
            REQUIRE(match->userSuppliedTag ==
                    std::string(reinterpret_cast<char const*>(confirmationTagBytes.data()),
                                confirmationTagBytes.size()));
          };
      verifySavedNotification(*sourceRequesterAValue, *sourceAttributeAValue);
      verifySavedNotification(*sourceRequesterBValue, *sourceAttributeBValue);
      auto const& savedAssumption =
          savedObject.ownershipAssumptionRecipientsByAttribute.front();
      REQUIRE(savedAssumption.attributeHandle == *sourceAttributeCValue);
      REQUIRE(savedAssumption.recipientFederateIds ==
              std::vector<std::uint64_t>{*sourceOwnerValue});
      auto const& savedAssumptionTag =
          savedObject.ownershipAssumptionUserSuppliedTagsByAttribute.front();
      REQUIRE(savedAssumptionTag.attributeHandle == *sourceAttributeCValue);
      REQUIRE(savedAssumptionTag.userSuppliedTag.empty());
      auto const& pendingAssumption =
          durableImage.pendingAttributeOwnershipAssumptions.front();
      REQUIRE(pendingAssumption.objectInstanceHandle == *sourceObjectValue);
      REQUIRE(pendingAssumption.receivingFederateId == *sourceOwnerValue);
      REQUIRE(pendingAssumption.attributeHandles ==
              std::vector<std::uint64_t>{*sourceAttributeCValue});
      REQUIRE(pendingAssumption.userSuppliedTag.empty());

      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          sourceObjectClass, sourceAttributes));
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
      ownerReports.onRequestAttributeOwnershipAssumption = [&] {
        ownerReports.callbackOrder.push_back("assumption");
      };

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(requesterA->connect(requesterAReports, callbackModel));
      REQUIRE_NOTHROW(requesterB->connect(requesterBReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshRequesterAHandle;
      FederateHandle freshRequesterBHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-confirm-divestiture-assumption-owner", L"publisher", federationName));
      REQUIRE_NOTHROW(freshRequesterAHandle = requesterA->joinFederationExecution(
          L"public-confirm-divestiture-assumption-requester-a", L"subscriber", federationName));
      REQUIRE_NOTHROW(freshRequesterBHandle = requesterB->joinFederationExecution(
          L"public-confirm-divestiture-assumption-requester-b", L"subscriber", federationName));
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
          AttributeHandleSet{freshAttributeA, freshAttributeB, freshAttributeC}));
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
        REQUIRE(ownerReports.attributeOwnershipAssumptionReports.empty());
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
      REQUIRE(ownerReports.attributeOwnershipAssumptionReports.size() == 1U);
      auto const& restoredAssumption =
          ownerReports.attributeOwnershipAssumptionReports.front();
      REQUIRE(restoredAssumption.objectInstance == sourceObjectInstance);
      REQUIRE(restoredAssumption.attributes ==
              AttributeHandleSet{freshAttributeC});
      REQUIRE(restoredAssumption.userSuppliedTag.size() == 0U);
      REQUIRE_FALSE(owner->isAttributeOwnedByFederate(
          sourceObjectInstance, freshAttributeC));
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

      auto const restoredComplete = std::find(
          ownerReports.callbackOrder.begin(), ownerReports.callbackOrder.end(),
          "restore-complete");
      auto const assumptionCallback = std::find(
          ownerReports.callbackOrder.begin(), ownerReports.callbackOrder.end(),
          "assumption");
      REQUIRE(restoredComplete != ownerReports.callbackOrder.end());
      REQUIRE(assumptionCallback != ownerReports.callbackOrder.end());
      REQUIRE(restoredComplete < assumptionCallback);
      if (callbackModel == HLA_EVOKED) {
        drainLive(*owner, *requesterA, *requesterB);
      }
      REQUIRE(ownerReports.attributeOwnershipAssumptionReports.size() == 1U);
      REQUIRE(requesterAReports.attributeOwnershipAcquisitionReports.size() == 1U);
      REQUIRE(requesterBReports.attributeOwnershipAcquisitionReports.size() == 1U);

      REQUIRE_NOTHROW(requesterA->unpublishObjectClassAttributes(
          freshRequesterAClass, AttributeHandleSet{freshRequesterAAttribute}));
      REQUIRE_NOTHROW(requesterB->unpublishObjectClassAttributes(
          freshRequesterBClass, AttributeHandleSet{freshRequesterBAttribute}));
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass,
          AttributeHandleSet{freshAttributeA, freshAttributeB, freshAttributeC}));
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
