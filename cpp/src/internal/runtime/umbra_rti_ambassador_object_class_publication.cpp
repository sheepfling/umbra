#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/runtime/utf8_string.hpp"
#endif

#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {

void UmbraRtiAmbassador::publishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("publishObjectClassAttributes");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValueResult = objectClassHandleValue(objectClass);
    if (!objectClassValueResult) {
      throw ObjectClassNotDefined(
          L"Publish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Publish Object Class Attributes requires defined AttributeHandle values.");
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
            L"Publish Object Class Attributes requires membership in a federation execution.");
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
      processClient->publishObjectClassAttributes(
          std::move(federationName),
          federateId,
          *objectClassValueResult,
          std::vector<std::uint64_t>{attributeValues->begin(), attributeValues->end()});
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  std::wstring federationName;
  std::vector<umbra::detail::DeclarationAdvisory> declarationAdvisories;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient>
      newlyEligibleAssumptions;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Publish Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Publish Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Publish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Publish Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        true);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result);
    }
    federationName = *joinedFederationName_;
    declarationAdvisories = registry.planDeclarationAdvisories(federationName);
    newlyEligibleAssumptions = registry.planAttributeOwnershipAssumptionsForFederate(
        federationName,
        *joinedFederateId_);
  }
  // The accepted §5.2 publication transition is the service-report boundary.
  // Keep it before separately queued declaration advisories and ownership
  // assumptions, and outside native locks so HLA_IMMEDIATE observers may
  // safely re-enter the ambassador.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"PublishObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_handle_set,
        L"Set of attribute designators",
        umbra::detail::formatMomAttributeHandleSet(attributes)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(newlyEligibleAssumptions),
      federationName,
      VariableLengthData());
  } catch (Exception const& exception) {
    emitExceptionReport(L"Publish Object Class Attributes", exception);
    appendFailedServiceReportToFileIfSelected(
        L"PublishObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClass(
    ObjectClassHandle const& objectClass) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClass");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class requires a defined ObjectClassHandle.");
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
            L"Unpublish Object Class requires membership in a federation execution.");
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
      processClient->unpublishObjectClass(
          std::move(federationName), federateId, *objectClassValue);
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
    requireFederationServiceOperationAvailable(L"Unpublish Object Class");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Object Class requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class requires a defined ObjectClassHandle.");
    }

    // The registry exposes the complete current publication set, including the
    // implicit HLAprivilegeToDeleteObject publication for the active epoch.
    // Reuse the attribute-set service so its pending-acquisition and ownership
    // boundaries remain identical for the whole-class and subset forms.
    auto const publishedAttributes = registry.publishedObjectClassAttributeHandles(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle);
    if (!publishedAttributes) {
      throw ObjectClassNotDefined(
          L"The supplied ObjectClassHandle is not defined in this federation execution.");
    }

    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *publishedAttributes,
        false);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The C++ whole-class overload is the no-optional-set form of the §5.3
  // Unpublish Object Class Attributes service. Preserve that service name and
  // its required optional-argument position as Table 5 Null. Emit after the
  // accepted registry transition and before separately queued advisories,
  // outside native locks so HLA_IMMEDIATE observers may safely re-enter.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::null_value,
        L"Optional set of attribute designators",
        umbra::detail::formatMomNull()}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnpublishObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional set of attribute designators",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

void UmbraRtiAmbassador::unpublishObjectClassAttributes(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes) {
  auto instrumentationScope = beginRtiCall("unpublishObjectClassAttributes");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Unpublish Object Class Attributes requires defined AttributeHandle values.");
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
            L"Unpublish Object Class Attributes requires membership in a federation execution.");
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
      processClient->unpublishObjectClassAttributes(
          std::move(federationName),
          federateId,
          *objectClassValue,
          std::vector<std::uint64_t>{attributeValues->begin(), attributeValues->end()});
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
    requireFederationServiceOperationAvailable(L"Unpublish Object Class Attributes");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unpublish Object Class Attributes requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectClassHandle = objectClassHandleValue(objectClass);
    if (!objectClassHandle) {
      throw ObjectClassNotDefined(
          L"Unpublish Object Class Attributes requires a defined ObjectClassHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Unpublish Object Class Attributes requires defined AttributeHandle values.");
    }
    auto const result = registry.setObjectClassAttributePublication(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassHandle,
        *attributeHandles,
        false);
    if (result != umbra::detail::ObjectClassAttributeDeclarationStatus::applied) {
      throwObjectClassAttributeDeclarationFailureForFederationManagement(result);
    }
    declarationAdvisories = registry.planDeclarationAdvisories(*joinedFederationName_);
  }
  // The §5.3 unpublication and its synchronous ownership cleanup have
  // succeeded before this point. Keep the successful-void record ahead of
  // separately queued declaration advisories and outside native locks so
  // HLA_IMMEDIATE observers may safely re-enter the ambassador.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnpublishObjectClassAttributes",
      umbra::detail::MomServiceType::declaration_management,
      {{umbra::detail::MomArgumentType::object_class_handle,
        L"Object class designator",
        umbra::detail::formatMomObjectClassHandle(objectClass)},
       {umbra::detail::MomArgumentType::attribute_handle_set,
        L"Optional set of attribute designators",
        umbra::detail::formatMomAttributeHandleSet(attributes)}},
      true);
  queueAmbassadorDeclarationAdvisories(std::move(declarationAdvisories));
  } catch (Exception const& exception) {
    emitExceptionReport(L"Unpublish Object Class Attributes", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UnpublishObjectClassAttributes",
        umbra::detail::MomServiceType::declaration_management,
        {{umbra::detail::MomArgumentType::object_class_handle,
          L"Object class designator",
          umbra::detail::formatMomObjectClassHandle(objectClass)},
         {umbra::detail::MomArgumentType::attribute_handle_set,
          L"Optional set of attribute designators",
          umbra::detail::formatMomAttributeHandleSet(attributes)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
