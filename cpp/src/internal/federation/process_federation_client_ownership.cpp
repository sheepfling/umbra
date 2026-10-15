#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <algorithm>
#include <utility>

namespace umbra::detail {

void ProcessFederationClient::installAttributeOwnershipReleaseClaimHandler() {
  auto bridge = requireCallbackBridge();
  bridge->setAttributeOwnershipReleaseClaimHandler(
      [this](ProcessFederationAttributeOwnershipAcquisitionEvent const& event) {
        return claimAttributeOwnershipRelease(event);
      });
}

std::optional<ProcessFederationAttributeOwnershipAcquisitionEvent>
ProcessFederationClient::claimAttributeOwnershipRelease(
    ProcessFederationAttributeOwnershipAcquisitionEvent const& event) {
  if (event.kind !=
      ProcessFederationAttributeOwnershipAcquisitionEventKind::request_release) {
    throw ProcessFederationClientError(
        "A process owner-release claim requires a release event.");
  }
  {
    std::scoped_lock lock(transactionMutex_);
    if (closing_.load(std::memory_order_acquire) ||
        !joinedFederationName_ || joinedFederateId_ != event.receivingFederateId) {
      return std::nullopt;
    }
  }
  ProcessFederationReceiveInteractionResult claim;
  claim.attributeOwnershipAcquisitionEvent = event;
  auto const response = request(
      TransportServiceOperation::claim_attribute_ownership_release,
      encodeProcessFederationReceiveInteractionResult(claim), false);
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationReceiveInteractionResult(response.payload)
                    .attributeOwnershipAcquisitionEvent;
  if (!result) {
    return std::nullopt;
  }
  if (result->kind != event.kind || result->requestId != event.requestId ||
      result->requestingFederateId != event.requestingFederateId ||
      result->receivingFederateId != event.receivingFederateId ||
      result->objectInstanceHandle != event.objectInstanceHandle ||
      result->userSuppliedTag != event.userSuppliedTag ||
      !std::includes(event.attributeHandles.begin(), event.attributeHandles.end(),
                     result->attributeHandles.begin(), result->attributeHandles.end())) {
    throw ProcessFederationClientError(
        "A process owner-release claim returned an inconsistent callback.");
  }
  return result;
}

ProcessFederationAttributeOwnershipCheckResult
ProcessFederationClient::attributeOwnershipCheck(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::is_attribute_owned_by_federate,
      encodeProcessFederationAttributeOwnershipCheckRequest(
          ProcessFederationAttributeOwnershipCheckRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipCheckResult(response.payload);
}

ProcessFederationAttributeOwnershipQueryResult
ProcessFederationClient::queryAttributeOwnership(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> requestedAttributeHandles) {
  auto response = request(
      TransportServiceOperation::query_attribute_ownership,
      encodeProcessFederationAttributeOwnershipQueryRequest(
          ProcessFederationAttributeOwnershipQueryRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(requestedAttributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipQueryResult(response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
ProcessFederationClient::attributeOwnershipAcquisitionIfAvailable(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> desiredAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_acquisition_if_available,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
          ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(desiredAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
      response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionResult
ProcessFederationClient::attributeOwnershipAcquisition(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> desiredAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_acquisition,
      encodeProcessFederationAttributeOwnershipAcquisitionRequest(
          ProcessFederationAttributeOwnershipAcquisitionRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(desiredAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionResult(
      response.payload);
}

ProcessFederationBooleanResult
ProcessFederationClient::unconditionalAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::unconditional_attribute_ownership_divestiture,
      encodeProcessFederationAttributeOwnershipAcquisitionRequest(
          ProcessFederationAttributeOwnershipAcquisitionRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag),
              callbacksEnabled}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload);
}

ProcessFederationAttributeOwnershipReleaseDeniedResult
ProcessFederationClient::attributeOwnershipReleaseDenied(
    std::wstring federationName,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_release_denied,
      encodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
          ProcessFederationAttributeOwnershipReleaseDeniedRequest{
              std::move(federationName),
              owningFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipReleaseDeniedResult(
      response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionCancellationResult
ProcessFederationClient::cancelAttributeOwnershipAcquisition(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::cancel_attribute_ownership_acquisition,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
          ProcessFederationAttributeOwnershipAcquisitionCancellationRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
      response.payload);
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
ProcessFederationClient::cancelNegotiatedAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::
          cancel_negotiated_attribute_ownership_divestiture,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
          ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
      response.payload);
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
ProcessFederationClient::negotiatedAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::negotiated_attribute_ownership_divestiture,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
          ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
      response.payload);
}

ProcessFederationConfirmDivestitureResult
ProcessFederationClient::confirmDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::confirm_divestiture,
      encodeProcessFederationConfirmDivestitureRequest(
          ProcessFederationConfirmDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationConfirmDivestitureResult(response.payload);
}

}  // namespace umbra::detail
