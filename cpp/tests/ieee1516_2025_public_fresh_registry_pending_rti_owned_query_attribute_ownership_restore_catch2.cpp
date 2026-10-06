#include "ieee1516_2025_federation_management_fixture_support.hpp"

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending RTI-owned Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][public-process-restart-pending-rti-owned-attribute-ownership-query]"
    "[restore][pending-application-request][mom]"
    "[pending-attribute-ownership-query-rti-owned][query-attribute-ownership]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.query-attribute-ownership][rti.service.is-attribute-owned-by-federate]"
    "[rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.attribute-is-owned-by-rti][federate.callback.initiate-federate-save]"
    "[federate.callback.federation-saved][federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.federation-restore-begun]"
     "[federate.callback.initiate-federate-restore][federate.callback.federation-restored]"
     "[callback-immediate][2025]") {
  auto runScenario = [](CallbackModel const callbackModel) {
  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-nrg-disabled-fom.xml")
          .wstring();
  auto const saveLabel = callbackModel == HLA_EVOKED
      ? std::wstring{L"pending-rti-owned-ownership-query-evoked"}
      : std::wstring{L"pending-rti-owned-ownership-query-immediate"};

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

  ObjectInstanceHandle sourceMomObject;
  ObjectClassHandle sourceMomClass;
  AttributeHandle sourceMomAttribute;
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceRequesterHandle;

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
                        L"pending-rti-owned-query-owner", L"observer",
                        federationName));
    REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
                        L"pending-rti-owned-query-requester", L"observer",
                        federationName));

    sourceMomClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    sourceMomAttribute = requester->getAttributeHandle(
        sourceMomClass, L"HLAfederateName");
    auto const sourceMomHandle =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
            owner.get());
    REQUIRE(sourceMomHandle != nullptr);
    auto const sourceSnapshot =
        sourceMomHandle->joinedFederateMomObjectSnapshotForTesting();
    REQUIRE(sourceSnapshot);
    sourceMomObject =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            sourceSnapshot->objectInstanceHandle);
    REQUIRE(sourceMomObject.isValid());
    REQUIRE(sourceMomClass.isValid());
    REQUIRE(sourceMomAttribute.isValid());
    AttributeHandleSet const attributes{sourceMomAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        sourceMomClass, attributes));
    drain(*requester);
    REQUIRE_FALSE(requesterReports.objectDiscoveryReports.empty());

    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        sourceMomObject, sourceMomAttribute));
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->queryAttributeOwnership(
        sourceMomObject, attributes));
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
    // Deliver only the owner's save completion.  The requester callback queue
    // intentionally retains the accepted RTI-owned ownership result until the
    // fresh-registry restore boundary.
    drain(*owner);
    REQUIRE(ownerReports.federationSavedReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.pendingAttributeOwnershipQueriesPresent);
    REQUIRE(image.pendingAttributeOwnershipQueries.size() == 1U);
    auto const& query = image.pendingAttributeOwnershipQueries.front();
    auto const sourceRequesterValue =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceRequesterHandle);
    REQUIRE(sourceRequesterValue.has_value());
    auto const sourceObjectValue =
        rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
            sourceMomObject);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(query.requestingFederateId == *sourceRequesterValue);
    REQUIRE(query.objectInstanceHandle == *sourceObjectValue);
    REQUIRE(query.reportKind == 2U);
    REQUIRE(query.owningFederateId == 0U);
    REQUIRE(query.requestedAttributeHandles.size() == 1U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
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
                        L"pending-rti-owned-query-owner", L"observer",
                        federationName));
    REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
                        L"pending-rti-owned-query-requester", L"observer",
                        federationName));
    REQUIRE(freshOwnerHandle == sourceOwnerHandle);
    REQUIRE(freshRequesterHandle == sourceRequesterHandle);

    auto const freshMomClass = requester->getObjectClassHandle(
        L"HLAobjectRoot.HLAmanager.HLAfederate");
    auto const freshMomAttribute = requester->getAttributeHandle(
        freshMomClass, L"HLAfederateName");
    REQUIRE(freshMomClass == sourceMomClass);
    REQUIRE(freshMomAttribute == sourceMomAttribute);
    AttributeHandleSet const attributes{freshMomAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        freshMomClass, attributes));
    drain(*requester);

    auto const freshMomHandle =
        dynamic_cast<rti1516_2025::umbra_binding_detail::UmbraRtiAmbassador*>(
            owner.get());
    REQUIRE(freshMomHandle != nullptr);
    auto const freshSnapshot =
        freshMomHandle->joinedFederateMomObjectSnapshotForTesting();
    REQUIRE(freshSnapshot);
    auto const freshMomObject =
        rti1516_2025::umbra_binding_detail::makeObjectInstanceHandle(
            freshSnapshot->objectInstanceHandle);
    REQUIRE(freshMomObject == sourceMomObject);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drainBoth(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);
    auto const& ownership = requesterReports.attributeOwnershipReports.front();
    REQUIRE(ownership.kind ==
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::rti);
    REQUIRE(ownership.objectInstance == freshMomObject);
    REQUIRE(ownership.attributes == attributes);
    REQUIRE_FALSE(ownership.owner.isValid());

    REQUIRE_FALSE(requester->isAttributeOwnedByFederate(
        freshMomObject, freshMomAttribute));
    drain(*requester);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }
  };

  SECTION("HLA_EVOKED") {
    runScenario(HLA_EVOKED);
  }
  SECTION("HLA_IMMEDIATE") {
    runScenario(rti1516_2025::HLA_IMMEDIATE);
  }
}
