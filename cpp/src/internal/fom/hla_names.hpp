#pragma once

// Canonical names for the standard HLA identifiers used by the embedded
// federation-management implementation.  Keep these values in one place:
// the FOM catalog stores UTF-8 names, while the public 1516.1 API boundary
// uses wide strings.

namespace umbra::detail::hla {

namespace utf8 {

namespace fom {

inline constexpr char object_root[] = "HLAobjectRoot";
inline constexpr char interaction_root[] = "HLAinteractionRoot";
inline constexpr char object_instance_name[] = "HLAobjectInstanceName";
inline constexpr char object_instance_handle[] = "HLAobjectInstanceHandle";
inline constexpr char unicode_string[] = "HLAunicodeString";

}  // namespace fom

namespace mom {

inline constexpr char standard_mim[] = "HLAstandardMIM";
inline constexpr char standard_mim_file_name[] = "HLAstandardMIM-2025.xml";
inline constexpr char interaction_manager_prefix[] = "HLAinteractionRoot.HLAmanager.";

inline constexpr char federate_object_class[] =
    "HLAobjectRoot.HLAmanager.HLAfederate";
inline constexpr char federation_object_class[] =
    "HLAobjectRoot.HLAmanager.HLAfederation";

inline constexpr char request_object_instances_updated[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
inline constexpr char request_object_instances_that_can_be_deleted[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted";
inline constexpr char request_object_instances_reflected[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
inline constexpr char request_object_instance_information[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation";
inline constexpr char request_synchronization_points[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
inline constexpr char request_synchronization_point_status[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus";
inline constexpr char request_fom_module_data_federation[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData";
inline constexpr char request_mim_data[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
inline constexpr char request_fom_module_data_federate[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData";
inline constexpr char request_publications[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications";
inline constexpr char request_subscriptions[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
inline constexpr char request_reflections_received[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived";
inline constexpr char request_updates_sent[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent";
inline constexpr char request_interactions_sent[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent";
inline constexpr char request_directed_interactions_sent[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent";
inline constexpr char request_interactions_received[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived";
inline constexpr char request_directed_interactions_received[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived";
inline constexpr char request_attribute_transportation_type_change[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange";
inline constexpr char request_interaction_transportation_type_change[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange";
inline constexpr char federate_set_timing[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
inline constexpr char federation_set_switches[] =
    "HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches";
inline constexpr char federate_set_switches[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
inline constexpr char federate_modify_attribute_state[] =
    "HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState";

inline constexpr char federate[] = "HLAfederate";
inline constexpr char federate_handle[] = "HLAfederateHandle";
inline constexpr char federate_name[] = "HLAfederateName";
inline constexpr char federate_state[] = "HLAfederateState";
inline constexpr char federate_host[] = "HLAfederateHost";
inline constexpr char federate_type[] = "HLAfederateType";
inline constexpr char federates_in_federation[] = "HLAfederatesInFederation";
inline constexpr char federation_name[] = "HLAfederationName";
inline constexpr char mim_designator[] = "HLAMIMdesignator";
// The service-report JSON spelling is distinct from the MIM attribute name.
inline constexpr char service_report_mim_designator[] = "HLAMIMDesignator";
inline constexpr char mim_data[] = "HLAMIMdata";
inline constexpr char fom_module_designator_list[] = "HLAFOMmoduleDesignatorList";
inline constexpr char fom_module_indicator[] = "HLAFOMmoduleIndicator";
inline constexpr char current_fdd[] = "HLAcurrentFDD";
inline constexpr char time_implementation_name[] = "HLAtimeImplementationName";
inline constexpr char time_manager_state[] = "HLAtimeManagerState";
inline constexpr char time_regulating[] = "HLAtimeRegulating";
inline constexpr char time_constrained[] = "HLAtimeConstrained";
inline constexpr char next_save_name[] = "HLAnextSaveName";
inline constexpr char next_save_time[] = "HLAnextSaveTime";
inline constexpr char last_save_name[] = "HLAlastSaveName";
inline constexpr char last_save_time[] = "HLAlastSaveTime";
inline constexpr char sync_point_name[] = "HLAsyncPointName";
inline constexpr char report_period[] = "HLAreportPeriod";
inline constexpr char auto_provide[] = "HLAautoProvide";
inline constexpr char automatic_resign_action[] = "HLAautomaticResignAction";
inline constexpr char asynchronous_delivery[] = "HLAasynchronousDelivery";
inline constexpr char object_instance[] = "HLAobjectInstance";
inline constexpr char attribute[] = "HLAattribute";
inline constexpr char attribute_state[] = "HLAattributeState";
inline constexpr char object_class_relevance_advisory[] =
    "HLAobjectClassRelevanceAdvisory";
inline constexpr char attribute_relevance_advisory[] =
    "HLAattributeRelevanceAdvisory";
inline constexpr char attribute_scope_advisory[] = "HLAattributeScopeAdvisory";
inline constexpr char interaction_relevance_advisory[] =
    "HLAinteractionRelevanceAdvisory";
inline constexpr char convey_region_designator_sets[] =
    "HLAconveyRegionDesignatorSets";
inline constexpr char service_reporting[] = "HLAserviceReporting";
inline constexpr char exception_reporting[] = "HLAexceptionReporting";
inline constexpr char send_service_reports_to_file[] =
    "HLAsendServiceReportsToFile";

inline constexpr char argument_type[] = "HLAargumentType";
inline constexpr char argument_name[] = "HLAargumentName";
inline constexpr char argument_value[] = "HLAargumentValue";
inline constexpr char returned_argument[] = "HLAreturnedArgument";
inline constexpr char serial_number[] = "HLAserialNumber";
inline constexpr char service[] = "HLAservice";
inline constexpr char supplied_arguments[] = "HLAsuppliedArguments";
inline constexpr char success_indicator[] = "HLAsuccessIndicator";
inline constexpr char exception[] = "HLAexception";
inline constexpr char rti_version[] = "HLARTIversion";
inline constexpr char attribute_list[] = "HLAattributeList";
inline constexpr char interaction_class[] = "HLAinteractionClass";
inline constexpr char transportation[] = "HLAtransportation";
inline constexpr char reliable[] = "HLAreliable";
inline constexpr char best_effort[] = "HLAbestEffort";
inline constexpr char default_update_rate[] = "HLAdefault";

}  // namespace mom

}  // namespace utf8

namespace wide {

namespace fom {

inline constexpr wchar_t object_root[] = L"HLAobjectRoot";
inline constexpr wchar_t interaction_root[] = L"HLAinteractionRoot";
inline constexpr wchar_t object_instance_name[] = L"HLAobjectInstanceName";
inline constexpr wchar_t object_instance_handle[] = L"HLAobjectInstanceHandle";
inline constexpr wchar_t unicode_string[] = L"HLAunicodeString";

}  // namespace fom

namespace mom {

inline constexpr wchar_t standard_mim[] = L"HLAstandardMIM";
inline constexpr wchar_t standard_mim_file_name[] = L"HLAstandardMIM-2025.xml";
inline constexpr wchar_t manager_federation_json[] = L"HLAmanager.HLAfederation";
inline constexpr wchar_t manager_federate_json[] = L"HLAmanager.HLAfederate";
inline constexpr wchar_t federate_object_class[] =
    L"HLAobjectRoot.HLAmanager.HLAfederate";
inline constexpr wchar_t federation_object_class[] =
    L"HLAobjectRoot.HLAmanager.HLAfederation";

inline constexpr wchar_t request_object_instances_updated[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesUpdated";
inline constexpr wchar_t request_object_instances_that_can_be_deleted[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesThatCanBeDeleted";
inline constexpr wchar_t request_object_instances_reflected[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstancesReflected";
inline constexpr wchar_t request_object_instance_information[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation";
inline constexpr wchar_t request_fom_module_data_federate[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData";
inline constexpr wchar_t request_publications[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestPublications";
inline constexpr wchar_t request_subscriptions[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestSubscriptions";
inline constexpr wchar_t request_reflections_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestReflectionsReceived";
inline constexpr wchar_t request_updates_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestUpdatesSent";
inline constexpr wchar_t request_interactions_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsSent";
inline constexpr wchar_t request_directed_interactions_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsSent";
inline constexpr wchar_t request_interactions_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestInteractionsReceived";
inline constexpr wchar_t request_directed_interactions_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestDirectedInteractionsReceived";
inline constexpr wchar_t request_attribute_transportation_type_change[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestAttributeTransportationTypeChange";
inline constexpr wchar_t request_interaction_transportation_type_change[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAservice.HLArequestInteractionTransportationTypeChange";
inline constexpr wchar_t request_synchronization_points[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPoints";
inline constexpr wchar_t request_synchronization_point_status[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus";
inline constexpr wchar_t request_fom_module_data_federation[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData";
inline constexpr wchar_t request_mim_data[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestMIMdata";
inline constexpr wchar_t request_mim_data_name[] = L"HLArequestMIMdata";
inline constexpr wchar_t request_object_instances_updated_name[] =
    L"HLArequestObjectInstancesUpdated";
inline constexpr wchar_t request_prefix[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.";

inline constexpr wchar_t report_service_invocation[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation";
inline constexpr wchar_t report_federate_lost[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost";
inline constexpr wchar_t report_exception[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException";
inline constexpr wchar_t report_mom_exception[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception";
inline constexpr wchar_t report_object_instances_updated[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesUpdated";
inline constexpr wchar_t report_object_instances_that_can_be_deleted[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesThatCanBeDeleted";
inline constexpr wchar_t report_object_instances_reflected[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstancesReflected";
inline constexpr wchar_t report_object_instance_information[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation";
inline constexpr wchar_t report_updates_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportUpdatesSent";
inline constexpr wchar_t report_interactions_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsSent";
inline constexpr wchar_t report_directed_interactions_sent[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsSent";
inline constexpr wchar_t report_reflections_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportReflectionsReceived";
inline constexpr wchar_t report_interactions_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionsReceived";
inline constexpr wchar_t report_directed_interactions_received[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionsReceived";
inline constexpr wchar_t report_object_class_publication[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassPublication";
inline constexpr wchar_t report_object_class_subscription[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectClassSubscription";
inline constexpr wchar_t report_interaction_publication[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionPublication";
inline constexpr wchar_t report_interaction_subscription[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportInteractionSubscription";
inline constexpr wchar_t report_directed_interaction_publication[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionPublication";
inline constexpr wchar_t report_directed_interaction_subscription[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportDirectedInteractionSubscription";
inline constexpr wchar_t report_fom_module_data_federate[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData";
inline constexpr wchar_t report_fom_module_data_federation[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData";
inline constexpr wchar_t report_mim_data[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportMIMdata";
inline constexpr wchar_t report_synchronization_points[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPoints";
inline constexpr wchar_t report_synchronization_point_status[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus";

inline constexpr wchar_t set_timing[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming";
inline constexpr wchar_t set_switches_federate[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches";
inline constexpr wchar_t set_switches_federation[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches";
inline constexpr wchar_t modify_attribute_state[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAmodifyAttributeState";
inline constexpr wchar_t set_switches_prefix[] =
    L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches.";

inline constexpr wchar_t federate[] = L"HLAfederate";
inline constexpr wchar_t federation[] = L"HLAfederation";
inline constexpr wchar_t federate_handle[] = L"HLAfederateHandle";
inline constexpr wchar_t federate_name[] = L"HLAfederateName";
inline constexpr wchar_t federate_state[] = L"HLAfederateState";
inline constexpr wchar_t federate_host[] = L"HLAfederateHost";
inline constexpr wchar_t federate_type[] = L"HLAfederateType";
inline constexpr wchar_t federates_in_federation[] = L"HLAfederatesInFederation";
inline constexpr wchar_t federation_name[] = L"HLAfederationName";
inline constexpr wchar_t mim_designator[] = L"HLAMIMdesignator";
inline constexpr wchar_t service_report_mim_designator[] = L"HLAMIMDesignator";
inline constexpr wchar_t mim_data[] = L"HLAMIMdata";
inline constexpr wchar_t fom_module_data[] = L"HLAFOMmoduleData";
inline constexpr wchar_t fom_module_designator_list[] = L"HLAFOMmoduleDesignatorList";
inline constexpr wchar_t fom_module_indicator[] = L"HLAFOMmoduleIndicator";
inline constexpr wchar_t current_fdd[] = L"HLAcurrentFDD";
inline constexpr wchar_t time_implementation_name[] = L"HLAtimeImplementationName";
inline constexpr wchar_t time_manager_state[] = L"HLAtimeManagerState";
inline constexpr wchar_t time_regulating[] = L"HLAtimeRegulating";
inline constexpr wchar_t time_constrained[] = L"HLAtimeConstrained";
inline constexpr wchar_t next_save_name[] = L"HLAnextSaveName";
inline constexpr wchar_t next_save_time[] = L"HLAnextSaveTime";
inline constexpr wchar_t last_save_name[] = L"HLAlastSaveName";
inline constexpr wchar_t last_save_time[] = L"HLAlastSaveTime";
inline constexpr wchar_t sync_point_name[] = L"HLAsyncPointName";
inline constexpr wchar_t sync_point_federates[] = L"HLAsyncPointFederates";
inline constexpr wchar_t sync_points[] = L"HLAsyncPoints";
inline constexpr wchar_t report_period[] = L"HLAreportPeriod";
inline constexpr wchar_t auto_provide[] = L"HLAautoProvide";
inline constexpr wchar_t automatic_resign_action[] = L"HLAautomaticResignAction";
inline constexpr wchar_t asynchronous_delivery[] = L"HLAasynchronousDelivery";
inline constexpr wchar_t object_instance[] = L"HLAobjectInstance";
inline constexpr wchar_t attribute[] = L"HLAattribute";
inline constexpr wchar_t attribute_state[] = L"HLAattributeState";
inline constexpr wchar_t object_class_relevance_advisory[] =
    L"HLAobjectClassRelevanceAdvisory";
inline constexpr wchar_t attribute_relevance_advisory[] =
    L"HLAattributeRelevanceAdvisory";
inline constexpr wchar_t attribute_scope_advisory[] = L"HLAattributeScopeAdvisory";
inline constexpr wchar_t interaction_relevance_advisory[] =
    L"HLAinteractionRelevanceAdvisory";
inline constexpr wchar_t convey_region_designator_sets[] =
    L"HLAconveyRegionDesignatorSets";
inline constexpr wchar_t service_reporting[] = L"HLAserviceReporting";
inline constexpr wchar_t exception_reporting[] = L"HLAexceptionReporting";
inline constexpr wchar_t send_service_reports_to_file[] =
    L"HLAsendServiceReportsToFile";
inline constexpr wchar_t argument_type[] = L"HLAargumentType";
inline constexpr wchar_t argument_name[] = L"HLAargumentName";
inline constexpr wchar_t argument_value[] = L"HLAargumentValue";
inline constexpr wchar_t returned_argument[] = L"HLAreturnedArgument";
inline constexpr wchar_t serial_number[] = L"HLAserialNumber";
inline constexpr wchar_t service[] = L"HLAservice";
inline constexpr wchar_t supplied_arguments[] = L"HLAsuppliedArguments";
inline constexpr wchar_t success_indicator[] = L"HLAsuccessIndicator";
inline constexpr wchar_t exception[] = L"HLAexception";
inline constexpr wchar_t rti_version[] = L"HLARTIversion";
inline constexpr wchar_t reliable[] = L"HLAreliable";
inline constexpr wchar_t best_effort[] = L"HLAbestEffort";
inline constexpr wchar_t transportation[] = L"HLAtransportation";
inline constexpr wchar_t default_update_rate[] = L"HLAdefault";
inline constexpr wchar_t default_update_rate_attribute[] =
    L"HLAdefaultUpdateRate";
inline constexpr wchar_t active[] = L"HLAactive";
inline constexpr wchar_t interaction_class[] = L"HLAinteractionClass";
inline constexpr wchar_t attribute_list[] = L"HLAattributeList";
inline constexpr wchar_t fault_description[] = L"HLAfaultDescription";
inline constexpr wchar_t object_class[] = L"HLAobjectClass";
inline constexpr wchar_t interaction_class_list[] = L"HLAinteractionClassList";
inline constexpr wchar_t interaction_counts[] = L"HLAinteractionCounts";
inline constexpr wchar_t interactions_received[] = L"HLAinteractionsReceived";
inline constexpr wchar_t interactions_sent[] = L"HLAinteractionsSent";
inline constexpr wchar_t directed_interactions_received[] =
    L"HLAdirectedInteractionsReceived";
inline constexpr wchar_t directed_interactions_sent[] =
    L"HLAdirectedInteractionsSent";
inline constexpr wchar_t known_class[] = L"HLAknownClass";
inline constexpr wchar_t logical_time[] = L"HLAlogicalTime";
inline constexpr wchar_t lookahead[] = L"HLAlookahead";
inline constexpr wchar_t max_update_rate[] = L"HLAmaxUpdateRate";
inline constexpr wchar_t number_of_classes[] = L"HLAnumberOfClasses";
inline constexpr wchar_t object_instance_counts[] = L"HLAobjectInstanceCounts";
inline constexpr wchar_t object_instances_deleted[] =
    L"HLAobjectInstancesDeleted";
inline constexpr wchar_t object_instances_discovered[] =
    L"HLAobjectInstancesDiscovered";
inline constexpr wchar_t object_instances_reflected[] =
    L"HLAobjectInstancesReflected";
inline constexpr wchar_t object_instances_registered[] =
    L"HLAobjectInstancesRegistered";
inline constexpr wchar_t object_instances_removed[] =
    L"HLAobjectInstancesRemoved";
inline constexpr wchar_t object_instances_that_can_be_deleted[] =
    L"HLAobjectInstancesThatCanBeDeleted";
inline constexpr wchar_t object_instances_updated[] =
    L"HLAobjectInstancesUpdated";
inline constexpr wchar_t owned_instance_attribute_list[] =
    L"HLAownedInstanceAttributeList";
inline constexpr wchar_t parameter_error[] = L"HLAparameterError";
inline constexpr wchar_t privilege_to_delete_object[] =
    L"HLAprivilegeToDeleteObject";
inline constexpr wchar_t reflect_counts[] = L"HLAreflectCounts";
inline constexpr wchar_t reflections_received[] = L"HLAreflectionsReceived";
inline constexpr wchar_t registered_class[] = L"HLAregisteredClass";
inline constexpr wchar_t report_service_file[] = L"HLAreportServiceFile";
inline constexpr wchar_t service_group[] = L"HLAserviceGroup";
inline constexpr wchar_t service_type[] = L"HLAserviceType";
inline constexpr wchar_t time_advancing_time[] = L"HLAtimeAdvancingTime";
inline constexpr wchar_t time_granted_time[] = L"HLAtimeGrantedTime";
inline constexpr wchar_t time_stamp[] = L"HLAtimeStamp";
inline constexpr wchar_t update_counts[] = L"HLAupdateCounts";
inline constexpr wchar_t updates_sent[] = L"HLAupdatesSent";
inline constexpr wchar_t custom_order[] = L"HLAcustomOrder";
inline constexpr wchar_t galt[] = L"HLAGALT";
inline constexpr wchar_t lits[] = L"HLALITS";
inline constexpr wchar_t ro_length[] = L"HLAROlength";
inline constexpr wchar_t tso_length[] = L"HLATSOlength";
inline constexpr wchar_t float64_time[] = L"HLAfloat64Time";
inline constexpr wchar_t integer64_time[] = L"HLAinteger64Time";

}  // namespace mom

}  // namespace wide

}  // namespace umbra::detail::hla
