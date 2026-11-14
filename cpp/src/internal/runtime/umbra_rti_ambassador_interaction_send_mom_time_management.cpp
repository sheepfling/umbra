#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/time/federation_time_factory_provider.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::handleEmbeddedMomTimeManagementInteractionRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  if (!federationName || !producingFederateId || !interactionClassHandle) {
    return false;
  }

  enum class TimeManagementRequest {
    flush_queue,
    modify_lookahead,
    time_advance,
    time_advance_available,
    next_message,
    next_message_available,
  };
  std::optional<TimeManagementRequest> request;
  std::optional<std::vector<std::uint8_t>> encodedArgument;
  std::optional<std::wstring> timeImplementationName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Send Interaction");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != *federationName ||
        *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Send Interaction.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const interactionClassName = registry.interactionClassNameFor(
        *federationName,
        *interactionClassHandle);
    if (!interactionClassName) {
      return false;
    }
    auto const isFlushQueueRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::flush_queue_request);
    auto const isModifyLookahead = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::modify_lookahead);
    auto const isNextMessageRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::next_message_request);
    auto const isNextMessageRequestAvailable =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::next_message_request_available);
    auto const isTimeAdvanceRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::time_advance_request);
    auto const isTimeAdvanceRequestAvailable =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::time_advance_request_available);
    if (!isFlushQueueRequest || !isModifyLookahead ||
        !isNextMessageRequest || !isNextMessageRequestAvailable ||
        !isTimeAdvanceRequest || !isTimeAdvanceRequestAvailable) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the MOM time-management class hierarchy.");
    }
    if (*isFlushQueueRequest) {
      request = TimeManagementRequest::flush_queue;
    } else if (*isModifyLookahead) {
      request = TimeManagementRequest::modify_lookahead;
    } else if (*isNextMessageRequest) {
      request = TimeManagementRequest::next_message;
    } else if (*isNextMessageRequestAvailable) {
      request = TimeManagementRequest::next_message_available;
    } else if (*isTimeAdvanceRequest) {
      request = TimeManagementRequest::time_advance;
    } else if (*isTimeAdvanceRequestAvailable) {
      request = TimeManagementRequest::time_advance_available;
    } else {
      return false;
    }

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"A MOM time-management request received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"A MOM time-management request received an unknown parameter.");
      }
      auto const expectedParameter =
          *request == TimeManagementRequest::flush_queue
              || *request == TimeManagementRequest::time_advance
              || *request == TimeManagementRequest::time_advance_available
              || *request == TimeManagementRequest::next_message
              || *request == TimeManagementRequest::next_message_available
          ? umbra::detail::hla::utf8::mom::time_stamp
          : umbra::detail::hla::utf8::mom::lookahead;
      if (*parameterName == expectedParameter) {
        encodedArgument =
            umbra::detail::variable_length_data_2025::copyBytes(parameterValue);
      }
      // Compatible MIM extensions may add parameters; only the pinned
      // time-stamp or lookahead argument is consumed by these services.
    }
    if (!encodedArgument) {
      if (*request == TimeManagementRequest::flush_queue) {
        throw InteractionParameterNotDefined(
            L"HLAflushQueueRequest requires HLAtimeStamp.");
      }
      if (*request == TimeManagementRequest::next_message) {
        throw InteractionParameterNotDefined(
            L"HLAnextMessageRequest requires HLAtimeStamp.");
      }
      if (*request == TimeManagementRequest::next_message_available) {
        throw InteractionParameterNotDefined(
            L"HLAnextMessageRequestAvailable requires HLAtimeStamp.");
      }
      if (*request == TimeManagementRequest::time_advance) {
        throw InteractionParameterNotDefined(
            L"HLAtimeAdvanceRequest requires HLAtimeStamp.");
      }
      if (*request == TimeManagementRequest::time_advance_available) {
        throw InteractionParameterNotDefined(
            L"HLAtimeAdvanceRequestAvailable requires HLAtimeStamp.");
      }
      throw InteractionParameterNotDefined(
          L"HLAmodifyLookahead requires HLAlookahead.");
    }
    if (!federateTimeState_) {
      if (*request == TimeManagementRequest::flush_queue) {
        throw InvalidLogicalTime(
            L"HLAflushQueueRequest requires an active logical-time implementation.");
      }
      if (*request == TimeManagementRequest::time_advance) {
        throw InvalidLogicalTime(
            L"HLAtimeAdvanceRequest requires an active logical-time implementation.");
      }
      if (*request == TimeManagementRequest::time_advance_available) {
        throw InvalidLogicalTime(
            L"HLAtimeAdvanceRequestAvailable requires an active logical-time implementation.");
      }
      if (*request == TimeManagementRequest::next_message ||
          *request == TimeManagementRequest::next_message_available) {
        throw InvalidLogicalTime(
            L"An HLA Next Message Request requires an active logical-time implementation.");
      }
      throw InvalidLookahead(
          L"HLAmodifyLookahead requires an active logical-time implementation.");
    }
    timeImplementationName = federateTimeState_->implementationName();
  }

  auto const timeFactory =
      umbra::detail::makeFederationLogicalTimeFactory(*timeImplementationName);
  if (!timeFactory || timeFactory->getName() != *timeImplementationName) {
    if (*request == TimeManagementRequest::flush_queue ||
        *request == TimeManagementRequest::time_advance ||
        *request == TimeManagementRequest::time_advance_available ||
        *request == TimeManagementRequest::next_message ||
        *request == TimeManagementRequest::next_message_available) {
      throw InvalidLogicalTime(
          L"A standard MOM time-advance request has no available logical-time implementation.");
    }
    throw InvalidLookahead(
        L"HLAmodifyLookahead has no available logical-time implementation.");
  }
  VariableLengthData encodedValue;
  if (!encodedArgument->empty()) {
    encodedValue.setData(encodedArgument->data(), encodedArgument->size());
  }

  if (*request == TimeManagementRequest::flush_queue ||
      *request == TimeManagementRequest::time_advance ||
      *request == TimeManagementRequest::time_advance_available ||
      *request == TimeManagementRequest::next_message ||
      *request == TimeManagementRequest::next_message_available) {
    std::unique_ptr<LogicalTime> requestedTime;
    try {
      requestedTime = timeFactory->decodeLogicalTime(encodedValue);
    } catch (Exception const&) {
      if (*request == TimeManagementRequest::time_advance) {
        throw InvalidLogicalTime(
            L"HLAtimeAdvanceRequest supplied an invalid HLAtimeStamp value.");
      }
      throw InvalidLogicalTime(
          L"HLAflushQueueRequest supplied an invalid HLAtimeStamp value.");
    }
    if (!requestedTime ||
        requestedTime->implementationName() != *timeImplementationName) {
      if (*request == TimeManagementRequest::time_advance) {
        throw InvalidLogicalTime(
            L"HLAtimeAdvanceRequest supplied an invalid HLAtimeStamp value.");
      }
      throw InvalidLogicalTime(
          L"HLAflushQueueRequest supplied an invalid HLAtimeStamp value.");
    }
    if (*request == TimeManagementRequest::flush_queue) {
      flushQueueRequest(*requestedTime);
    } else if (*request == TimeManagementRequest::time_advance) {
      timeAdvanceRequest(*requestedTime);
    } else if (*request == TimeManagementRequest::time_advance_available) {
      timeAdvanceRequestAvailable(*requestedTime);
    } else if (*request == TimeManagementRequest::next_message) {
      nextMessageRequest(*requestedTime);
    } else {
      nextMessageRequestAvailable(*requestedTime);
    }
  } else {
    std::unique_ptr<LogicalTimeInterval> requestedLookahead;
    try {
      requestedLookahead = timeFactory->decodeLogicalTimeInterval(encodedValue);
    } catch (Exception const&) {
      throw InvalidLookahead(
          L"HLAmodifyLookahead supplied an invalid HLAlookahead value.");
    }
    if (!requestedLookahead ||
        requestedLookahead->implementationName() != *timeImplementationName) {
      throw InvalidLookahead(
          L"HLAmodifyLookahead supplied an invalid HLAlookahead value.");
    }
    modifyLookahead(*requestedLookahead);
  }

  auto const sentParameters =
      ambassadorCopyInteractionParameterValues(parameterValues);
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
       umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SendInteraction",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry().recordSuccessfulInteractionSend(
        *federationName,
        *producingFederateId,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::reliable,
        false);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost Send Interaction membership while applying a time-management MOM request.");
    }
  }
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
