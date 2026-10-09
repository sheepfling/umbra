#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Filesystem state image restores pending time advance and deferred lookahead in a fresh registry",
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
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t sourceFactoryCalls = 0U;
  auto sourceTimeAdvanceFactory = [&source, sourceTime, &sourceFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++sourceFactoryCalls;
    return [&source,
            sourceTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (source.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(sourceTime->grant(generation));
      }
    };
  };
  auto sourceJoined = source.joinWithTimeState(
      L"exercise",
      sourceTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(sourceTimeAdvanceFactory));
  REQUIRE(sourceJoined.status == FederationRegistryStatus::applied);
  REQUIRE(sourceJoined.membership);

  auto regulation = sourceTime->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(5));
  REQUIRE(regulation.status == umbra::detail::FederateTimeEnableStatus::applied);
  REQUIRE(sourceTime->grantTimeRegulation(regulation.generation));

  auto deferredLookahead = sourceTime->modifyLookahead(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(1));
  REQUIRE(deferredLookahead ==
      umbra::detail::FederateTimeModifyLookaheadStatus::applied);
  auto advance = sourceTime->requestAdvance(
      std::make_shared<rti1516_2025::HLAinteger64Time>(5));
  REQUIRE(advance.status == umbra::detail::FederateTimeAdvanceStatus::applied);
  REQUIRE(sourceTime->snapshot().timeAdvancePending);
  REQUIRE(sourceTime->snapshot().pendingModifiedLookahead);
  auto scheduled = source.requestTimeAdvanceGrant(
      L"exercise",
      sourceJoined.membership->id,
      advance.generation);
  REQUIRE(scheduled.status == FederationTimeGrantStatus::applied);
  REQUIRE(scheduled.dispatches.size() == 1U);
  REQUIRE(sourceFactoryCalls == 1U);

  REQUIRE(source.requestFederationSave(
      L"exercise",
      sourceJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveBegun(
      L"exercise",
      sourceJoined.membership->id)
      .status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(source.federateSaveComplete(
      L"exercise",
      sourceJoined.membership->id)
      .saveCompletedSuccessfully);
  std::move(scheduled.dispatches.front())();
  REQUIRE_FALSE(sourceTime->snapshot().timeAdvancePending);

  EmbeddedFederationRegistry restarted({}, store);
  REQUIRE(restarted.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto restartedTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>(0));
  std::size_t restartedFactoryCalls = 0U;
  auto restartedTimeAdvanceFactory = [&restarted, restartedTime, &restartedFactoryCalls](
      std::uint64_t federateId,
      std::uint64_t generation,
      std::uint64_t dispatchIdentity)
      -> umbra::detail::FederationTimeGrantDispatch {
    ++restartedFactoryCalls;
    return [&restarted,
            restartedTime,
            federateId,
            generation,
            dispatchIdentity] {
      if (restarted.beginTimeAdvanceGrant(
              L"exercise",
              federateId,
              generation,
              dispatchIdentity) ==
          FederationTimeGrantStatus::applied) {
        static_cast<void>(restartedTime->grant(generation));
      }
    };
  };
  auto restartedJoined = restarted.joinWithTimeState(
      L"exercise",
      restartedTime,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      std::move(restartedTimeAdvanceFactory));
  REQUIRE(restartedJoined.status == FederationRegistryStatus::applied);
  REQUIRE(restartedJoined.membership);
  REQUIRE(restartedJoined.membership->id == sourceJoined.membership->id);

  REQUIRE(restarted.requestFederationRestore(
      L"exercise",
      restartedJoined.membership->id,
      L"pending-application-request-checkpoint")
      .status == umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = restarted.federateRestoreComplete(
      L"exercise",
      restartedJoined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restartedFactoryCalls == 1U);
  REQUIRE(restored.timeAdvanceGrantDispatches.size() == 1U);
  std::move(restored.timeAdvanceGrantDispatches.front())();

  auto restoredTime = restartedTime->snapshot();
  REQUIRE_FALSE(restoredTime.timeAdvancePending);
  REQUIRE(restoredTime.advanceMode ==
      umbra::detail::FederateTimeAdvanceMode::none);
  REQUIRE(restoredTime.lookahead);
  auto const* restoredLookahead =
      dynamic_cast<rti1516_2025::HLAinteger64Interval const*>(
          restoredTime.lookahead.get());
  REQUIRE(restoredLookahead);
  REQUIRE(restoredLookahead->getInterval() == 1);
  REQUIRE_FALSE(restoredTime.pendingModifiedLookahead);
  auto const* restoredCurrentTime =
      dynamic_cast<rti1516_2025::HLAinteger64Time const*>(
          restoredTime.currentTime.get());
  REQUIRE(restoredCurrentTime);
  REQUIRE(restoredCurrentTime->getTime() == 5);

  std::filesystem::remove_all(directory, ignored);
}
