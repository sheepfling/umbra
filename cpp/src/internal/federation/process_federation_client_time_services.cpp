#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

ProcessFederationLogicalTime ProcessFederationClient::queryLogicalTime(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_logical_time,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryLogicalTimeResult(response.payload).time;
}

ProcessFederationQueryLookaheadResult ProcessFederationClient::queryLookahead(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_lookahead,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryLookaheadResult(response.payload);
}

ProcessFederationModifyLookaheadResult
ProcessFederationClient::modifyLookahead(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTimeInterval lookahead) {
  auto response = request(
      TransportServiceOperation::modify_lookahead,
      encodeProcessFederationModifyLookaheadRequest(
          ProcessFederationModifyLookaheadRequest{
              std::move(federationName), federateId, std::move(lookahead)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationModifyLookaheadResult(response.payload);
}

ProcessFederationQueryTimeBoundsResult
ProcessFederationClient::queryTimeBounds(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_time_bounds,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryTimeBoundsResult(response.payload);
}

ProcessFederationEnableTimeRegulationResult
ProcessFederationClient::enableTimeRegulation(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTimeInterval lookahead) {
  auto response = request(
      TransportServiceOperation::enable_time_regulation,
      encodeProcessFederationEnableTimeRegulationRequest(
          ProcessFederationEnableTimeRegulationRequest{
              std::move(federationName), federateId, std::move(lookahead)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationEnableTimeRegulationResult(response.payload);
}

ProcessFederationEnableTimeConstrainedResult
ProcessFederationClient::enableTimeConstrained(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::enable_time_constrained,
      encodeProcessFederationEnableTimeConstrainedRequest(
          ProcessFederationEnableTimeConstrainedRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationEnableTimeConstrainedResult(response.payload);
}

ProcessFederationTimeDisableResult
ProcessFederationClient::disableTimeRegulation(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::disable_time_regulation,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeDisableResult(response.payload);
}

ProcessFederationTimeDisableResult
ProcessFederationClient::disableTimeConstrained(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::disable_time_constrained,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeDisableResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::timeAdvanceRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::time_advance_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult
ProcessFederationClient::timeAdvanceRequestAvailable(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::time_advance_request_available,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::nextMessageRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::next_message_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult
ProcessFederationClient::nextMessageRequestAvailable(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::next_message_request_available,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::flushQueueRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::flush_queue_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}


ProcessFederationUpdateRateValueResult
ProcessFederationClient::getUpdateRateValue(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::get_update_rate_value,
      encodeProcessFederationGetUpdateRateValueRequest(
          ProcessFederationGetUpdateRateValueRequest{
              std::move(federationName),
              federateId,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateRateValueResult(response.payload);
}

ProcessFederationUpdateRateValueResult
ProcessFederationClient::getUpdateRateValueForAttribute(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::get_update_rate_value_for_attribute,
      encodeProcessFederationGetUpdateRateValueForAttributeRequest(
          ProcessFederationGetUpdateRateValueForAttributeRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateRateValueResult(response.payload);
}




}  // namespace umbra::detail
