#include "internal/runtime/umbra_rti_ambassador.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
#include "internal/federation/process_federation_client.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/federate_handle.hpp"
#include "internal/handles/interaction_class_handle.hpp"
#include "internal/handles/object_class_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

namespace {

std::optional<OrderType> standardOrderTypeValue(std::wstring const& orderTypeName) {
  if (orderTypeName == L"Receive") {
    return RECEIVE;
  }
  if (orderTypeName == L"TimeStamp") {
    return TIMESTAMP;
  }
  return std::nullopt;
}

std::optional<std::wstring> standardOrderTypeName(OrderType orderType) {
  switch (orderType) {
    case RECEIVE:
      return L"Receive";
    case TIMESTAMP:
      return L"TimeStamp";
  }
  return std::nullopt;
}

}  // namespace

void UmbraRtiAmbassador::changeAttributeOrderType(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeAttributeOrderType");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Change Attribute Order Type requires a known ObjectInstanceHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Change Attribute Order Type requires defined AttributeHandle values.");
    }
    if (!standardOrderTypeName(orderType)) {
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
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
            L"Change Attribute Order Type requires membership in a federation execution.");
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
      auto const result = processClient->changeAttributeOrderType(
          std::move(federationName),
          federateId,
          *objectInstanceValue,
          *attributeValues,
          orderType);
      if (result.status !=
          umbra::detail::AttributeOrderTypeChangeStatus::applied) {
        throwAttributeOrderTypeChangeFailure(result.status);
      }
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
    requireFederationServiceOperationAvailable(L"Change Attribute Order Type");
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Change Attribute Order Type requires a known ObjectInstanceHandle.");
  }
  auto const attributeValues = ambassadorAttributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Attribute Order Type requires defined AttributeHandle values.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.24 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "Order type". Table 5 fixes the
  // corresponding MIM types and value forms: the instance handle and order
  // type use their quoted forms, while AttributeHandleSet is an
  // Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Attribute Order Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Change Attribute Order Type requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    auto const status = registry.changeAttributeOrderType(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceValue,
        *attributeValues,
        orderType);
    if (status != umbra::detail::AttributeOrderTypeChangeStatus::applied) {
      throwAttributeOrderTypeChangeFailure(status);
    }
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeAttributeOrderType",
      umbra::detail::MomServiceType::declaration_management,
      reportArguments,
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Attribute Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeDefaultAttributeOrderType(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeDefaultAttributeOrderType");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Change Default Attribute Order Type requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Change Default Attribute Order Type requires defined AttributeHandle values.");
    }
    if (!standardOrderTypeName(orderType)) {
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
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
            L"Change Default Attribute Order Type requires membership in a federation execution.");
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
      auto const result = processClient->changeDefaultAttributeOrderType(
          std::move(federationName),
          federateId,
          *objectClassValue,
          *attributeValues,
          orderType);
      if (result.status !=
          umbra::detail::AttributeOrderTypeDefaultStatus::applied) {
        throwAttributeOrderTypeDefaultFailure(result.status);
      }
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
    requireFederationServiceOperationAvailable(L"Change Default Attribute Order Type");
  }
  auto const objectClassValue = objectClassHandleValue(objectClass);
  if (!objectClassValue) {
    throw ObjectClassNotDefined(
        L"Change Default Attribute Order Type requires a defined ObjectClassHandle.");
  }
  auto const attributeValues = ambassadorAttributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Default Attribute Order Type requires defined AttributeHandle values.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.25 names these supplied values "Object class designator",
  // "Set of attribute designators", and "Order type". Table 5 fixes the
  // corresponding MIM types and value forms: the class handle and order type
  // use their quoted forms, while AttributeHandleSet is an
  // Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };

  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Default Attribute Order Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Change Default Attribute Order Type requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    auto const status = registry.changeDefaultAttributeOrderType(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassValue,
        *attributeValues,
        orderType);
    if (status != umbra::detail::AttributeOrderTypeDefaultStatus::applied) {
      throwAttributeOrderTypeDefaultFailure(status);
    }
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeDefaultAttributeOrderType",
      umbra::detail::MomServiceType::declaration_management,
      reportArguments,
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Default Attribute Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeInteractionOrderType(
    InteractionClassHandle const& interactionClass,
    OrderType orderType) {
  auto instrumentationScope = beginRtiCall("changeInteractionOrderType");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const handle = interactionClassHandleValue(interactionClass);
    if (!handle) {
      throw InteractionClassNotDefined(
          L"Change Interaction Order Type requires a defined InteractionClassHandle.");
    }
    if (!standardOrderTypeName(orderType)) {
      throw RTIinternalError(
          L"The supplied OrderType is not supported by the embedded 2025 profile.");
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
            L"Change Interaction Order Type requires membership in a federation execution.");
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
      auto const result = processClient->changeInteractionOrderType(
          std::move(federationName), federateId, *handle, orderType);
      if (result.status !=
          umbra::detail::InteractionOrderTypeChangeStatus::applied) {
        throwInteractionOrderTypeChangeFailure(result.status);
      }
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
    requireFederationServiceOperationAvailable(L"Change Interaction Order Type");
  }
  auto const interactionClassValue = interactionClassHandleValue(interactionClass);
  if (!interactionClassValue) {
    throw InteractionClassNotDefined(
        L"Change Interaction Order Type requires a defined InteractionClassHandle.");
  }
  if (!standardOrderTypeName(orderType)) {
    throw RTIinternalError(
        L"The supplied OrderType is not supported by the embedded 2025 profile.");
  }
  // Section 8.26 names these supplied values "Interaction class designator"
  // and "Order type". Table 5 leaves HLAargumentName implementation-defined,
  // while fixing their type/value forms: type 27 String(handle.toString()) and
  // type 38 quoted RECEIVE/TIMESTAMP respectively.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::interaction_class_handle,
       L"Interaction class designator",
       umbra::detail::formatMomInteractionClassHandle(interactionClass)},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)},
  };

  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Change Interaction Order Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Change Interaction Order Type requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    auto const status = registry.changeInteractionOrderType(
        *joinedFederationName_,
        *joinedFederateId_,
        *interactionClassValue,
        orderType);
    if (status != umbra::detail::InteractionOrderTypeChangeStatus::applied) {
      throwInteractionOrderTypeChangeFailure(status);
    }
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeInteractionOrderType",
      umbra::detail::MomServiceType::declaration_management,
      reportArguments,
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Interaction Order Type", exception);
    throw;
  }
}

void UmbraRtiAmbassador::requestAttributeTransportationTypeChange(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleSet const& attributes,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("requestAttributeTransportationTypeChange");
  try {
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Request Attribute Transportation Type Change requires a known ObjectInstanceHandle.");
  }
  auto const attributeValues = ambassadorAttributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Request Attribute Transportation Type Change requires defined AttributeHandle values.");
  }
  if (!transportationTypeHandleValue(transportationType)) {
    throw InvalidTransportationTypeHandle(
        L"Request Attribute Transportation Type Change requires a supported TransportationTypeHandle.");
  }
  // Section 6.25 names these supplied values "Object instance designator",
  // "Set of attribute designators", and "Transportation type". Table 5
  // fixes the corresponding MIM types and value forms: the two handles use
  // quoted handle.toString(), while AttributeHandleSet is Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

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
            L"Request Attribute Transportation Type Change requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->requestAttributeTransportationTypeChange(
          std::move(federationName),
          requestingFederateId,
          *objectInstanceValue,
          *attributeValues,
          *transportationTypeHandleValue(transportationType));
      if (result.status !=
          umbra::detail::AttributeTransportationTypeChangeStatus::applied) {
        throwAttributeTransportationTypeChangeFailure(result.status);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeTransportationTypeChangePlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Request Attribute Transportation Type Change");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Request Attribute Transportation Type Change requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const transportationName = ambassadorTransportationNameFromFederation(
        federationName,
        transportationType);
    if (!transportationName) {
      throw InvalidTransportationTypeHandle(
          L"Request Attribute Transportation Type Change requires a transportation type declared in the federation execution.");
    }
    plan = registry.planAttributeTransportationTypeChange(
        federationName,
        requestingFederateId,
        *objectInstanceValue,
        *attributeValues,
        *transportationName);
    if (plan.status !=
        umbra::detail::AttributeTransportationTypeChangeStatus::applied) {
      throwAttributeTransportationTypeChangeFailure(plan.status);
    }
  }
  // The request succeeds once the registry accepts the pending change.
  // Section 6.25 separately makes the responding confirmation callback the
  // boundary at which the selected transportation takes effect. Emit the
  // public report after releasing the registry lock.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"RequestAttributeTransportationTypeChange",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  if (plan.requestId != 0) {
    if (!plan.callbackRoute) {
      throw RTIinternalError(
          L"The embedded federation has no attribute transportation confirmation callback route.");
    }
    queueAmbassadorConfirmAttributeTransportationTypeChange(
        std::move(plan.callbackRoute),
        std::move(federationName),
        requestingFederateId,
        plan.requestId);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Request Attribute Transportation Type Change", exception);
    throw;
  }
}

void UmbraRtiAmbassador::changeDefaultAttributeTransportationType(
    ObjectClassHandle const& objectClass,
    AttributeHandleSet const& attributes,
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("changeDefaultAttributeTransportationType");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectClassValue = objectClassHandleValue(objectClass);
    if (!objectClassValue) {
      throw ObjectClassNotDefined(
          L"Change Default Attribute Transportation Type requires a defined ObjectClassHandle.");
    }
    auto const attributeValues = ambassadorAttributeHandleValues(attributes);
    if (!attributeValues) {
      throw AttributeNotDefined(
          L"Change Default Attribute Transportation Type requires defined AttributeHandle values.");
    }
    auto const transportationTypeValue =
        transportationTypeHandleValue(transportationType);
    if (!transportationTypeValue) {
      throw InvalidTransportationTypeHandle(
          L"Change Default Attribute Transportation Type requires a supported TransportationTypeHandle.");
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
            L"Change Default Attribute Transportation Type requires membership in a federation execution.");
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
      auto const result = processClient->changeDefaultAttributeTransportationType(
          std::move(federationName),
          federateId,
          *objectClassValue,
          *attributeValues,
          *transportationTypeValue);
      if (result.status !=
          umbra::detail::AttributeTransportationTypeDefaultStatus::applied) {
        throwAttributeTransportationTypeDefaultFailure(result.status);
      }
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
        L"Change Default Attribute Transportation Type");
  }
  auto const objectClassValue = objectClassHandleValue(objectClass);
  if (!objectClassValue) {
    throw ObjectClassNotDefined(
        L"Change Default Attribute Transportation Type requires a defined ObjectClassHandle.");
  }
  auto const attributeValues = ambassadorAttributeHandleValues(attributes);
  if (!attributeValues) {
    throw AttributeNotDefined(
        L"Change Default Attribute Transportation Type requires defined AttributeHandle values.");
  }
  if (!transportationTypeHandleValue(transportationType)) {
    throw InvalidTransportationTypeHandle(
        L"Change Default Attribute Transportation Type requires a supported TransportationTypeHandle.");
  }
  // Section 6.27 names these supplied values "Object class designator",
  // "Set of attribute designators", and "Transportation type". Table 5
  // fixes the corresponding MIM types and value forms: the class and
  // transportation handles use their quoted forms, while AttributeHandleSet
  // is an Array<AttributeHandle>.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_class_handle,
       L"Object class designator",
       umbra::detail::formatMomObjectClassHandle(objectClass)},
      {umbra::detail::MomArgumentType::attribute_handle_set,
       L"Set of attribute designators",
       umbra::detail::formatMomAttributeHandleSet(attributes)},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)},
  };

  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(
        L"Change Default Attribute Transportation Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Change Default Attribute Transportation Type requires membership in a federation execution.");
    }
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const transportationName = ambassadorTransportationNameFromFederation(
        *joinedFederationName_,
        transportationType);
    if (!transportationName) {
      throw InvalidTransportationTypeHandle(
          L"Change Default Attribute Transportation Type requires a transportation type declared in the federation execution.");
    }
    auto const status = registry.changeDefaultAttributeTransportationType(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectClassValue,
        *attributeValues,
        *transportationName);
    if (status != umbra::detail::AttributeTransportationTypeDefaultStatus::applied) {
      throwAttributeTransportationTypeDefaultFailure(status);
    }
  }
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"ChangeDefaultAttributeTransportationType",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Change Default Attribute Transportation Type", exception);
    throw;
  }
}
void UmbraRtiAmbassador::queryAttributeTransportationType(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandle const& attribute) {
  auto instrumentationScope = beginRtiCall("queryAttributeTransportationType");
  try {
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
  }
  auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceValue) {
    throw ObjectInstanceNotKnown(
        L"Query Attribute Transportation Type requires a known ObjectInstanceHandle.");
  }
  auto const attributeValue = attributeHandleValue(attribute);
  if (!attributeValue) {
    throw AttributeNotDefined(
        L"Query Attribute Transportation Type requires a defined AttributeHandle.");
  }
  // Section 6.28 names these supplied values "Object instance designator"
  // and "Attribute designator". Table 5 fixes their respective type-37 and
  // type-0 value forms as quoted handle.toString() text.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle,
       L"Attribute designator",
       umbra::detail::formatMomAttributeHandle(attribute)},
  };

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
            L"Query Attribute Transportation Type requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (!processClient) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      requestingFederateId = *joinedFederateId_;
    }
    try {
      auto const result = processClient->queryAttributeTransportationType(
          std::move(federationName),
          requestingFederateId,
          *objectInstanceValue,
          *attributeValue);
      if (result.status !=
          umbra::detail::AttributeTransportationTypeQueryStatus::applied) {
        throwAttributeTransportationTypeQueryFailure(result.status);
      }
      processClient->dispatchPendingPushedEvents();
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif

  std::wstring federationName;
  std::uint64_t requestingFederateId = 0;
  umbra::detail::AttributeTransportationTypeQueryPlan plan;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Query Attribute Transportation Type");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Query Attribute Transportation Type requires membership in a federation execution.");
    }
    federationName = *joinedFederationName_;
    requestingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, requestingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    plan = registry.planAttributeTransportationTypeQuery(
        federationName,
        requestingFederateId,
        *objectInstanceValue,
        *attributeValue);
    if (plan.status != umbra::detail::AttributeTransportationTypeQueryStatus::applied) {
      throwAttributeTransportationTypeQueryFailure(plan.status);
    }
  }
  // The query is successfully invoked once the plan is accepted. Its
  // separate Report Attribute Transportation Type callback is not the
  // service-reporting boundary. Emit the public MOM interaction after
  // releasing the registry lock so HLA_IMMEDIATE callbacks can re-enter.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"QueryAttributeTransportationType",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);
  if (!plan.callbackRoute) {
    throw RTIinternalError(
        L"The embedded federation has no attribute transportation report callback route.");
  }
  queueAmbassadorReportAttributeTransportationType(
      std::move(plan.callbackRoute),
      std::move(federationName),
      requestingFederateId,
      *objectInstanceValue,
      *attributeValue);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Query Attribute Transportation Type", exception);
    throw;
  }
}

TransportationTypeHandle UmbraRtiAmbassador::getTransportationTypeHandle(
    std::wstring const& transportationTypeName) {
  auto instrumentationScope = beginRtiCall("getTransportationTypeHandle");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const encodedName = umbra::detail::utf8FromWide(transportationTypeName);
    if (!encodedName) {
      throw InvalidTransportationName(
          L"The supplied transportation type name is not valid UTF-8 text.");
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
            L"Get Transportation Type Handle requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    std::optional<std::uint64_t> value;
    try {
      value = processClient->lookupTransportationTypeHandle(
          federationName, federateId, *encodedName);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!value) {
      throw InvalidTransportationName(
          L"The supplied transportation type name is not declared in this federation execution.");
    }
    return makeTransportationTypeHandle(*value);
  }
#endif
  TransportationTypeHandle transportationType;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Transportation Type Handle requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const encodedName = umbra::detail::utf8FromWide(transportationTypeName);
    auto const value = encodedName
        ? registry.transportationTypeHandleFor(*joinedFederationName_, *encodedName)
        : std::nullopt;
    if (!value) {
      throw InvalidTransportationName(
          L"The supplied transportation type name is not declared in this federation execution.");
    }
    transportationType = makeTransportationTypeHandle(*value);
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetTransportationTypeHandle",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Transportation type name",
        umbra::detail::formatMomString(transportationTypeName)}},
      {umbra::detail::MomArgumentType::transportation_type_handle,
       L"Transportation type handle",
       umbra::detail::formatMomTransportationTypeHandle(transportationType)});
  return transportationType;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Transportation Type Handle", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetTransportationTypeHandle",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Transportation type name",
          umbra::detail::formatMomString(transportationTypeName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getTransportationTypeName(
    TransportationTypeHandle const& transportationType) {
  auto instrumentationScope = beginRtiCall("getTransportationTypeName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const transportationTypeValue =
        transportationTypeHandleValue(transportationType);
    if (!transportationTypeValue) {
      throw InvalidTransportationTypeHandle(
          L"Get Transportation Type Name requires a valid TransportationTypeHandle.");
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
            L"Get Transportation Type Name requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    std::optional<std::wstring> transportationTypeName;
    try {
      transportationTypeName = processClient->lookupTransportationTypeName(
          federationName, federateId, *transportationTypeValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (!transportationTypeName) {
      throw InvalidTransportationTypeHandle(
          L"The supplied TransportationTypeHandle is not declared in this federation execution.");
    }
    return *transportationTypeName;
  }
#endif
  std::wstring transportationTypeName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Transportation Type Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const value = transportationTypeHandleValue(transportationType);
    auto const name = value
        ? registry.transportationTypeNameFor(*joinedFederationName_, *value)
        : std::nullopt;
    if (!name) {
      throw InvalidTransportationTypeHandle(
          L"The supplied TransportationTypeHandle is not declared in this federation execution.");
    }
    auto const wideName = umbra::detail::wideFromUtf8(*name);
    if (!wideName) {
      throw RTIinternalError(
          L"The embedded federation returned an invalid transportation type name.");
    }
    transportationTypeName = *wideName;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetTransportationTypeName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::transportation_type_handle,
        L"Transportation type handle",
        umbra::detail::formatMomTransportationTypeHandle(transportationType)}},
      {umbra::detail::MomArgumentType::string,
       L"Transportation type name",
       umbra::detail::formatMomString(transportationTypeName)});
  return transportationTypeName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Transportation Type Name", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetTransportationTypeName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::transportation_type_handle,
          L"Transportation type handle",
          umbra::detail::formatMomTransportationTypeHandle(transportationType)}},
        describeAmbassadorException(exception));
    throw;
  }
}

OrderType UmbraRtiAmbassador::getOrderType(std::wstring const& orderTypeName) {
  auto instrumentationScope = beginRtiCall("getOrderType");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // Order names are the fixed IEEE 1516.1 pair and do not require a FOM
  // lookup.  A process-boundary ambassador has no local federation registry,
  // so enforce only the public connection/membership guards before applying
  // the same standard name mapping used by the embedded profile.
  if (processEndpointActive_) {
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined) {
        throw FederateNotExecutionMember(
            L"Get Order Type requires membership in a federation execution.");
      }
    }
    auto const value = standardOrderTypeValue(orderTypeName);
    if (!value) {
      throw InvalidOrderName(
          L"The embedded profile supports only the Receive and TimeStamp order names.");
    }
    appendSuccessfulServiceReportToFileIfSelected(
        L"GetOrderType",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Order name",
          umbra::detail::formatMomString(orderTypeName)}},
        {umbra::detail::MomArgumentType::order_type,
         L"Order type",
         umbra::detail::formatMomOrderType(*value)});
    return *value;
  }
#endif
  OrderType orderType;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Order Type requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const value = standardOrderTypeValue(orderTypeName);
    if (!value) {
      throw InvalidOrderName(
          L"The embedded profile supports only the Receive and TimeStamp order names.");
    }
    orderType = *value;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetOrderType",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::string,
        L"Order name",
        umbra::detail::formatMomString(orderTypeName)}},
      {umbra::detail::MomArgumentType::order_type,
       L"Order type",
       umbra::detail::formatMomOrderType(orderType)});
  return orderType;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Order Type", exception);
    appendFailedServiceReportToFileIfSelected(
        L"GetOrderType",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::string,
          L"Order name",
          umbra::detail::formatMomString(orderTypeName)}},
        describeAmbassadorException(exception));
    throw;
  }
}

std::wstring UmbraRtiAmbassador::getOrderName(OrderType orderType) {
  auto instrumentationScope = beginRtiCall("getOrderName");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // See getOrderType: order-type names are static support-service data, while
  // the process profile deliberately does not install a local registry.
  if (processEndpointActive_) {
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined) {
        throw FederateNotExecutionMember(
            L"Get Order Name requires membership in a federation execution.");
      }
    }
    auto const name = standardOrderTypeName(orderType);
    if (!name) {
      throw InvalidOrderType(
          L"The supplied OrderType is not supported by this embedded profile.");
    }
    appendSuccessfulServiceReportToFileIfSelected(
        L"GetOrderName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::order_type,
          L"Order type",
          umbra::detail::formatMomOrderType(orderType)}},
        {umbra::detail::MomArgumentType::string,
         L"Order name",
         umbra::detail::formatMomString(*name)});
    return *name;
  }
#endif
  std::wstring orderTypeName;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Get Order Name requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const name = standardOrderTypeName(orderType);
    if (!name) {
      throw InvalidOrderType(
          L"The supplied OrderType is not supported by this embedded profile.");
    }
    orderTypeName = *name;
  }

  appendSuccessfulServiceReportToFileIfSelected(
      L"GetOrderName",
      umbra::detail::MomServiceType::support_services,
      {{umbra::detail::MomArgumentType::order_type,
        L"Order type",
        umbra::detail::formatMomOrderType(orderType)}},
      {umbra::detail::MomArgumentType::string,
       L"Order name",
       umbra::detail::formatMomString(orderTypeName)});
  return orderTypeName;
  } catch (Exception const& exception) {
    emitExceptionReport(L"Get Order Name", exception);
    std::wstring encodedOrderType;
    try {
      encodedOrderType = umbra::detail::formatMomOrderType(orderType);
    } catch (...) {
      // Table 5 has no canonical spelling for an invalid enum supplied to
      // GetOrderName. Keep the failed report's type-38 slot deterministic
      // without allowing diagnostic encoding to replace the public exception.
      encodedOrderType = umbra::detail::formatMomString(L"UNSUPPORTED");
    }
    appendFailedServiceReportToFileIfSelected(
        L"GetOrderName",
        umbra::detail::MomServiceType::support_services,
        {{umbra::detail::MomArgumentType::order_type,
          L"Order type",
          encodedOrderType}},
        describeAmbassadorException(exception));
    throw;
  }
}


}  // namespace rti1516_2025::umbra_binding_detail
#endif
