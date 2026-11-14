#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <RTI/encoding/BasicDataElements.h>

#include <mutex>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::handleEmbeddedMomSynchronizationPointAchievedRequest(
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

  std::optional<std::wstring> label;
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
    auto const isSynchronizationPointAchieved =
        registry.interactionClassIsSameOrDescendantOf(
            *federationName,
            *interactionClassHandle,
            umbra::detail::hla::utf8::mom::service_synchronization_point_achieved);
    if (!isSynchronizationPointAchieved) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAsynchronizationPointAchieved MOM class hierarchy.");
    }
    if (!*isSynchronizationPointAchieved) {
      return false;
    }

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"HLAsynchronizationPointAchieved received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAsynchronizationPointAchieved received an unknown parameter.");
      }
      if (*parameterName != umbra::detail::hla::utf8::mom::label) {
        continue;
      }
      HLAunicodeString decodedLabel;
      try {
        decodedLabel.decode(parameterValue);
      } catch (Exception const&) {
        throw InteractionParameterNotDefined(
            L"HLAsynchronizationPointAchieved received an invalid HLAlabel value.");
      }
      label = decodedLabel.get();
    }
    if (!label) {
      throw InteractionParameterNotDefined(
          L"HLAsynchronizationPointAchieved requires HLAlabel.");
    }
  }

  synchronizationPointAchieved(*label);
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
