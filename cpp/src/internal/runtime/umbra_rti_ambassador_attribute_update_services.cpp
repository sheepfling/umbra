#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/handles/attribute_handle.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;
using AttributeValue = AmbassadorAttributeValue;

#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
namespace {

std::optional<std::vector<std::uint64_t>> attributeValueHandleValues(
    AttributeHandleValueMap const& attributeValues) {
  std::vector<std::uint64_t> result;
  result.reserve(attributeValues.size());
  for (auto const& [attributeHandle, attributeValue] : attributeValues) {
    static_cast<void>(attributeValue);
    auto const value = attributeHandleValue(attributeHandle);
    if (!value) {
      return std::nullopt;
    }
    result.push_back(*value);
  }
  return result;
}

std::vector<AttributeValue> copyAttributeValues(
    AttributeHandleValueMap const& attributeValues) {
  std::vector<AttributeValue> result;
  result.reserve(attributeValues.size());
  for (auto const& [attributeHandle, attributeValue] : attributeValues) {
    auto const value = attributeHandleValue(attributeHandle);
    if (!value) {
      throw RTIinternalError(
          L"The Update Attribute Values map changed while Umbra was copying its values.");
    }
    result.emplace_back(*value, attributeValue);
  }
  return result;
}

}  // namespace
#endif

void UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("updateAttributeValues");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns this ordinary object-management
  // slice. Keep the official handle/value validation on the public adapter,
  // then copy the caller-owned buffers into the private process envelope.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Update Attribute Values requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Update Attribute Values requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = attributeValueHandleValues(attributeValues);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Update Attribute Values requires defined AttributeHandle values.");
    }

    auto const sentAttributes = copyAttributeValues(attributeValues);
    std::vector<umbra::detail::ProcessFederationAttributeValue>
        processAttributeValues;
    processAttributeValues.reserve(sentAttributes.size());
    for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
      auto bytes = umbra::detail::variable_length_data_2025::copyBytes(attributeValue);
      processAttributeValues.emplace_back(attributeHandle, std::move(bytes));
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);

    try {
      static_cast<void>(processClient->updateAttributeValues(
          std::move(federationName),
          producingFederateId,
          *objectInstanceValue,
          std::move(processAttributeValues),
          std::move(processTag)));
      while (processClient->pendingPushedAttributeUpdateCount() != 0U) {
        processClient->dispatchPushedAttributeUpdate();
      }
      while (processClient->pendingPushedEventCount() != 0U) {
        processClient->dispatchPushedReceiveOrder();
      }
      while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
        processClient->dispatchPushedObjectInstanceDiscovery();
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  try {
  // Preserve the 2025 service's connection and membership preconditions ahead
  // of caller-supplied handle validation, matching the other public services.
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Update Attribute Values requires membership in a federation execution.");
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
        L"Update Attribute Values requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeValueHandleValues(attributeValues);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Update Attribute Values requires defined AttributeHandle values.");
  }

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Update Attribute Values requires membership in a federation execution.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Update Attribute Values.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderAttributeUpdate(
        *federationName,
        *producingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::ReceiveOrderAttributeUpdateStatus::applied) {
      throwReceiveOrderAttributeUpdateFailure(plan.status);
    }
    return plan;
  };

  // Validate membership, object knowledge, source ownership, attribute
  // definitions, and FOM transportation before copying caller-owned buffers.
  // The durable copies below are made without a runtime lock, then the plan is
  // rechecked before recipients are selected for delivery.
  static_cast<void>(planCurrentRequest());
  std::vector<AttributeValue> sentAttributes = copyAttributeValues(attributeValues);
  VariableLengthData copiedTag(userSuppliedTag);
  AttributeHandleValueMap reportAttributeValues;
  for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
    reportAttributeValues.emplace(makeAttributeHandle(attributeHandle), attributeValue);
  }
  // Section 6.10.1 names four supplied values. The receive-order overload
  // leaves the optional timestamp unused, and Section 11.5.1 therefore
  // requires its corresponding supplied-argument element to be Null. The
  // accepted value map is rebuilt from durable copies before formatting so
  // caller-owned buffers cannot alter the report boundary.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_value_map,
       L"Constrained set of attribute designator and value pairs",
       umbra::detail::formatMomAttributeHandleValueMap(reportAttributeValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::null_value,
       L"Optional timestamp",
       umbra::detail::formatMomNull()},
  };
  auto plan = planCurrentRequest();

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::uint64_t recipientId = 0;
    std::vector<std::uint64_t> sentAttributeHandles;
    TransportationTypeHandle transportationType;
    std::string transportationName;
    bool reliableTransportation = false;
    std::optional<std::set<std::uint64_t>> sentRegionHandles;
    bool defaultRegionUsed = false;
    std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>
        sentRegionSnapshots;
  };
  std::vector<Delivery> deliveries;
  for (auto const& passel : plan.passels) {
    auto const transportationName = umbra::detail::wideFromUtf8(passel.transportationName);
    if (!transportationName) {
      throw RTIinternalError(
          L"The composed FOM contains an invalid UTF-8 transportation type name.");
    }
    auto const transportationValue = ambassadorTransportationTypeValueForFederation(
        *federationName,
        *transportationName);
    if (!transportationValue) {
      throw RTIinternalError(
          L"The composed FOM transportation type is not declared in this federation execution.");
    }
    auto const reliableTransportation = ambassadorTransportationTypeReliableForFederation(
        *federationName,
        passel.transportationName);
    if (!reliableTransportation) {
      throw RTIinternalError(
          L"The composed FOM transportation type has no delivery policy in this federation execution.");
    }
    TransportationTypeHandle transportationType = makeTransportationTypeHandle(*transportationValue);
    for (auto const& recipient : passel.recipients) {
      if (!recipient.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an eligible attribute-update recipient without a callback route.");
      }
      deliveries.push_back({
          recipient.callbackRoute,
          recipient.federateId,
          passel.sentAttributeHandles,
          transportationType,
          passel.transportationName,
          *reliableTransportation,
          passel.sentRegionHandles.empty()
              ? std::nullopt
              : std::optional<std::set<std::uint64_t>>(passel.sentRegionHandles),
          passel.defaultRegionUsed,
          passel.sentRegionSnapshots.empty()
              ? std::nullopt
              : std::optional<std::map<std::uint64_t, umbra::detail::RegionSpecificationSnapshot>>(
                    passel.sentRegionSnapshots),
      });
    }
  }

  // Every synchronous pre-callback delivery check has now succeeded. Section
  // 6.10's accepted update is the report boundary, and the §11.5 file record
  // must precede any induced Reflect Attribute Values callback.
  appendSuccessfulVoidServiceReportToFileIfSelected(
      L"UpdateAttributeValues",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      true);

  // The accepted Update Attribute Values invocation is the MOM counter's
  // source boundary. Record it after the service-report write has succeeded
  // and before any induced callback is exposed.
  std::set<std::string> acceptedTransportationNames;
  for (auto const& passel : plan.passels) {
    acceptedTransportationNames.insert(passel.transportationName);
  }
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulUpdateAttributeValues(
            *federationName,
            *producingFederateId,
            *objectInstanceHandle,
            plan.registeredObjectClassHandle,
            acceptedTransportationNames,
            &sentAttributes);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the Update Attribute Values membership before its accepted boundary.");
    }
  }

  // Do not hold either sender lock while submitting a route: HLA_IMMEDIATE may
  // synchronously enter a different federate's Reflect Attribute Values callback.
  for (auto& delivery : deliveries) {
    queueAmbassadorReceiveOrderAttributeUpdate(
        std::move(delivery.callbackRoute),
        *federationName,
        *producingFederateId,
        delivery.recipientId,
        *objectInstanceHandle,
        std::move(delivery.sentAttributeHandles),
        sentAttributes,
        copiedTag,
        delivery.transportationType,
        delivery.transportationName,
        delivery.reliableTransportation,
        std::move(delivery.sentRegionHandles),
        delivery.defaultRegionUsed,
        std::move(delivery.sentRegionSnapshots));
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Update Attribute Values", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UpdateAttributeValues",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_handle_value_map,
          L"Constrained set of attribute designator and value pairs",
          umbra::detail::formatMomAttributeHandleValueMap(attributeValues)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}
MessageRetractionHandle UmbraRtiAmbassador::updateAttributeValues(
    ObjectInstanceHandle const& objectInstance,
    AttributeHandleValueMap const& attributeValues,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("updateAttributeValues");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The process endpoint carries the same execution-owned timestamped
  // message identity used by the embedded scheduler. A zero identity retains
  // the legacy timestamp-preservation probe; a nonzero identity is surfaced
  // as the official MessageRetractionHandle.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t producingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Timestamped Update Attribute Values requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Timestamped Update Attribute Values requires a known ObjectInstanceHandle.");
    }
    auto const attributeHandles = attributeValueHandleValues(attributeValues);
    if (!attributeHandles) {
      throw AttributeNotDefined(
          L"Timestamped Update Attribute Values requires defined AttributeHandle values.");
    }

    auto timestamp = cloneAmbassadorReferenceLogicalTime(time.implementationName(), time);
    if (timestamp->isInitial() || timestamp->isFinal()) {
      throw InvalidLogicalTime(
          L"A timestamped service requires a finite logical timestamp.");
    }
    auto const encodedTimestamp = timestamp->encode();
    auto processTimestampBytes =
        umbra::detail::variable_length_data_2025::copyBytes(encodedTimestamp);

    auto const sentAttributes = copyAttributeValues(attributeValues);
    std::vector<umbra::detail::ProcessFederationAttributeValue>
        processAttributeValues;
    processAttributeValues.reserve(sentAttributes.size());
    for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
      auto bytes = umbra::detail::variable_length_data_2025::copyBytes(attributeValue);
      processAttributeValues.emplace_back(attributeHandle, std::move(bytes));
    }
    auto processTag =
        umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag);

    umbra::detail::ProcessFederationUpdateAttributeValuesResult result;
    try {
      result = processClient->updateAttributeValues(
          std::move(federationName),
          producingFederateId,
          *objectInstanceValue,
          std::move(processAttributeValues),
          std::move(processTag),
          umbra::detail::ProcessFederationLogicalTime{
              timestamp->implementationName(), std::move(processTimestampBytes)});
      while (processClient->pendingPushedAttributeUpdateCount() != 0U) {
        processClient->dispatchPushedAttributeUpdate();
      }
      while (processClient->pendingPushedEventCount() != 0U) {
        processClient->dispatchPushedReceiveOrder();
      }
      while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
        processClient->dispatchPushedObjectInstanceDiscovery();
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationCallbackBridgeError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }

    if (result.messageId == 0U) {
      return MessageRetractionHandle();
    }
    return makeMessageRetractionHandle(result.messageId);
  }
#endif
  try {
  std::shared_ptr<umbra::detail::FederateTimeState> timeState;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Update Attribute Values requires membership in a federation execution.");
    }
    if (!embeddedFederationRegistry().memberById(
            *joinedFederationName_,
            *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    timeState = federateTimeState_;
  }

  auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
  if (!objectInstanceHandle) {
    throw ObjectInstanceNotKnown(
        L"Timestamped Update Attribute Values requires a known ObjectInstanceHandle.");
  }
  auto const attributeHandles = attributeValueHandleValues(attributeValues);
  if (!attributeHandles) {
    throw AttributeNotDefined(
        L"Timestamped Update Attribute Values requires defined AttributeHandle values.");
  }

  auto timestamp = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateAmbassadorTsoTimestamp(timeSnapshot, *timestamp);

  std::optional<std::wstring> federationName;
  std::optional<std::uint64_t> producingFederateId;
  auto planCurrentRequest = [&]() {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Timestamped Update Attribute Values");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Update Attribute Values requires an active joined federate.");
    }
    if (!federationName) {
      federationName = *joinedFederationName_;
      producingFederateId = *joinedFederateId_;
    } else if (*joinedFederationName_ != *federationName ||
               *joinedFederateId_ != *producingFederateId) {
      throw FederateNotExecutionMember(
          L"The RTI ambassador's federation membership changed during Timestamped Update Attribute Values.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*federationName, *producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto plan = registry.planReceiveOrderAttributeUpdate(
        *federationName,
        *producingFederateId,
        *objectInstanceHandle,
        *attributeHandles);
    if (plan.status != umbra::detail::ReceiveOrderAttributeUpdateStatus::applied) {
      throwReceiveOrderAttributeUpdateFailure(plan.status);
    }
    return plan;
  };

  // Validate the object, ownership, attribute definitions, and transport
  // passels before copying caller-owned buffers, then re-plan immediately
  // before accepting the timestamped message.
  static_cast<void>(planCurrentRequest());
  std::vector<AttributeValue> sentAttributes = copyAttributeValues(attributeValues);
  VariableLengthData copiedTag(userSuppliedTag);
  auto plan = planCurrentRequest();
  AttributeHandleValueMap reportAttributeValues;
  for (auto const& [attributeHandle, attributeValue] : sentAttributes) {
    reportAttributeValues.emplace(makeAttributeHandle(attributeHandle), attributeValue);
  }
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::attribute_handle_value_map,
       L"Constrained set of attribute designator and value pairs",
       umbra::detail::formatMomAttributeHandleValueMap(reportAttributeValues)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::logical_time,
       L"Optional timestamp",
       umbra::detail::formatMomLogicalTime(*timestamp)},
  };
  bool const hasTimestampOrderedAttribute = std::any_of(
      plan.passels.begin(),
      plan.passels.end(),
      [](umbra::detail::ReceiveOrderAttributeUpdatePassel const& passel) {
        return passel.preferredOrderType == TIMESTAMP;
      });

  struct Delivery {
    umbra::detail::ObjectInstanceCallbackRoute callbackRoute;
    std::vector<umbra::detail::TsoAttributeUpdatePassel> receivePassels;
    std::vector<umbra::detail::TsoAttributeUpdatePassel> timestampPassels;
  };
  std::map<std::uint64_t, Delivery> deliveries;
  for (auto const& passel : plan.passels) {
    auto const transportationName = umbra::detail::wideFromUtf8(
        passel.transportationName);
    auto const transportationValue = transportationName
        ? ambassadorTransportationTypeValueForFederation(*federationName, *transportationName)
        : std::nullopt;
    auto const reliableTransportation = ambassadorTransportationTypeReliableForFederation(
        *federationName,
        passel.transportationName);
    if (!transportationValue || !reliableTransportation) {
      throw RTIinternalError(
          L"The composed FOM transportation type has no timestamped delivery policy in this federation execution.");
    }
    static_cast<void>(reliableTransportation);

    umbra::detail::TsoAttributeUpdatePassel payloadPassel;
    payloadPassel.transportationName = passel.transportationName;
    payloadPassel.preferredOrderType = passel.preferredOrderType;
    payloadPassel.sentAttributeHandles = passel.sentAttributeHandles;
    payloadPassel.sentRegionHandles = passel.sentRegionHandles;
    payloadPassel.sentRegionSnapshots = passel.sentRegionSnapshots;
    payloadPassel.defaultRegionUsed = passel.defaultRegionUsed;
    for (auto const& recipient : passel.recipients) {
      if (!recipient.callbackRoute) {
        throw RTIinternalError(
            L"The embedded federation has an eligible timestamped attribute recipient without a callback route.");
      }
      auto& delivery = deliveries[recipient.federateId];
      if (!delivery.callbackRoute) {
        delivery.callbackRoute = recipient.callbackRoute;
      }
      if (passel.preferredOrderType == TIMESTAMP) {
        delivery.timestampPassels.push_back(payloadPassel);
      } else {
        delivery.receivePassels.push_back(payloadPassel);
      }
    }
  }

  std::set<std::uint64_t> timeConstrainedRecipients;
  if (federationName && timeSnapshot.timeRegulating) {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const snapshot = embeddedFederationRegistry().timeSnapshotFor(
        *federationName);
    if (!snapshot) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer exposes a coherent time snapshot.");
    }
    for (auto const& federate : snapshot->federates) {
      if (federate.time.timeConstrained) {
        timeConstrainedRecipients.insert(federate.membership.id);
      }
    }
  }

  std::uint64_t messageId = 0;
  if (timeSnapshot.timeRegulating && hasTimestampOrderedAttribute) {
    std::vector<std::uint64_t> queuedTsoRecipientIds;
    std::vector<std::uint64_t> allTimestampedRecipientIds;
    for (auto const& [recipientId, delivery] : deliveries) {
      if (delivery.timestampPassels.empty()) {
        continue;
      }
      allTimestampedRecipientIds.push_back(recipientId);
      if (timeConstrainedRecipients.contains(recipientId)) {
        queuedTsoRecipientIds.push_back(recipientId);
      }
    }

    umbra::detail::TsoAttributeUpdateMessage message;
    message.producingFederateId = *producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandle;
    message.attributes = sentAttributes;
    message.userSuppliedTag = copiedTag;
    message.timestamp = std::shared_ptr<LogicalTime const>(timestamp);
    auto const snapshotsEqual = [](
        umbra::detail::RegionSpecificationSnapshot const& left,
        umbra::detail::RegionSpecificationSnapshot const& right) {
      if (left.dimensionHandles != right.dimensionHandles ||
          left.specificationCommitted != right.specificationCommitted ||
          left.committedRangeBounds.size() != right.committedRangeBounds.size()) {
        return false;
      }
      for (auto const& [dimensionHandle, leftBounds] : left.committedRangeBounds) {
        auto const rightBounds = right.committedRangeBounds.find(dimensionHandle);
        if (rightBounds == right.committedRangeBounds.end() ||
            leftBounds.lowerBound != rightBounds->second.lowerBound ||
            leftBounds.upperBound != rightBounds->second.upperBound) {
          return false;
        }
      }
      return true;
    };
    for (auto const& [recipientId, delivery] : deliveries) {
      if (!delivery.timestampPassels.empty()) {
        for (auto const& passel : delivery.timestampPassels) {
          for (auto const& [regionHandle, snapshot] : passel.sentRegionSnapshots) {
            auto const [snapshotPosition, inserted] =
                message.sentRegionSnapshots.emplace(regionHandle, snapshot);
            if (!inserted &&
                !snapshotsEqual(snapshotPosition->second, snapshot)) {
              throw RTIinternalError(
                  L"The accepted timestamped attribute update contains inconsistent source-region snapshots.");
            }
          }
        }
        message.passelsByRecipient.emplace(recipientId, delivery.timestampPassels);
      }
    }

    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const result = embeddedFederationRegistry()
        .enqueueTsoAttributeUpdate(
            *federationName,
            std::move(message),
            queuedTsoRecipientIds,
            allTimestampedRecipientIds);
    if (result.status != umbra::detail::FederationTsoRegistryStatus::applied ||
        result.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
        result.messageId == 0) {
      throw RTIinternalError(
          L"The embedded federation could not queue the timestamped Update Attribute Values service.");
    }
    messageId = result.messageId;
  }

  umbra::detail::MomServiceArgument returnedArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  if (messageId != 0U) {
    returnedArgument = {
        umbra::detail::MomArgumentType::message_retraction_handle,
        L"Message retraction designator",
        umbra::detail::formatMomMessageRetractionHandle(messageId)};
  }
  // Timestamped Update Attribute Values admission is the accepted service
  // boundary. Emit the standard MOM report after C++ validation/queue
  // admission, but before any immediate or grant-delayed reflection callback.
  appendSuccessfulServiceReportToFileIfSelected(
      L"UpdateAttributeValues",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument,
      true);

  // Count the successful timestamped invocation once queue admission and all
  // synchronous validation have completed. Timestamped payload delivery is a
  // later boundary and must not alter the MOM service-invocation count.
  std::set<std::string> acceptedTransportationNames;
  for (auto const& passel : plan.passels) {
    acceptedTransportationNames.insert(passel.transportationName);
  }
  {
    std::scoped_lock lock(ambassadorFederationManagementMutex());
    auto const status = embeddedFederationRegistry()
        .recordSuccessfulUpdateAttributeValues(
            *federationName,
            *producingFederateId,
            *objectInstanceHandle,
            plan.registeredObjectClassHandle,
            acceptedTransportationNames);
    if (status != umbra::detail::FederationRegistryStatus::applied) {
      throw RTIinternalError(
          L"The embedded federation lost the timestamped Update Attribute Values membership before its accepted boundary.");
    }
  }

  // Time-constrained recipients consume the typed payload at their grant.
  // Other recipients receive the timestamped callback immediately in the
  // bounded embedded profile; the callback still rechecks each accepted
  // passel immediately before entering user code.
  for (auto& [recipientId, delivery] : deliveries) {
    bool const queuedTimestamped =
        timeSnapshot.timeRegulating &&
        timeConstrainedRecipients.contains(recipientId) &&
        !delivery.timestampPassels.empty();
    if (!delivery.receivePassels.empty()) {
      queueAmbassadorTimestampedReflectAttributeUpdate(
          delivery.callbackRoute,
          *federationName,
          *producingFederateId,
          recipientId,
          *objectInstanceHandle,
          sentAttributes,
          std::move(delivery.receivePassels),
          copiedTag,
          std::shared_ptr<LogicalTime const>(timestamp),
          RECEIVE,
          RECEIVE,
          std::nullopt);
    }
    if (!queuedTimestamped && !delivery.timestampPassels.empty()) {
      queueAmbassadorTimestampedReflectAttributeUpdate(
          std::move(delivery.callbackRoute),
          *federationName,
          *producingFederateId,
          recipientId,
          *objectInstanceHandle,
          sentAttributes,
          std::move(delivery.timestampPassels),
          copiedTag,
          std::shared_ptr<LogicalTime const>(timestamp),
          timeSnapshot.timeRegulating ? TIMESTAMP : RECEIVE,
          RECEIVE,
          timeSnapshot.timeRegulating && messageId != 0
              ? std::optional<std::uint64_t>(messageId)
              : std::nullopt);
    }
  }

  if (!timeSnapshot.timeRegulating || messageId == 0) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Update Attribute Values", exception);
    appendFailedServiceReportToFileIfSelected(
        L"UpdateAttributeValues",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::attribute_handle_value_map,
          L"Constrained set of attribute designator and value pairs",
          umbra::detail::formatMomAttributeHandleValueMap(attributeValues)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::logical_time,
          L"Optional timestamp",
          umbra::detail::formatMomLogicalTime(time)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
