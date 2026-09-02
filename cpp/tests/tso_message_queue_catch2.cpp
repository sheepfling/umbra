#include <catch2/catch_test_macros.hpp>

#include "internal/time/tso_message_queue.hpp"

#include <RTI/time/HLAfloat64Time.h>
#include <RTI/time/HLAinteger64Time.h>

#include <cstdint>
#include <memory>

namespace {

std::shared_ptr<rti1516_2025::LogicalTime const> integerTime(std::int64_t value) {
  return std::make_shared<rti1516_2025::HLAinteger64Time const>(value);
}

}  // namespace

TEST_CASE(
    "Private 2025 TSO queue cannot bind a logical-time implementation after admission",
    "[tso][time][unit][time-management][logical-time]") {
  umbra::detail::TsoMessageQueue queue;
  auto const messageId = queue.allocateMessageId();

  // An initially unconfigured private queue may accept the first immutable
  // time value, but that admission must freeze the configuration boundary;
  // it must not later be rebound to a different implementation.
  REQUIRE(queue.enqueue(messageId, 2, integerTime(4)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE_FALSE(queue.configureImplementationName(L"HLAfloat64Time"));
  REQUIRE(queue.implementationName().empty());
  REQUIRE_FALSE(queue.configureImplementationName(L"HLAinteger64Time"));

  REQUIRE(queue.popEligible(2, *integerTime(4), true).size() == 1U);
  REQUIRE(queue.configureImplementationName(L"HLAinteger64Time"));
  REQUIRE(queue.implementationName() == L"HLAinteger64Time");
}

TEST_CASE(
    "Private 2025 TSO queue preserves the admitted implementation after draining",
    "[tso][time][unit][time-management][logical-time]") {
  umbra::detail::TsoMessageQueue queue;
  auto const messageId = queue.allocateMessageId();

  REQUIRE(queue.enqueue(messageId, 2, integerTime(6)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.popEligible(2, *integerTime(6), true).size() == 1U);

  // The pending vector is empty now, but the queue has already admitted an
  // HLAinteger64Time payload.  Draining it must not make a different time
  // implementation legal for later temporal state.
  REQUIRE_FALSE(queue.configureImplementationName(L"HLAfloat64Time"));
  REQUIRE(queue.configureImplementationName(L"HLAinteger64Time"));
  REQUIRE(queue.implementationName() == L"HLAinteger64Time");
}

TEST_CASE(
    "Private 2025 TSO queue orders equal timestamps and isolates recipients",
    "[tso][time][unit][time-management]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const first = queue.allocateMessageId();
  auto const second = queue.allocateMessageId();
  auto const otherRecipient = queue.allocateMessageId();

  REQUIRE(first != 0);
  REQUIRE(second == first + 1);
  REQUIRE(otherRecipient == second + 1);
  REQUIRE(queue.enqueue(first, 2, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(second, 2, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(otherRecipient, 3, integerTime(1)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);

  auto earliest = queue.earliestTimestampFor(2);
  REQUIRE(earliest.has_value());
  REQUIRE(earliest.value()->toString() == L"5");
  REQUIRE(queue.pendingCount() == 3);
  REQUIRE(queue.pendingCountFor(2) == 2);
  REQUIRE(queue.pendingCountFor(3) == 1);

  auto const exclusive = queue.popEligible(2, *integerTime(5), false);
  REQUIRE(exclusive.empty());
  REQUIRE(queue.pendingCountFor(2) == 2);

  auto const equalTimestampBatch = queue.popEligible(2, *integerTime(5), true);
  REQUIRE(equalTimestampBatch.size() == 2);
  REQUIRE(equalTimestampBatch[0].messageId == first);
  REQUIRE(equalTimestampBatch[1].messageId == second);
  REQUIRE(equalTimestampBatch[0].sequence < equalTimestampBatch[1].sequence);
  REQUIRE(queue.pendingCountFor(2) == 0);
  REQUIRE(queue.pendingCountFor(3) == 1);

  auto const otherBatch = queue.popEligible(3, *integerTime(1), true);
  REQUIRE(otherBatch.size() == 1);
  REQUIRE(otherBatch.front().messageId == otherRecipient);
  REQUIRE(queue.pendingCount() == 0);
}

TEST_CASE(
    "Private 2025 TSO queue retracts pending fanout before delivery",
    "[tso][time][retraction][unit][time-management]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const messageId = queue.allocateMessageId();

  REQUIRE(queue.enqueue(messageId, 2, integerTime(7)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(messageId, 3, integerTime(7)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  auto const retracted = queue.retract(messageId);
  REQUIRE(retracted.status == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.removedCount == 2);
  REQUIRE(queue.pendingCount() == 0);

  auto const repeated = queue.retract(messageId);
  REQUIRE(repeated.status == umbra::detail::TsoMessageQueueStatus::message_already_retracted);
  REQUIRE(repeated.removedCount == 0);
  REQUIRE(queue.enqueue(messageId, 2, integerTime(7)).status ==
          umbra::detail::TsoMessageQueueStatus::message_already_retracted);
}

TEST_CASE(
    "Private 2025 TSO queue rejects invalid time inputs and fences delivered messages",
    "[tso][time][retraction][unit][time-management]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const deliveredId = queue.allocateMessageId();
  auto const initialId = queue.allocateMessageId();
  auto const finalId = queue.allocateMessageId();
  auto const mismatchId = queue.allocateMessageId();

  REQUIRE(queue.enqueue(0, 2, integerTime(1)).status ==
          umbra::detail::TsoMessageQueueStatus::invalid_message_id);
  REQUIRE(queue.enqueue(deliveredId, 0, integerTime(1)).status ==
          umbra::detail::TsoMessageQueueStatus::invalid_recipient);
  auto initial = std::make_shared<rti1516_2025::HLAinteger64Time>();
  auto final = std::make_shared<rti1516_2025::HLAinteger64Time>();
  final->setFinal();
  REQUIRE(queue.enqueue(initialId, 2, initial).status ==
          umbra::detail::TsoMessageQueueStatus::invalid_timestamp);
  REQUIRE(queue.enqueue(finalId, 2, final).status ==
          umbra::detail::TsoMessageQueueStatus::invalid_timestamp);
  REQUIRE(queue.enqueue(
              mismatchId,
              2,
              std::make_shared<rti1516_2025::HLAfloat64Time const>(1.0))
              .status ==
          umbra::detail::TsoMessageQueueStatus::logical_time_implementation_mismatch);

  REQUIRE(queue.enqueue(deliveredId, 2, integerTime(3)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  auto const delivered = queue.popEligible(2, *integerTime(3), true);
  REQUIRE(delivered.size() == 1);
  auto const retractDelivered = queue.retract(deliveredId);
  REQUIRE(retractDelivered.status ==
          umbra::detail::TsoMessageQueueStatus::message_already_delivered);
  REQUIRE(retractDelivered.removedCount == 0);
  REQUIRE(queue.enqueue(deliveredId, 2, integerTime(4)).status ==
          umbra::detail::TsoMessageQueueStatus::message_already_delivered);

  auto const partialId = queue.allocateMessageId();
  REQUIRE(queue.enqueue(partialId, 2, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(partialId, 3, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.popEligible(2, *integerTime(5), true).size() == 1);
  auto const partialRetraction = queue.retract(partialId);
  REQUIRE(partialRetraction.status ==
          umbra::detail::TsoMessageQueueStatus::message_already_delivered);
  REQUIRE(partialRetraction.removedCount == 0);
  REQUIRE(queue.pendingCountFor(3) == 1);
}

TEST_CASE(
    "Private 2025 TSO queue withdraws pending fanout after another recipient delivery",
    "[tso][time][retraction][unit][time-management]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const messageId = queue.allocateMessageId();

  REQUIRE(queue.enqueue(messageId, 2, integerTime(9)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(messageId, 3, integerTime(9)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.popEligible(2, *integerTime(9), true).size() == 1);
  REQUIRE(queue.pendingCountFor(3) == 1);

  // The owner-level recipient ledger decides whether recipient 2 needs a
  // Request Retraction callback. This queue-level operation is responsible
  // only for suppressing recipient 3's still-pending fanout entry.
  auto const retracted = queue.retractPending(messageId);
  REQUIRE(retracted.status == umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(retracted.removedCount == 1);
  REQUIRE(queue.pendingCount() == 0);
  REQUIRE(queue.popEligible(3, *integerTime(9), true).empty());

  auto const repeated = queue.retractPending(messageId);
  REQUIRE(repeated.status ==
          umbra::detail::TsoMessageQueueStatus::message_already_retracted);
}

TEST_CASE(
    "Private 2025 TSO queue keeps an exclusive boundary separate from an inclusive grant",
    "[tso][time][unit][time-management]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const beforeId = queue.allocateMessageId();
  auto const atId = queue.allocateMessageId();
  REQUIRE(queue.enqueue(beforeId, 2, integerTime(9)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(atId, 2, integerTime(10)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);

  auto const before = queue.popEligible(2, *integerTime(10), false);
  REQUIRE(before.size() == 1);
  REQUIRE(before.front().messageId == beforeId);
  REQUIRE(queue.pendingCountFor(2) == 1);

  auto const atBoundary = queue.popEligible(2, *integerTime(10), true);
  REQUIRE(atBoundary.size() == 1);
  REQUIRE(atBoundary.front().messageId == atId);
  REQUIRE(queue.pendingCount() == 0);
}

TEST_CASE(
    "Private 2025 TSO queue exposes stable global pending order and recipient cleanup",
    "[tso][time][unit][time-management][resignation]") {
  umbra::detail::TsoMessageQueue queue(L"HLAinteger64Time");
  auto const later = queue.allocateMessageId();
  auto const firstAtTimestamp = queue.allocateMessageId();
  auto const secondAtTimestamp = queue.allocateMessageId();

  REQUIRE(queue.enqueue(later, 2, integerTime(9)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(firstAtTimestamp, 3, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);
  REQUIRE(queue.enqueue(secondAtTimestamp, 2, integerTime(5)).status ==
          umbra::detail::TsoMessageQueueStatus::applied);

  auto const pending = queue.pendingMessages();
  REQUIRE(pending.size() == 3);
  REQUIRE(pending[0].messageId == firstAtTimestamp);
  REQUIRE(pending[1].messageId == secondAtTimestamp);
  REQUIRE(pending[2].messageId == later);
  REQUIRE(pending[0].recipientFederateId == 3);
  REQUIRE(pending[1].recipientFederateId == 2);
  REQUIRE(pending[2].recipientFederateId == 2);
  REQUIRE(pending[0].sequence < pending[1].sequence);

  REQUIRE(queue.discardRecipient(2) == 2);
  REQUIRE(queue.pendingCount() == 1);
  auto const remaining = queue.pendingMessages();
  REQUIRE(remaining.size() == 1);
  REQUIRE(remaining.front().messageId == firstAtTimestamp);
  REQUIRE(remaining.front().recipientFederateId == 3);
  REQUIRE(queue.discardRecipient(2) == 0);
}
