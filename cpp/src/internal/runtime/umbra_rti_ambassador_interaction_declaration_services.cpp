#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/runtime/utf8_string.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/process_federation_client.hpp"
#endif

#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
std::optional<std::set<std::uint64_t>> ambassadorInteractionClassHandleValues(
    InteractionClassHandleSet const& interactionClasses) {
  std::set<std::uint64_t> values;
  for (InteractionClassHandle const& interactionClass : interactionClasses) {
    auto const value = interactionClassHandleValue(interactionClass);
    if (!value) {
      return std::nullopt;
    }
    values.insert(*value);
  }
  return values;
}
void UmbraRtiAmbassador::publishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("publishInteractionClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Publish Interaction Class requires a defined InteractionClassHandle.");
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
            L"Publish Interaction Class requires membership in a federation execution.");
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
      processClient->publishInteractionClass(
          std::move(federationName), federateId, *handle);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Publish Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Publish Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Publish Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassPublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        true);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The accepted §5.4 declaration transition is the service-report boundary.
  // Emit outside native locks so an HLA_IMMEDIATE MOM observer can safely
  // re-enter this ambassador before declaration advisories are queued.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"PublishInteractionClass",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
      umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Interaction Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"PublishInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unpublishInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("unpublishInteractionClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unpublish Interaction Class requires a defined InteractionClassHandle.");
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
            L"Unpublish Interaction Class requires membership in a federation execution.");
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
      processClient->unpublishInteractionClass(
          std::move(federationName), federateId, *handle);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Unpublish Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unpublish Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassPublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        false);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The accepted §5.5 declaration transition is the service-report boundary.
  // Keep the public MOM interaction outside native locks and ahead of any
  // separately queued declaration advisories.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishInteractionClass",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Interaction Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnpublishInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::publishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("publishObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Publish Object Class Directed Interactions requires a defined ObjectClassHandle.");
    }
    auto const interactionClassHandleValuesResult =
        ambassadorInteractionClassHandleValues(interactionClasses);
    if (!interactionClassHandleValuesResult) {
      throw InteractionClassNotDefined(
          L"Publish Object Class Directed Interactions requires defined InteractionClassHandle values.");
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
            L"Publish Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->publishObjectClassDirectedInteractions(
          std::move(federationName),
          federateId,
          *objectClassHandleValueResult,
          std::vector<std::uint64_t>(
              interactionClassHandleValuesResult->begin(),
              interactionClassHandleValuesResult->end()));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Publish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Publish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Publish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      ambassadorInteractionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Publish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.publishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Publish Object Class Directed Interactions");
  }
  }
  // The accepted §5.6 declaration transition is the service-report boundary.
  // An empty interaction-class set is still a successful invocation (and adds
  // no publications), so it must retain its supplied Array<InteractionClassHandle>
  // report argument rather than being treated as an absent argument. Emit
  // outside native locks so an HLA_IMMEDIATE observer can safely re-enter.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"PublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Set of interaction class designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"PublishObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::interaction_class_handle_set,
          L"Set of interaction class designators",
          umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
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
            L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->unpublishObjectClassDirectedInteractions(
          std::move(federationName),
          federateId,
          *objectClassHandleValueResult,
          std::nullopt);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unpublish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const result = registry.unpublishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      std::nullopt);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unpublish Object Class Directed Interactions");
  }
  }
  // The C++ whole-class overload leaves the standards narrative's optional set
  // of interaction class designators unused. Preserve that required report
  // position as Null, rather than conflating it with a supplied-empty set.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of interaction class designators",
         umbra::detail::formatMomNull()}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnpublishObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional set of interaction class designators",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
    }
    auto const interactionClassHandleValuesResult =
        ambassadorInteractionClassHandleValues(interactionClasses);
    if (!interactionClassHandleValuesResult) {
      throw InteractionClassNotDefined(
          L"Unpublish Object Class Directed Interactions requires defined InteractionClassHandle values.");
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
            L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->unpublishObjectClassDirectedInteractions(
          std::move(federationName),
          federateId,
          *objectClassHandleValueResult,
          std::vector<std::uint64_t>(
              interactionClassHandleValuesResult->begin(),
              interactionClassHandleValuesResult->end()));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  {
  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(
      L"Unpublish Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unpublish Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      ambassadorInteractionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Unpublish Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.unpublishObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unpublish Object Class Directed Interactions");
  }
  }
  // Unlike the whole-class overload, this entry point supplies the optional
  // interaction-class set, including an explicitly supplied empty set. Write
  // the successful-void record only after the registry accepts the §5.7
  // declaration transition.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Optional set of interaction class designators",
         umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnpublishObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::interaction_class_handle_set,
          L"Optional set of interaction class designators",
          umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
        describeAmbassadorException(exception));
    throw;
  }
}
#endif
}  // namespace rti1516_2025::umbra_binding_detail
