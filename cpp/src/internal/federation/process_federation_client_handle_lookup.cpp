#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <utility>

namespace umbra::detail {

std::optional<std::uint64_t>
ProcessFederationClient::lookupInteractionClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring interactionClassName) {
  auto response = request(
      TransportServiceOperation::get_interaction_class_handle,
      encodeProcessFederationGetInteractionClassHandleRequest(
          ProcessFederationGetInteractionClassHandleRequest{
              std::move(federationName),
              federateId,
              std::move(interactionClassName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupFederateHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring federateName) {
  auto response = request(
      TransportServiceOperation::get_federate_handle,
      encodeProcessFederationGetFederateHandleRequest(
          ProcessFederationGetFederateHandleRequest{
              std::move(federationName),
              federateId,
              std::move(federateName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupFederateName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t targetFederateId) {
  auto response = request(
      TransportServiceOperation::get_federate_name,
      encodeProcessFederationGetFederateNameRequest(
          ProcessFederationGetFederateNameRequest{
              std::move(federationName), federateId, targetFederateId}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t> ProcessFederationClient::normalizeFederateHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_federate_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::normalizeObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_object_class_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::normalizeInteractionClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_interaction_class_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::normalizeObjectInstanceHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_object_instance_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectClassName,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::get_object_class_handle,
      encodeProcessFederationGetObjectClassHandleRequest(
          ProcessFederationGetObjectClassHandleRequest{
              std::move(federationName),
              federateId,
              std::move(objectClassName),
              callbacksEnabled}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupParameterHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::wstring parameterName) {
  auto response = request(
      TransportServiceOperation::get_parameter_handle,
      encodeProcessFederationGetParameterHandleRequest(
          ProcessFederationGetParameterHandleRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(parameterName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupAttributeHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::wstring attributeName) {
  auto response = request(
      TransportServiceOperation::get_attribute_handle,
      encodeProcessFederationGetAttributeHandleRequest(
          ProcessFederationGetAttributeHandleRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupObjectInstanceHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::get_object_instance_handle,
      encodeProcessFederationGetObjectInstanceHandleRequest(
          ProcessFederationGetObjectInstanceHandleRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::get_object_instance_name,
      encodeProcessFederationGetObjectInstanceNameRequest(
          ProcessFederationGetObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupKnownObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::get_known_object_class_handle,
      encodeProcessFederationGetKnownObjectClassHandleRequest(
          ProcessFederationGetKnownObjectClassHandleRequest{
              std::move(federationName), federateId, objectInstanceHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupObjectClassName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::get_object_class_name,
      encodeProcessFederationGetObjectClassNameRequest(
          ProcessFederationGetObjectClassNameRequest{
              std::move(federationName), federateId, objectClassHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring>
ProcessFederationClient::lookupInteractionClassName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::get_interaction_class_name,
      encodeProcessFederationGetInteractionClassNameRequest(
          ProcessFederationGetInteractionClassNameRequest{
              std::move(federationName), federateId, interactionClassHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring> ProcessFederationClient::lookupAttributeName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::get_attribute_name,
      encodeProcessFederationGetAttributeNameRequest(
          ProcessFederationGetAttributeNameRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              attributeHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring> ProcessFederationClient::lookupParameterName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::uint64_t parameterHandle) {
  auto response = request(
      TransportServiceOperation::get_parameter_name,
      encodeProcessFederationGetParameterNameRequest(
          ProcessFederationGetParameterNameRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              parameterHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupDimensionHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string dimensionName) {
  auto response = request(
      TransportServiceOperation::get_dimension_handle,
      encodeProcessFederationGetDimensionHandleRequest(
          ProcessFederationGetDimensionHandleRequest{
              std::move(federationName), federateId, std::move(dimensionName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupDimensionName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_name,
      encodeProcessFederationDimensionRequest(
          ProcessFederationDimensionRequest{
              std::move(federationName), federateId, dimensionHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupTransportationTypeHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string transportationTypeName) {
  auto response = request(
      TransportServiceOperation::get_transportation_type_handle,
      encodeProcessFederationGetTransportationTypeHandleRequest(
          ProcessFederationGetTransportationTypeHandleRequest{
              std::move(federationName), federateId,
              std::move(transportationTypeName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring>
ProcessFederationClient::lookupTransportationTypeName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::get_transportation_type_name,
      encodeProcessFederationTransportationTypeRequest(
          ProcessFederationTransportationTypeRequest{
              std::move(federationName), federateId,
              transportationTypeHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

ProcessFederationDimensionUpperBoundResult
ProcessFederationClient::lookupDimensionUpperBound(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_upper_bound,
      encodeProcessFederationDimensionRequest(
          ProcessFederationDimensionRequest{
              std::move(federationName), federateId, dimensionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDimensionUpperBoundResult(response.payload);
}

ProcessFederationAvailableDimensionsResult
ProcessFederationClient::availableDimensionsForObjectClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::get_available_dimensions_for_object_class,
      encodeProcessFederationClassHandleRequest(
          ProcessFederationClassHandleRequest{
              std::move(federationName), federateId, objectClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAvailableDimensionsResult(response.payload);
}

ProcessFederationAvailableDimensionsResult
ProcessFederationClient::availableDimensionsForInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::get_available_dimensions_for_interaction_class,
      encodeProcessFederationClassHandleRequest(
          ProcessFederationClassHandleRequest{
              std::move(federationName), federateId, interactionClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAvailableDimensionsResult(response.payload);
}

}  // namespace umbra::detail
