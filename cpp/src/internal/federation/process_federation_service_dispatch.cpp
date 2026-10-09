#include "internal/federation/process_federation_service.hpp"
#include "internal/federation/process_federation_service_helpers.hpp"

#include <exception>

namespace umbra::detail {

TransportServiceMessage ProcessFederationService::handle(
    ProcessTransportSession &session,
    TransportServiceMessage const &request) {
  try {
    switch (request.operation) {
      case TransportServiceOperation::create_federation_execution:
        return handleCreate(request);
      case TransportServiceOperation::destroy_federation_execution:
        return handleDestroy(request);
      case TransportServiceOperation::join_federation_execution:
        return handleJoin(session, request);
      case TransportServiceOperation::resign_federation_execution:
        return handleResign(session, request);
      case TransportServiceOperation::register_federation_synchronization_point:
        return handleRegisterFederationSynchronizationPoint(session, request);
      case TransportServiceOperation::synchronization_point_achieved:
        return handleSynchronizationPointAchieved(session, request);
      case TransportServiceOperation::request_federation_save:
        return handleRequestFederationSave(session, request);
      case TransportServiceOperation::federate_save_begun:
      case TransportServiceOperation::federate_save_complete:
      case TransportServiceOperation::federate_save_not_complete:
        return handleFederateSaveControl(session, request, request.operation);
      case TransportServiceOperation::query_federation_save_status:
        return handleQueryFederationSaveStatus(session, request);
      case TransportServiceOperation::abort_federation_save:
        return handleAbortFederationSave(session, request);
      case TransportServiceOperation::request_federation_restore:
        return handleRequestFederationRestore(session, request);
      case TransportServiceOperation::federate_restore_complete:
        return handleFederateRestoreComplete(session, request);
      case TransportServiceOperation::federate_restore_not_complete:
        return handleFederateRestoreNotComplete(session, request);
      case TransportServiceOperation::abort_federation_restore:
        return handleAbortFederationRestore(session, request);
      case TransportServiceOperation::query_federation_restore_status:
        return handleQueryFederationRestoreStatus(session, request);
      case TransportServiceOperation::send_interaction:
        return handleSendInteraction(session, request);
      case TransportServiceOperation::send_interaction_with_regions:
        return handleSendInteractionWithRegions(session, request);
      case TransportServiceOperation::send_directed_interaction:
        return handleSendDirectedInteraction(session, request);
      case TransportServiceOperation::retract:
        return handleRetract(session, request);
      case TransportServiceOperation::update_attribute_values:
        return handleUpdateAttributeValues(session, request);
      case TransportServiceOperation::request_attribute_value_update:
        return handleRequestAttributeValueUpdate(session, request);
      case TransportServiceOperation::request_attribute_value_update_class:
        return handleRequestAttributeValueUpdateClass(session, request);
      case TransportServiceOperation::request_attribute_value_update_class_with_regions:
        return handleRequestAttributeValueUpdateClassWithRegions(session, request);
      case TransportServiceOperation::is_attribute_owned_by_federate:
        return handleAttributeOwnershipCheck(session, request);
      case TransportServiceOperation::query_attribute_ownership:
        return handleQueryAttributeOwnership(session, request);
      case TransportServiceOperation::attribute_ownership_acquisition_if_available:
        return handleAttributeOwnershipAcquisitionIfAvailable(session, request);
      case TransportServiceOperation::attribute_ownership_acquisition:
        return handleAttributeOwnershipAcquisition(session, request);
      case TransportServiceOperation::attribute_ownership_release_denied:
        return handleAttributeOwnershipReleaseDenied(session, request);
      case TransportServiceOperation::cancel_attribute_ownership_acquisition:
        return handleAttributeOwnershipAcquisitionCancellation(session, request);
      case TransportServiceOperation::
          cancel_negotiated_attribute_ownership_divestiture:
        return handleCancelNegotiatedAttributeOwnershipDivestiture(
            session, request);
      case TransportServiceOperation::negotiated_attribute_ownership_divestiture:
        return handleNegotiatedAttributeOwnershipDivestiture(session, request);
      case TransportServiceOperation::confirm_divestiture:
        return handleConfirmDivestiture(session, request);
      case TransportServiceOperation::unconditional_attribute_ownership_divestiture:
        return handleUnconditionalAttributeOwnershipDivestiture(session, request);
      case TransportServiceOperation::receive_interaction:
        return handleReceiveInteraction(session, request);
      case TransportServiceOperation::acknowledge_tso_delivery:
        return handleAcknowledgeTsoDelivery(session, request);
      case TransportServiceOperation::query_logical_time:
        return handleQueryLogicalTime(session, request);
      case TransportServiceOperation::query_lookahead:
        return handleQueryLookahead(session, request);
      case TransportServiceOperation::modify_lookahead:
        return handleModifyLookahead(session, request);
      case TransportServiceOperation::enable_time_regulation:
        return handleEnableTimeRegulation(session, request);
      case TransportServiceOperation::query_time_bounds:
        return handleQueryTimeBounds(session, request);
      case TransportServiceOperation::enable_time_constrained:
        return handleEnableTimeConstrained(session, request);
      case TransportServiceOperation::disable_time_regulation:
        return handleDisableTimeRegulation(session, request);
      case TransportServiceOperation::disable_time_constrained:
        return handleDisableTimeConstrained(session, request);
      case TransportServiceOperation::time_advance_request:
        return handleTimeAdvanceRequest(session, request);
      case TransportServiceOperation::time_advance_request_available:
        return handleTimeAdvanceRequestAvailable(session, request);
      case TransportServiceOperation::next_message_request:
        return handleNextMessageRequest(session, request);
      case TransportServiceOperation::next_message_request_available:
        return handleNextMessageRequestAvailable(session, request);
      case TransportServiceOperation::flush_queue_request:
        return handleFlushQueueRequest(session, request);
      case TransportServiceOperation::receive_attribute_update:
        return handleReceiveAttributeUpdate(session, request);
      case TransportServiceOperation::receive_object_instance_discovery:
        return handleReceiveObjectInstanceDiscovery(session, request);
      case TransportServiceOperation::get_federate_handle:
        return handleGetFederateHandle(session, request);
      case TransportServiceOperation::get_federate_name:
        return handleGetFederateName(session, request);
      case TransportServiceOperation::normalize_federate_handle:
      case TransportServiceOperation::normalize_object_class_handle:
      case TransportServiceOperation::normalize_interaction_class_handle:
      case TransportServiceOperation::normalize_object_instance_handle:
        return handleNormalizeHandle(session, request);
      case TransportServiceOperation::get_interaction_class_handle:
        return handleGetInteractionClassHandle(session, request);
      case TransportServiceOperation::get_object_class_handle:
        return handleGetObjectClassHandle(session, request);
      case TransportServiceOperation::get_parameter_handle:
        return handleGetParameterHandle(session, request);
      case TransportServiceOperation::publish_object_class_attributes:
      case TransportServiceOperation::unpublish_object_class_attributes:
      case TransportServiceOperation::unpublish_object_class:
        return handleObjectClassAttributeDeclaration(session, request);
      case TransportServiceOperation::subscribe_object_class_attributes:
      case TransportServiceOperation::unsubscribe_object_class_attributes:
        return handleObjectClassAttributeSubscription(session, request);
      case TransportServiceOperation::subscribe_object_class_attributes_with_regions:
      case TransportServiceOperation::unsubscribe_object_class_attributes_with_regions:
        return handleObjectClassAttributeRegionalSubscription(session, request);
      case TransportServiceOperation::register_object_instance:
        return handleRegisterObjectInstance(session, request);
      case TransportServiceOperation::local_delete_object_instance:
        return handleLocalDeleteObjectInstance(session, request);
      case TransportServiceOperation::delete_object_instance:
        return handleDeleteObjectInstance(session, request);
      case TransportServiceOperation::reserve_object_instance_name:
        return handleReserveObjectInstanceName(session, request);
      case TransportServiceOperation::release_object_instance_name:
        return handleReleaseObjectInstanceName(session, request);
      case TransportServiceOperation::reserve_multiple_object_instance_names:
        return handleReserveMultipleObjectInstanceNames(session, request);
      case TransportServiceOperation::release_multiple_object_instance_names:
        return handleReleaseMultipleObjectInstanceNames(session, request);
      case TransportServiceOperation::get_dimension_handle:
        return handleGetDimensionHandle(session, request);
      case TransportServiceOperation::get_dimension_name:
        return handleGetDimensionName(session, request);
      case TransportServiceOperation::get_transportation_type_handle:
        return handleGetTransportationTypeHandle(session, request);
      case TransportServiceOperation::get_transportation_type_name:
        return handleGetTransportationTypeName(session, request);
      case TransportServiceOperation::get_dimension_upper_bound:
        return handleGetDimensionUpperBound(session, request);
      case TransportServiceOperation::get_available_dimensions_for_object_class:
        return handleGetAvailableDimensionsForObjectClass(session, request);
      case TransportServiceOperation::get_available_dimensions_for_interaction_class:
        return handleGetAvailableDimensionsForInteractionClass(session, request);
      case TransportServiceOperation::create_region:
        return handleCreateRegion(session, request);
      case TransportServiceOperation::commit_region_modifications:
        return handleCommitRegionModifications(session, request);
      case TransportServiceOperation::delete_region:
        return handleDeleteRegion(session, request);
      case TransportServiceOperation::get_dimension_handle_set:
        return handleGetDimensionHandleSet(session, request);
      case TransportServiceOperation::get_range_bounds:
        return handleGetRangeBounds(session, request);
      case TransportServiceOperation::set_range_bounds:
        return handleSetRangeBounds(session, request);
      case TransportServiceOperation::get_attribute_scope_advisory_switch:
        return handleGetAttributeScopeAdvisorySwitch(session, request);
      case TransportServiceOperation::set_attribute_scope_advisory_switch:
        return handleSetAttributeScopeAdvisorySwitch(session, request);
      case TransportServiceOperation::get_object_class_relevance_advisory_switch:
        return handleGetObjectClassRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::get_attribute_relevance_advisory_switch:
        return handleGetAttributeRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::get_interaction_relevance_advisory_switch:
        return handleGetInteractionRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::set_attribute_relevance_advisory_switch:
        return handleSetAttributeRelevanceAdvisorySwitch(session, request);
      case TransportServiceOperation::get_convey_region_designator_sets_switch:
        return handleGetConveyRegionDesignatorSetsSwitch(session, request);
      case TransportServiceOperation::get_allow_relaxed_ddm_switch:
        return handleGetAllowRelaxedDDMSwitch(session, request);
      case TransportServiceOperation::set_convey_region_designator_sets_switch:
        return handleSetConveyRegionDesignatorSetsSwitch(session, request);
      case TransportServiceOperation::get_service_reporting_switch:
        return handleGetServiceReportingSwitch(session, request);
      case TransportServiceOperation::set_service_reporting_switch:
        return handleSetServiceReportingSwitch(session, request);
      case TransportServiceOperation::get_exception_reporting_switch:
        return handleGetExceptionReportingSwitch(session, request);
      case TransportServiceOperation::report_service_exception:
        return handleReportServiceException(session, request);
      case TransportServiceOperation::report_failed_service_invocation:
        return handleReportFailedServiceInvocation(session, request);
      case TransportServiceOperation::report_successful_service_invocation:
        return handleReportSuccessfulServiceInvocation(session, request);
      case TransportServiceOperation::report_successful_void_service_invocation:
        return handleReportSuccessfulVoidServiceInvocation(session, request);
      case TransportServiceOperation::recheck_exception_report:
        return handleRecheckExceptionReport(session, request);
      case TransportServiceOperation::set_exception_reporting_switch:
        return handleSetExceptionReportingSwitch(session, request);
      case TransportServiceOperation::get_send_service_reports_to_file_switch:
        return handleGetSendServiceReportsToFileSwitch(session, request);
      case TransportServiceOperation::set_send_service_reports_to_file_switch:
        return handleSetSendServiceReportsToFileSwitch(session, request);
      case TransportServiceOperation::get_automatic_resign_directive:
        return handleGetAutomaticResignDirective(session, request);
      case TransportServiceOperation::set_automatic_resign_directive:
        return handleSetAutomaticResignDirective(session, request);
      case TransportServiceOperation::publish_object_class_directed_interactions:
      case TransportServiceOperation::unpublish_object_class_directed_interactions:
      case TransportServiceOperation::subscribe_object_class_directed_interactions:
      case TransportServiceOperation::unsubscribe_object_class_directed_interactions:
        return handleObjectClassDirectedInteractionDeclaration(session, request);
      case TransportServiceOperation::register_object_instance_with_regions:
        return handleRegisterObjectInstanceWithRegions(session, request);
      case TransportServiceOperation::associate_regions_for_updates:
      case TransportServiceOperation::unassociate_regions_for_updates:
        return handleObjectInstanceRegionAssociation(session, request);
      case TransportServiceOperation::get_attribute_handle:
        return handleGetAttributeHandle(session, request);
      case TransportServiceOperation::get_object_instance_handle:
        return handleGetObjectInstanceHandle(session, request);
      case TransportServiceOperation::get_object_instance_name:
        return handleGetObjectInstanceName(session, request);
      case TransportServiceOperation::get_known_object_class_handle:
        return handleGetKnownObjectClassHandle(session, request);
      case TransportServiceOperation::get_update_rate_value:
        return handleGetUpdateRateValue(session, request);
      case TransportServiceOperation::get_update_rate_value_for_attribute:
        return handleGetUpdateRateValueForAttribute(session, request);
      case TransportServiceOperation::get_object_class_name:
        return handleGetObjectClassName(session, request);
      case TransportServiceOperation::get_interaction_class_name:
        return handleGetInteractionClassName(session, request);
      case TransportServiceOperation::get_attribute_name:
        return handleGetAttributeName(session, request);
      case TransportServiceOperation::get_parameter_name:
        return handleGetParameterName(session, request);
      case TransportServiceOperation::publish_interaction_class:
      case TransportServiceOperation::unpublish_interaction_class:
      case TransportServiceOperation::subscribe_interaction_class:
      case TransportServiceOperation::unsubscribe_interaction_class:
        return handleInteractionClassDeclaration(session, request);
      case TransportServiceOperation::change_interaction_order_type:
        return handleChangeInteractionOrderType(session, request);
      case TransportServiceOperation::change_attribute_order_type:
        return handleChangeAttributeOrderType(session, request);
      case TransportServiceOperation::change_default_attribute_order_type:
        return handleChangeDefaultAttributeOrderType(session, request);
      case TransportServiceOperation::change_default_attribute_transportation_type:
        return handleChangeDefaultAttributeTransportationType(session, request);
      case TransportServiceOperation::request_attribute_transportation_type_change:
        return handleRequestAttributeTransportationTypeChange(session, request);
      case TransportServiceOperation::query_attribute_transportation_type:
        return handleQueryAttributeTransportationType(session, request);
      case TransportServiceOperation::request_interaction_transportation_type_change:
        return handleRequestInteractionTransportationTypeChange(session, request);
      case TransportServiceOperation::query_interaction_transportation_type:
        return handleQueryInteractionTransportationType(session, request);
      case TransportServiceOperation::subscribe_interaction_class_with_regions:
      case TransportServiceOperation::unsubscribe_interaction_class_with_regions:
        return handleInteractionClassRegionalSubscription(session, request);
    }
  } catch (ProcessFederationServiceProtocolError const &) {
    return invalid(request);
  } catch (std::exception const &) {
    return internalError(request);
  }
  return invalid(request);
}

}  // namespace umbra::detail
