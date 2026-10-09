#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/umbra_rti_ambassador_ownership_callback_dispatch.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/federation/federation_management_coordinator.hpp"
#include "internal/federation/federation_registry.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/utf8_string.hpp"

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include <RTI/FederateAmbassador.h>

#include <cstdint>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

void UmbraRtiAmbassador::queryAttributeOwnership(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes) {
  auto instrumentationScope = beginRtiCall("queryAttributeOwnership");
  try {
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
            L"Query Attribute Ownership requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Query Attribute Ownership requires a known ObjectInstanceHandle.");
    }
    auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!requestedAttributeHandles) {
      throw AttributeNotDefined(
          L"Query Attribute Ownership requires defined AttributeHandle values.");
    }

    auto const processReportArguments =
        std::vector<umbra::detail::MomServiceArgument>{
            {umbra::detail::MomArgumentType::object_instance_handle,
             L"Object instance designator",
             umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
            {umbra::detail::MomArgumentType::attribute_handle_set,
             L"Set of attribute designators",
             umbra::detail::formatMomAttributeHandleSet(attributes)},
        };

    umbra::detail::ProcessFederationAttributeOwnershipQueryResult result;
    try {
      result = processClient->queryAttributeOwnership(
          federationName,
          requestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              requestedAttributeHandles->begin(),
              requestedAttributeHandles->end()));
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::AttributeOwnershipQueryStatus::applied) {
      throwAttributeOwnershipQueryFailure(result.status);
    }
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"QueryAttributeOwnership",
        umbra::detail::MomServiceType::ownership_management,
        processReportArguments,
        true);
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent object services.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Ownership");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Ownership requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Query Attribute Ownership requires a known ObjectInstanceHandle.");
  }
  auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Query Attribute Ownership requires defined AttributeHandle values.");
  }
  // Section 7.17 names these supplied values "Object instance designator"
  // and "Set of attribute designators". Table 5 fixes their respective
  // type-37 and type-1 value forms as quoted handle.toString() text and a
  // bracketed array of quoted handle.toString() values.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Ownership");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Ownership requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipQuery(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeOwnershipQueryStatus::applied) {
      throwAttributeOwnershipQueryFailure(plan.status);
    }
  }

  // The accepted request is the service-report boundary. Section 7.18
  // separately requires one or more later ownership-result callbacks. Emit
  // the public report after releasing the registry lock so HLA_IMMEDIATE
  // recipients cannot re-enter the ownership-query transaction.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"QueryAttributeOwnership",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::uint64_t requestId = 0;
    umbra::detail::AttributeOwnershipQueryReportKind reportKind =
        umbra::detail::AttributeOwnershipQueryReportKind::unowned;
    std::uint64_t owningFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const & recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an ownership-query recipient without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.requestId,
        recipient.reportKind,
        recipient.owningFederateId,
        recipient.attributeHandles,
    });
  }

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter Inform Attribute Ownership user code.
  for (auto& delivery : deliveries) {
    queueAmbassadorAttributeOwnershipQueryReport(
        std::move(delivery.callbackRoute),
        federationName,
        delivery.requestId,
        requestingFederateId,
        *objectInstanceHandle,
        delivery.reportKind,
        delivery.owningFederateId,
        std::move(delivery.requestedAttributeHandles));
  }
  } catch (Exception const & exception) {
    emitExceptionReport(L"Query Attribute Ownership", exception);
    throw;
  }
}

bool UmbraRtiAmbassador::isAttributeOwnedByFederate(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandle const & attribute) {
  auto instrumentationScope = beginRtiCall("isAttributeOwnedByFederate");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceHandleValueResult = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Is Attribute Owned By Federate requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandleValueResult = attributeHandleValue(attribute);
    if (!attributeHandleValueResult) {
      throw AttributeNotDefined(
          L"Is Attribute Owned By Federate requires a defined AttributeHandle.");
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
            L"Is Attribute Owned By Federate requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationAttributeOwnershipCheckResult result;
    try {
      result = processClient->attributeOwnershipCheck(
          std::move(federationName),
          federateId,
          *objectInstanceHandleValueResult,
          *attributeHandleValueResult);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status != umbra::detail::AttributeOwnershipCheckStatus::applied) {
      throwAttributeOwnershipCheckFailure(result.status);
    }
    return result.ownedByRequestingFederate;
  }
#endif
  // This uses the same official connection, membership, known-instance, and
  // known-class boundaries as Query Attribute Ownership, but returns only the
  // invoking federate's boolean ownership status and has no callback effect.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Is Attribute Owned By Federate");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Is Attribute Owned By Federate requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Is Attribute Owned By Federate requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandle = attributeHandleValue(attribute);
  if (!attributeHandle) {
    throw AttributeNotDefined(
        L"Is Attribute Owned By Federate requires a defined AttributeHandle.");
  }

  std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
  requireConnectedForFederationManagement(lifecycle_);
  requireFederationServiceOperationAvailable(L"Is Attribute Owned By Federate");
  if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
      !joinedFederationName_ || !joinedFederateId_) {
    throw FederateNotExecutionMember(
        L"Is Attribute Owned By Federate requires membership in a federation execution.");
  }
  auto& registry = embeddedFederationRegistry();
  if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  auto const result = registry.attributeOwnedByFederate(
      *joinedFederationName_,
      *joinedFederateId_,
      *objectInstanceHandle,
      *attributeHandle);
  if (result.status != umbra::detail::AttributeOwnershipCheckStatus::applied) {
    throwAttributeOwnershipCheckFailure(result.status);
  }
  return result.ownedByRequestingFederate;
  } catch (Exception const & exception) {
    emitExceptionReport(L"Is Attribute Owned By Federate", exception);
    throw;
  }
}

void UmbraRtiAmbassador::negotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("negotiatedAttributeOwnershipDivestiture");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t divestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Negotiated Attribute Ownership Divestiture requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      divestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->negotiatedAttributeOwnershipDivestiture(
          federationName,
          divestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandles->begin(), attributeHandles->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status !=
          umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::
              applied) {
        throwNegotiatedAttributeOwnershipDivestitureFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Preserve the original tag with the private Waiting state. The bounded
  // negotiated planner may carry it to Request Divestiture Confirmation for
  // either a regular or an If Available candidate; a later owner-search
  // extension must retain this exact 2025 tag for its own callback paths.
  auto copiedTag = umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
  // Section 7.3.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Build the file-text forms before the
  // registry commits the private Waiting state, so formatting cannot leave an
  // accepted request without its selected service-report record. The Table 5
  // tag literal stays confined to private file reporting (RL-077), not a
  // future public MOM-interaction representation.
  VariableLengthData const reportTag(userSuppliedTag);
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  std::vector<umbra::detail::AttributeOwnershipAssumptionRecipient> assumptionRecipients;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planNegotiatedAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::NegotiatedAttributeOwnershipDivestitureStatus::applied) {
      throwNegotiatedAttributeOwnershipDivestitureFailure(plan.status);
    }
    workItems = std::move(plan.workItems);
    assumptionRecipients = std::move(plan.assumptionRecipients);
  }

  // Report only after the registry transaction releases both native locks.
  // This permits the standard MOM interaction route without re-entering the
  // federation lock. Queue follow-up ownership callbacks only after the
  // accepted Section 7.3 service report.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"NegotiatedAttributeOwnershipDivestiture",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
  // Negotiated divestiture also starts Request Attribute Ownership Assumption
  // callbacks for currently eligible federates. The registry carries the
  // original divestiture tag on each grouped recipient; the empty fallback is
  // used only by legacy unconditional/resign records that do not carry one.
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(assumptionRecipients),
      federationName,
      VariableLengthData());
  } catch (Exception const & exception) {
    emitExceptionReport(L"Negotiated Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::confirmDivestiture(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & confirmedAttributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("confirmDivestiture");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t divestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Confirm Divestiture requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      divestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Confirm Divestiture requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(confirmedAttributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Confirm Divestiture requires defined AttributeHandle values.");
    }
    // Build the same successful-void record before crossing the process
    // boundary, preserving Table 5's file type 63 for the coordinator's
    // report route (which maps the public static MIM type separately).
    VariableLengthData const processReportTag(userSuppliedTag);
    auto const processReportArguments =
        std::vector<umbra::detail::MomServiceArgument>{
            {umbra::detail::MomArgumentType::object_instance_handle,
             L"Object instance designator",
             umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
            {umbra::detail::MomArgumentType::attribute_handle_set,
             L"Set of attribute designators",
             umbra::detail::formatMomAttributeHandleSet(confirmedAttributes)},
            {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
             L"User-supplied tag",
             umbra::detail::formatMomUserSuppliedTag(processReportTag)},
        };
    try {
      auto const result = processClient->confirmDivestiture(
          federationName,
          divestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandles->begin(), attributeHandles->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status != umbra::detail::ConfirmDivestitureStatus::applied) {
        throwConfirmDivestitureFailure(result.status);
      }
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"ConfirmDivestiture",
          umbra::detail::MomServiceType::ownership_management,
          processReportArguments,
          true);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Confirm Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(L"Confirm Divestiture requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(L"Confirm Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(confirmedAttributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(L"Confirm Divestiture requires defined AttributeHandle values.");
  }
  auto copiedTag = umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
  // Section 7.6.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Keep Table 5's type-63 form in the
  // report arguments; the public MOM encoder maps it to static MIM type 60.
  VariableLengthData const reportTag(userSuppliedTag);
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(confirmedAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  umbra::detail::ConfirmDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Confirm Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(L"Confirm Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planConfirmDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::ConfirmDivestitureStatus::applied) {
      throwConfirmDivestitureFailure(plan.status);
    }
  }

  // Record the successful ownership transfer only after the registry
  // transaction releases both native locks. This enables the public MOM
  // interaction route and keeps it ahead of queued acquisition notifications.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ConfirmDivestiture",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  queueAmbassadorConfirmDivestitureNotifications(std::move(plan.notifications), federationName);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Confirm Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::cancelNegotiatedAttributeOwnershipDivestiture(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes) {
  auto instrumentationScope = beginRtiCall("cancelNegotiatedAttributeOwnershipDivestiture");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t divestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Cancel Negotiated Attribute Ownership Divestiture requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      divestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Cancel Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Cancel Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->cancelNegotiatedAttributeOwnershipDivestiture(
          federationName,
          divestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandles->begin(), attributeHandles->end()));
      if (result.status !=
          umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus::
              applied) {
        throwCancelNegotiatedAttributeOwnershipDivestitureFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Cancel Negotiated Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Cancel Negotiated Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Section 7.14.1 supplies the object instance designator and set of
  // attribute designators. Preserve both source forms before the registry
  // commits the cancellation, so formatting cannot leave an accepted
  // cancellation without its selected file record.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Negotiated Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Negotiated Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planCancelNegotiatedAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status !=
        umbra::detail::CancelNegotiatedAttributeOwnershipDivestitureStatus::applied) {
      throwCancelNegotiatedAttributeOwnershipDivestitureFailure(plan.status);
    }
    followupWorkItems = std::move(plan.followupWorkItems);
  }

  // The accepted Section 7.14 cancellation is the service-report boundary.
  // Emit after releasing the registry and ambassador locks: an interaction-
  // selected HLA_IMMEDIATE observer may synchronously re-enter the public
  // surface.  Keep this report ahead of the separately queued regular-
  // acquisition follow-up work, preserving the service-before-callback order
  // required by the MOM contract.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"CancelNegotiatedAttributeOwnershipDivestiture",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(std::move(followupWorkItems), federationName);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Cancel Negotiated Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::unconditionalAttributeOwnershipDivestiture(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("unconditionalAttributeOwnershipDivestiture");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t divestingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Unconditional Attribute Ownership Divestiture requires federation membership.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      divestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Unconditional Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandlesResult = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandlesResult) {
      throw AttributeNotDefined(
          L"Unconditional Attribute Ownership Divestiture requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->unconditionalAttributeOwnershipDivestiture(
          federationName,
          divestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandlesResult->begin(),
              attributeHandlesResult->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag),
          callbacks_->isEnabled());
      if (!result.value) {
        throw RTIinternalError(
            L"The process endpoint did not accept Unconditional Attribute Ownership Divestiture.");
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unconditional Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unconditional Attribute Ownership Divestiture requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Unconditional Attribute Ownership Divestiture requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Unconditional Attribute Ownership Divestiture requires defined AttributeHandle values.");
  }
  // Copy before the registry commits the immediate unowned state, so a failed
  // allocation never leaves an ownership-assumption callback without the
  // required divestiture tag.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.2.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and the tag Binary Data. The tag keeps Table 5's type-63
  // representation here; the HLAreportServiceInvocation encoder translates it
  // to the static MIM's UserSuppliedTag value 60 for public MOM interactions.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
  };

  std::wstring federationName;
  umbra::detail::UnconditionalAttributeOwnershipDivestiturePlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Unconditional Attribute Ownership Divestiture");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Unconditional Attribute Ownership Divestiture requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planUnconditionalAttributeOwnershipDivestiture(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        umbra::detail::variable_length_data_2025::copyBytes(copiedTag));
    if (plan.status !=
        umbra::detail::UnconditionalAttributeOwnershipDivestitureStatus::applied) {
      throwUnconditionalAttributeOwnershipDivestitureFailure(plan.status);
    }
  }

  // Report only after the registry transaction releases both native locks.
  // This permits the standard MOM interaction route without re-entering the
  // federation lock. Queue ownership callbacks only after the service report,
  // preserving the service-call boundary ahead of later callback work.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UnconditionalAttributeOwnershipDivestiture",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  // Established regular requests receive their normal acquisition work first.
  // Any HLA_IMMEDIATE acquisition can therefore make an attribute owned before
  // a later assumption offer is submitted; each offer independently rechecks
  // its standard unowned/published/pending preconditions at callback entry.
  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
      std::move(plan.acquisitionWorkItems),
      federationName);
  queueAmbassadorAttributeOwnershipAssumptionRecipients(
      std::move(plan.assumptionRecipients),
      federationName,
      copiedTag);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Unconditional Attribute Ownership Divestiture", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipAcquisition(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & desiredAttributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipAcquisition");
  try {
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
            L"Attribute Ownership Acquisition requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
    }
    auto const desiredAttributeHandles = ambassadorAttributeHandleValues(desiredAttributes);
    if (!desiredAttributeHandles) {
      throw AttributeNotDefined(
          L"Attribute Ownership Acquisition requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->attributeOwnershipAcquisition(
          federationName,
          requestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              desiredAttributeHandles->begin(),
              desiredAttributeHandles->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status !=
          umbra::detail::AttributeOwnershipAcquisitionStatus::applied) {
        throwAttributeOwnershipAcquisitionFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
  }
  auto const desiredAttributeHandles = ambassadorAttributeHandleValues(desiredAttributes);
  if (!desiredAttributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Acquisition requires defined AttributeHandle values.");
  }
  // Copy before the registry accepts the request so a failed allocation never
  // leaves a private acquisition reservation without its callback tag.
  auto copiedTag = umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
  VariableLengthData const reportTag(userSuppliedTag);
  // Section 7.8.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag.  Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63.  Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(desiredAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(reportTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> workItems;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeOwnershipAcquisition(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *desiredAttributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::AttributeOwnershipAcquisitionStatus::applied) {
      throwAttributeOwnershipAcquisitionFailure(plan.status);
    }
    // The accepted Section 7.8 request is the service-report boundary. Write
    // it before any separately queued release or acquisition work, so the
    // selected file records this successful invocation rather than a later
    // ownership callback.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipAcquisition",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    workItems = std::move(plan.workItems);
  }

  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(std::move(workItems), federationName);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Attribute Ownership Acquisition", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipAcquisitionIfAvailable(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & desiredAttributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipAcquisitionIfAvailable");
  try {
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
            L"Attribute Ownership Acquisition If Available requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Attribute Ownership Acquisition If Available requires a known ObjectInstanceHandle.");
    }
    auto const desiredAttributeHandles = ambassadorAttributeHandleValues(desiredAttributes);
    if (!desiredAttributeHandles) {
      throw AttributeNotDefined(
          L"Attribute Ownership Acquisition If Available requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->attributeOwnershipAcquisitionIfAvailable(
          federationName,
          requestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              desiredAttributeHandles->begin(),
              desiredAttributeHandles->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status !=
          umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied) {
        throwAttributeOwnershipAcquisitionIfAvailableFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Acquisition If Available");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition If Available requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Acquisition If Available requires a known ObjectInstanceHandle.");
  }
  auto const desiredAttributeHandles = ambassadorAttributeHandleValues(desiredAttributes);
  if (!desiredAttributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Acquisition If Available requires defined AttributeHandle values.");
  }
  // Copy before the registry accepts the request so a failed allocation never
  // leaves a private acquisition reservation without a callback payload.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.9.1 supplies the object instance designator, set of attribute
  // designators, and user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63. Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(desiredAttributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionIfAvailablePlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Acquisition If Available");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Acquisition If Available requires federation membership.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipAcquisitionIfAvailable(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *desiredAttributeHandles,
        umbra::detail::variable_length_data_2025::copyBytes(copiedTag));
    if (plan.status !=
        umbra::detail::AttributeOwnershipAcquisitionIfAvailableStatus::applied) {
      throwAttributeOwnershipAcquisitionIfAvailableFailure(plan.status);
    }
    // A missing callback route means this invocation cannot complete
    // successfully. Roll back the private reservation before it can reserve a
    // report serial or append a successful-void record.
    if (plan.requestId != 0 && !plan.callbackRoute) {
      registry.cancelAttributeOwnershipAcquisitionIfAvailable(
          federationName,
          requestingFederateId,
          *objectInstanceHandle,
          plan.requestId);
      throw RTIinternalError(
          L"The embedded federation has an ownership-acquisition requester without a callback route.");
    }
    // The accepted Section 7.9 request is the service-report boundary. Write
    // it before the supplied-empty no-callback return or any separately queued
    // unavailable/acquisition callback, so the file records the successful
    // invocation rather than later ownership delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipAcquisitionIfAvailable",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
  }

  // The empty-attribute case has no ownership transition or callback, but it
  // remains a successful supplied-empty Section 7.9 invocation and was
  // recorded above.
  if (plan.requestId == 0) {
    return;
  }

  try {
    // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
    // may synchronously establish ownership and enter its standard callbacks.
    ownership_callback_detail::queueAttributeOwnershipAcquisitionIfAvailableReport(
        std::move(plan.callbackRoute),
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        plan.requestId,
        std::move(copiedTag));
  } catch (...) {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    embeddedFederationRegistry().cancelAttributeOwnershipAcquisitionIfAvailable(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        plan.requestId);
    throw;
  }
  } catch (Exception const & exception) {
    emitExceptionReport(L"Attribute Ownership Acquisition If Available", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipReleaseDenied(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes,
    VariableLengthData const & userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipReleaseDenied");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t owningFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Attribute Ownership Release Denied requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      owningFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Attribute Ownership Release Denied requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Attribute Ownership Release Denied requires defined AttributeHandle values.");
    }
    try {
      auto const result = processClient->attributeOwnershipReleaseDenied(
          federationName,
          owningFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandles->begin(), attributeHandles->end()),
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status !=
          umbra::detail::AttributeOwnershipReleaseDeniedStatus::applied) {
        throwAttributeOwnershipReleaseDeniedFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching regular acquisition.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Release Denied");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Release Denied requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Release Denied requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Release Denied requires defined AttributeHandle values.");
  }
  // Copy before the registry terminates pending acquisitions so a failed
  // allocation cannot leave their required unavailable callbacks tagless.
  VariableLengthData copiedTag = userSuppliedTag;
  // Section 7.12.1 supplies the object instance designator, the set of
  // attribute designators for which the joined federate is unwilling to divest
  // ownership, and the user-supplied tag. Table 5 makes the first two type-37
  // and type-1 forms and depicts its Binary Data tag as type 63. Keep that
  // source-specific literal confined to the private file record (RL-077), not
  // a future live MOM interaction path.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators for which the joined federate is unwilling to divest ownership",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };

  std::wstring federationName;
  std::vector<umbra::detail::AttributeOwnershipUnavailableRecipient> recipients;
  std::vector<umbra::detail::AttributeOwnershipAcquisitionWorkItem> followupWorkItems;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Attribute Ownership Release Denied");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Release Denied requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const owningFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, owningFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeOwnershipReleaseDenied(
        federationName,
        owningFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::AttributeOwnershipReleaseDeniedStatus::applied) {
      throwAttributeOwnershipReleaseDeniedFailure(plan.status);
    }
    // The accepted Section 7.12 denial is the service-report boundary. Write
    // it before any separately queued Attribute Ownership Unavailable
    // callbacks, so the selected file records the successful invocation rather
    // than downstream acquisition termination delivery.
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"AttributeOwnershipReleaseDenied",
        umbra::detail::MomServiceType::ownership_management,
        reportArguments);
    recipients = std::move(plan.recipients);
    followupWorkItems = std::move(plan.followupWorkItems);
  }

  ownership_callback_detail::queueAttributeOwnershipUnavailableRecipients(
      std::move(recipients),
      federationName,
      copiedTag);
  queueAmbassadorAttributeOwnershipAcquisitionWorkItems(
      std::move(followupWorkItems),
      federationName);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Attribute Ownership Release Denied", exception);
    throw;
  }
}

void UmbraRtiAmbassador::attributeOwnershipDivestitureIfWanted(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes,
    VariableLengthData const & userSuppliedTag,
    AttributeHandleSet& divestedAttributes) {
  auto instrumentationScope = beginRtiCall("attributeOwnershipDivestitureIfWanted");
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the adjacent ownership
  // services. Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Divestiture If Wanted");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Divestiture If Wanted requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Attribute Ownership Divestiture If Wanted requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Attribute Ownership Divestiture If Wanted requires defined AttributeHandle values.");
  }
  // Copy before the registry commits the synchronous transfer, so an
  // allocation failure cannot leave a notification without its required
  // divestiture tag.
  auto copiedTag = umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);

  std::wstring federationName;
  umbra::detail::AttributeOwnershipDivestitureIfWantedPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Attribute Ownership Divestiture If Wanted");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Attribute Ownership Divestiture If Wanted requires federation membership.");
    }
    federationName = *joinedFederationName_;
    auto const divestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, divestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipDivestitureIfWanted(
        federationName,
        divestingFederateId,
        *objectInstanceHandle,
        *attributeHandles,
        std::move(copiedTag));
    if (plan.status != umbra::detail::AttributeOwnershipDivestitureIfWantedStatus::applied) {
      throwAttributeOwnershipDivestitureIfWantedFailure(plan.status);
    }
  }

  // The binding's out parameter reports exactly the subset that transferred
  // synchronously. It is assigned before routes are submitted because an
  // HLA_IMMEDIATE notification may enter user code before this service returns.
  AttributeHandleSet resolvedDivestedAttributes;
  for (std::uint64_t const attributeHandle : plan.divestedAttributeHandles) {
    resolvedDivestedAttributes.insert(makeAttributeHandle(attributeHandle));
  }
  divestedAttributes = std::move(resolvedDivestedAttributes);

  queueAmbassadorAttributeOwnershipDivestitureIfWantedNotifications(
      std::move(plan.notifications),
      federationName);
  } catch (Exception const & exception) {
    emitExceptionReport(L"Attribute Ownership Divestiture If Wanted", exception);
    throw;
  }
}

void UmbraRtiAmbassador::cancelAttributeOwnershipAcquisition(
    ObjectInstanceHandle const & objectInstance,
    AttributeHandleSet const & attributes) {
  auto instrumentationScope = beginRtiCall("cancelAttributeOwnershipAcquisition");
  try {
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
            L"Cancel Attribute Ownership Acquisition requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceHandleValueResult =
        objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandleValueResult) {
      throw ObjectInstanceNotKnown(
          L"Cancel Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Cancel Attribute Ownership Acquisition requires defined AttributeHandle values.");
    }
    auto const processReportArguments =
        std::vector<umbra::detail::MomServiceArgument>{
            {umbra::detail::MomArgumentType::object_instance_handle,
             L"Object instance designator",
             umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
            {umbra::detail::MomArgumentType::attribute_handle_set,
             L"Set of attribute designators",
             umbra::detail::formatMomAttributeHandleSet(attributes)},
        };
    try {
      auto const result = processClient->cancelAttributeOwnershipAcquisition(
          federationName,
          requestingFederateId,
          *objectInstanceHandleValueResult,
          std::vector<std::uint64_t>(
              attributeHandles->begin(), attributeHandles->end()));
      if (result.status !=
          umbra::detail::AttributeOwnershipAcquisitionCancellationStatus::
              applied) {
        throwAttributeOwnershipAcquisitionCancellationFailure(result.status);
      }
      appendSuccessfulVoidServiceReportToFileIfSelected(
          L"CancelAttributeOwnershipAcquisition",
          umbra::detail::MomServiceType::ownership_management,
          processReportArguments,
          true);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const & error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the regular acquisition path.
  // Save/restore has no implemented state in this profile.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Attribute Ownership Acquisition requires federation membership.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Cancel Attribute Ownership Acquisition requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Cancel Attribute Ownership Acquisition requires defined AttributeHandle values.");
  }
  // Section 7.15.1 supplies the object instance designator and the set of
  // attribute designators.  Preserve both source forms before the registry
  // commits the cancellation, so formatting cannot leave an accepted
  // cancellation without its selected file record.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
  };

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeOwnershipAcquisitionCancellationPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Cancel Attribute Ownership Acquisition");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Cancel Attribute Ownership Acquisition requires federation membership.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeOwnershipAcquisitionCancellation(
        federationName,
        requestingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status !=
        umbra::detail::AttributeOwnershipAcquisitionCancellationStatus::applied) {
      throwAttributeOwnershipAcquisitionCancellationFailure(plan.status);
    }
  }

  // The accepted Section 7.15 cancellation is the service-report boundary.
  // Emit it after releasing native locks so an HLA_IMMEDIATE observer can
  // safely re-enter the RTI, and before either the supplied-empty no-callback
  // return or the separately queued confirmation callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"CancelAttributeOwnershipAcquisition",
      umbra::detail::MomServiceType::ownership_management,
      reportArguments,
      true);

  // An empty attribute set has no cancellation transition or callback, but it
  // remains a successful supplied-empty Section 7.15 invocation and was
  // recorded above.
  if (plan.cancellationId == 0) {
    return;
  }
  queueAmbassadorAttributeOwnershipAcquisitionCancellationConfirmation(
      std::move(plan.callbackRoute),
      federationName,
      requestingFederateId,
      *objectInstanceHandle,
      plan.cancellationId,
      std::move(plan.attributeHandles));
  } catch (Exception const & exception) {
    emitExceptionReport(L"Cancel Attribute Ownership Acquisition", exception);
    throw;
  }
}

#endif

}  // namespace rti1516_2025::umbra_binding_detail
