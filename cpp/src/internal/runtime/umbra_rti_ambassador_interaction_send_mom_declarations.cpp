#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::handleEmbeddedMomInteractionDeclarationRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  if (!federationName || !producingFederateId || !interactionClassHandle) {
    return false;
  }

  std::optional<InteractionClassHandle> targetInteractionClass;
  bool unpublish = false;
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
    auto const isPublishInteractionClass =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::publish_interaction_class);
    auto const isUnpublishInteractionClass =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::unpublish_interaction_class);
    if (!isPublishInteractionClass || !isUnpublishInteractionClass) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the standard interaction publish/unpublish MOM class hierarchy.");
    }
    if (*isPublishInteractionClass == *isUnpublishInteractionClass) {
      return false;
    }
    unpublish = *isUnpublishInteractionClass;

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"HLApublishInteractionClass received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLApublishInteractionClass received an unknown parameter.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class) {
        try {
          targetInteractionClass =
              ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(
                  parameterValue);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLApublishInteractionClass received an invalid HLAinteractionClass value.");
        }
        if (!::rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                *targetInteractionClass)) {
          throw InteractionParameterNotDefined(
              L"HLApublishInteractionClass received an invalid HLAinteractionClass value.");
        }
      }
      // Compatible MIM extensions may add parameters; the standard
      // interaction-class designator is the value consumed by this service.
    }
    if (!targetInteractionClass) {
      throw InteractionParameterNotDefined(
          unpublish
              ? L"HLAunpublishInteractionClass requires HLAinteractionClass."
              : L"HLApublishInteractionClass requires HLAinteractionClass.");
    }
  }

  if (unpublish) {
    unpublishInteractionClass(*targetInteractionClass);
  } else {
    publishInteractionClass(*targetInteractionClass);
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
          unpublish
              ? L"The embedded federation lost Send Interaction membership while applying HLAunpublishInteractionClass."
              : L"The embedded federation lost Send Interaction membership while applying HLApublishInteractionClass.");
    }
  }
  return true;
}

bool UmbraRtiAmbassador::handleEmbeddedMomObjectAttributePublicationRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  if (!federationName || !producingFederateId || !interactionClassHandle) {
    return false;
  }

  std::optional<ObjectClassHandle> objectClass;
  std::optional<AttributeHandleSet> attributes;
  bool unpublish = false;
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
    auto const isPublishObjectClassAttributes =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::publish_object_class_attributes);
    auto const isUnpublishObjectClassAttributes =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::unpublish_object_class_attributes);
    if (!isPublishObjectClassAttributes || !isUnpublishObjectClassAttributes) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the standard object-attribute publish/unpublish MOM class hierarchy.");
    }
    if (*isPublishObjectClassAttributes == *isUnpublishObjectClassAttributes) {
      return false;
    }
    unpublish = *isUnpublishObjectClassAttributes;

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            unpublish
                ? L"HLAunpublishObjectClassAttributes received an invalid parameter handle."
                : L"HLApublishObjectClassAttributes received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            unpublish
                ? L"HLAunpublishObjectClassAttributes received an unknown parameter."
                : L"HLApublishObjectClassAttributes received an unknown parameter.");
      }
      if (*parameterName == "HLAobjectClass") {
        try {
          objectClass =
              ::rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
                  parameterValue);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              unpublish
                  ? L"HLAunpublishObjectClassAttributes received an invalid HLAobjectClass value."
                  : L"HLApublishObjectClassAttributes received an invalid HLAobjectClass value.");
        }
        if (!::rti1516_2025::umbra_binding_detail::objectClassHandleValue(
                *objectClass)) {
          throw InteractionParameterNotDefined(
              unpublish
                  ? L"HLAunpublishObjectClassAttributes received an invalid HLAobjectClass value."
                  : L"HLApublishObjectClassAttributes received an invalid HLAobjectClass value.");
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
                  L"HLApublishObjectClassAttributes received a malformed HLAattributeList value.");
            }
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeAttributeHandle(
                    encodedHandle->encode());
            auto const decodedValue =
                ::rti1516_2025::umbra_binding_detail::attributeHandleValue(
                    decodedHandle);
            if (!decodedValue) {
              throw RTIinternalError(
                  L"HLApublishObjectClassAttributes received an invalid attribute handle.");
            }
            decodedAttributes.insert(
                ::rti1516_2025::umbra_binding_detail::makeAttributeHandle(
                    *decodedValue));
          }
          attributes = std::move(decodedAttributes);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLApublishObjectClassAttributes received a malformed HLAattributeList value.");
        }
        continue;
      }
      // Compatible MIM extensions may add parameters; the two standard
      // service arguments above are the only values consumed here.
    }
    if (!objectClass || !attributes) {
      throw InteractionParameterNotDefined(
          unpublish
              ? L"HLAunpublishObjectClassAttributes requires HLAobjectClass and HLAattributeList."
              : L"HLApublishObjectClassAttributes requires HLAobjectClass and HLAattributeList.");
    }
  }

  if (unpublish) {
    unpublishObjectClassAttributes(*objectClass, *attributes);
  } else {
    publishObjectClassAttributes(*objectClass, *attributes);
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
          unpublish
              ? L"The embedded federation lost Send Interaction membership while applying HLAunpublishObjectClassAttributes."
              : L"The embedded federation lost Send Interaction membership while applying HLApublishObjectClassAttributes.");
    }
  }
  return true;
}

bool UmbraRtiAmbassador::handleEmbeddedMomObjectDirectedInteractionPublicationRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  if (!federationName || !producingFederateId || !interactionClassHandle) {
    return false;
  }

  std::optional<ObjectClassHandle> objectClass;
  std::optional<InteractionClassHandleSet> interactionClasses;
  bool unpublish = false;
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
    auto const isPublishDirectedInteractions =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::publish_object_class_directed_interactions);
    auto const isUnpublishDirectedInteractions =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::unpublish_object_class_directed_interactions);
    if (!isPublishDirectedInteractions || !isUnpublishDirectedInteractions) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the standard directed-interaction publish/unpublish MOM class hierarchy.");
    }
    if (*isPublishDirectedInteractions == *isUnpublishDirectedInteractions) {
      return false;
    }
    unpublish = *isUnpublishDirectedInteractions;

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            unpublish
                ? L"HLAunpublishObjectClassDirectedInteractions received an invalid parameter handle."
                : L"HLApublishObjectClassDirectedInteractions received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            unpublish
                ? L"HLAunpublishObjectClassDirectedInteractions received an unknown parameter."
                : L"HLApublishObjectClassDirectedInteractions received an unknown parameter.");
      }
      if (*parameterName == "HLAobjectClass") {
        try {
          objectClass =
              ::rti1516_2025::umbra_binding_detail::decodeObjectClassHandle(
                  parameterValue);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              unpublish
                  ? L"HLAunpublishObjectClassDirectedInteractions received an invalid HLAobjectClass value."
                  : L"HLApublishObjectClassDirectedInteractions received an invalid HLAobjectClass value.");
        }
        if (!::rti1516_2025::umbra_binding_detail::objectClassHandleValue(
                *objectClass)) {
          throw InteractionParameterNotDefined(
              unpublish
                  ? L"HLAunpublishObjectClassDirectedInteractions received an invalid HLAobjectClass value."
                  : L"HLApublishObjectClassDirectedInteractions received an invalid HLAobjectClass value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class_list) {
        try {
          rti1516_2025::HLAvariableArrayT<rti1516_2025::HLAoctet>
              handlePrototype;
          rti1516_2025::HLAvariableArray handles{
              static_cast<rti1516_2025::DataElement const&>(handlePrototype)};
          handles.decode(parameterValue);
          InteractionClassHandleSet decodedInteractionClasses;
          for (std::size_t index = 0U; index < handles.size(); ++index) {
            auto const* encodedHandle = dynamic_cast<
                rti1516_2025::HLAvariableArray const*>(&handles.get(index));
            if (encodedHandle == nullptr) {
              throw RTIinternalError(
                  L"HLApublishObjectClassDirectedInteractions received a malformed HLAinteractionClassList value.");
            }
            auto const decodedHandle =
                ::rti1516_2025::umbra_binding_detail::decodeInteractionClassHandle(
                    encodedHandle->encode());
            auto const decodedValue =
                ::rti1516_2025::umbra_binding_detail::interactionClassHandleValue(
                    decodedHandle);
            if (!decodedValue) {
              throw RTIinternalError(
                  L"HLApublishObjectClassDirectedInteractions received an invalid interaction class handle.");
            }
            decodedInteractionClasses.insert(
                ::rti1516_2025::umbra_binding_detail::makeInteractionClassHandle(
                    *decodedValue));
          }
          interactionClasses = std::move(decodedInteractionClasses);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLApublishObjectClassDirectedInteractions received a malformed HLAinteractionClassList value.");
        }
        continue;
      }
      // Compatible MIM extensions may add parameters; the standard object
      // class and interaction-class list are the only values consumed here.
    }
    if (!objectClass || !interactionClasses) {
      throw InteractionParameterNotDefined(
          unpublish
              ? L"HLAunpublishObjectClassDirectedInteractions requires HLAobjectClass and HLAinteractionClassList."
              : L"HLApublishObjectClassDirectedInteractions requires HLAobjectClass and HLAinteractionClassList.");
    }
  }

  if (unpublish) {
    unpublishObjectClassDirectedInteractions(*objectClass, *interactionClasses);
  } else {
    publishObjectClassDirectedInteractions(*objectClass, *interactionClasses);
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
          unpublish
              ? L"The embedded federation lost Send Interaction membership while applying HLAunpublishObjectClassDirectedInteractions."
              : L"The embedded federation lost Send Interaction membership while applying HLApublishObjectClassDirectedInteractions.");
    }
  }
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
