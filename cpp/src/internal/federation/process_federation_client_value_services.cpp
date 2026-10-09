#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

ProcessFederationSendInteractionResult ProcessFederationClient::sendInteraction(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_interaction,
      encodeProcessFederationSendInteractionRequest(
          ProcessFederationSendInteractionRequest{
              std::move(federationName),
              producingFederateId,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationSendInteractionResult
ProcessFederationClient::sendInteractionWithRegions(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::set<std::uint64_t> sentRegionHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_interaction_with_regions,
      encodeProcessFederationSendInteractionRequest(
          ProcessFederationSendInteractionRequest{
              std::move(federationName),
              producingFederateId,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp),
              std::move(sentRegionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationSendInteractionResult
ProcessFederationClient::sendDirectedInteraction(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_directed_interaction,
      encodeProcessFederationSendDirectedInteractionRequest(
          ProcessFederationSendDirectedInteractionRequest{
              std::move(federationName),
              producingFederateId,
              objectInstanceHandle,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationRetractResult ProcessFederationClient::retract(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId) {
  auto response = request(
      TransportServiceOperation::retract,
      encodeProcessFederationRetractRequest(
          ProcessFederationRetractRequest{
              std::move(federationName), producingFederateId, messageId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRetractResult(response.payload);
}

ProcessFederationUpdateAttributeValuesResult
ProcessFederationClient::updateAttributeValues(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<ProcessFederationAttributeValue> attributeValues,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::update_attribute_values,
      encodeProcessFederationUpdateAttributeValuesRequest(
          ProcessFederationUpdateAttributeValuesRequest{
              std::move(federationName),
              producingFederateId,
              objectInstanceHandle,
              std::move(attributeValues),
              std::move(userSuppliedTag),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateAttributeValuesResult(response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdate(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update,
      encodeProcessFederationRequestAttributeValueUpdateRequest(
          ProcessFederationRequestAttributeValueUpdateRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(requestedAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdateClass(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update_class,
      encodeProcessFederationRequestAttributeValueUpdateClassRequest(
          ProcessFederationRequestAttributeValueUpdateClassRequest{
              std::move(federationName),
              requestingFederateId,
              objectClassHandle,
              std::move(requestedAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdateClassWithRegions(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update_class_with_regions,
      encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
          ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest{
              std::move(federationName),
              requestingFederateId,
              objectClassHandle,
              std::move(requestedAttributeHandles),
              std::move(requestRegionsByAttribute),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    // Semantic planner failures keep the transport rejection bit, but carry
    // a typed result so the public ambassador can preserve the official HLA
    // exception rather than collapsing every remote validation into
    // RTIinternalError.  Empty rejection payloads remain transport failures.
    if (response.status == TransportServiceStatus::rejected &&
        !response.payload.empty()) {
      return decodeProcessFederationRequestAttributeValueUpdateResult(
          response.payload);
    }
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

std::optional<ProcessFederationInteractionEvent>
ProcessFederationClient::receiveInteraction(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_interaction,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationReceiveInteractionResult(response.payload);
  auto event = std::move(result.event);
  result.event.reset();
  {
    std::scoped_lock lock(transactionMutex_);
    if (session_) {
      bufferReceiveResult(std::move(result));
    }
  }
  return event;
}

std::optional<ProcessFederationAttributeUpdateEvent>
ProcessFederationClient::receiveAttributeUpdate(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_attribute_update,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReceiveAttributeUpdateResult(response.payload)
      .event;
}

std::optional<ProcessFederationObjectInstanceDiscoveryEvent>
ProcessFederationClient::receiveObjectInstanceDiscovery(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_object_instance_discovery,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
             response.payload)
      .event;
}


}  // namespace umbra::detail
