#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/parameter_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <optional>

namespace rti1516_2025::umbra_binding_detail {
InteractionClassHandle UmbraRtiAmbassador::getInteractionClassHandle(
    std::wstring const& interactionClassName) {
  auto instrumentationScope = beginRtiCall("getInteractionClassHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // A configured process endpoint owns the joined federate's FOM lookup. Do
  // not consult the process-local registry here: doing so would make a
  // remote ambassador appear to have a second, possibly stale, definition.
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
            L"Get Interaction Class Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::uint64_t> handle;
    try {
      handle = processClient->lookupInteractionClassHandle(
          federationName, federateId, interactionClassName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw NameNotFound(
          L"The supplied interaction class name is not defined in this federation execution.");
    }
    return makeInteractionClassHandle(*handle);
  }
#endif
  InteractionClassHandle interactionClass;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Interaction Class Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const encodedName = umbra::detail::utf8FromWide(interactionClassName);
    if (!encodedName) {
      throw NameNotFound(L"The supplied interaction class name is not valid UTF-8 text.");
    }
    auto const handle = registry.interactionClassHandleFor(*joinedFederationName_, *encodedName);
    if (!handle) {
      throw NameNotFound(
          L"The supplied interaction class name is not defined in this federation execution.");
    }
    interactionClass = makeInteractionClassHandle(*handle);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetInteractionClassHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Interaction class name",
        umbra::detail::formatMomString(interactionClassName)}},
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class handle",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)});
  return interactionClass;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Class Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetInteractionClassHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Interaction class name",
          umbra::detail::formatMomString(interactionClassName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getInteractionClassName(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("getInteractionClassName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Interaction Class Name requires a valid InteractionClassHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Interaction Class Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::wstring> interactionClassName;
    try {
      interactionClassName = processClient->lookupInteractionClassName(
          federationName, federateId, *interactionClassHandle);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!interactionClassName) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }
    return *interactionClassName;
  }
#endif
  std::wstring interactionClassName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Interaction Class Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InvalidInteractionClassHandle(
          L"Get Interaction Class Name requires a valid InteractionClassHandle.");
    }
    auto const encodedName = registry.interactionClassNameFor(*joinedFederationName_, *handle);
    if (!encodedName) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }
    auto const decodedName = umbra::detail::wideFromUtf8(*encodedName);
    if (!decodedName) {
      throw RTIinternalError(
          L"The embedded federation stored an interaction class name that is not valid UTF-8 text.");
    }
    interactionClassName = *decodedName;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetInteractionClassName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class handle",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      {umbra::detail::MomArgumentType::string,
       L"Interaction class name",
       umbra::detail::formatMomString(interactionClassName)});
  return interactionClassName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Interaction Class Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetInteractionClassName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class handle",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

ParameterHandle UmbraRtiAmbassador::getParameterHandle(
    InteractionClassHandle const& interactionClass,
    std::wstring const& parameterName) {
  auto instrumentationScope = beginRtiCall("getParameterHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Parameter Handle requires a valid InteractionClassHandle.");
    }
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Parameter Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::uint64_t> handle;
    try {
      handle = processClient->lookupParameterHandle(
          federationName,
          federateId,
          *interactionClassHandle,
          parameterName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!handle) {
      throw NameNotFound(
          L"The supplied parameter name is not defined for this interaction class.");
    }
    return makeParameterHandle(*handle);
  }
#endif
  ParameterHandle parameter;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Parameter Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Parameter Handle requires a valid InteractionClassHandle.");
    }
    auto const encodedInteractionClassName = registry.interactionClassNameFor(
        *joinedFederationName_,
        *interactionClassHandle);
    if (!encodedInteractionClassName) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }

    auto const encodedParameterName = umbra::detail::utf8FromWide(parameterName);
    if (!encodedParameterName) {
      throw NameNotFound(L"The supplied parameter name is not valid UTF-8 text.");
    }
    auto const parameterHandle = registry.parameterHandleFor(
        *joinedFederationName_,
        *encodedInteractionClassName,
        *encodedParameterName);
    if (!parameterHandle) {
      throw NameNotFound(L"The supplied parameter name is not defined for this interaction class.");
    }
    parameter = makeParameterHandle(*parameterHandle);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetParameterHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class handle",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::string,
        L"Parameter name",
        umbra::detail::formatMomString(parameterName)}},
      {umbra::detail::MomArgumentType::parameter_handle,
       L"Parameter handle",
       umbra::detail::formatMomParameterHandle(parameter)});
  return parameter;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Parameter Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetParameterHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class handle",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::string,
          L"Parameter name",
          umbra::detail::formatMomString(parameterName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getParameterName(
    InteractionClassHandle const& interactionClass,
    ParameterHandle const& parameter) {
  auto instrumentationScope = beginRtiCall("getParameterName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Parameter Name requires a valid InteractionClassHandle.");
    }
    auto const parameterHandleValueResult = parameterHandleValue(parameter);
    if (!parameterHandleValueResult) {
      throw InvalidParameterHandle(
          L"Get Parameter Name requires a valid ParameterHandle.");
    }
    std::wstring federationName;
    std::uint64_t federateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Get Parameter Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }

    std::optional<std::wstring> parameterName;
    try {
      parameterName = processClient->lookupParameterName(
          federationName,
          federateId,
          *interactionClassHandle,
          *parameterHandleValueResult);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!parameterName) {
      throw InteractionParameterNotDefined(
          L"The supplied ParameterHandle is not defined for this interaction class.");
    }
    return *parameterName;
  }
#endif
  std::wstring parameterName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Parameter Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const interactionClassHandle = interactionClassHandleValue(interactionClass);
    if (!interactionClassHandle) {
      throw InvalidInteractionClassHandle(
          L"Get Parameter Name requires a valid InteractionClassHandle.");
    }
    auto const encodedInteractionClassName = registry.interactionClassNameFor(
        *joinedFederationName_,
        *interactionClassHandle);
    if (!encodedInteractionClassName) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not known in this federation execution.");
    }

    auto const parameterHandle = parameterHandleValue(parameter);
    if (!parameterHandle) {
      throw InvalidParameterHandle(L"Get Parameter Name requires a valid ParameterHandle.");
    }
    auto const encodedParameterName = registry.parameterNameFor(
        *joinedFederationName_,
        *encodedInteractionClassName,
        *parameterHandle);
    if (!encodedParameterName) {
      throw InteractionParameterNotDefined(
          L"The supplied ParameterHandle is not defined for this interaction class.");
    }
    auto const decodedName = umbra::detail::wideFromUtf8(*encodedParameterName);
    if (!decodedName) {
      throw RTIinternalError(
          L"The embedded federation stored a parameter name that is not valid UTF-8 text.");
    }
    parameterName = *decodedName;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetParameterName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class handle",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::parameter_handle,
        L"Parameter handle",
        umbra::detail::formatMomParameterHandle(parameter)}},
      {umbra::detail::MomArgumentType::string,
       L"Parameter name",
       umbra::detail::formatMomString(parameterName)});
  return parameterName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Parameter Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetParameterName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class handle",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::parameter_handle,
          L"Parameter handle",
          umbra::detail::formatMomParameterHandle(parameter)}},
        describeAmbassadorException(exception));
    throw;
  }
}
}  // namespace rti1516_2025::umbra_binding_detail
