#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rebinds pending time-role callbacks and fences stale work",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore]"
    "[time-management][time-role][callbacks][pending-application-request-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto timeState = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  std::size_t dispatchCount = 0U;
  std::size_t grantCount = 0U;
  auto roleFactory = [timeState, &dispatchCount, &grantCount](
      std::uint64_t federateId,
      std::uint64_t generation,
      umbra::detail::FederationTimeRoleEnableKind kind)
      -> umbra::detail::FederationTimeGrantDispatch {
    REQUIRE(federateId != 0U);
    REQUIRE(generation != 0U);
    auto const callbackEpoch = timeState->callbackEpoch();
    return [timeState, generation, kind, callbackEpoch, &dispatchCount, &grantCount] {
      ++dispatchCount;
      std::shared_ptr<rti1516_2025::LogicalTime const> enabledTime;
      if (kind == umbra::detail::FederationTimeRoleEnableKind::regulation) {
        enabledTime = timeState->grantTimeRegulationIfCurrent(
            generation,
            callbackEpoch);
      } else {
        enabledTime = timeState->grantTimeConstrainedIfCurrent(
            generation,
            callbackEpoch);
      }
      if (enabledTime) {
        ++grantCount;
      }
    };
  };

  auto joined = registry.joinWithTimeState(
      L"exercise",
      timeState,
      L"trainer",
      L"alice",
      noOpCallbackRoute(),
      {},
      std::move(roleFactory));
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  auto request = timeState->requestTimeRegulation(
      std::make_shared<rti1516_2025::HLAinteger64Interval>(2));
  REQUIRE(request.status == umbra::detail::FederateTimeEnableStatus::applied);
  auto const staleEpoch = timeState->callbackEpoch();

  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"pending-role-checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  // Mutate the live state so restore has to reapply the still-pending role
  // request rather than merely retaining the current enabled mode.
  REQUIRE(timeState->grantTimeRegulation(request.generation));
  REQUIRE(timeState->snapshot().timeRegulating);

  REQUIRE(registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"pending-role-checkpoint").status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = registry.federateRestoreComplete(
      L"exercise", joined.membership->id);
  REQUIRE(restored.status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.timeRoleEnableDispatches.size() == 1U);
  REQUIRE_FALSE(timeState->snapshot().timeRegulating);
  REQUIRE(timeState->snapshot().timeRegulationPending);

  // The closure that existed before restore carries the old epoch and cannot
  // consume the restored generation, even though the generation value itself
  // is intentionally reused by the saved application-request ledger.
  REQUIRE_FALSE(timeState->grantTimeRegulationIfCurrent(
      request.generation,
      staleEpoch));
  REQUIRE(dispatchCount == 0U);
  REQUIRE(grantCount == 0U);

  auto dispatch = std::move(restored.timeRoleEnableDispatches.front());
  dispatch();
  REQUIRE(dispatchCount == 1U);
  REQUIRE(grantCount == 1U);
  REQUIRE(timeState->snapshot().timeRegulating);
  REQUIRE_FALSE(timeState->snapshot().timeRegulationPending);
}
