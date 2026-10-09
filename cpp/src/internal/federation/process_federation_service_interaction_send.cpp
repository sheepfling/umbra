#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"
#include "internal/federation/process_federation_service_payload_helpers.hpp"
#include "internal/federation/transport_service_protocol.hpp"

#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/federate_handle.hpp"

#include <RTI/Exception.h>
#include <RTI/VariableLengthData.h>
#include <RTI/encoding/BasicDataElements.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace umbra::detail {
using process_federation_payload::parameterVector;

namespace {

std::optional<std::uint32_t> decodeProcessHlaInteger32BE(
    std::vector<std::uint8_t> const& bytes) {
  std::uint32_t encoded = 0U;
  if (!readUnsigned(bytes, 0U, ByteOrder::big, encoded)) {
    return std::nullopt;
  }
  return encoded;
}

std::optional<bool> decodeProcessHlaSwitch(
    std::vector<std::uint8_t> const& bytes) {
  auto const encoded = decodeProcessHlaInteger32BE(bytes);
  if (!encoded || *encoded > 1U) {
    return std::nullopt;
  }
  return *encoded == 1U;
}

std::optional<rti1516_2025::ResignAction> decodeProcessHlaResignAction(
    std::vector<std::uint8_t> const& bytes) {
  auto const encoded = decodeProcessHlaInteger32BE(bytes);
  if (!encoded) {
    return std::nullopt;
  }
  switch (*encoded) {
    case 0U:
      return rti1516_2025::UNCONDITIONALLY_DIVEST_ATTRIBUTES;
    case 1U:
      return rti1516_2025::DELETE_OBJECTS;
    case 2U:
      return rti1516_2025::CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
    case 3U:
      return rti1516_2025::DELETE_OBJECTS_THEN_DIVEST;
    case 4U:
      return rti1516_2025::CANCEL_THEN_DELETE_THEN_DIVEST;
    case 5U:
      return rti1516_2025::NO_ACTION;
    default:
      return std::nullopt;
  }
}

}  // namespace

TransportServiceMessage ProcessFederationService::handleSendInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const sendRequest = decodeProcessFederationSendInteractionRequest(request.payload);
  if (request.operation == TransportServiceOperation::send_interaction_with_regions &&
      !sendRequest.sentRegionHandles.has_value()) {
    return invalid(request);
  }
  if (request.operation == TransportServiceOperation::send_interaction &&
      sendRequest.sentRegionHandles.has_value()) {
    return invalid(request);
  }
  if (sendRequest.timestamp) {
    validateProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != sendRequest.federationName ||
        state->second.federateId != sendRequest.producingFederateId ||
        !registry_.memberById(
            sendRequest.federationName,
            sendRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  // HLAsetSwitches is a predefined Subscribe-only MOM interaction.  It is
  // consumed by the RTI rather than routed through the ordinary interaction
  // planner (which would correctly reject it because no federate subscribes
  // to the adjustment class).  Keep the process endpoint's decoding and
  // mutation boundary equivalent to the embedded public path.
  auto const interactionClassName = registry_.interactionClassNameFor(
      sendRequest.federationName, sendRequest.interactionClassHandle);
  auto const isFederationSetSwitches =
      registry_.interactionClassIsSameOrDescendantOf(
          sendRequest.federationName,
          sendRequest.interactionClassHandle,
          hla::utf8::mom::federation_set_switches);
  if (interactionClassName &&
      (*interactionClassName == hla::utf8::mom::federation_set_switches ||
       (isFederationSetSwitches && *isFederationSetSwitches))) {
    if (sendRequest.timestamp || sendRequest.sentRegionHandles ||
        sendRequest.sentParameterHandles.empty()) {
      return rejected(request);
    }
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope) {
      return rejected(request);
    }

    std::optional<bool> autoProvideSwitchValue;
    for (auto const parameterHandle : sendRequest.sentParameterHandles) {
      auto const value = std::find_if(
          envelope->parameterValues.begin(),
          envelope->parameterValues.end(),
          [parameterHandle](ProcessFederationInteractionParameterValue const& entry) {
            return entry.first == parameterHandle;
          });
      if (value == envelope->parameterValues.end()) {
        return rejected(request);
      }
      auto const parameterName = registry_.parameterNameFor(
          sendRequest.federationName,
          *interactionClassName,
          parameterHandle);
      if (!parameterName) {
        return rejected(request);
      }
      if (*parameterName != hla::utf8::mom::auto_provide) {
        // Compatible MOM subclasses may add parameters; only the predefined
        // federation-wide HLAautoProvide switch is interpreted here.
        continue;
      }
      auto const decoded = decodeProcessHlaSwitch(value->second);
      if (!decoded) {
        return rejected(request);
      }
      autoProvideSwitchValue = *decoded;
    }

    if (autoProvideSwitchValue &&
        registry_.setAutoProvideSwitch(
            sendRequest.federationName,
            sendRequest.producingFederateId,
            *autoProvideSwitchValue) != FederationRegistryStatus::applied) {
      return rejected(request);
    }
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationSendInteractionResult(
            ProcessFederationSendInteractionResult{}));
  }

  auto const isFederateSetSwitches = registry_.interactionClassIsSameOrDescendantOf(
      sendRequest.federationName,
      sendRequest.interactionClassHandle,
      hla::utf8::mom::federate_set_switches);
  if (interactionClassName && isFederateSetSwitches && *isFederateSetSwitches) {
    if (sendRequest.timestamp || sendRequest.sentRegionHandles) {
      return rejected(request);
    }

    auto emitMomExceptionReport = [&](std::wstring exceptionText,
                                      bool parameterError) {
      auto const report = registry_.planMomExceptionReport(
          sendRequest.federationName,
          sendRequest.producingFederateId);
      if (report.status != MomExceptionReportStatus::applied) {
        return true;
      }

      auto const reportPayload = encodeProcessFederationInteractionEnvelope(
          ProcessFederationInteractionEnvelope{
              {{report.routing.federateParameterHandle,
                variable_length_data_2025::copyBytes(
                    rti1516_2025::umbra_binding_detail::makeFederateHandle(
                                report.reportedFederateId).encode())},
               {report.routing.serviceParameterHandle,
                variable_length_data_2025::copyBytes(rti1516_2025::HLAunicodeString{
                    hla::wide::mom::set_switches_federate}
                                .encode())},
               {report.routing.exceptionParameterHandle,
                variable_length_data_2025::copyBytes(
                    rti1516_2025::HLAunicodeString{exceptionText}
                                .encode())},
               {report.routing.parameterErrorParameterHandle,
                variable_length_data_2025::copyBytes(
                    rti1516_2025::HLAboolean{parameterError}.encode())}},
              {}});
      std::vector<std::pair<ProcessTransportSession*,
                            ProcessFederationInteractionEvent>>
          pushedEvents;
      {
        std::scoped_lock lock(mutex_);
        for (auto const& recipient : report.recipients) {
          auto const receivingSession =
              sessionsByFederateId_.find(recipient.federateId);
          if (receivingSession == sessionsByFederateId_.end()) {
            continue;
          }
          auto const state = sessions_.find(receivingSession->second);
          if (state == sessions_.end()) {
            continue;
          }
          ProcessFederationInteractionEvent event;
          event.receivingFederateId = recipient.federateId;
          event.interactionClassHandle =
              recipient.receivedInteractionClassHandle;
          event.parameterHandles =
              parameterVector(recipient.receivedParameterHandles);
          event.payload = reportPayload;
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
      if (options_.pushReceiveOrderEvents) {
        for (auto const& pushedEvent : pushedEvents) {
          auto const eventMessage = TransportServiceMessage{
              TransportServiceMessageKind::event,
              TransportServiceOperation::receive_interaction,
              TransportServiceStatus::ok,
              0U,
              encodeProcessFederationReceiveInteractionResult(
                  ProcessFederationReceiveInteractionResult{
                      pushedEvent.second})};
          if (pushedEvent.first == nullptr ||
              !pushedEvent.first->send(eventMessage)) {
            return false;
          }
        }
      }
      return true;
    };

    auto rejectMalformed = [&](std::wstring exceptionText) {
      if (!emitMomExceptionReport(std::move(exceptionText), true)) {
        return internalError(request);
      }
      return rejected(request);
    };

    if (sendRequest.sentParameterHandles.empty()) {
      return rejectMalformed(
          L"InteractionParameterNotDefined: HLAsetSwitches requires at least one declared parameter.");
    }
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope) {
      return rejectMalformed(
          L"InteractionParameterNotDefined: HLAsetSwitches interaction payload is malformed.");
    }

    FederateMOMSwitchUpdate update;
    bool predefinedParameterSupplied = false;
    for (auto const parameterHandle : sendRequest.sentParameterHandles) {
      auto const value = std::find_if(
          envelope->parameterValues.begin(),
          envelope->parameterValues.end(),
          [parameterHandle](ProcessFederationInteractionParameterValue const& entry) {
            return entry.first == parameterHandle;
          });
      if (value == envelope->parameterValues.end()) {
        return rejectMalformed(
            L"InteractionParameterNotDefined: HLAsetSwitches parameter value is missing.");
      }
      auto const parameterName = registry_.parameterNameFor(
          sendRequest.federationName,
          *interactionClassName,
          parameterHandle);
      if (!parameterName) {
        return rejectMalformed(
            L"InteractionParameterNotDefined: HLAsetSwitches parameter is not defined.");
      }
      if (*parameterName == hla::utf8::mom::automatic_resign_action) {
        auto const decoded = decodeProcessHlaResignAction(value->second);
        if (!decoded) {
          return rejectMalformed(
              L"InteractionParameterNotDefined: HLAsetSwitches HLAresignAction value is malformed.");
        }
        update.automaticResignAction = *decoded;
        predefinedParameterSupplied = true;
        continue;
      }
      if (*parameterName != hla::utf8::mom::object_class_relevance_advisory &&
          *parameterName != hla::utf8::mom::attribute_relevance_advisory &&
          *parameterName != hla::utf8::mom::attribute_scope_advisory &&
          *parameterName != hla::utf8::mom::interaction_relevance_advisory &&
          *parameterName != hla::utf8::mom::convey_region_designator_sets &&
          *parameterName != hla::utf8::mom::service_reporting &&
          *parameterName != hla::utf8::mom::exception_reporting &&
          *parameterName != hla::utf8::mom::send_service_reports_to_file) {
        // A compatible predefined-interaction subclass may carry extension
        // parameters.  Receive them, but process only the standard subset.
        continue;
      }
      auto const decoded = decodeProcessHlaSwitch(value->second);
      if (!decoded) {
        return rejectMalformed(
            L"InteractionParameterNotDefined: HLAsetSwitches switch value is malformed.");
      }
      predefinedParameterSupplied = true;
      if (*parameterName == hla::utf8::mom::object_class_relevance_advisory) {
        update.objectClassRelevanceAdvisory = *decoded;
      } else if (*parameterName == hla::utf8::mom::attribute_relevance_advisory) {
        update.attributeRelevanceAdvisory = *decoded;
      } else if (*parameterName == hla::utf8::mom::attribute_scope_advisory) {
        update.attributeScopeAdvisory = *decoded;
      } else if (*parameterName == hla::utf8::mom::interaction_relevance_advisory) {
        update.interactionRelevanceAdvisory = *decoded;
      } else if (*parameterName == hla::utf8::mom::convey_region_designator_sets) {
        update.conveyRegionDesignatorSets = *decoded;
      } else if (*parameterName == hla::utf8::mom::service_reporting) {
        update.serviceReporting = *decoded;
      } else if (*parameterName == hla::utf8::mom::exception_reporting) {
        update.exceptionReporting = *decoded;
      } else if (*parameterName == hla::utf8::mom::send_service_reports_to_file) {
        update.sendServiceReportsToFile = *decoded;
      }
    }

    if (!predefinedParameterSupplied) {
      return rejectMalformed(
          L"InteractionParameterNotDefined: HLAsetSwitches requires at least one predefined parameter.");
    }

    auto const status = registry_.applyFederateMOMSwitchUpdate(
        sendRequest.federationName,
        sendRequest.producingFederateId,
        update);
    if (status != FederateMOMSwitchUpdateStatus::applied) {
      if (status ==
          FederateMOMSwitchUpdateStatus::report_service_invocations_are_subscribed) {
        // §11.5.1 requires a distinct RTI-originated HLAreportMOMexception
        // interaction for this well-formed-but-rejected MOM request.  The
        // process endpoint has no local callback route, so project the same
        // planned recipient set into its interaction event seam.  The public
        // client drains pushed events before it observes this rejected
        // response, preserving the original RTIinternalError at the API.
        if (!emitMomExceptionReport(
                L"RTIinternalError: HLAsetSwitches cannot enable Service Reporting while report-service invocations are subscribed.",
                false)) {
          return internalError(request);
        }
      }
      return rejected(request);
    }
    if (update.exceptionReporting.has_value() &&
        !enqueueJoinedFederateMomConditionalAttributeUpdate(
            sendRequest.federationName,
            sendRequest.producingFederateId,
            {std::string(hla::utf8::mom::exception_reporting)})) {
      return internalError(request);
    }
    return responseFor(
        request,
        TransportServiceStatus::ok,
        encodeProcessFederationSendInteractionResult(
            ProcessFederationSendInteractionResult{}));
  }

  auto const plan = registry_.planReceiveOrderInteraction(
      sendRequest.federationName,
      sendRequest.producingFederateId,
      sendRequest.interactionClassHandle,
      sendRequest.sentParameterHandles,
      sendRequest.sentRegionHandles
          ? &*sendRequest.sentRegionHandles
          : nullptr);
  if (plan.status != ReceiveOrderInteractionStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool queueTimestampedInteraction =
      sendRequest.timestamp && producingTimeSnapshot.timeRegulating &&
      plan.preferredOrderType == rti1516_2025::TIMESTAMP;
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedInteraction) {
    auto const execution = registry_.timeSnapshotFor(sendRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedInteraction) {
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope && !sendRequest.sentParameterHandles.empty()) {
      return rejected(request);
    }

    TsoInteractionMessage message;
    message.producingFederateId = sendRequest.producingFederateId;
    message.sentInteractionClassHandle = sendRequest.interactionClassHandle;
    message.sentParameterHandles = sendRequest.sentParameterHandles;
    message.transportationName = plan.transportationName;
    if (sendRequest.sentRegionHandles) {
      message.sentRegionHandles = *sendRequest.sentRegionHandles;
    }
    message.sentRegionSnapshots = plan.sentRegionSnapshots;
    message.defaultRegionUsed = plan.defaultRegionUsed;
    message.timestamp = decodeProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    message.sentOrderType = rti1516_2025::TIMESTAMP;
    message.receivedOrderType = rti1516_2025::TIMESTAMP;
    if (envelope) {
      if (!envelope->userSuppliedTag.empty()) {
        message.userSuppliedTag.setData(
            envelope->userSuppliedTag.data(),
            envelope->userSuppliedTag.size());
      }
      for (std::uint64_t const parameterHandle : sendRequest.sentParameterHandles) {
        auto const found = std::find_if(
            envelope->parameterValues.begin(),
            envelope->parameterValues.end(),
            [parameterHandle](
                ProcessFederationInteractionParameterValue const& value) {
              return value.first == parameterHandle;
            });
        if (found == envelope->parameterValues.end()) {
          return rejected(request);
        }
        rti1516_2025::VariableLengthData value;
        if (!found->second.empty()) {
          value.setData(found->second.data(), found->second.size());
        }
        message.parameters.emplace_back(parameterHandle, std::move(value));
      }
    } else {
      message.userSuppliedTag.setData(
          sendRequest.payload.data(),
          sendRequest.payload.size());
    }

    std::vector<std::uint64_t> queuedRecipients;
    std::vector<std::uint64_t> allTimestampedRecipients;
    queuedRecipients.reserve(plan.recipients.size());
    allTimestampedRecipients.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      allTimestampedRecipients.push_back(recipient.federateId);
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        queuedRecipients.push_back(recipient.federateId);
      }
    }
    auto const result = registry_.enqueueTsoInteraction(
        sendRequest.federationName,
        std::move(message),
        queuedRecipients,
        allTimestampedRecipients);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(
          messageId,
          sendRequest.producingFederateId);
    }
  }

  std::vector<ProcessFederationInteractionEvent> events;
  events.reserve(plan.recipients.size());
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& recipient : plan.recipients) {
    if (queueTimestampedInteraction &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++queuedRecipientCount;
      }
      continue;
    }
    auto resolvedRecipient = recipient;
    if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
      auto const current = registry_.receiveOrderInteractionRecipientFor(
          sendRequest.federationName,
          sendRequest.producingFederateId,
          recipient.federateId,
          sendRequest.interactionClassHandle,
          sendRequest.sentParameterHandles,
          sendRequest.sentRegionHandles
              ? &*sendRequest.sentRegionHandles
              : nullptr,
          plan.sentRegionSnapshots.empty()
              ? nullptr
              : &plan.sentRegionSnapshots);
      if (!current) {
        continue;
      }
      resolvedRecipient = *current;
    }
    if (resolvedRecipient.callbackRoute) {
      resolvedRecipient.callbackRoute.enqueueReceiveOrder(
          [](rti1516_2025::FederateAmbassador&) {});
    }
    ProcessFederationInteractionEvent event{
        sendRequest.producingFederateId,
        resolvedRecipient.federateId,
        resolvedRecipient.receivedInteractionClassHandle,
        parameterVector(resolvedRecipient.receivedParameterHandles),
        sendRequest.payload,
        plan.transportationName,
        sendRequest.timestamp,
        std::nullopt,
        messageId == 0U ? std::nullopt
                        : std::optional<std::uint64_t>(messageId)};
    event.defaultRegionUsed =
        resolvedRecipient.conveyRegionDesignatorSets && plan.defaultRegionUsed;
    if (resolvedRecipient.conveyRegionDesignatorSets &&
        (sendRequest.sentRegionHandles.has_value() || plan.defaultRegionUsed)) {
      event.sentRegionHandles = sendRequest.sentRegionHandles;
    }
    events.push_back(std::move(event));
  }

  std::uint32_t recipientCount = 0U;
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
      auto const receivingSession = sessionsByFederateId_.find(event.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end()) {
        continue;
      }
      if (options_.pushReceiveOrderEvents) {
        // The push path is exactly-once at this service boundary: the event
        // frame is the receiver's delivery, so do not also leave a duplicate
        // copy in the legacy polling queue.  The callback bridge owns
        // conversion to official C++ handle/value types.
        pushedEvents.emplace_back(receivingSession->second, event);
      } else {
        // The default service mode retains the original deterministic polling
        // seam used by the focused registry/service unit tests.
        state->second.interactionEvents.push_back(std::move(event));
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (queueTimestampedInteraction) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }

  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_interaction,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveInteractionResult(
              ProcessFederationReceiveInteractionResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
      if (pushedEvent.second.retractionMessageId) {
        std::scoped_lock lock(mutex_);
        pendingPushedRetractionRecipients_[
            *pushedEvent.second.retractionMessageId]
            .push_back(pushedEvent.first);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSendInteractionResult(
          ProcessFederationSendInteractionResult{recipientCount, messageId}));
}

TransportServiceMessage ProcessFederationService::handleSendInteractionWithRegions(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  // The regional form uses the same validated interaction envelope and
  // recipient fan-out as ordinary Send Interaction.  The operation-specific
  // guard in handleSendInteraction requires an engaged region set, keeping
  // omitted regions distinguishable from the standard overload.
  return handleSendInteraction(session, request);
}

TransportServiceMessage ProcessFederationService::handleSendDirectedInteraction(
    ProcessTransportSession& session,
    TransportServiceMessage const& request) {
  auto const sendRequest =
      decodeProcessFederationSendDirectedInteractionRequest(request.payload);
  if (sendRequest.timestamp) {
    validateProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
  }
  std::shared_ptr<FederateTimeState> producingTimeState;
  {
    std::scoped_lock lock(mutex_);
    auto const state = sessions_.find(&session);
    if (state == sessions_.end() ||
        !state->second.federationName.has_value() ||
        *state->second.federationName != sendRequest.federationName ||
        state->second.federateId != sendRequest.producingFederateId ||
        !registry_.memberById(
            sendRequest.federationName,
             sendRequest.producingFederateId)) {
      return rejected(request);
    }
    producingTimeState = state->second.timeState;
  }

  auto const plan = registry_.planReceiveOrderDirectedInteraction(
      sendRequest.federationName,
      sendRequest.producingFederateId,
      sendRequest.objectInstanceHandle,
      sendRequest.interactionClassHandle,
      sendRequest.sentParameterHandles);
  if (plan.status != ReceiveOrderDirectedInteractionStatus::applied) {
    return rejected(request);
  }

  auto const producingTimeSnapshot = producingTimeState
      ? producingTimeState->snapshot()
      : FederateTimeSnapshot{};
  bool const queueTimestampedDirectedInteraction =
      sendRequest.timestamp && producingTimeSnapshot.timeRegulating;
  std::set<std::uint64_t> timeConstrainedRecipients;
  if (queueTimestampedDirectedInteraction) {
    auto const execution = registry_.timeSnapshotFor(sendRequest.federationName);
    if (!execution) {
      return internalError(request);
    }
    for (auto const& federate : execution->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::vector<std::uint64_t> queuedRecipientIds;
  queuedRecipientIds.reserve(plan.recipients.size());
  if (queueTimestampedDirectedInteraction) {
    for (auto const& recipient : plan.recipients) {
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        queuedRecipientIds.push_back(recipient.federateId);
      }
    }
  }

  std::uint64_t messageId = 0U;
  if (queueTimestampedDirectedInteraction && !plan.recipients.empty()) {
    auto timestamp = decodeProcessLogicalTime(
        *sendRequest.timestamp,
        federationDefinition_->logicalTimeImplementationName);
    auto const envelope =
        decodeProcessFederationInteractionEnvelope(sendRequest.payload);
    if (!envelope && !sendRequest.sentParameterHandles.empty()) {
      return rejected(request);
    }

    TsoDirectedInteractionMessage message;
    message.producingFederateId = sendRequest.producingFederateId;
    message.objectInstanceHandle = sendRequest.objectInstanceHandle;
    message.sentInteractionClassHandle = sendRequest.interactionClassHandle;
    message.sentParameterHandles = sendRequest.sentParameterHandles;
    message.transportationName = plan.transportationName;
    message.timestamp = std::move(timestamp);
    message.sentOrderType = rti1516_2025::TIMESTAMP;
    message.receivedOrderType = rti1516_2025::RECEIVE;
    if (envelope) {
      message.userSuppliedTag.setData(
          envelope->userSuppliedTag.data(), envelope->userSuppliedTag.size());
      for (std::uint64_t const parameterHandle : sendRequest.sentParameterHandles) {
        auto const found = std::find_if(
            envelope->parameterValues.begin(),
            envelope->parameterValues.end(),
            [parameterHandle](ProcessFederationInteractionParameterValue const& value) {
              return value.first == parameterHandle;
            });
        if (found == envelope->parameterValues.end()) {
          return rejected(request);
        }
        rti1516_2025::VariableLengthData value;
        value.setData(found->second.data(), found->second.size());
        message.parameters.emplace_back(parameterHandle, std::move(value));
      }
    } else {
      message.userSuppliedTag.setData(
          sendRequest.payload.data(), sendRequest.payload.size());
    }

    InteractionCallbackRoute processRoute;
    processRoute.submit = [](FederateCallbackInvocation) {};
    for (auto const& recipient : plan.recipients) {
      auto resolvedRecipient = recipient;
      if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
        auto const current = registry_.receiveOrderDirectedInteractionRecipientFor(
            sendRequest.federationName,
            sendRequest.producingFederateId,
            recipient.federateId,
            sendRequest.objectInstanceHandle,
            sendRequest.interactionClassHandle,
            sendRequest.sentParameterHandles);
        if (!current) {
          continue;
        }
        resolvedRecipient = *current;
      }
      message.recipients.push_back({
          resolvedRecipient.federateId,
          sendRequest.objectInstanceHandle,
          resolvedRecipient.receivedInteractionClassHandle,
          resolvedRecipient.receivedParameterHandles,
          processRoute});
    }
    if (message.recipients.empty()) {
      return responseFor(
          request,
          TransportServiceStatus::ok,
          encodeProcessFederationSendInteractionResult(
              ProcessFederationSendInteractionResult{}));
    }
    auto const result = registry_.enqueueTsoDirectedInteraction(
        sendRequest.federationName,
        std::move(message),
        queuedRecipientIds);
    if (result.status != FederationTsoRegistryStatus::applied ||
        result.queueStatus != TsoMessageQueueStatus::applied ||
        result.messageId == 0U) {
      return rejected(request);
    }
    messageId = result.messageId;
    {
      std::scoped_lock lock(mutex_);
      processTsoMessageProducers_.emplace(messageId, sendRequest.producingFederateId);
    }
  }

  std::vector<ProcessFederationInteractionEvent> events;
  events.reserve(plan.recipients.size());
  std::uint32_t queuedRecipientCount = 0U;
  for (auto const& recipient : plan.recipients) {
    if (queueTimestampedDirectedInteraction &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      if (queuedRecipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++queuedRecipientCount;
      }
      continue;
    }
    auto resolvedRecipient = recipient;
    if (resolvedRecipient.receivedInteractionClassHandle == 0U) {
      auto const current = registry_.receiveOrderDirectedInteractionRecipientFor(
          sendRequest.federationName,
          sendRequest.producingFederateId,
          recipient.federateId,
          sendRequest.objectInstanceHandle,
          sendRequest.interactionClassHandle,
          sendRequest.sentParameterHandles);
      if (!current) {
        continue;
      }
      resolvedRecipient = *current;
    }
    events.push_back(ProcessFederationInteractionEvent{
        sendRequest.producingFederateId,
        resolvedRecipient.federateId,
        resolvedRecipient.receivedInteractionClassHandle,
        parameterVector(resolvedRecipient.receivedParameterHandles),
        sendRequest.payload,
        plan.transportationName,
        sendRequest.timestamp,
        sendRequest.objectInstanceHandle,
        messageId == 0U ? std::nullopt
                        : std::optional<std::uint64_t>(messageId)});
  }

  if (messageId != 0U && options_.pushReceiveOrderEvents) {
    // The pushed frame is the process callback boundary for a timestamped
    // directed interaction. Establish the recipient reservation before the
    // frame is exposed so the receiver's post-callback acknowledgement makes
    // the producer's retraction designator terminal, just as the pull path
    // does in handleReceiveInteraction.
    events.erase(
        std::remove_if(
            events.begin(),
            events.end(),
            [&](ProcessFederationInteractionEvent const& event) {
              return !registry_.beginTsoInteractionCallback(
                  sendRequest.federationName,
                  event.receivingFederateId,
                  messageId);
            }),
        events.end());
  }

  std::uint32_t recipientCount = 0U;
  std::vector<std::pair<ProcessTransportSession*, ProcessFederationInteractionEvent>>
      pushedEvents;
  std::vector<ProcessTransportSession*> retractionRecipients;
  {
    std::scoped_lock lock(mutex_);
    for (auto& event : events) {
      auto const receivingSession = sessionsByFederateId_.find(event.receivingFederateId);
      if (receivingSession == sessionsByFederateId_.end()) {
        continue;
      }
      auto const state = sessions_.find(receivingSession->second);
      if (state == sessions_.end()) {
        continue;
      }
      if (options_.pushReceiveOrderEvents) {
        pushedEvents.emplace_back(receivingSession->second, event);
      } else {
        state->second.interactionEvents.push_back(std::move(event));
      }
      if (messageId != 0U) {
        retractionRecipients.push_back(receivingSession->second);
      }
      if (recipientCount != std::numeric_limits<std::uint32_t>::max()) {
        ++recipientCount;
      }
    }
  }

  if (queueTimestampedDirectedInteraction) {
    auto const totalCount = static_cast<std::uint64_t>(recipientCount) +
        queuedRecipientCount;
    recipientCount = static_cast<std::uint32_t>(std::min<std::uint64_t>(
        totalCount,
        std::numeric_limits<std::uint32_t>::max()));
  }

  if (messageId != 0U) {
    std::scoped_lock lock(mutex_);
    pendingPushedRetractionRecipients_.emplace(
        messageId, std::move(retractionRecipients));
  }
  if (options_.pushReceiveOrderEvents) {
    for (auto const& pushedEvent : pushedEvents) {
      auto const eventMessage = TransportServiceMessage{
          TransportServiceMessageKind::event,
          TransportServiceOperation::receive_interaction,
          TransportServiceStatus::ok,
          0U,
          encodeProcessFederationReceiveInteractionResult(
              ProcessFederationReceiveInteractionResult{pushedEvent.second})};
      if (pushedEvent.first == nullptr || !pushedEvent.first->send(eventMessage)) {
        return internalError(request);
      }
    }
  }
  return responseFor(
      request,
      TransportServiceStatus::ok,
      encodeProcessFederationSendInteractionResult(
          ProcessFederationSendInteractionResult{recipientCount, messageId}));
}

}  // namespace umbra::detail
