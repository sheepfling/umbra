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
  // Transportation, switch, timing, and attribute-state MOM controls.

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
