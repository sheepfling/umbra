#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rejects an in-memory snapshot when its durable envelope is gone",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][failure]") {
  class CommitThenForgetStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      lastCommit = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      return std::nullopt;
    }

    umbra::detail::FederationSaveCommitDescriptor lastCommit;
  };

  auto store = std::make_shared<CommitThenForgetStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);
  REQUIRE(registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint").status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", joined.membership->id).saveCompletedSuccessfully);

  auto restore = registry.requestFederationRestore(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
  REQUIRE(restore.notifications.size() == 1U);
  REQUIRE(restore.notifications.front().kind ==
      umbra::detail::FederationRestoreNotificationKind::request_failed);
}
