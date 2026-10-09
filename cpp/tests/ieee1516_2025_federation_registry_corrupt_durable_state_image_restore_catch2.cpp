#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rejects a corrupt durable state image",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image][restore][failure]") {
  class CorruptStateImageStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    void commit(umbra::detail::FederationSaveCommitDescriptor const& descriptor) override {
      committed = descriptor;
    }

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const&,
        std::wstring const&) const override {
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      ++image.normalizationSeed;
      if (image.interactionDeclarations.empty() && !image.members.empty()) {
        umbra::detail::FederationStateImageInteractionDeclaration declaration;
        declaration.federateId = image.members.front().id;
        declaration.publishedInteractionClasses = {1U};
        image.interactionDeclarations.push_back(std::move(declaration));
        image.interactionDeclarationCount = image.interactionDeclarations.size();
      } else if (!image.interactionDeclarations.empty()) {
        image.interactionDeclarations.front().publishedInteractionClasses.push_back(
            image.interactionDeclarations.front().publishedInteractionClasses.empty()
                ? 1U
                : image.interactionDeclarations.front().publishedInteractionClasses.back() + 1U);
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

    umbra::detail::FederationSaveCommitDescriptor committed;
  };

  auto store = std::make_shared<CorruptStateImageStore>();
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
}
