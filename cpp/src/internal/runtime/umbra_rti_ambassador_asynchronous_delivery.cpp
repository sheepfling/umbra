#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/time/federate_time_state.hpp"

#include <mutex>
#include <optional>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

namespace {

void flushAsynchronousReceiveCallbacks(
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState) {
  if (!timeState) {
    return;
  }
  auto callbacks = timeState->takeEligibleAsynchronousReceiveCallbacks();
  for (auto& callback : callbacks) {
    if (callback) {
      callback();
    }
  }
}

}  // namespace

void flushAmbassadorAsynchronousReceiveCallbacks(
    std::shared_ptr<umbra::detail::FederateTimeState> const& timeState) {
  flushAsynchronousReceiveCallbacks(timeState);
}

// Asynchronous-delivery service implementations.

void UmbraRtiAmbassador::enableAsynchronousDelivery() {
  auto instrumentationScope = beginRtiCall("enableAsynchronousDelivery");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  umbra::detail::FederateAsynchronousDeliveryStatus result;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Enable Asynchronous Delivery");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Enable Asynchronous Delivery requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    result = timeState->enableAsynchronousDelivery();
    if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::asynchronous_delivery});
    }
  }

  if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
    // HLA_IMMEDIATE observers may enter Python/Java callbacks synchronously;
    // never report while the native federation locks are held.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"EnableAsynchronousDelivery",
        umbra::detail::MomServiceType::time_management,
        {},
        true);
  }

  switch (result) {
    case umbra::detail::FederateAsynchronousDeliveryStatus::applied:
      // Enabling the switch also makes receive-order callbacks that arrived
      // while the constrained federate was Time Granted immediately eligible.
      if (momWork) {
        queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
            momWork->federationName,
            momWork->objectInstanceHandle,
            std::move(momWork->attributeHandles));
      }
      flushAsynchronousReceiveCallbacks(timeState);
      return;
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_enabled:
      throw AsynchronousDeliveryAlreadyEnabled(
          L"Asynchronous delivery is already enabled for the joined federate.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_disabled:
      throw RTIinternalError(
          L"Umbra received an invalid asynchronous-delivery enable outcome.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown asynchronous-delivery enable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Enable Asynchronous Delivery", exception);
    throw;
  }
}

void UmbraRtiAmbassador::disableAsynchronousDelivery() {
  auto instrumentationScope = beginRtiCall("disableAsynchronousDelivery");
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  umbra::detail::FederateAsynchronousDeliveryStatus result;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Disable Asynchronous Delivery");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Disable Asynchronous Delivery requires membership in a federation execution.");
    }
    timeState = federateTimeState_;
    result = timeState->disableAsynchronousDelivery();
    if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::asynchronous_delivery});
    }
  }

  if (result == umbra::detail::FederateAsynchronousDeliveryStatus::applied) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"DisableAsynchronousDelivery",
        umbra::detail::MomServiceType::time_management,
        {},
        true);
  }

  switch (result) {
    case umbra::detail::FederateAsynchronousDeliveryStatus::applied:
      if (momWork) {
        queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
            momWork->federationName,
            momWork->objectInstanceHandle,
            std::move(momWork->attributeHandles));
      }
      return;
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_disabled:
      throw AsynchronousDeliveryAlreadyDisabled(
          L"Asynchronous delivery is already disabled for the joined federate.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::already_enabled:
      throw RTIinternalError(
          L"Umbra received an invalid asynchronous-delivery disable outcome.");
    case umbra::detail::FederateAsynchronousDeliveryStatus::inactive:
      throw FederateNotExecutionMember(
          L"The joined federate's logical-time state is no longer active.");
  }
  throw RTIinternalError(L"Umbra encountered an unknown asynchronous-delivery disable outcome.");
  } catch (Exception const& exception) {
    emitExceptionReport(L"Disable Asynchronous Delivery", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
