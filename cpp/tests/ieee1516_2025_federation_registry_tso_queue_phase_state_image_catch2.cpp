#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation save images snapshot queued in-transit and delivered TSO phases",
    "[unit][kernel][federation-registry][save-restore][durable-save][state-image][tso-queue-state]") {
  auto store = std::make_shared<umbra::detail::MemoryFederationSaveCommitStore>();
  EmbeddedFederationRegistry registry({}, store);
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);

  auto receiverTime = std::make_shared<umbra::detail::FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<rti1516_2025::HLAinteger64Time>());
  auto receiver = registry.joinWithTimeState(
      L"exercise",
      receiverTime,
      L"receiver",
      L"receiver",
      noOpCallbackRoute());
  auto observer = registry.join(
      L"exercise",
      L"observer",
      L"observer",
      noOpCallbackRoute());
  REQUIRE(receiver.membership);
  REQUIRE(observer.membership);

  auto enqueue = [&](std::int64_t timestamp) {
    auto const allocated = registry.allocateTsoMessageId(L"exercise");
    REQUIRE(allocated.status == umbra::detail::FederationTsoRegistryStatus::applied);
    auto const enqueued = registry.enqueueTsoMessage(
        L"exercise",
        allocated.messageId,
        receiver.membership->id,
        std::make_shared<rti1516_2025::HLAinteger64Time>(timestamp));
    REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
    REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
    return allocated.messageId;
  };

  auto const deliveredId = enqueue(5);
  auto const inTransitId = enqueue(6);
  auto const queuedId = enqueue(7);

  auto delivered = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(5),
      true);
  REQUIRE(delivered.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivered.delivery.messages.size() == 1U);
  REQUIRE(delivered.delivery.messages.front().messageId == deliveredId);
  REQUIRE(registry.completeTsoDelivery(
      L"exercise", delivered.delivery.messages.front()).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto inTransit = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.delivery.messages.size() == 1U);
  REQUIRE(inTransit.delivery.messages.front().messageId == inTransitId);

  auto requested = registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"phase-snapshot");
  REQUIRE(requested.status == umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(requested.notifications.size() == 2U);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE(registry.federateSaveBegun(
      L"exercise", observer.membership->id).status ==
      umbra::detail::FederationSaveControlStatus::applied);
  REQUIRE_FALSE(registry.federateSaveComplete(
      L"exercise", receiver.membership->id).saveCompletedSuccessfully);
  REQUIRE(registry.federateSaveComplete(
      L"exercise", observer.membership->id).saveCompletedSuccessfully);

  auto const commits = store->snapshotCommits();
  REQUIRE(commits.size() == 1U);
  auto const image = umbra::detail::FederationStateImageCodec::decode(
      commits.front().stateImage);
  REQUIRE(image.tsoQueueEntries.size() == 3U);
  auto const phaseFor = [&](std::uint64_t messageId) {
    auto const entry = std::find_if(
        image.tsoQueueEntries.begin(),
        image.tsoQueueEntries.end(),
        [messageId](umbra::detail::FederationStateImageTsoQueueEntry const& candidate) {
          return candidate.messageId == messageId;
        });
    REQUIRE(entry != image.tsoQueueEntries.end());
    REQUIRE(entry->recipientFederateId == receiver.membership->id);
    return entry->phase;
  };
  REQUIRE(phaseFor(deliveredId) == 2U);
  REQUIRE(phaseFor(inTransitId) == 1U);
  REQUIRE(phaseFor(queuedId) == 0U);

  // A process acknowledgement carries only the execution-owned message id;
  // the registry resolves the private queue sequence at this boundary.
  REQUIRE(registry.completeTsoDeliveryFor(
      L"exercise", receiver.membership->id, inTransitId).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
}
