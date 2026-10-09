#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleRegisterObjectInstanceWithRegions(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const registrationRequest =
      decodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != registrationRequest.federationName ||
        state->second.federateId != registrationRequest.federateId ||
        !registry_.memberById(
            registrationRequest.federationName, registrationRequest.federateId)) {
      return rejected(request);
    }
  }
  std::wstring const *requestedName = nullptr;
  if (registrationRequest.requestedObjectInstanceName.has_value()) {
    requestedName = &*registrationRequest.requestedObjectInstanceName;
  }
  auto const result = registry_.registerObjectInstance(
      registrationRequest.federationName,
      registrationRequest.federateId,
      registrationRequest.objectClassHandle,
      &registrationRequest.updateRegionsByAttribute,
      requestedName);
  if (result.status != ObjectInstanceRegistrationStatus::applied) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRegisterObjectInstanceResult(
            ProcessFederationRegisterObjectInstanceResult{0U, {}, result.status}));
  }
  auto discoveries = registry_.planObjectInstanceDiscoveriesForInstance(
      registrationRequest.federationName, result.objectInstanceHandle);
  if (!enqueueObjectInstanceDiscoveries(
          registrationRequest.federationName, std::move(discoveries))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRegisterObjectInstanceResult(
          ProcessFederationRegisterObjectInstanceResult{
              result.objectInstanceHandle,
              result.objectInstanceName,
              result.status}));
}

TransportServiceMessage
ProcessFederationService::handleObjectInstanceRegionAssociation(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const associationRequest =
      decodeProcessFederationObjectInstanceRegionAssociationRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != associationRequest.federationName ||
        state->second.federateId != associationRequest.federateId ||
        !registry_.memberById(
            associationRequest.federationName,
            associationRequest.federateId)) {
      return rejected(request);
    }
  }

  ObjectInstanceRegionAssociationScopePlan result;
  if (request.operation == TransportServiceOperation::associate_regions_for_updates) {
    result = registry_.associateRegionsForUpdatesWithScopeChanges(
        associationRequest.federationName,
        associationRequest.federateId,
        associationRequest.objectInstanceHandle,
        associationRequest.attributesAndRegions);
  } else if (
      request.operation ==
      TransportServiceOperation::unassociate_regions_for_updates) {
    result = registry_.unassociateRegionsForUpdatesWithScopeChanges(
        associationRequest.federationName,
        associationRequest.federateId,
        associationRequest.objectInstanceHandle,
        associationRequest.attributesAndRegions);
  } else {
    return invalid(request);
  }

  if (result.status == ObjectInstanceRegionAssociationStatus::applied) {
    // Discovery is ordered before the corresponding scope transition, and
    // owner-directed relevance advisories follow the same committed plan.
    if (!enqueueObjectInstanceDiscoveries(
            associationRequest.federationName, std::move(result.discoveries)) ||
        !enqueueObjectInstanceScopeChanges(
            associationRequest.federationName, std::move(result.recipients))) {
      return internalError(request);
    }
    if (!enqueueAttributeRelevanceAdvisories(
            associationRequest.federationName,
            std::move(result.attributeRelevanceAdvisories))) {
      return internalError(request);
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationObjectInstanceRegionAssociationResult(
          ProcessFederationObjectInstanceRegionAssociationResult{result.status}));
}

}  // namespace umbra::detail
