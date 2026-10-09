#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/process_federation_client.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace rti1516_2025::umbra_binding_detail {

using namespace service_failure_translation;

unsigned long UmbraRtiAmbassador::normalizeServiceGroup(ServiceGroup serviceGroup) {
  auto instrumentationScope = beginRtiCall("normalizeServiceGroup");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::scoped_lock lock(mutex_);
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Service Group requires membership in a federation execution.");
    }
    switch (serviceGroup) {
      case FEDERATION_MANAGEMENT:
      case DECLARATION_MANAGEMENT:
      case OBJECT_MANAGEMENT:
      case OWNERSHIP_MANAGEMENT:
      case TIME_MANAGEMENT:
      case DATA_DISTRIBUTION_MANAGEMENT:
      case SUPPORT_SERVICES:
        return static_cast<unsigned long>(serviceGroup);
      default:
        throw InvalidServiceGroup(
            L"Normalize Service Group requires a supported ServiceGroup indicator.");
    }
  }
#endif
  unsigned long normalized = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Service Group requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    // HLAserviceGroup has the fixed standard upper bound of 7. The official
    // ServiceGroup indicator therefore already gives its point-range coordinate;
    // unlike execution-issued handles it must not be replaced with an arbitrary
    // per-execution value outside that dimension's domain.
    switch (serviceGroup) {
      case FEDERATION_MANAGEMENT:
      case DECLARATION_MANAGEMENT:
      case OBJECT_MANAGEMENT:
      case OWNERSHIP_MANAGEMENT:
      case TIME_MANAGEMENT:
      case DATA_DISTRIBUTION_MANAGEMENT:
      case SUPPORT_SERVICES:
        normalized = static_cast<unsigned long>(serviceGroup);
        break;
      default:
        throw InvalidServiceGroup(
            L"Normalize Service Group requires a supported ServiceGroup indicator.");
    }
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"NormalizeServiceGroup",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::service_group,
        L"Service group indicator",
        umbra::detail::formatMomServiceGroup(serviceGroup)}},
      {umbra::detail::MomArgumentType::number,
       L"Normalized value",
       umbra::detail::formatMomNumber(std::to_wstring(normalized))});
  return normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Service Group", exception);
    std::wstring encodedServiceGroup;
    try {
      encodedServiceGroup = umbra::detail::formatMomServiceGroup(serviceGroup);
    } catch (...) {
      // Table 5 has no canonical spelling for an invalid ServiceGroup enum.
      // Keep the type-50 supplied slot deterministic without replacing the
      // public InvalidServiceGroup diagnostic.
      encodedServiceGroup = umbra::detail::formatMomString(L"UNSUPPORTED");
    }
    appendFailedServiceReportToFileIfSelected(
        L"NormalizeServiceGroup",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::service_group,
          L"Service group indicator",
          encodedServiceGroup}},
        describeAmbassadorException(exception));
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeFederateHandle(
    FederateHandle const& federate) {
  auto instrumentationScope = beginRtiCall("normalizeFederateHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const federateValue = federateHandleValue(federate);
    if (!federateValue) {
      throw InvalidFederateHandle(
          L"Normalize Federate Handle requires a valid FederateHandle.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Normalize Federate Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> normalized;
    try {
      normalized = processClient->normalizeFederateHandle(
          std::move(federationName), requestingFederateId, *federateValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!normalized) {
      throw InvalidFederateHandle(
          L"The supplied FederateHandle is not valid in this federation execution.");
    }
    return static_cast<unsigned long>(*normalized);
  }
#endif
  unsigned long normalized = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Federate Handle requires membership in a federation execution.");
    }
    auto const federateValue = federateHandleValue(federate);
    if (!federateValue) {
      throw InvalidFederateHandle(
          L"Normalize Federate Handle requires a valid FederateHandle.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const result = registry.normalizedFederateHandleValueFor(
        *joinedFederationName_, *federateValue);
    if (!result) {
      throw InvalidFederateHandle(
          L"The supplied FederateHandle is not valid in this federation execution.");
    }
    normalized = *result;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"NormalizeFederateHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::federate_handle,
        L"Federate handle",
        umbra::detail::formatMomFederateHandle(federate)}},
      {umbra::detail::MomArgumentType::number,
       L"Normalized value",
       umbra::detail::formatMomNumber(std::to_wstring(normalized))});
  return normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Federate Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"NormalizeFederateHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::federate_handle,
          L"Federate handle",
          umbra::detail::formatMomFederateHandle(federate)}},
        describeAmbassadorException(exception));
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeObjectClassHandle(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("normalizeObjectClassHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw InvalidObjectClassHandle(
          L"Normalize Object Class Handle requires a valid ObjectClassHandle.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Normalize Object Class Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> normalized;
    try {
      normalized = processClient->normalizeObjectClassHandle(
          std::move(federationName), requestingFederateId, *objectClassValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!normalized) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not valid in this federation execution.");
    }
    return static_cast<unsigned long>(*normalized);
  }
#endif
  unsigned long normalized = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Object Class Handle requires membership in a federation execution.");
    }
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw InvalidObjectClassHandle(
          L"Normalize Object Class Handle requires a valid ObjectClassHandle.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const result = registry.normalizedObjectClassHandleValueFor(
        *joinedFederationName_, *objectClassValue);
    if (!result) {
      throw InvalidObjectClassHandle(
          L"The supplied ObjectClassHandle is not valid in this federation execution.");
    }
    normalized = *result;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"NormalizeObjectClassHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class handle",
        umbra::detail::formatMomObjectClassHandle(objectClass)}},
      {umbra::detail::MomArgumentType::number,
       L"Normalized value",
       umbra::detail::formatMomNumber(std::to_wstring(normalized))});
  return normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Object Class Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"NormalizeObjectClassHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class handle",
          umbra::detail::formatMomObjectClassHandle(objectClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeInteractionClassHandle(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("normalizeInteractionClassHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const interactionClassValue = interactionClassHandleValue(interactionClass);
    if (!interactionClassValue) {
      throw InvalidInteractionClassHandle(
          L"Normalize Interaction Class Handle requires a valid InteractionClassHandle.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Normalize Interaction Class Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> normalized;
    try {
      normalized = processClient->normalizeInteractionClassHandle(
          std::move(federationName), requestingFederateId, *interactionClassValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!normalized) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not valid in this federation execution.");
    }
    return static_cast<unsigned long>(*normalized);
  }
#endif
  unsigned long normalized = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Interaction Class Handle requires membership in a federation execution.");
    }
    auto const interactionClassValue = interactionClassHandleValue(interactionClass);
    if (!interactionClassValue) {
      throw InvalidInteractionClassHandle(
          L"Normalize Interaction Class Handle requires a valid InteractionClassHandle.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const result = registry.normalizedInteractionClassHandleValueFor(
        *joinedFederationName_, *interactionClassValue);
    if (!result) {
      throw InvalidInteractionClassHandle(
          L"The supplied InteractionClassHandle is not valid in this federation execution.");
    }
    normalized = *result;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"NormalizeInteractionClassHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class handle",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      {umbra::detail::MomArgumentType::number,
       L"Normalized value",
       umbra::detail::formatMomNumber(std::to_wstring(normalized))});
  return normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Interaction Class Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"NormalizeInteractionClassHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class handle",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

unsigned long UmbraRtiAmbassador::normalizeObjectInstanceHandle(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("normalizeObjectInstanceHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw InvalidObjectInstanceHandle(
          L"Normalize Object Instance Handle requires a valid ObjectInstanceHandle.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Normalize Object Instance Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> normalized;
    try {
      normalized = processClient->normalizeObjectInstanceHandle(
          std::move(federationName), requestingFederateId, *objectInstanceValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!normalized) {
      throw InvalidObjectInstanceHandle(
          L"The supplied ObjectInstanceHandle is not valid in this federation execution.");
    }
    return static_cast<unsigned long>(*normalized);
  }
#endif
  unsigned long normalized = 0UL;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Normalize Object Instance Handle requires membership in a federation execution.");
    }
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw InvalidObjectInstanceHandle(
          L"Normalize Object Instance Handle requires a valid ObjectInstanceHandle.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const result = registry.normalizedObjectInstanceHandleValueFor(
        *joinedFederationName_, *objectInstanceValue);
    if (!result) {
      throw InvalidObjectInstanceHandle(
          L"The supplied ObjectInstanceHandle is not valid in this federation execution.");
    }
    normalized = *result;
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"NormalizeObjectInstanceHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::object_instance_handle,
        L"Object instance handle",
        umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
      {umbra::detail::MomArgumentType::number,
       L"Normalized value",
       umbra::detail::formatMomNumber(std::to_wstring(normalized))});
  return normalized;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Normalize Object Instance Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"NormalizeObjectInstanceHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance handle",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
        describeAmbassadorException(exception));
    throw;
  }
}
}  // namespace rti1516_2025::umbra_binding_detail
