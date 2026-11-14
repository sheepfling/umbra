#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::handleEmbeddedMomInteractionSubscriptionRequest(
    std::optional<std::wstring>& federationName,
    std::optional<std::uint64_t> const& producingFederateId,
    std::optional<std::uint64_t> const& interactionClassHandle,
    InteractionClassHandle const& interactionClass,
    ParameterHandleValueMap const& parameterValues,
    VariableLengthData const& userSuppliedTag) {
  static_cast<void>(interactionClass);
  static_cast<void>(userSuppliedTag);
  if (!federationName || !producingFederateId || !interactionClassHandle) {
    return false;
  }

  enum class RequestKind {
    interaction_subscribe,
    interaction_unsubscribe,
    object_attribute_subscribe,
    object_attribute_unsubscribe,
    object_directed_subscribe,
    object_directed_unsubscribe,
  };
  std::optional<RequestKind> requestKind;
  std::optional<InteractionClassHandle> targetInteractionClass;
  std::optional<ObjectClassHandle> targetObjectClass;
  std::optional<AttributeHandleSet> targetAttributes;
  std::optional<InteractionClassHandleSet> targetDirectedInteractions;
  std::optional<bool> active;
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
    auto const isSubscribeRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::service_subscribe_interaction_class);
    auto const isUnsubscribeRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::service_unsubscribe_interaction_class);
    auto const isObjectAttributeSubscribeRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_subscribe_object_class_attributes);
    auto const isObjectAttributeUnsubscribeRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_unsubscribe_object_class_attributes);
    auto const isObjectDirectedSubscribeRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_subscribe_object_class_directed_interactions);
    auto const isObjectDirectedUnsubscribeRequest =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_unsubscribe_object_class_directed_interactions);
    if (!isSubscribeRequest || !isUnsubscribeRequest ||
        !isObjectAttributeSubscribeRequest ||
        !isObjectAttributeUnsubscribeRequest ||
        !isObjectDirectedSubscribeRequest ||
        !isObjectDirectedUnsubscribeRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve a standard MOM subscription class hierarchy.");
    }
    auto const requestCount =
        static_cast<unsigned>(*isSubscribeRequest) +
        static_cast<unsigned>(*isUnsubscribeRequest) +
        static_cast<unsigned>(*isObjectAttributeSubscribeRequest) +
        static_cast<unsigned>(*isObjectAttributeUnsubscribeRequest) +
        static_cast<unsigned>(*isObjectDirectedSubscribeRequest) +
        static_cast<unsigned>(*isObjectDirectedUnsubscribeRequest);
    if (requestCount == 0U) {
      return false;
    }
    if (requestCount != 1U) {
      throw RTIinternalError(
          L"A standard MOM subscription request matched conflicting service class hierarchies.");
    }
    if (*isSubscribeRequest) {
      requestKind = RequestKind::interaction_subscribe;
    } else if (*isUnsubscribeRequest) {
      requestKind = RequestKind::interaction_unsubscribe;
    } else if (*isObjectAttributeSubscribeRequest) {
      requestKind = RequestKind::object_attribute_subscribe;
    } else if (*isObjectAttributeUnsubscribeRequest) {
      requestKind = RequestKind::object_attribute_unsubscribe;
    } else if (*isObjectDirectedSubscribeRequest) {
      requestKind = RequestKind::object_directed_subscribe;
    } else {
      requestKind = RequestKind::object_directed_unsubscribe;
    }

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"A standard interaction subscription MOM request contains an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"A standard interaction subscription MOM request contains an unknown parameter.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class) {
        try {
          auto const decoded = decodeInteractionClassHandle(parameterValue);
          auto const decodedValue = interactionClassHandleValue(decoded);
          if (!decodedValue) {
            throw InteractionParameterNotDefined(
                L"A standard interaction subscription MOM request contains an invalid HLAinteractionClass value.");
          }
          targetInteractionClass = makeInteractionClassHandle(*decodedValue);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"A standard interaction subscription MOM request contains an invalid HLAinteractionClass value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::object_class) {
        try {
          auto const decoded = decodeObjectClassHandle(parameterValue);
          auto const decodedValue = objectClassHandleValue(decoded);
          if (!decodedValue) {
            throw InteractionParameterNotDefined(
                L"A standard object-attribute subscription MOM request contains an invalid HLAobjectClass value.");
          }
          targetObjectClass = makeObjectClassHandle(*decodedValue);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"A standard object-attribute subscription MOM request contains an invalid HLAobjectClass value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::attribute_list) {
        try {
          HLAvariableArrayT<HLAoctet> handlePrototype;
          HLAvariableArray handles{
              static_cast<DataElement const&>(handlePrototype)};
          handles.decode(parameterValue);
          AttributeHandleSet decodedAttributes;
          for (std::size_t index = 0U; index < handles.size(); ++index) {
            auto const* encodedHandle =
                dynamic_cast<HLAvariableArray const*>(&handles.get(index));
            if (encodedHandle == nullptr) {
              throw InteractionParameterNotDefined(
                  L"A standard object-attribute subscription MOM request contains a malformed HLAattributeList value.");
            }
            auto const decoded = decodeAttributeHandle(encodedHandle->encode());
            auto const decodedValue = attributeHandleValue(decoded);
            if (!decodedValue) {
              throw InteractionParameterNotDefined(
                  L"A standard object-attribute subscription MOM request contains an invalid HLAattributeList handle.");
            }
            decodedAttributes.insert(makeAttributeHandle(*decodedValue));
          }
          targetAttributes = std::move(decodedAttributes);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"A standard object-attribute subscription MOM request contains a malformed HLAattributeList value.");
        }
        continue;
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::interaction_class_list) {
        try {
          HLAvariableArrayT<HLAoctet> handlePrototype;
          HLAvariableArray handles{
              static_cast<DataElement const&>(handlePrototype)};
          handles.decode(parameterValue);
          InteractionClassHandleSet decodedInteractions;
          for (std::size_t index = 0U; index < handles.size(); ++index) {
            auto const* encodedHandle =
                dynamic_cast<HLAvariableArray const*>(&handles.get(index));
            if (encodedHandle == nullptr) {
              throw InteractionParameterNotDefined(
                  L"A standard directed-interaction subscription MOM request contains a malformed HLAinteractionClassList value.");
            }
            auto const decoded =
                decodeInteractionClassHandle(encodedHandle->encode());
            auto const decodedValue = interactionClassHandleValue(decoded);
            if (!decodedValue) {
              throw InteractionParameterNotDefined(
                  L"A standard directed-interaction subscription MOM request contains an invalid HLAinteractionClassList handle.");
            }
            decodedInteractions.insert(makeInteractionClassHandle(*decodedValue));
          }
          targetDirectedInteractions = std::move(decodedInteractions);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"A standard directed-interaction subscription MOM request contains a malformed HLAinteractionClassList value.");
        }
        continue;
      }
      if ((*requestKind == RequestKind::interaction_subscribe ||
           *requestKind == RequestKind::object_attribute_subscribe) &&
          *parameterName == umbra::detail::hla::utf8::mom::active) {
        HLAboolean decoded;
        try {
          decoded.decode(parameterValue);
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLAsubscribeInteractionClass received an invalid HLAactive value.");
        }
        active = decoded.get();
      }
    }
    if ((*requestKind == RequestKind::interaction_subscribe ||
         *requestKind == RequestKind::interaction_unsubscribe) &&
        !targetInteractionClass) {
      throw InteractionParameterNotDefined(
          L"A standard interaction subscription MOM request requires HLAinteractionClass.");
    }
    if (*requestKind == RequestKind::object_attribute_subscribe &&
        (!targetObjectClass || !targetAttributes)) {
      throw InteractionParameterNotDefined(
          L"A standard object-attribute subscription MOM request requires HLAobjectClass and HLAattributeList.");
    }
    if (*requestKind == RequestKind::object_attribute_unsubscribe &&
        !targetObjectClass) {
      throw InteractionParameterNotDefined(
          L"A standard object-attribute unsubscription MOM request requires HLAobjectClass.");
    }
    if ((*requestKind == RequestKind::object_directed_subscribe ||
         *requestKind == RequestKind::object_directed_unsubscribe) &&
        (!targetObjectClass || !targetDirectedInteractions)) {
      throw InteractionParameterNotDefined(
          L"A standard directed-interaction subscription MOM request requires HLAobjectClass and HLAinteractionClassList.");
    }
    if ((*requestKind == RequestKind::interaction_subscribe ||
         *requestKind == RequestKind::object_attribute_subscribe) && !active) {
      throw InteractionParameterNotDefined(
          L"A standard subscription MOM request requires HLAactive.");
    }
  }

  switch (*requestKind) {
    case RequestKind::interaction_subscribe:
      subscribeInteractionClass(*targetInteractionClass, *active);
      break;
    case RequestKind::interaction_unsubscribe:
      unsubscribeInteractionClass(*targetInteractionClass);
      break;
    case RequestKind::object_attribute_subscribe:
      subscribeObjectClassAttributes(
          *targetObjectClass, *targetAttributes, *active);
      break;
    case RequestKind::object_attribute_unsubscribe:
      if (targetAttributes) {
        unsubscribeObjectClassAttributes(*targetObjectClass, *targetAttributes);
      } else {
        unsubscribeObjectClass(*targetObjectClass);
      }
      break;
    case RequestKind::object_directed_subscribe:
      subscribeObjectClassDirectedInteractions(
          *targetObjectClass, *targetDirectedInteractions);
      break;
    case RequestKind::object_directed_unsubscribe:
      unsubscribeObjectClassDirectedInteractions(
          *targetObjectClass, *targetDirectedInteractions);
      break;
  }
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
