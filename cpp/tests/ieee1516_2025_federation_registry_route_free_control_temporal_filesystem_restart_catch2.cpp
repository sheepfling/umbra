#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores route-free control and temporal state in a fresh registry",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore]"
    "[process-restart][pending-application-request-state]") {
  auto const directory = temporarySaveCommitDirectory();
  std::error_code ignored;
  std::filesystem::remove_all(directory, ignored);
  auto store = std::make_shared<umbra::detail::FilesystemFederationSaveCommitStore>(
      directory);

  EmbeddedFederationRegistry source({}, store);
  REQUIRE(source.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto sourceTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute());
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto sourceRoleRequest = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(sourceRoleRequest.status ==
      umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->snapshot().timeRegulationPending);

  REQUIRE(source.requestFederationSave(
      L"exercise", sourceJoined.membership->id, L"process-restart-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise", sourceJoined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise", sourceJoined.membership->id).saveCompletedSuccessfully);

  // A new registry has no process-local Federation snapshot. It joins the
  // same member identity, loads only the durable route-free image, and keeps
  // its new live callback route/factory.
  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  std::size_t roleDispatchCount = 0U;
  std::size_t roleGrantCount = 0U;
  auto roleFactory = [restartedTime, &roleDispatchCount, &roleGrantCount](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    REQUIRE(federateId != 0U);
    REQUIRE(generation != 0U);
    auto const callbackEpoch = restartedTime->callbackEpoch();
    return [restartedTime,
            generation,
            kind,
            callbackEpoch,
            &roleDispatchCount,
            &roleGrantCount] {
      ++roleDispatchCount;
      auto enabledTime = kind == umbra::detail::FederationTimeRoleEnableKind::regulation
          ? restartedTime->grantTimeRegulationIfCurrent(generation, callbackEpoch)
          : restartedTime->grantTimeConstrainedIfCurrent(generation, callbackEpoch);
      if (enabledTime) {
        ++roleGrantCount;
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      {},
      std::move(roleFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  auto restore = restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"process-restart-checkpoint");
  REQUIRE(restore.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise", restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.timeRoleEnableDispatches.size() == 1U);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulating);
  REQUIRE(restartedTime->snapshot().timeRegulationPending);
  REQUIRE(restartedTime->snapshot().nextGeneration ==
      sourceTime->snapshot().nextGeneration);

  auto dispatch = std::move(restored.timeRoleEnableDispatches.front());
  dispatch();
  REQUIRE(roleDispatchCount == 1U);
  REQUIRE(roleGrantCount == 1U);
  REQUIRE(restartedTime->snapshot().timeRegulating);
  REQUIRE_FALSE(restartedTime->snapshot().timeRegulationPending);

  std::filesystem::remove_all(directory, ignored);
}
