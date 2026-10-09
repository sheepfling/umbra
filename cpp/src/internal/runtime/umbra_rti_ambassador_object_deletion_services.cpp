#include "internal/runtime/umbra_rti_ambassador.hpp"

#include "internal/encoding/variable_length_data_2025.hpp"
#include "internal/handles/message_retraction_handle.hpp"
#include "internal/handles/object_instance_handle.hpp"
#include "internal/handles/transportation_type_handle.hpp"
#include "internal/observability/mom_service_report_encoding.hpp"
#include "internal/runtime/ambassador_shared_utilities.hpp"
#include "internal/runtime/ambassador_string_helpers.hpp"
#include "internal/runtime/umbra_rti_ambassador_service_failure_translation.hpp"

#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

namespace rti1516_2025::umbra_binding_detail {
using namespace service_failure_translation;

void UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag) {
  auto instrumentationScope = beginRtiCall("deleteObjectInstance");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint owns this receive-order object-deletion
  // slice. Keep the official handle and membership checks on the public
  // adapter, then carry the caller-owned tag through the private envelope.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t deletingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Delete Object Instance requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      deletingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Delete Object Instance requires a known ObjectInstanceHandle.");
    }
    umbra::detail::ProcessFederationDeleteObjectInstanceResult result;
    try {
      result = processClient->deleteObjectInstance(
          std::move(federationName),
          deletingFederateId,
          *objectInstanceValue,
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag));
      if (result.status != umbra::detail::ObjectInstanceDeletionStatus::applied) {
        throwObjectInstanceDeletionFailure(result.status);
      }
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    return;
  }
#endif
  try {
  std::wstring federationName;
  std::vector<umbra::detail::ObjectInstanceRemovalRecipient> removals;
  bool accepted = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Delete Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Delete Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"The supplied ObjectInstanceHandle is not known to this federate.");
    }
    auto deletion = registry.deleteObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle);
    if (deletion.status != umbra::detail::ObjectInstanceDeletionStatus::applied) {
      throwObjectInstanceDeletionFailure(deletion.status);
    }

    accepted = true;
    federationName = *joinedFederationName_;
    removals = std::move(deletion.recipients);
  }
  // Section 6.16 has accepted the receive-order deletion and committed its
  // local object transition. Emit after releasing native locks and before
  // queueing any induced Remove Object Instance callbacks.
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"DeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
         {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
          L"User-supplied tag",
          umbra::detail::formatMomUserSuppliedTag(userSuppliedTag)},
         {umbra::detail::MomArgumentType::null_value,
          L"Optional timestamp",
          umbra::detail::formatMomNull()}},
        true);
  }
  queueAmbassadorObjectInstanceRemovals(std::move(removals), federationName, userSuppliedTag);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Delete Object Instance", exception);
    appendFailedServiceReportToFileIfSelected(
        L"DeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
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

MessageRetractionHandle UmbraRtiAmbassador::deleteObjectInstance(
    ObjectInstanceHandle const& objectInstance,
    VariableLengthData const& userSuppliedTag,
    LogicalTime const& time) {
  auto instrumentationScope = beginRtiCall("deleteObjectInstance");
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  // The configured process endpoint carries timestamp preservation and
  // callback reconstruction. Its execution-owned message identity is the
  // same identity used by the process Retract service, so project it through
  // the official return type instead of discarding the accepted designator.
  if (processEndpointActive_) {
    std::wstring federationName;
    std::uint64_t deletingFederateId = 0U;
    umbra::detail::ProcessFederationClient* processClient = nullptr;
    {
      std::scoped_lock lock(mutex_);
      requireConnectedForFederationManagement(lifecycle_);
      if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
          !joinedFederationName_ || !joinedFederateId_) {
        throw FederateNotExecutionMember(
            L"Timestamped Delete Object Instance requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      deletingFederateId = *joinedFederateId_;
    }

    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Timestamped Delete Object Instance requires a known ObjectInstanceHandle.");
    }
    auto timestamp = cloneAmbassadorReferenceLogicalTime(time.implementationName(), time);
    if (timestamp->isInitial() || timestamp->isFinal()) {
      throw InvalidLogicalTime(
          L"A timestamped service requires a finite logical timestamp.");
    }
    auto const encodedTimestamp = timestamp->encode();
    auto processTimestampBytes =
        umbra::detail::variable_length_data_2025::copyBytes(encodedTimestamp);

    umbra::detail::ProcessFederationDeleteObjectInstanceResult result;
    try {
      result = processClient->deleteObjectInstance(
          std::move(federationName),
          deletingFederateId,
          *objectInstanceValue,
          umbra::detail::variable_length_data_2025::copyBytes(userSuppliedTag),
          umbra::detail::ProcessFederationLogicalTime{
              timestamp->implementationName(),
              std::move(processTimestampBytes)});
      if (result.status !=
          umbra::detail::ObjectInstanceDeletionStatus::applied) {
        throwObjectInstanceDeletionFailure(result.status);
      }
      while (processClient->pendingPushedEventCount() != 0U) {
        processClient->dispatchPushedReceiveOrder();
      }
      while (processClient->pendingPushedObjectInstanceDiscoveryCount() != 0U) {
        processClient->dispatchPushedObjectInstanceDiscovery();
      }
      while (processClient->pendingPushedObjectInstanceRemovalCount() != 0U) {
        processClient->dispatchPushedObjectInstanceRemoval();
      }
      while (processClient->pendingPushedObjectInstanceScopeChangeCount() != 0U) {
        processClient->dispatchPushedObjectInstanceScopeChange();
      }
      while (processClient->pendingPushedAttributeRelevanceAdvisoryCount() != 0U) {
        processClient->dispatchPushedAttributeRelevanceAdvisory();
      }
      while (processClient->pendingPushedAttributeUpdateCount() != 0U) {
        processClient->dispatchPushedAttributeUpdate();
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
    requireFederationServiceOperationAvailable(L"Timestamped Delete Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ || !federateTimeState_) {
      throw FederateNotExecutionMember(
          L"Timestamped Delete Object Instance requires membership in a federation execution.");
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
        L"Timestamped Delete Object Instance requires a known ObjectInstanceHandle.");
  }

  auto timestamp = cloneAmbassadorReferenceLogicalTime(timeState->implementationName(), time);
  auto const timeSnapshot = timeState->snapshot();
  validateAmbassadorTsoTimestamp(timeSnapshot, *timestamp);
  VariableLengthData copiedTag(userSuppliedTag);

  std::wstring federationName;
  std::uint64_t producingFederateId = 0;
  std::vector<std::uint64_t> tsoRecipientIds;
  rti1516_2025::OrderType deletionSentOrderType = rti1516_2025::RECEIVE;
  bool provideRetraction = false;
  umbra::detail::FederationTsoObjectDeletionEnqueueResult enqueueResult;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_ ||
        !federateTimeState_ || federateTimeState_ != timeState) {
      throw FederateNotExecutionMember(
          L"Timestamped Delete Object Instance requires an active joined federate.");
    }
    federationName = *joinedFederationName_;
    producingFederateId = *joinedFederateId_;
    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(federationName, producingFederateId)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }

    auto const plan = registry.planTsoObjectInstanceDeletion(
        federationName,
        producingFederateId,
        *objectInstanceHandle);
    if (plan.status != umbra::detail::ObjectInstanceDeletionStatus::applied) {
      throwObjectInstanceDeletionFailure(plan.status);
    }
    deletionSentOrderType = plan.preferredOrderType;
    provideRetraction =
        timeSnapshot.timeRegulating &&
        plan.preferredOrderType == rti1516_2025::TIMESTAMP;

    std::set<std::uint64_t> timeConstrainedRecipients;
    if (provideRetraction) {
      auto const federationSnapshot = registry.timeSnapshotFor(federationName);
      if (!federationSnapshot) {
        throw RTIinternalError(
            L"The embedded federation no longer exposes a coherent time snapshot.");
      }
      for (auto const& federate : federationSnapshot->federates) {
        if (federate.time.timeConstrained) {
          timeConstrainedRecipients.insert(federate.membership.id);
        }
      }
    }
    for (auto const& recipient : plan.recipients) {
      if (provideRetraction &&
          timeConstrainedRecipients.contains(recipient.receivingFederateId)) {
        tsoRecipientIds.push_back(recipient.receivingFederateId);
      }
    }

    umbra::detail::TsoObjectDeletionMessage message;
    message.producingFederateId = producingFederateId;
    message.objectInstanceHandle = *objectInstanceHandle;
    message.userSuppliedTag = copiedTag;
    message.timestamp = std::shared_ptr<LogicalTime const>(timestamp);
    message.sentOrderType = plan.preferredOrderType;
    enqueueResult = registry.enqueueTsoObjectDeletion(
        federationName,
        producingFederateId,
        *objectInstanceHandle,
        std::move(message),
        tsoRecipientIds);
  }

  if (enqueueResult.status == umbra::detail::FederationTsoRegistryStatus::federation_does_not_exist ||
      enqueueResult.status == umbra::detail::FederationTsoRegistryStatus::federate_not_member) {
    throw FederateNotExecutionMember(
        L"The embedded federation no longer records this RTI ambassador as a member.");
  }
  if (enqueueResult.deletionStatus !=
      umbra::detail::ObjectInstanceDeletionStatus::applied) {
    throwObjectInstanceDeletionFailure(enqueueResult.deletionStatus);
  }
  if (enqueueResult.status != umbra::detail::FederationTsoRegistryStatus::applied ||
      enqueueResult.queueStatus != umbra::detail::TsoMessageQueueStatus::applied ||
      enqueueResult.messageId == 0) {
    throw RTIinternalError(
        L"The embedded federation could not queue the timestamped Delete Object Instance service.");
  }

  // Timestamped deletion is an accepted sender-side service at this queue
  // boundary.  Report it through the standard MOM interaction before any
  // recipient consumes the queued Remove Object Instance callback.  The
  // recipient-side callback report remains distinct: it describes delivery
  // order and callback metadata, while this record describes the originating
  // Delete Object Instance invocation and its optional retraction result.
  auto const reportArguments = std::vector<umbra::detail::MomServiceArgument>{
      {umbra::detail::MomArgumentType::object_instance_handle,
       L"Object instance designator",
       umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
      {umbra::detail::MomArgumentType::table_5_user_supplied_tag,
       L"User-supplied tag",
       umbra::detail::formatMomUserSuppliedTag(copiedTag)},
      {umbra::detail::MomArgumentType::logical_time,
       L"Optional timestamp",
       umbra::detail::formatMomLogicalTime(*timestamp)},
  };
  auto returnedArgument = umbra::detail::MomServiceArgument{
      umbra::detail::MomArgumentType::null_value,
      L"",
      umbra::detail::formatMomNull()};
  if (provideRetraction) {
    returnedArgument = {
        umbra::detail::MomArgumentType::message_retraction_handle,
        L"Message retraction designator",
        umbra::detail::formatMomMessageRetractionHandle(enqueueResult.messageId)};
  }
  appendSuccessfulServiceReportToFileIfSelected(
      L"DeleteObjectInstance",
      umbra::detail::MomServiceType::object_management,
      reportArguments,
      returnedArgument,
      true);

  std::set<std::uint64_t> queuedRecipients(
      tsoRecipientIds.begin(), tsoRecipientIds.end());
  for (auto const& recipient : enqueueResult.recipients) {
    if (queuedRecipients.contains(recipient.receivingFederateId)) {
      continue;
    }
    queueAmbassadorTimestampedObjectInstanceRemoval(
        recipient.callbackRoute,
        recipient.serviceReportRoute,
        federationName,
        recipient.receivingFederateId,
        recipient.objectInstanceHandle,
        enqueueResult.messageId,
        copiedTag,
        std::shared_ptr<LogicalTime const>(timestamp),
        provideRetraction,
        deletionSentOrderType,
        rti1516_2025::RECEIVE);
  }

  if (!provideRetraction) {
    return MessageRetractionHandle();
  }
  return makeMessageRetractionHandle(enqueueResult.messageId);
  } catch (Exception const& exception) {
    emitExceptionReport(L"Delete Object Instance", exception);
    appendFailedServiceReportToFileIfSelected(
        L"DeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)},
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

void UmbraRtiAmbassador::localDeleteObjectInstance(
    ObjectInstanceHandle const& objectInstance) {
  auto instrumentationScope = beginRtiCall("localDeleteObjectInstance");
  try {
#if defined(UMBRA_ENABLE_EMBEDDED_FEDERATION_MANAGEMENT)
  if (processEndpointActive_) {
    auto const objectInstanceValue = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceValue) {
      throw ObjectInstanceNotKnown(
          L"Local Delete Object Instance requires a known ObjectInstanceHandle.");
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
            L"Local Delete Object Instance requires membership in a federation execution.");
      }
      processClient = processFederationClient_.get();
      if (processClient == nullptr) {
        throw RTIinternalError(
            L"The configured process endpoint has no active federation client.");
      }
      federationName = *joinedFederationName_;
      federateId = *joinedFederateId_;
    }
    umbra::detail::ProcessFederationLocalDeleteObjectInstanceResult result;
    try {
      result = processClient->localDeleteObjectInstance(
          std::move(federationName), federateId, *objectInstanceValue);
    } catch (umbra::detail::ProcessFederationServiceProtocolError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    } catch (umbra::detail::ProcessFederationClientError const& error) {
      throw RTIinternalError(wideAscii(error.what()));
    }
    if (result.status !=
        umbra::detail::LocalObjectInstanceDeletionStatus::applied) {
      throwLocalObjectInstanceDeletionFailure(result.status);
    }
    return;
  }
#endif
  bool accepted = false;
  {
    std::scoped_lock lock(mutex_, ambassadorFederationManagementMutex());
    requireConnectedForFederationManagement(lifecycle_);
    requireFederationServiceOperationAvailable(L"Local Delete Object Instance");
    if (lifecycle_.state() != umbra::detail::FederateLifecycleState::joined ||
        !joinedFederationName_ || !joinedFederateId_) {
      throw FederateNotExecutionMember(
          L"Local Delete Object Instance requires membership in a federation execution.");
    }

    auto& registry = embeddedFederationRegistry();
    if (!registry.memberById(*joinedFederationName_, *joinedFederateId_)) {
      throw FederateNotExecutionMember(
          L"The embedded federation no longer records this RTI ambassador as a member.");
    }
    auto const objectInstanceHandle = objectInstanceHandleValue(objectInstance);
    if (!objectInstanceHandle) {
      throw ObjectInstanceNotKnown(
          L"Local Delete Object Instance requires a known ObjectInstanceHandle.");
    }

    auto const status = registry.localDeleteObjectInstance(
        *joinedFederationName_,
        *joinedFederateId_,
        *objectInstanceHandle);
    if (status != umbra::detail::LocalObjectInstanceDeletionStatus::applied) {
      throwLocalObjectInstanceDeletionFailure(status);
    }
    accepted = true;
  }
  // Section 6.18 completes the invoking federate's local-forget transition at
  // successful invocation. Emit after releasing native locks so an
  // HLA_IMMEDIATE observer can safely re-enter this ambassador.
  if (accepted) {
    appendSuccessfulVoidServiceReportToFileIfSelected(
        L"LocalDeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
        true);
  }
  } catch (Exception const& exception) {
    emitExceptionReport(L"Local Delete Object Instance", exception);
    appendFailedServiceReportToFileIfSelected(
        L"LocalDeleteObjectInstance",
        umbra::detail::MomServiceType::object_management,
        {{umbra::detail::MomArgumentType::object_instance_handle,
          L"Object instance designator",
          umbra::detail::formatMomObjectInstanceHandle(objectInstance)}},
        describeAmbassadorException(exception),
        true);
    throw;
  }
}

}  // namespace rti1516_2025::umbra_binding_detail
