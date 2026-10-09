#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores latest object application values in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][application-value-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceJoined = source.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);
  auto const federateId = sourceJoined.membership->id;
  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", federateId, *server, {*efficiency}, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  auto registered = source.registerObjectInstance(
      L"exercise", federateId, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  REQUIRE(registered.objectInstanceHandle != 0U);

  std::vector<rti1516_2025::Octet> valueBytes{
      static_cast<rti1516_2025::Octet>(0x01U),
      static_cast<rti1516_2025::Octet>(0xfeU),
      static_cast<rti1516_2025::Octet>(0x7fU)};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(valueBytes.data(), valueBytes.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise",
      federateId,
      registered.objectInstanceHandle,
      *server,
      {"HLAreliable"},
      &values) == FederationRegistryStatus::applied);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedJoined = restarted.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == federateId);
  REQUIRE(restarted.requestFederationRestore(
      L"exercise", federateId, L"process-restart-application-value-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  // A fresh registry has no process-local object snapshot. Saving immediately
  // after restore therefore proves that object identity and the latest value
  // were materialized from the durable image rather than retained in memory.
  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-application-value-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);

  auto committed = store->load(
      L"exercise", L"process-restart-application-value-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objects.size() == 1U);
  auto const& object = image.objects.front();
  REQUIRE(object.handle == registered.objectInstanceHandle);
  REQUIRE(object.name == registered.objectInstanceName);
  REQUIRE(object.registeredObjectClassHandle == *server);
  REQUIRE(object.attributeValuesPresent);
  REQUIRE(object.attributeValues.size() == 1U);
  REQUIRE(object.attributeValues.front().attributeHandle == *efficiency);
  REQUIRE(object.attributeValues.front().value ==
      std::string{reinterpret_cast<char const*>(valueBytes.data()), valueBytes.size()});

  std::filesystem::remove_all(directory, ignored);
}
