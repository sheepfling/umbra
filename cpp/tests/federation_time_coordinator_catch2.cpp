#include <catch2/catch_test_macros.hpp>

#include "internal/time/federation_time_coordinator.hpp"

#include <memory>

#include <RTI/time/HLAinteger64Interval.h>
#include <RTI/time/HLAinteger64Time.h>

namespace {

using umbra::detail::FederateTimeEnableStatus;
using umbra::detail::FederateTimeState;
using umbra::detail::FederationTimeCoordinator;
using umbra::detail::FederationTimeCoordinatorStatus;
using rti1516_2025::HLAinteger64Interval;
using rti1516_2025::HLAinteger64Time;

std::shared_ptr<FederateTimeState> integerTimeState() {
  return std::make_shared<FederateTimeState>(
      L"HLAinteger64Time",
      std::make_shared<HLAinteger64Time>());
}

HLAinteger64Time const& asIntegerTime(
    std::shared_ptr<rti1516_2025::LogicalTime const> const& time) {
  auto const* value = dynamic_cast<HLAinteger64Time const*>(time.get());
  REQUIRE(value != nullptr);
  return *value;
}

}  // namespace

TEST_CASE(
    "Federation time coordinator retains a coherent federation-owned time input set",
    "[unit][kernel][time-management][federation-time]") {
  FederationTimeCoordinator coordinator;
  auto regulating = integerTimeState();
  auto advancing = integerTimeState();

  REQUIRE(
      coordinator.registerFederate(0, regulating).status ==
      FederationTimeCoordinatorStatus::invalid_time_state);
  REQUIRE(
      coordinator.registerFederate(1, nullptr).status ==
      FederationTimeCoordinatorStatus::invalid_time_state);
  REQUIRE(
      coordinator.registerFederate(1, regulating).status ==
      FederationTimeCoordinatorStatus::applied);
  REQUIRE(
      coordinator.registerFederate(2, advancing).status ==
      FederationTimeCoordinatorStatus::applied);
  REQUIRE(
      coordinator.registerFederate(1, integerTimeState()).status ==
      FederationTimeCoordinatorStatus::federate_already_registered);

  auto regulation = regulating->requestTimeRegulation(std::make_shared<HLAinteger64Interval>(2));
  REQUIRE(regulation.status == FederateTimeEnableStatus::applied);
  auto advance = advancing->requestAdvance(std::make_shared<HLAinteger64Time>(7));
  REQUIRE(advance.generation != 0);

  regulating.reset();
  advancing.reset();
  auto snapshot = coordinator.snapshot();
  REQUIRE(snapshot.size() == 2);
  REQUIRE(snapshot[0].federateId == 1);
  REQUIRE(snapshot[0].time.active);
  REQUIRE(snapshot[0].time.timeRegulationPending);
  REQUIRE_FALSE(snapshot[0].time.timeRegulating);
  REQUIRE(snapshot[0].time.requestedLookahead);
  REQUIRE(asIntegerTime(snapshot[0].time.currentTime).isInitial());
  REQUIRE(snapshot[1].federateId == 2);
  REQUIRE(snapshot[1].time.timeAdvancePending);
  REQUIRE(snapshot[1].time.requestedTime);
  REQUIRE(asIntegerTime(snapshot[1].time.requestedTime).getTime() == 7);

  REQUIRE(
      coordinator.unregisterFederate(1).status ==
      FederationTimeCoordinatorStatus::applied);
  REQUIRE(
      coordinator.unregisterFederate(1).status ==
      FederationTimeCoordinatorStatus::federate_not_registered);
  REQUIRE(coordinator.size() == 1);
  REQUIRE_FALSE(coordinator.empty());
  REQUIRE(
      coordinator.unregisterFederate(2).status ==
      FederationTimeCoordinatorStatus::applied);
  REQUIRE(coordinator.empty());
}

TEST_CASE(
    "Federation time coordinator exposes queued, in-transit, and delivered TSO state",
    "[unit][kernel][time-management][federation-time][tso][lits]") {
  FederationTimeCoordinator coordinator;
  REQUIRE(
      coordinator.registerFederate(1, integerTimeState()).status ==
      FederationTimeCoordinatorStatus::applied);
  REQUIRE(
      coordinator.registerFederate(2, integerTimeState()).status ==
      FederationTimeCoordinatorStatus::applied);

  auto const messageId = coordinator.allocateTsoMessageId();
  REQUIRE(messageId != 0);
  auto const timestamp = std::make_shared<HLAinteger64Time>(5);
  REQUIRE(
      coordinator.enqueueTsoMessage(messageId, 1, timestamp).status ==
      umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(
      coordinator.enqueueTsoMessage(messageId, 2, timestamp).status ==
      umbra::detail::TsoMessageQueueStatus::applied);

  auto recipientSnapshot = coordinator.tsoSnapshotFor(1);
  REQUIRE(recipientSnapshot.queued.size() == 1);
  REQUIRE(asIntegerTime(recipientSnapshot.queued.front().timestamp).getTime() == 5);

  auto delivery = coordinator.beginTsoDelivery(1, HLAinteger64Time(5), true);
  REQUIRE(delivery.status == umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(delivery.messages.size() == 1);
  REQUIRE(asIntegerTime(delivery.messages.front().timestamp).getTime() == 5);
  recipientSnapshot = coordinator.tsoSnapshotFor(1);
  REQUIRE(recipientSnapshot.queued.empty());
  REQUIRE(recipientSnapshot.inTransit.size() == 1);
  REQUIRE(recipientSnapshot.deliveredSinceLastAdvance.empty());

  auto const retraction = coordinator.retractTsoMessage(messageId);
  REQUIRE(
      retraction.status == umbra::detail::TsoMessageQueueStatus::message_already_delivered);
  auto otherSnapshot = coordinator.tsoSnapshotFor(2);
  REQUIRE(otherSnapshot.queued.size() == 1);

  REQUIRE(
      coordinator.completeTsoDelivery(delivery.messages.front()) ==
      umbra::detail::FederationTsoDeliveryStatus::applied);
  REQUIRE(
      coordinator.completeTsoDelivery(delivery.messages.front()) ==
      umbra::detail::FederationTsoDeliveryStatus::message_already_completed);
  recipientSnapshot = coordinator.tsoSnapshotFor(1);
  REQUIRE(recipientSnapshot.inTransit.empty());
  REQUIRE(recipientSnapshot.deliveredSinceLastAdvance.size() == 1);

  REQUIRE(coordinator.clearDeliveredTsoMessages(1) == 1);
  REQUIRE(coordinator.tsoSnapshotFor(1).deliveredSinceLastAdvance.empty());
}
