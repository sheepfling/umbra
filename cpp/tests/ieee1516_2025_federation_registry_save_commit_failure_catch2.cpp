#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation save completion reports not-saved when durable commit fails",
    "[unit][kernel][federation-registry][save-restore][durable-save][failure]") {
  class FailingStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const&) override {
      throw std::runtime_error("injected save commit failure");
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      return std::nullopt;
    }
  };

  auto store = std::make_shared<FailingStore>();
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

  auto completed = registry.federateSaveComplete(
      L"exercise", joined.membership->id);
  REQUIRE_FALSE(completed.saveCompletedSuccessfully);
  REQUIRE(completed.notifications.size() == 1U);
  REQUIRE_FALSE(completed.notifications.front().successful);
  REQUIRE(completed.notifications.front().failureReason ==
      rti1516_2025::RTI_UNABLE_TO_SAVE);
}
