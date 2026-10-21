#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "ieee1516_2025_public_federation_restore_test_support.hpp"
#include "internal/federation/federation_state_image.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

TEST_CASE(
    "Embedded public fresh-registry restore rebinds a pending regular ownership transaction",
    "[integration][development-profile][federation-management][ownership-management]"
    "[save-restore][durable-save][filesystem][fresh-registry][multi-federate]"
    "[ownership-ledger-state][restore-member-identity-rebinding]"
    "[rti.service.connect][rti.service.create-federation-execution]"
    "[rti.service.join-federation-execution]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.publish-object-class-attributes]"
    "[rti.service.subscribe-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.attribute-ownership-acquisition]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete]"
    "[rti.service.resign-federation-execution]"
    "[rti.service.destroy-federation-execution][rti.service.disconnect]"
    "[federate.callback.discover-object-instance]"
    "[federate.callback.reflect-attribute-values]"
    "[federate.callback.request-attribute-ownership-release]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
    "[federate.callback.request-federation-restore-succeeded]"
    "[federate.callback.initiate-federate-restore]"
    "[federate.callback.federation-restored]"
    "[requirement:hla-1516.1-2025:clause-4.1.2][2025]") {
  using public_federation_restore_test_support::ScopedEmbeddedFederationRegistry;
  using public_federation_restore_test_support::configurationForServiceReportDirectory;
  using public_federation_restore_test_support::temporaryFederationSaveDirectory;
  using public_federation_restore_test_support::temporaryServiceReportDirectory;

  auto const saveDirectory = temporaryFederationSaveDirectory();
  auto const reportDirectory = temporaryServiceReportDirectory();
  auto configuration =
      configurationForServiceReportDirectory(reportDirectory.path());
  configuration.withRtiAddress(L"in-process");
  auto const saveStore =
      std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
          saveDirectory.path());
  auto const federationName = nextFederationName();
  auto const attributeFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "attribute-update-passel-fom.xml")
          .wstring();
  auto const switchFom =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" /
       "data" / "switch-support-enabled-fom.xml")
          .wstring();
  std::vector<std::wstring> const fomModules{attributeFom, switchFom};
  std::wstring const saveLabel = L"same-type-ownership-identity-rebinding";
  std::vector<unsigned char> const valueBytes{0x71U, 0x72U};
  std::vector<unsigned char> const acquisitionTagBytes{
      0x52U, 0x45U, 0x42U, 0x49, 0x4EU, 0x44U};
  VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(), acquisitionTagBytes.size());
  FederateHandle sourceOwnerHandle;
  ObjectInstanceHandle sourceObject;
  AttributeHandle sourceAttribute;
  std::uint64_t sourceOwnerId = 0U;
  std::uint64_t sourceRequesterId = 0U;

  auto const drain = [](RTIambassador& first, RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
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

    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(sourceOwnerHandle = owner->joinFederationExecution(
        L"saved-ownership-owner", L"participant", federationName));
    auto const sourceRequesterHandle = requester->joinFederationExecution(
        L"saved-ownership-requester", L"participant", federationName);

    auto const ownerId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceOwnerHandle);
    auto const requesterId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            sourceRequesterHandle);
    REQUIRE(ownerId.has_value());
    REQUIRE(requesterId.has_value());
    sourceOwnerId = *ownerId;
    sourceRequesterId = *requesterId;
    REQUIRE(sourceOwnerId != sourceRequesterId);

    suppressDeclarationRelevanceAdvisories(*owner);
    suppressDeclarationRelevanceAdvisories(*requester);
    auto const objectClass = owner->getObjectClassHandle(
        L"HLAobjectRoot.UmbraAttributeFixtureBase.UmbraAttributeFixtureChild");
    sourceAttribute = owner->getAttributeHandle(objectClass, L"ReliableBaseA");
    REQUIRE(objectClass.isValid());
    REQUIRE(sourceAttribute.isValid());
    auto const sourceAttributeId =
        rti1516_2025::umbra_binding_detail::attributeHandleValue(
            sourceAttribute);
    REQUIRE(sourceAttributeId.has_value());
    AttributeHandleSet const attributes{sourceAttribute};
    REQUIRE_NOTHROW(requester->subscribeObjectClassAttributes(
        objectClass, attributes));
    REQUIRE_NOTHROW(owner->publishObjectClassAttributes(
        objectClass, attributes));
    REQUIRE_NOTHROW(sourceObject = owner->registerObjectInstance(objectClass));
    REQUIRE(sourceObject.isValid());
    drain(*owner, *requester);
    REQUIRE(requesterReports.objectDiscoveryReports.size() == 1U);

    REQUIRE_NOTHROW(requester->publishObjectClassAttributes(
        objectClass, attributes));
    AttributeHandleValueMap values;
    values.emplace(
        sourceAttribute,
        VariableLengthData(valueBytes.data(), valueBytes.size()));
    VariableLengthData valueTag;
    REQUIRE_NOTHROW(owner->updateAttributeValues(
        sourceObject, values, valueTag));
    drain(*owner, *requester);
    REQUIRE_NOTHROW(requester->attributeOwnershipAcquisition(
        sourceObject, attributes, acquisitionTag));
    REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.empty());

    REQUIRE_NOTHROW(owner->requestFederationSave(saveLabel));
    REQUIRE_NOTHROW(owner->federateSaveBegun());
    REQUIRE_NOTHROW(requester->federateSaveBegun());
    REQUIRE_NOTHROW(owner->federateSaveComplete());
    REQUIRE_NOTHROW(requester->federateSaveComplete());

    auto const durable = saveStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.members.size() == 2U);
    REQUIRE(image.objects.size() == 1U);
    auto const& savedObject = image.objects.front();
    REQUIRE(savedObject.producingFederateId == sourceOwnerId);
    REQUIRE(savedObject.pendingAttributeOwnershipAcquisitionRequests.size() ==
            1U);
    auto const& savedRequest =
        savedObject.pendingAttributeOwnershipAcquisitionRequests.front();
    REQUIRE(savedRequest.requestingFederateId == sourceRequesterId);
    REQUIRE(savedRequest.desiredAttributeHandles ==
            std::vector<std::uint64_t>{*sourceAttributeId});
    REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.size() == 1U);
    REQUIRE(savedRequest.releaseCallbacksQueuedByOwningFederate.front().first ==
            sourceOwnerId);
    REQUIRE(savedRequest.userSuppliedTag ==
            std::string(acquisitionTagBytes.begin(), acquisitionTagBytes.end()));

    drain(*owner, *requester);
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
    REQUIRE_NOTHROW(owner->connect(ownerReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(requester->connect(requesterReports, HLA_EVOKED));
    REQUIRE_NOTHROW(owner->createFederationExecution(
        federationName,
        fomModules,
        standard_hla::mom::integer64_time));

    REQUIRE_NOTHROW(owner->joinFederationExecution(
        L"burned-ownership-member-one", L"participant", federationName));
    REQUIRE_NOTHROW(requester->joinFederationExecution(
        L"burned-ownership-member-two", L"participant", federationName));
    REQUIRE_NOTHROW(owner->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));

    FederateHandle restoredOwnerHandle;
    FederateHandle restoredRequesterHandle;
    REQUIRE_NOTHROW(restoredOwnerHandle = owner->joinFederationExecution(
        L"restored-ownership-owner", L"participant", federationName));
    REQUIRE_NOTHROW(restoredRequesterHandle = requester->joinFederationExecution(
        L"restored-ownership-requester", L"participant", federationName));
    auto const restoredOwnerId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            restoredOwnerHandle);
    auto const restoredRequesterId =
        rti1516_2025::umbra_binding_detail::federateHandleValue(
            restoredRequesterHandle);
    REQUIRE(restoredOwnerId.has_value());
    REQUIRE(restoredRequesterId.has_value());
    REQUIRE_FALSE(*restoredOwnerId == sourceOwnerId);
    REQUIRE_FALSE(*restoredRequesterId == sourceRequesterId);

    REQUIRE_NOTHROW(owner->requestFederationRestore(saveLabel));
    drain(*owner, *requester);
    REQUIRE(ownerReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(ownerReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(requesterReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE_NOTHROW(owner->federateRestoreComplete());
    REQUIRE_NOTHROW(requester->federateRestoreComplete());
    drain(*owner, *requester);
    REQUIRE(ownerReports.federationRestoredReportCount == 1U);
    REQUIRE(requesterReports.federationRestoredReportCount == 1U);

    REQUIRE(ownerReports.attributeOwnershipReleaseRequestReports.size() == 1U);
    auto const& releaseRequest =
        ownerReports.attributeOwnershipReleaseRequestReports.front();
    REQUIRE(releaseRequest.objectInstance == sourceObject);
    REQUIRE(releaseRequest.attributes == AttributeHandleSet{sourceAttribute});
    CHECK(variableLengthDataBytes(releaseRequest.userSuppliedTag) ==
          acquisitionTagBytes);
    CHECK(requesterReports.attributeOwnershipAcquisitionReports.empty());

    REQUIRE_NOTHROW(owner->resignFederationExecution(
        CANCEL_THEN_DELETE_THEN_DIVEST));
    REQUIRE_NOTHROW(requester->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(owner->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(owner->disconnect());
    REQUIRE_NOTHROW(requester->disconnect());
  }
}
