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
namespace {
[[nodiscard]] std::shared_ptr<rti1516_2025::LogicalTime const>
makeProcessTsoRetractionLowerBound(FederateTimeSnapshot const& snapshot) {
  if (!snapshot.timeRegulating || !snapshot.currentTime ||
      !snapshot.lookahead || snapshot.implementationName.empty()) {
    return {};
  }

  auto const* baseTime = snapshot.currentTime.get();
  if (snapshot.timeAdvancePending) {
    if (!snapshot.advanceRequestTime) {
      return {};
    }
    baseTime = snapshot.advanceRequestTime.get();
  }
  if (baseTime == nullptr ||
      baseTime->implementationName() != snapshot.implementationName ||
      snapshot.lookahead->implementationName() != snapshot.implementationName) {
    return {};
  }

  auto factory = rti1516_2025::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
      snapshot.implementationName);
  if (!factory || factory->getName() != snapshot.implementationName) {
    return {};
  }
  std::unique_ptr<rti1516_2025::LogicalTime> lowerBound;
  try {
    lowerBound = factory->decodeLogicalTime(baseTime->encode());
    if (!lowerBound ||
        lowerBound->implementationName() != snapshot.implementationName) {
      return {};
    }
    *lowerBound += *snapshot.lookahead;
  } catch (rti1516_2025::Exception const&) {
    return {};
  }
  std::shared_ptr<rti1516_2025::LogicalTime const> result = std::move(lowerBound);
  return result;
}
}  // namespace
TransportServiceMessage ProcessFederationService::handleRetract(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const retractRequest = decodeProcessFederationRetractRequest(request.payload);
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != retractRequest.federationName ||
        state->second.federateId != retractRequest.producingFederateId ||
        !registry_.memberById(
            retractRequest.federationName, retractRequest.producingFederateId)) {
        return rejected(request);
    }
    producingTimeState = state->second.timeState;
    auto const owner = processTsoMessageProducers_.find(retractRequest.messageId);
    if (owner == processTsoMessageProducers_.end() ||
        owner->second != retractRequest.producingFederateId) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationRetractResult(
              {ProcessFederationRetractStatus::invalid_handle}));
      }
  }

  // A retraction designator remains live while a joined federate temporarily
  // disables time regulation, but the Retract service itself is only legal
  // while the producer is currently time regulating. Keep this check at the
  // process service boundary so the accepted message and its producer-owned
  // identity remain untouched for a later re-enable.  When regulation is
  // active, enforce the strict timestamp-versus-(current/requested time plus
  // actual lookahead) precondition from IEEE 1516.1-2025 clause 8.22.3.
  auto const producingTimeSnapshot =
      producingTimeState ? producingTimeState->snapshot() : FederateTimeSnapshot{};
  if (!producingTimeSnapshot.timeRegulating) {
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationRetractResult(
            {ProcessFederationRetractStatus::time_regulation_not_enabled}));
  }

  auto const retractionLowerBound =
      makeProcessTsoRetractionLowerBound(producingTimeSnapshot);
  if (!retractionLowerBound) {
    return internalError(request);
  }

  auto const result = registry_.retractTsoMessageForProducer(
      retractRequest.federationName,
      retractRequest.producingFederateId,
      retractRequest.messageId,
      retractionLowerBound,
      true);
  ProcessFederationRetractStatus status =
      ProcessFederationRetractStatus::invalid_handle;
  if (result.status == FederationTsoRegistryStatus::federate_not_member) {
    status = ProcessFederationRetractStatus::federate_not_member;
  } else if (result.status == FederationTsoRegistryStatus::applied &&
             !result.timestampEligible) {
    status = ProcessFederationRetractStatus::message_no_longer_retractable;
  } else if (result.status == FederationTsoRegistryStatus::applied) {
    switch (result.queueResult.status) {
      case TsoMessageQueueStatus::applied:
        status = ProcessFederationRetractStatus::applied;
        break;
      case TsoMessageQueueStatus::message_already_retracted:
      case TsoMessageQueueStatus::message_already_delivered:
      case TsoMessageQueueStatus::message_not_found:
        status = ProcessFederationRetractStatus::message_no_longer_retractable;
        break;
      default:
        status = ProcessFederationRetractStatus::invalid_handle;
        break;
    }
  }

  if (status == ProcessFederationRetractStatus::applied) {
    std::vector<ProcessTransportSession*> pushedRecipients;
    {
      std::scoped_lock lock(mutex_);
      // Retain the producer entry until the next Retract observes the
      // terminalized queue state. This lets a repeated use of the same
      // designator report MessageCanNoLongerBeRetracted instead of looking
      // like an unrelated invalid handle.
      for (auto& [ignoredSession, state] : sessions_) {
        state.interactionEvents.erase(
            std::remove_if(
                state.interactionEvents.begin(),
                state.interactionEvents.end(),
                [&retractRequest](ProcessFederationInteractionEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.interactionEvents.end());
        state.objectInstanceRemovalEvents.erase(
            std::remove_if(
                state.objectInstanceRemovalEvents.begin(),
                state.objectInstanceRemovalEvents.end(),
                [&retractRequest](
                    ProcessFederationObjectInstanceRemovalEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.objectInstanceRemovalEvents.end());
        state.attributeUpdateEvents.erase(
            std::remove_if(
                state.attributeUpdateEvents.begin(),
                state.attributeUpdateEvents.end(),
                [&retractRequest](
                    ProcessFederationAttributeUpdateEvent const& event) {
                  return event.retractionMessageId == retractRequest.messageId;
                }),
            state.attributeUpdateEvents.end());
      }
      auto pushed = pendingPushedRetractionRecipients_.find(
          retractRequest.messageId);
      if (pushed != pendingPushedRetractionRecipients_.end()) {
        pushedRecipients = std::move(pushed->second);
        pendingPushedRetractionRecipients_.erase(pushed);
      }
    }
    for (auto* recipient : pushedRecipients) {
      if (recipient == nullptr ||
          !recipient->send(TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::request_retraction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationRequestRetractionEvent(
                  ProcessFederationRequestRetractionEvent{
                      retractRequest.messageId})})) {
        return internalError(request);
      }
    }
  } else if (status == ProcessFederationRetractStatus::message_no_longer_retractable) {
    std::scoped_lock lock(mutex_);
    processTsoMessageProducers_.erase(retractRequest.messageId);
    pendingPushedRetractionRecipients_.erase(retractRequest.messageId);
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationRetractResult({status}));
}


}  // namespace umbra::detail
