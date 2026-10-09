#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {

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

bool ProcessFederationClient::getObjectClassRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_object_class_relevance_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

bool ProcessFederationClient::getInteractionRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_interaction_relevance_advisory_switch,
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

bool ProcessFederationClient::getConveyRegionDesignatorSetsSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_convey_region_designator_sets_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

bool ProcessFederationClient::getAllowRelaxedDDMSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_allow_relaxed_ddm_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setConveyRegionDesignatorSetsSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_convey_region_designator_sets_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
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


}  // namespace umbra::detail
