#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <utility>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handleChangeDefaultAttributeTransportationType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.federateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.federateId)) {
      return rejected(request);
    }
  }
  std::set<std::uint64_t> attributeHandles(
      changeRequest.attributeHandles.begin(),
      changeRequest.attributeHandles.end());
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto const status = transportationName
                          ? registry_.changeDefaultAttributeTransportationType(
                                changeRequest.federationName,
                                changeRequest.federateId,
                                changeRequest.objectClassHandle,
                                attributeHandles,
                                *transportationName)
                          : AttributeTransportationTypeDefaultStatus::
                                invalid_transportation_type;
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeDefaultResult(
          ProcessFederationAttributeTransportationTypeDefaultResult{status}));
}

TransportServiceMessage
ProcessFederationService::handleRequestAttributeTransportationTypeChange(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.requestingFederateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto plan = transportationName
                  ? registry_.planAttributeTransportationTypeChange(
                        changeRequest.federationName,
                        changeRequest.requestingFederateId,
                        changeRequest.objectInstanceHandle,
                        std::set<std::uint64_t>(
                            changeRequest.attributeHandles.begin(),
                            changeRequest.attributeHandles.end()),
                        *transportationName)
                  : AttributeTransportationTypeChangePlan{
                        AttributeTransportationTypeChangeStatus::
                            invalid_transportation_type};
  if (plan.status == AttributeTransportationTypeChangeStatus::applied &&
      plan.requestId != 0U) {
    ProcessFederationAttributeTransportationTypeChangeEvent event{
        plan.requestId,
        changeRequest.requestingFederateId,
        plan.objectInstanceHandle,
        std::move(plan.attributeHandles),
        std::move(plan.transportationName)};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event),
                      std::nullopt})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        registry_.cancelAttributeTransportationTypeChange(
            changeRequest.federationName,
            changeRequest.requestingFederateId,
            plan.requestId);
        return internalError(request);
      }
      state->second.attributeTransportationTypeChangeEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeChangeResult(
          ProcessFederationAttributeTransportationTypeChangeResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryAttributeTransportationType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const queryRequest =
      decodeProcessFederationQueryAttributeTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName, queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const plan = registry_.planAttributeTransportationTypeQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.objectInstanceHandle,
      queryRequest.attributeHandle);
  if (plan.status == AttributeTransportationTypeQueryStatus::applied) {
    ProcessFederationAttributeTransportationTypeQueryEvent event{
        queryRequest.requestingFederateId,
        plan.objectInstanceHandle,
        plan.attributeHandle,
        plan.transportationName};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event)})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        return internalError(request);
      }
      state->second.attributeTransportationTypeQueryEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationAttributeTransportationTypeQueryResult(
      ProcessFederationAttributeTransportationTypeQueryResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleRequestInteractionTransportationTypeChange(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const changeRequest =
      decodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != changeRequest.federationName ||
        state->second.federateId != changeRequest.requestingFederateId ||
        !registry_.memberById(
            changeRequest.federationName, changeRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const transportationName = registry_.transportationTypeNameFor(
      changeRequest.federationName,
      changeRequest.transportationTypeHandle);
  auto plan = transportationName
                  ? registry_.planInteractionTransportationTypeChange(
                        changeRequest.federationName,
                        changeRequest.requestingFederateId,
                        changeRequest.interactionClassHandle,
                        *transportationName)
                  : InteractionTransportationTypeChangePlan{
                        InteractionTransportationTypeChangeStatus::
                            invalid_transportation_type};
  if (plan.status == InteractionTransportationTypeChangeStatus::applied) {
    ProcessFederationInteractionTransportationTypeChangeEvent event{
        changeRequest.requestingFederateId,
        plan.interactionClassHandle,
        std::move(plan.transportationName)};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event),
                      std::nullopt})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        registry_.cancelInteractionTransportationTypeChange(
            changeRequest.federationName,
            changeRequest.requestingFederateId,
            plan.interactionClassHandle);
        return internalError(request);
      }
      state->second.interactionTransportationTypeChangeEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionTransportationTypeChangeResult(
          ProcessFederationInteractionTransportationTypeChangeResult{
              plan.status}));
}

TransportServiceMessage
ProcessFederationService::handleQueryInteractionTransportationType(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  auto const queryRequest =
      decodeProcessFederationQueryInteractionTransportationTypeRequest(
          request.payload);
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != queryRequest.federationName ||
        state->second.federateId != queryRequest.requestingFederateId ||
        !registry_.memberById(
            queryRequest.federationName, queryRequest.requestingFederateId)) {
      return rejected(request);
    }
  }
  auto const plan = registry_.planInteractionTransportationTypeQuery(
      queryRequest.federationName,
      queryRequest.requestingFederateId,
      queryRequest.queriedFederateId,
      queryRequest.interactionClassHandle);
  if (plan.status == InteractionTransportationTypeQueryStatus::applied) {
    ProcessFederationInteractionTransportationTypeQueryEvent event{
        queryRequest.requestingFederateId,
        plan.queriedFederateId,
        plan.interactionClassHandle,
        plan.transportationName};
    if (options_.pushReceiveOrderEvents) {
      if (!session.send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::nullopt,
                      std::move(event)})})) {
        return internalError(request);
      }
    } else {
      std::scoped_lock lock(mutex_);
      auto const state = sessions_.find(&session);
      if (state == sessions_.end()) {
        return internalError(request);
      }
      state->second.interactionTransportationTypeQueryEvents.push_back(
          std::move(event));
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationInteractionTransportationTypeQueryResult(
          ProcessFederationInteractionTransportationTypeQueryResult{
              plan.status}));
}

}  // namespace umbra::detail
