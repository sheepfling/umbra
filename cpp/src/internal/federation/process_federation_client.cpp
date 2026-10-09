#include "internal/federation/process_federation_client.hpp"
#include "internal/federation/process_federation_client_error.hpp"
#include "internal/federation/process_federation_client_event_templates.hpp"

#include <RTI/Exception.h>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace umbra::detail {
namespace {

[[nodiscard]] char const* statusName(TransportServiceStatus status) noexcept {
  switch (status) {
    case TransportServiceStatus::ok:
      return "ok";
    case TransportServiceStatus::rejected:
      return "rejected";
    case TransportServiceStatus::invalid_request:
      return "invalid-request";
    case TransportServiceStatus::internal_error:
      return "internal-error";
    case TransportServiceStatus::service_reporting_interlock:
      return "service-reporting-interlock";
  }
  return "unknown";
}

[[nodiscard]] char const* operationName(
    TransportServiceOperation operation) noexcept {
  switch (operation) {
    case TransportServiceOperation::create_federation_execution:
      return "create-federation-execution";
    case TransportServiceOperation::destroy_federation_execution:
      return "destroy-federation-execution";
    case TransportServiceOperation::unpublish_object_class_attributes:
      return "unpublish-object-class-attributes";
    case TransportServiceOperation::unpublish_object_class:
      return "unpublish-object-class";
    case TransportServiceOperation::get_update_rate_value:
      return "get-update-rate-value";
    case TransportServiceOperation::get_update_rate_value_for_attribute:
      return "get-update-rate-value-for-attribute";
    case TransportServiceOperation::join_federation_execution:
      return "join-federation-execution";
    case TransportServiceOperation::resign_federation_execution:
      return "resign-federation-execution";
    case TransportServiceOperation::query_logical_time:
      return "query-logical-time";
    case TransportServiceOperation::query_lookahead:
      return "query-lookahead";
    case TransportServiceOperation::modify_lookahead:
      return "modify-lookahead";
    case TransportServiceOperation::enable_time_regulation:
      return "enable-time-regulation";
    case TransportServiceOperation::query_time_bounds:
      return "query-time-bounds";
    case TransportServiceOperation::enable_time_constrained:
      return "enable-time-constrained";
    case TransportServiceOperation::time_advance_request:
      return "time-advance-request";
    case TransportServiceOperation::time_advance_request_available:
      return "time-advance-request-available";
    case TransportServiceOperation::next_message_request:
      return "next-message-request";
    case TransportServiceOperation::next_message_request_available:
      return "next-message-request-available";
    case TransportServiceOperation::flush_queue_request:
      return "flush-queue-request";
    case TransportServiceOperation::acknowledge_tso_delivery:
      return "acknowledge-tso-delivery";
    case TransportServiceOperation::get_object_instance_handle:
      return "get-object-instance-handle";
    case TransportServiceOperation::get_object_instance_name:
      return "get-object-instance-name";
    case TransportServiceOperation::get_object_class_name:
      return "get-object-class-name";
    case TransportServiceOperation::get_interaction_class_name:
      return "get-interaction-class-name";
    case TransportServiceOperation::get_attribute_name:
      return "get-attribute-name";
    case TransportServiceOperation::get_parameter_name:
      return "get-parameter-name";
    case TransportServiceOperation::get_dimension_name:
      return "get-dimension-name";
    case TransportServiceOperation::get_transportation_type_handle:
      return "get-transportation-type-handle";
    case TransportServiceOperation::get_transportation_type_name:
      return "get-transportation-type-name";
    case TransportServiceOperation::get_available_dimensions_for_object_class:
      return "get-available-dimensions-for-object-class";
    case TransportServiceOperation::get_available_dimensions_for_interaction_class:
      return "get-available-dimensions-for-interaction-class";
    case TransportServiceOperation::get_convey_region_designator_sets_switch:
      return "get-convey-region-designator-sets-switch";
    case TransportServiceOperation::get_allow_relaxed_ddm_switch:
      return "get-allow-relaxed-ddm-switch";
    case TransportServiceOperation::set_convey_region_designator_sets_switch:
      return "set-convey-region-designator-sets-switch";
    case TransportServiceOperation::subscribe_interaction_class_with_regions:
      return "subscribe-interaction-class-with-regions";
    case TransportServiceOperation::unsubscribe_interaction_class_with_regions:
      return "unsubscribe-interaction-class-with-regions";
    case TransportServiceOperation::send_interaction_with_regions:
      return "send-interaction-with-regions";
    case TransportServiceOperation::request_attribute_value_update:
      return "request-attribute-value-update";
    case TransportServiceOperation::request_attribute_value_update_class:
      return "request-attribute-value-update-class";
    case TransportServiceOperation::request_attribute_value_update_class_with_regions:
      return "request-attribute-value-update-class-with-regions";
    case TransportServiceOperation::time_advance_grant:
      return "time-advance-grant";
    case TransportServiceOperation::send_interaction:
      return "send-interaction";
    case TransportServiceOperation::update_attribute_values:
      return "update-attribute-values";
    case TransportServiceOperation::receive_interaction:
      return "receive-interaction";
    case TransportServiceOperation::receive_attribute_update:
      return "receive-attribute-update";
    case TransportServiceOperation::get_interaction_class_handle:
      return "get-interaction-class-handle";
    case TransportServiceOperation::get_object_class_handle:
      return "get-object-class-handle";
    case TransportServiceOperation::get_parameter_handle:
      return "get-parameter-handle";
    case TransportServiceOperation::publish_interaction_class:
      return "publish-interaction-class";
    case TransportServiceOperation::unpublish_interaction_class:
      return "unpublish-interaction-class";
    case TransportServiceOperation::subscribe_interaction_class:
      return "subscribe-interaction-class";
    case TransportServiceOperation::unsubscribe_interaction_class:
      return "unsubscribe-interaction-class";
    case TransportServiceOperation::publish_object_class_attributes:
      return "publish-object-class-attributes";
    case TransportServiceOperation::register_object_instance:
      return "register-object-instance";
    case TransportServiceOperation::get_attribute_handle:
      return "get-attribute-handle";
    case TransportServiceOperation::subscribe_object_class_attributes:
      return "subscribe-object-class-attributes";
    case TransportServiceOperation::unsubscribe_object_class_attributes:
      return "unsubscribe-object-class-attributes";
    case TransportServiceOperation::receive_object_instance_discovery:
      return "receive-object-instance-discovery";
    case TransportServiceOperation::reserve_object_instance_name:
      return "reserve-object-instance-name";
    case TransportServiceOperation::release_object_instance_name:
      return "release-object-instance-name";
    case TransportServiceOperation::reserve_multiple_object_instance_names:
      return "reserve-multiple-object-instance-names";
    case TransportServiceOperation::release_multiple_object_instance_names:
      return "release-multiple-object-instance-names";
    case TransportServiceOperation::get_dimension_handle:
      return "get-dimension-handle";
    case TransportServiceOperation::get_dimension_upper_bound:
      return "get-dimension-upper-bound";
    case TransportServiceOperation::create_region:
      return "create-region";
    case TransportServiceOperation::commit_region_modifications:
      return "commit-region-modifications";
    case TransportServiceOperation::delete_region:
      return "delete-region";
    case TransportServiceOperation::get_dimension_handle_set:
      return "get-dimension-handle-set";
    case TransportServiceOperation::get_range_bounds:
      return "get-range-bounds";
    case TransportServiceOperation::set_range_bounds:
      return "set-range-bounds";
    case TransportServiceOperation::register_object_instance_with_regions:
      return "register-object-instance-with-regions";
    case TransportServiceOperation::subscribe_object_class_attributes_with_regions:
      return "subscribe-object-class-attributes-with-regions";
    case TransportServiceOperation::unsubscribe_object_class_attributes_with_regions:
      return "unsubscribe-object-class-attributes-with-regions";
    case TransportServiceOperation::get_attribute_scope_advisory_switch:
      return "get-attribute-scope-advisory-switch";
    case TransportServiceOperation::set_attribute_scope_advisory_switch:
      return "set-attribute-scope-advisory-switch";
    case TransportServiceOperation::get_object_class_relevance_advisory_switch:
      return "get-object-class-relevance-advisory-switch";
    case TransportServiceOperation::associate_regions_for_updates:
      return "associate-regions-for-updates";
    case TransportServiceOperation::unassociate_regions_for_updates:
      return "unassociate-regions-for-updates";
    case TransportServiceOperation::get_attribute_relevance_advisory_switch:
      return "get-attribute-relevance-advisory-switch";
    case TransportServiceOperation::set_attribute_relevance_advisory_switch:
      return "set-attribute-relevance-advisory-switch";
    case TransportServiceOperation::get_interaction_relevance_advisory_switch:
      return "get-interaction-relevance-advisory-switch";
    case TransportServiceOperation::publish_object_class_directed_interactions:
      return "publish-object-class-directed-interactions";
    case TransportServiceOperation::unpublish_object_class_directed_interactions:
      return "unpublish-object-class-directed-interactions";
    case TransportServiceOperation::subscribe_object_class_directed_interactions:
      return "subscribe-object-class-directed-interactions";
    case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
      return "unsubscribe-object-class-directed-interactions";
    case TransportServiceOperation::send_directed_interaction:
      return "send-directed-interaction";
    case TransportServiceOperation::retract:
      return "retract";
    case TransportServiceOperation::request_retraction:
      return "request-retraction";
    case TransportServiceOperation::local_delete_object_instance:
      return "local-delete-object-instance";
    case TransportServiceOperation::delete_object_instance:
      return "delete-object-instance";
    case TransportServiceOperation::is_attribute_owned_by_federate:
      return "is-attribute-owned-by-federate";
    case TransportServiceOperation::get_known_object_class_handle:
      return "get-known-object-class-handle";
    case TransportServiceOperation::query_attribute_ownership:
      return "query-attribute-ownership";
    case TransportServiceOperation::attribute_ownership_acquisition_if_available:
      return "attribute-ownership-acquisition-if-available";
    case TransportServiceOperation::attribute_ownership_acquisition:
      return "attribute-ownership-acquisition";
    case TransportServiceOperation::attribute_ownership_release_denied:
      return "attribute-ownership-release-denied";
    case TransportServiceOperation::cancel_attribute_ownership_acquisition:
      return "cancel-attribute-ownership-acquisition";
    case TransportServiceOperation::
        cancel_negotiated_attribute_ownership_divestiture:
      return "cancel-negotiated-attribute-ownership-divestiture";
    case TransportServiceOperation::negotiated_attribute_ownership_divestiture:
      return "negotiated-attribute-ownership-divestiture";
    case TransportServiceOperation::confirm_divestiture:
      return "confirm-divestiture";
    case TransportServiceOperation::get_federate_handle:
      return "get-federate-handle";
    case TransportServiceOperation::get_federate_name:
      return "get-federate-name";
    case TransportServiceOperation::normalize_federate_handle:
      return "normalize-federate-handle";
    case TransportServiceOperation::normalize_object_class_handle:
      return "normalize-object-class-handle";
    case TransportServiceOperation::normalize_interaction_class_handle:
      return "normalize-interaction-class-handle";
    case TransportServiceOperation::normalize_object_instance_handle:
      return "normalize-object-instance-handle";
    case TransportServiceOperation::get_automatic_resign_directive:
      return "get-automatic-resign-directive";
    case TransportServiceOperation::set_automatic_resign_directive:
      return "set-automatic-resign-directive";
    case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
      return "unconditional-attribute-ownership-divestiture";
    case TransportServiceOperation::change_interaction_order_type:
      return "change-interaction-order-type";
    case TransportServiceOperation::change_attribute_order_type:
      return "change-attribute-order-type";
    case TransportServiceOperation::change_default_attribute_order_type:
      return "change-default-attribute-order-type";
    case TransportServiceOperation::change_default_attribute_transportation_type:
      return "change-default-attribute-transportation-type";
    case TransportServiceOperation::request_attribute_transportation_type_change:
      return "request-attribute-transportation-type-change";
    case TransportServiceOperation::query_attribute_transportation_type:
      return "query-attribute-transportation-type";
    case TransportServiceOperation::request_interaction_transportation_type_change:
      return "request-interaction-transportation-type-change";
    case TransportServiceOperation::query_interaction_transportation_type:
      return "query-interaction-transportation-type";
    case TransportServiceOperation::register_federation_synchronization_point:
      return "register-federation-synchronization-point";
    case TransportServiceOperation::synchronization_point_achieved:
      return "synchronization-point-achieved";
    case TransportServiceOperation::request_federation_save:
      return "request-federation-save";
    case TransportServiceOperation::federate_save_begun:
      return "federate-save-begun";
    case TransportServiceOperation::federate_save_complete:
      return "federate-save-complete";
    case TransportServiceOperation::federate_save_not_complete:
      return "federate-save-not-complete";
    case TransportServiceOperation::query_federation_save_status:
      return "query-federation-save-status";
    case TransportServiceOperation::abort_federation_save:
      return "abort-federation-save";
    case TransportServiceOperation::request_federation_restore:
      return "request-federation-restore";
    case TransportServiceOperation::federate_restore_complete:
      return "federate-restore-complete";
    case TransportServiceOperation::federate_restore_not_complete:
      return "federate-restore-not-complete";
    case TransportServiceOperation::abort_federation_restore:
      return "abort-federation-restore";
    case TransportServiceOperation::query_federation_restore_status:
      return "query-federation-restore-status";
    case TransportServiceOperation::get_service_reporting_switch:
      return "get-service-reporting-switch";
    case TransportServiceOperation::set_service_reporting_switch:
      return "set-service-reporting-switch";
    case TransportServiceOperation::get_exception_reporting_switch:
      return "get-exception-reporting-switch";
    case TransportServiceOperation::set_exception_reporting_switch:
      return "set-exception-reporting-switch";
    case TransportServiceOperation::report_service_exception:
      return "report-service-exception";
    case TransportServiceOperation::report_failed_service_invocation:
      return "report-failed-service-invocation";
    case TransportServiceOperation::report_successful_service_invocation:
      return "report-successful-service-invocation";
    case TransportServiceOperation::report_successful_void_service_invocation:
      return "report-successful-void-service-invocation";
    case TransportServiceOperation::recheck_exception_report:
      return "recheck-exception-report";
    case TransportServiceOperation::get_send_service_reports_to_file_switch:
      return "get-send-service-reports-to-file-switch";
    case TransportServiceOperation::set_send_service_reports_to_file_switch:
      return "set-send-service-reports-to-file-switch";
  }
  return "unknown";
}

}  // namespace

std::string requestFailure(
    TransportServiceOperation operation,
    TransportServiceStatus status) {
  return std::string("Process federation ") + operationName(operation) +
      " was not accepted (" + statusName(status) + ").";
}

ProcessFederationClient::ProcessFederationClient(
    ProcessTransportAddress address,
    TransportEndpointIdentity localIdentity,
    std::shared_ptr<RuntimeInstrumentation> instrumentation,
    FailureHandler failureHandler,
    ForcedResignationHandler forcedResignationHandler)
    : failureHandler_(failureHandler ? std::move(failureHandler)
                                    : FailureHandler{[](std::wstring) {}}),
      connection_(ProcessTransportConnection::connectClient(
          this,
          std::move(address),
          std::move(localIdentity),
          [this](std::wstring description) {
            bool dispatchNow = false;
            {
              std::scoped_lock lock(transactionMutex_);
              pendingTransportFailures_.push_back(std::move(description));
              dispatchNow = requestDepth_ == 0U;
            }
            if (dispatchNow) {
              dispatchTransportFailures();
            }
          },
          forcedResignationHandler
              ? std::move(forcedResignationHandler)
              : ForcedResignationHandler{[](std::wstring) { return false; }},
          std::move(instrumentation))),
      shutdownConnection_(connection_),
      session_(std::make_unique<ProcessTransportSession>(connection_)),
      attributeRelevanceAdvisorySwitchState_(
          std::make_shared<std::atomic_bool>(true)) {}

ProcessFederationClient::~ProcessFederationClient() {
  close();
}

void ProcessFederationClient::bufferReceiveResult(
    ProcessFederationReceiveInteractionResult result) {
  if (result.event) {
    pendingEvents_.emplace_back(std::move(*result.event));
  }
  if (result.attributeEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeEvent));
  }
  if (result.attributeValueUpdateRequestEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeValueUpdateRequestEvent));
  }
  if (result.attributeOwnershipQueryEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeOwnershipQueryEvent));
  }
  if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
  }
  if (result.attributeOwnershipAcquisitionEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeOwnershipAcquisitionEvent));
  }
  if (result.attributeOwnershipUnavailableEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeOwnershipUnavailableEvent));
  }
  if (result.discoveryEvent) {
    pendingEvents_.emplace_back(std::move(*result.discoveryEvent));
  }
  if (result.removalEvent) {
    pendingEvents_.emplace_back(std::move(*result.removalEvent));
  }
  if (result.scopeChangeEvent) {
    pendingEvents_.emplace_back(std::move(*result.scopeChangeEvent));
  }
  if (result.attributeRelevanceAdvisoryEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeRelevanceAdvisoryEvent));
  }
  if (result.attributeTransportationTypeChangeEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeTransportationTypeChangeEvent));
  }
  if (result.attributeTransportationTypeQueryEvent) {
    pendingEvents_.emplace_back(std::move(*result.attributeTransportationTypeQueryEvent));
  }
  if (result.interactionTransportationTypeChangeEvent) {
    pendingEvents_.emplace_back(std::move(*result.interactionTransportationTypeChangeEvent));
  }
  if (result.interactionTransportationTypeQueryEvent) {
    pendingEvents_.emplace_back(std::move(*result.interactionTransportationTypeQueryEvent));
  }
  if (result.synchronizationPointAnnouncementEvent) {
    pendingEvents_.emplace_back(std::move(*result.synchronizationPointAnnouncementEvent));
  }
  if (result.federationSynchronizedEvent) {
    pendingEvents_.emplace_back(std::move(*result.federationSynchronizedEvent));
  }
  if (result.saveEvent) {
    pendingEvents_.emplace_back(std::move(*result.saveEvent));
  }
  if (result.restoreEvent) {
    pendingEvents_.emplace_back(std::move(*result.restoreEvent));
  }
}

void ProcessFederationClient::bufferEvent(TransportServiceMessage message) {
  if (message.kind != TransportServiceMessageKind::event ||
      message.status != TransportServiceStatus::ok || message.requestId != 0U) {
    throw ProcessFederationClientError(
        "Process federation client received an invalid unsolicited event.");
  }
  switch (message.operation) {
    case TransportServiceOperation::receive_interaction: {
      auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
      auto const count = pendingEvents_.size();
      bufferReceiveResult(std::move(result));
      if (pendingEvents_.size() == count) {
        throw ProcessFederationClientError(
            "Process federation client received an empty unsolicited event.");
      }
      return;
    }
    case TransportServiceOperation::receive_attribute_update: {
      auto result = decodeProcessFederationReceiveAttributeUpdateResult(message.payload);
      if (!result.event) {
        throw ProcessFederationClientError(
            "Process federation client received an empty unsolicited attribute event.");
      }
      pendingEvents_.emplace_back(std::move(*result.event));
      return;
    }
    case TransportServiceOperation::receive_object_instance_discovery: {
      auto result = decodeProcessFederationReceiveObjectInstanceDiscoveryResult(message.payload);
      if (!result.event) {
        throw ProcessFederationClientError(
            "Process federation client received an empty unsolicited discovery event.");
      }
      pendingEvents_.emplace_back(std::move(*result.event));
      return;
    }
    case TransportServiceOperation::time_advance_grant: {
      auto result = decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingEvents_.emplace_back(PendingFlushQueueGrant{
            std::move(*result.grantedTime), std::move(*result.optimisticTime)});
      } else {
        pendingEvents_.emplace_back(std::move(*result.grantedTime));
      }
      return;
    }
    case TransportServiceOperation::request_retraction: {
      auto const retraction = decodeProcessFederationRequestRetractionEvent(message.payload);
      auto const removed = std::erase_if(pendingEvents_, [&retraction](PendingEvent const& event) {
        return std::visit([&retraction](auto const& value) {
          using Event = std::decay_t<decltype(value)>;
          if constexpr (std::is_same_v<Event, ProcessFederationInteractionEvent> ||
                        std::is_same_v<Event, ProcessFederationObjectInstanceRemovalEvent>) {
            return value.retractionMessageId == retraction.messageId;
          }
          return false;
        }, event);
      });
      if (removed != 0U) {
        deferTsoDeliveryAcknowledgement(retraction.messageId);
      } else {
        // Even Request Retraction can invoke user code. Admit it in stream
        // order and dispatch only after the enclosing exchange releases.
        pendingEvents_.emplace_back(PendingRequestRetraction{retraction.messageId});
      }
      return;
    }
    default:
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed message.");
  }
}

TransportServiceMessage ProcessFederationClient::request(
    TransportServiceOperation operation,
    std::vector<std::uint8_t> payload,
    bool dispatchCallbacks) {
  std::unique_lock lock(transactionMutex_);
  if (closing_.load(std::memory_order_acquire) || !session_) {
    throw ProcessFederationClientError(
        "A process federation request was attempted after client close.");
  }
  if (nextRequestId_ == std::numeric_limits<std::uint64_t>::max()) {
    throw ProcessFederationClientError(
        "The process federation request identity space is exhausted.");
  }
  auto const requestId = ++nextRequestId_;
  ++requestDepth_;
  try {
    TransportServiceMessage requestMessage{
        TransportServiceMessageKind::request, operation,
        TransportServiceStatus::ok, requestId, std::move(payload)};
    if (!session_->send(requestMessage)) {
      throw ProcessFederationClientError(
          std::string("Process federation ") + operationName(operation) +
          " could not send its request.");
    }
    while (true) {
      TransportServiceMessage response;
      if (!session_->receive(response)) {
        throw ProcessFederationClientError(
            std::string("Process federation ") + operationName(operation) +
            " lost its response.");
      }
      if (response.kind == TransportServiceMessageKind::event) {
        bufferEvent(std::move(response));
        continue;
      }
      if (response.kind != TransportServiceMessageKind::response ||
          response.requestId != requestId || response.operation != operation) {
        throw ProcessFederationClientError(
            std::string("Process federation client received a response for another request (expected ") +
            operationName(operation) + "#" + std::to_string(requestId) + ", got " +
            operationName(response.operation) + "#" +
            std::to_string(response.requestId) + ").");
      }
      --requestDepth_;
      lock.unlock();
      // The complete response belongs to this caller now. In particular an
      // evoker holding the dispatcher can acquire the stream while a service
      // thread waits for that dispatcher: no exchange/dispatcher ABBA cycle.
      if (dispatchCallbacks) {
        dispatchPendingPushedEvents();
        flushDeferredTsoDeliveryAcknowledgements();
      }
      return response;
    }
  } catch (...) {
    if (lock.owns_lock()) {
      --requestDepth_;
      lock.unlock();
    }
    // A projection hook must release its lifetime lease before any user
    // callback can tear down the client. Its outer evoke/service boundary
    // will report a queued transport failure after projection returns.
    if (dispatchCallbacks) {
      dispatchTransportFailures();
    }
    throw;
  }
}

void ProcessFederationClient::dispatchTransportFailures() {
  FailureHandler handler;
  std::deque<std::wstring> failures;
  {
    std::scoped_lock lock(transactionMutex_);
    handler = failureHandler_;
    failures.swap(pendingTransportFailures_);
  }
  if (handler) {
    for (auto& description : failures) {
      handler(std::move(description));
    }
  }
}

ProcessFederationLogicalTime ProcessFederationClient::receivePushedTimeAdvanceGrant() {
  return receivePendingEvent<ProcessFederationLogicalTime>();
}

ProcessFederationClient::PendingFlushQueueGrant
ProcessFederationClient::receivePushedFlushQueueGrant() {
  return receivePendingEvent<PendingFlushQueueGrant>();
}

void ProcessFederationClient::acknowledgeTsoDelivery(
    std::uint64_t messageId) {
  if (messageId == 0U) {
    throw ProcessFederationClientError(
        "A process TSO delivery acknowledgement requires a message identity.");
  }
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  {
    std::scoped_lock lock(transactionMutex_);
    if (requestDepth_ != 0U) {
      deferTsoDeliveryAcknowledgement(messageId);
      return;
    }
    if (!session_ || !joinedFederationName_ || joinedFederateId_ == 0U) {
      return;
    }
    federationName = *joinedFederationName_;
    federateId = joinedFederateId_;
  }
  auto response = request(
      TransportServiceOperation::acknowledge_tso_delivery,
      encodeProcessFederationAcknowledgeTsoDeliveryRequest(
          ProcessFederationAcknowledgeTsoDeliveryRequest{
              std::move(federationName), federateId, messageId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto const result =
      decodeProcessFederationTsoDeliveryAcknowledgementResult(response.payload);
  switch (result.status) {
    case ProcessFederationTsoDeliveryAcknowledgementStatus::applied:
    case ProcessFederationTsoDeliveryAcknowledgementStatus::already_completed:
    case ProcessFederationTsoDeliveryAcknowledgementStatus::not_in_transit:
      return;
  }
  throw ProcessFederationClientError(
      "Process federation client received an invalid TSO delivery acknowledgement result.");
}

void ProcessFederationClient::deferTsoDeliveryAcknowledgement(
    std::uint64_t messageId) {
  std::scoped_lock lock(transactionMutex_);
  if (messageId == 0U) {
    throw ProcessFederationClientError(
        "A process TSO delivery acknowledgement requires a message identity.");
  }
  if (std::find(
          deferredTsoDeliveryAcknowledgements_.begin(),
          deferredTsoDeliveryAcknowledgements_.end(),
          messageId) == deferredTsoDeliveryAcknowledgements_.end()) {
    deferredTsoDeliveryAcknowledgements_.push_back(messageId);
  }
}

void ProcessFederationClient::flushDeferredTsoDeliveryAcknowledgements() {
  while (true) {
    std::uint64_t messageId = 0U;
    {
      std::scoped_lock lock(transactionMutex_);
      if (requestDepth_ != 0U || deferredTsoDeliveryAcknowledgements_.empty()) {
        return;
      }
      messageId = deferredTsoDeliveryAcknowledgements_.front();
      deferredTsoDeliveryAcknowledgements_.pop_front();
    }
    acknowledgeTsoDelivery(messageId);
  }
}

ProcessFederationInteractionEvent ProcessFederationClient::receivePushedEvent() {
  return receivePendingEvent<ProcessFederationInteractionEvent>(true);
}

ProcessFederationObjectInstanceRemovalEvent
ProcessFederationClient::receivePushedObjectInstanceRemoval() {
  return receivePendingEvent<ProcessFederationObjectInstanceRemovalEvent>(true);
}

ProcessFederationAttributeUpdateEvent
ProcessFederationClient::receivePushedAttributeUpdate() {
  return receivePendingEvent<ProcessFederationAttributeUpdateEvent>(true);
}

ProcessFederationObjectInstanceDiscoveryEvent
ProcessFederationClient::receivePushedObjectInstanceDiscovery() {
  return receivePendingEvent<ProcessFederationObjectInstanceDiscoveryEvent>(true);
}

ProcessFederationObjectInstanceScopeChangeEvent
ProcessFederationClient::receivePushedObjectInstanceScopeChange() {
  return receivePendingEvent<ProcessFederationObjectInstanceScopeChangeEvent>(true);
}

ProcessFederationAttributeRelevanceAdvisoryEvent
ProcessFederationClient::receivePushedAttributeRelevanceAdvisory() {
  return receivePendingEvent<ProcessFederationAttributeRelevanceAdvisoryEvent>(true);
}

ProcessFederationAttributeTransportationTypeChangeEvent
ProcessFederationClient::receivePushedAttributeTransportationTypeChange() {
  return receivePendingEvent<ProcessFederationAttributeTransportationTypeChangeEvent>();
}

ProcessFederationAttributeTransportationTypeQueryEvent
ProcessFederationClient::receivePushedAttributeTransportationTypeQuery() {
  return receivePendingEvent<ProcessFederationAttributeTransportationTypeQueryEvent>();
}

ProcessFederationInteractionTransportationTypeChangeEvent
ProcessFederationClient::receivePushedInteractionTransportationTypeChange() {
  return receivePendingEvent<ProcessFederationInteractionTransportationTypeChangeEvent>();
}

ProcessFederationInteractionTransportationTypeQueryEvent
ProcessFederationClient::receivePushedInteractionTransportationTypeQuery() {
  return receivePendingEvent<ProcessFederationInteractionTransportationTypeQueryEvent>();
}

ProcessFederationSynchronizationPointAnnouncementEvent
ProcessFederationClient::receivePushedSynchronizationPointAnnouncement() {
  return receivePendingEvent<ProcessFederationSynchronizationPointAnnouncementEvent>();
}

ProcessFederationFederationSynchronizedEvent
ProcessFederationClient::receivePushedFederationSynchronized() {
  return receivePendingEvent<ProcessFederationFederationSynchronizedEvent>();
}

ProcessFederationSaveEvent ProcessFederationClient::receivePushedFederationSave() {
  return receivePendingEvent<ProcessFederationSaveEvent>();
}

ProcessFederationRestoreEvent
ProcessFederationClient::receivePushedFederationRestore() {
  return receivePendingEvent<ProcessFederationRestoreEvent>();
}

std::shared_ptr<ProcessFederationCallbackBridge>
ProcessFederationClient::requireCallbackBridge() const {
  std::scoped_lock lock(transactionMutex_);
  if (!callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge has not been attached.");
  }
  return callbackBridge_;
}

}  // namespace umbra::detail
