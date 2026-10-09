#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/federation/federation_registry.hpp"
#include "internal/federation/process_federation_client.hpp"
#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/region_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectInstanceHandle const &objectInstance,
    AttributeHandleSet const &attributes,
    VariableLengthData const &userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdate");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns this object-instance request slice.
  // Validate official handles on the public adapter, then carry only copied
  // values over the private request/response seam; the provider callback is
  // returned through the existing process receive fence.
  if (processEndpointActive_) {
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Request Attribute Value Update requires a known ObjectInstanceHandle.");
    }
    auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!requestedAttributeHandles) {
      throw AttributeNotDefined(
          L"Request Attribute Value Update requires defined AttributeHandle values.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Attribute Value Update requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
    try {
      static_cast<void>(processClient->requestAttributeValueUpdate(
          std::move(federationName),
          requestingFederateId,
          *objectInstanceValue,
          std::vector<std::uint64_t>(requestedAttributeHandles->begin(),
                                     requestedAttributeHandles->end()),
          std::move(processTag)));
      while (processClient->pendingPushedAttributeValueUpdateRequestCount() !=
             0U) {
        processClient->dispatchPushedAttributeValueUpdateRequest();
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the other object services.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
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
        L"Request Attribute Value Update requires a known ObjectInstanceHandle.");
  }
  auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update requires defined AttributeHandle values.");
  }

  // A joined-federate MOM object is RTI-owned: Request Attribute Value Update
  // reflects the immutable MOM values directly and never induces a Provide
  // Attribute Value Update callback at a federate. Keep this branch separate
  // from the ordinary ownership planner so the private MOM ledger is not
  // mistaken for a federate-created object instance.
  std::optional<umbra::detail::ObjectInstanceCallbackRoute> momCallbackRoute;
  std::optional<std::map<std::uint64_t, VariableLengthData>>
      momPlannedAttributeValues;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> momReportArguments;
  std::wstring momFederationName;
  std::uint64_t momFederateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    auto &registry = embeddedFederationRegistry();
    auto momPlan = registry.planJoinedFederateMomAttributeValueUpdate(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (momPlan.rtiOwnedMomObject) {
      if (momPlan.status !=
          umbra::detail::JoinedFederateMomAttributeValueUpdateStatus::applied ||
          !momPlan.recipient) {
        throwJoinedFederateMomAttributeValueUpdateFailure(momPlan.status);
      }
      VariableLengthData copiedTag(userSuppliedTag);
      momReportArguments = std::vector<umbra::detail::MomServiceArgument>{
          {umbra::detail::MomArgumentType::object_instance_handle,
           L"Object instance designator",
           umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
          {umbra::detail::MomArgumentType::attribute_handle_set,
           L"Set of attribute designators",
           umbra::detail::formatMomAttributeHandleSet(attributes)},
          {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
           L"User-supplied tag",
           umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      };
      momFederationName = *joinedFederationName_;
      momFederateId = *joinedFederateId_;
      momCallbackRoute = std::move(momPlan.recipient->callbackRoute);
      // Request Attribute Value Update samples an RTI-owned MOM value at the
      // accepted service boundary.  Preserve that immutable plan through the
      // callback queue so a receive-order callback delivered ahead of the
      // MOM reflection cannot change a value such as HLAROlength before
      // the requested reflection is observed.
      momPlannedAttributeValues = std::move(
          momPlan.recipient->attributeValues);
    }
  }
  if (momCallbackRoute) {
    if (!momReportArguments) {
      throw RTIinternalError(
          L"Umbra could not format the accepted MOM Request Attribute Value Update report.");
    }
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestAttributeValueUpdate",
        umbra::detail::MomServiceType::object_management,
        *momReportArguments,
        true);
    queueAmbassadorJoinedFederateMomAttributeValueUpdate(
        std::move(*momCallbackRoute),
        std::move(momFederationName),
        momFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles,
        false,
        std::move(momPlannedAttributeValues));
    return;
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> reportArguments;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeValueUpdateRequest(
        *federationName,
        *requestingFederateId,
        *objectInstanceHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeValueUpdateRequestStatus::applied) {
      throwAttributeValueUpdateRequestFailure(plan.status);
    }
    return plan;
  };

  // The request has no caller-owned value payload, but copy the tag only
  // after its current object and attribute boundary has been validated. Then
  // replan before routing so a concurrent resign or lifecycle change cannot
  // use a stale recipient snapshot.
  static_cast<void>(planCurrentRequest());
  VariableLengthData copiedTag(userSuppliedTag);
  // Section 6.21 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "User-supplied tag".  Table 5 fixes
  // their type-37, type-1, and Binary Data value forms respectively.  Preserve
  // the copied tag in the file record so a caller-owned input buffer cannot
  // alter the accepted service's audit boundary.
  reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  auto plan = planCurrentRequest();
  if (!reportArguments) {
    throw RTIinternalError(
        L"Umbra could not format the accepted Request Attribute Value Update report.");
  }
  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
    std::uint64_t requestId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const &recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
        0U,
    });
  }

  // Persist each provider group before exposing the accepted service report
  // or queuing a callback. The registry owns the request identity and copies
  // the caller tag into its route-free ledger; a failed registration is a
  // deterministic RTI error rather than a silent in-memory fallback.
  for (auto &delivery : deliveries) {
    auto const requestId = embeddedFederationRegistry()
        .registerAttributeValueUpdateRequest(
            *federationName,
            *requestingFederateId,
            delivery.providingFederateId,
            *objectInstanceHandle,
            delivery.requestedAttributeHandles,
            umbra::detail::variable_length_data_2025::copyBytes(copiedTag));
    if (!requestId) {
      throw RTIinternalError(
          L"The embedded federation could not persist the Request Attribute Value Update request.");
    }
    delivery.requestId = *requestId;
  }

  // The accepted request is the reporting boundary. Submit the selected
  // public MOM interaction outside the registry lock so HLA_IMMEDIATE report
  // delivery cannot re-enter a held service lock before the provider callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestAttributeValueUpdate",
      umbra::detail::MomServiceType::object_management,
      *reportArguments,
      true);

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto &delivery : deliveries) {
    queueAmbassadorAttributeValueUpdateProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        *objectInstanceHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag,
        delivery.requestId);
  }
  } catch (Exception const &exception) {
    emitExceptionReport(L"Request Attribute Value Update", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdate(
    ObjectClassHandle const &objectClass,
    AttributeHandleSet const &attributes,
    VariableLengthData const &userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdate");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns the class-form request slice.  The
  // public adapter validates official handles, then carries only copied
  // values across the private class request/response seam; each expanded
  // provider callback returns through the existing receive fence.
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Request Attribute Value Update requires a defined ObjectClassHandle.");
    }
    auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
    if (!requestedAttributeHandles) {
      throw AttributeNotDefined(
          L"Request Attribute Value Update requires defined AttributeHandle values.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Attribute Value Update requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
    try {
      static_cast<void>(processClient->requestAttributeValueUpdateClass(
          std::move(federationName),
          requestingFederateId,
          *objectClassValue,
          std::vector<std::uint64_t>(requestedAttributeHandles->begin(),
                                     requestedAttributeHandles->end()),
          std::move(processTag)));
      while (processClient->pendingPushedAttributeValueUpdateRequestCount() !=
             0U) {
        processClient->dispatchPushedAttributeValueUpdateRequest();
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, as with the object-instance form.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const requestedObjectClassHandle = objectClassHandleValue(objectClass);
  if (!requestedObjectClassHandle) {
    throw ObjectClassNotDefined(
        L"Request Attribute Value Update requires a defined ObjectClassHandle.");
  }
  auto const requestedAttributeHandles = ambassadorAttributeHandleValues(attributes);
  if (!requestedAttributeHandles) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update requires defined AttributeHandle values.");
  }

  std::vector<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
      momRecipients;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> momReportArguments;
  std::wstring momFederationName;
  std::uint64_t momFederateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    auto &registry = embeddedFederationRegistry();
    auto momPlan = registry.planJoinedFederateMomAttributeValueUpdateClass(
        *joinedFederationName_,
        *joinedFederateId_,
        *requestedObjectClassHandle,
        *requestedAttributeHandles);
    if (momPlan.rtiOwnedMomObject) {
      // Reuse the ordinary class planner's validation boundary for the public
      // class designator and requested attributes, then route only the
      // RTI-owned MOM values discovered by the dedicated plan.
      auto validation = registry.planAttributeValueUpdateClassRequest(
          *joinedFederationName_,
          *joinedFederateId_,
          *requestedObjectClassHandle,
          *requestedAttributeHandles);
      if (validation.status !=
          umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
        throwAttributeValueUpdateClassRequestFailure(validation.status);
      }
      VariableLengthData copiedTag(userSuppliedTag);
      momReportArguments = std::vector<umbra::detail::MomServiceArgument>{
          {umbra::detail::MomArgumentType::object_class_handle,
           L"Object class designator",
           umbra::detail::formatMomObjectClassHandle(objectClass)},
          {umbra::detail::MomArgumentType::attribute_handle_set,
           L"Set of attribute designators",
           umbra::detail::formatMomAttributeHandleSet(attributes)},
          {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
           L"User-supplied tag",
           umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      };
      momFederationName = *joinedFederationName_;
      momFederateId = *joinedFederateId_;
      momRecipients = std::move(momPlan.recipients);
    }
  }
  if (!momRecipients.empty() || !momFederationName.empty()) {
    if (!momReportArguments) {
      throw RTIinternalError(
          L"Umbra could not format the accepted MOM Request Attribute Value Update report.");
    }
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestAttributeValueUpdate",
        umbra::detail::MomServiceType::object_management,
        *momReportArguments,
        true);
    for (auto &recipient : momRecipients) {
      queueAmbassadorJoinedFederateMomAttributeValueUpdate(
          std::move(recipient.callbackRoute),
          momFederationName,
          momFederateId,
          recipient.objectInstanceHandle,
          *requestedAttributeHandles);
    }
    return;
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> requestingFederateId;
  std::optional<std::vector<umbra::detail::MomServiceArgument>> reportArguments;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Request Attribute Value Update");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update.");
    }

    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planAttributeValueUpdateClassRequest(
        *federationName,
        *requestingFederateId,
        *requestedObjectClassHandle,
        *requestedAttributeHandles);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
    return plan;
  };

  // Copy the tag only after the selected class and its attributes have been
  // validated. Replanning immediately before routing prevents a concurrent
  // resign or lifecycle transition from using a stale owner snapshot.
  static_cast<void>(planCurrentRequest());
  VariableLengthData copiedTag(userSuppliedTag);
  // The class overload changes only the first standards-facing argument; the
  // same attribute-set and copied Binary Data tag representation is required.
  reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  auto plan = planCurrentRequest();
  if (!reportArguments) {
    throw RTIinternalError(
        L"Umbra could not format the accepted Request Attribute Value Update report.");
  }
  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
    std::uint64_t requestId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const &recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.objectInstanceHandle,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
        0U,
    });
  }

  // Persist every expanded object/provider delivery before exposing the
  // accepted service report or queuing a callback. The class handle is part
  // of the durable identity used by callback-time hierarchy revalidation.
  for (auto &delivery : deliveries) {
    auto const requestId = embeddedFederationRegistry()
        .registerAttributeValueUpdateClassRequest(
            *federationName,
            *requestingFederateId,
            delivery.providingFederateId,
            delivery.objectInstanceHandle,
            *requestedObjectClassHandle,
            delivery.requestedAttributeHandles,
            umbra::detail::variable_length_data_2025::copyBytes(copiedTag));
    if (!requestId) {
      throw RTIinternalError(
          L"The embedded federation could not persist the object-class Request Attribute Value Update request.");
    }
    delivery.requestId = *requestId;
  }

  // Keep the public report before any later per-instance Provide Attribute
  // Value Update callbacks, without submitting immediate callbacks under the
  // registry lock used to establish the accepted plan.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestAttributeValueUpdate",
      umbra::detail::MomServiceType::object_management,
      *reportArguments,
      true);

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto &delivery : deliveries) {
    queueAmbassadorAttributeValueUpdateClassProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        *federationName,
        *requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag,
        std::nullopt,
        delivery.requestId);
  }
  } catch (Exception const &exception) {
    emitExceptionReport(L"Request Attribute Value Update", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeValueUpdateWithRegions(
    ObjectClassHandle const &objectClass,
    AttributeHandleSetRegionHandleSetPairVector const &attributesAndRegions,
    VariableLengthData const &userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("requestAttributeValueUpdateWithRegions");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns the regional class-form request
  // slice. Validate official handles and normalize the caller's pair vector
  // before carrying the copied map across the private request/response seam.
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Request Attribute Value Update With Regions requires a defined ObjectClassHandle.");
    }
    auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
    if (pairValues.invalidAttributeHandle) {
      throw AttributeNotDefined(
          L"Request Attribute Value Update With Regions requires defined AttributeHandle values.");
    }
    if (pairValues.invalidRegionHandle) {
      throw InvalidRegion(
          L"Request Attribute Value Update With Regions requires defined RegionHandle values.");
    }
    std::set<std::uint64_t> requestedAttributeHandles;
    for (auto const& [attributeHandle, regions] : pairValues.values) {
      static_cast<void>(regions);
      requestedAttributeHandles.insert(attributeHandle);
    }
    if (requestedAttributeHandles.empty()) {
      throw AttributeNotDefined(
          L"Request Attribute Value Update With Regions requires at least one attribute designator.");
    }
    std::wstring federationName;
    std::uint64_t requestingFederateId = 0U;
    umbra::detail::ProcessFederationClient *processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Request Attribute Value Update With Regions requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);
    try {
      auto const result = processClient->requestAttributeValueUpdateClassWithRegions(
          std::move(federationName),
          requestingFederateId,
          *objectClassValue,
          std::vector<std::uint64_t>(requestedAttributeHandles.begin(),
                                     requestedAttributeHandles.end()),
          std::move(pairValues.values),
          std::move(processTag));
      if (result.status !=
          umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
        throwAttributeValueUpdateClassRequestFailure(result.status);
      }
      while (processClient->pendingPushedAttributeValueUpdateRequestCount() !=
             0U) {
        processClient->dispatchPushedAttributeValueUpdateRequest();
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const &error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  try {
  // Preserve the official connection and membership preconditions before
  // caller-supplied handle validation, matching the ordinary class request.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Attribute Value Update With Regions");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update With Regions requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
  }

  auto const requestedObjectClassHandle = objectClassHandleValue(objectClass);
  if (!requestedObjectClassHandle) {
    throw ObjectClassNotDefined(
        L"Request Attribute Value Update With Regions requires a defined ObjectClassHandle.");
  }
  auto pairValues = ambassadorAttributeRegionPairValues(attributesAndRegions);
  if (pairValues.invalidAttributeHandle) {
    throw AttributeNotDefined(
        L"Request Attribute Value Update With Regions requires defined AttributeHandle values.");
  }
  if (pairValues.invalidRegionHandle) {
    throw InvalidRegion(
        L"Request Attribute Value Update With Regions requires defined RegionHandle values.");
  }
  auto const requestedAttributeHandles = [&]() {
    std::set<std::uint64_t> result;
    for (auto const& [attributeHandle, regions] : pairValues.values) {
      static_cast<void>(regions);
      result.insert(attributeHandle);
    }
    return result;
  }();
  // Copy the tag only after handle/pair-shape validation, but before either
  // the MOM or ordinary routing branch records the accepted invocation.
  VariableLengthData copiedTag(userSuppliedTag);

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeValueUpdateClassRequestPlan plan;
  std::vector<umbra::detail::JoinedFederateMomAttributeValueUpdateRecipient>
      momRecipients;
  std::wstring momFederationName;
  std::uint64_t momFederateId = 0;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Value Update With Regions requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeValueUpdateClassRequest(
        federationName,
        requestingFederateId,
        *requestedObjectClassHandle,
        requestedAttributeHandles,
        &pairValues.values);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
    // The class-level regional overload shares the ordinary planner's
    // validation boundary, but RTI-owned HLAfederate objects are not present
    // in its federate-created object ledger.  Consult the dedicated MOM plan
    // after validation and retain the request regions for callback-time
    // re-evaluation.
    auto const momPlan = registry.planJoinedFederateMomAttributeValueUpdateClass(
        federationName,
        requestingFederateId,
        *requestedObjectClassHandle,
        requestedAttributeHandles,
        false,
        &pairValues.values);
    if (momPlan.rtiOwnedMomObject) {
      momFederationName = federationName;
      momFederateId = requestingFederateId;
      momRecipients = momPlan.recipients;
    }
  }

  if (!momRecipients.empty() || !momFederationName.empty()) {
    auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
        {umbra::detail::MomArgumentType::object_class_handle,
         L"Object class designator",
         umbra::detail::formatMomObjectClassHandle(objectClass)},
        {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
         L"Collection of attribute designator set and region designator set pairs",
         umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
        {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
         L"User-supplied tag",
         umbra::detail::formatMomUserSuppliedTag(copiedTag)},
    };
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"RequestAttributeValueUpdateWithRegions",
        umbra::detail::MomServiceType::data_distribution_management,
        reportArguments,
        true);
    for (auto &recipient : momRecipients) {
      queueAmbassadorJoinedFederateMomAttributeValueUpdate(
          std::move(recipient.callbackRoute),
          momFederationName,
          momFederateId,
          recipient.objectInstanceHandle,
          requestedAttributeHandles,
          false,
          std::nullopt,
          pairValues.values);
    }
    return;
  }

  // The regional pair shape is already validated before the first registry
  // plan; the accepted plan above has validated the class, attributes, and
  // region ownership/context. Replanning after this point closes the same
  // membership/ownership race as the ordinary class request.
  // §9.13 is a successful-void DDM service.  Keep the copied tag in the
  // supplied-argument record so caller-owned storage cannot change the audit
  // value after this accepted request boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_set_region_set_pair_list,
       L"Collection of attribute designator set and region designator set pairs",
       umbra::detail::formatMomAttributeSetRegionSetPairList(attributesAndRegions)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
  };
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        *joinedFederationName_ != federationName ||
        *joinedFederateId_ != requestingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Request Attribute Value Update With Regions.");
    }
    auto &registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeValueUpdateClassRequest(
        federationName,
        requestingFederateId,
        *requestedObjectClassHandle,
        requestedAttributeHandles,
        &pairValues.values);
    if (plan.status != umbra::detail::AttributeValueUpdateClassRequestStatus::applied) {
      throwAttributeValueUpdateClassRequestFailure(plan.status);
    }
  }

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    umbra::detail::FederateServiceReportRoute serviceReportRoute;
    std::uint64_t objectInstanceHandle = 0;
    std::uint64_t providingFederateId = 0;
    std::set<std::uint64_t> requestedAttributeHandles;
    std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute;
    std::uint64_t requestId = 0;
  };
  std::vector<Delivery> deliveries;
  deliveries.reserve(plan.recipients.size());
  for (auto const &recipient : plan.recipients) {
    if (!recipient.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has an attribute-update provider without a callback route.");
    }
    deliveries.push_back({
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        recipient.objectInstanceHandle,
        recipient.providingFederateId,
        recipient.requestedAttributeHandles,
        [&]() {
          std::map<std::uint64_t, std::set<std::uint64_t>> result;
          for (auto const& [attributeHandle, regions] : pairValues.values) {
            if (recipient.requestedAttributeHandles.contains(attributeHandle)) {
              result.emplace(attributeHandle, regions);
            }
          }
          return result;
        }(),
        0U,
    });
  }

  // Persist each regional provider delivery before exposing the accepted
  // §9.13 service report or queueing a callback. The request-region map is
  // copied into the route-free ledger and rechecked at callback time.
  for (auto &delivery : deliveries) {
    auto const requestId = embeddedFederationRegistry()
        .registerAttributeValueUpdateRegionalRequest(
            federationName,
            requestingFederateId,
            delivery.providingFederateId,
            delivery.objectInstanceHandle,
            *requestedObjectClassHandle,
            delivery.requestedAttributeHandles,
            delivery.requestRegionsByAttribute,
            umbra::detail::variable_length_data_2025::copyBytes(copiedTag));
    if (!requestId) {
      throw RTIinternalError(
          L"The embedded federation could not persist the regional Request Attribute Value Update request.");
    }
    delivery.requestId = *requestId;
  }

  // The requester-side §9.13 record precedes any separately queued Provide
  // Attribute Value Update callbacks. Submit it after persistence and without
  // holding the registry lock so an immediate report recipient cannot re-enter
  // the service lock.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestAttributeValueUpdateWithRegions",
      umbra::detail::MomServiceType::data_distribution_management,
      reportArguments,
      true);

  // Do not hold the requester lock while submitting a route: HLA_IMMEDIATE
  // may synchronously enter the owner's Provide Attribute Value Update callback.
  for (auto &delivery : deliveries) {
    queueAmbassadorAttributeValueUpdateClassProvide(
        std::move(delivery.callbackRoute),
        std::move(delivery.serviceReportRoute),
        federationName,
        requestingFederateId,
        delivery.providingFederateId,
        delivery.objectInstanceHandle,
        *requestedObjectClassHandle,
        std::move(delivery.requestedAttributeHandles),
        copiedTag,
        std::move(delivery.requestRegionsByAttribute),
        delivery.requestId);
  }
  } catch (Exception const &exception) {
    emitExceptionReport(L"Request Attribute Value Update With Regions", exception);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
