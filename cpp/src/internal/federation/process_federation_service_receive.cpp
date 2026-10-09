#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/dimension_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <limits>
#include <memory>
#include <iterator>
#include <set>
#include <utility>

namespace umbra::detail {
using process_federation_payload::parameterVector;
TransportServiceMessage ProcessFederationService::handleReceiveInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const receiveRequest = decodeProcessFederationReceiveInteractionRequest(
      request.payload);
  ProcessFederationReceiveInteractionResult result;
  std::vector<AttributeOwnershipAcquisitionWorkItem> acquisitionFollowups;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != receiveRequest.federationName ||
        state->second.federateId != receiveRequest.receivingFederateId ||
        !registry_.memberById(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId)) {
      return rejected(request);
    }
    if (!state->second.synchronizationPointAnnouncementEvents.empty()) {
      result.synchronizationPointAnnouncementEvent = std::move(
          state->second.synchronizationPointAnnouncementEvents.front());
      state->second.synchronizationPointAnnouncementEvents.pop_front();
    } else if (!state->second.federationSynchronizedEvents.empty()) {
      result.federationSynchronizedEvent = std::move(
          state->second.federationSynchronizedEvents.front());
      state->second.federationSynchronizedEvents.pop_front();
    } else if (!state->second.saveEvents.empty()) {
      result.saveEvent = std::move(state->second.saveEvents.front());
      state->second.saveEvents.pop_front();
    } else if (!state->second.restoreEvents.empty()) {
      result.restoreEvent = std::move(state->second.restoreEvents.front());
      state->second.restoreEvents.pop_front();
    } else if (!state->second.interactionEvents.empty()) {
      result.event = std::move(state->second.interactionEvents.front());
      state->second.interactionEvents.pop_front();
    } else if (!state->second.objectInstanceDiscoveryEvents.empty() &&
               (state->second.objectInstanceRemovalEvents.empty() ||
                state->second.objectInstanceDiscoveryEvents.front()
                        .callbackOrderSequence <
                    state->second.objectInstanceRemovalEvents.front()
                        .callbackOrderSequence)) {
      // Object discovery is delivered before reflections for a newly
      // registered instance, matching the official callback ordering. When a
      // removal is also pending, the per-session sequence preserves the
      // causal order in which discovery/removal work entered the process queue.
      result.discoveryEvent = std::move(
          state->second.objectInstanceDiscoveryEvents.front());
      state->second.objectInstanceDiscoveryEvents.pop_front();
    } else if (!state->second.objectInstanceRemovalEvents.empty()) {
      // A receive-order deletion reserves the recipient at the producer's
      // service boundary, but the known-instance transition commits only when
      // this recipient crosses its callback boundary. Skip stale reservations
      // (for example after resignation) without manufacturing a callback.
      while (!state->second.objectInstanceRemovalEvents.empty()) {
        auto event = std::move(
            state->second.objectInstanceRemovalEvents.front());
        state->second.objectInstanceRemovalEvents.pop_front();
        auto const snapshot = event.rtiOwnedMomObject
            ? registry_.beginJoinedFederateMomObjectRemoval(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle)
            : event.retractionMessageId
            ? registry_.beginTsoObjectInstanceRemoval(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle,
                  *event.retractionMessageId)
            : registry_.beginObjectInstanceRemoval(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle);
        if (!snapshot) {
          continue;
        }
        event.producingFederateId = snapshot->producingFederateId;
        result.removalEvent = std::move(event);
        break;
      }
    } else {
      // Scope transitions are rechecked at the process delivery boundary so
      // a later region/subscription mutation, switch disable, or resignation
      // cannot leak stale Attributes In/Out Of Scope work.
      while (!state->second.objectInstanceScopeChangeEvents.empty()) {
        auto event = std::move(
            state->second.objectInstanceScopeChangeEvents.front());
        state->second.objectInstanceScopeChangeEvents.pop_front();
        auto const eligible = registry_.objectInstanceScopeAttributes(
            receiveRequest.federationName,
            receiveRequest.receivingFederateId,
            event.objectInstanceHandle,
            event.attributeHandles,
            event.inScope);
        if (eligible.empty()) {
          continue;
        }
        event.attributeHandles = eligible;
        result.scopeChangeEvent = std::move(event);
        break;
      }
      if (!result.scopeChangeEvent &&
          !state->second.attributeRelevanceAdvisoryEvents.empty()) {
        while (!state->second.attributeRelevanceAdvisoryEvents.empty()) {
          auto event = std::move(
              state->second.attributeRelevanceAdvisoryEvents.front());
          state->second.attributeRelevanceAdvisoryEvents.pop_front();
          if (event.providingFederateId != receiveRequest.receivingFederateId) {
            continue;
          }
          auto const eligible = registry_.attributeRelevanceAdvisoryAttributes(
              receiveRequest.federationName,
              event.providingFederateId,
              event.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandles,
              event.turnUpdatesOn);
          if (eligible.empty()) {
            continue;
          }
          event.attributeHandles = eligible;
          if (event.turnUpdatesOn) {
            event.updateRateDesignator =
                registry_.attributeRelevanceAdvisoryUpdateRateDesignatorFor(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    *event.attributeHandles.begin());
          } else {
            event.updateRateDesignator.reset();
          }
          result.attributeRelevanceAdvisoryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !state->second.attributeValueUpdateRequestEvents.empty()) {
        result.attributeValueUpdateRequestEvent = std::move(
            state->second.attributeValueUpdateRequestEvents.front());
        state->second.attributeValueUpdateRequestEvents.pop_front();
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !state->second.attributeUpdateEvents.empty()) {
        // Keep the original receive-interaction polling operation as the
        // single deterministic receive fence. Attribute reflections use the
        // adjacent result slot so older process clients do not need a second
        // competing poll that would consume a response intended for an
        // interaction test server.
        result.attributeEvent =
            std::move(state->second.attributeUpdateEvents.front());
        state->second.attributeUpdateEvents.pop_front();
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent) {
        while (!state->second.attributeTransportationTypeChangeEvents.empty()) {
          auto event = std::move(
              state->second.attributeTransportationTypeChangeEvents.front());
          state->second.attributeTransportationTypeChangeEvents.pop_front();
          auto const delivery = registry_.beginAttributeTransportationTypeChange(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              event.requestId);
          if (!delivery || delivery->attributeHandles.empty() ||
              delivery->transportationName.empty()) {
            continue;
          }
          event.objectInstanceHandle = delivery->objectInstanceHandle;
          event.attributeHandles = std::move(delivery->attributeHandles);
          event.transportationName = std::move(delivery->transportationName);
          result.attributeTransportationTypeChangeEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent) {
        while (!state->second.attributeTransportationTypeQueryEvents.empty()) {
          auto event = std::move(
              state->second.attributeTransportationTypeQueryEvents.front());
          state->second.attributeTransportationTypeQueryEvents.pop_front();
          auto const projection = registry_.attributeTransportationTypeQueryFor(
              receiveRequest.federationName,
              receiveRequest.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandle);
          if (!projection || projection->transportationName.empty()) {
            continue;
          }
          event.objectInstanceHandle = projection->objectInstanceHandle;
          event.attributeHandle = projection->attributeHandle;
          event.transportationName = projection->transportationName;
          result.attributeTransportationTypeQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent) {
        while (!state->second.attributeOwnershipQueryEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipQueryEvents.front());
          state->second.attributeOwnershipQueryEvents.pop_front();
          auto const projection = registry_.attributeOwnershipQueryRecipientFor(
              receiveRequest.federationName,
              event.requestId,
              receiveRequest.receivingFederateId,
              event.objectInstanceHandle,
              event.reportKind,
              event.owningFederateId,
              event.attributeHandles);
          if (!projection) {
            continue;
          }
          event.attributeHandles = projection->attributeHandles;
          if (event.attributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent) {
        while (!state->second.interactionTransportationTypeChangeEvents.empty()) {
          auto event = std::move(
              state->second.interactionTransportationTypeChangeEvents.front());
          state->second.interactionTransportationTypeChangeEvents.pop_front();
          auto const transportationName =
              registry_.beginInteractionTransportationTypeChange(
                  receiveRequest.federationName,
                  event.receivingFederateId,
                  event.interactionClassHandle);
          if (!transportationName || transportationName->empty()) {
            continue;
          }
          event.transportationName = std::move(*transportationName);
          result.interactionTransportationTypeChangeEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.interactionTransportationTypeChangeEvent) {
        while (!state->second.interactionTransportationTypeQueryEvents.empty()) {
          auto event = std::move(
              state->second.interactionTransportationTypeQueryEvents.front());
          state->second.interactionTransportationTypeQueryEvents.pop_front();
          auto const projection = registry_.interactionTransportationTypeQueryFor(
              receiveRequest.federationName,
              event.receivingFederateId,
              event.queriedFederateId,
              event.interactionClassHandle);
          if (!projection || projection->transportationName.empty()) {
            continue;
          }
          event.interactionClassHandle = projection->interactionClassHandle;
          event.transportationName = projection->transportationName;
          result.interactionTransportationTypeQueryEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent) {
        while (!state->second.attributeOwnershipUnavailableEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipUnavailableEvents.front());
          state->second.attributeOwnershipUnavailableEvents.pop_front();
          auto const delivery = registry_.attributeOwnershipUnavailableRecipientFor(
              receiveRequest.federationName,
              event.receivingFederateId,
              event.objectInstanceHandle,
              event.attributeHandles);
          if (!delivery || delivery->attributeHandles.empty()) {
            continue;
          }
          event.attributeHandles = delivery->attributeHandles;
          result.attributeOwnershipUnavailableEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent &&
          !result.attributeOwnershipUnavailableEvent) {
        while (!state->second.attributeOwnershipAcquisitionEvents.empty()) {
          auto event = std::move(
              state->second.attributeOwnershipAcquisitionEvents.front());
          state->second.attributeOwnershipAcquisitionEvents.pop_front();
          if (event.kind ==
              ProcessFederationAttributeOwnershipAcquisitionEventKind::
                  acquisition_notification) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionNotification(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->securedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         request_release) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionRelease(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->candidateAttributeHandles;
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         cancellation_confirmation) {
            auto const delivery =
                registry_.beginAttributeOwnershipAcquisitionCancellation(
                    receiveRequest.federationName,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->confirmedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         request_divestiture_confirmation) {
            auto const delivery =
                registry_.beginRequestDivestitureConfirmation(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.requestingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.candidateIsIfAvailable,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->releasedAttributeHandles;
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         confirm_divestiture_notification) {
            if (event.receivingFederateId != event.requestingFederateId) {
              continue;
            }
            auto const delivery =
                registry_.beginConfirmDivestitureNotification(
                    receiveRequest.federationName,
                    event.receivingFederateId,
                    event.objectInstanceHandle,
                    event.requestId,
                    event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->securedAttributeHandles;
            for (auto& followup : delivery->followupWorkItems) {
              acquisitionFollowups.push_back(std::move(followup));
            }
          } else if (event.kind ==
                     ProcessFederationAttributeOwnershipAcquisitionEventKind::
                         ownership_assumption) {
            auto const delivery = registry_.attributeOwnershipAssumptionDeliveryFor(
                receiveRequest.federationName,
                event.receivingFederateId,
                event.objectInstanceHandle,
                event.attributeHandles);
            if (!delivery) {
              continue;
            }
            event.attributeHandles = delivery->attributeHandles;
          } else {
            continue;
          }
          if (event.attributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipAcquisitionEvent = std::move(event);
          break;
        }
      }
      if (!result.scopeChangeEvent &&
          !result.attributeRelevanceAdvisoryEvent &&
          !result.attributeValueUpdateRequestEvent &&
          !result.attributeEvent &&
          !result.attributeTransportationTypeChangeEvent &&
          !result.attributeTransportationTypeQueryEvent &&
          !result.attributeOwnershipQueryEvent &&
          !result.attributeOwnershipAcquisitionEvent &&
          !result.attributeOwnershipUnavailableEvent) {
        while (!state->second
                    .attributeOwnershipAcquisitionIfAvailableEvents.empty()) {
          auto event = std::move(state->second
                                     .attributeOwnershipAcquisitionIfAvailableEvents
                                     .front());
          state->second.attributeOwnershipAcquisitionIfAvailableEvents.pop_front();
          auto const delivery =
              registry_.beginAttributeOwnershipAcquisitionIfAvailable(
                  receiveRequest.federationName,
                  receiveRequest.receivingFederateId,
                  event.objectInstanceHandle,
                  event.requestId);
          if (!delivery) {
            continue;
          }
          event.securedAttributeHandles = delivery->securedAttributeHandles;
          event.unavailableAttributeHandles =
              delivery->unavailableAttributeHandles;
          if (event.securedAttributeHandles.empty() &&
              event.unavailableAttributeHandles.empty()) {
            continue;
          }
          result.attributeOwnershipAcquisitionIfAvailableEvent =
              std::move(event);
          break;
        }
      }
    }
  }
  if (!acquisitionFollowups.empty() &&
      !enqueueAttributeOwnershipAcquisitionWorkItems(
          receiveRequest.federationName,
          std::move(acquisitionFollowups))) {
    return internalError(request);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationReceiveInteractionResult(result));
}

}  // namespace umbra::detail
