#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#endif

#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

void UmbraRtiAmbassador::queryInteractionTransportationType(
    FederateHandle const& federate,
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("queryInteractionTransportationType");
  try {
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
  }
  auto const queriedFederateId = federateHandleValue(federate);
  if (!queriedFederateId) {
    throw FederateNotExecutionMember(
        L"Query Interaction Transportation Type requires a valid FederateHandle.");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Query Interaction Transportation Type requires a defined InteractionClassHandle.");
  }
  // Section 6.32 names these supplied values "Federate designator" and
  // "Interaction class designator". Table 5 fixes their respective type-15
  // and type-27 value forms as quoted handle.toString() text.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::federate_handle,
       L"Federate designator",
       umbra::detail::formatMomFederateHandle(federate)},
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
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
            L"Query Interaction Transportation Type requires membership in a federation execution.");
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
      auto const result = processClient->queryInteractionTransportationType(
          std::move(federationName),
          requestingFederateId,
          *queriedFederateId,
          *interactionClassValue);
      if (result.status !=
          umbra::detail::InteractionTransportationTypeQueryStatus::applied) {
        throwAmbassadorInteractionTransportationTypeQueryFailure(result.status);
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
  umbra::detail::InteractionTransportationTypeQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Query Interaction Transportation Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Interaction Transportation Type requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planInteractionTransportationTypeQuery(
        federationName,
        requestingFederateId,
        *queriedFederateId,
        *interactionClassValue);
    if (plan.status != umbra::detail::InteractionTransportationTypeQueryStatus::applied) {
      throwAmbassadorInteractionTransportationTypeQueryFailure(plan.status);
    }
  }
  // The query is successfully invoked once the plan is accepted. Its
  // separate Report Interaction Transportation Type callback is not the
  // service-reporting boundary. Emit the public MOM interaction after
  // releasing the registry lock so HLA_IMMEDIATE callbacks can re-enter.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"QueryInteractionTransportationType",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no interaction transportation report callback route.");
  }
  queueAmbassadorReportInteractionTransportationType(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *queriedFederateId,
      *interactionClassValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Interaction Transportation Type", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
