#include "ieee1516_2025_federation_registry_test_support.hpp"

TEST_CASE(
    "The registry retains captured regional attribute TSO snapshots instead of rereading live regions",
    "[unit][kernel][federation-registry][tso][ddm][timestamped-regional-attribute-update]") {
  EmbeddedFederationRegistry registry;
  REQUIRE(registry.create(L"exercise", validDefinition()).status ==
      FederationRegistryStatus::applied);
  auto producer = registry.join(L"exercise", L"producer", L"producer");
  auto receiver = registry.join(L"exercise", L"receiver", L"receiver");
  REQUIRE(producer.membership.has_value());
  REQUIRE(receiver.membership.has_value());

  // The opaque source handle is deliberately absent from the live registry.
  // A timestamped service has already accepted its committed specification,
  // so queue admission must retain the supplied invocation snapshot rather
  // than attempting a second lookup against mutable federation state.
  umbra::detail::TsoAttributeUpdateMessage message;
  message.producingFederateId = producer.membership->id;
  message.objectInstanceHandle = 91;
  message.attributes.emplace_back(1, rti1516_2025::VariableLengthData{});
  message.timestamp = std::make_shared<rti1516_2025::HLAinteger64Time>(7);

  umbra::detail::TsoAttributeUpdatePassel passel;
  passel.sentAttributeHandles = {1};
  passel.sentRegionHandles = {42};
  passel.sentRegionSnapshots.emplace(
      42,
      umbra::detail::RegionSpecificationSnapshot{
          {17},
          {{17, umbra::detail::RegionRangeBounds{0, 5}}},
          true});
  message.sentRegionSnapshots = passel.sentRegionSnapshots;
  message.passelsByRecipient.emplace(
      receiver.membership->id,
      std::vector<umbra::detail::TsoAttributeUpdatePassel>{passel});

  auto const enqueued = registry.enqueueTsoAttributeUpdate(
      L"exercise",
      std::move(message),
      {receiver.membership->id},
      {receiver.membership->id});
  REQUIRE(enqueued.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(enqueued.queueStatus == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(enqueued.messageId != 0);

  auto const delivery = registry.beginTsoPayloadDelivery(
      L"exercise",
      receiver.membership->id,
      rti1516_2025::HLAinteger64Time(7),
      true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoRegistryStatus::applied);
  REQUIRE(delivery.deliveryStatus == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.deliveries.size() == 1);
  auto const* attributeDelivery =
      std::get_if<umbra::detail::TsoAttributeUpdateDelivery>(&delivery.deliveries.front());
  REQUIRE(attributeDelivery != nullptr);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.size() == 1);
  REQUIRE(attributeDelivery->message.sentRegionSnapshots.contains(42));
  auto const& snapshot = attributeDelivery->message.sentRegionSnapshots.at(42);
  REQUIRE(snapshot.dimensionHandles == std::set<std::uint64_t>{17});
  REQUIRE(snapshot.committedRangeBounds.at(17).lowerBound == 0);
  REQUIRE(snapshot.committedRangeBounds.at(17).upperBound == 5);
}
