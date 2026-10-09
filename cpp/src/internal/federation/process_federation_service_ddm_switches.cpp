#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <optional>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleGetAttributeScopeAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.attributeScopeAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetAttributeScopeAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAttributeScopeAdvisorySwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetAttributeRelevanceAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.attributeRelevanceAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetObjectClassRelevanceAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.objectClassRelevanceAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetInteractionRelevanceAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.interactionRelevanceAdvisorySwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetAttributeRelevanceAdvisorySwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setAttributeRelevanceAdvisorySwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetConveyRegionDesignatorSetsSwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.conveyRegionDesignatorSetsSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleGetAllowRelaxedDDMSwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const switchValue = registry_.allowRelaxedDDMSwitchFor(
      switchRequest.federationName, switchRequest.federateId);
  if (!switchValue) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{*switchValue}));
}

TransportServiceMessage
ProcessFederationService::handleSetConveyRegionDesignatorSetsSwitch(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const switchRequest =
      decodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != switchRequest.federationName ||
        state->second.federateId != switchRequest.federateId ||
        !registry_.memberById(
            switchRequest.federationName, switchRequest.federateId)) {
      return rejected(request);
    }
  }
  auto const status = registry_.setConveyRegionDesignatorSetsSwitch(
      switchRequest.federationName,
      switchRequest.federateId,
      switchRequest.switchValue);
  if (status != FederationRegistryStatus::applied) {
    return rejected(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationBooleanResult(
          ProcessFederationBooleanResult{switchRequest.switchValue}));
}

}  // namespace umbra::detail
