#include "ieee1516_2025_federation_management_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_federation_listing_test_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"

namespace {
TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending Query Attribute Ownership callback under HLA_EVOKED and HLA_IMMEDIATE",
    "[integration][development-profile][federation-management][ownership-management][save-restore]"
    "[durable-save][filesystem][process-restart][public-process-restart-pending-attribute-ownership-query]"
    "[restore][pending-application-request]"
    "[pending-attribute-ownership-query][query-attribute-ownership]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.subscribe-object-class-attributes][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.query-attribute-ownership][rti.service.request-federation-save]"
    "[rti.service.federate-save-begun][rti.service.federate-save-complete]"
    "[rti.service.request-federation-restore][rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.inform-attribute-ownership][federate.callback.initiate-federate-save]"
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
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "attribute-update-passel-fom.xml")
          .wstring();
   auto const saveLabel = callbackModel == HLA_EVOKED
       ? std::wstring{L"pending-ownership-query-evoked"}
       : std::wstring{L"pending-ownership-query-immediate"};
  std::vector<unsigned char> const valueBytes{0x71U, 0x75U, 0x65U, 0x72U, 0x79U};

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

  ObjectInstanceHandle sourceObjectInstance;
  ObjectClassHandle sourceObjectClass;
  AttributeHandle sourceAttribute;
  FederateHandle sourceOwnerHandle;
  FederateHandle sourceRequesterHandle;

  {
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
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
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(sourceRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));

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

    AttributeHandleValueMap values;
    values.emplace(
        sourceAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData emptyTag;
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObjectInstance, values, emptyTag));
    drain(*requester);
    REQUIRE(requesterReports.attributeReflectionReports.size() == 1U);

    // Leave the ownership-result callback evoked but undelivered. The save
    // image must retain the accepted request rather than only the queued
    // process-local closure.
    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->disableCallbacks());
    }
    REQUIRE_NOTHROW(requester->queryAttributeOwnership(
        sourceObjectInstance, attributes));
    REQUIRE(requesterReports.attributeOwnershipReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());
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
            sourceObjectInstance);
    REQUIRE(sourceObjectValue.has_value());
    REQUIRE(query.requestingFederateId == *sourceRequesterValue);
    REQUIRE(query.objectInstanceHandle == *sourceObjectValue);
    REQUIRE(query.reportKind == 0U);
    REQUIRE(query.requestedAttributeHandles.size() == 1U);

    if (callbackModel == rti1516_2025::HLA_IMMEDIATE) {
      REQUIRE_NOTHROW(requester->enableCallbacks());
    }
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(requester->disconnect());
    REQUIRE_NOTHROW(owner->disconnect());
  }

  {
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(nullptr,
                                                                     saveStore);
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
                        L"pending-query-owner", L"publisher", federationName));
    REQUIRE_NOTHROW(freshRequesterHandle = requester->joinFederationExecution(
                        L"pending-query-requester", L"subscriber", federationName));
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
            ReportingFederateAmbassador::AttributeOwnershipReport::Kind::federate);
    REQUIRE(ownership.objectInstance == sourceObjectInstance);
    REQUIRE(ownership.attributes == attributes);
    REQUIRE(ownership.owner == freshOwnerHandle);

    // The restored request is one-shot; a second evoke pass cannot replay it.
    drain(*requester);
    REQUIRE(requesterReports.attributeOwnershipReports.size() == 1U);

    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
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
} // namespace
