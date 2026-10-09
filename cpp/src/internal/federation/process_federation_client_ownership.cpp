#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <utility>

namespace umbra::detail {

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
