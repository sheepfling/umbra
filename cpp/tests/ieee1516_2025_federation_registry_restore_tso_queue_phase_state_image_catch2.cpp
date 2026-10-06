#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "Federation restore rehydrates the saved TSO queue phases",
    "[unit][kernel][federation-registry][save-restore][durable-save][restore][tso-queue-state]") {
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
  auto const deliveredMessage = delivered.delivery.messages.front();
  REQUIRE(registry.completeTsoDelivery(L"exercise", deliveredMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto inTransit = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(6),
      true);
  REQUIRE(inTransit.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(inTransit.delivery.messages.size() == 1U);
  REQUIRE(inTransit.delivery.messages.front().messageId == inTransitId);
  auto const inTransitMessage = inTransit.delivery.messages.front();

  REQUIRE(registry.requestFederationSave(
      L"exercise", receiver.membership->id, L"phase-restore").status ==
      umbra::detail::FederationSaveControlStatus::applied);
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

  // Mutate the live queue after the save boundary. A successful restore must
  // replace this post-save state with the three phase records captured above.
  REQUIRE(registry.completeTsoDelivery(L"exercise", inTransitMessage).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto const postSave = enqueue(8);
  auto postSaveDelivery = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(8),
      true);
  REQUIRE(postSaveDelivery.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(postSaveDelivery.delivery.messages.size() == 2U);
  REQUIRE(std::any_of(
      postSaveDelivery.delivery.messages.begin(),
      postSaveDelivery.delivery.messages.end(),
      [queuedId](umbra::detail::TsoQueuedMessage const& message) {
        return message.messageId == queuedId;
      }));
  REQUIRE(std::any_of(
      postSaveDelivery.delivery.messages.begin(),
      postSaveDelivery.delivery.messages.end(),
      [postSave](umbra::detail::TsoQueuedMessage const& message) {
        return message.messageId == postSave;
      }));

  auto restore = registry.requestFederationRestore(
      L"exercise", receiver.membership->id, L"phase-restore");
  REQUIRE(restore.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(registry.federateRestoreComplete(
      L"exercise", receiver.membership->id).status ==
      umbra::detail::FederationRestoreControlStatus::applied);
  auto restored = registry.federateRestoreComplete(
      L"exercise", observer.membership->id);
  REQUIRE(restored.status == umbra::detail::FederationRestoreControlStatus::applied);
  REQUIRE(restored.notifications.size() == 2U);
  REQUIRE(std::all_of(
      restored.notifications.begin(),
      restored.notifications.end(),
      [](umbra::detail::FederationRestoreNotification const& notification) {
        return notification.successful;
      }));

  auto restoredInTransit = registry.completeTsoDelivery(
      L"exercise", inTransitMessage);
  REQUIRE(restoredInTransit.delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  auto restoredDelivered = registry.completeTsoDelivery(
      L"exercise", deliveredMessage);
  REQUIRE(restoredDelivered.delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::message_already_completed);

  auto restoredQueued = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(7),
      true);
  REQUIRE(restoredQueued.delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(restoredQueued.delivery.messages.size() == 1U);
  REQUIRE(restoredQueued.delivery.messages.front().messageId == queuedId);
  REQUIRE(registry.completeTsoDelivery(
      L"exercise", restoredQueued.delivery.messages.front()).delivery.status ==
      umbra::detail::FederationTsoDeliveryStatus::applied);

  auto const noPostSaveMessage = registry.beginTsoDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(8),
      true);
  REQUIRE(noPostSaveMessage.delivery.status == umbra::detail::FederationTsoDeliveryStatus::no_messages);
}
