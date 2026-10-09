#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

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

ProcessFederationInteractionOrderTypeChangeResult
ProcessFederationClient::changeInteractionOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_interaction_order_type,
      encodeProcessFederationChangeInteractionOrderTypeRequest(
          ProcessFederationChangeInteractionOrderTypeRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionOrderTypeChangeResult(
      response.payload);
}

ProcessFederationAttributeOrderTypeChangeResult
ProcessFederationClient::changeAttributeOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> attributeHandles,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_attribute_order_type,
      encodeProcessFederationChangeAttributeOrderTypeRequest(
          ProcessFederationChangeAttributeOrderTypeRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOrderTypeChangeResult(response.payload);
}

ProcessFederationAttributeOrderTypeDefaultResult
ProcessFederationClient::changeDefaultAttributeOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> attributeHandles,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_default_attribute_order_type,
      encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
          ProcessFederationChangeDefaultAttributeOrderTypeRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOrderTypeDefaultResult(response.payload);
}

ProcessFederationAttributeTransportationTypeDefaultResult
ProcessFederationClient::changeDefaultAttributeTransportationType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> attributeHandles,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::change_default_attribute_transportation_type,
      encodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
          ProcessFederationChangeDefaultAttributeTransportationTypeRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeDefaultResult(
      response.payload);
}

ProcessFederationAttributeTransportationTypeChangeResult
ProcessFederationClient::requestAttributeTransportationTypeChange(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> attributeHandles,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::request_attribute_transportation_type_change,
      encodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
          ProcessFederationRequestAttributeTransportationTypeChangeRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeChangeResult(
      response.payload);
}

ProcessFederationAttributeTransportationTypeQueryResult
ProcessFederationClient::queryAttributeTransportationType(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::query_attribute_transportation_type,
      encodeProcessFederationQueryAttributeTransportationTypeRequest(
          ProcessFederationQueryAttributeTransportationTypeRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeQueryResult(
      response.payload);
}

ProcessFederationInteractionTransportationTypeChangeResult
ProcessFederationClient::requestInteractionTransportationTypeChange(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::request_interaction_transportation_type_change,
      encodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
          ProcessFederationRequestInteractionTransportationTypeChangeRequest{
              std::move(federationName),
              requestingFederateId,
              interactionClassHandle,
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionTransportationTypeChangeResult(
      response.payload);
}

ProcessFederationInteractionTransportationTypeQueryResult
ProcessFederationClient::queryInteractionTransportationType(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::query_interaction_transportation_type,
      encodeProcessFederationQueryInteractionTransportationTypeRequest(
          ProcessFederationQueryInteractionTransportationTypeRequest{
              std::move(federationName),
              requestingFederateId,
              queriedFederateId,
              interactionClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionTransportationTypeQueryResult(
      response.payload);
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
  if (response.status == TransportServiceStatus::service_reporting_interlock) {
    throw rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM(
        L"The report-service-invocation interaction cannot be subscribed while service reporting is enabled.");
  }
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

void ProcessFederationClient::subscribeInteractionClassWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> regionHandles,
    bool active) {
  auto response = request(
      TransportServiceOperation::subscribe_interaction_class_with_regions,
      encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          ProcessFederationInteractionClassRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(regionHandles),
              active}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeInteractionClassWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> regionHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_interaction_class_with_regions,
      encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          ProcessFederationInteractionClassRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(regionHandles),
              false}));
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

void ProcessFederationClient::unpublishObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class_attributes,
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

void ProcessFederationClient::unpublishObjectClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class,
      encodeProcessFederationObjectClassAttributeDeclarationRequest(
          ProcessFederationObjectClassAttributeDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {}}));
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


}  // namespace umbra::detail
