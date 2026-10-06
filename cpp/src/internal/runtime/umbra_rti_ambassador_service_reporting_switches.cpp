#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#endif

#include <mutex>
#include <optional>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
bool UmbraRtiAmbassador::getServiceReportingSwitch() const {
  auto instrumentationScope = beginRtiCall("getServiceReportingSwitch");
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
            L"Get Service Reporting Switch requires membership in a federation execution.");
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
      return processClient->getServiceReportingSwitch(
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
  requireFederationServiceOperationAvailable(L"Get Service Reporting Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Service Reporting Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .serviceReportingSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Service Reporting Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setServiceReportingSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setServiceReportingSwitch");
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
            L"Set Service Reporting Switch requires membership in a federation execution.");
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
      processClient->setServiceReportingSwitch(
          std::move(federationName), federateId, switchValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Set Service Reporting Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Service Reporting Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry().setServiceReportingSwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::ServiceReportingSwitchStatus::applied) {
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::service_reporting});
    } else if (status == umbra::detail::ServiceReportingSwitchStatus::
                   report_service_invocations_are_subscribed) {
      throw ReportServiceInvocationsAreSubscribed(
          L"The Service Reporting Switch cannot be enabled while the report-service-invocation interaction is subscribed.");
    } else if (status == umbra::detail::ServiceReportingSwitchStatus::
                   federation_does_not_exist ||
               status == umbra::detail::ServiceReportingSwitchStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(L"Set Service Reporting Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Service Reporting Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getExceptionReportingSwitch() const {
  auto instrumentationScope = beginRtiCall("getExceptionReportingSwitch");
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
            L"Get Exception Reporting Switch requires membership in a federation execution.");
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
      auto const result = processClient->getExceptionReportingSwitch(
          std::move(federationName), federateId);
      switch (result.status) {
        case umbra::detail::FederationServiceOperationStatus::available:
          return result.value;
        case umbra::detail::FederationServiceOperationStatus::save_in_progress:
          throw SaveInProgress(L"Get Exception Reporting Switch is unavailable during federation save.");
        case umbra::detail::FederationServiceOperationStatus::restore_in_progress:
          throw RestoreInProgress(L"Get Exception Reporting Switch is unavailable during federation restore.");
        case umbra::detail::FederationServiceOperationStatus::federation_does_not_exist:
        case umbra::detail::FederationServiceOperationStatus::federate_not_member:
          throw FederateNotExecutionMember(L"Get Exception Reporting Switch requires an execution member.");
      }
      throw RTIinternalError(L"Get Exception Reporting Switch returned an unknown operation state.");
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
  }
#endif
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Get Exception Reporting Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Exception Reporting Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .exceptionReportingSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Exception Reporting Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setExceptionReportingSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setExceptionReportingSwitch");
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
            L"Set Exception Reporting Switch requires membership in a federation execution.");
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
      auto const result = processClient->setExceptionReportingSwitch(
          std::move(federationName), federateId, switchValue);
      switch (result.status) {
        case umbra::detail::FederationServiceOperationStatus::available:
          break;
        case umbra::detail::FederationServiceOperationStatus::save_in_progress:
          throw SaveInProgress(L"Set Exception Reporting Switch is unavailable during federation save.");
        case umbra::detail::FederationServiceOperationStatus::restore_in_progress:
          throw RestoreInProgress(L"Set Exception Reporting Switch is unavailable during federation restore.");
        case umbra::detail::FederationServiceOperationStatus::federation_does_not_exist:
        case umbra::detail::FederationServiceOperationStatus::federate_not_member:
          throw FederateNotExecutionMember(L"Set Exception Reporting Switch requires an execution member.");
        default:
          throw RTIinternalError(L"Set Exception Reporting Switch returned an unknown operation state.");
      }
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
    requireFederationServiceOperationAvailable(L"Set Exception Reporting Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Exception Reporting Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry().setExceptionReportingSwitch(
        *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      accepted = true;
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::exception_reporting});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Exception Reporting Switch encountered an unknown outcome.");
    }
  }
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"SetExceptionReportingSwitch",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::boolean,
          L"SwitchValue",
          umbra::detail::formatMomBoolean(switchValue)}},
        true);
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Exception Reporting Switch", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::getSendServiceReportsToFileSwitch() const {
  auto instrumentationScope = beginRtiCall("getSendServiceReportsToFileSwitch");
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
            L"Get Send Service Reports To File Switch requires membership in a federation execution.");
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
      return processClient->getSendServiceReportsToFileSwitch(
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
  requireFederationServiceOperationAvailable(L"Get Send Service Reports To File Switch");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Get Send Service Reports To File Switch requires membership in a federation execution.");
  }
  auto const switchValue = embeddedFederationRegistry()
      .sendServiceReportsToFileSwitchFor(*joinedFederationName_, *joinedFederateId_);
  if (!switchValue) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  return *switchValue;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Send Service Reports To File Switch", exception);
    throw;
  }
}

void UmbraRtiAmbassador::setSendServiceReportsToFileSwitch(bool switchValue) {
  auto instrumentationScope = beginRtiCall("setSendServiceReportsToFileSwitch");
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
            L"Set Send Service Reports To File Switch requires membership in a federation execution.");
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
      processClient->setSendServiceReportsToFileSwitch(
          std::move(federationName), federateId, switchValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::optional<AmbassadorJoinedFederateMomConditionalWork> momWork;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Set Send Service Reports To File Switch");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Set Send Service Reports To File Switch requires membership in a federation execution.");
    }
    auto const status = embeddedFederationRegistry()
        .setSendServiceReportsToFileSwitch(
            *joinedFederationName_, *joinedFederateId_, switchValue);
    if (status == umbra::detail::FederationRegistryStatus::applied) {
      momWork = ambassadorJoinedFederateMomConditionalWorkFor(
          embeddedFederationRegistry(),
          *joinedFederationName_,
          *joinedFederateId_,
          {umbra::detail::hla::utf8::mom::send_service_reports_to_file});
    } else if (status == umbra::detail::FederationRegistryStatus::federation_does_not_exist ||
               status == umbra::detail::FederationRegistryStatus::federate_not_member) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    } else {
      throw RTIinternalError(
          L"Set Send Service Reports To File Switch encountered an unknown outcome.");
    }
  }
  if (momWork) {
    queueAmbassadorJoinedFederateMomConditionalAttributeUpdate(
        momWork->federationName,
        momWork->objectInstanceHandle,
        std::move(momWork->attributeHandles));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Set Send Service Reports To File Switch", exception);
    throw;
  }
}
#endif

}  // namespace rti1516_2025::umbra_binding_detail
