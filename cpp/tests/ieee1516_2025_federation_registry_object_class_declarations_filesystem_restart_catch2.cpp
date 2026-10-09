#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores object-class attribute declarations in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][object-class-declaration-state]") {
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
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", federateId, *server, {*efficiency}, true, "High") ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.changeDefaultAttributeTransportationType(
      L"exercise", federateId, *server, {*efficiency}, "HLAreliable") ==
      umbra::detail::AttributeTransportationTypeDefaultStatus::applied);
  REQUIRE(source.changeDefaultAttributeOrderType(
      L"exercise", federateId, *server, {*efficiency}, rti1516_2025::TIMESTAMP) ==
      umbra::detail::AttributeOrderTypeDefaultStatus::applied);

  auto sourceDeclaration = source.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(sourceDeclaration.has_value());
  REQUIRE(sourceDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(sourceDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(sourceDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  auto const savedGeneration = sourceDeclaration->subscriptionGeneration;
  REQUIRE(savedGeneration != 0U);

  REQUIRE(source.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
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
      L"exercise", federateId, L"process-restart-object-class-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restarted.federateRestoreComplete(L"exercise", federateId).status ==
      umbra::detail::FederationRestoreControlStatus::applied);

  auto restoredDeclaration = restarted.objectClassAttributeDeclarationFor(
      L"exercise", federateId, *server);
  REQUIRE(restoredDeclaration.has_value());
  REQUIRE(restoredDeclaration->explicitlyPublishedAttributes ==
      std::set<std::uint64_t>{*efficiency});
  REQUIRE(restoredDeclaration->subscribedAttributes.at(*efficiency));
  REQUIRE(restoredDeclaration->subscribedUpdateRateDesignators.at(*efficiency) ==
      "High");
  REQUIRE(restoredDeclaration->subscriptionGeneration == savedGeneration);

  REQUIRE(restarted.requestFederationSave(
      L"exercise", federateId, L"process-restart-object-class-after-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveBegun(L"exercise", federateId).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(restarted.federateSaveComplete(L"exercise", federateId)
      .saveCompletedSuccessfully);
  auto committed = store->load(
      L"exercise", L"process-restart-object-class-after-restore");
  REQUIRE(committed.has_value());
  auto image = umbra::detail::FederationStateImageCodec::decode(
      committed->stateImage);
  REQUIRE(image.objectClassAttributeDeclarations.size() == 1U);
  REQUIRE(image.objectClassAttributeDeclarations.front().federateId == federateId);
  REQUIRE(image.objectClassAttributeDeclarations.front().subscriptionGeneration ==
      savedGeneration);
  REQUIRE(image.objectClassAttributeDeclarations.front().classes.size() == 1U);
  auto const& restoredClass =
      image.objectClassAttributeDeclarations.front().classes.front();
  REQUIRE(restoredClass.objectClassHandle == *server);
  REQUIRE(restoredClass.explicitlyPublishedAttributeHandles ==
      std::vector<std::uint64_t>{*efficiency});
  REQUIRE(restoredClass.subscribedAttributes.size() == 1U);
  REQUIRE(restoredClass.subscribedAttributes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedAttributes.front().active);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.size() == 1U);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.subscribedUpdateRateDesignators.front().value == "High");
  REQUIRE(restoredClass.defaultTransportationTypes.size() == 1U);
  REQUIRE(restoredClass.defaultTransportationTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultTransportationTypes.front().value == "HLAreliable");
  REQUIRE(restoredClass.defaultOrderTypes.size() == 1U);
  REQUIRE(restoredClass.defaultOrderTypes.front().attributeHandle == *efficiency);
  REQUIRE(restoredClass.defaultOrderTypes.front().orderType == 2U);

  std::filesystem::remove_all(directory, ignored);
}
