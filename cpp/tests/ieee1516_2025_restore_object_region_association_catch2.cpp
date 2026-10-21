#include "ieee1516_2025_public_federation_restore_test_support.hpp"
#include "ieee1516_2025_federation_management_fixture_support.hpp"
#include "internal/federation/federation_state_image.hpp"

TEST_CASE(
    "Embedded public fresh-registry restore rebinds one saved object-region association",
    "[integration][development-profile][federation-management][object-management]"
    "[ddm][save-restore][durable-save][filesystem][fresh-registry]"
    "[restore-member-identity-rebinding]"
    "[rti.service.get-object-class-handle][rti.service.get-attribute-handle]"
    "[rti.service.get-dimension-handle][rti.service.publish-object-class-attributes]"
    "[rti.service.register-object-instance][rti.service.update-attribute-values]"
    "[rti.service.create-region][rti.service.set-range-bounds]"
    "[rti.service.commit-region-modifications][rti.service.associate-regions-for-updates]"
    "[rti.service.subscribe-object-class-attributes-with-regions]"
    "[rti.service.unassociate-regions-for-updates][rti.service.delete-region]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.resign-federation-execution]"
    "[federate.callback.discover-object-instance][federate.callback.reflect-attribute-values]"
    "[federate.callback.initiate-federate-save][federate.callback.federation-saved]"
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
  auto const federationName = nextFederationName();
  auto const fomModule =
      (std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data" /
       "regional-ownership-fanout-fom.xml")
          .wstring();
  std::wstring const saveLabel = L"single-object-region-association-rebind";
  std::vector<unsigned char> const initialBytes{0x31U, 0x41U};
  std::vector<unsigned char> const updatedBytes{0x59U, 0x26U};
  auto const drain = [](
                          RTIambassador& first,
                          RTIambassador& second) {
    for (int pass = 0; pass != 32; ++pass) {
      static_cast<void>(first.evokeCallback(0.0));
      static_cast<void>(second.evokeCallback(0.0));
    }
  };

  FederateHandle savedPublisherHandle;
  FederateHandle savedReceiverHandle;
  ObjectClassHandle objectClass;
  AttributeHandle attribute;
  DimensionHandle dimension;
  ObjectInstanceHandle objectInstance;
  RegionHandle sourceRegion;
  {
    auto const sourceStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const sourceRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, sourceStore);
    ScopedEmbeddedFederationRegistry sourceScope(sourceRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(savedPublisherHandle = publisher->joinFederationExecution(
        L"saved-region-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(savedReceiverHandle = receiver->joinFederationExecution(
        L"saved-region-receiver", L"participant", federationName));

    objectClass = publisher->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    attribute = publisher->getAttributeHandle(objectClass, L"ProviderAValue");
    dimension = publisher->getDimensionHandle(L"UmbraRegionX");
    REQUIRE(objectClass.isValid());
    REQUIRE(attribute.isValid());
    REQUIRE(dimension.isValid());
    REQUIRE_NOTHROW(publisher->publishObjectClassAttributes(
        objectClass, AttributeHandleSet{attribute}));

    REQUIRE_NOTHROW(sourceRegion =
        publisher->createRegion(DimensionHandleSet{dimension}));
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion, dimension, RangeBounds(2UL, 4UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(
        RegionHandleSet{sourceRegion}));
    REQUIRE_NOTHROW(objectInstance =
        publisher->registerObjectInstance(objectClass));
    AttributeHandleSetRegionHandleSetPairVector const sourceAssociation{{
        AttributeHandleSet{attribute}, RegionHandleSet{sourceRegion}}};
    REQUIRE_NOTHROW(publisher->associateRegionsForUpdates(
        objectInstance, sourceAssociation));

    AttributeHandleValueMap initialValues;
    initialValues.emplace(
        attribute,
        VariableLengthData(initialBytes.data(), initialBytes.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        objectInstance, initialValues, VariableLengthData{}));

    REQUIRE_NOTHROW(publisher->requestFederationSave(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(receiverReports.initiateFederateSaveReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE_NOTHROW(publisher->federateSaveBegun());
    REQUIRE_NOTHROW(receiver->federateSaveBegun());
    REQUIRE_NOTHROW(publisher->federateSaveComplete());
    REQUIRE_NOTHROW(receiver->federateSaveComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationSavedReportCount == 1U);
    REQUIRE(receiverReports.federationSavedReportCount == 1U);

    auto const durable = sourceStore->load(federationName, saveLabel);
    REQUIRE(durable.has_value());
    auto const image = umbra::detail::FederationStateImageCodec::decode(
        durable->stateImage);
    REQUIRE(image.objectInstanceCount == 1U);
    REQUIRE(image.objects.size() == 1U);
    REQUIRE(image.regionCount == 1U);
    REQUIRE(image.regions.size() == 1U);
    REQUIRE(image.regions.front().inUse);
    REQUIRE(image.regions.front().specificationCommitted);
    REQUIRE(image.regions.front().ownerFederateId ==
            image.objects.front().producingFederateId);
    REQUIRE(image.objects.front().attributeValues.size() == 1U);
    auto const savedAssociation = std::find_if(
        image.objects.front().attributes.begin(),
        image.objects.front().attributes.end(),
        [attribute](umbra::detail::FederationStateImageObjectAttribute const& item) {
          return item.handle ==
              rti1516_2025::umbra_binding_detail::attributeHandleValue(attribute)
                  .value();
        });
    REQUIRE(savedAssociation != image.objects.front().attributes.end());
    REQUIRE(savedAssociation->updateRegionHandles ==
            std::vector<std::uint64_t>{image.regions.front().handle});
    REQUIRE(image.objects.front().attributeValues.front().value ==
            std::string(initialBytes.begin(), initialBytes.end()));

    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }

  {
    auto const freshStore =
        std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
            saveDirectory.path());
    auto const freshRegistry =
        std::make_shared<umbra::detail::EmbeddedFederationRegistry>(
            nullptr, freshStore);
    ScopedEmbeddedFederationRegistry freshScope(freshRegistry);
    ReportingFederateAmbassador publisherReports;
    ReportingFederateAmbassador receiverReports;
    auto publisher = makeRti();
    auto receiver = makeRti();

    REQUIRE_NOTHROW(publisher->connect(
        publisherReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(receiver->connect(
        receiverReports, HLA_EVOKED, configuration));
    REQUIRE_NOTHROW(publisher->createFederationExecution(
        federationName, fomModule, standard_hla::mom::integer64_time));
    REQUIRE_NOTHROW(publisher->joinFederationExecution(
        L"burned-region-first", L"participant", federationName));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(receiver->joinFederationExecution(
        L"burned-region-second", L"participant", federationName));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    FederateHandle restoredPublisherHandle;
    FederateHandle restoredReceiverHandle;
    REQUIRE_NOTHROW(restoredPublisherHandle = publisher->joinFederationExecution(
        L"restored-region-publisher", L"participant", federationName));
    REQUIRE_NOTHROW(restoredReceiverHandle = receiver->joinFederationExecution(
        L"restored-region-receiver", L"participant", federationName));
    REQUIRE(restoredPublisherHandle != savedPublisherHandle);
    REQUIRE(restoredReceiverHandle != savedReceiverHandle);

    REQUIRE_NOTHROW(publisher->requestFederationRestore(saveLabel));
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.requestFederationRestoreSucceededReports ==
            std::vector<std::wstring>{saveLabel});
    REQUIRE(publisherReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(receiverReports.initiateFederateRestoreReports.size() == 1U);
    REQUIRE(publisherReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-region-publisher");
    REQUIRE(receiverReports.initiateFederateRestoreReports.front().federateName ==
            L"saved-region-receiver");
    REQUIRE_NOTHROW(publisher->federateRestoreComplete());
    REQUIRE_NOTHROW(receiver->federateRestoreComplete());
    drain(*publisher, *receiver);
    REQUIRE(publisherReports.federationRestoredReportCount == 1U);
    REQUIRE(receiverReports.federationRestoredReportCount == 1U);

    auto const restoredBounds = publisher->getRangeBounds(sourceRegion, dimension);
    REQUIRE(restoredBounds.getLowerBound() == 2UL);
    REQUIRE(restoredBounds.getUpperBound() == 4UL);
    // This mutation verifies the restored region owner was rebound to the live
    // publisher, not left pointing at the saved registry's federate identity.
    REQUIRE_NOTHROW(publisher->setRangeBounds(
        sourceRegion, dimension, RangeBounds(1UL, 4UL)));
    REQUIRE_NOTHROW(publisher->commitRegionModifications(
        RegionHandleSet{sourceRegion}));

    auto const restoredObject = objectInstance;
    objectClass = publisher->getObjectClassHandle(
        L"HLAobjectRoot.UmbraRegionalOwnershipFanout");
    attribute = publisher->getAttributeHandle(objectClass, L"ProviderAValue");
    dimension = receiver->getDimensionHandle(L"UmbraRegionX");
    auto const receiverRegion = receiver->createRegion(DimensionHandleSet{dimension});
    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion, dimension, RangeBounds(5UL, 7UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(
        RegionHandleSet{receiverRegion}));
    AttributeHandleSetRegionHandleSetPairVector const receiverSubscription{{
        AttributeHandleSet{attribute}, RegionHandleSet{receiverRegion}}};
    REQUIRE_NOTHROW(receiver->subscribeObjectClassAttributesWithRegions(
        objectClass, receiverSubscription, true));
    drain(*publisher, *receiver);
    CHECK(receiverReports.objectDiscoveryReports.empty());
    CHECK(receiverReports.attributeReflectionReports.empty());

    REQUIRE_NOTHROW(receiver->setRangeBounds(
        receiverRegion, dimension, RangeBounds(3UL, 5UL)));
    REQUIRE_NOTHROW(receiver->commitRegionModifications(
        RegionHandleSet{receiverRegion}));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.objectDiscoveryReports.size() == 1U);
    REQUIRE(receiverReports.objectDiscoveryReports.front().objectInstance ==
            objectInstance);
    REQUIRE(receiverReports.objectDiscoveryReports.front().producingFederate ==
            restoredPublisherHandle);
    receiverReports.attributeReflectionReports.clear();

    AttributeHandleValueMap updatedValues;
    updatedValues.emplace(
        attribute,
        VariableLengthData(updatedBytes.data(), updatedBytes.size()));
    REQUIRE_NOTHROW(publisher->updateAttributeValues(
        restoredObject, updatedValues, VariableLengthData{}));
    drain(*publisher, *receiver);
    REQUIRE(receiverReports.attributeReflectionReports.size() == 1U);
    REQUIRE(receiverReports.attributeReflectionReports.front().objectInstance ==
            objectInstance);
    REQUIRE(receiverReports.attributeReflectionReports.front().producingFederate ==
            restoredPublisherHandle);
    REQUIRE(variableLengthDataBytes(
                receiverReports.attributeReflectionReports.front()
                    .attributeValues.at(attribute)) == updatedBytes);

    REQUIRE_NOTHROW(receiver->unsubscribeObjectClassAttributesWithRegions(
        objectClass, receiverSubscription));
    REQUIRE_NOTHROW(receiver->deleteRegion(receiverRegion));
    AttributeHandleSetRegionHandleSetPairVector const sourceAssociation{{
        AttributeHandleSet{attribute}, RegionHandleSet{sourceRegion}}};
    REQUIRE_NOTHROW(publisher->unassociateRegionsForUpdates(
        restoredObject, sourceAssociation));
    REQUIRE_NOTHROW(publisher->deleteRegion(sourceRegion));
    REQUIRE_NOTHROW(receiver->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->resignFederationExecution(NO_ACTION));
    REQUIRE_NOTHROW(publisher->destroyFederationExecution(federationName));
    REQUIRE_NOTHROW(receiver->disconnect());
    REQUIRE_NOTHROW(publisher->disconnect());
  }
}
