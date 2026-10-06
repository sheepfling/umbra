#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#endif

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

void UmbraRtiAmbassador::requestInteractionTransportationTypeChange(
    InteractionClassHandle const& interactionClass,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("requestInteractionTransportationTypeChange");
  try {
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Request Interaction Transportation Type Change requires a defined InteractionClassHandle.");
  }
  if (!transportationTypeHandleValue(transportationType)) {
    throw InvalidTransportationTypeHandle(
        L"Request Interaction Transportation Type Change requires a supported TransportationTypeHandle.");
  }
  // Section 6.30 names these supplied values "Interaction class designator"
  // and "Transportation type". Table 5 leaves HLAargumentName
  // implementation-defined, while fixing the two quoted handle.toString()
  // representations as MIM argument types 27 and 59.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Interaction Transportation Type Change requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->requestInteractionTransportationTypeChange(
          std::move(federationName),
          requestingFederateId,
          *interactionClassValue,
          *transportationTypeHandleValue(transportationType));
      if (result.status !=
          umbra::detail::InteractionTransportationTypeChangeStatus::applied) {
        throwAmbassadorInteractionTransportationTypeChangeFailure(result.status);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::InteractionTransportationTypeChangePlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Interaction Transportation Type Change");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Interaction Transportation Type Change requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const transportationName = ambassadorTransportationNameFromFederation(
        federationName,
        transportationType);
    if (!transportationName) {
      throw InvalidTransportationTypeHandle(
          L"Request Interaction Transportation Type Change requires a transportation type declared in the federation execution.");
    }
    plan = registry.planInteractionTransportationTypeChange(
        federationName,
        requestingFederateId,
        *interactionClassValue,
        *transportationName);
    if (plan.status != umbra::detail::InteractionTransportationTypeChangeStatus::applied) {
      throwAmbassadorInteractionTransportationTypeChangeFailure(plan.status);
    }
  }
  // The request has succeeded once the registry has accepted the pending
  // change. Section 6.30 separately makes the later confirmation callback
  // the boundary at which the preferred transportation takes effect. Emit
  // the public report after releasing the registry lock.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestInteractionTransportationTypeChange",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no interaction transportation confirmation callback route.");
  }
  queueAmbassadorConfirmInteractionTransportationTypeChange(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *interactionClassValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Interaction Transportation Type Change", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
