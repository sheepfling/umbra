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
void UmbraRtiAmbassador::subscribeInteractionClass(
    InteractionClassHandle const& interactionClass,
    bool active) {
  auto instrumentationScope = beginRtiCall("subscribeInteractionClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Subscribe Interaction Class requires a defined InteractionClassHandle.");
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
            L"Subscribe Interaction Class requires membership in a federation execution.");
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
      processClient->subscribeInteractionClass(
          std::move(federationName), federateId, *handle, active);
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
    requireFederationServiceOperationAvailable(L"Subscribe Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Subscribe Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Subscribe Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassSubscription(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        active);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The accepted §5.10 subscription transition is the service-report
  // boundary. Emit outside native locks so an HLA_IMMEDIATE observer can
  // safely re-enter this ambassador before declaration advisories are queued.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeInteractionClass",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional passive subscription indicator",
        umbra::detail::formatMomBoolean(!active)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Interaction Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SubscribeInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional passive subscription indicator",
          umbra::detail::formatMomBoolean(!active)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeInteractionClass(
    InteractionClassHandle const& interactionClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeInteractionClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unsubscribe Interaction Class requires a defined InteractionClassHandle.");
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
            L"Unsubscribe Interaction Class requires membership in a federation execution.");
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
      processClient->unsubscribeInteractionClass(
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
    requireFederationServiceOperationAvailable(L"Unsubscribe Interaction Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unsubscribe Interaction Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Unsubscribe Interaction Class requires a defined InteractionClassHandle.");
    }
    auto const result = registry.setInteractionClassSubscription(
        *joinedFederationName_,
        *joinedFederateId_,
        *handle,
        std::nullopt);
    if (result != umbra::detail::InteractionClassDeclarationStatus::applied) {
      throwInteractionClassDeclarationFailure(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The accepted §5.11 unsubscription transition is the service-report
  // boundary. Keep its public MOM interaction outside native locks and ahead
  // of separately queued declaration advisories.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeInteractionClass",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::interaction_class_handle,
        L"Interaction class designator",
        umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Interaction Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeInteractionClass",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::interaction_class_handle,
          L"Interaction class designator",
          umbra::detail::formatMomInteractionClassHandle(interactionClass)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::subscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses,
    bool universally) {
  auto instrumentationScope = beginRtiCall("subscribeObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Subscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
    }
    auto const interactionClassHandleValuesResult =
        ambassadorInteractionClassHandleValues(interactionClasses);
    if (!interactionClassHandleValuesResult) {
      throw InteractionClassNotDefined(
          L"Subscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
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
            L"Subscribe Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->subscribeObjectClassDirectedInteractions(
          std::move(federationName),
          federateId,
          *objectClassHandleValueResult,
          std::vector<std::uint64_t>(
              interactionClassHandleValuesResult->begin(),
              interactionClassHandleValuesResult->end()),
          universally);
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
      L"Subscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Subscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Subscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      ambassadorInteractionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Subscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.subscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult,
      universally);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Subscribe Object Class Directed Interactions");
  }
  }
  // The accepted §5.12 subscription transition is the service-report
  // boundary. The public C++ binding represents the optional universal
  // subscription indicator as its effective defaulted Boolean selector, so a
  // call that relies on the binding default records `false` (by ownership) and
  // an explicit universal subscription records `true`. A supplied empty set
  // remains a real type-28 argument; it is not an absent argument.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"SubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Set of directed interaction designators",
        umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)},
       {umbra::detail::MomArgumentType::boolean,
        L"Optional universal subscription indicator",
         umbra::detail::formatMomBoolean(universally)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Subscribe Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"SubscribeObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::interaction_class_handle_set,
          L"Set of directed interaction designators",
          umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)},
         {umbra::detail::MomArgumentType::boolean,
          L"Optional universal subscription indicator",
          umbra::detail::formatMomBoolean(universally)}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
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
            L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->unsubscribeObjectClassDirectedInteractions(
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
      L"Unsubscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const result = registry.unsubscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      std::nullopt);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unsubscribe Object Class Directed Interactions");
  }
  }
  // The official C++ whole-class overload omits the standards-facing optional
  // set. Preserve that required Table 5 slot as Null rather than conflating it
  // with the supplied-empty InteractionClassHandleSet form.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of directed interaction designators",
         umbra::detail::formatMomNull()}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional set of directed interaction designators",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception));
    throw;
  }
}

void UmbraRtiAmbassador::unsubscribeObjectClassDirectedInteractions(
    ObjectClassHandle const& objectClass,
    InteractionClassHandleSet const& interactionClasses) {
  auto instrumentationScope = beginRtiCall("unsubscribeObjectClassDirectedInteractions");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
    if (!objectClassHandleValueResult) {
      throw ObjectClassNotDefined(
          L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
    }
    auto const interactionClassHandleValuesResult =
        ambassadorInteractionClassHandleValues(interactionClasses);
    if (!interactionClassHandleValuesResult) {
      throw InteractionClassNotDefined(
          L"Unsubscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
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
            L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
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
      processClient->unsubscribeObjectClassDirectedInteractions(
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
      L"Unsubscribe Object Class Directed Interactions");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Unsubscribe Object Class Directed Interactions requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const objectClassHandleValueResult = objectClassHandleValue(objectClass);
  if (!objectClassHandleValueResult) {
    throw ObjectClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires a defined ObjectClassHandle.");
  }
  auto const interactionClassHandleValuesResult =
      ambassadorInteractionClassHandleValues(interactionClasses);
  if (!interactionClassHandleValuesResult) {
    throw InteractionClassNotDefined(
        L"Unsubscribe Object Class Directed Interactions requires defined InteractionClassHandle values.");
  }
  auto const result = registry.unsubscribeObjectClassDirectedInteractions(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectClassHandleValueResult,
      *interactionClassHandleValuesResult);
  if (result != umbra::detail::DirectedInteractionDeclarationStatus::applied) {
    throwDirectedInteractionDeclarationFailure(
        result,
        L"Unsubscribe Object Class Directed Interactions");
  }
  }
  // A supplied empty set is distinct from the whole-class overload above: §5.13
  // defines it as a successful no-op, so it remains a type-28 [] report
  // argument after the registry accepts the invocation.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnsubscribeObjectClassDirectedInteractions",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::interaction_class_handle_set,
        L"Optional set of directed interaction designators",
         umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unsubscribe Object Class Directed Interactions", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnsubscribeObjectClassDirectedInteractions",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::interaction_class_handle_set,
          L"Optional set of directed interaction designators",
          umbra::detail::formatMomInteractionClassHandleSet(interactionClasses)}},
        describeAmbassadorException(exception));
    throw;
  }
}
#endif
}  // namespace rti1516_2025::umbra_binding_detail
