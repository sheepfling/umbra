#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/federate_handle.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace umbra::detail {
using process_federation_payload::parameterVector;

bool ProcessFederationService::dispatchConnectionLossResult(
    std::wstring const& federationName,
    FederationRegistryResult result,
    std::optional<std::uint64_t> departedFederateId,
    std::optional<FederateLostReportPlan> federateLostReport,
    std::wstring faultDescription) {
  if (departedFederateId) {
    std::scoped_lock lock(mutex_);
    for (auto& [session, state] : sessions_) {
      static_cast<void>(session);
      if (!state.federationName || *state.federationName != federationName) {
        continue;
      }
      state.attributeOwnershipAcquisitionEvents.erase(
          std::remove_if(
              state.attributeOwnershipAcquisitionEvents.begin(),
              state.attributeOwnershipAcquisitionEvents.end(),
              [departedFederateId](
                  ProcessFederationAttributeOwnershipAcquisitionEvent const& event) {
                return event.requestingFederateId == *departedFederateId ||
                    event.receivingFederateId == *departedFederateId;
              }),
          state.attributeOwnershipAcquisitionEvents.end());
    }
  }
  if (!result.resignOwnershipAcquisitionWorkItems.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          federationName,
          std::move(result.resignOwnershipAcquisitionWorkItems))) {
    return false;
  }
  if (!result.resignOwnershipAssumptions.empty()) {
    std::vector<AttributeOwnershipAssumptionRecipient> assumptions;
    assumptions.reserve(result.resignOwnershipAssumptions.size());
    for (auto& assumption : result.resignOwnershipAssumptions) {
      assumptions.push_back({
          assumption.receivingFederateId,
          assumption.objectInstanceHandle,
          std::move(assumption.attributeHandles),
          std::move(assumption.callbackRoute)});
    }
    if (!enqueueAttributeOwnershipAssumptionRecipients(
            federationName,
            std::move(assumptions))) {
      return false;
    }
  }
  if (!result.resignObjectRemovals.empty()) {
    std::vector<ObjectInstanceRemovalRecipient> removals;
    removals.reserve(result.resignObjectRemovals.size());
    for (auto& removal : result.resignObjectRemovals) {
      removals.push_back({
          removal.receivingFederateId,
          removal.objectInstanceHandle,
          std::move(removal.callbackRoute),
          std::move(removal.serviceReportRoute),
          removal.rtiOwnedMomObject});
    }
    if (!enqueueObjectInstanceRemovals(
            federationName,
            std::move(removals),
            {})) {
      return false;
    }
  }
  if (!result.synchronizationNotifications.empty() &&
      !enqueueFederationSynchronizedNotifications(
          federationName,
          std::move(result.synchronizationNotifications))) {
    return false;
  }

  if (federateLostReport &&
      federateLostReport->status == FederateLostReportStatus::applied &&
      federateLostReport->reportedFederateId != 0U &&
      federateLostReport->lastKnownTime &&
      federateLostReport->routing.interactionClassHandle != 0U &&
      federateLostReport->routing.federateParameterHandle != 0U &&
      federateLostReport->routing.federateNameParameterHandle != 0U &&
      federateLostReport->routing.timestampParameterHandle != 0U &&
      federateLostReport->routing.faultDescriptionParameterHandle != 0U &&
      !federateLostReport->recipients.empty()) {
    auto const payload = encodeProcessFederationInteractionEnvelope({
        {{federateLostReport->routing.federateParameterHandle,
          variable_length_data_2025::copyBytes(
              rti1516_2025::umbra_binding_detail::makeFederateHandle(
                          federateLostReport->reportedFederateId)
                          .encode())},
         {federateLostReport->routing.federateNameParameterHandle,
          variable_length_data_2025::copyBytes(rti1516_2025::HLAunicodeString{
                          federateLostReport->reportedFederateName}
                          .encode())},
         {federateLostReport->routing.timestampParameterHandle,
          variable_length_data_2025::copyBytes(
              federateLostReport->lastKnownTime->encode())},
         {federateLostReport->routing.faultDescriptionParameterHandle,
          variable_length_data_2025::copyBytes(
              rti1516_2025::HLAunicodeString{faultDescription}.encode())}},
        {}});
    std::vector<std::pair<ProcessTransportSession*,
                          ProcessFederationInteractionEvent>> pushedEvents;
    {
      std::scoped_lock lock(mutex_);
      for (auto const& recipient : federateLostReport->recipients) {
        auto const receivingSession =
            sessionsByFederateId_.find(recipient.federateId);
        if (receivingSession == sessionsByFederateId_.end()) {
          continue;
        }
        auto const state = sessions_.find(receivingSession->second);
        if (state == sessions_.end() ||
            state->second.federationName != federationName) {
          continue;
        }
        ProcessFederationInteractionEvent event;
        event.receivingFederateId = recipient.federateId;
        event.interactionClassHandle =
            recipient.receivedInteractionClassHandle;
        event.parameterHandles =
            parameterVector(recipient.receivedParameterHandles);
        event.payload = payload;
        event.transportationName = hla::utf8::mom::reliable;
        event.rtiOwnedMomInteraction = true;
        if (options_.pushReceiveOrderEvents) {
          pushedEvents.emplace_back(receivingSession->second,
                                    std::move(event));
        } else {
          state->second.interactionEvents.push_back(std::move(event));
        }
      }
    }
    for (auto const& [recipient, event] : pushedEvents) {
      if (recipient == nullptr || !recipient->send({
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult({event})})) {
        return false;
      }
    }
  }
  return true;
}

bool ProcessFederationService::connectionLost(
    std::wstring const& federationName,
    std::uint64_t departedFederateId,
    std::wstring faultDescription) {
  auto federateLostReport = registry_.planFederateLostReport(
      federationName, departedFederateId);
  auto result = registry_.connectionLost(federationName, departedFederateId);
  if (result.status != FederationRegistryStatus::applied) {
    return false;
  }
  return dispatchConnectionLossResult(
      federationName,
      std::move(result),
      departedFederateId,
      std::move(federateLostReport),
      std::move(faultDescription));
}

}  // namespace umbra::detail
