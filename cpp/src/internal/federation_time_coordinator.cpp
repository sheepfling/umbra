#include "internal/federation_time_coordinator.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {

FederationTimeCoordinator::FederationTimeCoordinator(std::wstring implementationName)
    : implementationName_(std::move(implementationName)),
      tsoQueue_(implementationName_) {}

std::wstring const& FederationTimeCoordinator::implementationName() const noexcept {
  return implementationName_;
}

bool FederationTimeCoordinator::configureImplementationName(std::wstring implementationName) {
  if (!implementationName_.empty() && implementationName_ != implementationName) {
    return false;
  }
  if (!tsoQueue_.configureImplementationName(implementationName)) {
    return false;
  }
  implementationName_ = std::move(implementationName);
  return true;
}

FederationTimeCoordinatorResult FederationTimeCoordinator::registerFederate(
    std::uint64_t federateId,
    std::shared_ptr<FederateTimeState> timeState) {
  if (federateId == 0 || !timeState) {
    return {FederationTimeCoordinatorStatus::invalid_time_state};
  }

  if (timeState->implementationName().empty() ||
      (!implementationName_.empty() &&
       timeState->implementationName() != implementationName_)) {
    return {FederationTimeCoordinatorStatus::invalid_time_state};
  }
  if (implementationName_.empty() &&
      !configureImplementationName(timeState->implementationName())) {
    return {FederationTimeCoordinatorStatus::invalid_time_state};
  }

  auto const [position, inserted] = federates_.emplace(federateId, std::move(timeState));
  static_cast<void>(position);
  return {
      inserted ? FederationTimeCoordinatorStatus::applied
               : FederationTimeCoordinatorStatus::federate_already_registered,
  };
}

FederationTimeCoordinatorResult FederationTimeCoordinator::unregisterFederate(
    std::uint64_t federateId) noexcept {
  if (federates_.erase(federateId) != 1) {
    return {FederationTimeCoordinatorStatus::federate_not_registered};
  }
  static_cast<void>(discardTsoRecipient(federateId));
  return {FederationTimeCoordinatorStatus::applied};
}

std::vector<RegisteredFederateTimeSnapshot> FederationTimeCoordinator::snapshot() const {
  std::vector<RegisteredFederateTimeSnapshot> result;
  result.reserve(federates_.size());
  for (auto const& [federateId, timeState] : federates_) {
    if (!timeState) {
      continue;
    }
    result.push_back({federateId, timeState->snapshot()});
  }
  return result;
}

std::uint64_t FederationTimeCoordinator::allocateTsoMessageId() noexcept {
  return tsoQueue_.allocateMessageId();
}

TsoMessageEnqueueResult FederationTimeCoordinator::enqueueTsoMessage(
    std::uint64_t messageId,
    std::uint64_t recipientFederateId,
    std::shared_ptr<rti1516_2025::LogicalTime const> timestamp) {
  return tsoQueue_.enqueue(messageId, recipientFederateId, std::move(timestamp));
}

TsoMessageRetractionResult FederationTimeCoordinator::retractTsoMessage(
    std::uint64_t messageId) {
  return tsoQueue_.retract(messageId);
}

FederationTsoDeliveryResult FederationTimeCoordinator::beginTsoDelivery(
    std::uint64_t recipientFederateId,
    rti1516_2025::LogicalTime const& boundary,
    bool inclusive) {
  if (recipientFederateId == 0) {
    return {FederationTsoDeliveryStatus::invalid_recipient, {}};
  }
  if (!implementationName_.empty() && boundary.implementationName() != implementationName_) {
    return {FederationTsoDeliveryStatus::invalid_boundary, {}};
  }

  auto messages = tsoQueue_.popEligible(recipientFederateId, boundary, inclusive);
  if (messages.empty()) {
    return {FederationTsoDeliveryStatus::no_messages, {}};
  }
  auto& inTransit = inTransitTsoMessages_[recipientFederateId];
  inTransit.insert(inTransit.end(), messages.begin(), messages.end());
  return {FederationTsoDeliveryStatus::applied, std::move(messages)};
}

FederationTsoDeliveryStatus FederationTimeCoordinator::completeTsoDelivery(
    TsoQueuedMessage const& message) {
  if (message.recipientFederateId == 0 || message.messageId == 0 || message.sequence == 0) {
    return FederationTsoDeliveryStatus::invalid_recipient;
  }
  auto const inTransit = inTransitTsoMessages_.find(message.recipientFederateId);
  if (inTransit == inTransitTsoMessages_.end()) {
    auto const delivered = deliveredTsoMessagesSinceLastAdvance_.find(message.recipientFederateId);
    if (delivered != deliveredTsoMessagesSinceLastAdvance_.end() &&
        std::any_of(
            delivered->second.begin(),
            delivered->second.end(),
            [&message](TsoQueuedMessage const& candidate) {
              return candidate.messageId == message.messageId &&
                  candidate.sequence == message.sequence;
            })) {
      return FederationTsoDeliveryStatus::message_already_completed;
    }
    return FederationTsoDeliveryStatus::message_not_in_transit;
  }

  auto const position = std::find_if(
      inTransit->second.begin(),
      inTransit->second.end(),
      [&message](TsoQueuedMessage const& candidate) {
        return candidate.messageId == message.messageId &&
            candidate.sequence == message.sequence;
      });
  if (position == inTransit->second.end()) {
    return FederationTsoDeliveryStatus::message_not_in_transit;
  }

  auto& delivered = deliveredTsoMessagesSinceLastAdvance_[message.recipientFederateId];
  delivered.push_back(*position);
  inTransit->second.erase(position);
  if (inTransit->second.empty()) {
    inTransitTsoMessages_.erase(inTransit);
  }
  return FederationTsoDeliveryStatus::applied;
}

std::size_t FederationTimeCoordinator::clearDeliveredTsoMessages(
    std::uint64_t recipientFederateId) noexcept {
  auto const delivered = deliveredTsoMessagesSinceLastAdvance_.find(recipientFederateId);
  if (delivered == deliveredTsoMessagesSinceLastAdvance_.end()) {
    return 0;
  }
  auto const count = delivered->second.size();
  deliveredTsoMessagesSinceLastAdvance_.erase(delivered);
  return count;
}

FederationTsoSnapshot FederationTimeCoordinator::tsoSnapshotFor(
    std::uint64_t recipientFederateId) const {
  FederationTsoSnapshot result;
  if (recipientFederateId == 0) {
    return result;
  }
  for (auto const& message : tsoQueue_.pendingMessages()) {
    if (message.recipientFederateId == recipientFederateId) {
      result.queued.push_back(message);
    }
  }
  if (auto const inTransit = inTransitTsoMessages_.find(recipientFederateId);
      inTransit != inTransitTsoMessages_.end()) {
    result.inTransit = inTransit->second;
  }
  if (auto const delivered = deliveredTsoMessagesSinceLastAdvance_.find(recipientFederateId);
      delivered != deliveredTsoMessagesSinceLastAdvance_.end()) {
    result.deliveredSinceLastAdvance = delivered->second;
  }
  return result;
}

std::size_t FederationTimeCoordinator::discardTsoRecipient(
    std::uint64_t recipientFederateId) noexcept {
  if (recipientFederateId == 0) {
    return 0;
  }
  auto count = tsoQueue_.discardRecipient(recipientFederateId);
  if (auto const inTransit = inTransitTsoMessages_.find(recipientFederateId);
      inTransit != inTransitTsoMessages_.end()) {
    count += inTransit->second.size();
    inTransitTsoMessages_.erase(inTransit);
  }
  if (auto const delivered = deliveredTsoMessagesSinceLastAdvance_.find(recipientFederateId);
      delivered != deliveredTsoMessagesSinceLastAdvance_.end()) {
    count += delivered->second.size();
    deliveredTsoMessagesSinceLastAdvance_.erase(delivered);
  }
  return count;
}

bool FederationTimeCoordinator::empty() const noexcept {
  return federates_.empty();
}

std::size_t FederationTimeCoordinator::size() const noexcept {
  return federates_.size();
}

}  // namespace umbra::detail
