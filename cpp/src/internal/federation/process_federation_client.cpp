#include "internal/federation/process_federation_client.hpp"

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

[[nodiscard]] std::string requestFailure(
    TransportServiceOperation operation,
    TransportServiceStatus status) {
  return std::string("Process federation ") + operationName(operation) +
      " was not accepted (" + statusName(status) + ").";
}

}  // namespace

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

void ProcessFederationClient::createFederationExecution(
    std::wstring federationName,
    std::vector<std::wstring> fomModules,
    std::optional<std::wstring> mimModule,
    std::wstring logicalTimeImplementationName) {
  auto response = request(
      TransportServiceOperation::create_federation_execution,
      encodeProcessFederationCreateRequest(
          ProcessFederationCreateRequest{
              std::move(federationName),
              std::move(fomModules),
              std::move(mimModule),
              std::move(logicalTimeImplementationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

ProcessFederationDestroyResult
ProcessFederationClient::destroyFederationExecution(
    std::wstring federationName) {
  auto response = request(
      TransportServiceOperation::destroy_federation_execution,
      encodeProcessFederationDestroyRequest(
          ProcessFederationDestroyRequest{std::move(federationName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDestroyResult(response.payload);
}

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName,
    std::vector<std::wstring> additionalFomModules) {
  return joinFederationExecution(
      std::move(federationName),
      std::move(federateType),
      std::move(requestedFederateName),
      std::move(additionalFomModules),
      ProcessFederationJoinConnectionSnapshot{});
}

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName,
    std::vector<std::wstring> additionalFomModules,
    ProcessFederationJoinConnectionSnapshot connection) {
  auto joinedFederationName = federationName;
  auto response = request(
      TransportServiceOperation::join_federation_execution,
      encodeProcessFederationJoinRequest(
          ProcessFederationJoinRequest{
              std::move(federationName),
              std::move(federateType),
              std::move(requestedFederateName),
              std::move(additionalFomModules),
              std::move(connection)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationJoinResult(response.payload);
  std::scoped_lock lock(transactionMutex_);
  if (!session_) {
    throw ProcessFederationClientError("A process federation join completed after close.");
  }
  joinedFederationName_ = std::move(joinedFederationName);
  joinedFederateId_ = result.federateId;
  return result;
}

void ProcessFederationClient::resignFederationExecution(
    std::wstring federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto resignedFederationName = federationName;
  auto response = request(
      TransportServiceOperation::resign_federation_execution,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName), federateId, resignAction}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  {
    std::scoped_lock lock(transactionMutex_);
    if (joinedFederationName_ &&
        *joinedFederationName_ == resignedFederationName &&
        joinedFederateId_ == federateId) {
      joinedFederationName_.reset();
      joinedFederateId_ = 0U;
      bridge = callbackBridge_;
    }
  }
  if (bridge) {
    bridge->cancelExceptionReportProjections();
  }
}

ProcessFederationRegisterSynchronizationPointResult
ProcessFederationClient::registerFederationSynchronizationPoint(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    std::vector<std::uint8_t> userSuppliedTag,
    std::vector<std::uint64_t> synchronizationSet,
    bool synchronizationSetWasSupplied) {
  auto response = request(
      TransportServiceOperation::register_federation_synchronization_point,
      encodeProcessFederationRegisterSynchronizationPointRequest(
          ProcessFederationRegisterSynchronizationPointRequest{
              std::move(federationName),
              federateId,
              std::move(label),
              std::move(userSuppliedTag),
              std::move(synchronizationSet),
              synchronizationSetWasSupplied}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterSynchronizationPointResult(
      response.payload);
}

ProcessFederationSynchronizationPointAchievedResult
ProcessFederationClient::synchronizationPointAchieved(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    bool successfully) {
  auto response = request(
      TransportServiceOperation::synchronization_point_achieved,
      encodeProcessFederationSynchronizationPointAchievedRequest(
          ProcessFederationSynchronizationPointAchievedRequest{
              std::move(federationName), federateId, std::move(label), successfully}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSynchronizationPointAchievedResult(
      response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::requestFederationSave(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label) {
  return requestFederationSave(
      std::move(federationName),
      federateId,
      std::move(label),
      std::nullopt);
}

ProcessFederationSaveControlResult ProcessFederationClient::requestFederationSave(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::request_federation_save,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{
              std::move(federationName),
              federateId,
              std::move(label),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::federateSaveBegun(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_begun,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::federateSaveComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_complete,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult
ProcessFederationClient::federateSaveNotComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_save_not_complete,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult
ProcessFederationClient::queryFederationSaveStatus(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_federation_save_status,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationSaveControlResult ProcessFederationClient::abortFederationSave(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::abort_federation_save,
      encodeProcessFederationSaveRequest(
          ProcessFederationSaveRequest{std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSaveControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::requestFederationRestore(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring label) {
  auto response = request(
      TransportServiceOperation::request_federation_restore,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, std::move(label)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::federateRestoreComplete(
    std::wstring federationName,
    std::uint64_t federateId,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::federate_restore_complete,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}, callbacksEnabled}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::federateRestoreNotComplete(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::federate_restore_not_complete,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::abortFederationRestore(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::abort_federation_restore,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

ProcessFederationRestoreControlResult
ProcessFederationClient::queryFederationRestoreStatus(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_federation_restore_status,
      encodeProcessFederationRestoreRequest(
          ProcessFederationRestoreRequest{
              std::move(federationName), federateId, {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRestoreControlResult(response.payload);
}

rti1516_2025::ResignAction ProcessFederationClient::getAutomaticResignDirective(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_automatic_resign_directive,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName),
              federateId,
              rti1516_2025::NO_ACTION}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationResignRequest(response.payload).resignAction;
}

void ProcessFederationClient::setAutomaticResignDirective(
    std::wstring federationName,
    std::uint64_t federateId,
    rti1516_2025::ResignAction resignAction) {
  auto response = request(
      TransportServiceOperation::set_automatic_resign_directive,
      encodeProcessFederationResignRequest(
          ProcessFederationResignRequest{
              std::move(federationName), federateId, resignAction}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

bool ProcessFederationClient::getServiceReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_service_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setServiceReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_service_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

ProcessFederationExceptionReportingSwitchResult
ProcessFederationClient::getExceptionReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_exception_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExceptionReportingSwitchResult(response.payload);
}

void ProcessFederationClient::reportServiceException(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring service,
    std::wstring exception) {
  auto const response = request(
      TransportServiceOperation::report_service_exception,
      encodeProcessFederationServiceExceptionRequest({
          std::move(federationName), federateId,
          std::move(service), std::move(exception)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportFailedServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments,
    std::wstring exception) {
  auto const response = request(
      TransportServiceOperation::report_failed_service_invocation,
      encodeProcessFederationFailedServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          std::move(exception)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportSuccessfulServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments,
    MomServiceArgument returnedArgument) {
  auto const response = request(
      TransportServiceOperation::report_successful_service_invocation,
      encodeProcessFederationSuccessfulServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          std::move(returnedArgument)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::reportSuccessfulVoidServiceInvocation(
    std::wstring federationName,
    std::uint64_t federateId,
    MomServiceType serviceType,
    std::wstring service,
    std::vector<MomServiceArgument> suppliedArguments) {
  auto const response = request(
      TransportServiceOperation::report_successful_void_service_invocation,
      encodeProcessFederationSuccessfulServiceInvocationRequest({
          std::move(federationName), federateId, serviceType,
          std::move(service), std::move(suppliedArguments),
          {MomArgumentType::null_value, L"", formatMomNull()}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

std::optional<ProcessFederationInteractionEvent>
ProcessFederationClient::recheckExceptionReport(
    ProcessFederationInteractionEvent event) {
  if (!event.exceptionReportFederateId) {
    return event;
  }
  std::wstring federationName;
  std::uint64_t federateId = 0U;
  {
    std::scoped_lock lock(transactionMutex_);
    if (!session_ || !joinedFederationName_ ||
        joinedFederateId_ != event.receivingFederateId) {
      return std::nullopt;
    }
    federationName = *joinedFederationName_;
    federateId = joinedFederateId_;
  }
  try {
    auto const response = request(
        TransportServiceOperation::recheck_exception_report,
        encodeProcessFederationExceptionReportRecheckRequest({
            std::move(federationName), federateId,
            *event.exceptionReportFederateId}), false);
    if (response.status != TransportServiceStatus::ok) {
      return std::nullopt;
    }
    auto projection = decodeProcessFederationReceiveInteractionResult(response.payload);
    if (!projection.event) {
      return std::nullopt;
    }
    event.interactionClassHandle = projection.event->interactionClassHandle;
    event.parameterHandles = std::move(projection.event->parameterHandles);
    return event;
  } catch (...) {
    // A stale or unreachable advisory report cannot replace a service error
    // or interrupt ordinary callback delivery.
    return std::nullopt;
  }
}

ProcessFederationExceptionReportingSwitchResult
ProcessFederationClient::setExceptionReportingSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_exception_reporting_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationExceptionReportingSwitchResult(response.payload);
}

bool ProcessFederationClient::getSendServiceReportsToFileSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_send_service_reports_to_file_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setSendServiceReportsToFileSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_send_service_reports_to_file_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

ProcessFederationLogicalTime ProcessFederationClient::queryLogicalTime(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_logical_time,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryLogicalTimeResult(response.payload).time;
}

ProcessFederationQueryLookaheadResult ProcessFederationClient::queryLookahead(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_lookahead,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryLookaheadResult(response.payload);
}

ProcessFederationModifyLookaheadResult
ProcessFederationClient::modifyLookahead(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTimeInterval lookahead) {
  auto response = request(
      TransportServiceOperation::modify_lookahead,
      encodeProcessFederationModifyLookaheadRequest(
          ProcessFederationModifyLookaheadRequest{
              std::move(federationName), federateId, std::move(lookahead)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationModifyLookaheadResult(response.payload);
}

ProcessFederationQueryTimeBoundsResult
ProcessFederationClient::queryTimeBounds(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::query_time_bounds,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationQueryTimeBoundsResult(response.payload);
}

ProcessFederationEnableTimeRegulationResult
ProcessFederationClient::enableTimeRegulation(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTimeInterval lookahead) {
  auto response = request(
      TransportServiceOperation::enable_time_regulation,
      encodeProcessFederationEnableTimeRegulationRequest(
          ProcessFederationEnableTimeRegulationRequest{
              std::move(federationName), federateId, std::move(lookahead)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationEnableTimeRegulationResult(response.payload);
}

ProcessFederationEnableTimeConstrainedResult
ProcessFederationClient::enableTimeConstrained(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::enable_time_constrained,
      encodeProcessFederationEnableTimeConstrainedRequest(
          ProcessFederationEnableTimeConstrainedRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationEnableTimeConstrainedResult(response.payload);
}

ProcessFederationTimeDisableResult
ProcessFederationClient::disableTimeRegulation(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::disable_time_regulation,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeDisableResult(response.payload);
}

ProcessFederationTimeDisableResult
ProcessFederationClient::disableTimeConstrained(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::disable_time_constrained,
      encodeProcessFederationQueryLogicalTimeRequest(
          ProcessFederationQueryLogicalTimeRequest{
              std::move(federationName), federateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeDisableResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::timeAdvanceRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::time_advance_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult
ProcessFederationClient::timeAdvanceRequestAvailable(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::time_advance_request_available,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::nextMessageRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::next_message_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult
ProcessFederationClient::nextMessageRequestAvailable(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::next_message_request_available,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

ProcessFederationTimeAdvanceResult ProcessFederationClient::flushQueueRequest(
    std::wstring federationName,
    std::uint64_t federateId,
    ProcessFederationLogicalTime requestedTime) {
  auto response = request(
      TransportServiceOperation::flush_queue_request,
      encodeProcessFederationTimeAdvanceRequest(
          ProcessFederationTimeAdvanceRequest{
              std::move(federationName), federateId, std::move(requestedTime)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationTimeAdvanceResult(response.payload);
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupInteractionClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring interactionClassName) {
  auto response = request(
      TransportServiceOperation::get_interaction_class_handle,
      encodeProcessFederationGetInteractionClassHandleRequest(
          ProcessFederationGetInteractionClassHandleRequest{
              std::move(federationName),
              federateId,
              std::move(interactionClassName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupFederateHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring federateName) {
  auto response = request(
      TransportServiceOperation::get_federate_handle,
      encodeProcessFederationGetFederateHandleRequest(
          ProcessFederationGetFederateHandleRequest{
              std::move(federationName),
              federateId,
              std::move(federateName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupFederateName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t targetFederateId) {
  auto response = request(
      TransportServiceOperation::get_federate_name,
      encodeProcessFederationGetFederateNameRequest(
          ProcessFederationGetFederateNameRequest{
              std::move(federationName), federateId, targetFederateId}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t> ProcessFederationClient::normalizeFederateHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_federate_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::normalizeObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_object_class_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::normalizeInteractionClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_interaction_class_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::normalizeObjectInstanceHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t handle) {
  auto response = request(
      TransportServiceOperation::normalize_object_instance_handle,
      encodeProcessFederationNormalizeHandleRequest(
          ProcessFederationNormalizeHandleRequest{
              std::move(federationName), federateId, handle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectClassName,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::get_object_class_handle,
      encodeProcessFederationGetObjectClassHandleRequest(
          ProcessFederationGetObjectClassHandleRequest{
              std::move(federationName),
              federateId,
              std::move(objectClassName),
              callbacksEnabled}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupParameterHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::wstring parameterName) {
  auto response = request(
      TransportServiceOperation::get_parameter_handle,
      encodeProcessFederationGetParameterHandleRequest(
          ProcessFederationGetParameterHandleRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(parameterName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t> ProcessFederationClient::lookupAttributeHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::wstring attributeName) {
  auto response = request(
      TransportServiceOperation::get_attribute_handle,
      encodeProcessFederationGetAttributeHandleRequest(
          ProcessFederationGetAttributeHandleRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupObjectInstanceHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::get_object_instance_handle,
      encodeProcessFederationGetObjectInstanceHandleRequest(
          ProcessFederationGetObjectInstanceHandleRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::get_object_instance_name,
      encodeProcessFederationGetObjectInstanceNameRequest(
          ProcessFederationGetObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupKnownObjectClassHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::get_known_object_class_handle,
      encodeProcessFederationGetKnownObjectClassHandleRequest(
          ProcessFederationGetKnownObjectClassHandleRequest{
              std::move(federationName), federateId, objectInstanceHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

ProcessFederationUpdateRateValueResult
ProcessFederationClient::getUpdateRateValue(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::get_update_rate_value,
      encodeProcessFederationGetUpdateRateValueRequest(
          ProcessFederationGetUpdateRateValueRequest{
              std::move(federationName),
              federateId,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateRateValueResult(response.payload);
}

ProcessFederationUpdateRateValueResult
ProcessFederationClient::getUpdateRateValueForAttribute(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::get_update_rate_value_for_attribute,
      encodeProcessFederationGetUpdateRateValueForAttributeRequest(
          ProcessFederationGetUpdateRateValueForAttributeRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateRateValueResult(response.payload);
}

ProcessFederationAttributeOwnershipCheckResult
ProcessFederationClient::attributeOwnershipCheck(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::is_attribute_owned_by_federate,
      encodeProcessFederationAttributeOwnershipCheckRequest(
          ProcessFederationAttributeOwnershipCheckRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipCheckResult(response.payload);
}

ProcessFederationAttributeOwnershipQueryResult
ProcessFederationClient::queryAttributeOwnership(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> requestedAttributeHandles) {
  auto response = request(
      TransportServiceOperation::query_attribute_ownership,
      encodeProcessFederationAttributeOwnershipQueryRequest(
          ProcessFederationAttributeOwnershipQueryRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(requestedAttributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipQueryResult(response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionIfAvailableResult
ProcessFederationClient::attributeOwnershipAcquisitionIfAvailable(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> desiredAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_acquisition_if_available,
      encodeProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest(
          ProcessFederationAttributeOwnershipAcquisitionIfAvailableRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(desiredAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionIfAvailableResult(
      response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionResult
ProcessFederationClient::attributeOwnershipAcquisition(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> desiredAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_acquisition,
      encodeProcessFederationAttributeOwnershipAcquisitionRequest(
          ProcessFederationAttributeOwnershipAcquisitionRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(desiredAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionResult(
      response.payload);
}

ProcessFederationBooleanResult
ProcessFederationClient::unconditionalAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag,
    bool callbacksEnabled) {
  auto response = request(
      TransportServiceOperation::unconditional_attribute_ownership_divestiture,
      encodeProcessFederationAttributeOwnershipAcquisitionRequest(
          ProcessFederationAttributeOwnershipAcquisitionRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag),
              callbacksEnabled}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload);
}

ProcessFederationAttributeOwnershipReleaseDeniedResult
ProcessFederationClient::attributeOwnershipReleaseDenied(
    std::wstring federationName,
    std::uint64_t owningFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::attribute_ownership_release_denied,
      encodeProcessFederationAttributeOwnershipReleaseDeniedRequest(
          ProcessFederationAttributeOwnershipReleaseDeniedRequest{
              std::move(federationName),
              owningFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipReleaseDeniedResult(
      response.payload);
}

ProcessFederationAttributeOwnershipAcquisitionCancellationResult
ProcessFederationClient::cancelAttributeOwnershipAcquisition(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::cancel_attribute_ownership_acquisition,
      encodeProcessFederationAttributeOwnershipAcquisitionCancellationRequest(
          ProcessFederationAttributeOwnershipAcquisitionCancellationRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOwnershipAcquisitionCancellationResult(
      response.payload);
}

ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult
ProcessFederationClient::cancelNegotiatedAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::
          cancel_negotiated_attribute_ownership_divestiture,
      encodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest(
          ProcessFederationCancelNegotiatedAttributeOwnershipDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationCancelNegotiatedAttributeOwnershipDivestitureResult(
      response.payload);
}

ProcessFederationNegotiatedAttributeOwnershipDivestitureResult
ProcessFederationClient::negotiatedAttributeOwnershipDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::negotiated_attribute_ownership_divestiture,
      encodeProcessFederationNegotiatedAttributeOwnershipDivestitureRequest(
          ProcessFederationNegotiatedAttributeOwnershipDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationNegotiatedAttributeOwnershipDivestitureResult(
      response.payload);
}

ProcessFederationConfirmDivestitureResult
ProcessFederationClient::confirmDivestiture(
    std::wstring federationName,
    std::uint64_t divestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> attributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::confirm_divestiture,
      encodeProcessFederationConfirmDivestitureRequest(
          ProcessFederationConfirmDivestitureRequest{
              std::move(federationName),
              divestingFederateId,
              objectInstanceHandle,
              std::move(attributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationConfirmDivestitureResult(response.payload);
}

std::optional<std::wstring> ProcessFederationClient::lookupObjectClassName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::get_object_class_name,
      encodeProcessFederationGetObjectClassNameRequest(
          ProcessFederationGetObjectClassNameRequest{
              std::move(federationName), federateId, objectClassHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring>
ProcessFederationClient::lookupInteractionClassName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::get_interaction_class_name,
      encodeProcessFederationGetInteractionClassNameRequest(
          ProcessFederationGetInteractionClassNameRequest{
              std::move(federationName), federateId, interactionClassHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring> ProcessFederationClient::lookupAttributeName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::get_attribute_name,
      encodeProcessFederationGetAttributeNameRequest(
          ProcessFederationGetAttributeNameRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              attributeHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::wstring> ProcessFederationClient::lookupParameterName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::uint64_t parameterHandle) {
  auto response = request(
      TransportServiceOperation::get_parameter_name,
      encodeProcessFederationGetParameterNameRequest(
          ProcessFederationGetParameterNameRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              parameterHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

void ProcessFederationClient::publishInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::publish_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::unpublish_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

ProcessFederationInteractionOrderTypeChangeResult
ProcessFederationClient::changeInteractionOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_interaction_order_type,
      encodeProcessFederationChangeInteractionOrderTypeRequest(
          ProcessFederationChangeInteractionOrderTypeRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionOrderTypeChangeResult(
      response.payload);
}

ProcessFederationAttributeOrderTypeChangeResult
ProcessFederationClient::changeAttributeOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> attributeHandles,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_attribute_order_type,
      encodeProcessFederationChangeAttributeOrderTypeRequest(
          ProcessFederationChangeAttributeOrderTypeRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOrderTypeChangeResult(response.payload);
}

ProcessFederationAttributeOrderTypeDefaultResult
ProcessFederationClient::changeDefaultAttributeOrderType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> attributeHandles,
    rti1516_2025::OrderType orderType) {
  auto response = request(
      TransportServiceOperation::change_default_attribute_order_type,
      encodeProcessFederationChangeDefaultAttributeOrderTypeRequest(
          ProcessFederationChangeDefaultAttributeOrderTypeRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              orderType}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeOrderTypeDefaultResult(response.payload);
}

ProcessFederationAttributeTransportationTypeDefaultResult
ProcessFederationClient::changeDefaultAttributeTransportationType(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> attributeHandles,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::change_default_attribute_transportation_type,
      encodeProcessFederationChangeDefaultAttributeTransportationTypeRequest(
          ProcessFederationChangeDefaultAttributeTransportationTypeRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeDefaultResult(
      response.payload);
}

ProcessFederationAttributeTransportationTypeChangeResult
ProcessFederationClient::requestAttributeTransportationTypeChange(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> attributeHandles,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::request_attribute_transportation_type_change,
      encodeProcessFederationRequestAttributeTransportationTypeChangeRequest(
          ProcessFederationRequestAttributeTransportationTypeChangeRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              {attributeHandles.begin(), attributeHandles.end()},
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeChangeResult(
      response.payload);
}

ProcessFederationAttributeTransportationTypeQueryResult
ProcessFederationClient::queryAttributeTransportationType(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t attributeHandle) {
  auto response = request(
      TransportServiceOperation::query_attribute_transportation_type,
      encodeProcessFederationQueryAttributeTransportationTypeRequest(
          ProcessFederationQueryAttributeTransportationTypeRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              attributeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAttributeTransportationTypeQueryResult(
      response.payload);
}

ProcessFederationInteractionTransportationTypeChangeResult
ProcessFederationClient::requestInteractionTransportationTypeChange(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t interactionClassHandle,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::request_interaction_transportation_type_change,
      encodeProcessFederationRequestInteractionTransportationTypeChangeRequest(
          ProcessFederationRequestInteractionTransportationTypeChangeRequest{
              std::move(federationName),
              requestingFederateId,
              interactionClassHandle,
              transportationTypeHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionTransportationTypeChangeResult(
      response.payload);
}

ProcessFederationInteractionTransportationTypeQueryResult
ProcessFederationClient::queryInteractionTransportationType(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t queriedFederateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::query_interaction_transportation_type,
      encodeProcessFederationQueryInteractionTransportationTypeRequest(
          ProcessFederationQueryInteractionTransportationTypeRequest{
              std::move(federationName),
              requestingFederateId,
              queriedFederateId,
              interactionClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationInteractionTransportationTypeQueryResult(
      response.payload);
}

void ProcessFederationClient::subscribeInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    bool active) {
  auto response = request(
      TransportServiceOperation::subscribe_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, active}));
  if (response.status == TransportServiceStatus::service_reporting_interlock) {
    throw rti1516_2025::FederateServiceInvocationsAreBeingReportedViaMOM(
        L"The report-service-invocation interaction cannot be subscribed while service reporting is enabled.");
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::unsubscribe_interaction_class,
      encodeProcessFederationInteractionClassDeclarationRequest(
          ProcessFederationInteractionClassDeclarationRequest{
              std::move(federationName), federateId, interactionClassHandle, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeInteractionClassWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> regionHandles,
    bool active) {
  auto response = request(
      TransportServiceOperation::subscribe_interaction_class_with_regions,
      encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          ProcessFederationInteractionClassRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(regionHandles),
              active}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeInteractionClassWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle,
    std::set<std::uint64_t> regionHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_interaction_class_with_regions,
      encodeProcessFederationInteractionClassRegionalSubscriptionRequest(
          ProcessFederationInteractionClassRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              interactionClassHandle,
              std::move(regionHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::publishObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::publish_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::vector<std::uint64_t>> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> interactionClassHandles,
    bool universally) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              universally}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassDirectedInteractions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::vector<std::uint64_t>> interactionClassHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_directed_interactions,
      encodeProcessFederationObjectClassDirectedInteractionDeclarationRequest(
          ProcessFederationObjectClassDirectedInteractionDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(interactionClassHandles),
              false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::publishObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::publish_object_class_attributes,
      encodeProcessFederationObjectClassAttributeDeclarationRequest(
          ProcessFederationObjectClassAttributeDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class_attributes,
      encodeProcessFederationObjectClassAttributeDeclarationRequest(
          ProcessFederationObjectClassAttributeDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unpublishObjectClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::unpublish_object_class,
      encodeProcessFederationObjectClassAttributeDeclarationRequest(
          ProcessFederationObjectClassAttributeDeclarationRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles,
    bool active,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_attributes,
      encodeProcessFederationObjectClassAttributeSubscriptionRequest(
          ProcessFederationObjectClassAttributeSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles),
              active,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassAttributes(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> attributeHandles) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_attributes,
      encodeProcessFederationObjectClassAttributeSubscriptionRequest(
          ProcessFederationObjectClassAttributeSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributeHandles),
              false,
              {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::subscribeObjectClassAttributesWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions,
    bool active,
    std::string updateRateDesignator) {
  auto response = request(
      TransportServiceOperation::subscribe_object_class_attributes_with_regions,
      encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          ProcessFederationObjectClassAttributeRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributesAndRegions),
              active,
              std::move(updateRateDesignator)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

void ProcessFederationClient::unsubscribeObjectClassAttributesWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::unsubscribe_object_class_attributes_with_regions,
      encodeProcessFederationObjectClassAttributeRegionalSubscriptionRequest(
          ProcessFederationObjectClassAttributeRegionalSubscriptionRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(attributesAndRegions),
              false,
              {}}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
}

ProcessFederationRegisterObjectInstanceResult
ProcessFederationClient::registerObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::optional<std::wstring> requestedObjectInstanceName) {
  auto response = request(
      TransportServiceOperation::register_object_instance,
      encodeProcessFederationRegisterObjectInstanceRequest(
          ProcessFederationRegisterObjectInstanceRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(requestedObjectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterObjectInstanceResult(response.payload);
}

ProcessFederationLocalDeleteObjectInstanceResult
ProcessFederationClient::localDeleteObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) {
  auto response = request(
      TransportServiceOperation::local_delete_object_instance,
      encodeProcessFederationLocalDeleteObjectInstanceRequest(
          ProcessFederationLocalDeleteObjectInstanceRequest{
              std::move(federationName), federateId, objectInstanceHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationLocalDeleteObjectInstanceResult(
      response.payload);
}

ProcessFederationDeleteObjectInstanceResult
ProcessFederationClient::deleteObjectInstance(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::delete_object_instance,
      encodeProcessFederationDeleteObjectInstanceRequest(
          ProcessFederationDeleteObjectInstanceRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(userSuppliedTag),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDeleteObjectInstanceResult(response.payload);
}

ProcessFederationReserveObjectInstanceNameResult
ProcessFederationClient::reserveObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::reserve_object_instance_name,
      encodeProcessFederationReserveObjectInstanceNameRequest(
          ProcessFederationReserveObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReserveObjectInstanceNameResult(response.payload);
}

ProcessFederationObjectInstanceNameReleaseResult
ProcessFederationClient::releaseObjectInstanceName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::wstring objectInstanceName) {
  auto response = request(
      TransportServiceOperation::release_object_instance_name,
      encodeProcessFederationReserveObjectInstanceNameRequest(
          ProcessFederationReserveObjectInstanceNameRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceNameReleaseResult(
      response.payload);
}

ProcessFederationReserveMultipleObjectInstanceNamesResult
ProcessFederationClient::reserveMultipleObjectInstanceNames(
    std::wstring federationName,
    std::uint64_t federateId,
    std::set<std::wstring> objectInstanceNames) {
  auto response = request(
      TransportServiceOperation::reserve_multiple_object_instance_names,
      encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          ProcessFederationReserveMultipleObjectInstanceNamesRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceNames)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReserveMultipleObjectInstanceNamesResult(
      response.payload);
}

ProcessFederationReleaseMultipleObjectInstanceNamesResult
ProcessFederationClient::releaseMultipleObjectInstanceNames(
    std::wstring federationName,
    std::uint64_t federateId,
    std::set<std::wstring> objectInstanceNames) {
  auto response = request(
      TransportServiceOperation::release_multiple_object_instance_names,
      encodeProcessFederationReserveMultipleObjectInstanceNamesRequest(
          ProcessFederationReserveMultipleObjectInstanceNamesRequest{
              std::move(federationName),
              federateId,
              std::move(objectInstanceNames)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReleaseMultipleObjectInstanceNamesResult(
      response.payload);
}

std::optional<std::uint64_t> ProcessFederationClient::lookupDimensionHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string dimensionName) {
  auto response = request(
      TransportServiceOperation::get_dimension_handle,
      encodeProcessFederationGetDimensionHandleRequest(
          ProcessFederationGetDimensionHandleRequest{
              std::move(federationName), federateId, std::move(dimensionName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring> ProcessFederationClient::lookupDimensionName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_name,
      encodeProcessFederationDimensionRequest(
          ProcessFederationDimensionRequest{
              std::move(federationName), federateId, dimensionHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

std::optional<std::uint64_t>
ProcessFederationClient::lookupTransportationTypeHandle(
    std::wstring federationName,
    std::uint64_t federateId,
    std::string transportationTypeName) {
  auto response = request(
      TransportServiceOperation::get_transportation_type_handle,
      encodeProcessFederationGetTransportationTypeHandleRequest(
          ProcessFederationGetTransportationTypeHandleRequest{
              std::move(federationName), federateId,
              std::move(transportationTypeName)}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationHandleResult(response.payload).handle;
}

std::optional<std::wstring>
ProcessFederationClient::lookupTransportationTypeName(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t transportationTypeHandle) {
  auto response = request(
      TransportServiceOperation::get_transportation_type_name,
      encodeProcessFederationTransportationTypeRequest(
          ProcessFederationTransportationTypeRequest{
              std::move(federationName), federateId,
              transportationTypeHandle}));
  if (response.status == TransportServiceStatus::rejected) {
    return std::nullopt;
  }
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationStringResult(response.payload).value;
}

ProcessFederationDimensionUpperBoundResult
ProcessFederationClient::lookupDimensionUpperBound(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_upper_bound,
      encodeProcessFederationDimensionRequest(
          ProcessFederationDimensionRequest{
              std::move(federationName), federateId, dimensionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDimensionUpperBoundResult(response.payload);
}

ProcessFederationAvailableDimensionsResult
ProcessFederationClient::availableDimensionsForObjectClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) {
  auto response = request(
      TransportServiceOperation::get_available_dimensions_for_object_class,
      encodeProcessFederationClassHandleRequest(
          ProcessFederationClassHandleRequest{
              std::move(federationName), federateId, objectClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAvailableDimensionsResult(response.payload);
}

ProcessFederationAvailableDimensionsResult
ProcessFederationClient::availableDimensionsForInteractionClass(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t interactionClassHandle) {
  auto response = request(
      TransportServiceOperation::get_available_dimensions_for_interaction_class,
      encodeProcessFederationClassHandleRequest(
          ProcessFederationClassHandleRequest{
              std::move(federationName), federateId, interactionClassHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationAvailableDimensionsResult(response.payload);
}

ProcessFederationCreateRegionResult ProcessFederationClient::createRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::vector<std::uint64_t> dimensionHandles) {
  auto response = request(
      TransportServiceOperation::create_region,
      encodeProcessFederationCreateRegionRequest(
          ProcessFederationCreateRegionRequest{
              std::move(federationName), federateId, std::move(dimensionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationCreateRegionResult(response.payload);
}

ProcessFederationRegionStatusResult
ProcessFederationClient::commitRegionModifications(
    std::wstring federationName,
    std::uint64_t federateId,
    std::vector<std::uint64_t> regionHandles) {
  auto response = request(
      TransportServiceOperation::commit_region_modifications,
      encodeProcessFederationRegionSetRequest(
          ProcessFederationRegionSetRequest{
              std::move(federationName), federateId, std::move(regionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

ProcessFederationRegionStatusResult ProcessFederationClient::deleteRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  auto response = request(
      TransportServiceOperation::delete_region,
      encodeProcessFederationRegionRequest(
          ProcessFederationRegionRequest{
              std::move(federationName), federateId, regionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

ProcessFederationDimensionSetResult
ProcessFederationClient::dimensionHandleSetForRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  auto response = request(
      TransportServiceOperation::get_dimension_handle_set,
      encodeProcessFederationRegionRequest(
          ProcessFederationRegionRequest{
              std::move(federationName), federateId, regionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationDimensionSetResult(response.payload);
}

ProcessFederationRangeBoundsResult ProcessFederationClient::rangeBoundsForRegion(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle) {
  auto response = request(
      TransportServiceOperation::get_range_bounds,
      encodeProcessFederationGetRangeBoundsRequest(
          ProcessFederationGetRangeBoundsRequest{
              std::move(federationName),
              federateId,
              regionHandle,
              dimensionHandle}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRangeBoundsResult(response.payload);
}

ProcessFederationRegionStatusResult ProcessFederationClient::setRangeBounds(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle,
    unsigned long lowerBound,
    unsigned long upperBound) {
  auto response = request(
      TransportServiceOperation::set_range_bounds,
      encodeProcessFederationSetRangeBoundsRequest(
          ProcessFederationSetRangeBoundsRequest{
              std::move(federationName),
              federateId,
              regionHandle,
              dimensionHandle,
              lowerBound,
              upperBound}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegionStatusResult(response.payload);
}

bool ProcessFederationClient::getAttributeScopeAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_attribute_scope_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setAttributeScopeAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_attribute_scope_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

bool ProcessFederationClient::getAttributeRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_attribute_relevance_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

bool ProcessFederationClient::getObjectClassRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_object_class_relevance_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

bool ProcessFederationClient::getInteractionRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_interaction_relevance_advisory_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setAttributeRelevanceAdvisorySwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  // Predict the requested state before the request enters the receive loop.
  // This closes the HLA_EVOKED race in which a previously admitted advisory
  // is still waiting in the callback dispatcher while this setter disables
  // the switch. Restore the prior state if the service call fails.
  auto const previousState = attributeRelevanceAdvisorySwitchState_->exchange(
      switchValue,
      std::memory_order_acq_rel);
  try {
    auto response = request(
        TransportServiceOperation::set_attribute_relevance_advisory_switch,
        encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
            ProcessFederationAttributeScopeAdvisorySwitchRequest{
                std::move(federationName), federateId, switchValue}));
    if (response.status != TransportServiceStatus::ok) {
      throw ProcessFederationClientError(
          requestFailure(response.operation, response.status));
    }
    static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
  } catch (...) {
    attributeRelevanceAdvisorySwitchState_->store(
        previousState,
        std::memory_order_release);
    throw;
  }
}

bool ProcessFederationClient::getConveyRegionDesignatorSetsSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_convey_region_designator_sets_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

bool ProcessFederationClient::getAllowRelaxedDDMSwitch(
    std::wstring federationName,
    std::uint64_t federateId) {
  auto response = request(
      TransportServiceOperation::get_allow_relaxed_ddm_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, false}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationBooleanResult(response.payload).value;
}

void ProcessFederationClient::setConveyRegionDesignatorSetsSwitch(
    std::wstring federationName,
    std::uint64_t federateId,
    bool switchValue) {
  auto response = request(
      TransportServiceOperation::set_convey_region_designator_sets_switch,
      encodeProcessFederationAttributeScopeAdvisorySwitchRequest(
          ProcessFederationAttributeScopeAdvisorySwitchRequest{
              std::move(federationName), federateId, switchValue}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  static_cast<void>(decodeProcessFederationBooleanResult(response.payload));
}

ProcessFederationRegisterObjectInstanceResult
ProcessFederationClient::registerObjectInstanceWithRegions(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> updateRegionsByAttribute,
    std::optional<std::wstring> requestedObjectInstanceName) {
  auto response = request(
      TransportServiceOperation::register_object_instance_with_regions,
      encodeProcessFederationRegisterObjectInstanceWithRegionsRequest(
          ProcessFederationRegisterObjectInstanceWithRegionsRequest{
              std::move(federationName),
              federateId,
              objectClassHandle,
              std::move(updateRegionsByAttribute),
              std::move(requestedObjectInstanceName)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRegisterObjectInstanceResult(response.payload);
}

ProcessFederationObjectInstanceRegionAssociationResult
ProcessFederationClient::associateRegionsForUpdates(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::associate_regions_for_updates,
      encodeProcessFederationObjectInstanceRegionAssociationRequest(
          ProcessFederationObjectInstanceRegionAssociationRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(attributesAndRegions)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceRegionAssociationResult(
      response.payload);
}

ProcessFederationObjectInstanceRegionAssociationResult
ProcessFederationClient::unassociateRegionsForUpdates(
    std::wstring federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> attributesAndRegions) {
  auto response = request(
      TransportServiceOperation::unassociate_regions_for_updates,
      encodeProcessFederationObjectInstanceRegionAssociationRequest(
          ProcessFederationObjectInstanceRegionAssociationRequest{
              std::move(federationName),
              federateId,
              objectInstanceHandle,
              std::move(attributesAndRegions)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationObjectInstanceRegionAssociationResult(
      response.payload);
}

ProcessFederationSendInteractionResult ProcessFederationClient::sendInteraction(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_interaction,
      encodeProcessFederationSendInteractionRequest(
          ProcessFederationSendInteractionRequest{
              std::move(federationName),
              producingFederateId,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationSendInteractionResult
ProcessFederationClient::sendInteractionWithRegions(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::set<std::uint64_t> sentRegionHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_interaction_with_regions,
      encodeProcessFederationSendInteractionRequest(
          ProcessFederationSendInteractionRequest{
              std::move(federationName),
              producingFederateId,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp),
              std::move(sentRegionHandles)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationSendInteractionResult
ProcessFederationClient::sendDirectedInteraction(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::uint64_t interactionClassHandle,
    std::vector<std::uint64_t> sentParameterHandles,
    std::vector<std::uint8_t> payload,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::send_directed_interaction,
      encodeProcessFederationSendDirectedInteractionRequest(
          ProcessFederationSendDirectedInteractionRequest{
              std::move(federationName),
              producingFederateId,
              objectInstanceHandle,
              interactionClassHandle,
              std::move(sentParameterHandles),
              std::move(payload),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationSendInteractionResult(response.payload);
}

ProcessFederationRetractResult ProcessFederationClient::retract(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t messageId) {
  auto response = request(
      TransportServiceOperation::retract,
      encodeProcessFederationRetractRequest(
          ProcessFederationRetractRequest{
              std::move(federationName), producingFederateId, messageId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRetractResult(response.payload);
}

ProcessFederationUpdateAttributeValuesResult
ProcessFederationClient::updateAttributeValues(
    std::wstring federationName,
    std::uint64_t producingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<ProcessFederationAttributeValue> attributeValues,
    std::vector<std::uint8_t> userSuppliedTag,
    std::optional<ProcessFederationLogicalTime> timestamp) {
  auto response = request(
      TransportServiceOperation::update_attribute_values,
      encodeProcessFederationUpdateAttributeValuesRequest(
          ProcessFederationUpdateAttributeValuesRequest{
              std::move(federationName),
              producingFederateId,
              objectInstanceHandle,
              std::move(attributeValues),
              std::move(userSuppliedTag),
              std::move(timestamp)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationUpdateAttributeValuesResult(response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdate(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectInstanceHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update,
      encodeProcessFederationRequestAttributeValueUpdateRequest(
          ProcessFederationRequestAttributeValueUpdateRequest{
              std::move(federationName),
              requestingFederateId,
              objectInstanceHandle,
              std::move(requestedAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdateClass(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update_class,
      encodeProcessFederationRequestAttributeValueUpdateClassRequest(
          ProcessFederationRequestAttributeValueUpdateClassRequest{
              std::move(federationName),
              requestingFederateId,
              objectClassHandle,
              std::move(requestedAttributeHandles),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

ProcessFederationRequestAttributeValueUpdateResult
ProcessFederationClient::requestAttributeValueUpdateClassWithRegions(
    std::wstring federationName,
    std::uint64_t requestingFederateId,
    std::uint64_t objectClassHandle,
    std::vector<std::uint64_t> requestedAttributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> requestRegionsByAttribute,
    std::vector<std::uint8_t> userSuppliedTag) {
  auto response = request(
      TransportServiceOperation::request_attribute_value_update_class_with_regions,
      encodeProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest(
          ProcessFederationRequestAttributeValueUpdateClassWithRegionsRequest{
              std::move(federationName),
              requestingFederateId,
              objectClassHandle,
              std::move(requestedAttributeHandles),
              std::move(requestRegionsByAttribute),
              std::move(userSuppliedTag)}));
  if (response.status != TransportServiceStatus::ok) {
    // Semantic planner failures keep the transport rejection bit, but carry
    // a typed result so the public ambassador can preserve the official HLA
    // exception rather than collapsing every remote validation into
    // RTIinternalError.  Empty rejection payloads remain transport failures.
    if (response.status == TransportServiceStatus::rejected &&
        !response.payload.empty()) {
      return decodeProcessFederationRequestAttributeValueUpdateResult(
          response.payload);
    }
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationRequestAttributeValueUpdateResult(
      response.payload);
}

std::optional<ProcessFederationInteractionEvent>
ProcessFederationClient::receiveInteraction(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_interaction,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationReceiveInteractionResult(response.payload);
  auto event = std::move(result.event);
  result.event.reset();
  {
    std::scoped_lock lock(transactionMutex_);
    if (session_) {
      bufferReceiveResult(std::move(result));
    }
  }
  return event;
}

std::optional<ProcessFederationAttributeUpdateEvent>
ProcessFederationClient::receiveAttributeUpdate(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_attribute_update,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReceiveAttributeUpdateResult(response.payload)
      .event;
}

std::optional<ProcessFederationObjectInstanceDiscoveryEvent>
ProcessFederationClient::receiveObjectInstanceDiscovery(
    std::wstring federationName,
    std::uint64_t receivingFederateId) {
  auto response = request(
      TransportServiceOperation::receive_object_instance_discovery,
      encodeProcessFederationReceiveInteractionRequest(
          ProcessFederationReceiveInteractionRequest{
              std::move(federationName), receivingFederateId}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  return decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
             response.payload)
      .event;
}

void ProcessFederationClient::dispatchPushedReceiveOrder() {
  dispatchReceiveOrder(receivePushedEvent());
}

void ProcessFederationClient::dispatchReceiveOrder(
    ProcessFederationInteractionEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitReceiveOrder(std::move(event));
}

void ProcessFederationClient::dispatchAttributeUpdate(
    ProcessFederationAttributeUpdateEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeUpdate(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeUpdate() {
  dispatchAttributeUpdate(receivePushedAttributeUpdate());
}

void ProcessFederationClient::dispatchAttributeValueUpdateRequest(
    ProcessFederationAttributeValueUpdateRequestEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeValueUpdateRequest(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeValueUpdateRequest() {
  dispatchAttributeValueUpdateRequest(receivePendingEvent<ProcessFederationAttributeValueUpdateRequestEvent>());
}

void ProcessFederationClient::dispatchAttributeOwnershipQuery(
    ProcessFederationAttributeOwnershipQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipQuery() {
  dispatchAttributeOwnershipQuery(receivePendingEvent<ProcessFederationAttributeOwnershipQueryEvent>());
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisitionIfAvailable(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipAcquisitionIfAvailable(
      std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisitionIfAvailable() {
  dispatchAttributeOwnershipAcquisitionIfAvailable(receivePendingEvent<ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>());
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisition(
    ProcessFederationAttributeOwnershipAcquisitionEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipAcquisition(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisition() {
  dispatchAttributeOwnershipAcquisition(receivePendingEvent<ProcessFederationAttributeOwnershipAcquisitionEvent>());
}

void ProcessFederationClient::dispatchAttributeOwnershipUnavailable(
    ProcessFederationAttributeOwnershipUnavailableEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeOwnershipUnavailable(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipUnavailable() {
  dispatchAttributeOwnershipUnavailable(receivePendingEvent<ProcessFederationAttributeOwnershipUnavailableEvent>());
}

void ProcessFederationClient::dispatchObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceDiscovery(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceDiscovery() {
  dispatchObjectInstanceDiscovery(receivePushedObjectInstanceDiscovery());
}

void ProcessFederationClient::dispatchObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceRemoval(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceRemoval() {
  dispatchObjectInstanceRemoval(receivePushedObjectInstanceRemoval());
}

void ProcessFederationClient::dispatchObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitObjectInstanceScopeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceScopeChange() {
  dispatchObjectInstanceScopeChange(receivePushedObjectInstanceScopeChange());
}

void ProcessFederationClient::dispatchAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeRelevanceAdvisory(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeRelevanceAdvisory() {
  dispatchAttributeRelevanceAdvisory(
      receivePushedAttributeRelevanceAdvisory());
}

void ProcessFederationClient::dispatchAttributeTransportationTypeChange(
    ProcessFederationAttributeTransportationTypeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeChange() {
  dispatchAttributeTransportationTypeChange(receivePushedAttributeTransportationTypeChange());
}

void ProcessFederationClient::dispatchAttributeTransportationTypeQuery(
    ProcessFederationAttributeTransportationTypeQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitAttributeTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeQuery() {
  dispatchAttributeTransportationTypeQuery(receivePushedAttributeTransportationTypeQuery());
}

void ProcessFederationClient::dispatchInteractionTransportationTypeChange(
    ProcessFederationInteractionTransportationTypeChangeEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitInteractionTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeChange() {
  dispatchInteractionTransportationTypeChange(receivePushedInteractionTransportationTypeChange());
}

void ProcessFederationClient::dispatchInteractionTransportationTypeQuery(
    ProcessFederationInteractionTransportationTypeQueryEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitInteractionTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeQuery() {
  dispatchInteractionTransportationTypeQuery(receivePushedInteractionTransportationTypeQuery());
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationSucceeded(
    std::wstring label) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointRegistrationSucceeded(
      std::move(label));
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationFailed(
    std::wstring label,
    rti1516_2025::SynchronizationPointFailureReason failureReason) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointRegistrationFailed(
      std::move(label), failureReason);
}

void ProcessFederationClient::dispatchSynchronizationPointAnnouncement(
    ProcessFederationSynchronizationPointAnnouncementEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitSynchronizationPointAnnouncement(std::move(event));
}

void ProcessFederationClient::dispatchPushedSynchronizationPointAnnouncement() {
  dispatchSynchronizationPointAnnouncement(
      receivePushedSynchronizationPointAnnouncement());
}

void ProcessFederationClient::dispatchFederationSynchronized(
    ProcessFederationFederationSynchronizedEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationSynchronized(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSynchronized() {
  dispatchFederationSynchronized(receivePushedFederationSynchronized());
}

void ProcessFederationClient::dispatchFederationSave(
    ProcessFederationSaveEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationSave(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSave() {
  dispatchFederationSave(receivePushedFederationSave());
}

void ProcessFederationClient::dispatchFederationRestore(
    ProcessFederationRestoreEvent event) {
  auto bridge = requireCallbackBridge();
  bridge->submitFederationRestore(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationRestore() {
  dispatchFederationRestore(receivePushedFederationRestore());
}

void ProcessFederationClient::dispatchTimeRegulationEnabled(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeRegulationEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeConstrainedEnabled(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeConstrainedEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeAdvanceGrant(
    ProcessFederationLogicalTime event) {
  auto bridge = requireCallbackBridge();
  bridge->submitTimeAdvanceGrant(std::move(event));
}

void ProcessFederationClient::dispatchPushedTimeAdvanceGrant() {
  dispatchTimeAdvanceGrant(receivePushedTimeAdvanceGrant());
}

void ProcessFederationClient::dispatchFlushQueueGrant(
    ProcessFederationLogicalTime grantedTime,
    ProcessFederationLogicalTime optimisticTime) {
  auto bridge = requireCallbackBridge();
  bridge->submitFlushQueueGrant(
      std::move(grantedTime), std::move(optimisticTime));
}

void ProcessFederationClient::dispatchPushedFlushQueueGrant() {
  auto event = receivePushedFlushQueueGrant();
  dispatchFlushQueueGrant(
      std::move(event.grantedTime), std::move(event.optimisticTime));
}

void ProcessFederationClient::setTimeRoleEnableCompletionHandlers(
    CallbackCompletionHandler regulation,
    CallbackCompletionHandler constrained) {
  auto bridge = requireCallbackBridge();
  bridge->setTimeRoleEnableCompletionHandlers(
      std::move(regulation), std::move(constrained));
}

void ProcessFederationClient::dispatchPendingPushedEvents() {
  std::unique_lock lock(transactionMutex_);
  if (!callbackBridge_ ||
      (dispatchDepth_ != 0U && dispatchOwner_ != std::this_thread::get_id())) {
    return;
  }
  dispatchOwner_ = std::this_thread::get_id();
  ++dispatchDepth_;
  auto finish = [this] {
    if (--dispatchDepth_ == 0U) {
      dispatchOwner_ = {};
    }
  };
  try {
    while (callbackBridge_ && !pendingEvents_.empty()) {
      auto bridge = callbackBridge_;
      auto event = std::move(pendingEvents_.front());
      pendingEvents_.pop_front();
      lock.unlock();
      std::visit([&bridge](auto value) {
        using Event = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<Event, ProcessFederationInteractionEvent>) {
          bridge->submitReceiveOrder(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeUpdateEvent>) {
          bridge->submitAttributeUpdate(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeValueUpdateRequestEvent>) {
          bridge->submitAttributeValueUpdateRequest(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipQueryEvent>) {
          bridge->submitAttributeOwnershipQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>) {
          bridge->submitAttributeOwnershipAcquisitionIfAvailable(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipAcquisitionEvent>) {
          bridge->submitAttributeOwnershipAcquisition(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeOwnershipUnavailableEvent>) {
          bridge->submitAttributeOwnershipUnavailable(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceDiscoveryEvent>) {
          bridge->submitObjectInstanceDiscovery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceRemovalEvent>) {
          bridge->submitObjectInstanceRemoval(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationObjectInstanceScopeChangeEvent>) {
          bridge->submitObjectInstanceScopeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeRelevanceAdvisoryEvent>) {
          bridge->submitAttributeRelevanceAdvisory(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeTransportationTypeChangeEvent>) {
          bridge->submitAttributeTransportationTypeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationAttributeTransportationTypeQueryEvent>) {
          bridge->submitAttributeTransportationTypeQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationInteractionTransportationTypeChangeEvent>) {
          bridge->submitInteractionTransportationTypeChange(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationInteractionTransportationTypeQueryEvent>) {
          bridge->submitInteractionTransportationTypeQuery(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationSynchronizationPointAnnouncementEvent>) {
          bridge->submitSynchronizationPointAnnouncement(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationFederationSynchronizedEvent>) {
          bridge->submitFederationSynchronized(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationSaveEvent>) {
          bridge->submitFederationSave(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationRestoreEvent>) {
          bridge->submitFederationRestore(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, ProcessFederationLogicalTime>) {
          bridge->submitTimeAdvanceGrant(std::move(value));
        }
        else if constexpr (std::is_same_v<Event, PendingFlushQueueGrant>) {
          bridge->submitFlushQueueGrant(
              std::move(value.grantedTime), std::move(value.optimisticTime));
        }
        else if constexpr (std::is_same_v<Event, PendingRequestRetraction>) {
          bridge->submitRequestRetraction(value.messageId);
        }
      }, std::move(event));
      lock.lock();
    }
    finish();
  } catch (...) {
    if (!lock.owns_lock()) {
      lock.lock();
    }
    finish();
    throw;
  }
}

std::size_t ProcessFederationClient::pendingPushedEventCount() const noexcept {
  return pendingEventCount<ProcessFederationInteractionEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeUpdateCount() const noexcept {
  return pendingEventCount<ProcessFederationAttributeUpdateEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeValueUpdateRequestCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeValueUpdateRequestEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionIfAvailableCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipAcquisitionEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipUnavailableCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeOwnershipUnavailableEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceDiscoveryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceDiscoveryEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceRemovalCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceRemovalEvent>();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceScopeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationObjectInstanceScopeChangeEvent>();
}

std::size_t ProcessFederationClient::pendingPushedAttributeRelevanceAdvisoryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeRelevanceAdvisoryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeTransportationTypeChangeEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationAttributeTransportationTypeQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeChangeCount()
    const noexcept {
  return pendingEventCount<ProcessFederationInteractionTransportationTypeChangeEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeQueryCount()
    const noexcept {
  return pendingEventCount<ProcessFederationInteractionTransportationTypeQueryEvent>();
}

std::size_t
ProcessFederationClient::pendingPushedSynchronizationPointAnnouncementCount()
    const noexcept {
  return pendingEventCount<ProcessFederationSynchronizationPointAnnouncementEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationSynchronizedCount()
    const noexcept {
  return pendingEventCount<ProcessFederationFederationSynchronizedEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationSaveCount()
    const noexcept {
  return pendingEventCount<ProcessFederationSaveEvent>();
}

std::size_t ProcessFederationClient::pendingPushedFederationRestoreCount()
    const noexcept {
  return pendingEventCount<ProcessFederationRestoreEvent>();
}

std::size_t ProcessFederationClient::pendingPushedTimeAdvanceGrantCount()
    const noexcept {
  return pendingEventCount<ProcessFederationLogicalTime>();
}

std::size_t ProcessFederationClient::pendingPushedFlushQueueGrantCount()
    const noexcept {
  return pendingEventCount<PendingFlushQueueGrant>();
}

void ProcessFederationClient::attachCallbackBridge(
    rti1516_2025::FederateAmbassador& recipient,
    CallbackDispatchModel model) {
  std::scoped_lock lock(transactionMutex_);
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_shared<ProcessFederationCallbackBridge>(
      recipient, model);
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
  callbackBridge_->setExceptionReportProjectionHandler(
      [this](ProcessFederationInteractionEvent event) {
        return recheckExceptionReport(std::move(event));
      });
}

void ProcessFederationClient::attachCallbackBridge(
    std::shared_ptr<CallbackDispatcher> dispatcher,
    std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
        callbackSession) {
  std::scoped_lock lock(transactionMutex_);
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_shared<ProcessFederationCallbackBridge>(
      std::move(dispatcher), std::move(callbackSession));
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
  callbackBridge_->setExceptionReportProjectionHandler(
      [this](ProcessFederationInteractionEvent event) {
        return recheckExceptionReport(std::move(event));
      });
}

bool ProcessFederationClient::evokeOne(
    std::chrono::milliseconds minimumWait) {
  auto bridge = requireCallbackBridge();
  dispatchPendingPushedEvents();
  flushDeferredTsoDeliveryAcknowledgements();
  return bridge->evokeOne(minimumWait);
}

bool ProcessFederationClient::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  auto bridge = requireCallbackBridge();
  dispatchPendingPushedEvents();
  flushDeferredTsoDeliveryAcknowledgements();
  return bridge->evokeMultiple(minimumWait, maximumWait);
}

std::size_t ProcessFederationClient::pendingCallbackCount() const {
  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  {
    std::scoped_lock lock(transactionMutex_);
    bridge = callbackBridge_;
  }
  return bridge ? bridge->pendingCount() : 0U;
}

std::shared_ptr<ProcessTransportConnection>
ProcessFederationClient::connection() const noexcept {
  std::scoped_lock lock(transactionMutex_);
  return connection_;
}

void ProcessFederationClient::close() noexcept {
  closing_.store(true, std::memory_order_release);
  // This immutable shared owner is intentionally closed before taking the
  // transaction lock: a request may be holding that lock while blocked in a
  // socket receive, and closing the socket is what releases that request.
  auto const shutdownConnection = shutdownConnection_;
  if (shutdownConnection) {
    shutdownConnection->close();
  }

  std::shared_ptr<ProcessFederationCallbackBridge> bridge;
  std::unique_ptr<ProcessTransportSession> session;
  {
    std::scoped_lock lock(transactionMutex_);
    bridge = std::move(callbackBridge_);
    session = std::move(session_);
    connection_.reset();
    pendingEvents_.clear();
    pendingTransportFailures_.clear();
    deferredTsoDeliveryAcknowledgements_.clear();
    joinedFederationName_.reset();
    joinedFederateId_ = 0U;
  }
  // Bridge close may wait for callback-time projection/user invocation. Keep
  // that wait outside both the client exchange lock and the transport.
  if (bridge) {
    bridge->close();
  }
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

template <typename Event>
Event ProcessFederationClient::receivePendingEvent(bool receiveFromStream) {
  std::unique_lock lock(transactionMutex_);
  while (true) {
    auto found = std::find_if(pendingEvents_.begin(), pendingEvents_.end(),
        [](PendingEvent const& event) { return std::holds_alternative<Event>(event); });
    if (found != pendingEvents_.end()) {
      auto event = std::move(std::get<Event>(*found));
      pendingEvents_.erase(found);
      return event;
    }
    if (!receiveFromStream) {
      throw ProcessFederationClientError(
          "Process federation client has no queued event of the requested type.");
    }
    if (!session_) {
      throw ProcessFederationClientError(
          "A process federation event was requested after client close.");
    }
    TransportServiceMessage message;
    ++requestDepth_;
    bool received = false;
    try {
      received = session_->receive(message);
    } catch (...) {
      --requestDepth_;
      lock.unlock();
      dispatchTransportFailures();
      throw;
    }
    --requestDepth_;
    if (!received) {
      lock.unlock();
      dispatchTransportFailures();
      throw ProcessFederationClientError(
          "Process federation client lost its pushed event.");
    }
    bufferEvent(std::move(message));
  }
}

template <typename Event>
std::size_t ProcessFederationClient::pendingEventCount() const noexcept {
  std::scoped_lock lock(transactionMutex_);
  return static_cast<std::size_t>(std::count_if(
      pendingEvents_.begin(), pendingEvents_.end(),
      [](PendingEvent const& event) { return std::holds_alternative<Event>(event); }));
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
