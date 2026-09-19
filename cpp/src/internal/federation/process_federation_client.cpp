#include "internal/federation/process_federation_client.hpp"

#include <algorithm>
#include <limits>
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
  }
  return "unknown";
}

[[nodiscard]] char const* operationName(
    TransportServiceOperation operation) noexcept {
  switch (operation) {
    case TransportServiceOperation::create_federation_execution:
      return "create-federation-execution";
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
    case TransportServiceOperation::associate_regions_for_updates:
      return "associate-regions-for-updates";
    case TransportServiceOperation::unassociate_regions_for_updates:
      return "unassociate-regions-for-updates";
    case TransportServiceOperation::get_attribute_relevance_advisory_switch:
      return "get-attribute-relevance-advisory-switch";
    case TransportServiceOperation::set_attribute_relevance_advisory_switch:
      return "set-attribute-relevance-advisory-switch";
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
    : connection_(ProcessTransportConnection::connectClient(
          this,
          std::move(address),
          std::move(localIdentity),
          failureHandler ? std::move(failureHandler)
                         : FailureHandler{[](std::wstring) {}},
          forcedResignationHandler
              ? std::move(forcedResignationHandler)
              : ForcedResignationHandler{[](std::wstring) { return false; }},
          std::move(instrumentation))),
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

ProcessFederationJoinResult ProcessFederationClient::joinFederationExecution(
    std::wstring federationName,
    std::wstring federateType,
    std::optional<std::wstring> requestedFederateName,
    std::vector<std::wstring> additionalFomModules) {
  auto joinedFederationName = federationName;
  auto response = request(
      TransportServiceOperation::join_federation_execution,
      encodeProcessFederationJoinRequest(
          ProcessFederationJoinRequest{
              std::move(federationName),
              std::move(federateType),
              std::move(requestedFederateName),
              std::move(additionalFomModules)}));
  if (response.status != TransportServiceStatus::ok) {
    throw ProcessFederationClientError(
        requestFailure(response.operation, response.status));
  }
  auto result = decodeProcessFederationJoinResult(response.payload);
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
  if (joinedFederationName_ &&
      *joinedFederationName_ == resignedFederationName &&
      joinedFederateId_ == federateId) {
    joinedFederationName_.reset();
    joinedFederateId_ = 0U;
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
  if (result.attributeEvent) {
    pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
  }
  if (result.discoveryEvent) {
    pendingObjectInstanceDiscoveryEvents_.push_back(std::move(*result.discoveryEvent));
  }
  if (result.removalEvent) {
    pendingObjectInstanceRemovalEvents_.push_back(
        std::move(*result.removalEvent));
  }
  if (result.scopeChangeEvent) {
    pendingObjectInstanceScopeChangeEvents_.push_back(
        std::move(*result.scopeChangeEvent));
  }
  if (result.attributeRelevanceAdvisoryEvent) {
    pendingAttributeRelevanceAdvisoryEvents_.push_back(
        std::move(*result.attributeRelevanceAdvisoryEvent));
  }
  if (result.attributeValueUpdateRequestEvent) {
    pendingAttributeValueUpdateRequestEvents_.push_back(
        std::move(*result.attributeValueUpdateRequestEvent));
  }
  if (result.attributeOwnershipQueryEvent) {
    pendingAttributeOwnershipQueryEvents_.push_back(
        std::move(*result.attributeOwnershipQueryEvent));
  }
  if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
    pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
        std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
  }
  if (result.attributeOwnershipAcquisitionEvent) {
    pendingAttributeOwnershipAcquisitionEvents_.push_back(
        std::move(*result.attributeOwnershipAcquisitionEvent));
  }
  if (result.attributeOwnershipUnavailableEvent) {
    pendingAttributeOwnershipUnavailableEvents_.push_back(
        std::move(*result.attributeOwnershipUnavailableEvent));
  }
  if (result.attributeTransportationTypeChangeEvent) {
    pendingAttributeTransportationTypeChangeEvents_.push_back(
        std::move(*result.attributeTransportationTypeChangeEvent));
  }
  if (result.attributeTransportationTypeQueryEvent) {
    pendingAttributeTransportationTypeQueryEvents_.push_back(
        std::move(*result.attributeTransportationTypeQueryEvent));
  }
  if (result.interactionTransportationTypeChangeEvent) {
    pendingInteractionTransportationTypeChangeEvents_.push_back(
        std::move(*result.interactionTransportationTypeChangeEvent));
  }
  if (result.interactionTransportationTypeQueryEvent) {
    pendingInteractionTransportationTypeQueryEvents_.push_back(
        std::move(*result.interactionTransportationTypeQueryEvent));
  }
  if (result.synchronizationPointAnnouncementEvent) {
    pendingSynchronizationPointAnnouncementEvents_.push_back(
        std::move(*result.synchronizationPointAnnouncementEvent));
  }
  if (result.federationSynchronizedEvent) {
    pendingFederationSynchronizedEvents_.push_back(
        std::move(*result.federationSynchronizedEvent));
  }
  if (result.saveEvent) {
    pendingFederationSaveEvents_.push_back(std::move(*result.saveEvent));
  }
  if (result.restoreEvent) {
    pendingFederationRestoreEvents_.push_back(std::move(*result.restoreEvent));
  }
  return std::move(result.event);
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
  requireCallbackBridge();
  callbackBridge_->submitReceiveOrder(std::move(event));
}

void ProcessFederationClient::dispatchAttributeUpdate(
    ProcessFederationAttributeUpdateEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeUpdate(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeUpdate() {
  dispatchAttributeUpdate(receivePushedAttributeUpdate());
}

void ProcessFederationClient::dispatchAttributeValueUpdateRequest(
    ProcessFederationAttributeValueUpdateRequestEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeValueUpdateRequest(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeValueUpdateRequest() {
  if (pendingAttributeValueUpdateRequestEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute-value-update request event.");
  }
  auto event = std::move(pendingAttributeValueUpdateRequestEvents_.front());
  pendingAttributeValueUpdateRequestEvents_.pop_front();
  dispatchAttributeValueUpdateRequest(std::move(event));
}

void ProcessFederationClient::dispatchAttributeOwnershipQuery(
    ProcessFederationAttributeOwnershipQueryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeOwnershipQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipQuery() {
  if (pendingAttributeOwnershipQueryEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute-ownership query event.");
  }
  auto event = std::move(pendingAttributeOwnershipQueryEvents_.front());
  pendingAttributeOwnershipQueryEvents_.pop_front();
  dispatchAttributeOwnershipQuery(std::move(event));
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisitionIfAvailable(
    ProcessFederationAttributeOwnershipAcquisitionIfAvailableEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeOwnershipAcquisitionIfAvailable(
      std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisitionIfAvailable() {
  if (pendingAttributeOwnershipAcquisitionIfAvailableEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute-ownership acquisition-if-available event.");
  }
  auto event = std::move(
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_.front());
  pendingAttributeOwnershipAcquisitionIfAvailableEvents_.pop_front();
  dispatchAttributeOwnershipAcquisitionIfAvailable(std::move(event));
}

void ProcessFederationClient::dispatchAttributeOwnershipAcquisition(
    ProcessFederationAttributeOwnershipAcquisitionEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeOwnershipAcquisition(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipAcquisition() {
  if (pendingAttributeOwnershipAcquisitionEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute-ownership acquisition event.");
  }
  auto event = std::move(pendingAttributeOwnershipAcquisitionEvents_.front());
  pendingAttributeOwnershipAcquisitionEvents_.pop_front();
  dispatchAttributeOwnershipAcquisition(std::move(event));
}

void ProcessFederationClient::dispatchAttributeOwnershipUnavailable(
    ProcessFederationAttributeOwnershipUnavailableEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeOwnershipUnavailable(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeOwnershipUnavailable() {
  if (pendingAttributeOwnershipUnavailableEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute-ownership unavailable event.");
  }
  auto event = std::move(pendingAttributeOwnershipUnavailableEvents_.front());
  pendingAttributeOwnershipUnavailableEvents_.pop_front();
  dispatchAttributeOwnershipUnavailable(std::move(event));
}

void ProcessFederationClient::dispatchObjectInstanceDiscovery(
    ProcessFederationObjectInstanceDiscoveryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceDiscovery(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceDiscovery() {
  dispatchObjectInstanceDiscovery(receivePushedObjectInstanceDiscovery());
}

void ProcessFederationClient::dispatchObjectInstanceRemoval(
    ProcessFederationObjectInstanceRemovalEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceRemoval(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceRemoval() {
  dispatchObjectInstanceRemoval(receivePushedObjectInstanceRemoval());
}

void ProcessFederationClient::dispatchObjectInstanceScopeChange(
    ProcessFederationObjectInstanceScopeChangeEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitObjectInstanceScopeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedObjectInstanceScopeChange() {
  dispatchObjectInstanceScopeChange(receivePushedObjectInstanceScopeChange());
}

void ProcessFederationClient::dispatchAttributeRelevanceAdvisory(
    ProcessFederationAttributeRelevanceAdvisoryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeRelevanceAdvisory(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeRelevanceAdvisory() {
  dispatchAttributeRelevanceAdvisory(
      receivePushedAttributeRelevanceAdvisory());
}

void ProcessFederationClient::dispatchAttributeTransportationTypeChange(
    ProcessFederationAttributeTransportationTypeChangeEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeChange() {
  if (pendingAttributeTransportationTypeChangeEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute transportation-type change event.");
  }
  auto event = std::move(
      pendingAttributeTransportationTypeChangeEvents_.front());
  pendingAttributeTransportationTypeChangeEvents_.pop_front();
  dispatchAttributeTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchAttributeTransportationTypeQuery(
    ProcessFederationAttributeTransportationTypeQueryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitAttributeTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedAttributeTransportationTypeQuery() {
  if (pendingAttributeTransportationTypeQueryEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute transportation-type query event.");
  }
  auto event = std::move(pendingAttributeTransportationTypeQueryEvents_.front());
  pendingAttributeTransportationTypeQueryEvents_.pop_front();
  dispatchAttributeTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchInteractionTransportationTypeChange(
    ProcessFederationInteractionTransportationTypeChangeEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitInteractionTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeChange() {
  if (pendingInteractionTransportationTypeChangeEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued interaction transportation-type change event.");
  }
  auto event = std::move(
      pendingInteractionTransportationTypeChangeEvents_.front());
  pendingInteractionTransportationTypeChangeEvents_.pop_front();
  dispatchInteractionTransportationTypeChange(std::move(event));
}

void ProcessFederationClient::dispatchInteractionTransportationTypeQuery(
    ProcessFederationInteractionTransportationTypeQueryEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitInteractionTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchPushedInteractionTransportationTypeQuery() {
  if (pendingInteractionTransportationTypeQueryEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued interaction transportation-type query event.");
  }
  auto event = std::move(
      pendingInteractionTransportationTypeQueryEvents_.front());
  pendingInteractionTransportationTypeQueryEvents_.pop_front();
  dispatchInteractionTransportationTypeQuery(std::move(event));
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationSucceeded(
    std::wstring label) {
  requireCallbackBridge();
  callbackBridge_->submitSynchronizationPointRegistrationSucceeded(
      std::move(label));
}

void ProcessFederationClient::dispatchSynchronizationPointRegistrationFailed(
    std::wstring label,
    rti1516_2025::SynchronizationPointFailureReason failureReason) {
  requireCallbackBridge();
  callbackBridge_->submitSynchronizationPointRegistrationFailed(
      std::move(label), failureReason);
}

void ProcessFederationClient::dispatchSynchronizationPointAnnouncement(
    ProcessFederationSynchronizationPointAnnouncementEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitSynchronizationPointAnnouncement(std::move(event));
}

void ProcessFederationClient::dispatchPushedSynchronizationPointAnnouncement() {
  dispatchSynchronizationPointAnnouncement(
      receivePushedSynchronizationPointAnnouncement());
}

void ProcessFederationClient::dispatchFederationSynchronized(
    ProcessFederationFederationSynchronizedEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitFederationSynchronized(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSynchronized() {
  dispatchFederationSynchronized(receivePushedFederationSynchronized());
}

void ProcessFederationClient::dispatchFederationSave(
    ProcessFederationSaveEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitFederationSave(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationSave() {
  dispatchFederationSave(receivePushedFederationSave());
}

void ProcessFederationClient::dispatchFederationRestore(
    ProcessFederationRestoreEvent event) {
  requireCallbackBridge();
  callbackBridge_->submitFederationRestore(std::move(event));
}

void ProcessFederationClient::dispatchPushedFederationRestore() {
  dispatchFederationRestore(receivePushedFederationRestore());
}

void ProcessFederationClient::dispatchTimeRegulationEnabled(
    ProcessFederationLogicalTime event) {
  requireCallbackBridge();
  callbackBridge_->submitTimeRegulationEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeConstrainedEnabled(
    ProcessFederationLogicalTime event) {
  requireCallbackBridge();
  callbackBridge_->submitTimeConstrainedEnabled(std::move(event));
}

void ProcessFederationClient::dispatchTimeAdvanceGrant(
    ProcessFederationLogicalTime event) {
  requireCallbackBridge();
  callbackBridge_->submitTimeAdvanceGrant(std::move(event));
}

void ProcessFederationClient::dispatchPushedTimeAdvanceGrant() {
  dispatchTimeAdvanceGrant(receivePushedTimeAdvanceGrant());
}

void ProcessFederationClient::dispatchFlushQueueGrant(
    ProcessFederationLogicalTime grantedTime,
    ProcessFederationLogicalTime optimisticTime) {
  requireCallbackBridge();
  callbackBridge_->submitFlushQueueGrant(
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
  requireCallbackBridge();
  callbackBridge_->setTimeRoleEnableCompletionHandlers(
      std::move(regulation), std::move(constrained));
}

void ProcessFederationClient::dispatchPendingPushedEvents() {
  if (!callbackBridge_) {
    return;
  }
  while (!pendingEvents_.empty()) {
    dispatchPushedReceiveOrder();
  }
  while (!pendingObjectInstanceDiscoveryEvents_.empty()) {
    dispatchPushedObjectInstanceDiscovery();
  }
  while (!pendingObjectInstanceRemovalEvents_.empty()) {
    dispatchPushedObjectInstanceRemoval();
  }
  while (!pendingObjectInstanceScopeChangeEvents_.empty()) {
    dispatchPushedObjectInstanceScopeChange();
  }
  while (!pendingAttributeRelevanceAdvisoryEvents_.empty()) {
    dispatchPushedAttributeRelevanceAdvisory();
  }
  while (!pendingAttributeTransportationTypeChangeEvents_.empty()) {
    dispatchPushedAttributeTransportationTypeChange();
  }
  while (!pendingAttributeTransportationTypeQueryEvents_.empty()) {
    dispatchPushedAttributeTransportationTypeQuery();
  }
  while (!pendingInteractionTransportationTypeChangeEvents_.empty()) {
    dispatchPushedInteractionTransportationTypeChange();
  }
  while (!pendingInteractionTransportationTypeQueryEvents_.empty()) {
    dispatchPushedInteractionTransportationTypeQuery();
  }
  while (!pendingAttributeUpdateEvents_.empty()) {
    dispatchPushedAttributeUpdate();
  }
  while (!pendingAttributeValueUpdateRequestEvents_.empty()) {
    dispatchPushedAttributeValueUpdateRequest();
  }
  while (!pendingAttributeOwnershipQueryEvents_.empty()) {
    dispatchPushedAttributeOwnershipQuery();
  }
  while (!pendingAttributeOwnershipAcquisitionIfAvailableEvents_.empty()) {
    dispatchPushedAttributeOwnershipAcquisitionIfAvailable();
  }
  while (!pendingAttributeOwnershipAcquisitionEvents_.empty()) {
    dispatchPushedAttributeOwnershipAcquisition();
  }
  while (!pendingAttributeOwnershipUnavailableEvents_.empty()) {
    dispatchPushedAttributeOwnershipUnavailable();
  }
  while (!pendingSynchronizationPointAnnouncementEvents_.empty()) {
    dispatchPushedSynchronizationPointAnnouncement();
  }
  while (!pendingFederationSynchronizedEvents_.empty()) {
    dispatchPushedFederationSynchronized();
  }
  while (!pendingFederationSaveEvents_.empty()) {
    dispatchPushedFederationSave();
  }
  while (!pendingFederationRestoreEvents_.empty()) {
    dispatchPushedFederationRestore();
  }
  while (!pendingTimeAdvanceGrantEvents_.empty()) {
    dispatchPushedTimeAdvanceGrant();
  }
  while (!pendingFlushQueueGrantEvents_.empty()) {
    dispatchPushedFlushQueueGrant();
  }
}

std::size_t ProcessFederationClient::pendingPushedEventCount() const noexcept {
  return pendingEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeUpdateCount() const noexcept {
  return pendingAttributeUpdateEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeValueUpdateRequestCount()
    const noexcept {
  return pendingAttributeValueUpdateRequestEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipQueryCount()
    const noexcept {
  return pendingAttributeOwnershipQueryEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionIfAvailableCount()
    const noexcept {
  return pendingAttributeOwnershipAcquisitionIfAvailableEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeOwnershipAcquisitionCount()
    const noexcept {
  return pendingAttributeOwnershipAcquisitionEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeOwnershipUnavailableCount()
    const noexcept {
  return pendingAttributeOwnershipUnavailableEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceDiscoveryCount()
    const noexcept {
  return pendingObjectInstanceDiscoveryEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceRemovalCount()
    const noexcept {
  return pendingObjectInstanceRemovalEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedObjectInstanceScopeChangeCount()
    const noexcept {
  return pendingObjectInstanceScopeChangeEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedAttributeRelevanceAdvisoryCount()
    const noexcept {
  return pendingAttributeRelevanceAdvisoryEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeChangeCount()
    const noexcept {
  return pendingAttributeTransportationTypeChangeEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedAttributeTransportationTypeQueryCount()
    const noexcept {
  return pendingAttributeTransportationTypeQueryEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeChangeCount()
    const noexcept {
  return pendingInteractionTransportationTypeChangeEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedInteractionTransportationTypeQueryCount()
    const noexcept {
  return pendingInteractionTransportationTypeQueryEvents_.size();
}

std::size_t
ProcessFederationClient::pendingPushedSynchronizationPointAnnouncementCount()
    const noexcept {
  return pendingSynchronizationPointAnnouncementEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedFederationSynchronizedCount()
    const noexcept {
  return pendingFederationSynchronizedEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedFederationSaveCount()
    const noexcept {
  return pendingFederationSaveEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedFederationRestoreCount()
    const noexcept {
  return pendingFederationRestoreEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedTimeAdvanceGrantCount()
    const noexcept {
  return pendingTimeAdvanceGrantEvents_.size();
}

std::size_t ProcessFederationClient::pendingPushedFlushQueueGrantCount()
    const noexcept {
  return pendingFlushQueueGrantEvents_.size();
}

void ProcessFederationClient::attachCallbackBridge(
    rti1516_2025::FederateAmbassador& recipient,
    CallbackDispatchModel model) {
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_unique<ProcessFederationCallbackBridge>(
      recipient, model);
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
}

void ProcessFederationClient::attachCallbackBridge(
    std::shared_ptr<CallbackDispatcher> dispatcher,
    std::shared_ptr<rti1516_2025::umbra_binding_detail::CallbackSession>
        callbackSession) {
  if (callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge is already attached.");
  }
  callbackBridge_ = std::make_unique<ProcessFederationCallbackBridge>(
      std::move(dispatcher), std::move(callbackSession));
  callbackBridge_->setAttributeRelevanceAdvisorySwitchState(
      attributeRelevanceAdvisorySwitchState_);
  callbackBridge_->setTsoDeliveryCompletionHandler(
      [this](std::uint64_t messageId) { acknowledgeTsoDelivery(messageId); });
}

bool ProcessFederationClient::evokeOne(
    std::chrono::milliseconds minimumWait) {
  requireCallbackBridge();
  return callbackBridge_->evokeOne(minimumWait);
}

bool ProcessFederationClient::evokeMultiple(
    std::chrono::milliseconds minimumWait,
    std::chrono::milliseconds maximumWait) {
  requireCallbackBridge();
  return callbackBridge_->evokeMultiple(minimumWait, maximumWait);
}

std::size_t ProcessFederationClient::pendingCallbackCount() const {
  return callbackBridge_ ? callbackBridge_->pendingCount() : 0U;
}

std::shared_ptr<ProcessTransportConnection>
ProcessFederationClient::connection() const noexcept {
  return connection_;
}

void ProcessFederationClient::close() noexcept {
  if (callbackBridge_) {
    callbackBridge_->close();
    callbackBridge_.reset();
  }
  session_.reset();
  if (connection_) {
    connection_->close();
    connection_.reset();
  }
  pendingEvents_.clear();
  pendingAttributeUpdateEvents_.clear();
  pendingObjectInstanceDiscoveryEvents_.clear();
  pendingObjectInstanceRemovalEvents_.clear();
  pendingObjectInstanceScopeChangeEvents_.clear();
  pendingAttributeRelevanceAdvisoryEvents_.clear();
  pendingAttributeTransportationTypeChangeEvents_.clear();
  pendingAttributeTransportationTypeQueryEvents_.clear();
  pendingInteractionTransportationTypeChangeEvents_.clear();
  pendingInteractionTransportationTypeQueryEvents_.clear();
  pendingAttributeValueUpdateRequestEvents_.clear();
  pendingAttributeOwnershipQueryEvents_.clear();
  pendingAttributeOwnershipAcquisitionIfAvailableEvents_.clear();
  pendingAttributeOwnershipAcquisitionEvents_.clear();
  pendingAttributeOwnershipUnavailableEvents_.clear();
  pendingSynchronizationPointAnnouncementEvents_.clear();
  pendingFederationSynchronizedEvents_.clear();
  pendingFederationSaveEvents_.clear();
  pendingFederationRestoreEvents_.clear();
  pendingTimeAdvanceGrantEvents_.clear();
  pendingFlushQueueGrantEvents_.clear();
  deferredTsoDeliveryAcknowledgements_.clear();
  joinedFederationName_.reset();
  joinedFederateId_ = 0U;
}

TransportServiceMessage ProcessFederationClient::request(
    TransportServiceOperation operation,
    std::vector<std::uint8_t> payload) {
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation request was attempted after client close.");
  }
  ++requestDepth_;
  bool requestActive = true;
  auto releaseRequest = [this, &requestActive]() noexcept {
    if (requestActive) {
      --requestDepth_;
      requestActive = false;
    }
  };
  try {
    if (nextRequestId_ == std::numeric_limits<std::uint64_t>::max()) {
      throw ProcessFederationClientError(
          "The process federation request identity space is exhausted.");
    }
    auto const requestId = ++nextRequestId_;
    TransportServiceMessage requestMessage{
        TransportServiceMessageKind::request,
        operation,
        TransportServiceStatus::ok,
        requestId,
        std::move(payload)};
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
        if (response.status != TransportServiceStatus::ok ||
            response.requestId != 0U ||
            (response.operation != TransportServiceOperation::receive_interaction &&
             response.operation != TransportServiceOperation::receive_attribute_update &&
             response.operation !=
                 TransportServiceOperation::receive_object_instance_discovery &&
             response.operation != TransportServiceOperation::request_retraction &&
             response.operation != TransportServiceOperation::time_advance_grant)) {
          throw ProcessFederationClientError(
              "Process federation client received an invalid unsolicited event.");
        }
        if (response.operation == TransportServiceOperation::request_retraction) {
          auto const retraction =
              decodeProcessFederationRequestRetractionEvent(response.payload);
          auto const pendingEventsBefore = pendingEvents_.size();
          auto const pendingRemovalsBefore =
              pendingObjectInstanceRemovalEvents_.size();
          pendingEvents_.erase(
              std::remove_if(
                  pendingEvents_.begin(),
                  pendingEvents_.end(),
                  [&retraction](ProcessFederationInteractionEvent const& event) {
                    return event.retractionMessageId == retraction.messageId;
                  }),
              pendingEvents_.end());
          pendingObjectInstanceRemovalEvents_.erase(
              std::remove_if(
                  pendingObjectInstanceRemovalEvents_.begin(),
                  pendingObjectInstanceRemovalEvents_.end(),
                  [&retraction](
                      ProcessFederationObjectInstanceRemovalEvent const& event) {
                    return event.retractionMessageId == retraction.messageId;
                  }),
              pendingObjectInstanceRemovalEvents_.end());
          auto const removedFromPending =
              pendingEvents_.size() != pendingEventsBefore ||
              pendingObjectInstanceRemovalEvents_.size() != pendingRemovalsBefore;
          if (removedFromPending) {
            deferTsoDeliveryAcknowledgement(retraction.messageId);
          } else if (callbackBridge_) {
            callbackBridge_->submitRequestRetraction(retraction.messageId);
          }
        } else if (response.operation == TransportServiceOperation::time_advance_grant) {
          auto const result =
              decodeProcessFederationTimeAdvanceResult(response.payload);
          if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
              !result.grantedTime) {
            throw ProcessFederationClientError(
                "Process federation client received an invalid time-advance grant event.");
          }
          if (result.optimisticTime) {
            pendingFlushQueueGrantEvents_.push_back(
                PendingFlushQueueGrant{
                    std::move(*result.grantedTime),
                    std::move(*result.optimisticTime)});
          } else {
            pendingTimeAdvanceGrantEvents_.push_back(
                std::move(*result.grantedTime));
          }
        } else if (response.operation == TransportServiceOperation::receive_interaction) {
        auto event = decodeProcessFederationReceiveInteractionResult(response.payload);
        if (!event.event && !event.removalEvent && !event.scopeChangeEvent &&
            !event.attributeRelevanceAdvisoryEvent &&
            !event.attributeValueUpdateRequestEvent &&
            !event.attributeOwnershipQueryEvent &&
            !event.attributeOwnershipAcquisitionIfAvailableEvent &&
            !event.attributeOwnershipAcquisitionEvent &&
            !event.attributeOwnershipUnavailableEvent &&
            !event.attributeTransportationTypeChangeEvent &&
            !event.attributeTransportationTypeQueryEvent &&
            !event.interactionTransportationTypeChangeEvent &&
            !event.interactionTransportationTypeQueryEvent &&
            !event.synchronizationPointAnnouncementEvent &&
            !event.federationSynchronizedEvent &&
            !event.saveEvent &&
            !event.restoreEvent) {
          throw ProcessFederationClientError(
              "Process federation client received an empty unsolicited event.");
        }
        if (event.event) {
          pendingEvents_.push_back(std::move(*event.event));
        }
        if (event.removalEvent) {
          pendingObjectInstanceRemovalEvents_.push_back(
              std::move(*event.removalEvent));
        }
        if (event.scopeChangeEvent) {
          pendingObjectInstanceScopeChangeEvents_.push_back(
              std::move(*event.scopeChangeEvent));
        }
        if (event.attributeRelevanceAdvisoryEvent) {
          pendingAttributeRelevanceAdvisoryEvents_.push_back(
              std::move(*event.attributeRelevanceAdvisoryEvent));
        }
        if (event.attributeValueUpdateRequestEvent) {
          pendingAttributeValueUpdateRequestEvents_.push_back(
              std::move(*event.attributeValueUpdateRequestEvent));
        }
        if (event.attributeOwnershipQueryEvent) {
          pendingAttributeOwnershipQueryEvents_.push_back(
              std::move(*event.attributeOwnershipQueryEvent));
        }
        if (event.attributeOwnershipAcquisitionIfAvailableEvent) {
          pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
              std::move(*event.attributeOwnershipAcquisitionIfAvailableEvent));
        }
        if (event.attributeOwnershipAcquisitionEvent) {
          pendingAttributeOwnershipAcquisitionEvents_.push_back(
              std::move(*event.attributeOwnershipAcquisitionEvent));
        }
        if (event.attributeOwnershipUnavailableEvent) {
          pendingAttributeOwnershipUnavailableEvents_.push_back(
              std::move(*event.attributeOwnershipUnavailableEvent));
        }
        if (event.attributeTransportationTypeChangeEvent) {
          pendingAttributeTransportationTypeChangeEvents_.push_back(
              std::move(*event.attributeTransportationTypeChangeEvent));
        }
        if (event.attributeTransportationTypeQueryEvent) {
          pendingAttributeTransportationTypeQueryEvents_.push_back(
              std::move(*event.attributeTransportationTypeQueryEvent));
        }
        if (event.interactionTransportationTypeChangeEvent) {
          pendingInteractionTransportationTypeChangeEvents_.push_back(
              std::move(*event.interactionTransportationTypeChangeEvent));
        }
        if (event.interactionTransportationTypeQueryEvent) {
          pendingInteractionTransportationTypeQueryEvents_.push_back(
              std::move(*event.interactionTransportationTypeQueryEvent));
        }
        if (event.synchronizationPointAnnouncementEvent) {
          pendingSynchronizationPointAnnouncementEvents_.push_back(
              std::move(*event.synchronizationPointAnnouncementEvent));
        }
        if (event.federationSynchronizedEvent) {
          pendingFederationSynchronizedEvents_.push_back(
              std::move(*event.federationSynchronizedEvent));
        }
        if (event.saveEvent) {
          pendingFederationSaveEvents_.push_back(std::move(*event.saveEvent));
        }
        if (event.restoreEvent) {
          pendingFederationRestoreEvents_.push_back(
              std::move(*event.restoreEvent));
        }
        } else if (response.operation ==
                   TransportServiceOperation::receive_attribute_update) {
          auto event = decodeProcessFederationReceiveAttributeUpdateResult(
              response.payload);
          if (!event.event) {
          throw ProcessFederationClientError(
              "Process federation client received an empty unsolicited attribute event.");
          }
          pendingAttributeUpdateEvents_.push_back(std::move(*event.event));
        } else {
          auto event =
              decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
                  response.payload);
          if (!event.event) {
            throw ProcessFederationClientError(
                "Process federation client received an empty unsolicited discovery event.");
          }
          pendingObjectInstanceDiscoveryEvents_.push_back(
              std::move(*event.event));
        }
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
      // A service response can be preceded by unsolicited frames emitted for a
      // different joined federate.  Do not leave those frames stranded in the
      // private queues: HLA_IMMEDIATE must invoke them before this public RTI
      // service returns, while HLA_EVOKED still merely queues them for Evoke.
      dispatchPendingPushedEvents();
      // Callback completion can request a TSO acknowledgement.  Release the
      // outer request before flushing so the acknowledgement is a normal
      // follow-up request rather than a nested request on the same stream.
      releaseRequest();
      flushDeferredTsoDeliveryAcknowledgements();
      return response;
    }
  } catch (...) {
    releaseRequest();
    throw;
  }
}

ProcessFederationLogicalTime ProcessFederationClient::receivePushedTimeAdvanceGrant() {
  if (!pendingTimeAdvanceGrantEvents_.empty()) {
    auto event = std::move(pendingTimeAdvanceGrantEvents_.front());
    pendingTimeAdvanceGrantEvents_.pop_front();
    return event;
  }
  throw ProcessFederationClientError(
      "Process federation client has no queued time-advance grant event.");
}

ProcessFederationClient::PendingFlushQueueGrant
ProcessFederationClient::receivePushedFlushQueueGrant() {
  if (pendingFlushQueueGrantEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued Flush Queue Grant event.");
  }
  auto event = std::move(pendingFlushQueueGrantEvents_.front());
  pendingFlushQueueGrantEvents_.pop_front();
  return event;
}

void ProcessFederationClient::acknowledgeTsoDelivery(
    std::uint64_t messageId) {
  if (messageId == 0U) {
    throw ProcessFederationClientError(
        "A process TSO delivery acknowledgement requires a message identity.");
  }
  if (requestDepth_ != 0U) {
    deferTsoDeliveryAcknowledgement(messageId);
    return;
  }
  if (!joinedFederationName_ || joinedFederateId_ == 0U) {
    // A queued callback can be discarded during close/resignation.  There is
    // no live joined-federate identity left on which to send a private ACK.
    return;
  }
  auto response = request(
      TransportServiceOperation::acknowledge_tso_delivery,
      encodeProcessFederationAcknowledgeTsoDeliveryRequest(
          ProcessFederationAcknowledgeTsoDeliveryRequest{
              *joinedFederationName_, joinedFederateId_, messageId}));
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
  while (!deferredTsoDeliveryAcknowledgements_.empty()) {
    auto const messageId = deferredTsoDeliveryAcknowledgements_.front();
    deferredTsoDeliveryAcknowledgements_.pop_front();
    acknowledgeTsoDelivery(messageId);
  }
}

ProcessFederationInteractionEvent ProcessFederationClient::receivePushedEvent() {
  if (!pendingEvents_.empty()) {
    auto event = std::move(pendingEvents_.front());
    pendingEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::request_retraction &&
         message.operation != TransportServiceOperation::time_advance_grant)) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed message.");
    }
    if (message.operation == TransportServiceOperation::request_retraction) {
      auto const retraction =
          decodeProcessFederationRequestRetractionEvent(message.payload);
      auto const pendingEventsBefore = pendingEvents_.size();
      auto const pendingRemovalsBefore =
          pendingObjectInstanceRemovalEvents_.size();
      pendingEvents_.erase(
          std::remove_if(
              pendingEvents_.begin(),
              pendingEvents_.end(),
              [&retraction](ProcessFederationInteractionEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingEvents_.end());
      pendingObjectInstanceRemovalEvents_.erase(
          std::remove_if(
              pendingObjectInstanceRemovalEvents_.begin(),
              pendingObjectInstanceRemovalEvents_.end(),
              [&retraction](
                  ProcessFederationObjectInstanceRemovalEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingObjectInstanceRemovalEvents_.end());
      auto const removedFromPending =
          pendingEvents_.size() != pendingEventsBefore ||
          pendingObjectInstanceRemovalEvents_.size() != pendingRemovalsBefore;
      if (removedFromPending) {
        acknowledgeTsoDelivery(retraction.messageId);
      } else if (callbackBridge_) {
        callbackBridge_->submitRequestRetraction(retraction.messageId);
      }
      continue;
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (!result.event && !result.removalEvent && !result.scopeChangeEvent &&
        !result.attributeRelevanceAdvisoryEvent && !result.attributeEvent &&
        !result.discoveryEvent && !result.attributeValueUpdateRequestEvent &&
        !result.attributeOwnershipQueryEvent &&
        !result.attributeOwnershipAcquisitionIfAvailableEvent &&
        !result.attributeOwnershipAcquisitionEvent &&
        !result.attributeOwnershipUnavailableEvent &&
        !result.attributeTransportationTypeChangeEvent &&
        !result.attributeTransportationTypeQueryEvent &&
        !result.interactionTransportationTypeChangeEvent &&
        !result.interactionTransportationTypeQueryEvent) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed event.");
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
    if (result.attributeValueUpdateRequestEvent) {
      pendingAttributeValueUpdateRequestEvents_.push_back(
          std::move(*result.attributeValueUpdateRequestEvent));
    }
    if (result.attributeOwnershipQueryEvent) {
      pendingAttributeOwnershipQueryEvents_.push_back(
          std::move(*result.attributeOwnershipQueryEvent));
    }
    if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
    }
    if (result.attributeOwnershipAcquisitionEvent) {
      pendingAttributeOwnershipAcquisitionEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionEvent));
    }
    if (result.attributeOwnershipUnavailableEvent) {
      pendingAttributeOwnershipUnavailableEvents_.push_back(
          std::move(*result.attributeOwnershipUnavailableEvent));
    }
    if (result.attributeTransportationTypeChangeEvent) {
      pendingAttributeTransportationTypeChangeEvents_.push_back(
          std::move(*result.attributeTransportationTypeChangeEvent));
    }
    if (result.attributeTransportationTypeQueryEvent) {
      pendingAttributeTransportationTypeQueryEvents_.push_back(
          std::move(*result.attributeTransportationTypeQueryEvent));
    }
    if (result.interactionTransportationTypeChangeEvent) {
      pendingInteractionTransportationTypeChangeEvents_.push_back(
          std::move(*result.interactionTransportationTypeChangeEvent));
    }
    if (result.interactionTransportationTypeQueryEvent) {
      pendingInteractionTransportationTypeQueryEvents_.push_back(
          std::move(*result.interactionTransportationTypeQueryEvent));
    }
    if (result.event) {
      return std::move(*result.event);
    }
  }
}

ProcessFederationObjectInstanceRemovalEvent
ProcessFederationClient::receivePushedObjectInstanceRemoval() {
  if (!pendingObjectInstanceRemovalEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceRemovalEvents_.front());
    pendingObjectInstanceRemovalEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation removal event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed removal event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::request_retraction &&
         message.operation != TransportServiceOperation::time_advance_grant)) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed removal event.");
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    if (message.operation == TransportServiceOperation::request_retraction) {
      auto const retraction =
          decodeProcessFederationRequestRetractionEvent(message.payload);
      auto const pendingBefore = pendingObjectInstanceRemovalEvents_.size();
      pendingObjectInstanceRemovalEvents_.erase(
          std::remove_if(
              pendingObjectInstanceRemovalEvents_.begin(),
              pendingObjectInstanceRemovalEvents_.end(),
              [&retraction](
                  ProcessFederationObjectInstanceRemovalEvent const& event) {
                return event.retractionMessageId == retraction.messageId;
              }),
          pendingObjectInstanceRemovalEvents_.end());
      if (pendingObjectInstanceRemovalEvents_.size() != pendingBefore) {
        acknowledgeTsoDelivery(retraction.messageId);
      } else if (callbackBridge_) {
        callbackBridge_->submitRequestRetraction(retraction.messageId);
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.removalEvent) {
      if (result.event) {
        pendingEvents_.push_back(std::move(*result.event));
      }
      if (result.scopeChangeEvent) {
        pendingObjectInstanceScopeChangeEvents_.push_back(
            std::move(*result.scopeChangeEvent));
      }
      if (result.attributeRelevanceAdvisoryEvent) {
        pendingAttributeRelevanceAdvisoryEvents_.push_back(
            std::move(*result.attributeRelevanceAdvisoryEvent));
      }
      if (result.attributeEvent) {
        pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
      }
      if (result.discoveryEvent) {
        pendingObjectInstanceDiscoveryEvents_.push_back(
            std::move(*result.discoveryEvent));
      }
      if (result.attributeValueUpdateRequestEvent) {
        pendingAttributeValueUpdateRequestEvents_.push_back(
            std::move(*result.attributeValueUpdateRequestEvent));
      }
      if (result.attributeOwnershipQueryEvent) {
        pendingAttributeOwnershipQueryEvents_.push_back(
            std::move(*result.attributeOwnershipQueryEvent));
      }
      if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
        pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
            std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
      }
      if (result.attributeOwnershipAcquisitionEvent) {
        pendingAttributeOwnershipAcquisitionEvents_.push_back(
            std::move(*result.attributeOwnershipAcquisitionEvent));
      }
      if (result.attributeOwnershipUnavailableEvent) {
        pendingAttributeOwnershipUnavailableEvents_.push_back(
            std::move(*result.attributeOwnershipUnavailableEvent));
      }
      return std::move(*result.removalEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
    if (result.attributeValueUpdateRequestEvent) {
      pendingAttributeValueUpdateRequestEvents_.push_back(
          std::move(*result.attributeValueUpdateRequestEvent));
    }
    if (result.attributeOwnershipQueryEvent) {
      pendingAttributeOwnershipQueryEvents_.push_back(
          std::move(*result.attributeOwnershipQueryEvent));
    }
    if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
    }
    if (result.attributeOwnershipAcquisitionEvent) {
      pendingAttributeOwnershipAcquisitionEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionEvent));
    }
    if (result.attributeOwnershipUnavailableEvent) {
      pendingAttributeOwnershipUnavailableEvents_.push_back(
          std::move(*result.attributeOwnershipUnavailableEvent));
    }
    if (result.attributeTransportationTypeChangeEvent) {
      pendingAttributeTransportationTypeChangeEvents_.push_back(
          std::move(*result.attributeTransportationTypeChangeEvent));
    }
    if (result.attributeTransportationTypeQueryEvent) {
      pendingAttributeTransportationTypeQueryEvents_.push_back(
          std::move(*result.attributeTransportationTypeQueryEvent));
    }
    if (result.interactionTransportationTypeChangeEvent) {
      pendingInteractionTransportationTypeChangeEvents_.push_back(
          std::move(*result.interactionTransportationTypeChangeEvent));
    }
    if (result.interactionTransportationTypeQueryEvent) {
      pendingInteractionTransportationTypeQueryEvents_.push_back(
          std::move(*result.interactionTransportationTypeQueryEvent));
    }
  }
}

ProcessFederationAttributeUpdateEvent
ProcessFederationClient::receivePushedAttributeUpdate() {
  if (!pendingAttributeUpdateEvents_.empty()) {
    auto event = std::move(pendingAttributeUpdateEvents_.front());
    pendingAttributeUpdateEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation attribute event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed attribute event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        (message.operation !=
             TransportServiceOperation::receive_attribute_update &&
         message.operation != TransportServiceOperation::time_advance_grant) ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed attribute event.");
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveAttributeUpdateResult(message.payload);
    if (!result.event) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed attribute event.");
    }
    return std::move(*result.event);
  }
}

ProcessFederationObjectInstanceDiscoveryEvent
ProcessFederationClient::receivePushedObjectInstanceDiscovery() {
  if (!pendingObjectInstanceDiscoveryEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceDiscoveryEvents_.front());
    pendingObjectInstanceDiscoveryEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation discovery event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed discovery event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        (message.operation !=
             TransportServiceOperation::receive_object_instance_discovery &&
         message.operation != TransportServiceOperation::time_advance_grant) ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed discovery event.");
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveObjectInstanceDiscoveryResult(
        message.payload);
    if (!result.event) {
      throw ProcessFederationClientError(
          "Process federation client received an empty pushed discovery event.");
    }
    return std::move(*result.event);
  }
}

ProcessFederationObjectInstanceScopeChangeEvent
ProcessFederationClient::receivePushedObjectInstanceScopeChange() {
  if (!pendingObjectInstanceScopeChangeEvents_.empty()) {
    auto event = std::move(pendingObjectInstanceScopeChangeEvents_.front());
    pendingObjectInstanceScopeChangeEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation scope event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed scope event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::time_advance_grant) ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed scope event.");
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.scopeChangeEvent) {
      return std::move(*result.scopeChangeEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(std::move(*result.discoveryEvent));
    }
    if (result.attributeRelevanceAdvisoryEvent) {
      pendingAttributeRelevanceAdvisoryEvents_.push_back(
          std::move(*result.attributeRelevanceAdvisoryEvent));
    }
    if (result.attributeValueUpdateRequestEvent) {
      pendingAttributeValueUpdateRequestEvents_.push_back(
          std::move(*result.attributeValueUpdateRequestEvent));
    }
    if (result.attributeOwnershipQueryEvent) {
      pendingAttributeOwnershipQueryEvents_.push_back(
          std::move(*result.attributeOwnershipQueryEvent));
    }
    if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
    }
    if (result.attributeOwnershipAcquisitionEvent) {
      pendingAttributeOwnershipAcquisitionEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionEvent));
    }
    if (result.attributeOwnershipUnavailableEvent) {
      pendingAttributeOwnershipUnavailableEvents_.push_back(
          std::move(*result.attributeOwnershipUnavailableEvent));
    }
  }
}

ProcessFederationAttributeRelevanceAdvisoryEvent
ProcessFederationClient::receivePushedAttributeRelevanceAdvisory() {
  if (!pendingAttributeRelevanceAdvisoryEvents_.empty()) {
    auto event = std::move(pendingAttributeRelevanceAdvisoryEvents_.front());
    pendingAttributeRelevanceAdvisoryEvents_.pop_front();
    return event;
  }
  if (!session_) {
    throw ProcessFederationClientError(
        "A process federation advisory event was requested after client close.");
  }
  while (true) {
    TransportServiceMessage message;
    if (!session_->receive(message)) {
      throw ProcessFederationClientError(
          "Process federation client lost its pushed advisory event.");
    }
    if (message.kind != TransportServiceMessageKind::event ||
        (message.operation != TransportServiceOperation::receive_interaction &&
         message.operation != TransportServiceOperation::time_advance_grant) ||
        message.status != TransportServiceStatus::ok ||
        message.requestId != 0U) {
      throw ProcessFederationClientError(
          "Process federation client received an unexpected pushed advisory event.");
    }
    if (message.operation == TransportServiceOperation::time_advance_grant) {
      auto const result =
          decodeProcessFederationTimeAdvanceResult(message.payload);
      if (result.status != ProcessFederationTimeAdvanceStatus::applied ||
          !result.grantedTime) {
        throw ProcessFederationClientError(
            "Process federation client received an invalid time-advance grant event.");
      }
      if (result.optimisticTime) {
        pendingFlushQueueGrantEvents_.push_back(
            PendingFlushQueueGrant{
                std::move(*result.grantedTime),
                std::move(*result.optimisticTime)});
      } else {
        pendingTimeAdvanceGrantEvents_.push_back(std::move(*result.grantedTime));
      }
      continue;
    }
    auto result = decodeProcessFederationReceiveInteractionResult(message.payload);
    if (result.attributeRelevanceAdvisoryEvent) {
      return std::move(*result.attributeRelevanceAdvisoryEvent);
    }
    if (result.event) {
      pendingEvents_.push_back(std::move(*result.event));
    }
    if (result.scopeChangeEvent) {
      pendingObjectInstanceScopeChangeEvents_.push_back(
          std::move(*result.scopeChangeEvent));
    }
    if (result.attributeEvent) {
      pendingAttributeUpdateEvents_.push_back(std::move(*result.attributeEvent));
    }
    if (result.discoveryEvent) {
      pendingObjectInstanceDiscoveryEvents_.push_back(
          std::move(*result.discoveryEvent));
    }
    if (result.removalEvent) {
      pendingObjectInstanceRemovalEvents_.push_back(
          std::move(*result.removalEvent));
    }
    if (result.attributeValueUpdateRequestEvent) {
      pendingAttributeValueUpdateRequestEvents_.push_back(
          std::move(*result.attributeValueUpdateRequestEvent));
    }
    if (result.attributeOwnershipQueryEvent) {
      pendingAttributeOwnershipQueryEvents_.push_back(
          std::move(*result.attributeOwnershipQueryEvent));
    }
    if (result.attributeOwnershipAcquisitionIfAvailableEvent) {
      pendingAttributeOwnershipAcquisitionIfAvailableEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionIfAvailableEvent));
    }
    if (result.attributeOwnershipAcquisitionEvent) {
      pendingAttributeOwnershipAcquisitionEvents_.push_back(
          std::move(*result.attributeOwnershipAcquisitionEvent));
    }
    if (result.attributeOwnershipUnavailableEvent) {
      pendingAttributeOwnershipUnavailableEvents_.push_back(
          std::move(*result.attributeOwnershipUnavailableEvent));
    }
  }
}

ProcessFederationAttributeTransportationTypeChangeEvent
ProcessFederationClient::receivePushedAttributeTransportationTypeChange() {
  if (pendingAttributeTransportationTypeChangeEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute transportation-type change event.");
  }
  auto event = std::move(pendingAttributeTransportationTypeChangeEvents_.front());
  pendingAttributeTransportationTypeChangeEvents_.pop_front();
  return event;
}

ProcessFederationAttributeTransportationTypeQueryEvent
ProcessFederationClient::receivePushedAttributeTransportationTypeQuery() {
  if (pendingAttributeTransportationTypeQueryEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued attribute transportation-type query event.");
  }
  auto event = std::move(pendingAttributeTransportationTypeQueryEvents_.front());
  pendingAttributeTransportationTypeQueryEvents_.pop_front();
  return event;
}

ProcessFederationInteractionTransportationTypeChangeEvent
ProcessFederationClient::receivePushedInteractionTransportationTypeChange() {
  if (pendingInteractionTransportationTypeChangeEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued interaction transportation-type change event.");
  }
  auto event = std::move(
      pendingInteractionTransportationTypeChangeEvents_.front());
  pendingInteractionTransportationTypeChangeEvents_.pop_front();
  return event;
}

ProcessFederationInteractionTransportationTypeQueryEvent
ProcessFederationClient::receivePushedInteractionTransportationTypeQuery() {
  if (pendingInteractionTransportationTypeQueryEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued interaction transportation-type query event.");
  }
  auto event = std::move(
      pendingInteractionTransportationTypeQueryEvents_.front());
  pendingInteractionTransportationTypeQueryEvents_.pop_front();
  return event;
}

ProcessFederationSynchronizationPointAnnouncementEvent
ProcessFederationClient::receivePushedSynchronizationPointAnnouncement() {
  if (pendingSynchronizationPointAnnouncementEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued synchronization-point announcement event.");
  }
  auto event = std::move(
      pendingSynchronizationPointAnnouncementEvents_.front());
  pendingSynchronizationPointAnnouncementEvents_.pop_front();
  return event;
}

ProcessFederationFederationSynchronizedEvent
ProcessFederationClient::receivePushedFederationSynchronized() {
  if (pendingFederationSynchronizedEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued Federation Synchronized event.");
  }
  auto event = std::move(pendingFederationSynchronizedEvents_.front());
  pendingFederationSynchronizedEvents_.pop_front();
  return event;
}

ProcessFederationSaveEvent ProcessFederationClient::receivePushedFederationSave() {
  if (pendingFederationSaveEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued federation-save event.");
  }
  auto event = std::move(pendingFederationSaveEvents_.front());
  pendingFederationSaveEvents_.pop_front();
  return event;
}

ProcessFederationRestoreEvent
ProcessFederationClient::receivePushedFederationRestore() {
  if (pendingFederationRestoreEvents_.empty()) {
    throw ProcessFederationClientError(
        "Process federation client has no queued federation-restore event.");
  }
  auto event = std::move(pendingFederationRestoreEvents_.front());
  pendingFederationRestoreEvents_.pop_front();
  return event;
}

void ProcessFederationClient::requireCallbackBridge() const {
  if (!callbackBridge_) {
    throw ProcessFederationClientError(
        "A process federation callback bridge has not been attached.");
  }
}

}  // namespace umbra::detail
