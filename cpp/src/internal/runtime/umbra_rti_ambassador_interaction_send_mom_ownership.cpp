#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAvariableArray.h>

#include <mutex>
#include <optional>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::handleEmbeddedMomUnconditionalDivestitureRequest(
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

  std::optional<ObjectInstanceHandle> objectInstance;
  std::optional<AttributeHandleSet> attributes;
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
    auto const isUnconditionalDivestiture =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_unconditional_attribute_ownership_divestiture);
    if (!isUnconditionalDivestiture) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAunconditionalAttributeOwnershipDivestiture MOM class hierarchy.");
    }
    if (!*isUnconditionalDivestiture) {
      return false;
    }

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"HLAunconditionalAttributeOwnershipDivestiture received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAunconditionalAttributeOwnershipDivestiture received an unknown parameter.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::object_instance) {
        try {
          auto const decoded = decodeObjectInstanceHandle(parameterValue);
          auto const decodedValue = objectInstanceHandleValue(decoded);
          if (!decodedValue) {
            throw InteractionParameterNotDefined(
                L"HLAunconditionalAttributeOwnershipDivestiture contains an invalid HLAobjectInstance value.");
          }
          objectInstance = makeObjectInstanceHandle(*decodedValue);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLAunconditionalAttributeOwnershipDivestiture contains an invalid HLAobjectInstance value.");
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
                  L"HLAunconditionalAttributeOwnershipDivestiture contains a malformed HLAattributeList value.");
            }
            auto const decoded = decodeAttributeHandle(encodedHandle->encode());
            auto const decodedValue = attributeHandleValue(decoded);
            if (!decodedValue) {
              throw InteractionParameterNotDefined(
                  L"HLAunconditionalAttributeOwnershipDivestiture contains an invalid HLAattributeList handle.");
            }
            decodedAttributes.insert(makeAttributeHandle(*decodedValue));
          }
          attributes = std::move(decodedAttributes);
        } catch (InteractionParameterNotDefined const&) {
          throw;
        } catch (Exception const&) {
          throw InteractionParameterNotDefined(
              L"HLAunconditionalAttributeOwnershipDivestiture contains a malformed HLAattributeList value.");
        }
      }
    }
    if (!objectInstance || !attributes) {
      throw InteractionParameterNotDefined(
          L"HLAunconditionalAttributeOwnershipDivestiture requires HLAobjectInstance and HLAattributeList.");
    }
  }

  unconditionalAttributeOwnershipDivestiture(
      *objectInstance, *attributes, VariableLengthData{});
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
