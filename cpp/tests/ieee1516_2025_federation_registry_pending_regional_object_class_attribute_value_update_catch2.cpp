#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem fresh-registry restore rebinds a pending regional object-class Request Attribute Value Update",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-regional-pending-attribute-value-update][process-restart-regional-pending-attribute-value-update-negative][process-restart-regional-pending-attribute-value-update-provider-departure][pending-application-request-state][attribute-value-update]"
    "[object-management][ddm][region-state][object-lifecycle-state][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const soda = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda");
  auto const flavor = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Food.Drink.Soda", "Flavor");
  auto const dimension = source.dimensionHandleFor(L"exercise", "SodaFlavor");
  REQUIRE(soda.has_value());
  REQUIRE(flavor.has_value());
  REQUIRE(dimension.has_value());
  std::set<std::uint64_t> const flavorOnly{*flavor};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *soda, flavorOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto ownerRegion = source.createRegion(
      L"exercise", owner.membership->id, {*dimension});
  auto requesterRegion = source.createRegion(
      L"exercise", requester.membership->id, {*dimension});
  REQUIRE(ownerRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(requesterRegion.status == umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setRangeBounds(
      L"exercise", owner.membership->id, ownerRegion.regionHandle, *dimension,
      umbra::detail::RegionRangeBounds{0UL, 2UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.setRangeBounds(
      L"exercise", requester.membership->id, requesterRegion.regionHandle,
      *dimension, umbra::detail::RegionRangeBounds{1UL, 3UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", owner.membership->id, {ownerRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(source.commitRegionModifications(
      L"exercise", requester.membership->id, {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const ownerUpdateRegions{
      {*flavor, {ownerRegion.regionHandle}}};
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *soda, &ownerUpdateRegions);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  std::string const valueBytes{"regional-value", 14U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *flavor,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      owner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  std::map<std::uint64_t, std::set<std::uint64_t>> const requestRegions{
      {*flavor, {requesterRegion.regionHandle}}};
  auto planned = source.planAttributeValueUpdateClassRequest(
      L"exercise", requester.membership->id, *soda, flavorOnly,
      &requestRegions);
  REQUIRE(planned.status ==
      umbra::detail::AttributeValueUpdateClassRequestStatus::applied);
  REQUIRE(planned.recipients.size() == 1U);
  REQUIRE(planned.recipients.front().objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(planned.recipients.front().providingFederateId == owner.membership->id);
  REQUIRE(planned.recipients.front().requestedAttributeHandles == flavorOnly);

  std::vector<unsigned char> const requestTag{
      'r', 'e', 'g', '-', 'r', 'e', 's', 't', 'a', 'r', 't'};
  auto requestId = source.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      requester.membership->id,
      owner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(requestId.has_value());

  std::wstring const saveLabel =
      L"pending-regional-attribute-value-update-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", owner.membership->id, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", owner.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", requester.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(source.federateSaveComplete(
      L"exercise", owner.membership->id).saveCompletedSuccessfully);
  REQUIRE(source.federateSaveComplete(
      L"exercise", requester.membership->id).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.pendingAttributeValueUpdateRegionalRequests.size() == 1U);
  auto const& savedRequest =
      savedObject.pendingAttributeValueUpdateRegionalRequests.front();
  REQUIRE(savedRequest.requestId == *requestId);
  REQUIRE(savedRequest.requestingFederateId == requester.membership->id);
  REQUIRE(savedRequest.providingFederateId == owner.membership->id);
  REQUIRE(savedRequest.requestedObjectClassHandle == *soda);
  REQUIRE(savedRequest.requestedAttributeHandles ==
      std::vector<std::uint64_t>{*flavor});
  REQUIRE(savedRequest.requestRegionsByAttribute ==
      std::vector<std::pair<std::uint64_t, std::vector<std::uint64_t>>>{
          {*flavor, {requesterRegion.regionHandle}}});
  REQUIRE(savedRequest.userSuppliedTag ==
      std::string(requestTag.begin(), requestTag.end()));
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedOwner = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto restartedRequester = restarted.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(restartedOwner.membership);
  REQUIRE(restartedRequester.membership);
  REQUIRE(restartedOwner.membership->id == owner.membership->id);
  REQUIRE(restartedRequester.membership->id == requester.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise", restartedRequester.membership->id, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto firstRestore = restarted.federateRestoreComplete(
      L"exercise", restartedOwner.membership->id);
  REQUIRE(firstRestore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(firstRestore.attributeValueUpdateRegionalProvideWorkItems.empty());
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedRequester.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeValueUpdateRegionalProvideWorkItems.size() == 1U);
  auto const& restoredWork =
      restored.attributeValueUpdateRegionalProvideWorkItems.front();
  REQUIRE(restoredWork.requestId == *requestId);
  REQUIRE(restoredWork.requestingFederateId == restartedRequester.membership->id);
  REQUIRE(restoredWork.providingFederateId == restartedOwner.membership->id);
  REQUIRE(restoredWork.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(restoredWork.requestedObjectClassHandle == *soda);
  REQUIRE(restoredWork.requestedAttributeHandles == flavorOnly);
  REQUIRE(restoredWork.requestRegionsByAttribute == requestRegions);
  REQUIRE(restoredWork.userSuppliedTag == requestTag);

  auto provider = restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles,
      restoredWork.requestRegionsByAttribute);
  REQUIRE(provider.has_value());
  REQUIRE(provider->requestedAttributeHandles == flavorOnly);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      restoredWork.requestId,
      restoredWork.requestingFederateId,
      restoredWork.providingFederateId,
      restoredWork.objectInstanceHandle,
      restoredWork.requestedObjectClassHandle,
      restoredWork.requestedAttributeHandles,
      restoredWork.requestRegionsByAttribute));

  // The callback-entry boundary is one-shot even when the supplied identity
  // is wrong.  A retry with the original identity must not resurrect work
  // that was already consumed by the mismatched attempt.
  auto mismatchedRequestId = restarted.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(mismatchedRequestId.has_value());
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *mismatchedRequestId,
      restartedOwner.membership->id,
      restartedRequester.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *mismatchedRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // Region identity is retained across save/restore, but its committed bounds
  // may change before the callback begins.  A now-disjoint request is
  // suppressed and remains consumed exactly once.
  auto staleRegionRequestId = restarted.registerAttributeValueUpdateRegionalRequest(
      L"exercise",
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions,
      requestTag);
  REQUIRE(staleRegionRequestId.has_value());
  auto departedRequesterRequestId =
      restarted.registerAttributeValueUpdateRegionalRequest(
          L"exercise",
          restartedRequester.membership->id,
          restartedOwner.membership->id,
          registered.objectInstanceHandle,
          *soda,
          flavorOnly,
          requestRegions,
          requestTag);
  REQUIRE(departedRequesterRequestId.has_value());
  REQUIRE(restarted.setRangeBounds(
      L"exercise",
      restartedRequester.membership->id,
      requesterRegion.regionHandle,
      *dimension,
      umbra::detail::RegionRangeBounds{3UL, 4UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise",
      restartedRequester.membership->id,
      {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *staleRegionRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *staleRegionRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // Provider departure is a separate lifecycle fence: an accepted request
  // must be retired when the providing joined federate resigns before the
  // callback-entry boundary, and must not become a late Provide callback.
  REQUIRE(restarted.setRangeBounds(
      L"exercise",
      restartedRequester.membership->id,
      requesterRegion.regionHandle,
      *dimension,
      umbra::detail::RegionRangeBounds{1UL, 3UL}) ==
      umbra::detail::RegionServiceStatus::applied);
  REQUIRE(restarted.commitRegionModifications(
      L"exercise",
      restartedRequester.membership->id,
      {requesterRegion.regionHandle}) ==
      umbra::detail::RegionServiceStatus::applied);
  auto providerDepartureRequestId =
      restarted.registerAttributeValueUpdateRegionalRequest(
          L"exercise",
          restartedRequester.membership->id,
          restartedOwner.membership->id,
          registered.objectInstanceHandle,
          *soda,
          flavorOnly,
          requestRegions,
          requestTag);
  REQUIRE(providerDepartureRequestId.has_value());
  REQUIRE(restarted.resign(
      L"exercise",
      restartedOwner.membership->id,
      rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST).status ==
      FederationRegistryStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *providerDepartureRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *providerDepartureRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  // A pending request cannot induce a callback after its requester resigns;
  // the request is consumed at the same callback boundary.
  REQUIRE(restarted.resign(
      L"exercise",
      restartedRequester.membership->id,
      rti1516_2025::NO_ACTION).status == FederationRegistryStatus::applied);
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *departedRequesterRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));
  REQUIRE_FALSE(restarted.beginAttributeValueUpdateRegionalProvideRecipientFor(
      L"exercise",
      *departedRequesterRequestId,
      restartedRequester.membership->id,
      restartedOwner.membership->id,
      registered.objectInstanceHandle,
      *soda,
      flavorOnly,
      requestRegions));

  std::filesystem::remove_all(directory, ignored);
}
