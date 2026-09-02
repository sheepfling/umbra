#include "internal/federation/process_federation_client.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace umbra::detail {
namespace {

[[nodiscard]] char const* statusName(TransportServiceStatus status) noexcept {
  switch (status) {
    case TransportServiceStatus::ok:
      return "ok";
    case TransportServiceStatus::rejected:
      return "rejected";
    case TransportServiceStatus::invalid_request:
      return "invalid-request";
    case TransportServiceStatus::internal_error:
      return "internal-error";
  }
  return "unknown";
}

[[nodiscard]] char const* operationName(
    TransportServiceOperation operation) noexcept {
  switch (operation) {
    case TransportServiceOperation::create_federation_execution:
      return "create-federation-execution";
    case TransportServiceOperation::join_federation_execution:
      return "join-federation-execution";
    case TransportServiceOperation::resign_federation_execution:
      return "resign-federation-execution";
    case TransportServiceOperation::send_interaction:
      return "send-interaction";
    case TransportServiceOperation::update_attribute_values:
      return "update-attribute-values";
    case TransportServiceOperation::receive_interaction:
      return "receive-interaction";
    case TransportServiceOperation::receive_attribute_update:
      return "receive-attribute-update";
    case TransportServiceOperation::get_interaction_class_handle:
      return "get-interaction-class-handle";
    case TransportServiceOperation::get_object_class_handle:
      return "get-object-class-handle";
    case TransportServiceOperation::get_parameter_handle:
      return "get-parameter-handle";
    case TransportServiceOperation::publish_interaction_class:
      return "publish-interaction-class";
    case TransportServiceOperation::unpublish_interaction_class:
      return "unpublish-interaction-class";
    case TransportServiceOperation::subscribe_interaction_class:
      return "subscribe-interaction-class";
    case TransportServiceOperation::unsubscribe_interaction_class:
      return "unsubscribe-interaction-class";
    case TransportServiceOperation::publish_object_class_attributes:
      return "publish-object-class-attributes";
    case TransportServiceOperation::register_object_instance:
      return "register-object-instance";
    case TransportServiceOperation::get_attribute_handle:
      return "get-attribute-handle";
    case TransportServiceOperation::subscribe_object_class_attributes:
      return "subscribe-object-class-attributes";
    case TransportServiceOperation::unsubscribe_object_class_attributes:
      return "unsubscribe-object-class-attributes";
    case TransportServiceOperation::receive_object_instance_discovery:
      return "receive-object-instance-discovery";
    case TransportServiceOperation::reserve_object_instance_name:
      return "reserve-object-instance-name";
    case TransportServiceOperation::get_dimension_handle:
      return "get-dimension-handle";
    case TransportServiceOperation::get_dimension_upper_bound:
      return "get-dimension-upper-bound";
    case TransportServiceOperation::create_region:
      return "create-region";
    case TransportServiceOperation::commit_region_modifications:
      return "commit-region-modifications";
    case TransportServiceOperation::delete_region:
      return "delete-region";
    case TransportServiceOperation::get_dimension_handle_set:
      return "get-dimension-handle-set";
    case TransportServiceOperation::get_range_bounds:
      return "get-range-bounds";
    case TransportServiceOperation::set_range_bounds:
      return "set-range-bounds";
    case TransportServiceOperation::register_object_instance_with_regions:
      return "register-object-instance-with-regions";
    case TransportServiceOperation::subscribe_object_class_attributes_with_regions:
      return "subscribe-object-class-attributes-with-regions";
    case TransportServiceOperation::unsubscribe_object_class_attributes_with_regions:
      return "unsubscribe-object-class-attributes-with-regions";
    case TransportServiceOperation::get_attribute_scope_advisory_switch:
      return "get-attribute-scope-advisory-switch";
    case TransportServiceOperation::set_attribute_scope_advisory_switch:
      return "set-attribute-scope-advisory-switch";
    case TransportServiceOperation::associate_regions_for_updates:
      return "associate-regions-for-updates";
    case TransportServiceOperation::unassociate_regions_for_updates:
      return "unassociate-regions-for-updates";
    case TransportServiceOperation::get_attribute_relevance_advisory_switch:
      return "get-attribute-relevance-advisory-switch";
    case TransportServiceOperation::set_attribute_relevance_advisory_switch:
      return "set-attribute-relevance-advisory-switch";
    case TransportServiceOperation::publish_object_class_directed_interactions:
      return "publish-object-class-directed-interactions";
    case TransportServiceOperation::unpublish_object_class_directed_interactions:
      return "unpublish-object-class-directed-interactions";
    case TransportServiceOperation::subscribe_object_class_directed_interactions:
      return "subscribe-object-class-directed-interactions";
    case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
      return "unsubscribe-object-class-directed-interactions";
    case TransportServiceOperation::send_directed_interaction:
      return "send-directed-interaction";
    case TransportServiceOperation::retract:
      return "retract";
    case TransportServiceOperation::request_retraction:
      return "request-retraction";
    case TransportServiceOperation::local_delete_object_instance:
      return "local-delete-object-instance";
    case TransportServiceOperation::delete_object_instance:
      return "delete-object-instance";
  }
  return "unknown";
}

[[nodiscard]] std::string requestFailure(
    TransportServiceOperation operation,
    TransportServiceStatus status) {
  return std::string("Process federation ") + operationName(operation) +
      " was not accepted (" + statusName(status) + ").";
}

}  // namespace

ProcessFederationClient::ProcessFederationClient(
    ProcessTransportAddress address,
    TransportEndpointIdentity localIdentity,
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    FailureHandler failureHandler,
    ForcedResignationHandler forcedResignationHandler)
    : connection_(ProcessTransportConnection::connectClient(
          this,
          std::move(address),
          std::move(localIdentity),
          failureHandler ? std::move(failureHandler)
                         : FailureHandler{[](std::wstring) {}},
          forcedResignationHandler
              ? std::move(forcedResignationHandler)
              : ForcedResignationHandler{[](std::wstring) { return false; }},
          std::move(instrumentation))),
      session_(std::make_unique<ProcessTransportSession>(connection_)),
      attributeRelevanceAdvisorySwitchState_(
          std::make_shared<std::atomic_bool>(true)) {}

ProcessFederationClient::~ProcessFederationClient() {
  close();
}

void ProcessFederationClient::createFederationExecution(
    std::wstring federationName) {
  auto response = request(
      TransportServiceOperation::create_federation_execution,
      encodeProcessFederationCreateRequest(
          ProcessFederationCreateRequest{std::move(federationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName) {
  auto response = request(
      TransportServiceOperation::join_federation_execution,
      encodeProcessFederationJoinRequest(
          ProcessFederationJoinRequest{
              std::move(federationName),
              std::move(federateType),
              std::move(requestedFederateName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationJoinResult(response.payload);
}

void ProcessFederationClient::resignFederationExecution(
    std::wstring federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto response = request(
      TransportServiceOperation::resign_federation_execution,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName), federateId, resignAction}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

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

std::optional<std::uint64_t>
ProcessFederationClient::lookupObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectClassName) {
  auto response = request(
      TransportServiceOperation::get_object_class_handle,
      encodeProcessFederationGetObjectClassHandleRequest(
          ProcessFederationGetObjectClassHandleRequest{
              std::move(federationName),
              federateId,
              std::move(objectClassName)}));
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

void ProcessFederationClient::publishInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::publish_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::unpublish_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    bool active) {
  auto response = request(
      TransportServiceOperation::subscribe_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, active}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::unsubscribe_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::publishObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::publish_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::vector<std::uint64_t>> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> interactionClassHandles,
    bool universally) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              universally}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::vector<std::uint64_t>> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::publishObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::publish_object_class_attributes,
      encodeProcessFederationObjectClassAttributeDeclarationRequest(
          ProcessFederationObjectClassAttributeDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles,
    bool active,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_attributes,
      encodeProcessFederationObjectClassAttributeSubscriptionRequest(
          ProcessFederationObjectClassAttributeSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles),
              active,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_attributes,
      encodeProcessFederationObjectClassAttributeSubscriptionRequest(
          ProcessFederationObjectClassAttributeSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles),
              false,
              {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassAttributesWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions,
    bool active,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_attributes_with_regions,
      encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          ProcessFederationObjectClassAttributeRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributesAndRegions),
              active,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassAttributesWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_attributes_with_regions,
      encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          ProcessFederationObjectClassAttributeRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributesAndRegions),
              false,
              {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

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

ProcessFederationCreateRegionResult ProcessFederationClient::createRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::vector<std::uint64_t> dimensionHandles) {
  auto response = request(
      TransportServiceOperation::create_region,
      encodeProcessFederationCreateRegionRequest(
          ProcessFederationCreateRegionRequest{
              std::move(federationName), federateId, std::move(dimensionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationCreateRegionResult(response.payload);
}

ProcessFederationRegionStatusResult
ProcessFederationClient::commitRegionModifications(
    std::wstring federationName,
    std::uint64_t federateId,
    std::vector<std::uint64_t> regionHandles) {
  auto response = request(
      TransportServiceOperation::commit_region_modifications,
      encodeProcessFederationRegionSetRequest(
          ProcessFederationRegionSetRequest{
              std::move(federationName), federateId, std::move(regionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

ProcessFederationRegionStatusResult ProcessFederationClient::deleteRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  auto response = request(
      TransportServiceOperation::delete_region,
      encodeProcessFederationRegionRequest(
          ProcessFederationRegionRequest{
              std::move(federationName), federateId, regionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

ProcessFederationDimensionSetResult
ProcessFederationClient::dimensionHandleSetForRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_handle_set,
      encodeProcessFederationRegionRequest(
          ProcessFederationRegionRequest{
              std::move(federationName), federateId, regionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDimensionSetResult(response.payload);
}

ProcessFederationRangeBoundsResult ProcessFederationClient::rangeBoundsForRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_range_bounds,
      encodeProcessFederationGetRangeBoundsRequest(
          ProcessFederationGetRangeBoundsRequest{
              std::move(federationName),
              federateId,
              regionHandle,
              dimensionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRangeBoundsResult(response.payload);
}

ProcessFederationRegionStatusResult ProcessFederationClient::setRangeBounds(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle,
    unsigned long lowerBound,
    unsigned long upperBound) {
  auto response = request(
      TransportServiceOperation::set_range_bounds,
      encodeProcessFederationSetRangeBoundsRequest(
          ProcessFederationSetRangeBoundsRequest{
              std::move(federationName),
              federateId,
              regionHandle,
              dimensionHandle,
              lowerBound,
              upperBound}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

bool ProcessFederationClient::getAttributeScopeAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_attribute_scope_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setAttributeScopeAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_attribute_scope_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

bool ProcessFederationClient::getAttributeRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_attribute_relevance_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setAttributeRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  // Predict the requested state before the request enters the receive loop.
  // This closes the HLA_EVOKED race in which a previously admitted advisory
  // is still waiting in the callback dispatcher while this setter disables
  // the switch. Restore the prior state if the service call fails.
  auto const previousState = attributeRelevanceAdvisorySwitchState_->exchange(
      switchValue,
      std::memory_order_acq_rel);
  try {
    auto response = request(
        TransportServiceOperation::set_attribute_relevance_advisory_switch,
        encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
            ProcessFederationAttributeScopeAdvisorySwitchRequest{
                std::move(federationName), federateId, switchValue}));
    if (response.status != TransportServiceStatus::ok) {
      throw ProcessFederationClientError(
          requestFailure(response.operation, response.status));
    }
    static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
  } catch (...) {
    attributeRelevanceAdvisorySwitchState_->store(
        previousState,
        std::memory_order_release);
    throw;
  }
}

ProcessFederationRegisterObjectInstanceResult
ProcessFederationClient::registerObjectInstanceWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute,
    std::optional<std::wstring> requestedObjectInstanceName) {
  auto response = request(
      TransportServiceOperation::register_object_instance_with_regions,
      encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
          ProcessFederationRegisterObjectInstanceWithRegionsRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(updateRegionsByAttribute),
              std::move(requestedObjectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterObjectInstanceResult(response.payload);
}

ProcessFederationObjectInstanceRegionAssociationResult
ProcessFederationClient::associateRegionsForUpdates(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::associate_regions_for_updates,
      encodeProcessFederationObjectInstanceRegionAssociationRequest(
          ProcessFederationObjectInstanceRegionAssociationRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(attributesAndRegions)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceRegionAssociationResult(
      response.payload);
}

ProcessFederationObjectInstanceRegionAssociationResult
ProcessFederationClient::unassociateRegionsForUpdates(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::unassociate_regions_for_updates,
      encodeProcessFederationObjectInstanceRegionAssociationRequest(
          ProcessFederationObjectInstanceRegionAssociationRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(attributesAndRegions)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceRegionAssociationResult(
      response.payload);
}

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
  if (result.attributeEvent) {
    pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
  }
  if (result.discoveryEvent) {
    pendingObjectInstanceDiscoveryEvents_.push_back(std::move(*result.discoveryEvent));
  }
  if (result.removalEvent) {
    pendingObjectInstanceRemovalEvents_.push_back(
        std::move(*result.removalEvent));
  }
  if (result.scopeChangeEvent) {
    pendingObjectInstanceScopeChangeEvents_.push_back(
        std::move(*result.scopeChangeEvent));
  }
  if (result.attributeRelevanceAdvisoryEvent) {
    pendingAttributeRelevanceAdvisoryEvents_.push_back(
        std::move(*result.attributeRelevanceAdvisoryEvent));
  }
  return std::move(result.event);
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

void ProcessFederationClient::dispatchPushedReceiveOrder() {
  dispatchReceiveOrder(receivePushedEvent());
}

void ProcessFederationClient::dispatchReceiveOrder(
    ProcessFederationInteractionEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitReceiveOrder(std::move(event));
}

void ProcessFederationClient::dispatchAttributeUpdate(
    ProcessFederationAttributeUpdateEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeUpdate(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeUpdate() {
  dispatchAttributeUpdate(receivePushedAttributeUpdate());
}

void ProcessFederationClient::dispatchObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceDiscovery(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceDiscovery() {
  dispatchObjectInstanceDiscovery(receivePushedObjectInstanceDiscovery());
}

void ProcessFederationClient::dispatchObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceRemoval(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceRemoval() {
  dispatchObjectInstanceRemoval(receivePushedObjectInstanceRemoval());
}

void ProcessFederationClient::dispatchObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceScopeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceScopeChange() {
  dispatchObjectInstanceScopeChange(receivePushedObjectInstanceScopeChange());
}

void ProcessFederationClient::dispatchAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeRelevanceAdvisory(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeRelevanceAdvisory() {
  dispatchAttributeRelevanceAdvisory(
      receivePushedAttributeRelevanceAdvisory());
}

void ProcessFederationClient::dispatchPendingPushedEvents() {
  if (!callbackBridge_) {
    return;
  }
  while (!pendingEvents_.empty()) {
    dispatchPushedReceiveOrder();
  }
  while (!pendingObjectInstanceDiscoveryEvents_.empty()) {
    dispatchPushedObjectInstanceDiscovery();
  }
  while (!pendingObjectInstanceRemovalEvents_.empty()) {
    dispatchPushedObjectInstanceRemoval();
  }
  while (!pendingObjectInstanceScopeChangeEvents_.empty()) {
    dispatchPushedObjectInstanceScopeChange();
  }
  while (!pendingAttributeRelevanceAdvisoryEvents_.empty()) {
    dispatchPushedAttributeRelevanceAdvisory();
  }
  while (!pendingAttributeUpdateEvents_.empty()) {
    dispatchPushedAttributeUpdate();
  }
}

std::size_t ProcessFederationClient::pendingPushedEventCount() const noexcept {
  return pendingEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeUpdateCount() const noexcept {
  return pendingAttributeUpdateEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceDiscoveryCount()
    const noexcept {
  return pendingObjectInstanceDiscoveryEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceRemovalCount()
    const noexcept {
  return pendingObjectInstanceRemovalEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceScopeChangeCount()
    const noexcept {
  return pendingObjectInstanceScopeChangeEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeRelevanceAdvisoryCount()
    const noexcept {
  return pendingAttributeRelevanceAdvisoryEvents_.size();
}

void ProcessFederationClient::attachCallbackBridge(
    rti1516_2025::FederateAmbassador& recipient,
    CallbackDispatchModel model) {
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_unique<ProcessFederationCallbackBridge>(
      recipient, model);
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
}

void ProcessFederationClient::attachCallbackBridge(
    std::shared_ptr<CallbackDispatcher> dispatcher,
    std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
        callbackSession) {
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_unique<ProcessFederationCallbackBridge>(
      std::move(dispatcher), std::move(callbackSession));
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
}

bool ProcessFederationClient::evokeOne(
    std::chrono::milliseconds minimumWait) {
  requireCallbackBridge();
  return callbackBridge_->evokeOne(minimumWait);
}

bool ProcessFederationClient::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  requireCallbackBridge();
  return callbackBridge_->evokeMultiple(minimumWait, maximumWait);
}

std::size_t ProcessFederationClient::pendingCallbackCount() const {
  return callbackBridge_ ? callbackBridge_->pendingCount() : 0U;
}

std::shared_ptr<ProcessTransportConnection>
ProcessFederationClient::connection() const noexcept {
  return connection_;
}

void ProcessFederationClient::close() noexcept {
  if (callbackBridge_) {
    callbackBridge_->close();
    callbackBridge_.reset();
  }
  session_.reset();
  if (connection_) {
    connection_->close();
    connection_.reset();
  }
  pendingEvents_.clear();
  pendingAttributeUpdateEvents_.clear();
  pendingObjectInstanceDiscoveryEvents_.clear();
  pendingObjectInstanceRemovalEvents_.clear();
  pendingObjectInstanceScopeChangeEvents_.clear();
  pendingAttributeRelevanceAdvisoryEvents_.clear();
}

TransportServiceMessage ProcessFederationClient::request(
    TransportServiceOperation operation,
    std::vector<std::uint8_t> payload) {
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation request was attempted after client close.");
  }
  if (nextRequestId_ == std::numeric_limits<std::uint64_t>::max()) {
    throw ProcessFederationClientError(
        "The process federation request identity space is exhausted.");
  }
  auto const requestId = ++nextRequestId_;
  TransportServiceMessage requestMessage{
      TransportServiceMessageKind::request,
      operation,
      TransportServiceStatus::ok,
      requestId,
      std::move(payload)};
  if (!session_->send(requestMessage)) {
    throw ProcessFederationClientError(
        std::string("Process federation ") + operationName(operation) +
        " could not send its request.");
  }

  while (true) {
    TransportServiceMessage response;
    if (!session_->receive(response)) {
      throw ProcessFederationClientError(
          std::string("Process federation ") + operationName(operation) +
          " lost its response.");
    }
      if (response.kind == TransportServiceMessageKind::event) {
        if (response.status != TransportServiceStatus::ok ||
            response.requestId != 0U ||
            (response.operation != TransportServiceOperation::receive_interaction &&
             response.operation != TransportServiceOperation::receive_attribute_update &&
             response.operation !=
                 TransportServiceOperation::receive_object_instance_discovery &&
             response.operation != TransportServiceOperation::request_retraction)) {
          throw ProcessFederationClientError(
              "Process federation client received an invalid unsolicited event.");
        }
        if (response.operation == TransportServiceOperation::request_retraction) {
          auto const retraction =
              decodeProcessFederationRequestRetractionEvent(response.payload);
          auto const pendingEventsBefore = pendingEvents_.size();
          auto const pendingRemovalsBefore =
              pendingObjectInstanceRemovalEvents_.size();
          pendingEvents_.erase(
              std::remove_if(
                  pendingEvents_.begin(),
                  pendingEvents_.end(),
                  [&retraction](ProcessFederationInteractionEvent const& event) {
                    return event.retractionMessageId == retraction.messageId;
                  }),
              pendingEvents_.end());
          pendingObjectInstanceRemovalEvents_.erase(
              std::remove_if(
                  pendingObjectInstanceRemovalEvents_.begin(),
                  pendingObjectInstanceRemovalEvents_.end(),
                  [&retraction](
                      ProcessFederationObjectInstanceRemovalEvent const& event) {
                    return event.retractionMessageId == retraction.messageId;
                  }),
              pendingObjectInstanceRemovalEvents_.end());
          if (pendingEvents_.size() == pendingEventsBefore &&
              pendingObjectInstanceRemovalEvents_.size() == pendingRemovalsBefore &&
              callbackBridge_) {
            callbackBridge_->submitRequestRetraction(retraction.messageId);
          }
        } else if (response.operation == TransportServiceOperation::receive_interaction) {
        auto event = decodeProcessFederationReceiveInteractionResult(response.payload);
        if (!event.event && !event.removalEvent && !event.scopeChangeEvent &&
            !event.attributeRelevanceAdvisoryEvent) {
          throw ProcessFederationClientError(
              "Process federation client received an empty unsolicited event.");
        }
        if (event.event) {
          pendingEvents_.push_back(std::move(*event.event));
        }
        if (event.removalEvent) {
          pendingObjectInstanceRemovalEvents_.push_back(
              std::move(*event.removalEvent));
        }
        if (event.scopeChangeEvent) {
          pendingObjectInstanceScopeChangeEvents_.push_back(
              std::move(*event.scopeChangeEvent));
        }
        if (event.attributeRelevanceAdvisoryEvent) {
          pendingAttributeRelevanceAdvisoryEvents_.push_back(
              std::move(*event.attributeRelevanceAdvisoryEvent));
        }
        } else if (response.operation ==
                   TransportServiceOperation::receive_attribute_update) {
          auto event = decodeProcessFederationReceiveAttributeUpdateResult(
              response.payload);
          if (!event.event) {
          throw ProcessFederationClientError(
              "Process federation client received an empty unsolicited attribute event.");
          }
          pendingAttributeUpdateEvents_.push_back(std::move(*event.event));
        } else {
          auto event =
              decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
                  response.payload);
          if (!event.event) {
            throw ProcessFederationClientError(
                "Process federation client received an empty unsolicited discovery event.");
          }
          pendingObjectInstanceDiscoveryEvents_.push_back(
              std::move(*event.event));
        }
        continue;
    }
    if (response.kind != TransportServiceMessageKind::response ||
        response.requestId != requestId || response.operation != operation) {
      throw ProcessFederationClientError(
          "Process federation client received a response for another request.");
    }
    // A service response can be preceded by unsolicited frames emitted for a
    // different joined federate.  Do not leave those frames stranded in the
    // private queues: HLA_IMMEDIATE must invoke them before this public RTI
    // service returns, while HLA_EVOKED still merely queues them for Evoke.
    dispatchPendingPushedEvents();
    return response;
  }
}

ProcessFederationInteractionEvent ProcessFederationClient::receivePushedEvent() {
  if (!pendingEvents_.empty()) {
    auto event = std::move(pendingEvents_.front());
    pendingEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::request_retraction)) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed message.");
    }
    if (message.operation == TransportServiceOperation::request_retraction) {
      auto const retraction =
          decodeProcessFederationRequestRetractionEvent(message.payload);
      auto const pendingEventsBefore = pendingEvents_.size();
      auto const pendingRemovalsBefore =
          pendingObjectInstanceRemovalEvents_.size();
      pendingEvents_.erase(
          std::remove_if(
              pendingEvents_.begin(),
              pendingEvents_.end(),
              [&retraction](ProcessFederationInteractionEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingEvents_.end());
      pendingObjectInstanceRemovalEvents_.erase(
          std::remove_if(
              pendingObjectInstanceRemovalEvents_.begin(),
              pendingObjectInstanceRemovalEvents_.end(),
              [&retraction](
                  ProcessFederationObjectInstanceRemovalEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingObjectInstanceRemovalEvents_.end());
      if (pendingEvents_.size() == pendingEventsBefore &&
          pendingObjectInstanceRemovalEvents_.size() == pendingRemovalsBefore &&
          callbackBridge_) {
        callbackBridge_->submitRequestRetraction(retraction.messageId);
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (!result.event && !result.removalEvent && !result.scopeChangeEvent &&
        !result.attributeRelevanceAdvisoryEvent && !result.attributeEvent &&
        !result.discoveryEvent) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed event.");
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
    if (result.event) {
      return std::move(*result.event);
    }
  }
}

ProcessFederationObjectInstanceRemovalEvent
ProcessFederationClient::receivePushedObjectInstanceRemoval() {
  if (!pendingObjectInstanceRemovalEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceRemovalEvents_.front());
    pendingObjectInstanceRemovalEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation removal event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed removal event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::request_retraction)) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed removal event.");
    }
    if (message.operation == TransportServiceOperation::request_retraction) {
      auto const retraction =
          decodeProcessFederationRequestRetractionEvent(message.payload);
      auto const pendingBefore = pendingObjectInstanceRemovalEvents_.size();
      pendingObjectInstanceRemovalEvents_.erase(
          std::remove_if(
              pendingObjectInstanceRemovalEvents_.begin(),
              pendingObjectInstanceRemovalEvents_.end(),
              [&retraction](
                  ProcessFederationObjectInstanceRemovalEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingObjectInstanceRemovalEvents_.end());
      if (pendingObjectInstanceRemovalEvents_.size() == pendingBefore &&
          callbackBridge_) {
        callbackBridge_->submitRequestRetraction(retraction.messageId);
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.removalEvent) {
      if (result.event) {
        pendingEvents_.push_back(std::move(*result.event));
      }
      if (result.scopeChangeEvent) {
        pendingObjectInstanceScopeChangeEvents_.push_back(
            std::move(*result.scopeChangeEvent));
      }
      if (result.attributeRelevanceAdvisoryEvent) {
        pendingAttributeRelevanceAdvisoryEvents_.push_back(
            std::move(*result.attributeRelevanceAdvisoryEvent));
      }
      if (result.attributeEvent) {
        pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
      }
      if (result.discoveryEvent) {
        pendingObjectInstanceDiscoveryEvents_.push_back(
            std::move(*result.discoveryEvent));
      }
      return std::move(*result.removalEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
  }
}

ProcessFederationAttributeUpdateEvent
ProcessFederationClient::receivePushedAttributeUpdate() {
  if (!pendingAttributeUpdateEvents_.empty()) {
    auto event = std::move(pendingAttributeUpdateEvents_.front());
    pendingAttributeUpdateEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation attribute event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed attribute event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.operation != TransportServiceOperation::receive_attribute_update ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed attribute event.");
    }
    auto result = decodeProcessFederationReceiveAttributeUpdateResult(message.payload);
    if (!result.event) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed attribute event.");
    }
    return std::move(*result.event);
  }
}

ProcessFederationObjectInstanceDiscoveryEvent
ProcessFederationClient::receivePushedObjectInstanceDiscovery() {
  if (!pendingObjectInstanceDiscoveryEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceDiscoveryEvents_.front());
    pendingObjectInstanceDiscoveryEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation discovery event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed discovery event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.operation !=
            TransportServiceOperation::receive_object_instance_discovery ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed discovery event.");
    }
    auto result = decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
        message.payload);
    if (!result.event) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed discovery event.");
    }
    return std::move(*result.event);
  }
}

ProcessFederationObjectInstanceScopeChangeEvent
ProcessFederationClient::receivePushedObjectInstanceScopeChange() {
  if (!pendingObjectInstanceScopeChangeEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceScopeChangeEvents_.front());
    pendingObjectInstanceScopeChangeEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation scope event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed scope event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.operation != TransportServiceOperation::receive_interaction ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed scope event.");
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.scopeChangeEvent) {
      return std::move(*result.scopeChangeEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(std::move(*result.discoveryEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
  }
}

ProcessFederationAttributeRelevanceAdvisoryEvent
ProcessFederationClient::receivePushedAttributeRelevanceAdvisory() {
  if (!pendingAttributeRelevanceAdvisoryEvents_.empty()) {
    auto event = std::move(pendingAttributeRelevanceAdvisoryEvents_.front());
    pendingAttributeRelevanceAdvisoryEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation advisory event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed advisory event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.operation != TransportServiceOperation::receive_interaction ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed advisory event.");
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.attributeRelevanceAdvisoryEvent) {
      return std::move(*result.attributeRelevanceAdvisoryEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
  }
}

void ProcessFederationClient::requireCallbackBridge() const {
  if (!callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge has not been attached.");
  }
}

}  // namespace umbra::detail
