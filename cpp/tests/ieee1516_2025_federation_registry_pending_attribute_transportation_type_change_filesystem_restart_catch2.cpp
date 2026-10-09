#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores a pending attribute transportation-type change in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][process-restart-attribute-transportation-type-change][ownership-ledger-state]"
    "[attribute-transportation-type-change][transportation-management]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const ownerId = sourceJoined.membership->id;

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", ownerId, *server, efficiencyOnly, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", ownerId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::string const valueBytes{"\x4a\x06", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{{
      *efficiency,
      rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size()),
  }};
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", ownerId, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);

  auto change = source.planAttributeTransportationTypeChange(
      L"exercise", ownerId, registered.objectInstanceHandle,
      efficiencyOnly, "HLAbestEffort");
  REQUIRE(change.status ==
      umbra::detail::AttributeTransportationTypeChangeStatus::applied);
  REQUIRE(change.requestId != 0U);
  REQUIRE(change.objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(change.attributeHandles == efficiencyOnly);
  REQUIRE(change.transportationName == "HLAbestEffort");
  REQUIRE(change.callbackRoute);

  std::wstring const saveLabel = L"attribute-transportation-type-change-process-restart";
  REQUIRE(source.requestFederationSave(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", ownerId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", ownerId).saveCompletedSuccessfully);

  auto committed = store->load(L"exercise", saveLabel);
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& savedObject = image.objects.front();
  REQUIRE(savedObject.handle == registered.objectInstanceHandle);
  REQUIRE(savedObject.attributeValuesPresent);
  REQUIRE(savedObject.attributeValues.size() == 1U);
  REQUIRE(savedObject.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(savedObject.attributeValues.front().value == valueBytes);
  REQUIRE(savedObject.pendingAttributeTransportationTypeChanges.size() == 1U);
  auto const& savedChange =
      savedObject.pendingAttributeTransportationTypeChanges.front();
  REQUIRE(savedChange.requestId == change.requestId);
  REQUIRE(savedChange.requestingFederateId == ownerId);
  REQUIRE(savedChange.attributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(savedChange.transportationName == "HLAbestEffort");
  REQUIRE(savedObject.pendingOperationCount == 1U);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == ownerId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", ownerId, saveLabel).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(L"exercise", ownerId);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.attributeTransportationTypeChangeWorkItems.size() == 1U);
  auto const& restoredChange =
      restored.attributeTransportationTypeChangeWorkItems.front();
  REQUIRE(restoredChange.requestingFederateId == ownerId);
  REQUIRE(restoredChange.objectInstanceHandle ==
      registered.objectInstanceHandle);
  REQUIRE(restoredChange.requestId == change.requestId);
  REQUIRE(restoredChange.callbackRoute);

  auto delivered = restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId);
  REQUIRE(delivered.has_value());
  REQUIRE(delivered->objectInstanceHandle == registered.objectInstanceHandle);
  REQUIRE(delivered->attributeHandles == efficiencyOnly);
  REQUIRE(delivered->transportationName == "HLAbestEffort");
  REQUIRE_FALSE(restarted.beginAttributeTransportationTypeChange(
      L"exercise", ownerId, restoredChange.requestId));

  auto query = restarted.attributeTransportationTypeQueryFor(
      L"exercise", ownerId, registered.objectInstanceHandle, *efficiency);
  REQUIRE(query.has_value());
  REQUIRE(query->transportationName == "HLAbestEffort");

  std::filesystem::remove_all(directory, ignored);
}
