#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleQueryLogicalTime(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  // Query the state object established by Join. Process time-regulation and
  // grant operations mutate this retained object, so the read-only operation
  // must not reconstruct a new initial value for each query.
  if (!timeState) {
    return internalError(request);
  }
  auto const current = timeState->currentTime();
  if (!current || current->implementationName() != timeState->implementationName()) {
    return internalError(request);
  }
  auto const encoded = current->encode();
  std::vector<std::uint8_t> bytes;
  if (encoded.size() != 0U) {
    auto const* data = static_cast<unsigned char const*>(encoded.data());
    if (data == nullptr) {
      return internalError(request);
    }
    bytes.assign(data, data + encoded.size());
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryLogicalTimeResult(
          ProcessFederationQueryLogicalTimeResult{
          ProcessFederationLogicalTime{
                  timeState->implementationName(),
                  std::move(bytes)}}));
}

TransportServiceMessage ProcessFederationService::handleQueryLookahead(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  if (!timeState) {
    return internalError(request);
  }

  auto const current = timeState->currentLookahead();
  ProcessFederationQueryLookaheadResult result;
  switch (current.status) {
    case FederateTimeLookaheadStatus::applied: {
      if (!current.lookahead ||
          current.lookahead->implementationName() !=
              timeState->implementationName()) {
        return internalError(request);
      }
      auto const encoded = current.lookahead->encode();
      std::vector<std::uint8_t> bytes;
      if (encoded.size() != 0U) {
        auto const* data = static_cast<std::uint8_t const*>(encoded.data());
        if (data == nullptr) {
          return internalError(request);
        }
        bytes.assign(data, data + encoded.size());
      }
      result.status = ProcessFederationLookaheadStatus::applied;
      result.lookahead = ProcessFederationLogicalTimeInterval{
          timeState->implementationName(), std::move(bytes)};
      break;
    }
    case FederateTimeLookaheadStatus::not_enabled:
      result.status = ProcessFederationLookaheadStatus::not_enabled;
      break;
    case FederateTimeLookaheadStatus::inactive:
      result.status = ProcessFederationLookaheadStatus::inactive;
      break;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryLookaheadResult(result));
}

TransportServiceMessage ProcessFederationService::handleModifyLookahead(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const modify =
      decodeProcessFederationModifyLookaheadRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != modify.federationName ||
        state->second.federateId != modify.federateId ||
        !registry_.memberById(modify.federationName, modify.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }

  if (!timeState) {
    return internalError(request);
  }

  ProcessFederationModifyLookaheadResult processResult;
  processResult.status =
      ProcessFederationModifyLookaheadStatus::invalid_lookahead;
  if (modify.lookahead.implementationName != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationModifyLookaheadResult(processResult));
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      timeState->implementationName());
  if (!factory || factory->getName() != timeState->implementationName()) {
    return internalError(request);
  }
  rti1516_2025::VariableLengthData encodedLookahead;
  if (!modify.lookahead.encoding.empty()) {
    encodedLookahead.setData(
        modify.lookahead.encoding.data(), modify.lookahead.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodedLookahead;
  try {
    decodedLookahead = factory->decodeLogicalTimeInterval(encodedLookahead);
    auto zero = factory->makeZero();
    if (!decodedLookahead || !zero ||
        decodedLookahead->implementationName() != timeState->implementationName() ||
        *decodedLookahead < *zero) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationModifyLookaheadResult(processResult));
    }
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationModifyLookaheadResult(processResult));
  }

  auto const stateResult = timeState->modifyLookahead(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval>(
          std::move(decodedLookahead)));
  switch (stateResult) {
    case FederateTimeModifyLookaheadStatus::applied:
      processResult.status = ProcessFederationModifyLookaheadStatus::applied;
      break;
    case FederateTimeModifyLookaheadStatus::time_advance_pending:
      processResult.status =
          ProcessFederationModifyLookaheadStatus::time_advance_pending;
      break;
    case FederateTimeModifyLookaheadStatus::not_enabled:
      processResult.status = ProcessFederationModifyLookaheadStatus::not_enabled;
      break;
    case FederateTimeModifyLookaheadStatus::inactive:
      processResult.status = ProcessFederationModifyLookaheadStatus::inactive;
      break;
  }

  if (stateResult == FederateTimeModifyLookaheadStatus::applied) {
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(
        modify.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        modify.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationModifyLookaheadResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleQueryTimeBounds(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const query = decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != query.federationName ||
        state->second.federateId != query.federateId ||
        !registry_.memberById(query.federationName, query.federateId)) {
      return rejected(request);
    }
  }

  auto const snapshot = registry_.timeSnapshotFor(query.federationName);
  if (!snapshot) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationQueryTimeBoundsResult(
            ProcessFederationQueryTimeBoundsResult{
                ProcessFederationTimeBoundStatus::inconsistent_temporal_state,
                std::nullopt,
                std::nullopt}));
  }
  auto const bounds = FederationTimeBoundsCalculator{}.calculate(
      *snapshot, query.federateId);
  ProcessFederationQueryTimeBoundsResult result;
  switch (bounds.status) {
    case FederationTimeBoundStatus::available:
      result.status = ProcessFederationTimeBoundStatus::available;
      result.galt = encodeProcessLogicalTime(bounds.galt);
      result.lits = encodeProcessLogicalTime(bounds.lits);
      break;
    case FederationTimeBoundStatus::undefined:
      result.status = ProcessFederationTimeBoundStatus::undefined;
      result.lits = encodeProcessLogicalTime(bounds.lits);
      break;
    case FederationTimeBoundStatus::requesting_federate_not_registered:
      result.status =
          ProcessFederationTimeBoundStatus::requesting_federate_not_registered;
      break;
    case FederationTimeBoundStatus::factory_unavailable:
      result.status = ProcessFederationTimeBoundStatus::factory_unavailable;
      break;
    case FederationTimeBoundStatus::inconsistent_temporal_state:
      result.status =
          ProcessFederationTimeBoundStatus::inconsistent_temporal_state;
      break;
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationQueryTimeBoundsResult(result));
}

TransportServiceMessage ProcessFederationService::handleEnableTimeRegulation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const enable =
      decodeProcessFederationEnableTimeRegulationRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != enable.federationName ||
        state->second.federateId != enable.federateId ||
        !registry_.memberById(enable.federationName, enable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  // Decode the official interval before touching the federate state.  A
  // mismatched implementation or negative interval is a normal
  // InvalidLookahead outcome, while an unavailable factory is an endpoint
  // configuration failure.
  ProcessFederationEnableTimeRegulationResult processResult;
  processResult.status = ProcessFederationTimeEnableStatus::invalid_lookahead;
  if (enable.lookahead.implementationName != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      timeState->implementationName());
  if (!factory || factory->getName() != timeState->implementationName()) {
    return internalError(request);
  }
  rti1516_2025::VariableLengthData encodedLookahead;
  if (!enable.lookahead.encoding.empty()) {
    encodedLookahead.setData(
        enable.lookahead.encoding.data(), enable.lookahead.encoding.size());
  }
  std::unique_ptr<rti1516_2025::LogicalTimeInterval> decodedLookahead;
  try {
    decodedLookahead = factory->decodeLogicalTimeInterval(encodedLookahead);
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  if (!decodedLookahead ||
      decodedLookahead->implementationName() != timeState->implementationName()) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }
  try {
    auto zero = factory->makeZero();
    if (!zero || *decodedLookahead < *zero) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationEnableTimeRegulationResult(processResult));
    }
  } catch (rti1516_2025::Exception const&) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationEnableTimeRegulationResult(processResult));
  }

  auto requestResult = timeState->requestTimeRegulation(
      std::shared_ptr<rti1516_2025::LogicalTimeInterval>(
          std::move(decodedLookahead)));
  switch (requestResult.status) {
    case FederateTimeEnableStatus::applied:
      processResult.status = ProcessFederationTimeEnableStatus::applied;
      break;
    case FederateTimeEnableStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeEnableStatus::time_advance_pending;
      break;
    case FederateTimeEnableStatus::request_pending:
      processResult.status = ProcessFederationTimeEnableStatus::request_pending;
      break;
    case FederateTimeEnableStatus::already_enabled:
      processResult.status = ProcessFederationTimeEnableStatus::already_enabled;
      break;
    case FederateTimeEnableStatus::invalid_lookahead:
      processResult.status =
          ProcessFederationTimeEnableStatus::invalid_lookahead;
      break;
    case FederateTimeEnableStatus::inactive:
      processResult.status = ProcessFederationTimeEnableStatus::inactive;
      break;
    case FederateTimeEnableStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeEnableStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeEnableStatus::applied) {
    // Complete the accepted role request against server-owned state and return
    // the exact value that the client callback bridge will deliver through
    // FederateAmbassador::timeRegulationEnabled. Any newly eligible grants are
    // dispatched through the federation-owned scheduler after this transition.
    auto enabledTime = timeState->grantTimeRegulation(requestResult.generation);
    if (!enabledTime) {
      return internalError(request);
    }
    auto const encodedTime = enabledTime->encode();
    std::vector<std::uint8_t> bytes;
    if (encodedTime.size() != 0U) {
      auto const* data = static_cast<unsigned char const*>(encodedTime.data());
      if (data == nullptr) {
        return internalError(request);
      }
      bytes.assign(data, data + encodedTime.size());
    }
    processResult.enabledTime = ProcessFederationLogicalTime{
        timeState->implementationName(), std::move(bytes)};
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(enable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        enable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationEnableTimeRegulationResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleEnableTimeConstrained(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const enable =
      decodeProcessFederationEnableTimeConstrainedRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != enable.federationName ||
        state->second.federateId != enable.federateId ||
        !registry_.memberById(enable.federationName, enable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  auto const requestResult = timeState->requestTimeConstrained();
  ProcessFederationEnableTimeConstrainedResult processResult;
  switch (requestResult.status) {
    case FederateTimeEnableStatus::applied:
      processResult.status = ProcessFederationTimeEnableStatus::applied;
      break;
    case FederateTimeEnableStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeEnableStatus::time_advance_pending;
      break;
    case FederateTimeEnableStatus::request_pending:
      processResult.status = ProcessFederationTimeEnableStatus::request_pending;
      break;
    case FederateTimeEnableStatus::already_enabled:
      processResult.status = ProcessFederationTimeEnableStatus::already_enabled;
      break;
    case FederateTimeEnableStatus::invalid_lookahead:
      processResult.status =
          ProcessFederationTimeEnableStatus::invalid_lookahead;
      break;
    case FederateTimeEnableStatus::inactive:
      processResult.status = ProcessFederationTimeEnableStatus::inactive;
      break;
    case FederateTimeEnableStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeEnableStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeEnableStatus::applied) {
    // Complete the accepted role request against server-owned state and
    // return the exact callback value. Any newly eligible cross-federate time
    // grants are dispatched after this transition through the shared scheduler.
    auto enabledTime = timeState->grantTimeConstrained(requestResult.generation);
    if (!enabledTime) {
      return internalError(request);
    }
    processResult.enabledTime = encodeProcessLogicalTime(enabledTime);
    if (!processResult.enabledTime) {
      return internalError(request);
    }
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(enable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        enable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationEnableTimeConstrainedResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleDisableTimeRegulation(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleDisableTimeRole(session, request, true);
}

TransportServiceMessage ProcessFederationService::handleDisableTimeConstrained(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleDisableTimeRole(session, request, false);
}

TransportServiceMessage ProcessFederationService::handleDisableTimeRole(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    bool regulation) {
  auto const disable =
      decodeProcessFederationQueryLogicalTimeRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != disable.federationName ||
        state->second.federateId != disable.federateId ||
        !registry_.memberById(disable.federationName, disable.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  auto const stateResult = regulation
      ? timeState->disableTimeRegulation()
      : timeState->disableTimeConstrained();
  ProcessFederationTimeDisableResult processResult;
  switch (stateResult) {
    case FederateTimeDisableStatus::applied:
      processResult.status = ProcessFederationTimeDisableStatus::applied;
      break;
    case FederateTimeDisableStatus::not_enabled:
      processResult.status = ProcessFederationTimeDisableStatus::not_enabled;
      break;
    case FederateTimeDisableStatus::inactive:
      processResult.status = ProcessFederationTimeDisableStatus::inactive;
      break;
  }

  if (stateResult == FederateTimeDisableStatus::applied) {
    // Disabling either role can release a TAR that was waiting on the
    // corresponding temporal bound. Re-evaluate only after the role state has
    // changed, then dispatch any newly eligible grants through the same
    // federation-owned callback path used by enable and lookahead services.
    auto scheduled = registry_.reevaluateTimeAdvanceGrants(
        disable.federationName);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        disable.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTimeDisableResult(processResult));
}

TransportServiceMessage ProcessFederationService::handleTimeAdvanceRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::time_advance_request);
}

TransportServiceMessage
ProcessFederationService::handleTimeAdvanceRequestAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session,
      request,
      FederateTimeAdvanceMode::time_advance_request_available);
}

TransportServiceMessage ProcessFederationService::handleNextMessageRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::next_message_request);
}

TransportServiceMessage
ProcessFederationService::handleNextMessageRequestAvailable(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session,
      request,
      FederateTimeAdvanceMode::next_message_request_available);
}

TransportServiceMessage ProcessFederationService::handleFlushQueueRequest(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  return handleTimeAdvanceRequestForMode(
      session, request, FederateTimeAdvanceMode::flush_queue_request);
}

TransportServiceMessage ProcessFederationService::handleTimeAdvanceRequestForMode(
    ProcessTransportSession& session,
    TransportServiceMessage const& request,
    FederateTimeAdvanceMode mode) {
  auto const advance =
      decodeProcessFederationTimeAdvanceRequest(request.payload);
  std::shared_ptr<FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != advance.federationName ||
        state->second.federateId != advance.federateId ||
        !registry_.memberById(advance.federationName, advance.federateId)) {
      return rejected(request);
    }
    timeState = state->second.timeState;
  }
  if (!timeState) {
    return internalError(request);
  }

  ProcessFederationTimeAdvanceResult processResult;
  std::shared_ptr<rti1516_2025::LogicalTime const> decodedRequestedTime;
  try {
    decodedRequestedTime = decodeProcessLogicalTime(
        advance.requestedTime, timeState->implementationName());
  } catch (ProcessFederationServiceProtocolError const&) {
    processResult.status =
        ProcessFederationTimeAdvanceStatus::invalid_logical_time;
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationTimeAdvanceResult(processResult));
  }

  std::shared_ptr<rti1516_2025::LogicalTime> effectiveTime;
  if (mode == FederateTimeAdvanceMode::next_message_request ||
      mode == FederateTimeAdvanceMode::next_message_request_available) {
    auto const queued = registry_.earliestTsoTimestampFor(
        advance.federationName, advance.federateId);
    if (queued && *queued) {
      try {
        auto encodedQueued = encodeProcessLogicalTime(*queued);
        if (encodedQueued) {
          auto candidate = decodeProcessLogicalTime(
              *encodedQueued, timeState->implementationName());
          auto const currentTime = timeState->currentTime();
          if (currentTime && *candidate >= *currentTime &&
              *candidate <= *decodedRequestedTime) {
            effectiveTime = std::const_pointer_cast<rti1516_2025::LogicalTime>(
                std::move(candidate));
          }
        }
      } catch (rti1516_2025::Exception const&) {
        processResult.status =
            ProcessFederationTimeAdvanceStatus::invalid_logical_time;
        return responseFor(
            request,
            TransportServiceStatus::ok,
            encodeProcessFederationTimeAdvanceResult(processResult));
      } catch (ProcessFederationServiceProtocolError const&) {
        processResult.status =
            ProcessFederationTimeAdvanceStatus::invalid_logical_time;
        return responseFor(
            request,
            TransportServiceStatus::ok,
            encodeProcessFederationTimeAdvanceResult(processResult));
      }
    }
  }

  FederateTimeAdvanceResult requestResult;
  auto mutableRequestedTime = std::const_pointer_cast<rti1516_2025::LogicalTime>(
      std::move(decodedRequestedTime));
  if (mode == FederateTimeAdvanceMode::time_advance_request_available) {
    requestResult = timeState->requestAdvanceAvailable(
        std::move(mutableRequestedTime));
  } else if (mode == FederateTimeAdvanceMode::next_message_request) {
    requestResult = timeState->requestNextMessageAdvance(
        std::move(mutableRequestedTime), std::move(effectiveTime));
  } else if (mode == FederateTimeAdvanceMode::next_message_request_available) {
    requestResult = timeState->requestNextMessageAvailableAdvance(
        std::move(mutableRequestedTime), std::move(effectiveTime));
  } else if (mode == FederateTimeAdvanceMode::flush_queue_request) {
    requestResult = timeState->requestFlushQueueAdvance(
        std::move(mutableRequestedTime));
  } else {
    requestResult = timeState->requestAdvance(std::move(mutableRequestedTime));
  }
  switch (requestResult.status) {
    case FederateTimeAdvanceStatus::applied:
      processResult.status = ProcessFederationTimeAdvanceStatus::applied;
      break;
    case FederateTimeAdvanceStatus::logical_time_already_passed:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::logical_time_already_passed;
      break;
    case FederateTimeAdvanceStatus::time_advance_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_advance_pending;
      break;
    case FederateTimeAdvanceStatus::time_regulation_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_regulation_pending;
      break;
    case FederateTimeAdvanceStatus::time_constrained_pending:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::time_constrained_pending;
      break;
    case FederateTimeAdvanceStatus::invalid_logical_time:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::invalid_logical_time;
      break;
    case FederateTimeAdvanceStatus::inactive:
      processResult.status = ProcessFederationTimeAdvanceStatus::inactive;
      break;
    case FederateTimeAdvanceStatus::generation_exhausted:
      processResult.status =
          ProcessFederationTimeAdvanceStatus::generation_exhausted;
      break;
  }

  if (requestResult.status == FederateTimeAdvanceStatus::applied) {
    // Register the accepted request with the federation-owned scheduler. An
    // eligible grant is sent as an unsolicited event (possibly before this
    // response); a constrained request remains pending until a later temporal
    // state change re-evaluates the shared bound.
    auto scheduled = registry_.requestTimeAdvanceGrant(
        advance.federationName,
        advance.federateId,
        requestResult.generation);
    if (scheduled.status != FederationTimeGrantStatus::applied) {
      return internalError(request);
    }
    dispatchTimeAdvanceGrants(
        advance.federationName,
        std::move(scheduled.dispatches));
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationTimeAdvanceResult(processResult));
}

}  // namespace umbra::detail
