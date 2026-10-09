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

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {
using InteractionParameterValue = AmbassadorInteractionParameterValue;
}

// IEEE 1516.1-2025 receive-order directed interaction send implementation.

void UmbraRtiAmbassador::sendDirectedInteraction(
    InteractionClassHandle const& interactionClass,
    ObjectInstanceHandle const& objectInstance,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("sendDirectedInteraction");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // A configured process endpoint is a complete federation control plane for
  // this message slice. Route the directed target through the private service
  // rather than consulting the process-local embedded registry.
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
            L"Send Directed Interaction requires membership in a federation execution.");
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
          L"Send Directed Interaction requires a defined InteractionClassHandle.");
    }
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Send Directed Interaction requires a valid target ObjectInstanceHandle.");
    }
    auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
    if (!parameterHandles) {
      throw InteractionParameterNotDefined(
          L"Send Directed Interaction requires defined ParameterHandle values.");
    }

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
    try {
      auto const processPayload = umbra::detail::encodeProcessFederationInteractionEnvelope(
          umbra::detail::ProcessFederationInteractionEnvelope{
              std::move(processParameterValues), std::move(processTag)});
      static_cast<void>(processClient->sendDirectedInteraction(
          std::move(federationName),
          producingFederateId,
          *objectInstanceHandleValueResult,
          *interactionClassHandle,
          *parameterHandles,
          processPayload));
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
    return;
  }
#endif
  std::optional<std::wstring> momExceptionService;
  try {
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Directed Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Send Directed Interaction requires a defined InteractionClassHandle.");
  }
  auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandleValueResult) {
    throw ObjectInstanceNotKnown(
        L"Send Directed Interaction requires a valid target ObjectInstanceHandle.");
  }
  auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Send Directed Interaction requires defined ParameterHandle values.");
  }

  // Resolve the MOM service before registry planning so publication, target,
  // or routing failures use the standard HLAreportMOMexception projection.
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

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Send Directed Interaction requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Directed Interaction.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderDirectedInteraction(
        *federationName,
        *producingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status !=
        umbra::detail::ReceiveOrderDirectedInteractionStatus::applied) {
      throwReceiveOrderDirectedInteractionFailure(plan.status);
    }
    return plan;
  };

  // Validate publication, target knowledge, and parameter handles before
  // copying caller-owned values, then re-plan before routing.
  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      ambassadorCopyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  // Section 6.14.1 names five supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted parameter map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::parameter_handle_value_map,
       L"Constrained set of interaction parameter designator and value pairs",
       umbra::detail::formatMomParameterHandleValueMap(reportParameterValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 directed-interaction transportation name.");
  }
  auto const transportationValue = ambassadorTransportationTypeValueForFederation(
      *federationName,
      *transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded federation cannot deliver a directed interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  struct Delivery {
    umbra::detail::InteractionCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const& recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible directed-interaction recipient without a callback route.");
    }
    deliveries.push_back({recipient.callbackRoute, recipient.federateId});
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.14's accepted interaction is the report boundary, and the §11.5 file
  // record must precede any induced Receive Directed Interaction callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendDirectedInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);

  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulInteractionSend(
            *federationName,
            *producingFederateId,
            *interactionClassHandle,
            plan.transportationName,
            true);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Send Directed Interaction membership before its accepted boundary.");
    }
  }

  // Submit only after releasing the sender/runtime locks: HLA_IMMEDIATE may
  // synchronously enter another federate's Receive Directed Interaction callback.
  for (auto& delivery : deliveries) {
    queueAmbassadorReceiveOrderDirectedInteraction(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        plan.transportationName);
  }
  } catch (Exception const& exception) {
    if (momExceptionService) {
      emitMomExceptionReport(
          *momExceptionService,
          exception,
          ambassadorMomExceptionIsParameterError(exception));
    }
    emitExceptionReport(L"Send Directed Interaction", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SendDirectedInteraction",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::parameter_handle_value_map,
          L"Constrained set of interaction parameter designator and value pairs",
          umbra::detail::formatMomParameterHandleValueMap(parameterValues)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

// IEEE 1516.1-2025 timestamped directed interaction send implementation.

MessageRetractionHandle UmbraRtiAmbassador::sendDirectedInteraction(
    InteractionClassHandle const& interactionClass,
    ObjectInstanceHandle const& objectInstance,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("sendDirectedInteraction");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // Preserve the public timestamp validation and callback contract while
  // routing the accepted message through the process federation service. The
  // service returns the execution-owned retraction designator when it accepts
  // a timestamped directed interaction with at least one recipient.
  if (processEndpointActive_) {
    std::uint64_t processMessageId = 0U;
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Timestamped Send Directed Interaction requires membership in a federation execution.");
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
          L"Timestamped Send Directed Interaction requires a defined InteractionClassHandle.");
    }
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Timestamped Send Directed Interaction requires a valid target ObjectInstanceHandle.");
    }
    auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
    if (!parameterHandles) {
      throw InteractionParameterNotDefined(
          L"Timestamped Send Directed Interaction requires defined ParameterHandle values.");
    }

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

    try {
      auto const processPayload = umbra::detail::encodeProcessFederationInteractionEnvelope(
          umbra::detail::ProcessFederationInteractionEnvelope{
              std::move(processParameterValues), std::move(processTag)});
      auto const processResult = processClient->sendDirectedInteraction(
          std::move(federationName),
          producingFederateId,
          *objectInstanceHandleValueResult,
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
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Directed Interaction requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
  if (!interactionClassHandle) {
    throw InteractionClassNotDefined(
        L"Timestamped Send Directed Interaction requires a defined InteractionClassHandle.");
  }
  auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandleValueResult) {
    throw ObjectInstanceNotKnown(
        L"Timestamped Send Directed Interaction requires a valid target ObjectInstanceHandle.");
  }
  auto const parameterHandles = ambassadorInteractionParameterHandleValues(parameterValues);
  if (!parameterHandles) {
    throw InteractionParameterNotDefined(
        L"Timestamped Send Directed Interaction requires defined ParameterHandle values.");
  }

  // Resolve the MOM service before timestamp validation so every failure of a
  // timestamped MOM invocation can be projected through HLAreportMOMexception.
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
    requireFederationServiceOperationAvailable(
        L"Timestamped Send Directed Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Send Directed Interaction requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Send Directed Interaction.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderDirectedInteraction(
        *federationName,
        *producingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        *parameterHandles);
    if (plan.status !=
        umbra::detail::ReceiveOrderDirectedInteractionStatus::applied) {
      throwReceiveOrderDirectedInteractionFailure(plan.status);
    }
    return plan;
  };

  static_cast<void>(planCurrentRequest());
  std::vector<InteractionParameterValue> sentParameters =
      ambassadorCopyInteractionParameterValues(parameterValues);
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();

  ParameterHandleValueMap reportParameterValues;
  for (auto const& [parameterHandle, parameterValue] : sentParameters) {
    reportParameterValues.emplace(makeParameterHandle(parameterHandle), parameterValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
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

  auto const transportationName = umbra::detail::wideFromUtf8(plan.transportationName);
  if (!transportationName) {
    throw RTIinternalError(
        L"The composed FOM contains an invalid UTF-8 directed-interaction transportation name.");
  }
  auto const transportationValue = ambassadorTransportationTypeValueForFederation(
      *federationName,
      *transportationName);
  if (!transportationValue) {
    throw RTIinternalError(
        L"The embedded federation cannot deliver a directed interaction using this FOM transportation type.");
  }
  TransportationTypeHandle const transportationType =
      makeTransportationTypeHandle(*transportationValue);

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (timeSnapshot.timeRegulating) {
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

  std::vector<std::uint64_t> tsoRecipientIds;
  std::vector<umbra::detail::TsoDirectedInteractionRecipient> recipients;
  recipients.reserve(plan.recipients.size());
  for (auto const& candidate : plan.recipients) {
    if (!candidate.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an eligible directed-interaction recipient without a callback route.");
    }
    recipients.push_back({
        candidate.federateId,
        candidate.objectInstanceHandle,
        candidate.receivedInteractionClassHandle,
        candidate.receivedParameterHandles,
        candidate.callbackRoute});
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(candidate.federateId)) {
      tsoRecipientIds.push_back(candidate.federateId);
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
      !recipients.empty()) {
    umbra::detail::TsoDirectedInteractionMessage message;
    message.producingFederateId = *producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandleValueResult;
    message.sentInteractionClassHandle = *interactionClassHandle;
    message.sentParameterHandles = *parameterHandles;
    message.parameters = sentParameters;
    message.userSuppliedTag = copiedTag;
    message.transportationName = plan.transportationName;
    message.sentOrderType = TIMESTAMP;
    message.receivedOrderType = TIMESTAMP;
    message.recipients = recipients;
    message.timestamp = timestamp;

    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const result = embeddedFederationRegistry()
        .enqueueTsoDirectedInteraction(
            *federationName,
            std::move(message),
            tsoRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not enqueue the timestamped directed interaction.");
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
  // Timestamped directed-interaction admission is the accepted service
  // boundary.  Route the standard seven-parameter report through the shared
  // file-or-interaction selector before any immediate or queued directed
  // callback, preserving the configured backend semantics of the joined
  // federate just as the receive-order path does.
  appendSuccessfulServiceReportToFileIfSelected(
      L"SendDirectedInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument,
      true);

  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulInteractionSend(
            *federationName,
            *producingFederateId,
            *interactionClassHandle,
            plan.transportationName,
            true);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Send Directed Interaction membership before its accepted boundary.");
    }
  }

  for (auto const& recipient : recipients) {
    if (timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP &&
        timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
      continue;
    }
    queueAmbassadorTimestampedReceiveOrderDirectedInteraction(
        recipient.callbackRoute,
        *federationName,
        *producingFederateId,
        recipient.receivingFederateId,
        *objectInstanceHandleValueResult,
        *interactionClassHandle,
        sentParameters,
        copiedTag,
        transportationType,
        plan.transportationName,
        timestamp,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP
            ? TIMESTAMP
            : RECEIVE,
        RECEIVE,
        timeSnapshot.timeRegulating && plan.preferredOrderType == TIMESTAMP && messageId != 0
            ? std::optional<std::uint64_t>(messageId)
            : std::nullopt);
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
    emitExceptionReport(L"Timestamped Send Directed Interaction", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SendDirectedInteraction",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
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
