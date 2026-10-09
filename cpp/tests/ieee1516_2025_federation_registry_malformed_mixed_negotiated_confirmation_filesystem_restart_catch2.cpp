#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rejects malformed mixed negotiated confirmation images",
    "[unit][kernel][federation-registry][save-restore][durable-save][filesystem][restore][failure]"
    "[process-restart][process-restart-malformed-mixed-confirmation][ownership-ledger-state]"
    "[attribute-ownership-acquisition][attribute-ownership-acquisition-if-available]"
    "[negotiated-attribute-ownership-divestiture]") {
  enum class Mutation {
    missingIfAvailableCandidate,
    mismatchedIfAvailableCandidate,
    staleConfirmationFlags,
  };
  class MutatingStore final : public umbra::detail::FederationSaveCommitStore {
   public:
    MutatingStore(
        umbra::detail::FederationSaveCommitDescriptor descriptor,
        Mutation mutation)
        : committed(std::move(descriptor)), mutation(mutation) {}

    void commit(umbra::detail::FederationSaveCommitDescriptor const&) override {}

    [[nodiscard]] std::optional<umbra::detail::FederationSaveCommitDescriptor> load(
        std::wstring const& federationName,
        std::wstring const& label) const override {
      if (committed.federationName != federationName || committed.label != label) {
        return std::nullopt;
      }
      auto result = committed;
      auto image = umbra::detail::FederationStateImageCodec::decode(
          result.stateImage);
      REQUIRE(image.objects.size() == 1U);
      auto& object = image.objects.front();
      REQUIRE(object.pendingAttributeOwnershipAcquisitionRequests.size() == 1U);
      REQUIRE(object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.size() == 1U);
      REQUIRE(object.pendingNegotiatedAttributeOwnershipDivestitures.size() == 2U);
      switch (mutation) {
        case Mutation::missingIfAvailableCandidate:
          object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.clear();
          REQUIRE(object.pendingOperationCount > 0U);
          --object.pendingOperationCount;
          break;
        case Mutation::mismatchedIfAvailableCandidate:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (divestiture.acquiringFederateIsIfAvailable) {
              ++divestiture.acquisitionRequestId;
            }
          }
          break;
        case Mutation::staleConfirmationFlags:
          for (auto& divestiture :
               object.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (!divestiture.acquiringFederateIsIfAvailable) {
              divestiture.confirmationQueued = false;
              divestiture.confirmationDelivered = true;
            }
          }
          break;
      }
      result.stateImage = umbra::detail::FederationStateImageCodec::encode(image);
      return result;
    }

   private:
    umbra::detail::FederationSaveCommitDescriptor committed;
    Mutation mutation;
  };

  auto sourceStore = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry source({}, sourceStore);
  REQUIRE(source.create(L"exercise", composedRestaurantDefinition()).status ==
      FederationRegistryStatus::applied);
  auto owner = source.join(
      L"exercise", L"publisher", L"owner", noOpCallbackRoute());
  auto requester = source.join(
      L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
  REQUIRE(owner.membership);
  REQUIRE(requester.membership);

  auto const server = source.objectClassHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server");
  auto const efficiency = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Efficiency");
  auto const cheerfulness = source.attributeHandleFor(
      L"exercise", "HLAobjectRoot.Employee.Server", "Cheerfulness");
  REQUIRE(server.has_value());
  REQUIRE(efficiency.has_value());
  REQUIRE(cheerfulness.has_value());
  std::set<std::uint64_t> const mixedAttributes{*efficiency, *cheerfulness};
  std::set<std::uint64_t> const efficiencyOnly{*efficiency};
  std::set<std::uint64_t> const cheerfulnessOnly{*cheerfulness};
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", owner.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  REQUIRE(source.setObjectClassAttributeSubscription(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);
  auto registered = source.registerObjectInstance(
      L"exercise", owner.membership->id, *server);
  REQUIRE(registered.status ==
      umbra::detail::ObjectInstanceRegistrationStatus::applied);
  auto discoveries = source.planObjectInstanceDiscoveriesForInstance(
      L"exercise", registered.objectInstanceHandle);
  REQUIRE(discoveries.size() == 1U);
  REQUIRE(source.beginObjectInstanceDiscovery(
      L"exercise", requester.membership->id,
      registered.objectInstanceHandle));
  REQUIRE(source.setObjectClassAttributePublication(
      L"exercise", requester.membership->id, *server, mixedAttributes, true) ==
      umbra::detail::ObjectClassAttributeDeclarationStatus::applied);

  std::string const efficiencyValue{"\x91\x92", 2U};
  std::string const cheerfulnessValue{"\x93\x94", 2U};
  std::vector<std::pair<std::uint64_t, rti1516_2025::VariableLengthData>> values{
      {*efficiency,
       rti1516_2025::VariableLengthData(efficiencyValue.data(), efficiencyValue.size())},
      {*cheerfulness,
       rti1516_2025::VariableLengthData(cheerfulnessValue.data(), cheerfulnessValue.size())},
  };
  REQUIRE(source.recordSuccessfulUpdateAttributeValues(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      *server, {"HLAreliable"}, &values) == FederationRegistryStatus::applied);
  auto regular = source.planAttributeOwnershipAcquisition(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      efficiencyOnly, {'r', 'e', 'g', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(regular.status ==
      umbra::detail::AttributeOwnershipAcquisitionStatus::applied);
  REQUIRE(regular.workItems.size() == 1U);
  auto ifAvailable = source.planAttributeOwnershipAcquisitionIfAvailable(
      L"exercise", requester.membership->id, registered.objectInstanceHandle,
      cheerfulnessOnly, {'w', 't', 'a', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(ifAvailable.status ==
      umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied);
  REQUIRE(ifAvailable.requestId != 0U);
  auto divestiture = source.planNegotiatedAttributeOwnershipDivestiture(
      L"exercise", owner.membership->id, registered.objectInstanceHandle,
      mixedAttributes, {'d', 'i', 'v', '-', 'm', 'a', 'l', 'f', 'o', 'r', 'm'});
  REQUIRE(divestiture.status ==
      umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied);
  REQUIRE(divestiture.workItems.size() == 2U);

  std::wstring const saveLabel = L"ownership-negotiated-malformed-process-restart";
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
  auto commits = sourceStore->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  auto const baseCommit = commits.front();

  for (auto const mutation : {
           Mutation::missingIfAvailableCandidate,
           Mutation::mismatchedIfAvailableCandidate,
           Mutation::staleConfirmationFlags}) {
    auto store = std::make_shared<MutatingStore>(baseCommit, mutation);
    EmbeddedFederationRegistry restarted({}, store);
    REQUIRE(restarted.create(L"exercise", composedRestaurantDefinition()).status ==
        FederationRegistryStatus::applied);
    auto restartedOwner = restarted.join(
        L"exercise", L"publisher", L"owner", noOpCallbackRoute());
    auto restartedRequester = restarted.join(
        L"exercise", L"subscriber", L"requester", noOpCallbackRoute());
    REQUIRE(restartedOwner.membership);
    REQUIRE(restartedRequester.membership);
    auto restore = restarted.requestFederationRestore(
        L"exercise", restartedRequester.membership->id, saveLabel);
    REQUIRE(restore.status ==
        umbra::detail::FederationRestoreControlStatus::snapshot_not_found);
    REQUIRE(restore.notifications.size() == 1U);
    REQUIRE(restore.notifications.front().kind ==
        umbra::detail::FederationRestoreNotificationKind::request_failed);
    REQUIRE(restore.notifications.front().receivingFederateId ==
        restartedRequester.membership->id);
  }
}
