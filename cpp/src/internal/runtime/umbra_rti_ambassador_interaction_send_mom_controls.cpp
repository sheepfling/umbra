#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/encoding/byte_order.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
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
#include "internal/time/federation_time_factory_provider.hpp"
#include "internal/time/federate_time_state.hpp"
#include "internal/time/reference_time_selection.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <algorithm>
#include <cstdint>
#include <memory>
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

std::optional<std::uint32_t> decodeHlaInteger32BE(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  std::uint32_t encoded = 0U;
  if (bytes == nullptr ||
      !umbra::detail::readUnsigned(
          static_cast<void const *>(bytes),
          value.size(),
          umbra::detail::ByteOrder::big,
          encoded)) {
    return std::nullopt;
  }
  return encoded;
}

std::optional<bool> decodeHlaSwitch(VariableLengthData const& value) {
  auto const encoded = decodeHlaInteger32BE(value);
  if (!encoded) {
    return std::nullopt;
  }
  if (*encoded > 1U) {
    return std::nullopt;
  }
  return *encoded == 1U;
}

// HLAownership uses the same HLAinteger32BE representation as HLAswitch, but
// its enumerator names describe the requested ownership state.  Keep a
// separate decoder so a future MIM extension cannot accidentally reuse a
// switch-specific diagnostic or accept an unknown ownership enumerator.
std::optional<bool> decodeHlaOwnership(VariableLengthData const& value) {
  auto const encoded = decodeHlaInteger32BE(value);
  if (!encoded || *encoded > 1U) {
    return std::nullopt;
  }
  return *encoded == 1U;
}

// HLAstandardMIM represents HLAresignAction with HLAinteger32BE values zero
// through five.  Decode by the MIM vocabulary rather than relying on an enum
// cast so malformed incoming MOM payloads cannot silently alter a federate's
// automatic-resign policy.
std::optional<ResignAction> decodeHlaResignAction(VariableLengthData const& value) {
  auto const encoded = decodeHlaInteger32BE(value);
  if (!encoded) {
    return std::nullopt;
  }
  switch (*encoded) {
    case 0U:
      return UNCONDITIONALLY_DIVEST_ATTRIBUTES;
    case 1U:
      return DELETE_OBJECTS;
    case 2U:
      return CANCEL_PENDING_OWNERSHIP_ACQUISITIONS;
    case 3U:
      return DELETE_OBJECTS_THEN_DIVEST;
    case 4U:
      return CANCEL_THEN_DELETE_THEN_DIVEST;
    case 5U:
      return NO_ACTION;
    default:
      return std::nullopt;
  }
}

}  // namespace

bool UmbraRtiAmbassador::handleEmbeddedMomInteractionControlRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momSwitchWork;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> federationMomSwitchWork;
  std::optional<umbra::detail::AttributeTransportationTypeChangePlan>
      momAttributeTransportationTypeChange;
  std::optional<umbra::detail::InteractionTransportationTypeChangePlan>
      momInteractionTransportationTypeChange;

  // IEEE 1516.1-2025 §11.4.2 explicitly permits these three MOM control
  // interactions during a federate save/restore.  They are consumed as the
  // corresponding public service invocation rather than being blocked by the
  // generic MOM adjustment handlers below (whose service gates correctly
  // reject unrelated interactions during these operations).
  auto const handleMomSaveRestoreControlInteraction = [&]() {
    if (!federationName || !producingFederateId || !interactionClassHandle) {
      return false;
    }

    std::optional<std::string> interactionClassName;
    std::optional<std::uint64_t> successIndicatorHandle;
    std::vector<std::uint64_t> parameterHandles;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      requireConnectedForFederationManagement(lifecycle_);
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
      interactionClassName = registry.interactionClassNameFor(
          *federationName, *interactionClassHandle);
      if (!interactionClassName ||
          (*interactionClassName !=
               umbra::detail::hla::utf8::mom::service_federate_save_begun &&
           *interactionClassName !=
               umbra::detail::hla::utf8::mom::service_federate_save_complete &&
           *interactionClassName !=
               umbra::detail::hla::utf8::mom::service_federate_restore_complete)) {
        return false;
      }

      parameterHandles.reserve(parameterValues.size());
      for (auto const& [parameterHandle, unusedValue] : parameterValues) {
        static_cast<void>(unusedValue);
        auto const suppliedHandle = parameterHandleValue(parameterHandle);
        if (!suppliedHandle) {
          throw InteractionParameterNotDefined(
              L"A standard save/restore MOM interaction contains an invalid parameter handle.");
        }
        parameterHandles.push_back(*suppliedHandle);
        if (*interactionClassName !=
                umbra::detail::hla::utf8::mom::service_federate_save_begun) {
          auto const parameterName = registry.parameterNameFor(
              *federationName, *interactionClassName, *suppliedHandle);
          if (parameterName &&
              *parameterName == umbra::detail::hla::utf8::mom::success_indicator) {
            successIndicatorHandle = *suppliedHandle;
          }
        }
      }

      auto const plan = registry.planReceiveOrderInteraction(
          *federationName,
          *producingFederateId,
          *interactionClassHandle,
          parameterHandles);
      if (plan.status != umbra::detail::ReceiveOrderInteractionStatus::applied) {
        throwReceiveOrderInteractionFailure(plan.status);
      }
    }

    if (*interactionClassName !=
            umbra::detail::hla::utf8::mom::service_federate_save_begun &&
        !successIndicatorHandle) {
      throw InteractionParameterNotDefined(
          L"A standard save/restore completion interaction requires HLAsuccessIndicator.");
    }

    bool success = true;
    if (successIndicatorHandle) {
      auto const value = std::find_if(
          parameterValues.begin(), parameterValues.end(),
          [&](auto const& entry) {
            return parameterHandleValue(entry.first) == successIndicatorHandle;
          });
      if (value == parameterValues.end()) {
        throw RTIinternalError(
            L"The save/restore interaction success-indicator handle disappeared.");
      }
      rti1516_2025::HLAboolean decodedSuccess;
      try {
        decodedSuccess.decode(value->second);
      } catch (Exception const&) {
        throw RTIinternalError(
            L"A standard save/restore completion interaction contains an invalid HLAsuccessIndicator.");
      }
      success = decodedSuccess.get();
    }

    if (*interactionClassName ==
        umbra::detail::hla::utf8::mom::service_federate_save_begun) {
      if (!parameterValues.empty()) {
        throw InteractionParameterNotDefined(
            L"HLAfederateSaveBegun does not define interaction parameters.");
      }
      federateSaveBegun();
    } else if (*interactionClassName ==
               umbra::detail::hla::utf8::mom::service_federate_save_complete) {
      success ? federateSaveComplete() : federateSaveNotComplete();
    } else {
      success ? federateRestoreComplete() : federateRestoreNotComplete();
    }

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
            L"The embedded federation lost Send Interaction membership after an accepted save/restore MOM interaction.");
      }
    }
    return true;
  };

  if (handleMomSaveRestoreControlInteraction()) {
    return true;
  }
  if (handleEmbeddedMomResignFederationExecutionRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }
  if (handleEmbeddedMomInteractionSubscriptionRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }
  if (handleEmbeddedMomSynchronizationPointAchievedRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }
  if (handleEmbeddedMomUnconditionalDivestitureRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }

  if (handleEmbeddedMomInteractionDeclarationRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }
  if (handleEmbeddedMomObjectAttributePublicationRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }
  if (handleEmbeddedMomObjectDirectedInteractionPublicationRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }

  if (handleEmbeddedMomTimeManagementInteractionRequest(
          federationName,
          producingFederateId,
          interactionClassHandle,
          interactionClass,
          parameterValues,
          userSuppliedTag)) {
    return true;
  }



  // Transportation, switch, timing, and attribute-state MOM controls.

  // HLAservice interactions invoke the corresponding public service for the
  // sending joined federate.  In particular, §11.4.1 permits omission of an
  // interaction parameter that represents an optional service argument.  The
  // absent HLAattributeList is therefore the whole-class overload, not an
  // empty attribute set.
  auto const handleMomUnsubscribeObjectClassAttributes = [&]() {
    std::optional<ObjectClassHandle> objectClass;
    std::optional<AttributeHandleSet> attributes;
    {
      std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
      requireConnectedForFederationManagement(lifecycle_);
      requireFederationServiceOperationAvailable(L"Send Interaction");
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_ ||
          !federationName || !producingFederateId || !interactionClassHandle ||
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
      auto const isUnsubscribeRequest = registry.interactionClassIsSameOrDescendantOf(
          *federationName,
          *interactionClassHandle,
          umbra::detail::hla::utf8::mom::service_unsubscribe_object_class_attributes);
      if (!isUnsubscribeRequest) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the HLAunsubscribeObjectClassAttributes MOM class hierarchy.");
      }
      if (!*isUnsubscribeRequest) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLAunsubscribeObjectClassAttributes received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAunsubscribeObjectClassAttributes received an unknown parameter for this interaction class.");
        }
        if (*parameterName == "HLAobjectClass") {
          try {
            objectClass = ::rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
                parameterValue);
            if (!::rti1516_2025::umbra_binding_detail::objectClassHandleValue(*objectClass)) {
              throw RTIinternalError(
                  L"HLAunsubscribeObjectClassAttributes received an invalid HLAobjectClassHandle value.");
            }
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAunsubscribeObjectClassAttributes received an invalid HLAobjectClassHandle value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::attribute_list) {
          try {
            rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet>
                handlePrototype;
            rti1516_2025::HLAvariableArray handles{
                static_cast<rti1516_2025::DataElement const&>(handlePrototype)};
            handles.decode(parameterValue);
            AttributeHandleSet decodedAttributes;
            for (std::size_t index = 0U; index < handles.size(); ++index) {
              auto const* encodedHandle = dynamic_cast<
                  rti1516_2025::HLAvariableArray const*>(&handles.get(index));
              if (encodedHandle == nullptr) {
                throw RTIinternalError(
                    L"HLAunsubscribeObjectClassAttributes received a malformed HLAattributeHandleList value.");
              }
              auto const decodedHandle =
                  ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(
                      encodedHandle->encode());
              auto const decodedValue =
                  ::rti1516_2025::umbra_binding_detail::attributeHandleValue(decodedHandle);
              if (!decodedValue) {
                throw RTIinternalError(
                    L"HLAunsubscribeObjectClassAttributes received an invalid HLAattributeHandle value.");
              }
              decodedAttributes.insert(
                  ::rti1516_2025::umbra_binding_detail::makeAttributeHandle(*decodedValue));
            }
            attributes = std::move(decodedAttributes);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAunsubscribeObjectClassAttributes received a malformed HLAattributeHandleList value.");
          }
          continue;
        }
        // Compatible MIM extensions may add parameters; the two standard
        // service arguments above remain the only values consumed here.
      }
    }

    if (!objectClass) {
      throw InteractionParameterNotDefined(
          L"HLAunsubscribeObjectClassAttributes requires HLAobjectClass; HLAattributeList is optional.");
    }
    if (attributes) {
      unsubscribeObjectClassAttributes(*objectClass, *attributes);
    } else {
      unsubscribeObjectClass(*objectClass);
    }
    return true;
  };

  if (handleMomUnsubscribeObjectClassAttributes()) {
    return true;
  }

  // HLAenable/disableAsynchronousDelivery are self-service MOM interactions:
  // the inherited HLAfederate parameter identifies the joined federate whose
  // time state is changed.
  auto const handleMomAsynchronousDeliveryRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::shared_ptr<umbra::detail::FederateTimeState> targetTimeState;
    bool enable = false;
    std::wstring serviceName;
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
      auto const isEnable = registry.interactionClassIsSameOrDescendantOf(
          *federationName,
          *interactionClassHandle,
          umbra::detail::hla::utf8::mom::enable_asynchronous_delivery);
      auto const isDisable = registry.interactionClassIsSameOrDescendantOf(
          *federationName,
          *interactionClassHandle,
          umbra::detail::hla::utf8::mom::disable_asynchronous_delivery);
      if (!isEnable || !isDisable) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the asynchronous-delivery MOM class hierarchy.");
      }
      if (!*isEnable && !*isDisable) {
        return false;
      }
      enable = *isEnable;
      serviceName = enable ? L"EnableAsynchronousDelivery"
                           : L"DisableAsynchronousDelivery";

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"An asynchronous-delivery MOM request received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"An asynchronous-delivery MOM request received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                    parameterValue);
            targetFederateId =
                ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                    decodedHandle);
          } catch (Exception const&) {
            throw FederateNotExecutionMember(
                L"An asynchronous-delivery MOM request supplied an invalid HLAfederate handle.");
          }
          if (!targetFederateId) {
            throw FederateNotExecutionMember(
                L"An asynchronous-delivery MOM request supplied an invalid HLAfederate handle.");
          }
          continue;
        }
        // Compatible MIM extensions may add arguments; the inherited
        // HLAfederate designator remains the only standard argument consumed.
      }
      if (!targetFederateId) {
        throw InteractionParameterNotDefined(
            L"An asynchronous-delivery MOM request requires HLAfederate.");
      }
      if (!registry.memberById(*federationName, *targetFederateId)) {
        throw FederateNotExecutionMember(
            L"An asynchronous-delivery MOM request requires a joined target federate.");
      }
      targetTimeState = registry.timeStateFor(*federationName, *targetFederateId);
      if (!targetTimeState) {
        throw FederateNotExecutionMember(
            L"An asynchronous-delivery MOM request target has no active time state.");
      }
    }

    auto const status = enable
                            ? targetTimeState->enableAsynchronousDelivery()
                            : targetTimeState->disableAsynchronousDelivery();
    switch (status) {
      case umbra::detail::FederateAsynchronousDeliveryStatus::applied:
        break;
      case umbra::detail::FederateAsynchronousDeliveryStatus::already_enabled:
        throw AsynchronousDeliveryAlreadyEnabled(
            L"Asynchronous delivery is already enabled for the target federate.");
      case umbra::detail::FederateAsynchronousDeliveryStatus::already_disabled:
        throw AsynchronousDeliveryAlreadyDisabled(
            L"Asynchronous delivery is already disabled for the target federate.");
      case umbra::detail::FederateAsynchronousDeliveryStatus::inactive:
        throw FederateNotExecutionMember(
            L"The asynchronous-delivery MOM request target is no longer active.");
    }

    queueAmbassadorJoinedFederateMomConditionalAttributeUpdateForJoinedFederate(
        *federationName,
        *targetFederateId,
        {umbra::detail::hla::utf8::mom::asynchronous_delivery});
    if (*targetFederateId == *producingFederateId) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          serviceName,
          umbra::detail::MomServiceType::time_management,
          {},
          true);
    }
    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
      auto const recordStatus = embeddedFederationRegistry().recordSuccessfulInteractionSend(
          *federationName,
          *producingFederateId,
          *interactionClassHandle,
          umbra::detail::hla::utf8::mom::reliable,
          false);
      if (recordStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost Send Interaction membership while applying an asynchronous-delivery MOM request.");
      }
    }
    return true;
  };

  if (handleMomAsynchronousDeliveryRequest()) {
    return true;
  }

  // HLAdisableTimeConstrained applies the standard time-management transition
  // to the joined federate identified by its inherited HLAfederate parameter.
  auto const handleMomDisableTimeConstrainedRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::shared_ptr<umbra::detail::FederateTimeState> targetTimeState;
    std::vector<umbra::detail::TsoPayloadDelivery> tsoDeliveriesAsReceiveOrder;
    std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
    std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
    umbra::detail::FederateTimeDisableStatus disableStatus =
        umbra::detail::FederateTimeDisableStatus::inactive;
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
      auto const isDisableTimeConstrained =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::disable_time_constrained);
      if (!isDisableTimeConstrained) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the Disable Time Constrained MOM class hierarchy.");
      }
      if (!*isDisableTimeConstrained) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"A Disable Time Constrained MOM request received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"A Disable Time Constrained MOM request received an unknown parameter.");
        }
        if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
          continue;
        }
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                  parameterValue);
          targetFederateId =
              ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                  decodedHandle);
        } catch (Exception const&) {
          throw FederateNotExecutionMember(
              L"A Disable Time Constrained MOM request supplied an invalid HLAfederate handle.");
        }
        if (!targetFederateId) {
          throw FederateNotExecutionMember(
              L"A Disable Time Constrained MOM request supplied an invalid HLAfederate handle.");
        }
      }
      if (!targetFederateId) {
        throw InteractionParameterNotDefined(
            L"A Disable Time Constrained MOM request requires HLAfederate.");
      }
      if (!registry.memberById(*federationName, *targetFederateId)) {
        throw FederateNotExecutionMember(
            L"A Disable Time Constrained MOM request requires a joined target federate.");
      }
      targetTimeState = registry.timeStateFor(*federationName, *targetFederateId);
      if (!targetTimeState) {
        throw FederateNotExecutionMember(
            L"A Disable Time Constrained MOM request target has no active time state.");
      }

      disableStatus = targetTimeState->disableTimeConstrained();
      if (disableStatus == umbra::detail::FederateTimeDisableStatus::applied) {
        auto delivery = registry.beginTsoPayloadReceiveOrderDelivery(
            *federationName,
            *targetFederateId);
        if (delivery.status != umbra::detail::FederationTsoRegistryStatus::applied ||
            (delivery.deliveryStatus !=
                 umbra::detail::FederationTsoDeliveryStatus::applied &&
             delivery.deliveryStatus !=
                 umbra::detail::FederationTsoDeliveryStatus::no_messages)) {
          throw RTIinternalError(
              L"The embedded federation could not transition the target's queued timestamped payloads to receive-order delivery.");
        }
        tsoDeliveriesAsReceiveOrder = std::move(delivery.deliveries);
        auto scheduled = registry.reevaluateTimeAdvanceGrants(*federationName);
        if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
          newlyEligible = std::move(scheduled.dispatches);
        }
        momWork = ambassadorJoinedFederateMomConditionalWorkFor(
            registry,
            *federationName,
            *targetFederateId,
            {umbra::detail::hla::utf8::mom::time_constrained});
      }
    }

    switch (disableStatus) {
      case umbra::detail::FederateTimeDisableStatus::applied:
        break;
      case umbra::detail::FederateTimeDisableStatus::not_enabled:
        throw TimeConstrainedIsNotEnabled(
            L"Time constrained is not enabled for the target federate.");
      case umbra::detail::FederateTimeDisableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The Disable Time Constrained MOM request target is no longer active.");
    }

    if (momWork) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
          momWork->federationName,
          momWork->objectInstanceHandle,
          std::move(momWork->attributeHandles));
    }
    if (*targetFederateId == *producingFederateId) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"DisableTimeConstrained",
          umbra::detail::MomServiceType::time_management,
          {},
          true);
    }
    flushAmbassadorAsynchronousReceiveCallbacks(targetTimeState);
    queueAmbassadorTsoPayloadsAsReceiveOrder(
        *federationName,
        *targetFederateId,
        std::move(tsoDeliveriesAsReceiveOrder));
    submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));

    auto const sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData const copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
      auto const recordStatus =
          embeddedFederationRegistry().recordSuccessfulInteractionSend(
          *federationName,
          *producingFederateId,
          *interactionClassHandle,
          umbra::detail::hla::utf8::mom::reliable,
          false);
      if (recordStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost Send Interaction membership while applying HLAdisableTimeConstrained.");
      }
    }
    return true;
  };

  if (handleMomDisableTimeConstrainedRequest()) {
    return true;
  }

  // HLAdisableTimeRegulation applies the time-regulation role transition to
  // the joined federate named by its inherited HLAfederate parameter.
  auto const handleMomDisableTimeRegulationRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::vector<umbra::detail::FederationTimeGrantDispatch> newlyEligible;
    std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
    auto disableStatus = umbra::detail::FederateTimeDisableStatus::inactive;
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
      auto const isDisableTimeRegulation =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::disable_time_regulation);
      if (!isDisableTimeRegulation) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the Disable Time Regulation MOM class hierarchy.");
      }
      if (!*isDisableTimeRegulation) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"A Disable Time Regulation MOM request received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"A Disable Time Regulation MOM request received an unknown parameter.");
        }
        if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
          continue;
        }
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                  parameterValue);
          targetFederateId =
              ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                  decodedHandle);
        } catch (Exception const&) {
          throw FederateNotExecutionMember(
              L"A Disable Time Regulation MOM request supplied an invalid HLAfederate handle.");
        }
        if (!targetFederateId) {
          throw FederateNotExecutionMember(
              L"A Disable Time Regulation MOM request supplied an invalid HLAfederate handle.");
        }
      }
      if (!targetFederateId) {
        throw InteractionParameterNotDefined(
            L"A Disable Time Regulation MOM request requires HLAfederate.");
      }
      if (!registry.memberById(*federationName, *targetFederateId)) {
        throw FederateNotExecutionMember(
            L"A Disable Time Regulation MOM request requires a joined target federate.");
      }
      auto const targetTimeState = registry.timeStateFor(
          *federationName,
          *targetFederateId);
      if (!targetTimeState) {
        throw FederateNotExecutionMember(
            L"A Disable Time Regulation MOM request target has no active time state.");
      }

      disableStatus = targetTimeState->disableTimeRegulation();
      if (disableStatus == umbra::detail::FederateTimeDisableStatus::applied) {
        registry.applyTsoObjectDeletionProducerRegulationDisabled(
            *federationName,
            *targetFederateId);
        auto scheduled = registry.reevaluateTimeAdvanceGrants(*federationName);
        if (scheduled.status == umbra::detail::FederationTimeGrantStatus::applied) {
          newlyEligible = std::move(scheduled.dispatches);
        }
        momWork = ambassadorJoinedFederateMomConditionalWorkFor(
            registry,
            *federationName,
            *targetFederateId,
            {umbra::detail::hla::utf8::mom::time_regulating});
      }
    }

    switch (disableStatus) {
      case umbra::detail::FederateTimeDisableStatus::applied:
        break;
      case umbra::detail::FederateTimeDisableStatus::not_enabled:
        throw TimeRegulationIsNotEnabled(
            L"Time regulation is not enabled for the target federate.");
      case umbra::detail::FederateTimeDisableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The Disable Time Regulation MOM request target is no longer active.");
    }

    if (momWork) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
          momWork->federationName,
          momWork->objectInstanceHandle,
          std::move(momWork->attributeHandles));
    }
    if (*targetFederateId == *producingFederateId) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"DisableTimeRegulation",
          umbra::detail::MomServiceType::time_management,
          {},
          true);
    }
    submitAmbassadorTimeAdvanceGrantDispatches(std::move(newlyEligible));

    auto const sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData const copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
      auto const recordStatus =
          embeddedFederationRegistry().recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (recordStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost Send Interaction membership while applying HLAdisableTimeRegulation.");
      }
    }
    return true;
  };

  if (handleMomDisableTimeRegulationRequest()) {
    return true;
  }

  // HLAenableTimeConstrained starts the target's normal asynchronous role
  // transition; the time coordinator owns delivery of the eventual callback.
  auto const handleMomEnableTimeConstrainedRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::shared_ptr<umbra::detail::FederateTimeState> targetTimeState;
    auto enableStatus = umbra::detail::FederateTimeEnableStatus::inactive;
    std::vector<umbra::detail::FederationTimeGrantDispatch> grantDispatches;
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
      auto const isEnableTimeConstrained =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::enable_time_constrained);
      if (!isEnableTimeConstrained) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the Enable Time Constrained MOM class hierarchy.");
      }
      if (!*isEnableTimeConstrained) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"An Enable Time Constrained MOM request received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"An Enable Time Constrained MOM request received an unknown parameter.");
        }
        if (*parameterName != umbra::detail::hla::utf8::mom::federate) {
          continue;
        }
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                  parameterValue);
          targetFederateId =
              ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                  decodedHandle);
        } catch (Exception const&) {
          throw FederateNotExecutionMember(
              L"An Enable Time Constrained MOM request supplied an invalid HLAfederate handle.");
        }
        if (!targetFederateId) {
          throw FederateNotExecutionMember(
              L"An Enable Time Constrained MOM request supplied an invalid HLAfederate handle.");
        }
      }
      if (!targetFederateId) {
        throw InteractionParameterNotDefined(
            L"An Enable Time Constrained MOM request requires HLAfederate.");
      }
      if (!registry.memberById(*federationName, *targetFederateId)) {
        throw FederateNotExecutionMember(
            L"An Enable Time Constrained MOM request requires a joined target federate.");
      }
      targetTimeState = registry.timeStateFor(*federationName, *targetFederateId);
      if (!targetTimeState) {
        throw FederateNotExecutionMember(
            L"An Enable Time Constrained MOM request target has no active time state.");
      }

      auto const request = targetTimeState->requestTimeConstrained();
      enableStatus = request.status;
      if (enableStatus == umbra::detail::FederateTimeEnableStatus::applied) {
        auto scheduled = registry.reevaluateTimeAdvanceGrants(*federationName);
        if (scheduled.status != umbra::detail::FederationTimeGrantStatus::applied) {
          throw RTIinternalError(
              L"The federation could not schedule the accepted Enable Time Constrained request.");
        }
        grantDispatches = std::move(scheduled.dispatches);
      }
    }

    if (enableStatus == umbra::detail::FederateTimeEnableStatus::applied &&
        *targetFederateId == *producingFederateId) {
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"EnableTimeConstrained",
          umbra::detail::MomServiceType::time_management,
          {},
          true);
    }
    switch (enableStatus) {
      case umbra::detail::FederateTimeEnableStatus::applied:
        break;
      case umbra::detail::FederateTimeEnableStatus::time_advance_pending:
        throw InTimeAdvancingState(
            L"Enable Time Constrained cannot run while the target has a time advance pending.");
      case umbra::detail::FederateTimeEnableStatus::request_pending:
        throw RequestForTimeConstrainedPending(
            L"The target federate already has an Enable Time Constrained request awaiting its callback.");
      case umbra::detail::FederateTimeEnableStatus::already_enabled:
        throw TimeConstrainedAlreadyEnabled(
            L"Time constrained is already enabled for the target federate.");
      case umbra::detail::FederateTimeEnableStatus::invalid_lookahead:
        throw RTIinternalError(
            L"The embedded time-constrained request returned an unexpected lookahead failure.");
      case umbra::detail::FederateTimeEnableStatus::inactive:
        throw FederateNotExecutionMember(
            L"The Enable Time Constrained MOM request target is no longer active.");
      case umbra::detail::FederateTimeEnableStatus::generation_exhausted:
        throw RTIinternalError(
            L"Umbra exhausted the embedded temporal-request generation space.");
    }

    submitAmbassadorTimeAdvanceGrantDispatches(std::move(grantDispatches));
    auto const sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData const copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
      auto const recordStatus =
          embeddedFederationRegistry().recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (recordStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost Send Interaction membership while applying HLAenableTimeConstrained.");
      }
    }
    return true;
  };

  if (handleMomEnableTimeConstrainedRequest()) {
    return true;
  }

  // Enabling regulation requires both the requesting federate and its
  // lookahead. Reuse the public service path so implicit-advance and callback
  // semantics remain identical to an RTIambassador call.
  auto const handleMomEnableTimeRegulationRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::optional<std::vector<std::uint8_t>> encodedLookahead;
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
      auto const isEnableTimeRegulation =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::enable_time_regulation);
      if (!isEnableTimeRegulation) {
        throw RTIinternalError(
            L"The embedded federation cannot resolve the Enable Time Regulation MOM class hierarchy.");
      }
      if (!*isEnableTimeRegulation) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"An Enable Time Regulation MOM request received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"An Enable Time Regulation MOM request received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                    parameterValue);
            targetFederateId =
                ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                    decodedHandle);
          } catch (Exception const&) {
            throw FederateNotExecutionMember(
                L"An Enable Time Regulation MOM request supplied an invalid HLAfederate handle.");
          }
          if (!targetFederateId) {
            throw FederateNotExecutionMember(
                L"An Enable Time Regulation MOM request supplied an invalid HLAfederate handle.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::lookahead) {
          encodedLookahead =
              umbra::detail::variable_length_data_2025::copyBytes(parameterValue);
          continue;
        }
        // Compatible MIM extensions may add parameters; only the standard
        // federate designator and lookahead are consumed by this service.
      }
      if (!targetFederateId || !encodedLookahead) {
        throw InteractionParameterNotDefined(
            L"An Enable Time Regulation MOM request requires HLAfederate and HLAlookahead.");
      }
      if (*targetFederateId != *producingFederateId) {
        throw FederateNotExecutionMember(
            L"Enable Time Regulation must identify the joined federate sending the service request.");
      }
      if (!federateTimeState_) {
        throw FederateNotExecutionMember(
            L"Enable Time Regulation requires initialized logical time for the sending federate.");
      }
      timeImplementationName = federateTimeState_->implementationName();
    }

    auto const timeFactory =
        umbra::detail::makeFederationLogicalTimeFactory(*timeImplementationName);
    if (!timeFactory || timeFactory->getName() != *timeImplementationName) {
      throw InvalidLookahead(
          L"The Enable Time Regulation MOM request has no available logical-time implementation.");
    }
    VariableLengthData encodedLookaheadValue;
    if (!encodedLookahead->empty()) {
      encodedLookaheadValue.setData(
          encodedLookahead->data(), encodedLookahead->size());
    }
    std::unique_ptr<LogicalTimeInterval> lookahead;
    try {
      lookahead = timeFactory->decodeLogicalTimeInterval(encodedLookaheadValue);
    } catch (Exception const&) {
      throw InvalidLookahead(
          L"The Enable Time Regulation MOM request supplied an invalid HLAlookahead value.");
    }
    if (!lookahead || lookahead->implementationName() != *timeImplementationName) {
      throw InvalidLookahead(
          L"The Enable Time Regulation MOM request supplied an invalid HLAlookahead value.");
    }

    enableTimeRegulation(*lookahead);

    auto const sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData const copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
      auto const recordStatus =
          embeddedFederationRegistry().recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (recordStatus != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost Send Interaction membership while applying HLAenableTimeRegulation.");
      }
    }
    return true;
  };

  if (handleMomEnableTimeRegulationRequest()) {
    return true;
  }

  // The MIM local-delete request forgets the selected object only at the
  // invoking federate, so route it through the public local-delete service.
  auto const handleMomLocalDeleteObjectInstanceRequest = [&]() {
    std::optional<ObjectInstanceHandle> objectInstance;
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
      auto const isLocalDeleteObjectInstance =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::local_delete_object_instance);
      if (!isLocalDeleteObjectInstance || !*isLocalDeleteObjectInstance) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"HLAlocalDeleteObjectInstance received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAlocalDeleteObjectInstance received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
          try {
            objectInstance =
                ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(
                    parameterValue);
          } catch (Exception const&) {
            throw InteractionParameterNotDefined(
                L"HLAlocalDeleteObjectInstance received an invalid HLAobjectInstance value.");
          }
          if (!::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                  *objectInstance)) {
            throw InteractionParameterNotDefined(
                L"HLAlocalDeleteObjectInstance received an invalid HLAobjectInstance value.");
          }
        }
        // Compatible MIM extensions may add parameters; the standard object
        // instance designator is the only value consumed by this service.
      }
      if (!objectInstance) {
        throw InteractionParameterNotDefined(
            L"HLAlocalDeleteObjectInstance requires HLAobjectInstance.");
      }
    }

    localDeleteObjectInstance(*objectInstance);

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
            L"The embedded federation lost Send Interaction membership while applying HLAlocalDeleteObjectInstance.");
      }
    }
    return true;
  };

  if (handleMomLocalDeleteObjectInstanceRequest()) {
    return true;
  }

  // HLAdeleteObjectInstance is a subscribed MIM service interaction. Treat it
  // as the corresponding Delete Object Instance invocation, consuming an
  // optional timestamp and discarding any returned retraction designator.
  auto const handleMomDeleteObjectInstanceRequest = [&]() {
    std::optional<ObjectInstanceHandle> objectInstance;
    std::optional<VariableLengthData> deletionTag;
    std::optional<std::vector<std::uint8_t>> encodedTimestamp;
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
      auto const isDeleteObjectInstance =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::delete_object_instance);
      if (!isDeleteObjectInstance || !*isDeleteObjectInstance) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"HLAdeleteObjectInstance received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAdeleteObjectInstance received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
          try {
            objectInstance =
                ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(
                    parameterValue);
          } catch (Exception const&) {
            throw InteractionParameterNotDefined(
                L"HLAdeleteObjectInstance received an invalid HLAobjectInstance value.");
          }
          if (!::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                  *objectInstance)) {
            throw InteractionParameterNotDefined(
                L"HLAdeleteObjectInstance received an invalid HLAobjectInstance value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::user_tag) {
          deletionTag = parameterValue;
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::time_stamp) {
          encodedTimestamp =
              umbra::detail::variable_length_data_2025::copyBytes(parameterValue);
          if (!federateTimeState_) {
            throw InvalidLogicalTime(
                L"HLAdeleteObjectInstance supplied a timestamp without an active logical-time implementation.");
          }
          timeImplementationName = federateTimeState_->implementationName();
          continue;
        }
        // Compatible MIM extensions may add parameters; the standard object,
        // tag, and optional time stamp are the only values consumed here.
      }

      if (!objectInstance || !deletionTag) {
        throw InteractionParameterNotDefined(
            L"HLAdeleteObjectInstance requires HLAobjectInstance and HLAtag.");
      }
    }

    auto copiedTag = *deletionTag;
    if (encodedTimestamp) {
      auto const timeFactory =
          umbra::detail::makeFederationLogicalTimeFactory(*timeImplementationName);
      if (!timeFactory || timeFactory->getName() != *timeImplementationName) {
        throw InvalidLogicalTime(
            L"HLAdeleteObjectInstance supplied a timestamp for an unavailable logical-time implementation.");
      }
      VariableLengthData timestampEncoding;
      if (!encodedTimestamp->empty()) {
        timestampEncoding.setData(
            encodedTimestamp->data(), encodedTimestamp->size());
      }
      std::unique_ptr<LogicalTime> timestamp;
      try {
        timestamp = timeFactory->decodeLogicalTime(timestampEncoding);
      } catch (Exception const&) {
        throw InvalidLogicalTime(
            L"HLAdeleteObjectInstance received an invalid HLAtimeStamp value.");
      }
      if (!timestamp || timestamp->implementationName() != *timeImplementationName) {
        throw InvalidLogicalTime(
            L"HLAdeleteObjectInstance received an invalid HLAtimeStamp value.");
      }
      static_cast<void>(deleteObjectInstance(
          *objectInstance, copiedTag, *timestamp));
    } else {
      deleteObjectInstance(*objectInstance, copiedTag);
    }

    std::vector<InteractionParameterValue> sentParameters =
        ambassadorCopyInteractionParameterValues(parameterValues);
    VariableLengthData copiedInteractionTag(userSuppliedTag);
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
         umbra::detail::formatMomUserSuppliedTag(copiedInteractionTag)},
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
            L"The embedded federation lost Send Interaction membership while applying HLAdeleteObjectInstance.");
      }
    }
    return true;
  };

  if (handleMomDeleteObjectInstanceRequest()) {
    return true;
  }

  // HLAchangeInteractionOrderType is a subscribed MIM service interaction.
  // Apply its declaration service semantics before ordinary Send Interaction
  // planning can mistake this RTI control request for application traffic.
  auto const handleMomChangeInteractionOrderTypeRequest = [&]() {
    std::optional<std::uint64_t> requestedInteractionClass;
    std::optional<OrderType> sendOrder;
    std::optional<std::vector<umbra::detail::MomServiceArgument>>
        changeOrderReportArguments;
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
      auto const isChangeInteractionOrderType =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::change_interaction_order_type);
      if (!isChangeInteractionOrderType || !*isChangeInteractionOrderType) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"HLAchangeInteractionOrderType received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAchangeInteractionOrderType received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class) {
          try {
            auto const decoded =
                ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(
                    parameterValue);
            requestedInteractionClass =
                ::rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                    decoded);
          } catch (Exception const&) {
            throw InteractionParameterNotDefined(
                L"HLAchangeInteractionOrderType received an invalid HLAinteractionClass value.");
          }
          if (!requestedInteractionClass) {
            throw InteractionParameterNotDefined(
                L"HLAchangeInteractionOrderType received an invalid HLAinteractionClass value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::send_order) {
          auto const encodedOrder = decodeHlaInteger32BE(parameterValue);
          if (!encodedOrder || (*encodedOrder != 1U && *encodedOrder != 2U)) {
            throw InteractionParameterNotDefined(
                L"HLAchangeInteractionOrderType received an invalid HLAsendOrder value.");
          }
          sendOrder = *encodedOrder == 1U ? RECEIVE : TIMESTAMP;
          continue;
        }
        // Compatible MIM extensions may add parameters; these are the two
        // standard Change Interaction Order Type arguments consumed here.
      }

      if (!requestedInteractionClass || !sendOrder) {
        throw InteractionParameterNotDefined(
            L"HLAchangeInteractionOrderType requires HLAinteractionClass and HLAsendOrder.");
      }
      requireFederationServiceOperationAvailable(L"Change Interaction Order Type");
      auto const status = registry.changeInteractionOrderType(
          *federationName,
          *producingFederateId,
          *requestedInteractionClass,
          *sendOrder);
      if (status != umbra::detail::InteractionOrderTypeChangeStatus::applied) {
        throwInteractionOrderTypeChangeFailure(status);
      }
      auto const requestedHandle =
          ::rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
              *requestedInteractionClass);
      changeOrderReportArguments = std::vector<umbra::detail::MomServiceArgument>{
          {umbra::detail::MomArgumentType::interaction_class_handle,
           L"Interaction class designator",
           umbra::detail::formatMomInteractionClassHandle(requestedHandle)},
          {umbra::detail::MomArgumentType::order_type,
           L"Order type",
           umbra::detail::formatMomOrderType(*sendOrder)},
      };
    }

    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ChangeInteractionOrderType",
        umbra::detail::MomServiceType::declaration_management,
        *changeOrderReportArguments,
        true);
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
            L"The embedded federation lost Send Interaction membership while applying HLAchangeInteractionOrderType.");
      }
    }
    return true;
  };

  if (handleMomChangeInteractionOrderTypeRequest()) {
    return true;
  }

  // HLAchangeAttributeOrderType is a subscribed MIM service interaction.
  // Interpret it as the corresponding declaration service before the ordinary
  // Send Interaction publication path can treat it as application traffic.
  // The inherited HLAfederate parameter identifies the member whose service
  // context must be used; it is not necessarily the interaction sender.
  auto const handleMomChangeAttributeOrderTypeRequest = [&]() {
    std::optional<std::uint64_t> targetFederateId;
    std::optional<ObjectInstanceHandle> objectInstance;
    std::optional<std::set<std::uint64_t>> attributeValues;
    std::optional<OrderType> sendOrder;
    std::optional<std::vector<umbra::detail::MomServiceArgument>>
        changeOrderReportArguments;
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
      auto const isChangeAttributeOrderType =
          registry.interactionClassIsSameOrDescendantOf(
              *federationName,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::change_attribute_order_type);
      if (!isChangeAttributeOrderType || !*isChangeAttributeOrderType) {
        return false;
      }

      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandle) {
          throw InteractionParameterNotDefined(
              L"HLAchangeAttributeOrderType received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandle);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAchangeAttributeOrderType received an unknown parameter.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(
                    parameterValue);
            targetFederateId =
                ::rti1516_2025::umbra_binding_detail::federateHandleValue(
                    decodedHandle);
          } catch (Exception const&) {
            throw FederateNotExecutionMember(
                L"HLAchangeAttributeOrderType received an invalid HLAfederate value.");
          }
          if (!targetFederateId) {
            throw FederateNotExecutionMember(
                L"HLAchangeAttributeOrderType received an invalid HLAfederate value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
          try {
            objectInstance =
                ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(
                    parameterValue);
          } catch (Exception const&) {
            throw InteractionParameterNotDefined(
                L"HLAchangeAttributeOrderType received an invalid HLAobjectInstance value.");
          }
          if (!::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                  *objectInstance)) {
            throw InteractionParameterNotDefined(
                L"HLAchangeAttributeOrderType received an invalid HLAobjectInstance value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::attribute_list) {
          try {
            rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet>
                handlePrototype;
            rti1516_2025::HLAvariableArray handles{
                static_cast<rti1516_2025::DataElement const&>(handlePrototype)};
            handles.decode(parameterValue);
            std::set<std::uint64_t> decodedAttributes;
            for (std::size_t index = 0U; index < handles.size(); ++index) {
              auto const* encodedHandle = dynamic_cast<
                  rti1516_2025::HLAvariableArray const*>(&handles.get(index));
              if (encodedHandle == nullptr) {
                throw InteractionParameterNotDefined(
                    L"HLAchangeAttributeOrderType received a malformed HLAattributeList value.");
              }
              auto const decoded =
                  ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(
                      encodedHandle->encode());
              auto const value =
                  ::rti1516_2025::umbra_binding_detail::attributeHandleValue(
                      decoded);
              if (!value) {
                throw InteractionParameterNotDefined(
                    L"HLAchangeAttributeOrderType received an invalid attribute handle.");
              }
              decodedAttributes.insert(*value);
            }
            attributeValues = std::move(decodedAttributes);
          } catch (InteractionParameterNotDefined const&) {
            throw;
          } catch (Exception const&) {
            throw InteractionParameterNotDefined(
                L"HLAchangeAttributeOrderType received a malformed HLAattributeList value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::send_order) {
          auto const encodedOrder = decodeHlaInteger32BE(parameterValue);
          if (!encodedOrder || (*encodedOrder != 1U && *encodedOrder != 2U)) {
            throw InteractionParameterNotDefined(
                L"HLAchangeAttributeOrderType received an invalid HLAsendOrder value.");
          }
          sendOrder = *encodedOrder == 1U ? RECEIVE : TIMESTAMP;
          continue;
        }
        // Additional parameters on a compatible derived MIM interaction do
        // not change the three standard Change Attribute Order Type inputs.
      }

      if (!targetFederateId || !objectInstance || !attributeValues || !sendOrder) {
        throw InteractionParameterNotDefined(
            L"HLAchangeAttributeOrderType requires HLAfederate, HLAobjectInstance, HLAattributeList, and HLAsendOrder.");
      }
      if (!registry.memberById(*federationName, *targetFederateId)) {
        throw FederateNotExecutionMember(
            L"HLAchangeAttributeOrderType requires a joined target federate.");
      }

      AttributeHandleSet attributes;
      for (auto const value : *attributeValues) {
        attributes.insert(
            ::rti1516_2025::umbra_binding_detail::makeAttributeHandle(value));
      }
      requireFederationServiceOperationAvailable(L"Change Attribute Order Type");
      auto const objectInstanceValue =
          ::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
              *objectInstance);
      auto const attributeHandleValues = ambassadorAttributeHandleValues(attributes);
      if (!objectInstanceValue || !attributeHandleValues) {
        throw InteractionParameterNotDefined(
            L"HLAchangeAttributeOrderType contains an invalid standard handle value.");
      }
      auto const status = registry.changeAttributeOrderType(
          *federationName,
          *targetFederateId,
          *objectInstanceValue,
          *attributeHandleValues,
          *sendOrder);
      if (status != umbra::detail::AttributeOrderTypeChangeStatus::applied) {
        throwAttributeOrderTypeChangeFailure(status);
      }

      changeOrderReportArguments = std::vector<umbra::detail::MomServiceArgument>{
          {umbra::detail::MomArgumentType::object_instance_handle,
           L"Object instance designator",
           umbra::detail::formatMomObjectInstanceHandle(*objectInstance)},
          {umbra::detail::MomArgumentType::attribute_handle_set,
           L"Set of attribute designators",
           umbra::detail::formatMomAttributeHandleSet(attributes)},
          {umbra::detail::MomArgumentType::order_type,
           L"Order type",
           umbra::detail::formatMomOrderType(*sendOrder)},
      };
    }

    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"ChangeAttributeOrderType",
        umbra::detail::MomServiceType::declaration_management,
        *changeOrderReportArguments,
        true);
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
            L"The embedded federation lost Send Interaction membership while applying HLAchangeAttributeOrderType.");
      }
    }
    return true;
  };

  if (handleMomChangeAttributeOrderTypeRequest()) {
    return true;
  }

  // The 2025 MIM exposes transportation-type changes as subscribed MOM
  // interactions.  They are control requests, not ordinary application
  // interactions, so they must be consumed before planCurrentRequest() (which
  // quite correctly requires an application publication).  Reuse the same
  // registry plans and confirmation queues as the corresponding public
  // services so the callback remains the commit boundary.
  auto const handleMomTransportationTypeChangeRequest = [&]() {
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

    auto const isAttributeRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_attribute_transportation_type_change);
    auto const isInteractionRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::request_interaction_transportation_type_change);
    // Older or deliberately reduced development FOMs may omit these two
    // optional MIM request classes.  In that case this handler simply does
    // not claim the interaction; the ordinary Send Interaction path remains
    // responsible for its normal publication/error semantics.
    if (!isAttributeRequest || !isInteractionRequest) {
      return false;
    }
    if (!*isAttributeRequest && !*isInteractionRequest) {
      return false;
    }
    if (*isAttributeRequest && *isInteractionRequest) {
      throw RTIinternalError(
          L"The embedded federation gives a transportation-type MOM request ambiguous semantics.");
    }

    auto transportationNameForValue = [&](std::uint64_t value)
        -> std::optional<std::string> {
      return registry.transportationTypeNameFor(*federationName, value);
    };

    if (*isAttributeRequest) {
      std::optional<std::uint64_t> objectInstanceValue;
      std::optional<std::set<std::uint64_t>> attributeValues;
      std::optional<std::string> transportationName;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLArequestAttributeTransportationTypeChange received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLArequestAttributeTransportationTypeChange received an unknown parameter for this interaction class.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(
                    parameterValue);
            objectInstanceValue =
                ::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(
                    decodedHandle);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLArequestAttributeTransportationTypeChange received an invalid HLAobjectInstanceHandle value.");
          }
          if (!objectInstanceValue) {
            throw RTIinternalError(
                L"HLArequestAttributeTransportationTypeChange received an invalid HLAobjectInstanceHandle value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::attribute_list) {
          try {
            rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet>
                handlePrototype;
            rti1516_2025::HLAvariableArray handles{
                static_cast<rti1516_2025::DataElement const&>(handlePrototype)};
            handles.decode(parameterValue);
            std::set<std::uint64_t> decodedAttributes;
            for (std::size_t index = 0U; index < handles.size(); ++index) {
              auto const* encodedHandle = dynamic_cast<
                  rti1516_2025::HLAvariableArray const*>(&handles.get(index));
              if (encodedHandle == nullptr) {
                throw RTIinternalError(
                    L"HLArequestAttributeTransportationTypeChange received a malformed HLAattributeHandleList value.");
              }
              auto const decoded =
                  ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(
                      encodedHandle->encode());
              auto const value =
                  ::rti1516_2025::umbra_binding_detail::attributeHandleValue(decoded);
              if (!value) {
                throw RTIinternalError(
                    L"HLArequestAttributeTransportationTypeChange received an invalid HLAattributeHandle value.");
              }
              decodedAttributes.insert(*value);
            }
            attributeValues = std::move(decodedAttributes);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLArequestAttributeTransportationTypeChange received a malformed HLAattributeHandleList value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::transportation) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle(
                    parameterValue);
            auto const value =
                ::rti1516_2025::umbra_binding_detail::transportationTypeHandleValue(
                    decodedHandle);
            if (!value) {
              throw RTIinternalError(
                  L"HLArequestAttributeTransportationTypeChange received an invalid HLAtransportationTypeHandle value.");
            }
            transportationName = transportationNameForValue(*value);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLArequestAttributeTransportationTypeChange received an invalid HLAtransportationTypeHandle value.");
          }
          if (!transportationName) {
            throw InvalidTransportationTypeHandle(
                L"HLArequestAttributeTransportationTypeChange received a transportation type that is not declared in this federation execution.");
          }
          continue;
        }
        // A compatible MOM extension may add parameters to this interaction;
        // the three predefined values above are the only values consumed.
      }
      if (!objectInstanceValue || !attributeValues || !transportationName) {
        throw InteractionParameterNotDefined(
            L"HLArequestAttributeTransportationTypeChange requires HLAobjectInstance, HLAattributeList, and HLAtransportation parameters.");
      }
      auto plan = registry.planAttributeTransportationTypeChange(
          *federationName,
          *producingFederateId,
          *objectInstanceValue,
          *attributeValues,
          *transportationName);
      if (plan.status !=
          umbra::detail::AttributeTransportationTypeChangeStatus::applied) {
        throwAttributeTransportationTypeChangeFailure(plan.status);
      }
      momAttributeTransportationTypeChange = std::move(plan);
      return true;
    }

    std::optional<std::uint64_t> interactionClassValue;
    std::optional<std::string> transportationName;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionTransportationTypeChange received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLArequestInteractionTransportationTypeChange received an unknown parameter for this interaction class.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(
                  parameterValue);
          interactionClassValue =
              ::rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                  decodedHandle);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLArequestInteractionTransportationTypeChange received an invalid HLAinteractionClassHandle value.");
        }
        if (!interactionClassValue) {
          throw RTIinternalError(
              L"HLArequestInteractionTransportationTypeChange received an invalid HLAinteractionClassHandle value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::transportation) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeTransportationTypeHandle(
                  parameterValue);
          auto const value =
              ::rti1516_2025::umbra_binding_detail::transportationTypeHandleValue(
                  decodedHandle);
          if (!value) {
            throw RTIinternalError(
                L"HLArequestInteractionTransportationTypeChange received an invalid HLAtransportationTypeHandle value.");
          }
          transportationName = transportationNameForValue(*value);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLArequestInteractionTransportationTypeChange received an invalid HLAtransportationTypeHandle value.");
        }
        if (!transportationName) {
          throw InvalidTransportationTypeHandle(
              L"HLArequestInteractionTransportationTypeChange received a transportation type that is not declared in this federation execution.");
        }
        continue;
      }
      // A compatible MOM extension may add parameters to this interaction;
      // the two predefined values above are the only values consumed.
    }
    if (!interactionClassValue || !transportationName) {
      throw InteractionParameterNotDefined(
          L"HLArequestInteractionTransportationTypeChange requires HLAinteractionClass and HLAtransportation parameters.");
    }
    auto plan = registry.planInteractionTransportationTypeChange(
        *federationName,
        *producingFederateId,
        *interactionClassValue,
        *transportationName);
    if (plan.status !=
        umbra::detail::InteractionTransportationTypeChangeStatus::applied) {
      throwInteractionTransportationTypeChangeFailure(plan.status);
    }
    momInteractionTransportationTypeChange = std::move(plan);
    return true;
  };

  if (handleMomTransportationTypeChangeRequest()) {
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
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted transportation-type MOM request boundary.");
      }
    }
    if (momAttributeTransportationTypeChange) {
      auto plan = std::move(*momAttributeTransportationTypeChange);
      if (plan.requestId != 0U) {
        if (!plan.callbackRoute) {
          throw RTIinternalError(
              L"The embedded federation has no attribute transportation confirmation callback route.");
        }
        queueAmbassadorConfirmAttributeTransportationTypeChange(
            std::move(plan.callbackRoute),
            std::move(*federationName),
            *producingFederateId,
            plan.requestId);
      }
    } else {
      auto plan = std::move(*momInteractionTransportationTypeChange);
      if (!plan.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has no interaction transportation confirmation callback route.");
      }
      queueAmbassadorConfirmInteractionTransportationTypeChange(
          std::move(plan.callbackRoute),
          std::move(*federationName),
          *producingFederateId,
          plan.interactionClassHandle);
    }
    return true;
  }

  auto const handleMomSwitchAdjustment = [&]() {
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
    auto const isFederateSetTiming = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::federate_set_timing);
    if (!isFederateSetTiming) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAsetTiming MOM class hierarchy.");
    }
    if (*isFederateSetTiming) {
      std::optional<std::uint64_t> targetFederateId;
      std::optional<std::int32_t> reportPeriodSeconds;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLAsetTiming received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAsetTiming received an unknown parameter for this interaction class.");
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
          try {
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
            targetFederateId =
                ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAfederateReference value.");
          }
          if (!targetFederateId) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAfederateReference value.");
          }
          continue;
        }
        if (*parameterName == umbra::detail::hla::utf8::mom::report_period) {
          rti1516_2025::HLAinteger32BE decodedPeriod;
          try {
            decodedPeriod.decode(parameterValue);
          } catch (Exception const&) {
            throw RTIinternalError(
                L"HLAsetTiming received an invalid HLAseconds HLAinteger32BE value.");
          }
          reportPeriodSeconds = decodedPeriod.get();
          continue;
        }
        // A compatible MOM extension may carry an additional parameter.  It
        // is received but does not alter the predefined HLAsetTiming state.
      }
      if (!targetFederateId || !reportPeriodSeconds) {
        throw InteractionParameterNotDefined(
            L"HLAsetTiming requires both HLAfederate and HLAreportPeriod parameters.");
      }
      auto const result = registry.setFederateMomReportPeriod(
          *federationName,
          *producingFederateId,
          *targetFederateId,
          *reportPeriodSeconds);
      switch (result) {
        case umbra::detail::FederateMOMTimingUpdateStatus::applied:
          return true;
        case umbra::detail::FederateMOMTimingUpdateStatus::federation_does_not_exist:
        case umbra::detail::FederateMOMTimingUpdateStatus::requesting_federate_not_member:
        case umbra::detail::FederateMOMTimingUpdateStatus::target_federate_not_member:
          throw FederateNotExecutionMember(
              L"HLAsetTiming requires both the requesting and target federates to be joined.");
        case umbra::detail::FederateMOMTimingUpdateStatus::invalid_report_period:
          throw RTIinternalError(
              L"HLAsetTiming requires a non-negative HLAreportPeriod value.");
      }
      throw RTIinternalError(
          L"The embedded federation rejected the HLAsetTiming adjustment.");
    }
    auto const isFederationSetSwitches = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::federation_set_switches);
    auto const isFederateSetSwitches = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::federate_set_switches);
    if (!isFederationSetSwitches || !isFederateSetSwitches) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAsetSwitches MOM class hierarchy.");
    }

    if (*isFederationSetSwitches) {
      if (parameterValues.empty()) {
        throw InteractionParameterNotDefined(
            L"The federation HLAsetSwitches interaction requires at least one declared parameter.");
      }
      std::optional<bool> autoProvideSwitchValue;
      for (auto const& [parameterHandle, parameterValue] : parameterValues) {
        auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
        if (!suppliedParameterHandleValue) {
          throw InteractionParameterNotDefined(
              L"HLAsetSwitches received an invalid parameter handle.");
        }
        auto const parameterName = registry.parameterNameFor(
            *federationName,
            *interactionClassName,
            *suppliedParameterHandleValue);
        if (!parameterName) {
          throw InteractionParameterNotDefined(
              L"HLAsetSwitches received an unknown parameter for this interaction class.");
        }
        if (*parameterName != umbra::detail::hla::utf8::mom::auto_provide) {
          // A compatible MOM extension may carry a parameter on this class
          // or its subclass.  The RTI must receive it but process only the
          // predefined HLAautoProvide value.
          continue;
        }
        auto const switchValue = decodeHlaSwitch(parameterValue);
        if (!switchValue) {
          throw RTIinternalError(
              L"HLAsetSwitches received an invalid HLAswitch HLAinteger32BE value.");
        }
        autoProvideSwitchValue = *switchValue;
      }
      if (!autoProvideSwitchValue) {
        return true;
      }
      auto const previousAutoProvide = registry.autoProvideSwitchFor(
          *federationName,
          *producingFederateId);
      auto const result = registry.setAutoProvideSwitch(
          *federationName,
          *producingFederateId,
          *autoProvideSwitchValue);
      switch (result) {
        case umbra::detail::FederationRegistryStatus::applied:
          if (previousAutoProvide &&
              *previousAutoProvide != *autoProvideSwitchValue) {
            federationMomSwitchWork = ambassadorFederationMomConditionalWorkFor(
                registry,
                *federationName,
                {umbra::detail::hla::utf8::mom::auto_provide});
          }
          return true;
        case umbra::detail::FederationRegistryStatus::federation_does_not_exist:
        case umbra::detail::FederationRegistryStatus::federate_not_member:
          throw FederateNotExecutionMember(
              L"The embedded federation no longer records this RTI ambassador as a member.");
        default:
          throw RTIinternalError(
              L"The embedded federation rejected the HLAsetSwitches Auto Provide adjustment.");
      }
    }

    if (!*isFederateSetSwitches) {
      return false;
    }
    if (parameterValues.empty()) {
      throw InteractionParameterNotDefined(
          L"The joined-federate HLAsetSwitches interaction requires at least one declared parameter.");
    }

    umbra::detail::FederateMOMSwitchUpdate update;
    bool predefinedParameterSupplied = false;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLAsetSwitches received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAsetSwitches received an unknown parameter for this interaction class.");
      }

      if (*parameterName == umbra::detail::hla::utf8::mom::automatic_resign_action) {
        auto const resignAction = decodeHlaResignAction(parameterValue);
        if (!resignAction) {
          throw RTIinternalError(
              L"HLAsetSwitches received an invalid HLAresignAction HLAinteger32BE value.");
        }
        update.automaticResignAction = *resignAction;
        predefinedParameterSupplied = true;
        continue;
      }

      if (*parameterName != umbra::detail::hla::utf8::mom::object_class_relevance_advisory &&
          *parameterName != umbra::detail::hla::utf8::mom::attribute_relevance_advisory &&
          *parameterName != umbra::detail::hla::utf8::mom::attribute_scope_advisory &&
          *parameterName != umbra::detail::hla::utf8::mom::interaction_relevance_advisory &&
          *parameterName != umbra::detail::hla::utf8::mom::convey_region_designator_sets &&
          *parameterName != umbra::detail::hla::utf8::mom::service_reporting &&
          *parameterName != umbra::detail::hla::utf8::mom::exception_reporting &&
          *parameterName != umbra::detail::hla::utf8::mom::send_service_reports_to_file) {
        // MOM extensions may add parameters to a predefined interaction. IEEE
        // 1516.1-2025 §11.4.1 requires the RTI to receive those values but
        // process only the predefined parameters, so deliberately leave an
        // extension value opaque instead of imposing a local wire type.
        continue;
      }
      auto const switchValue = decodeHlaSwitch(parameterValue);
      if (!switchValue) {
        throw RTIinternalError(
            L"HLAsetSwitches received an invalid HLAswitch HLAinteger32BE value.");
      }
      predefinedParameterSupplied = true;
      if (*parameterName == umbra::detail::hla::utf8::mom::object_class_relevance_advisory) {
        update.objectClassRelevanceAdvisory = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::attribute_relevance_advisory) {
        update.attributeRelevanceAdvisory = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::attribute_scope_advisory) {
        update.attributeScopeAdvisory = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::interaction_relevance_advisory) {
        update.interactionRelevanceAdvisory = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::convey_region_designator_sets) {
        update.conveyRegionDesignatorSets = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::service_reporting) {
        update.serviceReporting = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::exception_reporting) {
        update.exceptionReporting = *switchValue;
      } else if (*parameterName == umbra::detail::hla::utf8::mom::send_service_reports_to_file) {
        update.sendServiceReportsToFile = *switchValue;
      }
    }

    if (!predefinedParameterSupplied) {
      throw InteractionParameterNotDefined(
          L"The promoted HLAsetSwitches interaction requires at least one predefined parameter.");
    }

    auto const result = registry.applyFederateMOMSwitchUpdate(
        *federationName,
        *producingFederateId,
        update);
    switch (result) {
      case umbra::detail::FederateMOMSwitchUpdateStatus::applied:
        {
          std::vector<std::string_view> changedAttributes;
          if (update.objectClassRelevanceAdvisory.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::object_class_relevance_advisory);
          }
          if (update.attributeRelevanceAdvisory.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::attribute_relevance_advisory);
          }
          if (update.attributeScopeAdvisory.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::attribute_scope_advisory);
          }
          if (update.interactionRelevanceAdvisory.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::interaction_relevance_advisory);
          }
          if (update.conveyRegionDesignatorSets.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::convey_region_designator_sets);
          }
          if (update.automaticResignAction.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::automatic_resign_action);
          }
          if (update.serviceReporting.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::service_reporting);
          }
          if (update.exceptionReporting.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::exception_reporting);
          }
          if (update.sendServiceReportsToFile.has_value()) {
            changedAttributes.emplace_back(umbra::detail::hla::utf8::mom::send_service_reports_to_file);
          }
          if (!changedAttributes.empty()) {
            momSwitchWork = ambassadorJoinedFederateMomConditionalWorkFor(
                registry,
                *federationName,
                *producingFederateId,
                std::move(changedAttributes));
          }
        }
        return true;
      case umbra::detail::FederateMOMSwitchUpdateStatus::federation_does_not_exist:
      case umbra::detail::FederateMOMSwitchUpdateStatus::federate_not_member:
        throw FederateNotExecutionMember(
            L"The embedded federation no longer records this RTI ambassador as a member.");
      case umbra::detail::FederateMOMSwitchUpdateStatus::invalid_resign_action:
        throw RTIinternalError(
            L"The embedded federation rejected an invalid HLAresignAction HLAsetSwitches value.");
      case umbra::detail::FederateMOMSwitchUpdateStatus::
          report_service_invocations_are_subscribed:
        // Preserve the rejected adjustment without mutating the switch state.
        // After this lock-held handler unwinds, Send Interaction's shared MOM
        // exception path emits the HLAsetSwitches-specific HLAreportMOMexception
        // with HLAparameterError=false.
        throw RTIinternalError(
            L"HLAsetSwitches cannot enable Service Reporting while report-service invocations are subscribed.");
      default:
        throw RTIinternalError(
            L"The embedded federation rejected the joined-federate HLAsetSwitches adjustment.");
    }
  };

  auto const handleMomAttributeStateAdjustment = [&]() {
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
    auto const isModifyAttributeState = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::federate_modify_attribute_state);
    if (!isModifyAttributeState) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAmodifyAttributeState MOM class hierarchy.");
    }
    if (!*isModifyAttributeState) {
      return false;
    }

    std::optional<std::uint64_t> targetFederateId;
    std::optional<std::uint64_t> objectInstanceHandle;
    std::optional<std::uint64_t> attributeHandle;
    std::optional<bool> owned;
    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandleValue = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandleValue) {
        throw InteractionParameterNotDefined(
            L"HLAmodifyAttributeState received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandleValue);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAmodifyAttributeState received an unknown parameter for this interaction class.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::federate) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeFederateHandle(parameterValue);
          targetFederateId =
              ::rti1516_2025::umbra_binding_detail::federateHandleValue(decodedHandle);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAfederateReference value.");
        }
        if (!targetFederateId) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAfederateReference value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeObjectInstanceHandle(parameterValue);
          objectInstanceHandle =
              ::rti1516_2025::umbra_binding_detail::objectInstanceHandleValue(decodedHandle);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAobjectInstanceHandle value.");
        }
        if (!objectInstanceHandle) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAobjectInstanceHandle value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::attribute) {
        try {
          auto const decodedHandle =
              ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(parameterValue);
          attributeHandle =
              ::rti1516_2025::umbra_binding_detail::attributeHandleValue(decodedHandle);
        } catch (Exception const&) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAattributeHandle value.");
        }
        if (!attributeHandle) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAattributeHandle value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::attribute_state) {
        owned = decodeHlaOwnership(parameterValue);
        if (!owned) {
          throw RTIinternalError(
              L"HLAmodifyAttributeState received an invalid HLAownership HLAinteger32BE value.");
        }
        continue;
      }
      // Compatible MOM extensions may add parameters to this interaction.
      // The predefined four values remain the only ones that affect state.
    }
    if (!targetFederateId || !objectInstanceHandle || !attributeHandle || !owned) {
      throw InteractionParameterNotDefined(
          L"HLAmodifyAttributeState requires HLAfederate, HLAobjectInstance, HLAattribute, and HLAattributeState parameters.");
    }

    auto const result = registry.setFederateMomAttributeState(
        *federationName,
        *producingFederateId,
        *targetFederateId,
        *objectInstanceHandle,
        *attributeHandle,
        *owned);
    switch (result) {
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::applied:
        return true;
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::federation_does_not_exist:
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::requesting_federate_not_member:
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::target_federate_not_member:
        throw FederateNotExecutionMember(
            L"HLAmodifyAttributeState requires both the requesting and target federates to be joined.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::object_instance_not_known:
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::target_does_not_know_object_instance:
        throw ObjectInstanceNotKnown(
            L"HLAmodifyAttributeState requires the target federate to know the object instance.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::attribute_not_defined:
        throw AttributeNotDefined(
            L"HLAmodifyAttributeState requires a defined attribute at the target's known class.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::target_attribute_not_published:
        throw AttributeNotPublished(
            L"HLAmodifyAttributeState requires the target federate to publish the attribute.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::attribute_owned_by_rti:
        throw RTIinternalError(
            L"HLAmodifyAttributeState cannot modify an attribute owned by the RTI.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::invalid_attribute_state:
        throw RTIinternalError(
            L"The embedded federation rejected an invalid HLAownership state.");
      case umbra::detail::FederateMOMAttributeStateUpdateStatus::inconsistent_catalog:
        throw RTIinternalError(
            L"The embedded federation cannot reconstruct the HLAmodifyAttributeState target.");
    }
    throw RTIinternalError(
        L"The embedded federation rejected the HLAmodifyAttributeState adjustment.");
  };

  if (handleMomAttributeStateAdjustment()) {
    // HLAmodifyAttributeState is an accepted MOM Send Interaction.  Its
    // synchronous ownership mutation produces no application callback and no
    // ownership notification, so only the sender-side interaction statistic
    // advances here.
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted HLAmodifyAttributeState boundary.");
      }
    }
    return true;
  }

  if (handleMomSwitchAdjustment()) {
    // HLAsetTiming/HLAsetSwitches are still successful Send Interaction
    // invocations. Count their accepted service boundary even though the
    // embedded profile consumes them as MOM adjustments without application
    // fan-out.
    {
      std::scoped_lock lock(ambassadorFederationManagementMutex());
      auto const status = embeddedFederationRegistry()
          .recordSuccessfulInteractionSend(
              *federationName,
              *producingFederateId,
              *interactionClassHandle,
              umbra::detail::hla::utf8::mom::reliable,
              false);
      if (status != umbra::detail::FederationRegistryStatus::applied) {
        throw RTIinternalError(
            L"The embedded federation lost the Send Interaction membership before its accepted MOM adjustment boundary.");
      }
    }
    if (momSwitchWork) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
          momSwitchWork->federationName,
          momSwitchWork->objectInstanceHandle,
          std::move(momSwitchWork->attributeHandles));
    }
    if (federationMomSwitchWork) {
      queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
          federationMomSwitchWork->federationName,
          federationMomSwitchWork->objectInstanceHandle,
          std::move(federationMomSwitchWork->attributeHandles));
    }
    return true;
  }
  return false;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
