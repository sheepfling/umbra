#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

ProcessFederationRegisterObjectInstanceResult
ProcessFederationClient::registerObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::wstring> requestedObjectInstanceName) {
  auto response = request(
      TransportServiceOperation::register_object_instance,
      encodeProcessFederationRegisterObjectInstanceRequest(
          ProcessFederationRegisterObjectInstanceRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(requestedObjectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterObjectInstanceResult(response.payload);
}

ProcessFederationLocalDeleteObjectInstanceResult
ProcessFederationClient::localDeleteObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::local_delete_object_instance,
      encodeProcessFederationLocalDeleteObjectInstanceRequest(
          ProcessFederationLocalDeleteObjectInstanceRequest{
              std::move(federationName), federateId, objectInstanceHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationLocalDeleteObjectInstanceResult(
      response.payload);
}

ProcessFederationDeleteObjectInstanceResult
ProcessFederationClient::deleteObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::delete_object_instance,
      encodeProcessFederationDeleteObjectInstanceRequest(
          ProcessFederationDeleteObjectInstanceRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(userSuppliedTag),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDeleteObjectInstanceResult(response.payload);
}

ProcessFederationReserveObjectInstanceNameResult
ProcessFederationClient::reserveObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::reserve_object_instance_name,
      encodeProcessFederationReserveObjectInstanceNameRequest(
          ProcessFederationReserveObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReserveObjectInstanceNameResult(response.payload);
}

ProcessFederationObjectInstanceNameReleaseResult
ProcessFederationClient::releaseObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::release_object_instance_name,
      encodeProcessFederationReserveObjectInstanceNameRequest(
          ProcessFederationReserveObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceNameReleaseResult(
      response.payload);
}

ProcessFederationReserveMultipleObjectInstanceNamesResult
ProcessFederationClient::reserveMultipleObjectInstanceNames(
    std::wstring federationName,
    std::uint64_t federateId,
    std::set<std::wstring> objectInstanceNames) {
  auto response = request(
      TransportServiceOperation::reserve_multiple_object_instance_names,
      encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          ProcessFederationReserveMultipleObjectInstanceNamesRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceNames)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReserveMultipleObjectInstanceNamesResult(
      response.payload);
}

ProcessFederationReleaseMultipleObjectInstanceNamesResult
ProcessFederationClient::releaseMultipleObjectInstanceNames(
    std::wstring federationName,
    std::uint64_t federateId,
    std::set<std::wstring> objectInstanceNames) {
  auto response = request(
      TransportServiceOperation::release_multiple_object_instance_names,
      encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          ProcessFederationReserveMultipleObjectInstanceNamesRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceNames)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
      response.payload);
}


}  // namespace umbra::detail
