#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#endif

#include <mutex>
#include <optional>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

ResignAction UmbraRtiAmbassador::getAutomaticResignDirective() {
  auto instrumentationScope = beginRtiCall("getAutomaticResignDirective");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Automatic Resign Directive requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getAutomaticResignDirective(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Automatic Resign Directive");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Automatic Resign Directive requires membership in a federation execution.");
  }
  auto const action = embeddedFederationRegistry()
      .automaticResignActionFor(*joinedFederationName_, *joinedFederateId_);
  if (!action) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *action;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Automatic Resign Directive", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setAutomaticResignDirective(ResignAction resignAction) {
  auto instrumentationScope = beginRtiCall("setAutomaticResignDirective");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Set Automatic Resign Directive requires membership in a federation execution.");
      }
      requireAmbassadorValidResignAction(resignAction);
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      processClient->setAutomaticResignDirective(
          std::move(federationName), federateId, resignAction);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  bool accepted = false;
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Automatic Resign Directive");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Automatic Resign Directive requires membership in a federation execution.");
    }
    requireAmbassadorValidResignAction(resignAction);
    auto const status = embeddedFederationRegistry().setAutomaticResignAction(
        *joinedFederationName_, *joinedFederateId_, resignAction);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::automatic_resign_action});
    } else if (status == umbra::detail::FederationRegistryStatus::invalid_resign_action) {
      throw InvalidResignAction(
          L"The supplied resign action is not an IEEE 1516.1 ResignAction value.");
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Automatic Resign Directive encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetAutomaticResignDirective",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::resign_action,
          L"AutomaticResignDirective",
          umbra::detail::formatMomResignAction(resignAction)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Automatic Resign Directive", exception);
    throw;
  }
}


bool UmbraRtiAmbassador::getAutoProvideSwitch() const {
  auto instrumentationScope = beginRtiCall("getAutoProvideSwitch");
  try {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Auto Provide Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Auto Provide Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry().autoProvideSwitchFor(
      *joinedFederationName_,
      *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Auto Provide Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getDelaySubscriptionEvaluationSwitch() const {
  auto instrumentationScope = beginRtiCall("getDelaySubscriptionEvaluationSwitch");
  try {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Get Delay Subscription Evaluation Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Delay Subscription Evaluation Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .delaySubscriptionEvaluationSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Delay Subscription Evaluation Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAdvisoriesUseKnownClassSwitch() const {
  auto instrumentationScope = beginRtiCall("getAdvisoriesUseKnownClassSwitch");
  try {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Advisories Use Known Class Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Advisories Use Known Class Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .advisoriesUseKnownClassSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Advisories Use Known Class Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getAllowRelaxedDDMSwitch() const {
  auto instrumentationScope = beginRtiCall("getAllowRelaxedDDMSwitch");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Allow Relaxed DDM Switch requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    try {
      return processClient->getAllowRelaxedDDMSwitch(
          std::move(federationName), federateId);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Allow Relaxed DDM Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Allow Relaxed DDM Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .allowRelaxedDDMSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Allow Relaxed DDM Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getNonRegulatedGrantSwitch() const {
  auto instrumentationScope = beginRtiCall("getNonRegulatedGrantSwitch");
  try {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Non Regulated Grant Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Non Regulated Grant Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .nonRegulatedGrantSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Non Regulated Grant Switch", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
