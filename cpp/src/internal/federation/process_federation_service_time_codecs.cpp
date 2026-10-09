#include "internal/federation/process_federation_service_protocol.hpp"
#include "internal/federation/process_federation_service_codec_support.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace umbra::detail {
using namespace process_federation_service_codec_support;
std::vector<std::uint8_t> encodeProcessFederationQueryLogicalTimeRequest(
    ProcessFederationQueryLogicalTimeRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time query requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process logical-time query requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  return std::move(writer).finish();
}

ProcessFederationQueryLogicalTimeRequest
decodeProcessFederationQueryLogicalTimeRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryLogicalTimeRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time query requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process logical-time query requires a federate identity.");
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationRequest(
    ProcessFederationEnableTimeRegulationRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-regulation request requires a federate identity.");
  if (request.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.lookahead.implementationName);
  writer.bytes(request.lookahead.encoding);
  return std::move(writer).finish();
}

ProcessFederationEnableTimeRegulationRequest
decodeProcessFederationEnableTimeRegulationRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationEnableTimeRegulationRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.lookahead.implementationName = reader.wideString();
  result.lookahead.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-regulation request requires a federate identity.");
  if (result.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation request requires a logical-time implementation.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationModifyLookaheadRequest(
    ProcessFederationModifyLookaheadRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process Modify Lookahead request requires a federate identity.");
  if (request.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.lookahead.implementationName);
  writer.bytes(request.lookahead.encoding);
  return std::move(writer).finish();
}

ProcessFederationModifyLookaheadRequest
decodeProcessFederationModifyLookaheadRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationModifyLookaheadRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.lookahead.implementationName = reader.wideString();
  result.lookahead.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process Modify Lookahead request requires a federate identity.");
  if (result.lookahead.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead request requires a logical-time implementation.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedRequest(
    ProcessFederationEnableTimeConstrainedRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-constrained request requires a federate identity.");
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  return std::move(writer).finish();
}

ProcessFederationEnableTimeConstrainedRequest
decodeProcessFederationEnableTimeConstrainedRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationEnableTimeConstrainedRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-constrained request requires a federate identity.");
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeAdvanceRequest(
    ProcessFederationTimeAdvanceRequest const& request) {
  if (request.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a federation name.");
  }
  requireNonzero(
      request.federateId,
      "A process time-advance request requires a federate identity.");
  if (request.requestedTime.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a logical-time implementation.");
  }
  PayloadWriter writer;
  writer.wideString(request.federationName);
  writer.unsigned64(request.federateId);
  writer.wideString(request.requestedTime.implementationName);
  writer.bytes(request.requestedTime.encoding);
  return std::move(writer).finish();
}

ProcessFederationTimeAdvanceRequest decodeProcessFederationTimeAdvanceRequest(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationTimeAdvanceRequest result;
  result.federationName = reader.wideString();
  result.federateId = reader.unsigned64();
  result.requestedTime.implementationName = reader.wideString();
  result.requestedTime.encoding = reader.bytes();
  reader.finish();
  if (result.federationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a federation name.");
  }
  requireNonzero(
      result.federateId,
      "A process time-advance request requires a federate identity.");
  if (result.requestedTime.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance request requires a logical-time implementation.");
  }
  return result;
}


std::vector<std::uint8_t> encodeProcessFederationQueryLogicalTimeResult(
    ProcessFederationQueryLogicalTimeResult const& result) {
  if (result.time.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time result requires an implementation name.");
  }
  PayloadWriter writer;
  writer.wideString(result.time.implementationName);
  writer.bytes(result.time.encoding);
  return std::move(writer).finish();
}

ProcessFederationQueryLogicalTimeResult
decodeProcessFederationQueryLogicalTimeResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  ProcessFederationQueryLogicalTimeResult result;
  result.time.implementationName = reader.wideString();
  result.time.encoding = reader.bytes();
  reader.finish();
  if (result.time.implementationName.empty()) {
    throw ProcessFederationServiceProtocolError(
        "A process logical-time result requires an implementation name.");
  }
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryLookaheadResult(
    ProcessFederationQueryLookaheadResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationLookaheadStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationLookaheadStatus::applied;
  if (result.lookahead.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an inconsistent interval marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.lookahead.has_value() ? 1U : 0U);
  if (result.lookahead) {
    if (result.lookahead->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process lookahead result requires a logical-time implementation.");
    }
    writer.wideString(result.lookahead->implementationName);
    writer.bytes(result.lookahead->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationQueryLookaheadResult
decodeProcessFederationQueryLookaheadResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationLookaheadStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an invalid interval marker.");
  }
  ProcessFederationQueryLookaheadResult result;
  result.status = static_cast<ProcessFederationLookaheadStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationLookaheadStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process lookahead result has an inconsistent interval marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTimeInterval lookahead;
    lookahead.implementationName = reader.wideString();
    lookahead.encoding = reader.bytes();
    if (lookahead.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process lookahead result requires a logical-time implementation.");
    }
    result.lookahead = std::move(lookahead);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationModifyLookaheadResult(
    ProcessFederationModifyLookaheadResult const& result) {
  auto const rawStatus = static_cast<std::uint8_t>(result.status);
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationModifyLookaheadStatus::invalid_lookahead)) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead result has an unknown status.");
  }
  PayloadWriter writer;
  writer.unsigned8(rawStatus);
  return std::move(writer).finish();
}

ProcessFederationModifyLookaheadResult
decodeProcessFederationModifyLookaheadResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationModifyLookaheadStatus::invalid_lookahead)) {
    throw ProcessFederationServiceProtocolError(
        "A process Modify Lookahead result has an unknown status.");
  }
  ProcessFederationModifyLookaheadResult result;
  result.status = static_cast<ProcessFederationModifyLookaheadStatus>(rawStatus);
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationQueryTimeBoundsResult(
    ProcessFederationQueryTimeBoundsResult const& result) {
  auto const rawStatus = static_cast<std::uint8_t>(result.status);
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeBoundStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-bounds result has an unknown status.");
  }
  bool const available =
      result.status == ProcessFederationTimeBoundStatus::available;
  bool const undefined =
      result.status == ProcessFederationTimeBoundStatus::undefined;
  if (available && (!result.galt || !result.lits)) {
    throw ProcessFederationServiceProtocolError(
        "An available process time-bounds result requires GALT and LITS.");
  }
  if (!available && result.galt) {
    throw ProcessFederationServiceProtocolError(
        "A non-available process time-bounds result cannot carry GALT.");
  }
  if (!available && !undefined && result.lits) {
    throw ProcessFederationServiceProtocolError(
        "An error process time-bounds result cannot carry LITS.");
  }
  PayloadWriter writer;
  writer.unsigned8(rawStatus);
  writeOptionalLogicalTime(writer, result.galt);
  writeOptionalLogicalTime(writer, result.lits);
  return std::move(writer).finish();
}

ProcessFederationQueryTimeBoundsResult
decodeProcessFederationQueryTimeBoundsResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeBoundStatus::inconsistent_temporal_state)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-bounds result has an unknown status.");
  }
  ProcessFederationQueryTimeBoundsResult result;
  result.status = static_cast<ProcessFederationTimeBoundStatus>(rawStatus);
  result.galt = readOptionalLogicalTime(reader);
  result.lits = readOptionalLogicalTime(reader);
  reader.finish();
  bool const available =
      result.status == ProcessFederationTimeBoundStatus::available;
  bool const undefined =
      result.status == ProcessFederationTimeBoundStatus::undefined;
  if (available && (!result.galt || !result.lits)) {
    throw ProcessFederationServiceProtocolError(
        "An available process time-bounds result requires GALT and LITS.");
  }
  if (!available && result.galt) {
    throw ProcessFederationServiceProtocolError(
        "A non-available process time-bounds result cannot carry GALT.");
  }
  if (!available && !undefined && result.lits) {
    throw ProcessFederationServiceProtocolError(
        "An error process time-bounds result cannot carry LITS.");
  }
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeRegulationResult(
    ProcessFederationEnableTimeRegulationResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if (result.enabledTime.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an invalid enabled-time marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.enabledTime.has_value() ? 1U : 0U);
  if (result.enabledTime) {
    if (result.enabledTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-regulation result requires a logical-time implementation.");
    }
    writer.wideString(result.enabledTime->implementationName);
    writer.bytes(result.enabledTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationEnableTimeRegulationResult
decodeProcessFederationEnableTimeRegulationResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an invalid enabled-time marker.");
  }
  ProcessFederationEnableTimeRegulationResult result;
  result.status = static_cast<ProcessFederationTimeEnableStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-regulation result has an inconsistent enabled-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime enabledTime;
    enabledTime.implementationName = reader.wideString();
    enabledTime.encoding = reader.bytes();
    if (enabledTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-regulation result requires a logical-time implementation.");
    }
    result.enabledTime = std::move(enabledTime);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t>
encodeProcessFederationEnableTimeConstrainedResult(
    ProcessFederationEnableTimeConstrainedResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if (result.enabledTime.has_value() != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an invalid enabled-time marker.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.enabledTime.has_value() ? 1U : 0U);
  if (result.enabledTime) {
    if (result.enabledTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-constrained result requires a logical-time implementation.");
    }
    writer.wideString(result.enabledTime->implementationName);
    writer.bytes(result.enabledTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationEnableTimeConstrainedResult
decodeProcessFederationEnableTimeConstrainedResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeEnableStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an invalid enabled-time marker.");
  }
  ProcessFederationEnableTimeConstrainedResult result;
  result.status = static_cast<ProcessFederationTimeEnableStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeEnableStatus::applied;
  if ((marker != 0U) != applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-constrained result has an inconsistent enabled-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime enabledTime;
    enabledTime.implementationName = reader.wideString();
    enabledTime.encoding = reader.bytes();
    if (enabledTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-constrained result requires a logical-time implementation.");
    }
    result.enabledTime = std::move(enabledTime);
  }
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeDisableResult(
    ProcessFederationTimeDisableResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeDisableStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-disable result has an unknown status.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  return std::move(writer).finish();
}

ProcessFederationTimeDisableResult decodeProcessFederationTimeDisableResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeDisableStatus::inactive)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-disable result has an unknown status.");
  }
  ProcessFederationTimeDisableResult result;
  result.status = static_cast<ProcessFederationTimeDisableStatus>(rawStatus);
  reader.finish();
  return result;
}

std::vector<std::uint8_t> encodeProcessFederationTimeAdvanceResult(
    ProcessFederationTimeAdvanceResult const& result) {
  auto const status = static_cast<std::uint8_t>(result.status);
  if (status > static_cast<std::uint8_t>(
                   ProcessFederationTimeAdvanceStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an unknown status.");
  }
  bool const applied =
      result.status == ProcessFederationTimeAdvanceStatus::applied;
  if (result.grantedTime && !applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an invalid grant-time marker.");
  }
  if (result.optimisticTime && (!applied || !result.grantedTime)) {
    throw ProcessFederationServiceProtocolError(
        "A process Flush Queue Grant has an invalid optimistic-time marker.");
  }
  if (result.optimisticTime &&
      result.optimisticTime->implementationName !=
          result.grantedTime->implementationName) {
    throw ProcessFederationServiceProtocolError(
        "A process Flush Queue Grant uses mismatched logical-time implementations.");
  }
  PayloadWriter writer;
  writer.unsigned8(status);
  writer.unsigned8(result.grantedTime.has_value() ? 1U : 0U);
  if (result.grantedTime) {
    if (result.grantedTime->implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-advance result requires a logical-time implementation.");
    }
    writer.wideString(result.grantedTime->implementationName);
    writer.bytes(result.grantedTime->encoding);
  }
  // The optional suffix preserves the original ordinary TAR wire shape while
  // allowing a Flush Queue Grant event to carry its second callback value.
  if (result.optimisticTime) {
    writer.unsigned8(1U);
    writer.wideString(result.optimisticTime->implementationName);
    writer.bytes(result.optimisticTime->encoding);
  }
  return std::move(writer).finish();
}

ProcessFederationTimeAdvanceResult decodeProcessFederationTimeAdvanceResult(
    std::span<std::uint8_t const> encoded) {
  PayloadReader reader(encoded);
  auto const rawStatus = reader.unsigned8();
  if (rawStatus > static_cast<std::uint8_t>(
                      ProcessFederationTimeAdvanceStatus::generation_exhausted)) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an unknown status.");
  }
  auto const marker = reader.unsigned8();
  if (marker > 1U) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an invalid grant-time marker.");
  }
  ProcessFederationTimeAdvanceResult result;
  result.status = static_cast<ProcessFederationTimeAdvanceStatus>(rawStatus);
  bool const applied =
      result.status == ProcessFederationTimeAdvanceStatus::applied;
  if (marker != 0U && !applied) {
    throw ProcessFederationServiceProtocolError(
        "A process time-advance result has an inconsistent grant-time marker.");
  }
  if (marker != 0U) {
    ProcessFederationLogicalTime grantedTime;
    grantedTime.implementationName = reader.wideString();
    grantedTime.encoding = reader.bytes();
    if (grantedTime.implementationName.empty()) {
      throw ProcessFederationServiceProtocolError(
          "A process time-advance result requires a logical-time implementation.");
    }
    result.grantedTime = std::move(grantedTime);
  }
  if (reader.remaining() != 0U) {
    auto const optimisticMarker = reader.unsigned8();
    if (optimisticMarker != 1U || !result.grantedTime || !applied) {
      throw ProcessFederationServiceProtocolError(
          "A process Flush Queue Grant has an invalid optimistic-time marker.");
    }
    ProcessFederationLogicalTime optimisticTime;
    optimisticTime.implementationName = reader.wideString();
    optimisticTime.encoding = reader.bytes();
    if (optimisticTime.implementationName.empty() ||
        optimisticTime.implementationName !=
            result.grantedTime->implementationName) {
      throw ProcessFederationServiceProtocolError(
          "A process Flush Queue Grant uses an invalid optimistic logical time.");
    }
    result.optimisticTime = std::move(optimisticTime);
  }
  reader.finish();
  return result;
}




}  // namespace umbra::detail
