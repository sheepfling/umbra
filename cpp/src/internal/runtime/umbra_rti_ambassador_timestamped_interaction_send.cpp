#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/time/reference_time_selection.hpp"
#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>
#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;
using InteractionParameterValue = AmbassadorInteractionParameterValue;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
MessageRetractionHandle UmbraRtiAmbassador::sendInteraction(
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("sendInteraction");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The process endpoint currently provides timestamp transport and callback
  // reconstruction, but not federation-wide time regulation or retraction.
  // Keep this bounded slice ahead of the embedded-only service path instead of
  // consulting the local registry (which is not the process federation).
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Timestamped Send Interaction requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    }

    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InteractionClassNotDefined(
          L"Timestamped Send Interaction requires a defined InteractionClassHandle.");
    }
    auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
    if (!parameterHandles) {
      throw InteractionParameterNotDefined(
          L"Timestamped Send Interaction requires defined ParameterHandle values.");
    }

    // Reconstruct once through the official factory so malformed, initial, or
    // final values fail with the public InvalidLogicalTime contract before the
    // private request is sent.  The server independently validates its own
    // federation implementation name at the service boundary.
    auto timestamp = cloneAmbassadorReferenceLogicalTime(time.implementationName(), time);
    if (timestamp->isInitial() || timestamp->isFinal()) {
      throw InvalidLogicalTime(
          L"A timestamped service requires a finite logical timestamp.");
    }
    auto const encodedTimestamp = timestamp->encode();
    auto processTimestampBytes =
        umbra::detail::variable_length_data_2025::copyBytes(encodedTimestamp);

    std::vector<umbra::detail::ProcessFederationInteractionParameterValue>
        processParameterValues;
    auto const sentParameters = ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedTag(userSuppliedTag);
    ParameterHandleValueMap reportParameterValues;
    processParameterValues.reserve(sentParameters.size());
    for (auto const& [parameterHandle, parameterValue] : sentParameters) {
      reportParameterValues.emplace(
          makeParameterHandle(parameterHandle), parameterValue);
      auto bytes = umbra::detail::variable_length_data_2025::copyBytes(parameterValue);
      processParameterValues.emplace_back(parameterHandle, std::move(bytes));
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);

    std::uint64_t processMessageId = 0U;
    try {
      auto const processPayload = umbra::detail::encodeProcessFederationInteractionEnvelope(
          umbra::detail::ProcessFederationInteractionEnvelope{
              std::move(processParameterValues), std::move(processTag)});
      auto const processResult = processClient->sendInteraction(
          std::move(federationName),
          producingFederateId,
          *interactionClassHandle,
          *parameterHandles,
          processPayload,
          umbra::detail::ProcessFederationLogicalTime{
              timestamp->implementationName(), std::move(processTimestampBytes)});
      processMessageId = processResult.messageId;
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    while (processClient->pendingPushedEventCount() != 0U) {
      try {
        processClient->dispatchPushedReceiveOrder();
      } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
    }
    while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
      try {
        processClient->dispatchPushedObjectInstanceDiscovery();
      } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
        throw RTIinternalError(wideAscii(error.what()));
      }
    }
    // A timestamped regular interaction now receives the execution-owned
    // process message identity.  Preserve the ordinary invalid-handle result
    // for the legacy process timestamp transport path, but expose the real
    // identity whenever the service admitted the message to its TSO queue.
    if (processMessageId != 0U) {
      return makeMessageRetractionHandle(processMessageId);
    }
    return MessageRetractionHandle();
  }
#endif
  std::optional<std::wstring> momExceptionService;
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Timestamped Send Interaction requires a defined InteractionClassHandle.");
  }
  auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Timestamped Send Interaction requires defined ParameterHandle values.");
  }

  // Preserve the fully-qualified MOM interaction name before timestamp
  // validation or registry planning can fail.  Timestamped MOM invocations
  // use the same HLAreportMOMexception projection as their receive-order
  // counterparts; application interactions retain only HLAreportException.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    if (joinedFederationName_ && joinedFederateId_) {
      auto const interactionName = embeddedFederationRegistry()
          .interactionClassNameFor(*joinedFederationName_, *interactionClassHandle);
      if (interactionName) {
        constexpr std::string_view kMomInteractionPrefix =
            umbra::detail::hla::utf8::mom::interaction_manager_prefix;
        if (interactionName->compare(0, kMomInteractionPrefix.size(),
                                     kMomInteractionPrefix) == 0) {
          momExceptionService = umbra::detail::wideFromUtf8(*interactionName);
        }
      }
    }
  }

  auto timestamp = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateAmbassadorTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Interaction requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Send Interaction.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderInteraction(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
      throwReceiveOrderInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      ambassadorCopyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::logical_time,
       L"Optional timestamp",
       umbra::detail::formatMomLogicalTime(*timestamp)},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 transportation type name.");
  }
  auto const transportationValue = ambassadorTransportationTypeValueForFederation(
      *federationName,
      *transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The composed FOM transportation type is not declared in this federation execution.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (federationName && timeSnapshot.timeRegulating) {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const snapshot = embeddedFederationRegistry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP) {
    std::vector<std::uint64_t> tsoRecipientIds;
    std::vector<std::uint64_t> allTimestampedRecipientIds;
    allTimestampedRecipientIds.reserve(plan.recipients.size());
    for (auto const& recipient : plan.recipients) {
      allTimestampedRecipientIds.push_back(recipient.federateId);
      if (timeConstrainedRecipients.contains(recipient.federateId)) {
        tsoRecipientIds.push_back(recipient.federateId);
      }
    }

    umbra::detail::TsoInteractionMessage message;
    message.producingFederateId = *producingFederateId;
    message.sentInteractionClassHandle = *interactionClassHandle;
    message.sentParameterHandles = *parameterHandles;
    message.parameters = sentParameters;
    message.userSuppliedTag = copiedTag;
    message.transportationName = plan.transportationName;
    message.defaultRegionUsed = plan.defaultRegionUsed;
    message.sentOrderType = TIMESTAMP;
    message.receivedOrderType = TIMESTAMP;
    std::shared_ptr<LogicalTime const> sharedTimestamp = timestamp;
    message.timestamp = std::move(sharedTimestamp);

    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const result = embeddedFederationRegistry().enqueueTsoInteraction(
        *federationName,
        std::move(message),
        tsoRecipientIds,
        allTimestampedRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not queue the timestamped Send Interaction.");
    }
    messageId = result.messageId;
  }

  umbra::detail::MomServiceArgument returnedArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  if (messageId != 0U) {
    returnedArgument = {
        umbra::detail::MomArgumentType::message_retraction_handle,
        L"Message retraction designator",
        umbra::detail::formatMomMessageRetractionHandle(messageId)};
  }
  // TSO admission is the accepted service boundary. Route the report through
  // the shared file-or-interaction selector only after the C++ registry has
  // assigned the retraction identity, but before any ordinary/timestamped
  // receive callback is queued.
  appendSuccessfulServiceReportToFileIfSelected(
      L"SendInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument,
      true);

  // Timestamped queue admission (or successful no-recipient validation) is
  // the accepted Send Interaction boundary. Count the service once, not once
  // per TSO/receive-order recipient.
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulInteractionSend(
            *federationName,
            *producingFederateId,
            *interactionClassHandle,
            plan.transportationName,
            false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Send Interaction membership before its accepted boundary.");
    }
  }

  // The first public slice queues TSO for time-constrained recipients. A
  // non-time-constrained recipient still receives the timestamped callback
  // immediately as Receive Order, with the sender's TSO designator when one
  // exists. The callback rechecks subscriptions at its own user-code boundary.
  for (auto const& recipient : plan.recipients) {
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(recipient.federateId)) {
      continue;
    }
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible recipient without a callback route.");
    }
    queueAmbassadorTimestampedReceiveOrderInteraction(
        recipient.callbackRoute,
        *federationName,
        umbra::detail::InteractionProducer::joinedFederate(*producingFederateId),
        recipient.federateId,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        plan.transportationName,
        std::shared_ptr<LogicalTime const>(timestamp),
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP
            ? TIMESTAMP
            : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt,
        std::nullopt,
        plan.defaultRegionUsed);
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    if (momExceptionService) {
      emitMomExceptionReport(
          *momExceptionService,
          exception,
          ambassadorMomExceptionIsParameterError(exception));
    }
    emitExceptionReport(L"Send Interaction", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SendInteraction",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::parameter_handle_value_map,
          L"Constrained set of interaction parameter designator and value pairs",
          umbra::detail::formatMomParameterHandleValueMap(parameterValues)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::logical_time,
          L"Optional timestamp",
          umbra::detail::formatMomLogicalTime(time)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}
#endif
}  // namespace rti1516_2025::umbra_binding_detail
