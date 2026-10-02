#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds pending Request Attribute Ownership Assumption through HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][restore][ownership-ledger-state]"
    "[process-restart-ownership-assumption-search]"
    "[public-process-restart-ownership-assumption-search]"
    "[ownership-assumption-research][callback-immediate]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance]"
    "[rti.service.unconditional-attribute-ownership-divestiture]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.request-attribute-ownership-assumption]"
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
        ? std::wstring{L"public-pending-ownership-assumption-evoked"}
        : std::wstring{L"public-pending-ownership-assumption-immediate"};
    std::vector<unsigned char> const assumptionTagBytes{
        0xA5U, 0x55U, 0x25U};
    VariableLengthData const assumptionTag(
        assumptionTagBytes.data(), assumptionTagBytes.size());
    std::vector<unsigned char> const valueBytes{0x4FU, 0x57U, 0x4EU};

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
    FederateHandle sourceCandidateHandle;
    ObjectInstanceHandle sourceObjectInstance;
    ObjectClassHandle sourceObjectClass;
    AttributeHandle sourceAttribute;

    {
      auto const sourceRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador candidateReports;
      auto owner = makeRti();
      auto candidate = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(candidate->connect(candidateReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
          L"public-pending-ownership-assumption-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(sourceCandidateHandle = candidate->joinFederationExecution(
          L"public-pending-ownership-assumption-candidate",
          L"candidate",
          federationName));

      sourceObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      sourceAttribute = owner->getAttributeHandle(
          sourceObjectClass, fixture_hla::fixture::reliable_child);
      REQUIRE(sourceObjectClass.isValid());
      REQUIRE(sourceAttribute.isValid());
      AttributeHandleSet const attributes{sourceAttribute};
      REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
          sourceObjectClass, attributes));
      REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
          sourceObjectClass, attributes));
      REQUIRE_NOTHROW(sourceObjectInstance =
                          owner->registerObjectInstance(sourceObjectClass));
      REQUIRE(sourceObjectInstance.isValid());
      drain(*candidate);
      REQUIRE(candidateReports.objectDiscoveryReports.size() == 1U);
      REQUIRE(candidateReports.objectDiscoveryReports.front().objectInstance ==
              sourceObjectInstance);
      REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
          sourceObjectClass, attributes));
      AttributeHandleValueMap values;
      values.emplace(
          sourceAttribute,
          VariableLengthData(valueBytes.data(), valueBytes.size()));
      VariableLengthData emptyValueTag;
      REQUIRE_NOTHROW(owner->updateAttributeValues(
          sourceObjectInstance, values, emptyValueTag));
      drain(*candidate);
      REQUIRE(candidateReports.attributeReflectionReports.size() == 1U);

      // HLA_IMMEDIATE would normally enter the candidate callback on the
      // divestiture call's stack. Disable both routes so the save captures the
      // same pre-callback boundary as HLA_EVOKED.
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(candidate->disableCallbacks());
      }
      REQUIRE_NOTHROW(owner->unconditionalAttributeOwnershipDivestiture(
          sourceObjectInstance, attributes, assumptionTag));
      REQUIRE(candidateReports.attributeOwnershipAssumptionReports.empty());
      // Keep the former owner out of the continuing assumption search. The
      // restored-callback assertion is intentionally about the saved
      // candidate route, not a second offer to the divesting federate after
      // the first callback returns without acquiring ownership.
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          sourceObjectClass, attributes));

      REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
      REQUIRE_NOTHROW(owner->federateSaveBegun());
      REQUIRE_NOTHROW(candidate->federateSaveBegun());
      REQUIRE_NOTHROW(owner->federateSaveComplete());
      REQUIRE_NOTHROW(candidate->federateSaveComplete());

      auto const durable = saveStore->load(federationName, saveLabel);
      REQUIRE(durable.has_value());
      auto const durableImage = umbra::detail::FederationStateImageCodec::decode(
          durable->stateImage);
      REQUIRE(durableImage.pendingAttributeOwnershipAssumptionsPresent);
      REQUIRE(durableImage.pendingAttributeOwnershipAssumptions.size() == 1U);
      auto const& pending = durableImage.pendingAttributeOwnershipAssumptions.front();
      auto const sourceCandidateValue =
          rti1516_2025::umbra_binding_detail::federateHandleValue(
              sourceCandidateHandle);
      auto const sourceObjectValue =
          rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              sourceObjectInstance);
      auto const sourceAttributeValue =
          rti1516_2025::umbra_binding_detail::attributeHandleValue(
              sourceAttribute);
      REQUIRE(sourceCandidateValue.has_value());
      REQUIRE(sourceObjectValue.has_value());
      REQUIRE(sourceAttributeValue.has_value());
      REQUIRE(pending.objectInstanceHandle == *sourceObjectValue);
      REQUIRE(pending.receivingFederateId == *sourceCandidateValue);
      REQUIRE(pending.attributeHandles ==
              std::vector<std::uint64_t>{*sourceAttributeValue});
      REQUIRE(pending.userSuppliedTag ==
              std::string(reinterpret_cast<char const*>(assumptionTagBytes.data()),
                          assumptionTagBytes.size()));

      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(candidate->disconnect());
    }

    {
      auto const freshRegistry =
          std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
              nullptr, saveStore);
      ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
      ReportingFederateAmbassador ownerReports;
      ReportingFederateAmbassador candidateReports;
      auto owner = makeRti();
      auto candidate = makeRti();

      REQUIRE_NOTHROW(owner->connect(ownerReports, callbackModel));
      REQUIRE_NOTHROW(candidate->connect(candidateReports, callbackModel));
      REQUIRE_NOTHROW(owner->createFederationExecution(
          federationName, fomModule, standard_hla::mom::integer64_time));
      FederateHandle freshOwnerHandle;
      FederateHandle freshCandidateHandle;
      REQUIRE_NOTHROW(freshOwnerHandle = owner->joinFederationExecution(
          L"public-pending-ownership-assumption-owner",
          L"publisher",
          federationName));
      REQUIRE_NOTHROW(freshCandidateHandle = candidate->joinFederationExecution(
          L"public-pending-ownership-assumption-candidate",
          L"candidate",
          federationName));
      REQUIRE(freshOwnerHandle == sourceOwnerHandle);
      REQUIRE(freshCandidateHandle == sourceCandidateHandle);

      auto const freshObjectClass = owner->getObjectClassHandle(
          fixture_hla::fom::attribute_fixture_child);
      auto const freshAttribute = owner->getAttributeHandle(
          freshObjectClass, fixture_hla::fixture::reliable_child);
      REQUIRE(freshObjectClass == sourceObjectClass);
      REQUIRE(freshAttribute == sourceAttribute);
      AttributeHandleSet const attributes{freshAttribute};
      REQUIRE_NOTHROW(candidate->subscribeObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(candidate->publishObjectClassAttributes(
          freshObjectClass, attributes));

      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->disableCallbacks());
        REQUIRE_NOTHROW(candidate->disableCallbacks());
      }
      candidateReports.onRequestAttributeOwnershipAssumption = [&] {
        candidateReports.callbackOrder.push_back("assumption");
      };

      REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
      if (callbackModel == HLA_EVOKED) {
        drainBoth(*owner, *candidate);
        REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
                std::vector<std::wstring>{saveLabel});
        REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
        REQUIRE(candidateReports.initiateFederateRestoreReports.size() == 1U);
        REQUIRE(candidateReports.attributeOwnershipAssumptionReports.empty());
      }

      REQUIRE_NOTHROW(owner->federateRestoreComplete());
      REQUIRE_NOTHROW(candidate->federateRestoreComplete());
      if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
        REQUIRE_NOTHROW(owner->enableCallbacks());
        REQUIRE_NOTHROW(candidate->enableCallbacks());
      } else {
        drainBoth(*owner, *candidate);
      }

      REQUIRE(ownerReports.federationRestoredReportCount == 1U);
      REQUIRE(candidateReports.federationRestoredReportCount == 1U);
      REQUIRE(ownerReports.attributeOwnershipAssumptionReports.empty());
      REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1U);
      auto const& assumption =
          candidateReports.attributeOwnershipAssumptionReports.front();
      REQUIRE(assumption.objectInstance == sourceObjectInstance);
      REQUIRE(assumption.attributes == attributes);
      REQUIRE(variableLengthDataBytes(assumption.userSuppliedTag) ==
              assumptionTagBytes);
      REQUIRE_FALSE(candidate->isAttributeOwnedByFederate(
          sourceObjectInstance, sourceAttribute));

      auto const restoredComplete = std::find(
          candidateReports.callbackOrder.begin(),
          candidateReports.callbackOrder.end(),
          "restore-complete");
      auto const assumptionCallback = std::find(
          candidateReports.callbackOrder.begin(),
          candidateReports.callbackOrder.end(),
          "assumption");
      REQUIRE(restoredComplete != candidateReports.callbackOrder.end());
      REQUIRE(assumptionCallback != candidateReports.callbackOrder.end());
      REQUIRE(restoredComplete < assumptionCallback);

      // The durable callback tuple is consumed exactly once at callback entry;
      // a later evoke pass must not replay the offer.
      drain(*candidate);
      REQUIRE(candidateReports.attributeOwnershipAssumptionReports.size() == 1U);

      REQUIRE_NOTHROW(candidate->unpublishObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->unpublishObjectClassAttributes(
          freshObjectClass, attributes));
      REQUIRE_NOTHROW(owner->resignFederationExecution(
          CANCEL_THEN_DELETE_THEN_DIVEST));
      REQUIRE_NOTHROW(candidate->resignFederationExecution(NO_ACTION));
      REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
      REQUIRE_NOTHROW(owner->disconnect());
      REQUIRE_NOTHROW(candidate->disconnect());
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
