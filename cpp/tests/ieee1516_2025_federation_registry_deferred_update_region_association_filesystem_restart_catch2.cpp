#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore promotes a deferred update-region association after ownership acquisition",
    "[integration][development-profile][federation-registry][save-restore][durable-save]"
    "[filesystem][process-restart][ownership-management][ddm]"
    "[deferred-update-region-association-state-image-restore]"
    "[rti.service.request-federation-save][rti.service.federate-save-begun]"
    "[rti.service.federate-save-complete][rti.service.request-federation-restore]"
    "[rti.service.federate-restore-complete][rti.service.associate-regions-for-updates]"
    "[rti.service.attribute-ownership-acquisition-if-available]"
    "[rti.service.attribute-ownership-divestiture-if-wanted]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  auto regionalDefinition = [&] {
    std::filesystem::path const testData =
        std::filesystem::path(UMBRA_SOURCE_DIRECTORY) / "cpp" / "tests" / "data";
    std::vector<PrevalidatedFomModule> modules{
        validatedModule(
            resourcePath("mim/HLAstandardMIM-2025.xml"),
            FomModuleKind::mim,
            L"urn:umbra:test:state-image-restore-mim"),
        validatedModule(
            testData / "regional-ownership-fanout-fom.xml",
            FomModuleKind::fom,
            L"urn:umbra:test:state-image-restore-ownership"),
    };
    LibXml2FomModuleComposer composer(
        resourcePath("schemas/IEEE1516-FDD-2025.xsd"));
    auto result = composer.compose(modules);
    CAPTURE(result.diagnostics);
    REQUIRE(result.status == FomCompositionStatus::valid);
    REQUIRE(result.catalog);
    REQUIRE(result.fdd);
    return FederationDefinition{
        std::move(result.modules),
        L"HLAinteger64Time",
        std::move(result.catalog),
        std::move(result.fdd),
    };
  }();
  auto restartDefinition = regionalDefinition;

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", std::move(regionalDefinition)).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"owner-type", L"owner", noOpCallbackRoute());
  auto newOwner = source.join(
      L"exercise", L"owner-type", L"new-owner", noOpCallbackRoute());
  auto receiver = source.join(
      L"exercise", L"receiver-type", L"receiver", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(newOwner.membership);
  REQUIRE(receiver.membership);

  auto const objectClass = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout");
  auto const attribute = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout", "ProviderBValue");
  auto const dimension = source.dimensionHandleFor(L"exercise", "UmbraRegionX");
  REQUIRE(objectClass.has_value());
  REQUIRE(attribute.has_value());
  REQUIRE(dimension.has_value());
  std::set<std::uint64_t> const attributes{*attribute};
  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", owner.membership->id, *objectClass, attributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributePublication(
               L"exercise", newOwner.membership->id, *objectClass, attributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto ownerRegionResult = source.createRegion(
      L"exercise", owner.membership->id, {*dimension});
  auto deferredRegionResult = source.createRegion(
      L"exercise", newOwner.membership->id, {*dimension});
  auto receiverRegionResult = source.createRegion(
      L"exercise", receiver.membership->id, {*dimension});
  REQUIRE(ownerRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(deferredRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(receiverRegionResult.status == umbra::detail::RegionServiceStatus::applied);
  auto const ownerRegion = ownerRegionResult.regionHandle;
  auto const deferredRegion = deferredRegionResult.regionHandle;
  auto const receiverRegion = receiverRegionResult.regionHandle;
  for (auto const [federateId, regionHandle] : std::vector<std::pair<std::uint64_t, std::uint64_t>>{
           {owner.membership->id, ownerRegion},
           {newOwner.membership->id, deferredRegion},
           {receiver.membership->id, receiverRegion}}) {
    REQUIRE(source.setRangeBounds(
                 L"exercise", federateId, regionHandle, *dimension,
                 umbra::detail::RegionRangeBounds{0UL, 2UL}) ==
        umbra::detail::RegionServiceStatus::applied);
    REQUIRE(source.commitRegionModifications(
                 L"exercise", federateId, {regionHandle}) ==
        umbra::detail::RegionServiceStatus::applied);
  }

  std::map<std::uint64_t, std::set<std::uint64_t>> const deferredSubscription{
      {*attribute, {deferredRegion}}};
  std::map<std::uint64_t, std::set<std::uint64_t>> const receiverSubscription{
      {*attribute, {receiverRegion}}};
  REQUIRE(source.setObjectClassAttributeRegionalSubscription(
               L"exercise", newOwner.membership->id, *objectClass,
               deferredSubscription, true) ==
      umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeRegionalSubscription(
               L"exercise", receiver.membership->id, *objectClass,
               receiverSubscription, true) ==
      umbra::detail::RegionalObjectClassAttributeDeclarationStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const ownerRegistration{
      {*attribute, {ownerRegion}}};
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *objectClass, &ownerRegistration);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 2U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", newOwner.membership->id, registered.objectInstanceHandle));
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", receiver.membership->id, registered.objectInstanceHandle));

  // Fresh-registry materialization is intentionally bounded to an object
  // whose latest application value is durable.  Seed that value through the
  // same accepted update boundary used by the process service so the restore
  // slice exercises the regional ownership ledger rather than an empty
  // registration shell.
  std::string const valueBytes{"restore"};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *attribute,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *objectClass,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  REQUIRE(source.associateRegionsForUpdates(
               L"exercise", newOwner.membership->id,
               registered.objectInstanceHandle, deferredSubscription) ==
      umbra::detail::ObjectInstanceRegionAssociationStatus::applied);
  auto beforeTransfer = source.planReceiveOrderAttributeUpdate(
      L"exercise", owner.membership->id, registered.objectInstanceHandle, {*attribute});
  REQUIRE(beforeTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(beforeTransfer.passels.size() == 1U);
  REQUIRE(beforeTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{ownerRegion});

  std::wstring const saveLabel = L"deferred-update-region-association-restore";
  REQUIRE(source.requestFederationSave(
               L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", newOwner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
               L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", newOwner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto savedImage = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(savedImage.deferredUpdateRegionAssociationsPresent);
  REQUIRE(savedImage.deferredUpdateRegionAssociations.size() == 1U);
  auto const& savedAssociation = savedImage.deferredUpdateRegionAssociations.front();
  REQUIRE(savedAssociation.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(savedAssociation.federateId == newOwner.membership->id);
  REQUIRE(savedAssociation.attributeHandle == *attribute);
  REQUIRE(savedAssociation.regionHandles == std::vector<std::uint64_t>{deferredRegion});

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", std::move(restartDefinition)).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"owner-type", L"owner", noOpCallbackRoute());
  auto restartedNewOwner = restarted.join(
      L"exercise", L"owner-type", L"new-owner", noOpCallbackRoute());
  auto restartedReceiver = restarted.join(
      L"exercise", L"receiver-type", L"receiver", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedNewOwner.membership);
  REQUIRE(restartedReceiver.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedNewOwner.membership->id == newOwner.membership->id);
  REQUIRE(restartedReceiver.membership->id == receiver.membership->id);

  REQUIRE(restarted.requestFederationRestore(
               L"exercise", restartedOwner.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedOwner.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedNewOwner.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(
               L"exercise", restartedReceiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto const restoredAttribute = restarted.attributeHandleFor(
      L"exercise", "HLAobjectRoot.UmbraRegionalOwnershipFanout", "ProviderBValue");
  REQUIRE(restoredAttribute.has_value());
  REQUIRE(*restoredAttribute == *attribute);
  auto restoredBeforeTransfer = restarted.planReceiveOrderAttributeUpdate(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute});
  REQUIRE(restoredBeforeTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(restoredBeforeTransfer.passels.size() == 1U);
  REQUIRE(restoredBeforeTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{ownerRegion});

  std::vector<unsigned char> const acquisitionTag{'r', 'e', 's', 't', 'o', 'r', 'e'};
  auto acquisition = restarted.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute}, acquisitionTag);
  REQUIRE(acquisition.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(acquisition.requestId != 0U);
  std::vector<unsigned char> const divestitureTag{'d', 'i', 'v', '-', 'r', 'e', 's', 't', 'o', 'r', 'e'};
  auto divestiture = restarted.planAttributeOwnershipDivestitureIfWanted(
      L"exercise", restartedOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute}, divestitureTag);
  REQUIRE(divestiture.status ==
      umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied);
  REQUIRE(divestiture.divestedAttributeHandles == std::set<std::uint64_t>{*restoredAttribute});
  REQUIRE(divestiture.notifications.size() == 1U);
  auto const& notification = divestiture.notifications.front();
  auto notificationDelivery = restarted.beginAttributeOwnershipDivestitureIfWantedNotification(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, notification.notificationId,
      notification.attributeHandles);
  REQUIRE(notificationDelivery.has_value());
  REQUIRE(notificationDelivery->securedAttributeHandles ==
      std::set<std::uint64_t>{*restoredAttribute});
  auto restoredOwnership = restarted.attributeOwnedByFederate(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, *restoredAttribute);
  REQUIRE(restoredOwnership.status ==
      umbra::detail::AttributeOwnershipCheckStatus::applied);
  REQUIRE(restoredOwnership.ownedByRequestingFederate);

  auto afterTransfer = restarted.planReceiveOrderAttributeUpdate(
      L"exercise", restartedNewOwner.membership->id,
      registered.objectInstanceHandle, {*restoredAttribute});
  REQUIRE(afterTransfer.status ==
      umbra::detail::ReceiveOrderAttributeUpdateStatus::applied);
  REQUIRE(afterTransfer.passels.size() == 1U);
  REQUIRE(afterTransfer.passels.front().sentRegionHandles ==
      std::set<std::uint64_t>{deferredRegion});

  std::filesystem::remove_all(directory, ignored);
}
