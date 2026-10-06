#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation save completion commits before saved notifications",
    "[unit][kernel][federation-registry][save-restore][durable-save]"
    "[tso-retraction-ledger-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto joined = registry.join(
      L"exercise", L"trainer", L"alice", noOpCallbackRoute());
  REQUIRE(joined.status == FederationRegistryStatus::applied);
  REQUIRE(joined.membership);

  umbra::detail::TsoInteractionMessage pendingInteraction;
  std::vector<rti1516_2025::Octet> pendingParameterBytes{0x01U, 0x02U};
  std::vector<rti1516_2025::Octet> pendingTagBytes{0x03U};
  pendingInteraction.producingFederateId = joined.membership->id;
  pendingInteraction.sentInteractionClassHandle = 1U;
  pendingInteraction.sentParameterHandles = {2U};
  pendingInteraction.parameters = {{
      2U,
      rti1516_2025::VariableLengthData(
          pendingParameterBytes.data(),
          pendingParameterBytes.size()),
  }};
  pendingInteraction.userSuppliedTag = rti1516_2025::VariableLengthData(
      pendingTagBytes.data(),
      pendingTagBytes.size());
  pendingInteraction.transportationName = "HLAreliable";
  pendingInteraction.timestamp =
      std::make_shared<rti1516_2025::HLAinteger64Time>(7);
  auto const enqueuedInteraction = registry.enqueueTsoInteraction(
      L"exercise",
      std::move(pendingInteraction),
      {joined.membership->id},
      {joined.membership->id});
  REQUIRE(enqueuedInteraction.status ==
      umbra::detail::FederationTsoRegistryStatus::applied);

  auto requested = registry.requestFederationSave(
      L"exercise", joined.membership->id, L"checkpoint");
  REQUIRE(requested.status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(requested.notifications.size() == 1U);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", joined.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  auto completed = registry.federateSaveComplete(
      L"exercise", joined.membership->id);
  REQUIRE(completed.saveCompletedSuccessfully);
  REQUIRE(completed.notifications.size() == 1U);
  REQUIRE(completed.notifications.front().successful);

  auto commits = store->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  REQUIRE(commits.front().federationName == L"exercise");
  REQUIRE(commits.front().label == L"checkpoint");
  REQUIRE(commits.front().logicalTimeImplementationName == L"HLAinteger64Time");
  REQUIRE(commits.front().memberFederateIds == std::vector<std::uint64_t>{joined.membership->id});
  REQUIRE_FALSE(commits.front().timed);
  REQUIRE(commits.front().stateImage.starts_with("umbra-federation-state/v1\n"));
  auto const image = umbra::detail::FederationStateImageCodec::decode(
      commits.front().stateImage);
  REQUIRE(image.federationName == L"exercise");
  REQUIRE(image.members.size() == 1U);
  REQUIRE(image.members.front().id == joined.membership->id);
  REQUIRE(image.tsoInteractionMessages.size() == 1U);
  REQUIRE(image.tsoInteractionMessages.front().messageId ==
      enqueuedInteraction.messageId);
  REQUIRE(image.tsoInteractionMessages.front().parameters.front().value ==
      std::string{"\x01\x02", 2U});
  REQUIRE(image.tsoRequestRetractionRecords.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().messageId ==
      enqueuedInteraction.messageId);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.size() == 1U);
  REQUIRE(image.tsoRequestRetractionRecords.front().recipientStates.front().state == 0U);
  REQUIRE(image.tsoQueueEntries.size() == 1U);
  REQUIRE(image.tsoQueueEntries.front().messageId == enqueuedInteraction.messageId);
  REQUIRE(image.tsoQueueEntries.front().recipientFederateId ==
      joined.membership->id);
  REQUIRE(image.tsoQueueEntries.front().phase == 0U);
}
