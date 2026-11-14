#include "internal/encoding/byte_order.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"

#include <cstdint>
#include <mutex>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

std::optional<std::uint32_t> decodeMimResignActionValue(
    VariableLengthData const& value) {
  auto const* bytes = static_cast<unsigned char const*>(value.data());
  std::uint32_t encoded = 0U;
  if (bytes == nullptr ||
      !umbra::detail::readUnsigned(
          bytes,
          value.size(),
          umbra::detail::ByteOrder::big,
          encoded)) {
    return std::nullopt;
  }
  return encoded;
}

ResignAction decodeMimResignAction(VariableLengthData const& value) {
  auto const encoded = decodeMimResignActionValue(value);
  if (!encoded) {
    throw InteractionParameterNotDefined(
        L"HLAresignFederationExecution requires an HLAinteger32BE HLAresignAction value.");
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
      throw InvalidResignAction(
          L"HLAresignFederationExecution received an HLAresignAction value outside the standard enumeration.");
  }
}

}  // namespace

bool UmbraRtiAmbassador::handleEmbeddedMomResignFederationExecutionRequest(
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

  std::optional<ResignAction> resignAction;
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
    auto const isResignRequest = registry.interactionClassIsSameOrDescendantOf(
        *federationName,
        *interactionClassHandle,
        umbra::detail::hla::utf8::mom::service_resign_federation_execution);
    if (!isResignRequest) {
      throw RTIinternalError(
          L"The embedded federation cannot resolve the HLAresignFederationExecution MOM class hierarchy.");
    }
    if (!*isResignRequest) {
      return false;
    }

    for (auto const& [parameterHandle, parameterValue] : parameterValues) {
      auto const suppliedParameterHandle = parameterHandleValue(parameterHandle);
      if (!suppliedParameterHandle) {
        throw InteractionParameterNotDefined(
            L"HLAresignFederationExecution received an invalid parameter handle.");
      }
      auto const parameterName = registry.parameterNameFor(
          *federationName,
          *interactionClassName,
          *suppliedParameterHandle);
      if (!parameterName) {
        throw InteractionParameterNotDefined(
            L"HLAresignFederationExecution received an unknown parameter.");
      }
      if (*parameterName == umbra::detail::hla::utf8::mom::resign_action) {
        resignAction = decodeMimResignAction(parameterValue);
      }
    }
    if (!resignAction) {
      throw InteractionParameterNotDefined(
          L"HLAresignFederationExecution requires HLAresignAction.");
    }
  }

  // The Table 7 service interaction is defined to invoke the corresponding
  // public service for its sending federate. Let that service own the full
  // resignation lifecycle, including ownership handling and the final MOM
  // service-report reservation before membership is removed.
  resignFederationExecution(*resignAction);
  return true;
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
