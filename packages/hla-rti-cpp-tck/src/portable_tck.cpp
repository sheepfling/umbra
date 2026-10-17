#include <RTI/encoding/BasicDataElements.h>
#include <RTI/encoding/HLAfixedRecord.h>
#include <RTI/encoding/HLAvariableArray.h>

#include "tck_harness.hpp"
#include "tck_api_foundation.hpp"
#include "tck_configuration_authorization.hpp"
#include "tck_factory_identity.hpp"
#include "tck_fom_module_scenarios.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_post_resignation_valid_input_scenarios.hpp"
#include "tck_transport_order_type_change.hpp"

namespace hla_rti_cpp_tck::detail {
void scenarioFqrPostgrantLowerTsoRetraction(Options const&, rti::CallbackModel);
void scenarioFqrPostgrantLowerTsoRetractionContract(Options const&, rti::CallbackModel);
void scenarioFqgPostgrantDeliveryCutoff(Options const&, rti::CallbackModel);
void scenarioFqgPostgrantDeliveryCutoffContract(Options const&, rti::CallbackModel);
void scenarioTarQueuedTsoBeforeGrant(Options const&, rti::CallbackModel);
void scenarioTarQueuedTsoBeforeGrantContract(Options const&, rti::CallbackModel);
void scenarioNmrLitsBelowGaltImmediateGrant(Options const&, rti::CallbackModel);
void scenarioNmrLitsBelowGaltImmediateGrantContract(Options const&, rti::CallbackModel);
}
#include "tck_portable_member_control_scenarios.hpp"
#include "tck_transport_order_scenarios.hpp"
#include "tck_regional_auto_provide_scenarios.hpp"
#include "tck_custom_transportation_scenarios.hpp"
#include "tck_portable_mom_declaration_queries.hpp"
#include "tck_portable_region_service_reports.hpp"
#include "tck_portable_mom_interaction_counts.hpp"
#include "tck_portable_resignation_ownership.hpp"
#include "tck_portable_service_report_scenarios.hpp"
#include "tck_timestamped_interaction_boundary_scenarios.hpp"
#include "tck_joined_federate_mom_updates_sent_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_instances_updated_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_instances_reflected_mim_attribute.hpp"
#include "tck_joined_federate_mom_reflections_received_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_instances_discovered_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_instances_removed_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_instances_deleted_mim_attribute.hpp"
#include "tck_joined_federate_mom_object_lifecycle.hpp"
#include "tck_joined_federate_mom_deletable_object_report.hpp"
#include "tck_joined_federate_mom_updated_object_report.hpp"
#include "tck_joined_federate_mom_updated_timestamped_object_report.hpp"
#include "tck_joined_federate_mom_updated_multi_class_object_report.hpp"
#include "tck_joined_federate_mom_updated_null_object_report.hpp"
#include "tck_joined_federate_mom_reflected_null_object_report.hpp"
#include "tck_joined_federate_mom_reflected_object_report.hpp"
#include "tck_joined_federate_mom_reflected_timestamped_object_report.hpp"
#include "tck_joined_federate_mom_reflected_multi_class_object_report.hpp"
#include "tck_joined_federate_mom_deletable_object_count.hpp"
#include "tck_receive_order_attribute_update.hpp"
#include "tck_data_encoding.hpp"
#include "tck_logical_time.hpp"
#include "tck_time_coordinated_interaction.hpp"
#include "tck_empty_attribute_subscription.hpp"
#include "tck_attribute_subscription_mode_transitions.hpp"
#include "tck_nmr_future_timestamped_object_deletion.hpp"
#include "tck_nmra_future_timestamped_object_deletion.hpp"
#include "tck_named_update_rate_subscription.hpp"
#include "tck_default_rate_no_reduction.hpp"
#include "tck_accumulating_attribute_subscriptions.hpp"
#include "tck_passive_subscription_no_start_registration.hpp"
#include "tck_selective_attribute_unsubscription.hpp"
#include "tck_whole_class_attribute_unsubscription.hpp"
#include "tck_empty_attribute_unsubscription.hpp"
#include "pending_negotiated_restore_tck.hpp"
#include "service_report_creation_failure_tck.hpp"
int hla_rti_cpp_tck_original_main(int argc, char** argv);

namespace {

using namespace hla_rti_cpp_tck::detail;

constexpr char factoryDiscoveryScenario[] = "java-tck.factory-discovery";
constexpr char rtiAmbassadorFactoryContractScenario[] =
    "cpp-tck.rti-ambassador-factory-contract";
constexpr char logicalTimeFactoryScenario[] = "java-tck.logical-time-factory";
constexpr char variableLengthDataContractScenario[] =
    "cpp-tck.variable-length-data-contract";
constexpr char logicalTimeContractScenario[] =
    "cpp-tck.logical-time-contract";
constexpr char logicalTimeFactoryFactoryContractScenario[] =
    "cpp-tck.logical-time-factory-factory-contract";
constexpr char logicalTimeDataElementsContractScenario[] =
    "cpp-tck.logical-time-data-elements-contract";
constexpr char exceptionHierarchyContractScenario[] =
    "cpp-tck.exception-hierarchy-contract";
constexpr char enumContractScenario[] = "cpp-tck.enum-contract";
constexpr char handleAndCollectionContractScenario[] =
    "cpp-tck.handle-and-collection-contract";
constexpr char dataElementContractScenario[] = "cpp-tck.data-element-contract";
constexpr char basicDataElementsContractScenario[] =
    "cpp-tck.basic-data-elements-contract";
constexpr char compositeDataElementsContractScenario[] =
    "cpp-tck.composite-data-elements-contract";
constexpr char configurationAndAuthorizationContractScenario[] =
    "cpp-tck.configuration-and-authorization-contract";
constexpr char runtimeIdentityContractScenario[] =
    "cpp-tck.runtime-identity-contract";
constexpr char authorizerFactoryFactoryContractScenario[] =
    "cpp-tck.authorizer-factory-factory-contract";
constexpr char nullFederateAmbassadorContractScenario[] =
    "cpp-tck.null-federate-ambassador-contract";
constexpr char connectionScenario[] = "java-tck.overloads-and-exceptions";
constexpr char connectionCallbackContractScenario[] =
    "cpp-tck.connection-callback-contract";
constexpr char callbackReentrancyScenario[] = "cpp-tck.callback-reentrancy";
constexpr char callbackReentrancyContractScenario[] =
    "cpp-tck.callback-reentrancy-contract";
constexpr char malformedInputsScenario[] = "java-tck.malformed-inputs";
constexpr char malformedInputsContractScenario[] =
    "cpp-tck.malformed-inputs-contract";
constexpr char federationLifecycleScenario[] = "java-tck.federation-membership";
constexpr char federationLifecycleContractScenario[] =
    "cpp-tck.federation-lifecycle-contract";
constexpr char federationManagementRequiresConnectionScenario[] =
    "cpp-tck.federation-management-requires-connection";
constexpr char unnamedJoinOverloadScenario[] =
    "cpp-tck.unnamed-join-overload";
constexpr char unnamedJoinOverloadContractScenario[] =
    "cpp-tck.unnamed-join-overload-contract";
constexpr char federationListServicesScenario[] =
    "cpp-tck.federation-list-services";
constexpr char federationListServicesContractScenario[] =
    "cpp-tck.federation-list-services-contract";
constexpr char federateLookupLifecycleScenario[] =
    "cpp-tck.federate-lookup-lifecycle";
constexpr char federateLookupLifecycleContractScenario[] =
    "cpp-tck.federate-lookup-lifecycle-contract";
constexpr char explicitMimCreationScenario[] =
    "cpp-tck.explicit-mim-creation";
constexpr char explicitMimCreationContractScenario[] =
    "cpp-tck.explicit-mim-creation-contract";
constexpr char federationMomCurrentFddScenario[] =
    "cpp-tck.federation-mom-current-fdd";
constexpr char federationMomCurrentFddContractScenario[] =
    "cpp-tck.federation-mom-current-fdd-contract";
constexpr char federationMomStaticIdentityScenario[] =
    "cpp-tck.federation-mom-static-identity";
constexpr char federationMomStaticIdentityContractScenario[] =
    "cpp-tck.federation-mom-static-identity-contract";
constexpr char federationMomStaticSwitchesScenario[] =
    "cpp-tck.federation-mom-static-switches";
constexpr char federationMomStaticSwitchesContractScenario[] =
    "cpp-tck.federation-mom-static-switches-contract";
constexpr char federationMomFederatesInFederationScenario[] =
    "cpp-tck.federation-mom-federates-in-federation";
constexpr char federationMomFederatesInFederationContractScenario[] =
    "cpp-tck.federation-mom-federates-in-federation-contract";
constexpr char federationMomFomModuleDesignatorListScenario[] =
    "cpp-tck.federation-mom-fom-module-designator-list";
constexpr char federationMomFomModuleDesignatorListContractScenario[] =
    "cpp-tck.federation-mom-fom-module-designator-list-contract";
constexpr char federationMomContentReportsScenario[] =
    "cpp-tck.federation-mom-content-reports";
constexpr char federationMomContentReportsContractScenario[] =
    "cpp-tck.federation-mom-content-reports-contract";
constexpr char serviceReportOrderTransportationLookupsScenario[] =
    "cpp-tck.service-report-order-transportation-lookups";
constexpr char serviceReportOrderTransportationLookupsContractScenario[] =
    "cpp-tck.service-report-order-transportation-lookups-contract";
constexpr char serviceReportDimensionUpperBoundScenario[] =
    "cpp-tck.service-report-dimension-upper-bound";
constexpr char serviceReportDimensionUpperBoundContractScenario[] =
    "cpp-tck.service-report-dimension-upper-bound-contract";
constexpr char serviceReportDimensionHandleSetScenario[] =
    "cpp-tck.service-report-dimension-handle-set";
constexpr char serviceReportDimensionHandleSetContractScenario[] =
    "cpp-tck.service-report-dimension-handle-set-contract";
constexpr char serviceReportRangeBoundsScenario[] =
    "cpp-tck.service-report-range-bounds";
constexpr char serviceReportRangeBoundsContractScenario[] =
    "cpp-tck.service-report-range-bounds-contract";
constexpr char serviceReportSetRangeBoundsScenario[] =
    "cpp-tck.service-report-set-range-bounds";
constexpr char serviceReportSetRangeBoundsContractScenario[] =
    "cpp-tck.service-report-set-range-bounds-contract";
constexpr char serviceReportCommitRegionModificationsScenario[] =
    "cpp-tck.service-report-commit-region-modifications";
constexpr char serviceReportCommitRegionModificationsContractScenario[] =
    "cpp-tck.service-report-commit-region-modifications-contract";
constexpr char serviceReportDeleteRegionScenario[] =
    "cpp-tck.service-report-delete-region";
constexpr char serviceReportDeleteRegionContractScenario[] =
    "cpp-tck.service-report-delete-region-contract";
constexpr char serviceReportCreateRegionScenario[] =
    "cpp-tck.service-report-create-region";
constexpr char serviceReportCreateRegionContractScenario[] =
    "cpp-tck.service-report-create-region-contract";
constexpr char regionRangeValidationScenario[] =
    "cpp-tck.region-range-validation";
constexpr char regionRangeValidationContractScenario[] =
    "cpp-tck.region-range-validation-contract";
constexpr char regionRangeDimensionValidationScenario[] =
    "cpp-tck.region-range-dimension-validation";
constexpr char regionRangeDimensionValidationContractScenario[] =
    "cpp-tck.region-range-dimension-validation-contract";
constexpr char serviceReportDimensionNameScenario[] =
    "cpp-tck.service-report-dimension-name";
constexpr char serviceReportDimensionNameContractScenario[] =
    "cpp-tck.service-report-dimension-name-contract";
constexpr char serviceReportDimensionHandleScenario[] =
    "cpp-tck.service-report-dimension-handle";
constexpr char serviceReportDimensionHandleContractScenario[] =
    "cpp-tck.service-report-dimension-handle-contract";
constexpr char serviceReportInteractionDimensionsScenario[] =
    "cpp-tck.service-report-interaction-dimensions";
constexpr char serviceReportInteractionDimensionsContractScenario[] =
    "cpp-tck.service-report-interaction-dimensions-contract";
constexpr char serviceReportObjectDimensionsScenario[] =
    "cpp-tck.service-report-object-dimensions";
constexpr char serviceReportObjectDimensionsContractScenario[] =
    "cpp-tck.service-report-object-dimensions-contract";
constexpr char serviceReportOrderTransportationLookupFailuresScenario[] =
    "cpp-tck.service-report-order-transportation-lookup-failures";
constexpr char serviceReportOrderTransportationLookupFailuresContractScenario[] =
    "cpp-tck.service-report-order-transportation-lookup-failures-contract";
constexpr char serviceReportFederateObjectClassLookupsScenario[] =
    "cpp-tck.service-report-federate-object-class-lookups";
constexpr char serviceReportFederateObjectClassLookupsContractScenario[] =
    "cpp-tck.service-report-federate-object-class-lookups-contract";
constexpr char serviceReportFederateObjectClassLookupFailuresScenario[] =
    "cpp-tck.service-report-federate-object-class-lookup-failures";
constexpr char serviceReportFederateObjectClassLookupFailuresContractScenario[] =
    "cpp-tck.service-report-federate-object-class-lookup-failures-contract";
constexpr char serviceReportInteractionParameterLookupsScenario[] =
    "cpp-tck.service-report-interaction-parameter-lookups";
constexpr char serviceReportInteractionParameterLookupsContractScenario[] =
    "cpp-tck.service-report-interaction-parameter-lookups-contract";
constexpr char serviceReportInteractionParameterLookupFailuresScenario[] =
    "cpp-tck.service-report-interaction-parameter-lookup-failures";
constexpr char serviceReportInteractionParameterLookupFailuresContractScenario[] =
    "cpp-tck.service-report-interaction-parameter-lookup-failures-contract";
constexpr char serviceReportInteractionScenario[] =
    "cpp-tck.service-report-interaction";
constexpr char serviceReportInteractionContractScenario[] =
    "cpp-tck.service-report-interaction-contract";
constexpr char serviceReportResignFederationExecutionScenario[] =
    "cpp-tck.service-report-resign-federation-execution";
constexpr char serviceReportResignFederationExecutionContractScenario[] =
    "cpp-tck.service-report-resign-federation-execution-contract";
constexpr char federateLostReportScenario[] =
    "cpp-tck.federate-lost-mom-report";
constexpr char federateLostReportContractScenario[] =
    "cpp-tck.federate-lost-mom-report-contract";
constexpr char handleNormalizationMembershipBoundariesScenario[] =
    "cpp-tck.handle-normalization-membership-boundaries";
constexpr char handleNormalizationMembershipBoundariesContractScenario[] =
    "cpp-tck.handle-normalization-membership-boundaries-contract";
constexpr char dimensionLookupValidHandlesAfterResignationScenario[] =
    "cpp-tck.dimension-lookup-valid-handles-after-resignation";
constexpr char dimensionLookupValidHandlesAfterResignationContractScenario[] =
    "cpp-tck.dimension-lookup-valid-handles-after-resignation-contract";
constexpr char updateRateLookupValidInputsAfterResignationScenario[] =
    "cpp-tck.update-rate-lookup-valid-inputs-after-resignation";
constexpr char updateRateLookupValidInputsAfterResignationContractScenario[] =
    "cpp-tck.update-rate-lookup-valid-inputs-after-resignation-contract";
constexpr char supportLookupValidInputsAfterResignationScenario[] =
    "cpp-tck.support-lookup-valid-inputs-after-resignation";
constexpr char supportLookupValidInputsAfterResignationContractScenario[] =
    "cpp-tck.support-lookup-valid-inputs-after-resignation-contract";
constexpr char supportSwitchGetterLifecycleBoundariesScenario[] =
    "cpp-tck.support-switch-getter-lifecycle-boundaries";
constexpr char supportSwitchGetterLifecycleBoundariesContractScenario[] =
    "cpp-tck.support-switch-getter-lifecycle-boundaries-contract";
constexpr char supportSwitchSetterLifecycleBoundariesScenario[] =
    "cpp-tck.support-switch-setter-lifecycle-boundaries";
constexpr char supportSwitchSetterLifecycleBoundariesContractScenario[] =
    "cpp-tck.support-switch-setter-lifecycle-boundaries-contract";
constexpr char automaticResignDirectiveLifecycleBoundariesScenario[] =
    "cpp-tck.automatic-resign-directive-lifecycle-boundaries";
constexpr char automaticResignDirectiveLifecycleBoundariesContractScenario[] =
    "cpp-tck.automatic-resign-directive-lifecycle-boundaries-contract";
constexpr char automaticResignDirectiveEnumValuesScenario[] =
    "cpp-tck.automatic-resign-directive-enum-values";
constexpr char automaticResignDirectiveEnumValuesContractScenario[] =
    "cpp-tck.automatic-resign-directive-enum-values-contract";
constexpr char advisorySwitchValueRoundTripsScenario[] =
    "cpp-tck.advisory-switch-value-round-trips";
constexpr char advisorySwitchValueRoundTripsContractScenario[] =
    "cpp-tck.advisory-switch-value-round-trips-contract";
constexpr char duplicateFederateNameJoinScenario[] =
    "cpp-tck.duplicate-federate-name-join-boundary";
constexpr char duplicateFederateNameJoinContractScenario[] =
    "cpp-tck.duplicate-federate-name-join-boundary-contract";
constexpr char repeatedJoinByMemberScenario[] =
    "cpp-tck.repeated-join-by-member-boundary";
constexpr char repeatedJoinByMemberContractScenario[] =
    "cpp-tck.repeated-join-by-member-boundary-contract";
constexpr char foreignFederateNameLookupScenario[] =
    "cpp-tck.foreign-federate-name-lookup-boundary";
constexpr char foreignFederateNameLookupContractScenario[] =
    "cpp-tck.foreign-federate-name-lookup-boundary-contract";
constexpr char foreignObjectInstanceNameLookupScenario[] =
    "cpp-tck.foreign-object-instance-name-lookup-boundary";
constexpr char foreignObjectInstanceNameLookupContractScenario[] =
    "cpp-tck.foreign-object-instance-name-lookup-boundary-contract";
constexpr char foreignObjectInstanceHandleLookupScenario[] =
    "cpp-tck.foreign-object-instance-handle-lookup-boundary";
constexpr char foreignObjectInstanceHandleLookupContractScenario[] =
    "cpp-tck.foreign-object-instance-handle-lookup-boundary-contract";
constexpr char localDeleteObjectInstanceNameLookupScenario[] =
    "cpp-tck.local-delete-object-instance-name-lookup-boundary";
constexpr char localDeleteObjectInstanceNameLookupContractScenario[] =
    "cpp-tck.local-delete-object-instance-name-lookup-boundary-contract";
constexpr char remoteDeleteKnownObjectClassLookupScenario[] =
    "cpp-tck.remote-delete-known-object-class-lookup-boundary";
constexpr char remoteDeleteKnownObjectClassLookupContractScenario[] =
    "cpp-tck.remote-delete-known-object-class-lookup-boundary-contract";
constexpr char remoteDeleteObjectInstanceNameLookupScenario[] =
    "cpp-tck.remote-delete-object-instance-name-lookup-boundary";
constexpr char remoteDeleteObjectInstanceNameLookupContractScenario[] =
    "cpp-tck.remote-delete-object-instance-name-lookup-boundary-contract";
constexpr char resignDeleteObjectLookupScenario[] =
    "cpp-tck.resign-delete-object-lookup-boundary";
constexpr char resignDeleteObjectLookupContractScenario[] =
    "cpp-tck.resign-delete-object-lookup-boundary-contract";
constexpr char resignNoActionOwnedAttributesScenario[] =
    "cpp-tck.resign-no-action-owned-attributes-rejection-boundary";
constexpr char resignNoActionOwnedAttributesContractScenario[] =
    "cpp-tck.resign-no-action-owned-attributes-rejection-boundary-contract";
constexpr char resignDeleteObjectsThenDivestCrossObjectEffectsScenario[] =
    "cpp-tck.resign-delete-objects-then-divest-cross-object-effects";
constexpr char resignDeleteObjectsThenDivestCrossObjectEffectsContractScenario[] =
    "cpp-tck.resign-delete-objects-then-divest-cross-object-effects-contract";
constexpr char resignUnconditionallyDivestAttributesPreservesObjectsScenario[] =
    "cpp-tck.resign-unconditionally-divest-attributes-preserves-objects";
constexpr char resignUnconditionallyDivestAttributesPreservesObjectsContractScenario[] =
    "cpp-tck.resign-unconditionally-divest-attributes-preserves-objects-contract";
constexpr char resignCancelPendingOwnershipAcquisitionsScenario[] =
    "cpp-tck.resign-cancel-pending-ownership-acquisitions-action";
constexpr char resignCancelPendingOwnershipAcquisitionsContractScenario[] =
    "cpp-tck.resign-cancel-pending-ownership-acquisitions-action-contract";
constexpr char resignCancelThenDeleteThenDivestCrossObjectEffectsScenario[] =
    "cpp-tck.resign-cancel-then-delete-then-divest-cross-object-effects";
constexpr char resignCancelThenDeleteThenDivestCrossObjectEffectsContractScenario[] =
    "cpp-tck.resign-cancel-then-delete-then-divest-cross-object-effects-contract";
constexpr char standardOrderTransportLookupsAfterResignationScenario[] =
    "cpp-tck.standard-order-transport-lookups-after-resignation";
constexpr char standardOrderTransportLookupsAfterResignationContractScenario[] =
    "cpp-tck.standard-order-transport-lookups-after-resignation-contract";
constexpr char objectAttributeDeclarationsValidInputsAfterResignationScenario[] =
    "cpp-tck.object-attribute-declarations-valid-inputs-after-resignation";
constexpr char objectAttributeDeclarationsValidInputsAfterResignationContractScenario[] =
    "cpp-tck.object-attribute-declarations-valid-inputs-after-resignation-contract";
constexpr char interactionDeclarationsValidInputsAfterResignationScenario[] =
    "cpp-tck.interaction-declarations-valid-inputs-after-resignation";
constexpr char interactionDeclarationsValidInputsAfterResignationContractScenario[] =
    "cpp-tck.interaction-declarations-valid-inputs-after-resignation-contract";
constexpr char directedInteractionDeclarationsValidInputsAfterResignationScenario[] =
    "cpp-tck.directed-interaction-declarations-valid-inputs-after-resignation";
constexpr char directedInteractionDeclarationsValidInputsAfterResignationContractScenario[] =
    "cpp-tck.directed-interaction-declarations-valid-inputs-after-resignation-contract";
constexpr char interactionTransportationQueryValidInputsAfterResignationScenario[] =
    "cpp-tck.interaction-transportation-query-valid-inputs-after-resignation";
constexpr char interactionTransportationQueryValidInputsAfterResignationContractScenario[] =
    "cpp-tck.interaction-transportation-query-valid-inputs-after-resignation-contract";
constexpr char serviceReportReceiveOrderInteractionScenario[] =
    "cpp-tck.service-report-receive-order-interaction";
constexpr char serviceReportReceiveOrderInteractionContractScenario[] =
    "cpp-tck.service-report-receive-order-interaction-contract";
constexpr char serviceReportTimestampedDirectedInteractionScenario[] =
    "cpp-tck.service-report-timestamped-directed-interaction";
constexpr char serviceReportTimestampedDirectedInteractionContractScenario[] =
    "cpp-tck.service-report-timestamped-directed-interaction-contract";
constexpr char serviceReportTimestampedAttributeUpdateScenario[] =
    "cpp-tck.service-report-timestamped-attribute-update";
constexpr char serviceReportTimestampedAttributeUpdateContractScenario[] =
    "cpp-tck.service-report-timestamped-attribute-update-contract";
constexpr char serviceReportTimestampedDeleteObjectInstanceScenario[] =
    "cpp-tck.service-report-timestamped-delete-object-instance";
constexpr char serviceReportTimestampedDeleteObjectInstanceContractScenario[] =
    "cpp-tck.service-report-timestamped-delete-object-instance-contract";
constexpr char serviceReportTimestampedInteractionScenario[] =
    "cpp-tck.service-report-timestamped-interaction";
constexpr char serviceReportTimestampedInteractionContractScenario[] =
    "cpp-tck.service-report-timestamped-interaction-contract";
constexpr char serviceReportTimestampedAttributeUpdateFailureScenario[] =
    "cpp-tck.service-report-timestamped-attribute-update-failure";
constexpr char serviceReportTimestampedAttributeUpdateFailureContractScenario[] =
    "cpp-tck.service-report-timestamped-attribute-update-failure-contract";
constexpr char serviceReportTimestampedInteractionFailureScenario[] =
    "cpp-tck.service-report-timestamped-interaction-failure";
constexpr char serviceReportTimestampedInteractionFailureContractScenario[] =
    "cpp-tck.service-report-timestamped-interaction-failure-contract";
constexpr char serviceReportTimestampedDeleteObjectInstanceFailureScenario[] =
    "cpp-tck.service-report-timestamped-delete-object-instance-failure";
constexpr char serviceReportTimestampedDeleteObjectInstanceFailureContractScenario[] =
    "cpp-tck.service-report-timestamped-delete-object-instance-failure-contract";
constexpr char serviceReportObjectAttributeUpdateRateLookupsScenario[] =
    "cpp-tck.service-report-object-attribute-update-rate-lookups";
constexpr char serviceReportObjectAttributeUpdateRateLookupsContractScenario[] =
    "cpp-tck.service-report-object-attribute-update-rate-lookups-contract";
constexpr char serviceReportObjectAttributeUpdateRateLookupFailuresScenario[] =
    "cpp-tck.service-report-object-attribute-update-rate-lookup-failures";
constexpr char serviceReportObjectAttributeUpdateRateLookupFailuresContractScenario[] =
    "cpp-tck.service-report-object-attribute-update-rate-lookup-failures-contract";
constexpr char serviceReportObjectAttributeDeclarationScenario[] =
    "cpp-tck.service-report-object-attribute-declaration";
constexpr char serviceReportObjectAttributeDeclarationContractScenario[] =
    "cpp-tck.service-report-object-attribute-declaration-contract";
constexpr char serviceReportDirectedInteractionDeclarationScenario[] =
    "cpp-tck.service-report-directed-interaction-declaration";
constexpr char serviceReportDirectedInteractionDeclarationContractScenario[] =
    "cpp-tck.service-report-directed-interaction-declaration-contract";
constexpr char serviceReportDirectedInteractionPublicationScenario[] =
    "cpp-tck.service-report-directed-interaction-publication";
constexpr char serviceReportDirectedInteractionPublicationContractScenario[] =
    "cpp-tck.service-report-directed-interaction-publication-contract";
constexpr char serviceReportInteractionPublicationScenario[] =
    "cpp-tck.service-report-interaction-publication";
constexpr char serviceReportInteractionPublicationContractScenario[] =
    "cpp-tck.service-report-interaction-publication-contract";
constexpr char serviceReportInteractionSubscriptionScenario[] =
    "cpp-tck.service-report-interaction-subscription";
constexpr char serviceReportInteractionSubscriptionContractScenario[] =
    "cpp-tck.service-report-interaction-subscription-contract";
constexpr char serviceReportAttributeUpdateScenario[] =
    "cpp-tck.service-report-attribute-update";
constexpr char serviceReportAttributeUpdateContractScenario[] =
    "cpp-tck.service-report-attribute-update-contract";
constexpr char serviceReportRequestAttributeValueUpdateScenario[] =
    "cpp-tck.service-report-request-attribute-value-update";
constexpr char serviceReportRequestAttributeValueUpdateContractScenario[] =
    "cpp-tck.service-report-request-attribute-value-update-contract";
constexpr char serviceReportRegisterObjectInstanceScenario[] =
    "cpp-tck.service-report-register-object-instance";
constexpr char serviceReportRegisterObjectInstanceContractScenario[] =
    "cpp-tck.service-report-register-object-instance-contract";
constexpr char serviceReportDeleteObjectInstanceScenario[] =
    "cpp-tck.service-report-delete-object-instance";
constexpr char serviceReportDeleteObjectInstanceContractScenario[] =
    "cpp-tck.service-report-delete-object-instance-contract";
constexpr char serviceReportInteractionFailureScenario[] =
    "cpp-tck.service-report-interaction-failure";
constexpr char serviceReportInteractionFailureContractScenario[] =
    "cpp-tck.service-report-interaction-failure-contract";
constexpr char serviceReportAttributeUpdateFailureScenario[] =
    "cpp-tck.service-report-attribute-update-failure";
constexpr char serviceReportAttributeUpdateFailureContractScenario[] =
    "cpp-tck.service-report-attribute-update-failure-contract";
constexpr char serviceReportDeleteObjectInstanceFailureScenario[] =
    "cpp-tck.service-report-delete-object-instance-failure";
constexpr char serviceReportDeleteObjectInstanceFailureContractScenario[] =
    "cpp-tck.service-report-delete-object-instance-failure-contract";
constexpr char serviceReportReserveObjectInstanceNameScenario[] =
    "cpp-tck.service-report-reserve-object-instance-name";
constexpr char serviceReportReserveObjectInstanceNameContractScenario[] =
    "cpp-tck.service-report-reserve-object-instance-name-contract";
constexpr char serviceReportReleaseObjectInstanceNameScenario[] =
    "cpp-tck.service-report-release-object-instance-name";
constexpr char serviceReportReleaseObjectInstanceNameContractScenario[] =
    "cpp-tck.service-report-release-object-instance-name-contract";
constexpr char serviceReportReleaseMultipleObjectInstanceNamesScenario[] =
    "cpp-tck.service-report-release-multiple-object-instance-names";
constexpr char serviceReportReleaseMultipleObjectInstanceNamesContractScenario[] =
    "cpp-tck.service-report-release-multiple-object-instance-names-contract";
constexpr char serviceReportQueryAttributeOwnershipScenario[] =
    "cpp-tck.service-report-query-attribute-ownership";
constexpr char serviceReportQueryAttributeOwnershipContractScenario[] =
    "cpp-tck.service-report-query-attribute-ownership-contract";
constexpr char serviceReportConfirmDivestitureScenario[] =
    "cpp-tck.service-report-confirm-divestiture";
constexpr char serviceReportConfirmDivestitureContractScenario[] =
    "cpp-tck.service-report-confirm-divestiture-contract";
constexpr char serviceReportCancelAttributeOwnershipAcquisitionScenario[] =
    "cpp-tck.service-report-cancel-attribute-ownership-acquisition";
constexpr char serviceReportCancelAttributeOwnershipAcquisitionContractScenario[] =
    "cpp-tck.service-report-cancel-attribute-ownership-acquisition-contract";
constexpr char serviceReportLocalDeleteObjectInstanceScenario[] =
    "cpp-tck.service-report-local-delete-object-instance";
constexpr char serviceReportLocalDeleteObjectInstanceContractScenario[] =
    "cpp-tck.service-report-local-delete-object-instance-contract";
constexpr char serviceReportLocalDeleteObjectInstanceFailureScenario[] =
    "cpp-tck.service-report-local-delete-object-instance-failure";
constexpr char serviceReportLocalDeleteObjectInstanceFailureContractScenario[] =
    "cpp-tck.service-report-local-delete-object-instance-failure-contract";
constexpr char serviceReportInterlockScenario[] =
    "cpp-tck.service-report-interlock";
constexpr char serviceReportInterlockContractScenario[] =
    "cpp-tck.service-report-interlock-contract";
constexpr char federateServiceReportingMomSwitchScenario[] =
    "cpp-tck.federate-service-reporting-mom-switch";
constexpr char federateServiceReportingMomSwitchContractId[] =
    "cpp-tck.federate-service-reporting-mom-switch-contract";
constexpr char federateServiceReportingMomAttributeScenario[] =
    "cpp-tck.federate-service-reporting-mim-attribute";
constexpr char federateServiceReportingMomAttributeContractId[] =
    "cpp-tck.federate-service-reporting-mim-attribute-contract";
constexpr char federateObjectClassRelevanceAdvisoryMomAttributeScenario[] =
    "cpp-tck.federate-object-class-relevance-advisory-mim-attribute";
constexpr char federateObjectClassRelevanceAdvisoryMomAttributeContractId[] =
    "cpp-tck.federate-object-class-relevance-advisory-mim-attribute-contract";
constexpr char federateAttributeRelevanceAdvisoryMomAttributeScenario[] =
    "cpp-tck.federate-attribute-relevance-advisory-mim-attribute";
constexpr char federateAttributeRelevanceAdvisoryMomAttributeContractId[] =
    "cpp-tck.federate-attribute-relevance-advisory-mim-attribute-contract";
constexpr char federateAttributeScopeAdvisoryMomAttributeScenario[] =
    "cpp-tck.federate-attribute-scope-advisory-mim-attribute";
constexpr char federateAttributeScopeAdvisoryMomAttributeContractId[] =
    "cpp-tck.federate-attribute-scope-advisory-mim-attribute-contract";
constexpr char federateInteractionRelevanceAdvisoryMomAttributeScenario[] =
    "cpp-tck.federate-interaction-relevance-advisory-mim-attribute";
constexpr char federateInteractionRelevanceAdvisoryMomAttributeContractId[] =
    "cpp-tck.federate-interaction-relevance-advisory-mim-attribute-contract";
constexpr char federateAsynchronousDeliveryMomAttributeScenario[] =
    "cpp-tck.federate-asynchronous-delivery-mim-attribute";
constexpr char federateAsynchronousDeliveryMomAttributeContractId[] =
    "cpp-tck.federate-asynchronous-delivery-mim-attribute-contract";
constexpr char federateTimeConstrainedMomAttributeScenario[] =
    "cpp-tck.federate-time-constrained-mim-attribute";
constexpr char federateTimeConstrainedMomAttributeContractId[] =
    "cpp-tck.federate-time-constrained-mim-attribute-contract";
constexpr char federateTimeRegulatingMomAttributeScenario[] =
    "cpp-tck.federate-time-regulating-mim-attribute";
constexpr char federateTimeRegulatingMomAttributeContractId[] =
    "cpp-tck.federate-time-regulating-mim-attribute-contract";
constexpr char federateLookaheadMomPeriodicScenario[] =
    "cpp-tck.federate-lookahead-mim-periodic";
constexpr char federateLookaheadMomPeriodicContractId[] =
    "cpp-tck.federate-lookahead-mim-periodic-contract";
constexpr char federateLogicalTimeMomPeriodicScenario[] =
    "cpp-tck.federate-logical-time-mim-periodic";
constexpr char federateLogicalTimeMomPeriodicContractId[] =
    "cpp-tck.federate-logical-time-mim-periodic-contract";
constexpr char federateTimeManagerStateMomAttributeScenario[] =
    "cpp-tck.federate-time-manager-state-mim-attribute";
constexpr char federateTimeManagerStateMomAttributeContractId[] =
    "cpp-tck.federate-time-manager-state-mim-attribute-contract";
constexpr char federateExceptionReportingMomSwitchScenario[] =
    "cpp-tck.federate-exception-reporting-mom-switch";
constexpr char federateExceptionReportingMomSwitchContractId[] =
    "cpp-tck.federate-exception-reporting-mom-switch-contract";
constexpr char federateExceptionReportingMomAttributeScenario[] =
    "cpp-tck.federate-exception-reporting-mim-attribute";
constexpr char federateExceptionReportingMomAttributeContractId[] =
    "cpp-tck.federate-exception-reporting-mim-attribute-contract";
constexpr char federateExceptionReportDeliveryScenario[] =
    "cpp-tck.federate-exception-report-delivery";
constexpr char federateExceptionReportDeliveryContractId[] =
    "cpp-tck.federate-exception-report-delivery-contract";
constexpr char federateMomExceptionReportDeliveryScenario[] =
    "cpp-tck.federate-mom-exception-report-delivery";
constexpr char federateMomExceptionReportDeliveryContractId[] =
    "cpp-tck.federate-mom-exception-report-delivery-contract";
constexpr char federateMomExceptionMissingParameterScenario[] =
    "cpp-tck.federate-mom-exception-missing-parameter";
constexpr char federateMomExceptionMissingParameterContractId[] =
    "cpp-tck.federate-mom-exception-missing-parameter-contract";
constexpr char federationMomExceptionMissingFomModuleIndicatorScenario[] =
    "cpp-tck.federation-mom-exception-missing-fom-module-indicator";
constexpr char federationMomExceptionMissingFomModuleIndicatorContractId[] =
    "cpp-tck.federation-mom-exception-missing-fom-module-indicator-contract";
constexpr char federateMomFomModuleContentReportScenario[] =
    "cpp-tck.federate-mom-fom-module-content-report";
constexpr char federateMomFomModuleContentReportContractId[] =
    "cpp-tck.federate-mom-fom-module-content-report-contract";
constexpr char federateMomObjectInstanceInformationScenario[] =
    "cpp-tck.federate-mom-object-instance-information";
constexpr char federateMomObjectInstanceInformationContractId[] =
    "cpp-tck.federate-mom-object-instance-information-contract";
constexpr char federateMomPublicationQueryScenario[] =
    "cpp-tck.federate-mom-publication-query";
constexpr char federateMomPublicationQueryContractId[] =
    "cpp-tck.federate-mom-publication-query-contract";
constexpr char federateMomCompleteObjectClassPublicationQueryScenario[] =
    "cpp-tck.federate-mom-complete-object-class-publication-query";
constexpr char federateMomCompleteObjectClassPublicationQueryContractId[] =
    "cpp-tck.federate-mom-complete-object-class-publication-query-contract";
constexpr char federateMomObjectClassAttributeUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-object-class-attribute-unpublication-query";
constexpr char federateMomObjectClassAttributeUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-object-class-attribute-unpublication-query-contract";
constexpr char federateMomObjectClassFullAttributeUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-object-class-full-attribute-unpublication-query";
constexpr char federateMomObjectClassFullAttributeUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-object-class-full-attribute-unpublication-query-contract";
constexpr char federateMomObjectClassWholeUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-object-class-whole-unpublication-query";
constexpr char federateMomObjectClassWholeUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-object-class-whole-unpublication-query-contract";
constexpr char federateMomInteractionClassWholeUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-interaction-class-whole-unpublication-query";
constexpr char federateMomInteractionClassWholeUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-interaction-class-whole-unpublication-query-contract";
constexpr char federateMomInteractionClassPartialUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-interaction-class-partial-unpublication-query";
constexpr char federateMomInteractionClassPartialUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-interaction-class-partial-unpublication-query-contract";
constexpr char federateMomMultipleInteractionClassFullUnpublicationQueryScenario[] =
    "cpp-tck.federate-mom-multiple-interaction-class-full-unpublication-query";
constexpr char federateMomMultipleInteractionClassFullUnpublicationQueryContractId[] =
    "cpp-tck.federate-mom-multiple-interaction-class-full-unpublication-query-contract";
constexpr char federateMomMultipleDirectedPublicationQueryScenario[] =
    "cpp-tck.federate-mom-multiple-directed-publication-query";
constexpr char federateMomMultipleDirectedPublicationQueryContractId[] =
    "cpp-tck.federate-mom-multiple-directed-publication-query-contract";
constexpr char federateMomMultipleDirectedPublicationPartialUnpublishQueryScenario[] =
    "cpp-tck.federate-mom-multiple-directed-publication-partial-unpublish-query";
constexpr char federateMomMultipleDirectedPublicationPartialUnpublishQueryContractId[] =
    "cpp-tck.federate-mom-multiple-directed-publication-partial-unpublish-query-contract";
constexpr char federateMomMultipleDirectedPublicationFullUnpublishQueryScenario[] =
    "cpp-tck.federate-mom-multiple-directed-publication-full-unpublish-query";
constexpr char federateMomMultipleDirectedPublicationFullUnpublishQueryContractId[] =
    "cpp-tck.federate-mom-multiple-directed-publication-full-unpublish-query-contract";
constexpr char federateMomSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-subscription-query";
constexpr char federateMomSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-subscription-query-contract";
constexpr char federateMomPassiveSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-passive-subscription-query";
constexpr char federateMomPassiveSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-passive-subscription-query-contract";
constexpr char federateMomPassiveInteractionSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-passive-interaction-subscription-query";
constexpr char federateMomPassiveInteractionSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-passive-interaction-subscription-query-contract";
constexpr char federateMomPassiveInteractionSubscriptionActivationQueryScenario[] =
    "cpp-tck.federate-mom-passive-interaction-subscription-activation-query";
constexpr char federateMomPassiveInteractionSubscriptionActivationQueryContractId[] =
    "cpp-tck.federate-mom-passive-interaction-subscription-activation-query-contract";
constexpr char federateMomMultipleInteractionSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-query";
constexpr char federateMomMultipleInteractionSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-query-contract";
constexpr char federateMomMultipleInteractionSubscriptionUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-unsubscribe-query";
constexpr char federateMomMultipleInteractionSubscriptionUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-unsubscribe-query-contract";
constexpr char federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-full-unsubscribe-query";
constexpr char federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-multiple-interaction-subscription-full-unsubscribe-query-contract";
constexpr char federateMomObjectAttributePartialUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-object-attribute-partial-unsubscribe-query";
constexpr char federateMomObjectAttributePartialUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-object-attribute-partial-unsubscribe-query-contract";
constexpr char federateMomObjectAttributeFullUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-object-attribute-full-unsubscribe-query";
constexpr char federateMomObjectAttributeFullUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-object-attribute-full-unsubscribe-query-contract";
constexpr char federateMomObjectClassFullUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-object-class-full-unsubscribe-query";
constexpr char federateMomObjectClassFullUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-object-class-full-unsubscribe-query-contract";
constexpr char federateMomObjectClassCompleteAttributeSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-object-class-complete-attribute-subscription-query";
constexpr char federateMomObjectClassCompleteAttributeSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-object-class-complete-attribute-subscription-query-contract";
constexpr char federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-object-class-passive-complete-attribute-subscription-query";
constexpr char federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-object-class-passive-complete-attribute-subscription-query-contract";
constexpr char federateMomObjectClassCompleteAttributeSubscriptionActivationQueryScenario[] =
    "cpp-tck.federate-mom-object-class-complete-attribute-subscription-activation-query";
constexpr char federateMomObjectClassCompleteAttributeSubscriptionActivationQueryContractId[] =
    "cpp-tck.federate-mom-object-class-complete-attribute-subscription-activation-query-contract";
constexpr char federateMomEmptySubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-empty-subscription-query";
constexpr char federateMomEmptySubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-empty-subscription-query-contract";
constexpr char federateMomDirectedSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-directed-subscription-query";
constexpr char federateMomDirectedSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-directed-subscription-query-contract";
constexpr char federateMomMultipleDirectedSubscriptionQueryScenario[] =
    "cpp-tck.federate-mom-multiple-directed-subscription-query";
constexpr char federateMomMultipleDirectedSubscriptionQueryContractId[] =
    "cpp-tck.federate-mom-multiple-directed-subscription-query-contract";
constexpr char federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenario[] =
    "cpp-tck.federate-mom-multiple-directed-subscription-full-unsubscribe-query";
constexpr char federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryContractId[] =
    "cpp-tck.federate-mom-multiple-directed-subscription-full-unsubscribe-query-contract";
constexpr char federateMomExceptionReportServicePreconditionScenario[] =
    "cpp-tck.federate-mom-exception-report-service-precondition";
constexpr char federateMomExceptionReportServicePreconditionContractId[] =
    "cpp-tck.federate-mom-exception-report-service-precondition-contract";
constexpr char federateSendServiceReportsToFileMomSwitchScenario[] =
    "cpp-tck.federate-send-service-reports-to-file-mom-switch";
constexpr char federateSendServiceReportsToFileMomSwitchContractId[] =
    "cpp-tck.federate-send-service-reports-to-file-mom-switch-contract";
constexpr char federateAutomaticResignActionMomSwitchScenario[] =
    "cpp-tck.federate-automatic-resign-action-mom-switch";
constexpr char federateAutomaticResignActionMomSwitchContractId[] =
    "cpp-tck.federate-automatic-resign-action-mom-switch-contract";
constexpr char federateAutomaticResignActionMomAttributeScenario[] =
    "cpp-tck.federate-automatic-resign-action-mim-attribute";
constexpr char federateAutomaticResignActionMomAttributeContractId[] =
    "cpp-tck.federate-automatic-resign-action-mim-attribute-contract";
constexpr char serviceReportSynchronizationScenario[] =
    "cpp-tck.service-report-synchronization";
constexpr char serviceReportSynchronizationContractScenario[] =
    "cpp-tck.service-report-synchronization-contract";
constexpr char serviceReportFederateSaveBegunScenario[] =
    "cpp-tck.service-report-federate-save-begun";
constexpr char serviceReportFederateSaveBegunContractScenario[] =
    "cpp-tck.service-report-federate-save-begun-contract";
constexpr char automaticResignDirectiveDeleteObjectsId[] =
    "cpp-tck.automatic-resign-directive-delete-objects";
constexpr char automaticResignDirectiveDeleteObjectsContractId[] =
    "cpp-tck.automatic-resign-directive-delete-objects-contract";
constexpr char publicHandleDecodingId[] = "cpp-tck.public-handle-decoding";
constexpr char publicHandleDecodingContractId[] =
    "cpp-tck.public-handle-decoding-contract";
constexpr char customTransportationAttributeDeliveryId[] =
    "cpp-tck.custom-transportation-attribute-delivery";
constexpr char customTransportationAttributeDeliveryContractId[] =
    "cpp-tck.custom-transportation-attribute-delivery-contract";
constexpr char customTransportationTimestampedAttributeDeliveryId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-delivery";
constexpr char customTransportationTimestampedAttributeDeliveryContractId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-delivery-contract";
constexpr char customTransportationTimestampedAttributeAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances";
constexpr char customTransportationTimestampedAttributeAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-attribute-alternate-advances-contract";
constexpr char customTransportationTimestampedInteractionAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances";
constexpr char customTransportationTimestampedInteractionAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-interaction-alternate-advances-contract";
constexpr char customTransportationDirectedInteractionDeliveryId[] =
    "cpp-tck.custom-transportation-directed-interaction-delivery";
constexpr char customTransportationDirectedInteractionDeliveryContractId[] =
    "cpp-tck.custom-transportation-directed-interaction-delivery-contract";
constexpr char customTransportationTimestampedDirectedInteractionAlternateAdvancesId[] =
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances";
constexpr char customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId[] =
    "cpp-tck.custom-transportation-timestamped-directed-interaction-alternate-advances-contract";
constexpr char customTransportationTimestampedRegionalInteractionDeliveryId[] =
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery";
constexpr char customTransportationTimestampedRegionalInteractionDeliveryContractId[] =
    "cpp-tck.custom-transportation-timestamped-regional-interaction-delivery-contract";
constexpr char delaySubscriptionEvaluationInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-interaction";
constexpr char delaySubscriptionEvaluationInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-interaction-contract";
constexpr char delaySubscriptionEvaluationDirectedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-directed-interaction";
constexpr char delaySubscriptionEvaluationDirectedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-directed-interaction-contract";
constexpr char delaySubscriptionEvaluationAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-attribute-update";
constexpr char delaySubscriptionEvaluationAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-attribute-update-contract";
constexpr char delaySubscriptionEvaluationTimestampedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-interaction";
constexpr char delaySubscriptionEvaluationTimestampedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-interaction-contract";
constexpr char delaySubscriptionEvaluationTimestampedDirectedInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction";
constexpr char delaySubscriptionEvaluationTimestampedDirectedInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction-contract";
constexpr char delaySubscriptionEvaluationTimestampedAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-attribute-update";
constexpr char delaySubscriptionEvaluationTimestampedAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-attribute-update-contract";
constexpr char delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-regional-attribute-update";
constexpr char delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateScenario[] =
    "cpp-tck.delay-subscription-evaluation-receive-order-regional-attribute-update";
constexpr char delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-receive-order-regional-attribute-update-contract";
constexpr char delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-regional-attribute-update-contract";
constexpr char delaySubscriptionEvaluationTimestampedRegionalInteractionScenario[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-regional-interaction";
constexpr char delaySubscriptionEvaluationTimestampedRegionalInteractionContractId[] =
    "cpp-tck.delay-subscription-evaluation-timestamped-regional-interaction-contract";
constexpr char federationSaveRestoreInterlocksScenario[] =
    "cpp-tck.federation-save-restore-interlocks";
constexpr char federationSaveRestoreInterlocksContractId[] =
    "cpp-tck.federation-save-restore-interlocks-contract";
constexpr char timedFederationSaveRestoreScenario[] =
    "cpp-tck.timed-federation-save-restore";
constexpr char timedFederationSaveRestoreContractId[] =
    "cpp-tck.timed-federation-save-restore-contract";
constexpr char fqrTimedFederationSaveCallbackScenario[] =
    "cpp-tck.fqr-timed-federation-save-callback";
constexpr char fqrTimedFederationSaveCallbackContractId[] =
    "cpp-tck.fqr-timed-federation-save-callback-contract";
constexpr char timedRegionalInteractionSaveRestoreScenario[] =
    "cpp-tck.timed-regional-interaction-save-restore";
constexpr char timedRegionalInteractionSaveRestoreContractId[] =
    "cpp-tck.timed-regional-interaction-save-restore-contract";
constexpr char timedDefaultRegionInteractionSaveRestoreScenario[] =
    "cpp-tck.timed-default-region-interaction-save-restore";
constexpr char timedDefaultRegionInteractionSaveRestoreContractId[] =
    "cpp-tck.timed-default-region-interaction-save-restore-contract";
constexpr char timedDefaultRegionAttributeSaveRestoreScenario[] =
    "cpp-tck.timed-default-region-attribute-save-restore";
constexpr char timedDefaultRegionAttributeSaveRestoreContractId[] =
    "cpp-tck.timed-default-region-attribute-save-restore-contract";
constexpr char allowRelaxedDdmScenario[] = "cpp-tck.allow-relaxed-ddm";
constexpr char allowRelaxedDdmContractId[] =
    "cpp-tck.allow-relaxed-ddm-contract";
constexpr char regionalMultiAttributeUpdateScenario[] =
    "cpp-tck.regional-multi-attribute-update";
constexpr char regionalMultiAttributeUpdateContractId[] =
    "cpp-tck.regional-multi-attribute-update-contract";
constexpr char regionalThreeDimensionalOverlapScenario[] =
    "cpp-tck.regional-three-dimensional-overlap";
constexpr char regionalThreeDimensionalOverlapContractId[] =
    "cpp-tck.regional-three-dimensional-overlap-contract";
constexpr char regionalAttributeUpdateCallbackDdmRecheckScenario[] =
    "cpp-tck.regional-attribute-update-callback-ddm-recheck";
constexpr char regionalAttributeUpdateCallbackDdmRecheckContractScenario[] =
    "cpp-tck.regional-attribute-update-callback-ddm-recheck-contract";
constexpr char regionalAttributeValueRequestFilteringScenario[] =
    "cpp-tck.regional-attribute-value-request-filtering";
constexpr char regionalAttributeValueRequestFilteringContractScenario[] =
    "cpp-tck.regional-attribute-value-request-filtering-contract";
constexpr char regionalAttributeValueUpdateResponseRecheckScenario[] =
    "cpp-tck.regional-attribute-value-update-response-recheck";
constexpr char regionalAttributeValueUpdateResponseRecheckContractScenario[] =
    "cpp-tck.regional-attribute-value-update-response-recheck-contract";
constexpr char regionalBoundariesScenario[] = "cpp-tck.regional-boundaries";
constexpr char regionalBoundariesContractId[] =
    "cpp-tck.regional-boundaries-contract";
constexpr char ownershipTransferRegionalUpdateScenario[] =
    "cpp-tck.ownership-transfer-regional-update";
constexpr char ownershipTransferRegionalUpdateContractId[] =
    "cpp-tck.ownership-transfer-regional-update-contract";
constexpr char momTransportationTypeChangeRequestScenario[] =
    "cpp-tck.mom-transportation-type-change-request";
constexpr char momTransportationTypeChangeRequestContractId[] =
    "cpp-tck.mom-transportation-type-change-request-contract";
constexpr char joinedFederateMomRegisteredObjectCountScenario[] =
    "cpp-tck.joined-federate-mom-registered-object-count";
constexpr char joinedFederateMomRegisteredObjectCountContractId[] =
    "cpp-tck.joined-federate-mom-registered-object-count-contract";
constexpr char joinedFederateMomObjectLifecycleScenario[] =
    "cpp-tck.joined-federate-mom-object-lifecycle";
constexpr char joinedFederateMomObjectLifecycleContractId[] =
    "cpp-tck.joined-federate-mom-object-lifecycle-contract";
constexpr char joinedFederateMomDeletableObjectReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report";
constexpr char joinedFederateMomDeletableObjectReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-that-can-be-deleted-report-contract";
constexpr char joinedFederateMomObjectInstancesUpdatedReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-report";
constexpr char joinedFederateMomObjectInstancesUpdatedReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-report-contract";
constexpr char joinedFederateMomObjectInstancesUpdatedTimestampedReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-timestamped-report";
constexpr char joinedFederateMomObjectInstancesUpdatedTimestampedReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-timestamped-report-contract";
constexpr char joinedFederateMomObjectInstancesUpdatedMultiClassReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-multi-class-report";
constexpr char joinedFederateMomObjectInstancesUpdatedMultiClassReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-multi-class-report-contract";
constexpr char joinedFederateMomObjectInstancesUpdatedNullReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-null-report";
constexpr char joinedFederateMomObjectInstancesUpdatedNullReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-null-report-contract";
constexpr char joinedFederateMomObjectInstancesReflectedNullReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-null-report";
constexpr char joinedFederateMomObjectInstancesReflectedNullReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-null-report-contract";
constexpr char joinedFederateMomObjectInstancesReflectedReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-report";
constexpr char joinedFederateMomObjectInstancesReflectedReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-report-contract";
constexpr char joinedFederateMomObjectInstancesReflectedTimestampedReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-timestamped-report";
constexpr char joinedFederateMomObjectInstancesReflectedTimestampedReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-timestamped-report-contract";
constexpr char joinedFederateMomObjectInstancesReflectedMultiClassReportScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-multi-class-report";
constexpr char joinedFederateMomObjectInstancesReflectedMultiClassReportContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-multi-class-report-contract";
constexpr char joinedFederateMomDeletableObjectCountScenario[] =
    "cpp-tck.joined-federate-mom-deletable-object-count";
constexpr char joinedFederateMomDeletableObjectCountContractId[] =
    "cpp-tck.joined-federate-mom-deletable-object-count-contract";
constexpr char joinedFederateMomRoLengthPeriodicScenario[] =
    "cpp-tck.joined-federate-mom-ro-length-periodic";
constexpr char joinedFederateMomRoLengthPeriodicContractId[] =
    "cpp-tck.joined-federate-mom-ro-length-periodic-contract";
constexpr char joinedFederateMomTsoLengthPeriodicScenario[] =
    "cpp-tck.joined-federate-mom-tso-length-periodic";
constexpr char joinedFederateMomTsoLengthPeriodicContractId[] =
    "cpp-tck.joined-federate-mom-tso-length-periodic-contract";
constexpr char joinedFederateMomInteractionsReceivedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-interactions-received-mim-attribute";
constexpr char joinedFederateMomInteractionsReceivedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-interactions-received-mim-attribute-contract";
constexpr char joinedFederateMomInteractionsSentAttributeScenario[] =
    "cpp-tck.joined-federate-mom-interactions-sent-mim-attribute";
constexpr char joinedFederateMomInteractionsSentAttributeContractId[] =
    "cpp-tck.joined-federate-mom-interactions-sent-mim-attribute-contract";
constexpr char joinedFederateMomDirectedInteractionsReceivedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-directed-interactions-received-mim-attribute";
constexpr char joinedFederateMomDirectedInteractionsReceivedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-directed-interactions-received-mim-attribute-contract";
constexpr char joinedFederateMomDirectedInteractionsSentAttributeScenario[] =
    "cpp-tck.joined-federate-mom-directed-interactions-sent-mim-attribute";
constexpr char joinedFederateMomDirectedInteractionsSentAttributeContractId[] =
    "cpp-tck.joined-federate-mom-directed-interactions-sent-mim-attribute-contract";
constexpr char joinedFederateMomUpdatesSentAttributeScenario[] =
    "cpp-tck.joined-federate-mom-updates-sent-mim-attribute";
constexpr char joinedFederateMomUpdatesSentAttributeContractId[] =
    "cpp-tck.joined-federate-mom-updates-sent-mim-attribute-contract";
constexpr char joinedFederateMomObjectInstancesUpdatedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-mim-attribute";
constexpr char joinedFederateMomObjectInstancesUpdatedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-updated-mim-attribute-contract";
constexpr char joinedFederateMomObjectInstancesReflectedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-mim-attribute";
constexpr char joinedFederateMomObjectInstancesReflectedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-reflected-mim-attribute-contract";
constexpr char joinedFederateMomReflectionsReceivedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-reflections-received-mim-attribute";
constexpr char joinedFederateMomReflectionsReceivedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-reflections-received-mim-attribute-contract";
constexpr char joinedFederateMomObjectInstancesDiscoveredAttributeScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-discovered-mim-attribute";
constexpr char joinedFederateMomObjectInstancesDiscoveredAttributeContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-discovered-mim-attribute-contract";
constexpr char joinedFederateMomObjectInstancesDeletedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-deleted-mim-attribute";
constexpr char joinedFederateMomObjectInstancesDeletedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-deleted-mim-attribute-contract";
constexpr char joinedFederateMomObjectInstancesRemovedAttributeScenario[] =
    "cpp-tck.joined-federate-mom-object-instances-removed-mim-attribute";
constexpr char joinedFederateMomObjectInstancesRemovedAttributeContractId[] =
    "cpp-tck.joined-federate-mom-object-instances-removed-mim-attribute-contract";
constexpr char receiveOrderAttributeUpdateScenario[] =
    "cpp-tck.receive-order-attribute-update";
constexpr char receiveOrderAttributeUpdateContractId[] =
    "cpp-tck.receive-order-attribute-update-contract";
constexpr char attributeUpdateOrderClassificationScenario[] =
    "cpp-tck.attribute-update-order-classification";
constexpr char attributeUpdateOrderClassificationContractId[] =
    "cpp-tck.attribute-update-order-classification-contract";
constexpr char receiveOrderInteractionScenario[] =
    "cpp-tck.receive-order-interaction";
constexpr char receiveOrderInteractionContractId[] =
    "cpp-tck.receive-order-interaction-contract";
constexpr char interactionOrderClassificationScenario[] =
    "cpp-tck.interaction-order-classification";
constexpr char interactionOrderClassificationContractId[] =
    "cpp-tck.interaction-order-classification-contract";
constexpr char interactionPublicationSendTransitionScenario[] =
    "cpp-tck.interaction-publication-send-transition";
constexpr char interactionPublicationSendTransitionContractId[] =
    "cpp-tck.interaction-publication-send-transition-contract";
constexpr char interactionActiveAndPassiveSubscriptionModesScenario[] =
    "cpp-tck.interaction-active-and-passive-subscription-modes";
constexpr char interactionActiveAndPassiveSubscriptionModesContractId[] =
    "cpp-tck.interaction-active-and-passive-subscription-modes-contract";
constexpr char receiveOrderObjectRemovalScenario[] =
    "cpp-tck.receive-order-object-removal";
constexpr char receiveOrderObjectRemovalContractId[] =
    "cpp-tck.receive-order-object-removal-contract";
constexpr char objectRemovalOrderClassificationScenario[] =
    "cpp-tck.object-removal-order-classification";
constexpr char objectRemovalOrderClassificationContractId[] =
    "cpp-tck.object-removal-order-classification-contract";
constexpr char javaTimestampedObjectRemovalScenario[] =
    "java-tck.timestamped-object-removal";
constexpr char federationRestoreAbortScenario[] =
    "cpp-tck.federation-restore-abort";
constexpr char federationRestoreAbortContractId[] =
    "cpp-tck.federation-restore-abort-contract";
constexpr char federationRestoreOwnershipAssumptionScenario[] =
    "cpp-tck.federation-restore-work-item-ownership-assumption";
constexpr char federationRestoreOwnershipAssumptionContractId[] =
    "cpp-tck.federation-restore-work-item-ownership-assumption-contract";
constexpr char pendingNegotiatedRestorePackageScenario[] =
    "cpp-tck.pending-negotiated-ownership-package-filesystem-restore";
constexpr char serviceReportCreationFailureScenario[] =
    "cpp-tck.service-report-file-creation-failure";
constexpr char serviceReportConnectFailureScenario[] =
    "cpp-tck.service-report-file-connect-failure";
constexpr char ownershipAcquisitionIfAvailableScenario[] =
    "cpp-tck.ownership-acquisition-if-available";
constexpr char ownershipAcquisitionIfAvailableContractId[] =
    "cpp-tck.ownership-acquisition-if-available-contract";
constexpr char attributeOwnershipAcquisitionCancellationScenario[] =
    "cpp-tck.attribute-ownership-acquisition-cancellation";
constexpr char attributeOwnershipAcquisitionCancellationContractId[] =
    "cpp-tck.attribute-ownership-acquisition-cancellation-contract";
constexpr char negotiatedDivestitureConfirmationFlowScenario[] =
    "cpp-tck.negotiated-divestiture-confirmation-flow";
constexpr char negotiatedDivestitureConfirmationFlowContractId[] =
    "cpp-tck.negotiated-divestiture-confirmation-flow-contract";
constexpr char attributeOwnershipReleaseDeniedScenario[] =
    "cpp-tck.attribute-ownership-release-denied";
constexpr char attributeOwnershipReleaseDeniedContractId[] =
    "cpp-tck.attribute-ownership-release-denied-contract";
constexpr char attributeOwnershipDivestitureIfWantedScenario[] =
    "cpp-tck.attribute-ownership-divestiture-if-wanted";
constexpr char attributeOwnershipDivestitureIfWantedContractId[] =
    "cpp-tck.attribute-ownership-divestiture-if-wanted-contract";
constexpr char attributeOwnershipQueryOwnerReportScenario[] =
    "cpp-tck.attribute-ownership-query-owner-report";
constexpr char attributeOwnershipQueryOwnerReportContractId[] =
    "cpp-tck.attribute-ownership-query-owner-report-contract";
constexpr char attributeOwnershipQueryUnownedResultScenario[] =
    "cpp-tck.attribute-ownership-query-unowned-result";
constexpr char attributeOwnershipQueryUnownedResultContractId[] =
    "cpp-tck.attribute-ownership-query-unowned-result-contract";
constexpr char attributeOwnershipQueryRtiOwnedResultScenario[] =
    "cpp-tck.attribute-ownership-query-rti-owned-result";
constexpr char attributeOwnershipQueryRtiOwnedResultContractId[] =
    "cpp-tck.attribute-ownership-query-rti-owned-result-contract";
constexpr char timestampedDirectedInteractionRetractionScenario[] =
    "cpp-tck.timestamped-directed-interaction-retraction";
constexpr char timestampedDirectedInteractionRetractionContractId[] =
    "cpp-tck.timestamped-directed-interaction-retraction-contract";
constexpr char timestampedDirectedInteractionRetractionFanoutScenario[] =
    "cpp-tck.timestamped-directed-interaction-retraction-fanout";
constexpr char timestampedDirectedInteractionRetractionFanoutContractId[] =
    "cpp-tck.timestamped-directed-interaction-retraction-fanout-contract";
constexpr char timestampedInteractionsScenario[] =
    "cpp-tck.timestamped-interactions";
constexpr char timestampedInteractionsContractId[] =
    "cpp-tck.timestamped-interactions-contract";
constexpr char unregulatedSenderTimestampedInteractionScenario[] =
    "cpp-tck.unregulated-sender-timestamped-interaction";
constexpr char unregulatedSenderTimestampedInteractionContractId[] =
    "cpp-tck.unregulated-sender-timestamped-interaction-contract";
constexpr char timestampedInteractionPreferredReceiveConversionScenario[] =
    "cpp-tck.timestamped-interaction-preferred-receive-conversion";
constexpr char timestampedInteractionPreferredReceiveConversionContractId[] =
    "cpp-tck.timestamped-interaction-preferred-receive-conversion-contract";
constexpr char timestampedInteractionSenderRegulationTransitionScenario[] =
    "cpp-tck.timestamped-interaction-sender-regulation-transition";
constexpr char timestampedInteractionSenderRegulationTransitionContractId[] =
    "cpp-tck.timestamped-interaction-sender-regulation-transition-contract";
constexpr char interactionFddPreferredOrderDefaultScenario[] =
    "cpp-tck.interaction-fdd-preferred-order-default";
constexpr char interactionFddPreferredOrderDefaultContractId[] =
    "cpp-tck.interaction-fdd-preferred-order-default-contract";
constexpr char attributeFddPreferredOrderDefaultScenario[] =
    "cpp-tck.attribute-fdd-preferred-order-default";
constexpr char attributeFddPreferredOrderDefaultContractId[] =
    "cpp-tck.attribute-fdd-preferred-order-default-contract";
constexpr char timestampedDeletePrivilegePreferredOrderScenario[] =
    "cpp-tck.timestamped-delete-privilege-preferred-order";
constexpr char timestampedDeletePrivilegePreferredOrderContractId[] =
    "cpp-tck.timestamped-delete-privilege-preferred-order-contract";
constexpr char timestampedDeletePrivilegeTimestampOrderScenario[] =
    "cpp-tck.timestamped-delete-privilege-timestamp-order";
constexpr char timestampedDeletePrivilegeTimestampOrderContractId[] =
    "cpp-tck.timestamped-delete-privilege-timestamp-order-contract";
constexpr char sentTsoPerRecipientOrderConversionScenario[] =
    "cpp-tck.sent-tso-per-recipient-order-conversion";
constexpr char sentTsoPerRecipientOrderConversionContractId[] =
    "cpp-tck.sent-tso-per-recipient-order-conversion-contract";
constexpr char timestampedInteractionSourceResignationScenario[] =
    "cpp-tck.timestamped-interaction-source-resignation";
constexpr char timestampedInteractionSourceResignationContractId[] =
    "cpp-tck.timestamped-interaction-source-resignation-contract";
constexpr char timestampedInteractionSourceResignationFanoutScenario[] =
    "cpp-tck.timestamped-interaction-source-resignation-fanout";
constexpr char timestampedInteractionSourceResignationFanoutContractId[] =
    "cpp-tck.timestamped-interaction-source-resignation-fanout-contract";
constexpr char timestampedInteractionTsoDesignatorTerminalizationScenario[] =
    "cpp-tck.timestamped-interaction-tso-designator-terminalization";
constexpr char timestampedInteractionTsoDesignatorTerminalizationContractId[] =
    "cpp-tck.timestamped-interaction-tso-designator-terminalization-contract";
constexpr char timestampedInteractionCrossProducerOrderScenario[] =
    "cpp-tck.timestamped-interaction-cross-producer-order";
constexpr char timestampedInteractionCrossProducerOrderContractId[] =
    "cpp-tck.timestamped-interaction-cross-producer-order-contract";
constexpr char timestampedInteractionNoFanoutScenario[] =
    "cpp-tck.timestamped-interaction-no-fanout";
constexpr char timestampedInteractionNoFanoutContractId[] =
    "cpp-tck.timestamped-interaction-no-fanout-contract";
constexpr char timestampedInteractionRetractionFanoutScenario[] =
    "cpp-tck.timestamped-interaction-retraction-fanout";
constexpr char timestampedInteractionRetractionFanoutContractId[] =
    "cpp-tck.timestamped-interaction-retraction-fanout-contract";
constexpr char timestampedInteractionReenableScenario[] =
    "cpp-tck.timestamped-interaction-reenable";
constexpr char timestampedInteractionReenableContractId[] =
    "cpp-tck.timestamped-interaction-reenable-contract";
constexpr char timestampedInteractionRegulationReenableScenario[] =
    "cpp-tck.timestamped-interaction-regulation-reenable";
constexpr char timestampedInteractionRegulationReenableContractId[] =
    "cpp-tck.timestamped-interaction-regulation-reenable-contract";
constexpr char timestampedDirectedInteractionRegulationReenableScenario[] =
    "cpp-tck.timestamped-directed-interaction-regulation-reenable";
constexpr char timestampedDirectedInteractionRegulationReenableContractId[] =
    "cpp-tck.timestamped-directed-interaction-regulation-reenable-contract";
constexpr char timestampedDirectedInteractionReenableScenario[] =
    "cpp-tck.timestamped-directed-interaction-reenable";
constexpr char timestampedDirectedInteractionReenableContractId[] =
    "cpp-tck.timestamped-directed-interaction-reenable-contract";
constexpr char timestampedAttributeUpdateScenario[] =
    "cpp-tck.timestamped-attribute-update";
constexpr char javaTimestampedAttributeUpdateScenario[] =
    "java-tck.timestamped-attribute-update";
constexpr char timestampedAttributeUpdateContractId[] =
    "cpp-tck.timestamped-attribute-update-contract";
constexpr char timestampedAttributeUpdateReenableScenario[] =
    "cpp-tck.timestamped-attribute-update-reenable";
constexpr char timestampedAttributeUpdateReenableContractId[] =
    "cpp-tck.timestamped-attribute-update-reenable-contract";
constexpr char timestampedAttributeUpdateRegulationReenableScenario[] =
    "cpp-tck.timestamped-attribute-update-regulation-reenable";
constexpr char timestampedAttributeUpdateRegulationReenableContractId[] =
    "cpp-tck.timestamped-attribute-update-regulation-reenable-contract";
constexpr char timestampedAttributeSourceResignationScenario[] =
    "cpp-tck.timestamped-attribute-source-resignation";
constexpr char timestampedAttributeSourceResignationContractId[] =
    "cpp-tck.timestamped-attribute-source-resignation-contract";
constexpr char timestampedAttributeSourceResignationFanoutScenario[] =
    "cpp-tck.timestamped-attribute-source-resignation-fanout";
constexpr char timestampedAttributeSourceResignationFanoutContractId[] =
    "cpp-tck.timestamped-attribute-source-resignation-fanout-contract";
constexpr char timestampedAttributeUpdateOwnershipTransferScenario[] =
    "cpp-tck.timestamped-attribute-update-ownership-transfer";
constexpr char timestampedAttributeUpdateOwnershipTransferContractId[] =
    "cpp-tck.timestamped-attribute-update-ownership-transfer-contract";
constexpr char timestampedAttributeUpdateQueuedPasselRetractionScenario[] =
    "cpp-tck.timestamped-attribute-update-queued-passel-retraction";
constexpr char timestampedAttributeUpdateQueuedPasselRetractionContractId[] =
    "cpp-tck.timestamped-attribute-update-queued-passel-retraction-contract";
constexpr char mixedOrderAttributePasselRetractionScenario[] =
    "cpp-tck.mixed-order-attribute-passel-retraction";
constexpr char mixedOrderAttributePasselRetractionContractId[] =
    "cpp-tck.mixed-order-attribute-passel-retraction-contract";
constexpr char timestampedAttributeImmediateRecipientMultiTransportRetractionScenario[] =
    "cpp-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction";
constexpr char timestampedAttributeImmediateRecipientMultiTransportRetractionContractId[] =
    "cpp-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction-contract";
constexpr char timestampedAttributeSenderRegulationTransitionScenario[] =
    "cpp-tck.timestamped-attribute-sender-regulation-transition";
constexpr char timestampedAttributeSenderRegulationTransitionContractId[] =
    "cpp-tck.timestamped-attribute-sender-regulation-transition-contract";
constexpr char timestampedAttributeUpdateNoFanoutScenario[] =
    "cpp-tck.timestamped-attribute-update-no-fanout";
constexpr char timestampedAttributeUpdateNoFanoutContractId[] =
    "cpp-tck.timestamped-attribute-update-no-fanout-contract";
constexpr char timestampedAttributeUpdateFlushQueueFutureInputScenario[] =
    "cpp-tck.timestamped-attribute-update-flush-queue-future-input";
constexpr char timestampedAttributeUpdateFlushQueueFutureInputContractId[] =
    "cpp-tck.timestamped-attribute-update-flush-queue-future-input-contract";
constexpr char timestampedInteractionFlushQueueFutureInputScenario[] =
    "cpp-tck.timestamped-interaction-flush-queue-future-input";
constexpr char timestampedInteractionFlushQueueFutureInputContractId[] =
    "cpp-tck.timestamped-interaction-flush-queue-future-input-contract";
constexpr char fqrPostgrantLowerTsoRetractionScenario[] =
    "cpp-tck.fqr-postgrant-lower-tso-retraction";
constexpr char fqrPostgrantLowerTsoRetractionContractId[] =
    "cpp-tck.fqr-postgrant-lower-tso-retraction-contract";
constexpr char fqgPostgrantDeliveryCutoffScenario[] =
    "cpp-tck.fqg-postgrant-delivery-cutoff";
constexpr char fqgPostgrantDeliveryCutoffContractId[] =
    "cpp-tck.fqg-postgrant-delivery-cutoff-contract";
constexpr char tarQueuedTsoBeforeGrantScenario[] =
    "cpp-tck.tar-queued-tso-before-grant";
constexpr char tarQueuedTsoBeforeGrantContractId[] =
    "cpp-tck.tar-queued-tso-before-grant-contract";
constexpr char timestampedAttributeOrderCohortScenario[] =
    "cpp-tck.timestamped-attribute-order-cohort";
constexpr char timestampedAttributeOrderCohortContractId[] =
    "cpp-tck.timestamped-attribute-order-cohort-contract";
constexpr char timestampedObjectDeletionNoFanoutScenario[] =
    "cpp-tck.timestamped-object-deletion-no-fanout";
constexpr char timestampedObjectDeletionNoFanoutContractId[] =
    "cpp-tck.timestamped-object-deletion-no-fanout-contract";
constexpr char timestampedObjectDeletionTombstoneScenario[] =
    "cpp-tck.timestamped-object-deletion-tombstone";
constexpr char timestampedObjectDeletionTombstoneContractId[] =
    "cpp-tck.timestamped-object-deletion-tombstone-contract";
constexpr char timestampedObjectDeletionRetractionJoinedOwnersScenario[] =
    "cpp-tck.timestamped-object-deletion-retraction-joined-owners";
constexpr char timestampedObjectDeletionRetractionJoinedOwnersContractId[] =
    "cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract";
constexpr char timestampedObjectDeletionSourceResignationFanoutScenario[] =
    "cpp-tck.timestamped-object-deletion-source-resignation-fanout";
constexpr char timestampedObjectDeletionSourceResignationFanoutContractId[] =
    "cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract";
constexpr char timestampedObjectDeletionScenario[] =
    "cpp-tck.timestamped-object-deletion";
constexpr char timestampedObjectDeletionContractId[] =
    "cpp-tck.timestamped-object-deletion-contract";
constexpr char timestampedObjectDeletionRegulationReenableScenario[] =
    "cpp-tck.timestamped-object-deletion-regulation-reenable";
constexpr char timestampedObjectDeletionRegulationReenableContractId[] =
    "cpp-tck.timestamped-object-deletion-regulation-reenable-contract";
constexpr char alternateTimeAdvancesScenario[] =
    "cpp-tck.alternate-time-advances";
constexpr char alternateTimeAdvancesContractId[] =
    "cpp-tck.alternate-time-advances-contract";
constexpr char pendingAlternateAdvanceStateScenario[] =
    "cpp-tck.pending-alternate-time-advance-state";
constexpr char pendingAlternateAdvanceStateContractId[] =
    "cpp-tck.pending-alternate-time-advance-state-contract";
constexpr char nextMessageRequestScenario[] =
    "cpp-tck.next-message-request";
constexpr char nextMessageRequestContractId[] =
    "cpp-tck.next-message-request-contract";
constexpr char nmrNoPregrantTsoFloorScenario[] =
    "cpp-tck.nmr-no-pregrant-tso-floor";
constexpr char nmrNoPregrantTsoFloorContractId[] =
    "cpp-tck.nmr-no-pregrant-tso-floor-contract";
constexpr char nmrReceivedTsoFloorScenario[] =
    "cpp-tck.nmr-received-tso-floor";
constexpr char nmrReceivedTsoFloorContractId[] =
    "cpp-tck.nmr-received-tso-floor-contract";
constexpr char nmrReceivedTsoAttributeUpdateFloorScenario[] =
    "cpp-tck.nmr-received-tso-attribute-update-floor";
constexpr char nmrReceivedTsoAttributeUpdateFloorContractId[] =
    "cpp-tck.nmr-received-tso-attribute-update-floor-contract";
constexpr char nmrReceivedTsoDirectedInteractionFloorScenario[] =
    "cpp-tck.nmr-received-tso-directed-interaction-floor";
constexpr char nmrReceivedTsoDirectedInteractionFloorContractId[] =
    "cpp-tck.nmr-received-tso-directed-interaction-floor-contract";
constexpr char nmrEmptyMessageSetGrantTimeScenario[] =
    "cpp-tck.nmr-empty-message-set-grant-time";
constexpr char nmrEmptyMessageSetGrantTimeContractId[] =
    "cpp-tck.nmr-empty-message-set-grant-time-contract";
constexpr char nmrRequestBelowGaltImmediateGrantScenario[] =
    "cpp-tck.nmr-request-below-galt-immediate-grant";
constexpr char nmrRequestBelowGaltImmediateGrantContractId[] =
    "cpp-tck.nmr-request-below-galt-immediate-grant-contract";
constexpr char nmrLitsBelowGaltImmediateGrantScenario[] =
    "cpp-tck.nmr-lits-below-galt-immediate-grant";
constexpr char nmrLitsBelowGaltImmediateGrantContractId[] =
    "cpp-tck.nmr-lits-below-galt-immediate-grant-contract";
constexpr char nmraNoPregrantTsoFloorScenario[] =
    "cpp-tck.nmra-no-pregrant-tso-floor";
constexpr char nmraNoPregrantTsoFloorContractId[] =
    "cpp-tck.nmra-no-pregrant-tso-floor-contract";
constexpr char sendMessageTimeStatePreservationGrantedScenario[] =
    "cpp-tck.send-message-time-state-preservation-granted";
constexpr char sendMessageTimeStatePreservationGrantedContractId[] =
    "cpp-tck.send-message-time-state-preservation-granted-contract";
constexpr char availableTimeAdvancesInclusiveGaltScenario[] =
    "cpp-tck.available-time-advances-inclusive-galt";
constexpr char availableTimeAdvancesInclusiveGaltContractId[] =
    "cpp-tck.available-time-advances-inclusive-galt-contract";
constexpr char timeConstrainedEnableAtGaltEqualityScenario[] =
    "cpp-tck.time-constrained-enable-galt-equality";
constexpr char timeConstrainedEnableAtGaltEqualityContractId[] =
    "cpp-tck.time-constrained-enable-galt-equality-contract";
constexpr char timeRegulationImplicitAdvanceScenario[] =
    "cpp-tck.time-regulation-implicit-advance";
constexpr char timeRegulationImplicitAdvanceContractId[] =
    "cpp-tck.time-regulation-implicit-advance-contract";
constexpr char timestampedInteractionMixedAdvancesScenario[] =
    "cpp-tck.timestamped-interaction-mixed-advances";
constexpr char timestampedInteractionMixedAdvancesContractId[] =
    "cpp-tck.timestamped-interaction-mixed-advances-contract";
constexpr char timeCoordinatedInteractionScenario[] =
    "cpp-tck.time-coordinated-interaction";
constexpr char emptyAttributeSubscriptionScenario[] =
    "cpp-tck.empty-attribute-subscription-is-no-op";
constexpr char namedUpdateRateSubscriptionScenario[] =
    "cpp-tck.named-update-rate-subscription";
constexpr char defaultRateNoReductionScenario[] =
    "cpp-tck.default-rate-subscription-no-reduction";
constexpr char accumulatingAttributeSubscriptionsScenario[] =
    "cpp-tck.attribute-subscription-invocations-accumulate";
constexpr char attributeSubscriptionModeTransitionsScenario[] =
    "cpp-tck.attribute-subscription-per-attribute-mode-transitions";
constexpr char attributeSubscriptionModeTransitionsContractId[] =
    "cpp-tck.attribute-subscription-per-attribute-mode-transitions-contract";
constexpr char passiveSubscriptionNoStartRegistrationScenario[] =
    "cpp-tck.passive-subscription-no-start-registration";
constexpr char selectiveAttributeUnsubscriptionScenario[] =
    "cpp-tck.selective-attribute-unsubscription";
constexpr char wholeClassAttributeUnsubscriptionScenario[] =
    "cpp-tck.whole-class-attribute-unsubscription";
constexpr char emptyAttributeUnsubscriptionScenario[] =
    "cpp-tck.empty-attribute-unsubscription-is-no-op";
constexpr char timestampedInteractionTaraEqualBoundaryScenario[] =
    "cpp-tck.timestamped-interaction-tara-equal-boundary";
constexpr char timestampedInteractionTaraEqualBoundaryContractId[] =
    "cpp-tck.timestamped-interaction-tara-equal-boundary-contract";
constexpr char timestampedInteractionTarEqualBoundaryScenario[] =
    "cpp-tck.timestamped-interaction-tar-equal-boundary";
constexpr char timestampedInteractionTarEqualBoundaryContractId[] =
    "cpp-tck.timestamped-interaction-tar-equal-boundary-contract";
constexpr char timestampedInteractionTarPostGrantCutoffScenario[] =
    "cpp-tck.timestamped-interaction-tar-post-grant-cutoff";
constexpr char timestampedInteractionTarPostGrantCutoffContractId[] =
    "cpp-tck.timestamped-interaction-tar-post-grant-cutoff-contract";
constexpr char timestampedAttributeTarPostGrantCutoffScenario[] =
    "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff";
constexpr char timestampedAttributeTarPostGrantCutoffContractId[] =
    "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff-contract";
constexpr char timestampedAttributeTimeAdvancingCutoffScenario[] =
    "cpp-tck.timestamped-attribute-update-time-advancing-cutoff";
constexpr char timestampedAttributeTimeAdvancingCutoffContractId[] =
    "cpp-tck.timestamped-attribute-update-time-advancing-cutoff-contract";
constexpr char timestampedAttributeReflectRegionSnapshotScenario[] =
    "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot";
constexpr char timestampedAttributeReflectRegionSnapshotContractId[] =
    "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot-contract";
constexpr char timestampedInteractionPublicationSendTransitionScenario[] =
    "cpp-tck.timestamped-interaction-publication-send-transition";
constexpr char timestampedInteractionPublicationSendTransitionContractId[] =
    "cpp-tck.timestamped-interaction-publication-send-transition-contract";
constexpr char timestampedDirectedAlternateAdvancesScenario[] =
    "cpp-tck.timestamped-directed-alternate-advances";
constexpr char timestampedDirectedAlternateAdvancesContractId[] =
    "cpp-tck.timestamped-directed-alternate-advances-contract";
constexpr char timestampedAttributeUpdateAlternateAdvancesScenario[] =
    "cpp-tck.timestamped-attribute-update-alternate-advances";
constexpr char timestampedAttributeUpdateAlternateAdvancesContractId[] =
    "cpp-tck.timestamped-attribute-update-alternate-advances-contract";
constexpr char timestampedObjectDeletionMixedAdvancesScenario[] =
    "cpp-tck.timestamped-object-deletion-mixed-advances";
constexpr char timestampedObjectDeletionMixedAdvancesContractId[] =
    "cpp-tck.timestamped-object-deletion-mixed-advances-contract";
constexpr char timestampedObjectDeletionFutureNmrArrivalScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival";
constexpr char timestampedObjectDeletionFutureNmrArrivalContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmr-arrival-contract";
constexpr char timestampedObjectDeletionFutureNmraArrivalScenario[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival";
constexpr char timestampedObjectDeletionFutureNmraArrivalContractId[] =
    "cpp-tck.timestamped-object-deletion-future-nmra-arrival-contract";
constexpr char timestampedDirectedInteractionTarNmrScenario[] =
    "cpp-tck.timestamped-directed-interaction-tar-nmr";
constexpr char timestampedDirectedInteractionTarNmrContractId[] =
    "cpp-tck.timestamped-directed-interaction-tar-nmr-contract";
constexpr char timestampedDirectedInteractionSourceResignationScenario[] =
    "cpp-tck.timestamped-directed-interaction-source-resignation";
constexpr char timestampedDirectedInteractionSourceResignationContractId[] =
    "cpp-tck.timestamped-directed-interaction-source-resignation-contract";
constexpr char timestampedDirectedInteractionSourceResignationFanoutScenario[] =
    "cpp-tck.timestamped-directed-interaction-source-resignation-fanout";
constexpr char timestampedDirectedInteractionSourceResignationFanoutContractId[] =
    "cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract";
constexpr char timestampedDirectedInteractionImmediateSourceResignationScenario[] =
    "cpp-tck.timestamped-directed-interaction-immediate-source-resignation";
constexpr char timestampedDirectedInteractionImmediateSourceResignationContractId[] =
    "cpp-tck.timestamped-directed-interaction-immediate-source-resignation-contract";
constexpr char modifyLookaheadScenario[] = "cpp-tck.modify-lookahead";
constexpr char modifyLookaheadContractId[] =
    "cpp-tck.modify-lookahead-contract";
constexpr char modifyLookaheadServiceReportContractId[] =
    "cpp-tck.modify-lookahead-service-report-contract";
constexpr char transportationTypeChangeScenario[] =
    "cpp-tck.transportation-type-change";
constexpr char transportationTypeChangeContractId[] =
    "cpp-tck.transportation-type-change-contract";
constexpr char orderTypeChangeScenario[] = "cpp-tck.order-type-change";
constexpr char orderTypeChangeContractId[] =
    "cpp-tck.order-type-change-contract";
constexpr char transportOrderScenario[] = "cpp-tck.transport-order";
constexpr char transportOrderContractId[] = "cpp-tck.transport-order-contract";
constexpr char standardOrderAndTransportationLookupsScenario[] =
    "cpp-tck.standard-order-and-transportation-lookups";
constexpr char standardOrderAndTransportationLookupsContractId[] =
    "cpp-tck.standard-order-and-transportation-lookups-contract";
constexpr char javaSynchronizationScenario[] = "java-tck.synchronization";
constexpr char synchronizationPointsScenario[] = "cpp-tck.synchronization-points";
constexpr char synchronizationPointContractScenario[] =
    "cpp-tck.synchronization-point-contract";
constexpr char synchronizationExplicitSetLateJoinScenario[] =
    "cpp-tck.synchronization-point-explicit-set-late-join";
constexpr char synchronizationExplicitSetLateJoinContractScenario[] =
    "cpp-tck.synchronization-point-explicit-set-late-join-contract";
constexpr char unconditionalAttributeOwnershipDivestitureScenario[] =
    "cpp-tck.unconditional-attribute-ownership-divestiture";
constexpr char unconditionalAttributeOwnershipDivestitureContractId[] =
    "cpp-tck.unconditional-attribute-ownership-divestiture-contract";
constexpr char autoProvideDisabledDiscoveryOnlyScenario[] =
    "cpp-tck.auto-provide-disabled-discovery-only";
constexpr char autoProvideDisabledDiscoveryOnlyContractId[] =
    "cpp-tck.auto-provide-disabled-discovery-only-contract";
constexpr char autoProvideDisabledExplicitRequestScenario[] =
    "cpp-tck.auto-provide-disabled-explicit-request";
constexpr char autoProvideDisabledExplicitRequestContractId[] =
    "cpp-tck.auto-provide-disabled-explicit-request-contract";
constexpr char federationAutoProvideMomSwitchScenario[] =
    "cpp-tck.federation-auto-provide-mom-switch";
constexpr char federationAutoProvideMomSwitchContractId[] =
    "cpp-tck.federation-auto-provide-mom-switch-contract";
constexpr char regionalAutoProvideResponseScenario[] =
    "cpp-tck.regional-auto-provide-response";
constexpr char regionalAutoProvideResponseContractId[] =
    "cpp-tck.regional-auto-provide-response-contract";
constexpr char regionalAutoProvideTimestampedResponseScenario[] =
    "cpp-tck.regional-auto-provide-timestamped-response";
constexpr char regionalAutoProvideTimestampedResponseContractId[] =
    "cpp-tck.regional-auto-provide-timestamped-response-contract";
constexpr char regionalAutoProvideOverlapScenario[] =
    "cpp-tck.regional-auto-provide-overlap";
constexpr char regionalAutoProvideOverlapContractId[] =
    "cpp-tck.regional-auto-provide-overlap-contract";
constexpr char regionalAutoProvideMultiSourceScenario[] =
    "cpp-tck.regional-auto-provide-multi-source";
constexpr char regionalAutoProvideMultiSourceContractId[] =
    "cpp-tck.regional-auto-provide-multi-source-contract";
constexpr char regionalAutoProvideMultiProviderScenario[] =
    "cpp-tck.regional-auto-provide-multi-provider";
constexpr char regionalAutoProvideMultiProviderContractId[] =
    "cpp-tck.regional-auto-provide-multi-provider-contract";
constexpr char regionalAutoProvideCallbackControlScenario[] =
    "cpp-tck.regional-auto-provide-callback-control";
constexpr char regionalAutoProvideCallbackControlContractId[] =
    "cpp-tck.regional-auto-provide-callback-control-contract";
constexpr char objectRegistrationServiceBoundariesScenario[] =
    "cpp-tck.object-registration-service-boundaries";
constexpr char objectRegistrationServiceBoundariesContractId[] =
    "cpp-tck.object-registration-service-boundaries-contract";
constexpr char objectDeletionServiceBoundariesScenario[] =
    "cpp-tck.object-deletion-service-boundaries";
constexpr char objectDeletionServiceBoundariesContractId[] =
    "cpp-tck.object-deletion-service-boundaries-contract";
constexpr char attributeUpdateServiceBoundariesScenario[] =
    "cpp-tck.attribute-update-service-boundaries";
constexpr char attributeUpdateServiceBoundariesContractId[] =
    "cpp-tck.attribute-update-service-boundaries-contract";
constexpr char interactionServiceBoundariesScenario[] =
    "cpp-tck.interaction-service-boundaries";
constexpr char interactionServiceBoundariesContractId[] =
    "cpp-tck.interaction-service-boundaries-contract";
constexpr char attributeValueRequestServiceBoundariesScenario[] =
    "cpp-tck.attribute-value-request-service-boundaries";
constexpr char attributeValueRequestServiceBoundariesContractId[] =
    "cpp-tck.attribute-value-request-service-boundaries-contract";
constexpr char connectionServiceBoundariesScenario[] =
    "cpp-tck.connection-service-boundaries";
constexpr char connectionServiceBoundariesContractId[] =
    "cpp-tck.connection-service-boundaries-contract";
constexpr char ownershipServiceBoundariesScenario[] =
    "cpp-tck.ownership-service-boundaries";
constexpr char ownershipServiceBoundariesContractId[] =
    "cpp-tck.ownership-service-boundaries-contract";

void scenarioFactoryDiscoveryPortable(Options const&, rti::CallbackModel) {
  rti::RTIambassadorFactory factory;
  auto ambassador = factory.createRTIambassador();
  require(
      static_cast<bool>(ambassador),
      "RTIambassadorFactory returned no standard RTIambassador");

  rti::HLAinteger32BE value{0x1234abcd};
  auto const encoded = value.encode();
  require(
      encoded.size() == 4U,
      "The standard C++ encoder did not produce the expected four-octet value");
  rti::HLAinteger32BE decoded;
  decoded.decode(encoded);
  require(
      decoded.get() == 0x1234abcd,
      "The standard C++ encoder did not round-trip through the discovered API");
}

void scenarioRTIambassadorFactoryPortableContract(
    Options const&,
    rti::CallbackModel) {
  rti::RTIambassadorFactory firstFactory;
  auto firstAmbassador = firstFactory.createRTIambassador();
  require(
      static_cast<bool>(firstAmbassador),
      "RTIambassadorFactory did not create a standard RTIambassador");

  rti::RTIambassadorFactory secondFactory;
  auto secondAmbassador = secondFactory.createRTIambassador();
  require(
      static_cast<bool>(secondAmbassador),
      "RTIambassadorFactory was not reusable for a second standard RTIambassador");
}

void scenarioVariableLengthDataPortable(
    Options const&,
    rti::CallbackModel) {
  verifyVariableLengthDataContract();
}

void scenarioLogicalTimeDataElementsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioLogicalTimeDataElementsContract(options, model);
}

void scenarioExceptionHierarchyPortable(Options const&, rti::CallbackModel) {
  verifyStandardExceptionHierarchyContract();
}

void scenarioEnumPortable(Options const&, rti::CallbackModel) {
  verifyStandardEnumContract();
}

void scenarioHandleAndCollectionPortable(Options const&, rti::CallbackModel) {
  verifyStandardHandleAndCollectionContract();
}

void scenarioDataElementPortable(Options const&, rti::CallbackModel) {
  verifyStandardDataElementContract();
}

void scenarioBasicDataElementsPortable(Options const&, rti::CallbackModel) {
  verifyBasicDataElementsContract();
}

void scenarioCompositeDataElementsPortable(Options const&, rti::CallbackModel) {
  verifyCompositeDataElementsContract();
}

void scenarioConfigurationAndAuthorizationPortable(
    Options const&,
    rti::CallbackModel) {
  verifyStandardConfigurationAndAuthorizationContract();
}

void scenarioRuntimeIdentityPortable(Options const&, rti::CallbackModel) {
  verifyStandardRuntimeIdentityContract();
}

void scenarioAuthorizerFactoryFactoryPortable(
    Options const&,
    rti::CallbackModel) {
  verifyStandardAuthorizerFactoryFactoryContract();
}

void scenarioNullFederateAmbassadorPortable(
    Options const&,
    rti::CallbackModel) {
  verifyStandardNullFederateAmbassadorContract();
}

void scenarioConnectionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnection(options, model);
}

void scenarioConnectionCallbackContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionCallbackContract(options, model);
}

void scenarioCallbackReentrancyPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackReentrancy(options, model);
}

void scenarioCallbackReentrancyContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCallbackReentrancyContract(options, model);
}

void scenarioMalformedInputsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMalformedInputs(options, model);
}

void scenarioMalformedInputsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMalformedInputsContract(options, model);
}

void scenarioFederationLifecyclePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationLifecycle(options, model);
}

void scenarioFederationLifecycleContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationLifecycleContract(options, model);
}

void scenarioFederationManagementRequiresConnectionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationManagementRequiresConnection(options, model);
}

void scenarioUnnamedJoinOverloadPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnnamedJoinOverload(options, model);
}

void scenarioUnnamedJoinOverloadContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnnamedJoinOverloadContract(options, model);
}

void scenarioFederationListServicesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationListServices(options, model);
}

void scenarioFederationListServicesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationListServicesContract(options, model);
}

void scenarioFederateLookupLifecyclePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLookupLifecycle(options, model);
}

void scenarioFederateLookupLifecycleContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLookupLifecycleContract(options, model);
}

void scenarioExplicitMimCreationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioExplicitMimCreation(options, model);
}

void scenarioExplicitMimCreationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioExplicitMimCreationContract(options, model);
}

void scenarioFederationMomCurrentFddPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomCurrentFdd(options, model);
}

void scenarioFederationMomCurrentFddContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomCurrentFddContract(options, model);
}

void scenarioFederationMomContentReportsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomContentReports(options, model);
}

void scenarioFederationMomContentReportsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomContentReportsContract(options, model);
}

void scenarioServiceReportOrderTransportationLookupsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookups(options, model);
}

void scenarioServiceReportOrderTransportationLookupsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookupsContract(options, model);
}

void scenarioServiceReportDimensionUpperBoundPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionUpperBound(options, model);
}

void scenarioServiceReportDimensionUpperBoundContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionUpperBoundContract(options, model);
}

void scenarioServiceReportDimensionHandleSetPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandleSet(options, model);
}

void scenarioServiceReportDimensionHandleSetContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandleSetContract(options, model);
}

void scenarioServiceReportRangeBoundsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportRangeBounds(options, model);
}

void scenarioServiceReportRangeBoundsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportRangeBoundsContract(options, model);
}

void scenarioServiceReportSetRangeBoundsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSetRangeBounds(options, model);
}

void scenarioServiceReportSetRangeBoundsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportSetRangeBoundsContract(options, model);
}

void scenarioServiceReportCommitRegionModificationsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCommitRegionModifications(options, model);
}

void scenarioServiceReportCommitRegionModificationsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCommitRegionModificationsContract(options, model);
}

void scenarioServiceReportDeleteRegionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDeleteRegion(options, model);
}

void scenarioServiceReportDeleteRegionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDeleteRegionContract(options, model);
}

void scenarioServiceReportCreateRegionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCreateRegion(options, model);
}

void scenarioServiceReportCreateRegionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportCreateRegionContract(options, model);
}

void scenarioRegionRangeValidationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeValidation(options, model);
}

void scenarioRegionRangeValidationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeValidationContract(options, model);
}

void scenarioRegionRangeDimensionValidationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeDimensionValidation(options, model);
}

void scenarioRegionRangeDimensionValidationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionRangeDimensionValidationContract(options, model);
}

void scenarioServiceReportDimensionNamePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionName(options, model);
}

void scenarioServiceReportDimensionNameContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionNameContract(options, model);
}

void scenarioServiceReportDimensionHandlePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandle(options, model);
}

void scenarioServiceReportDimensionHandleContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportDimensionHandleContract(options, model);
}

void scenarioServiceReportInteractionDimensionsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionDimensions(options, model);
}

void scenarioServiceReportInteractionDimensionsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionDimensionsContract(options, model);
}

void scenarioServiceReportObjectDimensionsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportObjectDimensions(options, model);
}

void scenarioServiceReportObjectDimensionsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportObjectDimensionsContract(options, model);
}

void scenarioServiceReportOrderTransportationLookupFailuresPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookupFailures(options, model);
}

void scenarioServiceReportOrderTransportationLookupFailuresContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportOrderTransportationLookupFailuresContract(options, model);
}

void scenarioServiceReportFederateObjectClassLookupsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateObjectClassLookups(options, model);
}

void scenarioServiceReportFederateObjectClassLookupsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateObjectClassLookupsContract(options, model);
}

void scenarioServiceReportFederateObjectClassLookupFailuresPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateObjectClassLookupFailures(options, model);
}

void scenarioServiceReportFederateObjectClassLookupFailuresContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportFederateObjectClassLookupFailuresContract(options, model);
}

void scenarioServiceReportInteractionParameterLookupsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionParameterLookups(options, model);
}

void scenarioServiceReportInteractionParameterLookupsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionParameterLookupsContract(options, model);
}

void scenarioServiceReportInteractionParameterLookupFailuresPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionParameterLookupFailures(options, model);
}

void scenarioServiceReportInteractionParameterLookupFailuresContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionParameterLookupFailuresContract(options, model);
}

void scenarioServiceReportInteractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteraction(options, model);
}

void scenarioServiceReportInteractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportInteractionContract(options, model);
}

void scenarioServiceReportResignFederationExecutionPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.mimFom.empty(),
      "Resignation service-report testing requires an adapter-supplied standard MIM");

  Session subject(options, model, "service-report-resignation-subject");
  Session observer(options, model, "service-report-resignation-observer");
  auto const federation =
      federationName(options, "service-report-resign-federation-execution");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-service-report-resignation-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-service-report-resignation-observer",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Resignation service-report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> reportParameters;
  for (auto const& name : {
           L"HLAservice",
           L"HLAserviceType",
           L"HLAsuccessIndicator",
           L"HLAsuppliedArguments",
           L"HLAreturnedArgument",
           L"HLAexception",
           L"HLAserialNumber",
           L"HLAfederate"}) {
    auto const parameter = observer.rtiAmbassador().getParameterHandle(reportClass, name);
    require(
        parameter.isValid(),
        "Resignation service-report parameter lookup returned an invalid handle");
    reportParameters.push_back(parameter);
  }

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().resignFederationExecution(rti::NO_ACTION);

  waitFor(
      observer,
      [&] { return observer.recorder().interaction().present; },
      options,
      "the ResignFederationExecution HLAreportServiceInvocation callback");
  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == reportParameters.size() &&
          !report.producer.isValid(),
      "Resignation service-report callback returned the wrong interaction identity or producer metadata");
  require(
      report.tag.empty() && !report.regions.has_value(),
      "Resignation service-report callback returned user tag or region metadata");
  require(
      report.transportation ==
          observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable"),
      "Resignation service-report callback returned the wrong transportation type");
  for (auto const parameterHandle : reportParameters) {
    require(
        report.parameters.count(parameterHandle) == 1U,
        "Resignation service-report callback omitted a standard report parameter");
  }

  auto const reportedFederate = observer.rtiAmbassador().decodeFederateHandle(
      report.parameters.at(reportParameters.at(7U)));
  require(
      reportedFederate == subjectFederate,
      "Resignation service-report callback identified the wrong joined federate");
  rti::HLAunicodeString service;
  service.decode(report.parameters.at(reportParameters[0]));
  require(
      service.get() == L"ResignFederationExecution",
      "Resignation service-report callback returned the wrong service name");
  rti::HLAinteger16BE serviceType;
  serviceType.decode(report.parameters.at(reportParameters[1]));
  require(
      serviceType.get() == 0,
      "Resignation service-report callback returned the wrong service type");
  rti::HLAboolean success;
  success.decode(report.parameters.at(reportParameters[2]));
  require(
      success.get(),
      "Resignation service-report callback did not report a successful invocation");
  rti::HLAunicodeString exception;
  exception.decode(report.parameters.at(reportParameters[5]));
  require(
      exception.get().empty(),
      "Successful resignation service report carried an exception");
  rti::HLAinteger32BE serial;
  serial.decode(report.parameters.at(reportParameters[6]));
  require(
      serial.get() >= 0,
      "Resignation service-report callback returned a negative serial number");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioServiceReportResignFederationExecutionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioServiceReportResignFederationExecutionPortable(options, model);
}

void scenarioFederateLostMomReportPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Federate-lost MOM reporting requires an adapter-managed fault fixture");
  require(
      !options.fom.empty(),
      "Federate-lost MOM reporting requires an adapter-supplied application FOM module");
  require(
      !options.mimFom.empty(),
      "Federate-lost MOM reporting requires an adapter-supplied standard MIM");
  auto withStage = [](char const* stage, auto&& operation) -> decltype(operation()) {
    try {
      return operation();
    } catch (rti::Exception const& error) {
      throw std::runtime_error(
          std::string("HLAreportFederateLost ") + stage + ": " +
          toNarrow(error.name()) + ": " + toNarrow(error.what()));
    } catch (std::exception const& error) {
      throw std::runtime_error(
          std::string("HLAreportFederateLost ") + stage + ": " + error.what());
    }
  };

  Session observer(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(options, "federate-lost-mom-report");
  auto const observerConnection = withStage(
      "observer connect failed", [&] { return observer.connect(); });
  auto const lostConnection = withStage(
      "lost-member connect failed", [&] { return lost.connect(); });
  require(
      observerConnection.addressUsed && lostConnection.addressUsed,
      "The adapter rtiAddress was not selected for both federates: " +
          options.rtiAddress);
  // The application module supplies the non-empty FOM module list required
  // for a valid federation create; the tested interaction remains defined by
  // the standard MIM and no application-class behavior is asserted.
  withStage("federation creation failed", [&] {
    observer.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  });

  auto const observerName = options.ownerFederateName;
  auto const lostName = options.memberFederateName;
  withStage("observer join failed", [&] {
    observer.join(observerName, options.federateType, federation);
  });
  withStage("lost-member join failed", [&] {
    lost.join(lostName, options.federateType, federation);
  });
  auto const lostFederate = lost.federateHandle();

  auto const reportClass = withStage("interaction-class lookup failed", [&] {
    return observer.rtiAmbassador().getInteractionClassHandle(
        L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFederateLost");
  });
  require(reportClass.isValid(), "HLAreportFederateLost lookup returned an invalid handle");
  auto const federateParameter = withStage("HLAfederate parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfederate");
  });
  auto const nameParameter = withStage("HLAfederateName parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfederateName");
  });
  auto const timeParameter = withStage("HLAtimeStamp parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAtimeStamp");
  });
  auto const faultParameter = withStage("HLAfaultDescription parameter lookup failed", [&] {
    return observer.rtiAmbassador().getParameterHandle(reportClass, L"HLAfaultDescription");
  });
  require(
      federateParameter.isValid() && nameParameter.isValid() &&
          timeParameter.isValid() && faultParameter.isValid(),
      "HLAreportFederateLost parameter lookup returned an invalid handle");
  withStage("subscription failed", [&] {
    observer.rtiAmbassador().subscribeInteractionClass(reportClass);
  });

  if (!options.connectionLossReadyMarker.empty()) {
    std::ofstream readyMarker(
        options.connectionLossReadyMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(readyMarker),
        "Could not write the connection-loss readiness marker");
    readyMarker << "ready\n";
    require(
        static_cast<bool>(readyMarker),
        "Could not flush the connection-loss readiness marker");
  }
  auto const lossDeadline = std::chrono::steady_clock::now() +
      std::chrono::milliseconds(options.timeoutMilliseconds);
  while (std::chrono::steady_clock::now() < lossDeadline &&
         (!lost.recorder().hasConnectionLost() ||
          !observer.recorder().interaction().present)) {
    for (Session* federate : {&lost, &observer}) {
      try {
        static_cast<void>(
            federate->rtiAmbassador().evokeMultipleCallbacks(0.0, 0.05));
      } catch (...) {
        // A provider may surface the socket fault on an evoke operation before
        // queuing ConnectionLost; continue servicing both federates.
      }
    }
    if (!lost.recorder().hasConnectionLost() ||
        !observer.recorder().interaction().present) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  require(
      lost.recorder().hasConnectionLost(),
      "Timed out waiting for HLAreportFederateLost delivery connectionLost callback");
  require(
      !lost.recorder().connectionLostDescription().empty(),
      "HLAreportFederateLost delivery connectionLost did not provide a fault description");
  require(
      observer.recorder().interaction().present,
      "Timed out waiting for the subscribed HLAreportFederateLost callback");
  if (!options.connectionLossMarker.empty()) {
    std::ofstream marker(
        options.connectionLossMarker,
        std::ios::binary | std::ios::trunc);
    require(
        static_cast<bool>(marker),
        "Could not write the connection-loss completion marker");
    marker << "callback-ok\n";
    require(
        static_cast<bool>(marker),
        "Could not flush the connection-loss completion marker");
  }

  auto const report = observer.recorder().interaction();
  require(
      report.interaction == reportClass &&
          report.parameters.size() == 4U &&
          report.parameters.count(federateParameter) == 1U &&
          report.parameters.count(nameParameter) == 1U &&
          report.parameters.count(timeParameter) == 1U &&
          report.parameters.count(faultParameter) == 1U,
      "HLAreportFederateLost callback returned the wrong identity or parameter set");
  auto const reliable = withStage("HLAreliable lookup failed", [&] {
    return observer.rtiAmbassador().getTransportationTypeHandle(L"HLAreliable");
  });
  require(
      report.transportation == reliable,
      "HLAreportFederateLost callback did not use reliable transportation");

  auto const reportedFederate = withStage("federate-handle decoding failed", [&] {
    return observer.rtiAmbassador().decodeFederateHandle(
        report.parameters.at(federateParameter));
  });
  require(
      reportedFederate == lostFederate,
      "HLAreportFederateLost identified the wrong joined federate");
  rti::HLAunicodeString reportedName;
  withStage("federate-name decoding failed", [&] {
    reportedName.decode(report.parameters.at(nameParameter));
  });
  require(
      reportedName.get() == lostName,
      "HLAreportFederateLost returned the wrong federate name");
  auto timeFactory = withStage("logical-time factory creation failed", [&] {
    return rti::HLAlogicalTimeFactoryFactory::makeLogicalTimeFactory(
        options.logicalTimeImplementationName);
  });
  require(timeFactory != nullptr, "The adapter logical-time factory was unavailable");
  auto reportedTime = withStage("logical timestamp decoding failed", [&] {
    return timeFactory->decodeLogicalTime(report.parameters.at(timeParameter));
  });
  require(
      reportedTime != nullptr && !reportedTime->toString().empty(),
      "HLAreportFederateLost did not encode a standard logical timestamp");
  rti::HLAunicodeString faultDescription;
  withStage("fault-description decoding failed", [&] {
    faultDescription.decode(report.parameters.at(faultParameter));
  });

  try {
    lost.disconnect();
  } catch (...) {
    // The connection-loss callback may follow an implementation's forced disconnect.
  }
  withStage("observer resignation failed", [&] {
    observer.resign(rti::NO_ACTION);
  });
  withStage("federation destruction failed", [&] {
    observer.rtiAmbassador().destroyFederationExecution(federation);
  });
  withStage("observer disconnect failed", [&] { observer.disconnect(); });
}

void scenarioFederateLostMomReportContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLostMomReportPortable(options, model);
}

void scenarioHandleNormalizationMembershipBoundariesPortable(
    Options const& options,
    rti::CallbackModel model) {
  Session owner(options, model, "handle-normalization-boundary-owner");
  Session member(options, model, "handle-normalization-boundary-member");
  Session probe(options, model, "handle-normalization-boundary-probe");
  auto const federation =
      federationName(options, "handle-normalization-membership-boundaries");
  connectAndJoin(owner, member, options, federation, options.fom);

  rti::ObjectClassHandle objectClass;
  rti::AttributeHandle attribute;
  rti::InteractionClassHandle interactionClass;
  rti::ParameterHandle parameter;
  handles(owner, options, objectClass, attribute, interactionClass, parameter);
  rti::AttributeHandleSet publishedAttributes;
  publishedAttributes.insert(attribute);
  owner.rtiAmbassador().publishObjectClassAttributes(objectClass, publishedAttributes);
  auto const object = owner.rtiAmbassador().registerObjectInstance(objectClass);
  require(
      object.isValid(),
      "Handle-normalization boundary fixture returned an invalid object handle");

  auto requireNormalizationState = [&](Session& caller,
                                       std::wstring const& expected,
                                       std::string const& phase) {
    requireException(
        [&] {
          static_cast<void>(caller.rtiAmbassador().normalizeServiceGroup(
              rti::SUPPORT_SERVICES));
        },
        expected,
        "normalize support service group " + phase);
    requireException(
        [&] {
          static_cast<void>(caller.rtiAmbassador().normalizeFederateHandle(
              owner.federateHandle()));
        },
        expected,
        "normalize federate handle " + phase);
    requireException(
        [&] {
          static_cast<void>(caller.rtiAmbassador().normalizeObjectClassHandle(objectClass));
        },
        expected,
        "normalize object-class handle " + phase);
    requireException(
        [&] {
          static_cast<void>(caller.rtiAmbassador().normalizeInteractionClassHandle(
              interactionClass));
        },
        expected,
        "normalize interaction-class handle " + phase);
    requireException(
        [&] {
          static_cast<void>(caller.rtiAmbassador().normalizeObjectInstanceHandle(object));
        },
        expected,
        "normalize object-instance handle " + phase);
  };

  requireNormalizationState(probe, L"NotConnected", "before connect");
  probe.connect();
  requireNormalizationState(
      probe,
      L"FederateNotExecutionMember",
      "after connect before join");
  probe.join(
      options.memberFederateName + L"-handle-normalization-probe",
      options.federateType,
      federation);
  probe.resign(rti::NO_ACTION);
  requireNormalizationState(
      probe,
      L"FederateNotExecutionMember",
      "after resignation while connected");
  probe.disconnect();

  member.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioHandleNormalizationMembershipBoundariesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioHandleNormalizationMembershipBoundariesPortable(options, model);
}

void scenarioDimensionLookupValidHandlesAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.ddmFom.empty(),
      "Post-resignation dimension lookup requires an adapter-supplied dimensional FOM");
  require(
      !options.ddmDimensionNames.empty(),
      "Post-resignation dimension lookup requires an adapter-supplied dimension name");

  Session owner(options, model, "dimension-lookup-resigned-owner");
  Session member(options, model, "dimension-lookup-resigned-member");
  auto const federation =
      federationName(options, "dimension-lookup-valid-handles-after-resignation");
  connectAndJoin(owner, member, options, federation, options.ddmFom);

  auto const objectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const interactionClass = owner.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const dimension = owner.rtiAmbassador().getDimensionHandle(
      options.ddmDimensionNames.front());
  require(
      objectClass.isValid() && interactionClass.isValid() && dimension.isValid(),
      "Post-resignation dimension lookup fixture returned an invalid standard handle");
  require(
      owner.rtiAmbassador().getDimensionName(dimension) ==
              options.ddmDimensionNames.front() &&
          owner.rtiAmbassador().getDimensionUpperBound(dimension) > 0U,
      "Post-resignation dimension lookup fixture did not verify its live dimension handle");

  owner.resign(rti::NO_ACTION);
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionName(dimension));
      },
      L"FederateNotExecutionMember",
      "looking up a valid dimension name after resignation");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getDimensionUpperBound(dimension));
      },
      L"FederateNotExecutionMember",
      "looking up a valid dimension upper bound after resignation");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForObjectClass(
            objectClass));
      },
      L"FederateNotExecutionMember",
      "looking up valid object-class dimensions after resignation");
  requireException(
      [&] {
        static_cast<void>(owner.rtiAmbassador().getAvailableDimensionsForInteractionClass(
            interactionClass));
      },
      L"FederateNotExecutionMember",
      "looking up valid interaction-class dimensions after resignation");

  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioDimensionLookupValidHandlesAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDimensionLookupValidHandlesAfterResignationPortable(options, model);
}

void scenarioUpdateRateLookupValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.rateFom.empty(),
      "Post-resignation update-rate lookup requires an adapter-supplied rate FOM");
  require(
      !options.rateDesignatorName.empty(),
      "Post-resignation update-rate lookup requires an adapter-supplied designator");

  Session owner(options, model, "update-rate-resigned-owner");
  Session member(options, model, "update-rate-resigned-member");
  auto const federation =
      federationName(options, "update-rate-lookup-valid-inputs-after-resignation");
  connectAndJoin(owner, member, options, federation, options.rateFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const memberClass = member.rtiAmbassador().getObjectClassHandle(
      options.rateObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.rateBestEffortAttributeName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.rateBestEffortAttributeName);
  require(
      ownerClass.isValid() && memberClass.isValid() && ownerAttribute.isValid() &&
          memberAttribute.isValid(),
      "Post-resignation update-rate fixture returned an invalid standard handle");

  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      rti::AttributeHandleSet{memberAttribute},
      true,
      options.rateDesignatorName);
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Post-resignation update-rate fixture returned an invalid object handle");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "post-resignation update-rate object discovery");

  auto const namedRate = member.rtiAmbassador().getUpdateRateValue(
      options.rateDesignatorName);
  require(namedRate > 0.0, "Adapter-supplied update-rate designator was not valid");
  require(
      member.rtiAmbassador().getUpdateRateValueForAttribute(object, memberAttribute) ==
          namedRate,
      "Valid object/attribute did not resolve the active update-rate designator");

  member.resign(rti::NO_ACTION);
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().getUpdateRateValue(
            options.rateDesignatorName));
      },
      L"FederateNotExecutionMember",
      "looking up a valid update-rate designator after resignation");
  requireException(
      [&] {
        static_cast<void>(member.rtiAmbassador().getUpdateRateValueForAttribute(
            object,
            memberAttribute));
      },
      L"FederateNotExecutionMember",
      "looking up a valid object-attribute update rate after resignation");

  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  member.disconnect();
  owner.disconnect();
}

void scenarioUpdateRateLookupValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUpdateRateLookupValidInputsAfterResignationPortable(options, model);
}

void scenarioSupportLookupValidInputsAfterResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Post-resignation support lookup testing requires an adapter-supplied FOM");
  require(
      !options.objectClassName.empty() && !options.attributeName.empty() &&
          !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Post-resignation support lookup testing requires adapter-supplied class and member names");

  Session subject(options, model, "support-lookup-resigned-subject");
  auto const federation =
      federationName(options, "support-lookup-valid-inputs-after-resignation");
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-support-lookup-resigned-subject",
      options.federateType,
      federation);

  auto const objectClass = subject.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const attribute = subject.rtiAmbassador().getAttributeHandle(
      objectClass,
      options.attributeName);
  auto const interactionClass = subject.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const parameter = subject.rtiAmbassador().getParameterHandle(
      interactionClass,
      options.parameterName);
  require(
      objectClass.isValid() && attribute.isValid() && interactionClass.isValid() &&
          parameter.isValid(),
      "Adapter-supplied FOM returned an invalid support-service lookup handle");
  require(
      subject.rtiAmbassador().getObjectClassName(objectClass) ==
              options.objectClassName &&
          subject.rtiAmbassador().getAttributeName(objectClass, attribute) ==
              options.attributeName &&
          subject.rtiAmbassador().getInteractionClassName(interactionClass) ==
              options.interactionClassName &&
          subject.rtiAmbassador().getParameterName(interactionClass, parameter) ==
              options.parameterName,
      "Adapter-supplied FOM support-service lookups did not round-trip while joined");

  subject.resign(rti::NO_ACTION);
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectClassHandle(
            options.objectClassName));
      },
      L"FederateNotExecutionMember",
      "looking up a valid object class after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getObjectClassName(objectClass));
      },
      L"FederateNotExecutionMember",
      "looking up a valid object-class name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAttributeHandle(
            objectClass,
            options.attributeName));
      },
      L"FederateNotExecutionMember",
      "looking up a valid attribute after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getAttributeName(
            objectClass,
            attribute));
      },
      L"FederateNotExecutionMember",
      "looking up a valid attribute name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getInteractionClassHandle(
            options.interactionClassName));
      },
      L"FederateNotExecutionMember",
      "looking up a valid interaction class after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getInteractionClassName(
            interactionClass));
      },
      L"FederateNotExecutionMember",
      "looking up a valid interaction-class name after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterHandle(
            interactionClass,
            options.parameterName));
      },
      L"FederateNotExecutionMember",
      "looking up a valid parameter after resignation");
  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().getParameterName(
            interactionClass,
            parameter));
      },
      L"FederateNotExecutionMember",
      "looking up a valid parameter name after resignation");

  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioSupportLookupValidInputsAfterResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSupportLookupValidInputsAfterResignationPortable(options, model);
}

void scenarioDuplicateFederateNameJoinBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Duplicate federate-name join testing requires an adapter-supplied FOM");

  Session owner(options, model, "duplicate-federate-name-owner");
  Session joiner(options, model, "duplicate-federate-name-joiner");
  auto const federation =
      federationName(options, "duplicate-federate-name-join-boundary");
  auto const ownerName = options.ownerFederateName + L"-dup-owner";
  auto const joinerName = options.memberFederateName + L"-dup-joiner";
  require(ownerName != joinerName, "Adapter federate names must be distinct");

  owner.connect();
  joiner.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(ownerName, options.federateType, federation);

  requireException(
      [&] {
        static_cast<void>(joiner.rtiAmbassador().joinFederationExecution(
            ownerName,
            options.federateType,
            federation));
      },
      L"FederateNameAlreadyInUse",
      "joining with a name already used by an execution member");

  joiner.join(joinerName, options.federateType, federation);
  require(
      owner.federateHandle() != joiner.federateHandle(),
      "Distinct federate names did not produce distinct federate handles");

  owner.resign(rti::NO_ACTION);
  joiner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  joiner.disconnect();
  owner.disconnect();
}

void scenarioDuplicateFederateNameJoinBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDuplicateFederateNameJoinBoundaryPortable(options, model);
}

void scenarioRepeatedJoinByMemberBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Repeated-join testing requires an adapter-supplied FOM");

  Session subject(options, model, "repeated-join-by-member-subject");
  auto const federation =
      federationName(options, "repeated-join-by-member-boundary");
  auto const federateName = options.ownerFederateName + L"-repeated-join-member";
  subject.connect();
  subject.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(federateName, options.federateType, federation);

  requireException(
      [&] {
        static_cast<void>(subject.rtiAmbassador().joinFederationExecution(
            federateName,
            options.federateType,
            federation));
      },
      L"FederateAlreadyExecutionMember",
      "joining an execution a second time through an existing member ambassador");

  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
}

void scenarioRepeatedJoinByMemberBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRepeatedJoinByMemberBoundaryPortable(options, model);
}

void scenarioForeignFederateNameLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federate-name execution-scope testing requires an adapter-supplied FOM");

  Session local(options, model, "foreign-federate-name-local");
  Session foreign(options, model, "foreign-federate-name-foreign");
  auto const localFederation =
      federationName(options, "federate-name-lookup-local-execution");
  auto const foreignFederation =
      federationName(options, "federate-name-lookup-foreign-execution");
  auto const localName = options.ownerFederateName + L"-lookup-local";
  auto const foreignName = options.memberFederateName + L"-lookup-foreign";
  require(localName != foreignName, "Adapter federate names must be distinct");

  local.connect();
  foreign.connect();
  local.rtiAmbassador().createFederationExecution(
      localFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  foreign.rtiAmbassador().createFederationExecution(
      foreignFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  local.join(localName, options.federateType, localFederation);
  foreign.join(foreignName, options.federateType, foreignFederation);

  require(
      local.rtiAmbassador().getFederateHandle(localName) ==
          local.federateHandle(),
      "Federate-handle lookup did not resolve the local execution member");
  requireException(
      [&] {
        static_cast<void>(
            local.rtiAmbassador().getFederateHandle(foreignName));
      },
      L"NameNotFound",
      "looking up a name belonging to a different federation execution");
  require(
      foreign.rtiAmbassador().getFederateHandle(foreignName) ==
          foreign.federateHandle(),
      "Federate-handle lookup did not resolve the foreign execution's own member");

  local.resign(rti::NO_ACTION);
  foreign.resign(rti::NO_ACTION);
  local.rtiAmbassador().destroyFederationExecution(localFederation);
  foreign.rtiAmbassador().destroyFederationExecution(foreignFederation);
  local.disconnect();
  foreign.disconnect();
}

void scenarioForeignFederateNameLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioForeignFederateNameLookupBoundaryPortable(options, model);
}

void scenarioForeignObjectInstanceNameLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Object-name execution-scope testing requires an adapter-supplied FOM");

  Session local(options, model, "foreign-object-name-local");
  Session foreign(options, model, "foreign-object-name-foreign");
  auto const localFederation =
      federationName(options, "object-name-lookup-local-execution");
  auto const foreignFederation = options.federationOverride.empty()
      ? federationName(options, "object-name-lookup-foreign-execution")
      : localFederation + L"-foreign";
  auto const localFederateName =
      options.ownerFederateName + L"-obj-local";
  auto const foreignFederateName =
      options.memberFederateName + L"-obj-foreign";
  auto const localObjectName =
      options.ownerFederateName + L"-object-name-local";
  auto const foreignObjectName =
      options.memberFederateName + L"-object-name-foreign";
  require(
      localFederateName != foreignFederateName &&
          localObjectName != foreignObjectName,
      "Adapter-supplied federate and object names must be distinct");

  local.connect();
  foreign.connect();
  local.rtiAmbassador().createFederationExecution(
      localFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  foreign.rtiAmbassador().createFederationExecution(
      foreignFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  local.join(localFederateName, options.federateType, localFederation);
  foreign.join(foreignFederateName, options.federateType, foreignFederation);

  auto const localClass =
      local.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const localAttribute = local.rtiAmbassador().getAttributeHandle(
      localClass,
      options.attributeName);
  rti::AttributeHandleSet const localAttributes{localAttribute};
  local.rtiAmbassador().publishObjectClassAttributes(localClass, localAttributes);
  local.recorder().clearReservations();
  local.rtiAmbassador().reserveObjectInstanceName(localObjectName);
  waitFor(
      local,
      [&] { return local.recorder().reservationSucceeded(localObjectName); },
      options,
      "local object-instance name reservation");
  auto const localObject = local.rtiAmbassador().registerObjectInstance(
      localClass,
      localObjectName);

  auto const foreignClass =
      foreign.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const foreignAttribute = foreign.rtiAmbassador().getAttributeHandle(
      foreignClass,
      options.attributeName);
  rti::AttributeHandleSet const foreignAttributes{foreignAttribute};
  foreign.rtiAmbassador().publishObjectClassAttributes(
      foreignClass,
      foreignAttributes);
  foreign.recorder().clearReservations();
  foreign.rtiAmbassador().reserveObjectInstanceName(foreignObjectName);
  waitFor(
      foreign,
      [&] {
        return foreign.recorder().reservationSucceeded(foreignObjectName);
      },
      options,
      "foreign object-instance name reservation");
  auto const foreignObject = foreign.rtiAmbassador().registerObjectInstance(
      foreignClass,
      foreignObjectName);

  require(
      local.rtiAmbassador().getObjectInstanceHandle(localObjectName) ==
          localObject,
      "Object-name lookup did not resolve the local execution's object");
  require(
      foreign.rtiAmbassador().getObjectInstanceHandle(foreignObjectName) ==
          foreignObject,
      "Object-name lookup did not resolve the foreign execution's object");
  requireException(
      [&] {
        static_cast<void>(
            local.rtiAmbassador().getObjectInstanceHandle(foreignObjectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up an object name belonging to a different execution");

  local.rtiAmbassador().deleteObjectInstance(
      localObject,
      rti::VariableLengthData{});
  foreign.rtiAmbassador().deleteObjectInstance(
      foreignObject,
      rti::VariableLengthData{});
  local.rtiAmbassador().unpublishObjectClassAttributes(
      localClass,
      localAttributes);
  foreign.rtiAmbassador().unpublishObjectClassAttributes(
      foreignClass,
      foreignAttributes);
  local.resign(rti::NO_ACTION);
  foreign.resign(rti::NO_ACTION);
  local.rtiAmbassador().destroyFederationExecution(localFederation);
  foreign.rtiAmbassador().destroyFederationExecution(foreignFederation);
  local.disconnect();
  foreign.disconnect();
}

void scenarioForeignObjectInstanceNameLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioForeignObjectInstanceNameLookupBoundaryPortable(options, model);
}

void scenarioForeignObjectInstanceHandleLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Object-handle execution-scope testing requires an adapter-supplied FOM");

  Session local(options, model, "foreign-object-handle-local");
  Session foreign(options, model, "foreign-object-handle-foreign");
  auto const localFederation =
      federationName(options, "object-handle-lookup-local-execution");
  auto const foreignFederation = options.federationOverride.empty()
      ? federationName(options, "object-handle-lookup-foreign-execution")
      : localFederation + L"-foreign";
  auto const localFederateName =
      options.ownerFederateName + L"-hnd-local";
  auto const foreignFederateName =
      options.memberFederateName + L"-hnd-foreign";
  auto const localObjectName =
      options.ownerFederateName + L"-object-handle-local";
  auto const foreignFirstObjectName =
      options.memberFederateName + L"-object-handle-foreign-first";
  auto const foreignSecondObjectName =
      options.memberFederateName + L"-object-handle-foreign-second";

  local.connect();
  foreign.connect();
  local.rtiAmbassador().createFederationExecution(
      localFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  foreign.rtiAmbassador().createFederationExecution(
      foreignFederation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  local.join(localFederateName, options.federateType, localFederation);
  foreign.join(foreignFederateName, options.federateType, foreignFederation);

  auto const localClass =
      local.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const localAttribute = local.rtiAmbassador().getAttributeHandle(
      localClass,
      options.attributeName);
  rti::AttributeHandleSet const localAttributes{localAttribute};
  local.rtiAmbassador().publishObjectClassAttributes(localClass, localAttributes);
  auto const foreignClass =
      foreign.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const foreignAttribute = foreign.rtiAmbassador().getAttributeHandle(
      foreignClass,
      options.attributeName);
  rti::AttributeHandleSet const foreignAttributes{foreignAttribute};
  foreign.rtiAmbassador().publishObjectClassAttributes(
      foreignClass,
      foreignAttributes);

  auto registerNamedObject = [&](Session& session,
                                 rti::ObjectClassHandle const& objectClass,
                                 std::wstring const& objectName,
                                 std::string const& phase) {
    session.recorder().clearReservations();
    session.rtiAmbassador().reserveObjectInstanceName(objectName);
    waitFor(
        session,
        [&] { return session.recorder().reservationSucceeded(objectName); },
        options,
        phase + " name reservation");
    return session.rtiAmbassador().registerObjectInstance(
        objectClass,
        objectName);
  };

  auto const localObject = registerNamedObject(
      local,
      localClass,
      localObjectName,
      "local object");
  auto const foreignFirstObject = registerNamedObject(
      foreign,
      foreignClass,
      foreignFirstObjectName,
      "first foreign object");
  auto const foreignSecondObject = registerNamedObject(
      foreign,
      foreignClass,
      foreignSecondObjectName,
      "second foreign object");
  require(
      foreignFirstObject != foreignSecondObject,
      "Distinct foreign objects did not receive distinct object handles");

  auto const foreignObject = foreignFirstObject != localObject
      ? foreignFirstObject
      : foreignSecondObject;
  auto const foreignObjectName = foreignFirstObject != localObject
      ? foreignFirstObjectName
      : foreignSecondObjectName;
  require(
      foreignObject != localObject,
      "Could not select a foreign object handle distinct from the local handle");
  require(
      local.rtiAmbassador().getObjectInstanceName(localObject) == localObjectName,
      "Local object-handle lookup did not resolve its object name");
  require(
      foreign.rtiAmbassador().getObjectInstanceName(foreignObject) ==
          foreignObjectName,
      "Foreign object-handle lookup did not resolve its own execution's name");
  requireException(
      [&] {
        static_cast<void>(
            local.rtiAmbassador().getObjectInstanceName(foreignObject));
      },
      L"ObjectInstanceNotKnown",
      "looking up an object handle not present in the local execution");

  local.rtiAmbassador().deleteObjectInstance(
      localObject,
      rti::VariableLengthData{});
  foreign.rtiAmbassador().deleteObjectInstance(
      foreignFirstObject,
      rti::VariableLengthData{});
  foreign.rtiAmbassador().deleteObjectInstance(
      foreignSecondObject,
      rti::VariableLengthData{});
  local.rtiAmbassador().unpublishObjectClassAttributes(
      localClass,
      localAttributes);
  foreign.rtiAmbassador().unpublishObjectClassAttributes(
      foreignClass,
      foreignAttributes);
  local.resign(rti::NO_ACTION);
  foreign.resign(rti::NO_ACTION);
  local.rtiAmbassador().destroyFederationExecution(localFederation);
  foreign.rtiAmbassador().destroyFederationExecution(foreignFederation);
  local.disconnect();
  foreign.disconnect();
}

void scenarioForeignObjectInstanceHandleLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioForeignObjectInstanceHandleLookupBoundaryPortable(options, model);
}

void scenarioLocalDeleteObjectInstanceNameLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Local-delete object-name lookup testing requires an adapter-supplied FOM");

  Session owner(options, model, "local-delete-name-owner");
  Session member(options, model, "local-delete-name-member");
  auto const federation =
      federationName(options, "local-delete-object-name-lookup-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);

  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes);

  member.recorder().clearDiscovery();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Local-delete name-lookup setup returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "Local-delete name-lookup setup returned an empty object name");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "local-delete name-lookup object discovery");
  auto const discovery = member.recorder().discovery();
  require(
      discovery.object == object &&
          member.rtiAmbassador().getObjectInstanceName(discovery.object) == objectName,
      "Discovered object identity did not resolve before local deletion");

  member.rtiAmbassador().localDeleteObjectInstance(discovery.object);
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getObjectInstanceName(discovery.object));
      },
      L"ObjectInstanceNotKnown",
      "looking up the name of a locally forgotten object instance");
  require(
      owner.rtiAmbassador().getObjectInstanceName(object) == objectName,
      "Local deletion changed the publishing federate's object identity");

  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      memberClass,
      memberAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioLocalDeleteObjectInstanceNameLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioLocalDeleteObjectInstanceNameLookupBoundaryPortable(options, model);
}

void scenarioRemoteDeleteKnownObjectClassLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Remote-delete known-class lookup testing requires an adapter-supplied FOM");

  Session owner(options, model, "remote-delete-known-class-owner");
  Session member(options, model, "remote-delete-known-class-member");
  auto const federation =
      federationName(options, "remote-delete-known-class-lookup-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);

  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes);

  member.recorder().clearDiscovery();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Remote-delete known-class setup returned an invalid object handle");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "remote-delete known-class object discovery");
  auto const discovery = member.recorder().discovery();
  require(
      discovery.object == object &&
          member.rtiAmbassador().getKnownObjectClassHandle(discovery.object) == memberClass,
      "Discovered object did not resolve its known class before remote deletion");

  member.recorder().clearRemovals();
  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  requireException(
      [&] {
        static_cast<void>(
            owner.rtiAmbassador().getKnownObjectClassHandle(object));
      },
      L"ObjectInstanceNotKnown",
      "querying the deleting federate's known object class after deletion");
  waitFor(
      member,
      [&] {
        return member.recorder().hasRemoval(
            object,
            {},
            owner.federateHandle());
      },
      options,
      "remote-delete known-class removal callback");
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getKnownObjectClassHandle(discovery.object));
      },
      L"ObjectInstanceNotKnown",
      "querying the known object class after remote removal");

  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      memberClass,
      memberAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioRemoteDeleteKnownObjectClassLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRemoteDeleteKnownObjectClassLookupBoundaryPortable(options, model);
}

void scenarioRemoteDeleteObjectInstanceNameLookupBoundaryPortable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Remote-delete object-name lookup testing requires an adapter-supplied FOM");

  Session owner(options, model, "remote-delete-name-owner");
  Session member(options, model, "remote-delete-name-member");
  auto const federation =
      federationName(options, "remote-delete-object-name-lookup-boundary");
  connectAndJoin(owner, member, options, federation, options.fom);

  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);

  auto const memberClass =
      member.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const memberAttribute = member.rtiAmbassador().getAttributeHandle(
      memberClass,
      options.attributeName);
  rti::AttributeHandleSet const memberAttributes{memberAttribute};
  member.rtiAmbassador().subscribeObjectClassAttributes(
      memberClass,
      memberAttributes);

  member.recorder().clearDiscovery();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(object.isValid(), "Remote-delete name-lookup setup returned an invalid object handle");
  auto const objectName = owner.rtiAmbassador().getObjectInstanceName(object);
  require(!objectName.empty(), "Remote-delete name-lookup setup returned an empty object name");
  waitFor(
      member,
      [&] { return member.recorder().hasDiscovery(object); },
      options,
      "remote-delete object-name lookup discovery");
  auto const discovery = member.recorder().discovery();
  require(
      discovery.object == object &&
          member.rtiAmbassador().getObjectInstanceName(discovery.object) == objectName &&
          owner.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "Object name and handle did not resolve on both federates before deletion");

  member.recorder().clearRemovals();
  owner.rtiAmbassador().deleteObjectInstance(object, rti::VariableLengthData{});
  requireException(
      [&] {
        static_cast<void>(
            owner.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up the deleted object by name on the deleting federate");
  waitFor(
      member,
      [&] {
        return member.recorder().hasRemoval(
            object,
            {},
            owner.federateHandle());
      },
      options,
      "remote-delete object-name lookup removal callback");
  requireException(
      [&] {
        static_cast<void>(
            member.rtiAmbassador().getObjectInstanceName(discovery.object));
      },
      L"ObjectInstanceNotKnown",
      "looking up the name of an object after remote removal");

  member.rtiAmbassador().unsubscribeObjectClassAttributes(
      memberClass,
      memberAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  owner.resign(rti::NO_ACTION);
  member.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  owner.disconnect();
  member.disconnect();
}

void scenarioRemoteDeleteObjectInstanceNameLookupBoundaryContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRemoteDeleteObjectInstanceNameLookupBoundaryPortable(options, model);
}

void scenarioTimestampedInteractionsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractions(options, model);
}

void scenarioUnregulatedSenderTimestampedInteractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedInteraction(options, model);
}

void scenarioUnregulatedSenderTimestampedInteractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnregulatedSenderTimestampedInteractionContract(options, model);
}

void scenarioTimestampedInteractionsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionsContract(options, model);
}

void scenarioTimestampedInteractionPreferredReceiveConversionPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionPreferredReceiveConversion(options, model);
}

void scenarioTimestampedInteractionPreferredReceiveConversionContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionPreferredReceiveConversionContract(options, model);
}

void scenarioTimestampedInteractionSenderRegulationTransitionPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionSenderRegulationTransition(options, model);
}

void scenarioTimestampedInteractionSenderRegulationTransitionContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioTimestampedInteractionSenderRegulationTransitionContract(options, model);
}

void scenarioInteractionFddPreferredOrderDefaultPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioInteractionFddPreferredOrderDefault(options, model);
}

void scenarioInteractionFddPreferredOrderDefaultContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioInteractionFddPreferredOrderDefaultContract(options, model);
}

void scenarioAttributeFddPreferredOrderDefaultPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioAttributeFddPreferredOrderDefault(options, model);
}

void scenarioAttributeFddPreferredOrderDefaultContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioAttributeFddPreferredOrderDefaultContract(options, model);
}

void scenarioTimestampedAttributeUpdatePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdate(options, model);
}

void scenarioTimestampedAttributeUpdateContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateContract(options, model);
}

void scenarioTimestampedAttributeUpdateTarPostGrantCutoffPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTarPostGrantCutoff(options, model);
}

void scenarioTimestampedAttributeUpdateTarPostGrantCutoffContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTarPostGrantCutoffContract(options, model);
}

void scenarioTimestampedAttributeUpdateTimeAdvancingCutoffPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTimeAdvancingCutoff(options, model);
}

void scenarioTimestampedAttributeUpdateTimeAdvancingCutoffContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateTimeAdvancingCutoffContract(options, model);
}

void scenarioTimestampedAttributeUpdateReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateReenable(options, model);
}

void scenarioTimestampedAttributeUpdateReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateReenableContract(options, model);
}

void scenarioTimestampedAttributeUpdateRegulationReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateRegulationReenable(options, model);
}

void scenarioTimestampedAttributeUpdateRegulationReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateRegulationReenableContract(options, model);
}

void scenarioTimestampedAttributeSourceResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSourceResignation(options, model);
}

void scenarioTimestampedAttributeSourceResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSourceResignationContract(options, model);
}

void scenarioTimestampedAttributeSourceResignationFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSourceResignationFanout(options, model);
}

void scenarioTimestampedAttributeSourceResignationFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSourceResignationFanoutContract(options, model);
}

void scenarioTimestampedAttributeUpdateOwnershipTransferPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateOwnershipTransfer(options, model);
}

void scenarioTimestampedAttributeUpdateOwnershipTransferContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateOwnershipTransferContract(options, model);
}

void scenarioTimestampedAttributeUpdateQueuedPasselRetractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateQueuedPasselRetraction(options, model);
}

void scenarioTimestampedAttributeUpdateQueuedPasselRetractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateQueuedPasselRetractionContract(options, model);
}

void scenarioMixedOrderAttributePasselRetractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMixedOrderAttributePasselRetraction(options, model);
}

void scenarioMixedOrderAttributePasselRetractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMixedOrderAttributePasselRetractionContract(options, model);
}

void scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeImmediateRecipientMultiTransportRetraction(
      options,
      model);
}

void scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionContract(
      options,
      model);
}

void scenarioTimestampedAttributeSenderRegulationTransitionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSenderRegulationTransition(options, model);
}

void scenarioTimestampedAttributeSenderRegulationTransitionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeSenderRegulationTransitionContract(options, model);
}

void scenarioTimestampedAttributeUpdateNoFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateNoFanout(options, model);
}

void scenarioTimestampedAttributeUpdateNoFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateNoFanoutContract(options, model);
}

void scenarioTimestampedAttributeUpdateFlushQueueFutureInputPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateFlushQueueFutureInput(options, model);
}

void scenarioTimestampedAttributeUpdateFlushQueueFutureInputContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateFlushQueueFutureInputContract(options, model);
}

void scenarioTimestampedInteractionFlushQueueFutureInputPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionFlushQueueFutureInput(options, model);
}

void scenarioTimestampedInteractionFlushQueueFutureInputContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionFlushQueueFutureInputContract(options, model);
}

void scenarioFqrPostgrantLowerTsoRetractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqrPostgrantLowerTsoRetraction(options, model);
}

void scenarioFqrPostgrantLowerTsoRetractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqrPostgrantLowerTsoRetractionContract(options, model);
}

void scenarioFqgPostgrantDeliveryCutoffPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqgPostgrantDeliveryCutoff(options, model);
}

void scenarioFqgPostgrantDeliveryCutoffContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqgPostgrantDeliveryCutoffContract(options, model);
}

void scenarioTarQueuedTsoBeforeGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTarQueuedTsoBeforeGrant(options, model);
}

void scenarioTarQueuedTsoBeforeGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTarQueuedTsoBeforeGrantContract(options, model);
}

void scenarioTimestampedAttributeOrderCohortPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeOrderCohort(options, model);
}

void scenarioTimestampedAttributeOrderCohortContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeOrderCohortContract(options, model);
}

void scenarioTimestampedObjectDeletionNoFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionNoFanout(options, model);
}

void scenarioTimestampedObjectDeletionNoFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionNoFanoutContract(options, model);
}

void scenarioTimestampedObjectDeletionTombstonePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionTombstone(options, model);
}

void scenarioTimestampedObjectDeletionTombstoneContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionTombstoneContract(options, model);
}

void scenarioTimestampedObjectDeletionJoinedOwnerRetractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionJoinedOwnerRetraction(options, model);
}

void scenarioTimestampedObjectDeletionJoinedOwnerRetractionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionJoinedOwnerRetractionContract(options, model);
}

void scenarioTimestampedObjectDeletionSourceResignationFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionSourceResignationFanout(options, model);
}

void scenarioTimestampedObjectDeletionSourceResignationFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionSourceResignationFanoutContract(options, model);
}

void scenarioTimestampedObjectDeletionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletion(options, model);
}

void scenarioTimestampedObjectDeletionContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionContract(options, model);
}

void scenarioTimestampedObjectDeletionRegulationReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionRegulationReenable(options, model);
}

void scenarioTimestampedObjectDeletionRegulationReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionRegulationReenableContract(options, model);
}

void scenarioAlternateTimeAdvancesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAlternateTimeAdvances(options, model);
}

void scenarioAlternateTimeAdvancesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAlternateTimeAdvancesContract(options, model);
}

void scenarioPendingAlternateAdvanceState(
    Options const& options,
    rti::CallbackModel model) {
  Session regulator(options, model, "pending-alternate-advance-regulator");
  Session constrained(options, model, "pending-alternate-advance-constrained");
  auto const federation = federationName(options, "pending-alternate-advance");
  connectAndJoin(regulator, constrained, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      regulatorTime.factory->getName() == constrainedTime.factory->getName(),
      "pending alternate advances selected different logical-time factories");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() == 1U; },
      options,
      "pending alternate advances time-constrained callback");
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() == 1U; },
      options,
      "pending alternate advances time-regulation callback");

  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      1U);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  requireException(
      [&] {
        constrained.rtiAmbassador().timeAdvanceRequestAvailable(
            *constrainedTarget);
      },
      L"InTimeAdvancingState",
      "time advance request available while TAR is pending");
  requireException(
      [&] {
        constrained.rtiAmbassador().nextMessageRequestAvailable(
            *constrainedTarget);
      },
      L"InTimeAdvancingState",
      "next message request available while TAR is pending");
  requireException(
      [&] { constrained.rtiAmbassador().flushQueueRequest(*constrainedTarget); },
      L"InTimeAdvancingState",
      "flush queue request while TAR is pending");
  requireException(
      [&] { constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget); },
      L"InTimeAdvancingState",
      "second time advance request while TAR is pending");
  require(
      constrained.recorder().timeAdvanceGrants().empty(),
      "rejected alternate requests completed the original pending TAR");

  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  waitFor(
      regulator,
      constrained,
      [&] {
        return regulator.recorder().timeAdvanceGrants().size() == 1U &&
            constrained.recorder().timeAdvanceGrants().size() == 1U;
      },
      options,
      "pending alternate advances released by regulator grant");
  auto const regulatorGrants = regulator.recorder().timeAdvanceGrants();
  auto const constrainedGrants = constrained.recorder().timeAdvanceGrants();
  require(
      regulatorGrants.front().encoded == encodeTime(*regulatorTarget) &&
          constrainedGrants.front().encoded == encodeTime(*constrainedTarget),
      "pending alternate advances returned the wrong grant times");

  constrained.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  regulator.disconnect();
}

void scenarioPendingAlternateAdvanceStateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPendingAlternateAdvanceState(options, model);
}

void scenarioNextMessageRequestPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNextMessageRequest(options, model);
}

void scenarioNextMessageRequestContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNextMessageRequestContract(options, model);
}

void scenarioNmrNoPregrantTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoPregrantTsoFloor(options, model);
}

void scenarioNmrNoPregrantTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrNoPregrantTsoFloorContract(options, model);
}

void scenarioNmrReceivedTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoFloor(options, model);
}

void scenarioNmrReceivedTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoFloorContract(options, model);
}

void scenarioNmrReceivedTsoAttributeUpdateFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoAttributeUpdateFloor(options, model);
}

void scenarioNmrReceivedTsoAttributeUpdateFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoAttributeUpdateFloorContract(options, model);
}

void scenarioNmrReceivedTsoDirectedInteractionFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoDirectedInteractionFloor(options, model);
}

void scenarioNmrReceivedTsoDirectedInteractionFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrReceivedTsoDirectedInteractionFloorContract(options, model);
}

void scenarioNmrEmptyMessageSetGrantTimePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrEmptyMessageSetGrantTime(options, model);
}

void scenarioNmrEmptyMessageSetGrantTimeContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrEmptyMessageSetGrantTimeContract(options, model);
}

void scenarioNmrRequestBelowGaltImmediateGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRequestBelowGaltImmediateGrant(options, model);
}

void scenarioNmrRequestBelowGaltImmediateGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrRequestBelowGaltImmediateGrantContract(options, model);
}

void scenarioNmrLitsBelowGaltImmediateGrantPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrLitsBelowGaltImmediateGrant(options, model);
}

void scenarioNmrLitsBelowGaltImmediateGrantContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrLitsBelowGaltImmediateGrantContract(options, model);
}

void scenarioNmraNoPregrantTsoFloorPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoPregrantTsoFloor(options, model);
}

void scenarioNmraNoPregrantTsoFloorContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraNoPregrantTsoFloorContract(options, model);
}

void scenarioSendMessageTimeStatePreservationGrantedPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioSendMessageTimeStatePreservationGranted(options, model);
}

void scenarioSendMessageTimeStatePreservationGrantedContractPortable(
    Options const& options, rti::CallbackModel model) {
  scenarioSendMessageTimeStatePreservationGrantedContract(options, model);
}

void scenarioAvailableTimeAdvancesInclusiveGaltPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAvailableTimeAdvancesInclusiveGalt(options, model);
}

void scenarioAvailableTimeAdvancesInclusiveGaltContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAvailableTimeAdvancesInclusiveGaltContract(options, model);
}

void scenarioTimeConstrainedEnableAtGaltEqualityPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableAtGaltEquality(options, model);
}

void scenarioTimeConstrainedEnableAtGaltEqualityContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeConstrainedEnableAtGaltEqualityContract(options, model);
}

void scenarioTimeRegulationImplicitAdvancePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationImplicitAdvance(options, model);
}

void scenarioTimeRegulationImplicitAdvanceContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeRegulationImplicitAdvanceContract(options, model);
}

void scenarioTimestampedInteractionSourceResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionSourceResignation(options, model);
}

void scenarioTimeCoordinatedInteractionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimeCoordinatedInteraction(options, model);
}

void scenarioEmptyAttributeSubscriptionIsNoOpPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioEmptyAttributeSubscriptionIsNoOp(options, model);
}

void scenarioNamedUpdateRateSubscriptionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNamedUpdateRateSubscription(options, model);
}

void scenarioDefaultRateSubscriptionDoesNotReducePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDefaultRateSubscriptionDoesNotReduce(options, model);
}

void scenarioAttributeSubscriptionInvocationsAccumulatePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeSubscriptionInvocationsAccumulate(options, model);
}

void scenarioAttributeSubscriptionPerAttributeModeTransitionsPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeSubscriptionPerAttributeModeTransitions(options, model);
}

void scenarioAttributeSubscriptionPerAttributeModeTransitionsContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeSubscriptionPerAttributeModeTransitionsContract(options, model);
}

void scenarioPassiveSubscriptionDoesNotStartRegistrationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPassiveSubscriptionDoesNotStartRegistration(options, model);
}

void scenarioSelectiveAttributeUnsubscriptionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSelectiveAttributeUnsubscription(options, model);
}

void scenarioWholeClassAttributeUnsubscriptionPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioWholeClassAttributeUnsubscription(options, model);
}

void scenarioEmptyAttributeUnsubscriptionIsNoOpPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioEmptyAttributeUnsubscriptionIsNoOp(options, model);
}

void scenarioTimestampedInteractionSourceResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionSourceResignationContract(options, model);
}

void scenarioTimestampedInteractionSourceResignationFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionSourceResignationFanout(options, model);
}

void scenarioTimestampedInteractionSourceResignationFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionSourceResignationFanoutContract(options, model);
}

void scenarioTimestampedInteractionTsoDesignatorTerminalizationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTsoDesignatorTerminalization(options, model);
}

void scenarioTimestampedInteractionTsoDesignatorTerminalizationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionTsoDesignatorTerminalizationContract(options, model);
}

void scenarioTimestampedInteractionCrossProducerOrderPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionCrossProducerOrder(options, model);
}

void scenarioTimestampedInteractionCrossProducerOrderContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionCrossProducerOrderContract(options, model);
}

void scenarioTimestampedInteractionNoFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionNoFanout(options, model);
}

void scenarioTimestampedInteractionNoFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionNoFanoutContract(options, model);
}

void scenarioTimestampedInteractionRetractionFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionRetractionFanout(options, model);
}

void scenarioTimestampedInteractionRetractionFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionRetractionFanoutContract(options, model);
}

void scenarioTimestampedInteractionReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionReenable(options, model);
}

void scenarioTimestampedInteractionReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionReenableContract(options, model);
}

void scenarioTimestampedDirectedInteractionReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionReenable(options, model);
}

void scenarioTimestampedDirectedInteractionReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionReenableContract(options, model);
}

void scenarioTimestampedInteractionRegulationReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionRegulationReenable(options, model);
}

void scenarioTimestampedInteractionRegulationReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedInteractionRegulationReenableContract(options, model);
}

void scenarioTimestampedDirectedInteractionRegulationReenablePortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionRegulationReenable(options, model);
}

void scenarioTimestampedDirectedInteractionRegulationReenableContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionRegulationReenableContract(
      options, model);
}

void scenarioTimestampedDirectedAlternateAdvancesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedAlternateAdvances(options, model);
}

void scenarioTimestampedDirectedAlternateAdvancesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedAlternateAdvancesContract(options, model);
}

void scenarioTimestampedAttributeUpdateAlternateAdvancesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateAlternateAdvances(options, model);
}

void scenarioTimestampedAttributeUpdateAlternateAdvancesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedAttributeUpdateAlternateAdvancesContract(options, model);
}

void scenarioTimestampedObjectDeletionMixedAdvancesPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionMixedAdvances(options, model);
}

void scenarioTimestampedObjectDeletionMixedAdvancesContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedObjectDeletionMixedAdvancesContract(options, model);
}

void scenarioTimestampedObjectDeletionFutureNmrArrivalPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrFutureTimestampedObjectDeletion(options, model);
}

void scenarioTimestampedObjectDeletionFutureNmrArrivalContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmrFutureTimestampedObjectDeletionContract(options, model);
}

void scenarioTimestampedObjectDeletionFutureNmraArrivalPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraFutureTimestampedObjectDeletion(options, model);
}

void scenarioTimestampedObjectDeletionFutureNmraArrivalContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNmraFutureTimestampedObjectDeletionContract(options, model);
}

void scenarioTimestampedDirectedInteractionTarNmrPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionTarNmr(options, model);
}

void scenarioTimestampedDirectedInteractionTarNmrContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionTarNmrContract(options, model);
}

void scenarioTimestampedDirectedInteractionSourceResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionSourceResignation(options, model);
}

void scenarioTimestampedDirectedInteractionSourceResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionSourceResignationContract(options, model);
}

void scenarioTimestampedDirectedInteractionSourceResignationFanoutPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionSourceResignationFanout(options, model);
}

void scenarioTimestampedDirectedInteractionSourceResignationFanoutContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionSourceResignationFanoutContract(options, model);
}

void scenarioTimestampedDirectedInteractionImmediateSourceResignationPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionImmediateSourceResignation(options, model);
}

void scenarioTimestampedDirectedInteractionImmediateSourceResignationContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionImmediateSourceResignationContract(options, model);
}

void scenarioSynchronizationPointsPortable(
    Options const& options,
    rti::CallbackModel model) {
  Session lifecycle(options, model, "synchronization-lifecycle");
  rti::VariableLengthData const emptyTag;
  rti::FederateHandleSet const emptySynchronizationSet;
  requireException(
      [&] {
        lifecycle.rtiAmbassador().registerFederationSynchronizationPoint(
            L"tck-lifecycle-global-before-connect",
            emptyTag);
      },
      L"NotConnected",
      "global synchronization-point registration before connect");
  requireException(
      [&] {
        lifecycle.rtiAmbassador().registerFederationSynchronizationPoint(
            L"tck-lifecycle-explicit-before-connect",
            emptyTag,
            emptySynchronizationSet);
      },
      L"NotConnected",
      "explicit synchronization-point registration before connect");
  requireException(
      [&] {
        lifecycle.rtiAmbassador().synchronizationPointAchieved(
            L"tck-lifecycle-achieved-before-connect");
      },
      L"NotConnected",
      "synchronization-point achievement before connect");
  lifecycle.connect();
  requireException(
      [&] {
        lifecycle.rtiAmbassador().registerFederationSynchronizationPoint(
            L"tck-lifecycle-global-before-join",
            emptyTag);
      },
      L"FederateNotExecutionMember",
      "global synchronization-point registration before joining an execution");
  requireException(
      [&] {
        lifecycle.rtiAmbassador().registerFederationSynchronizationPoint(
            L"tck-lifecycle-explicit-before-join",
            emptyTag,
            emptySynchronizationSet);
      },
      L"FederateNotExecutionMember",
      "explicit synchronization-point registration before joining an execution");
  requireException(
      [&] {
        lifecycle.rtiAmbassador().synchronizationPointAchieved(
            L"tck-lifecycle-achieved-before-join");
      },
      L"FederateNotExecutionMember",
      "synchronization-point achievement before joining an execution");
  lifecycle.disconnect();

  Session first(options, model, "owner");
  Session second(options, model, "member");
  Session third(options, model, "late");
  auto const federation = federationName(options, "synchronization-points");

  first.connect();
  second.connect();
  third.connect();
  first.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  first.join(options.ownerFederateName, options.federateType, federation);
  second.join(options.memberFederateName, options.federateType, federation);

  std::vector<std::uint8_t> tagBytes{0x53U, 0x59U, 0x4EU, 0x43U};
  rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
  std::wstring const globalLabel = L"tck-global-synchronization";
  first.rtiAmbassador().registerFederationSynchronizationPoint(globalLabel, tag);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.synchronizationPointRegistrations().size() >= 1U &&
            first.synchronizationPointAnnouncements().size() >= 1U &&
            second.synchronizationPointAnnouncements().size() >= 1U;
      },
      options,
      "global synchronization-point registration and announcement");

  auto const registration = first.synchronizationPointRegistrations().front();
  require(registration.succeeded && registration.label == globalLabel,
          "synchronization-point registration did not succeed for the requested label");
  for (auto* session : {&first, &second}) {
    auto const announcements = session->synchronizationPointAnnouncements();
    require(announcements.front().label == globalLabel,
            "synchronization-point announcement returned the wrong label");
    require(announcements.front().tag == tagBytes,
            "synchronization-point announcement returned the wrong tag");
  }

  // A default synchronization set includes a member that joins after the
  // point is registered, so the late member receives the pending
  // announcement and participates in the completion barrier.
  third.join(L"tck-third", options.federateType, federation);
  waitFor(
      third,
      [&] { return third.synchronizationPointAnnouncements().size() >= 1U; },
      options,
      "late synchronization-point announcement");
  require(
      third.synchronizationPointRegistrations().empty(),
      "late synchronization member received a registration-success callback");
  auto const lateAnnouncements = third.synchronizationPointAnnouncements();
  require(lateAnnouncements.front().label == globalLabel &&
              lateAnnouncements.front().tag == tagBytes,
          "late synchronization-point announcement returned the wrong label or tag");

  // Registration is one-shot by label.  A duplicate is reported to the
  // requesting federate and does not replace the already announced point.
  second.rtiAmbassador().registerFederationSynchronizationPoint(globalLabel, tag);
  waitFor(
      second,
      [&] { return second.synchronizationPointRegistrations().size() >= 1U; },
      options,
      "duplicate synchronization-point registration result");
  auto const duplicate = second.synchronizationPointRegistrations().back();
  require(!duplicate.succeeded && duplicate.label == globalLabel &&
              duplicate.failureReason.has_value() &&
              duplicate.failureReason.value() ==
                  rti::SYNCHRONIZATION_POINT_LABEL_NOT_UNIQUE,
          "duplicate synchronization-point registration returned the wrong result");

  first.rtiAmbassador().synchronizationPointAchieved(globalLabel, true);
  second.rtiAmbassador().synchronizationPointAchieved(globalLabel, false);
  third.rtiAmbassador().synchronizationPointAchieved(globalLabel, true);
  waitForSessions(
      {&first, &second, &third},
      [&] {
        return first.federationSynchronized().size() >= 1U &&
            second.federationSynchronized().size() >= 1U &&
            third.federationSynchronized().size() >= 1U;
      },
      options,
      "synchronization-point completion");
  for (auto* session : {&first, &second, &third}) {
    auto const completions = session->federationSynchronized();
    require(completions.front().label == globalLabel,
            "federation synchronized callback returned the wrong label");
    require(completions.front().failedToSyncSet.count(second.federateHandle()) == 1U,
            "federation synchronized callback omitted the unsuccessful federate");
  }
  requireException(
      [&] { first.rtiAmbassador().synchronizationPointAchieved(globalLabel); },
      L"SynchronizationPointLabelNotAnnounced",
      "re-achieving a completed synchronization point");

  // An explicit synchronization set excludes the third member, which gives
  // the portable test a second registration form without relying on provider
  // membership reports or private synchronization state.
  std::wstring const explicitLabel = L"tck-explicit-synchronization";
  rti::FederateHandleSet invalidExplicitSet{rti::FederateHandle{}};
  requireException(
      [&] {
        first.rtiAmbassador().registerFederationSynchronizationPoint(
            L"tck-invalid-explicit-synchronization",
            tag,
            invalidExplicitSet);
      },
      L"InvalidFederateHandle",
      "explicit synchronization set with an invalid federate handle");
  rti::FederateHandleSet explicitSet{
      first.federateHandle(),
      second.federateHandle()};
  first.rtiAmbassador().registerFederationSynchronizationPoint(
      explicitLabel,
      tag,
      explicitSet);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.synchronizationPointRegistrations().size() >= 2U &&
            first.synchronizationPointAnnouncements().size() >= 2U &&
            second.synchronizationPointAnnouncements().size() >= 2U;
      },
      options,
      "explicit synchronization-set registration and announcement");
  for (int pass = 0; pass != 8; ++pass) {
    third.pump();
  }
  require(third.synchronizationPointAnnouncements().size() == 1U,
          "explicit synchronization set announced to an excluded federate");

  first.rtiAmbassador().synchronizationPointAchieved(explicitLabel);
  second.rtiAmbassador().synchronizationPointAchieved(explicitLabel);
  waitForSessions(
      {&first, &second},
      [&] {
        return first.federationSynchronized().size() >= 2U &&
            second.federationSynchronized().size() >= 2U;
      },
      options,
      "explicit synchronization-set completion");
  require(third.federationSynchronized().size() == 1U,
          "excluded federate received explicit synchronization completion");
  for (auto* session : {&first, &second}) {
    auto const completions = session->federationSynchronized();
    require(completions.back().label == explicitLabel &&
                completions.back().failedToSyncSet.empty(),
            "explicit synchronization completion returned an unexpected failure set");
  }

  third.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  first.resign(rti::NO_ACTION);
  first.rtiAmbassador().destroyFederationExecution(federation);
  third.disconnect();
  second.disconnect();
  first.disconnect();
}

void scenarioSynchronizationPointPortableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSynchronizationPointsPortable(options, model);
}

void scenarioSynchronizationExplicitSetLateJoinPortable(
    Options const& options,
    rti::CallbackModel model) {
  Session registrar(options, model, "synchronization-explicit-set-registrar");
  Session late(options, model, "synchronization-explicit-set-late-member");
  auto const federation = federationName(
      options,
      "synchronization-explicit-set-late-join");

  registrar.connect();
  late.connect();
  registrar.rtiAmbassador().createFederationExecution(
      federation,
      options.fom.wstring(),
      options.logicalTimeImplementationName);
  registrar.join(options.ownerFederateName, options.federateType, federation);

  std::wstring const label = L"tck-explicit-set-late-join";
  std::vector<std::uint8_t> tagBytes{0x53U, 0x45U, 0x54U};
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  rti::FederateHandleSet const synchronizationSet{registrar.federateHandle()};
  registrar.rtiAmbassador().registerFederationSynchronizationPoint(
      label,
      tag,
      synchronizationSet);
  waitFor(
      registrar,
      [&] {
        return registrar.synchronizationPointRegistrations().size() == 1U &&
            registrar.synchronizationPointAnnouncements().size() == 1U;
      },
      options,
      "explicit synchronization-set registration before late join");
  auto const registration = registrar.synchronizationPointRegistrations().front();
  auto const announcement = registrar.synchronizationPointAnnouncements().front();
  require(
      registration.succeeded && registration.label == label,
      "explicit synchronization-set registration returned the wrong result");
  require(
      announcement.label == label && announcement.tag == tagBytes,
      "explicit synchronization-set announcement returned the wrong label or tag");

  late.join(L"tck-explicit-set-late-member", options.federateType, federation);
  requireException(
      [&] { late.rtiAmbassador().synchronizationPointAchieved(label); },
      L"SynchronizationPointLabelNotAnnounced",
      "late joiner outside an explicit synchronization set achieved its label");

  registrar.rtiAmbassador().synchronizationPointAchieved(label);
  waitFor(
      registrar,
      [&] { return registrar.federationSynchronized().size() == 1U; },
      options,
      "single-member explicit synchronization-set completion");
  auto const completion = registrar.federationSynchronized().front();
  require(
      completion.label == label && completion.failedToSyncSet.empty(),
      "explicit synchronization-set completion returned the wrong label or failed set");

  late.resign(rti::NO_ACTION);
  registrar.resign(rti::NO_ACTION);
  registrar.rtiAmbassador().destroyFederationExecution(federation);
  late.disconnect();
  registrar.disconnect();
}

void scenarioSynchronizationExplicitSetLateJoinContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioSynchronizationExplicitSetLateJoinPortable(options, model);
}

void scenarioAutoProvideDisabledDiscoveryOnly(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Disabled Auto Provide testing requires an adapter-supplied ordinary FOM");
  Session owner(options, model, "auto-provide-disabled-owner");
  Session requester(options, model, "auto-provide-disabled-requester");
  auto const federation = federationName(
      options,
      "auto-provide-disabled-discovery-only");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Disabled Auto Provide lookup returned an invalid standard handle");
  require(
      !owner.rtiAmbassador().getAutoProvideSwitch() &&
          !requester.rtiAmbassador().getAutoProvideSwitch(),
      "ordinary adapter FOM unexpectedly enabled the standard Auto Provide switch");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  owner.recorder().clearProvidedUpdates();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Disabled Auto Provide registration returned an invalid object handle");

  waitFor(
      owner,
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "disabled Auto Provide discovery");
  if (model == rti::HLA_EVOKED) {
    static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
  } else {
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  }
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled Auto Provide generated a provideAttributeValueUpdate callback");
  auto const discovery = requester.recorder().discovery();
  require(
      discovery.present && discovery.object == object &&
          discovery.objectClass == requesterClass,
      "disabled Auto Provide discovery returned the wrong standard object");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAutoProvideDisabledDiscoveryOnlyContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutoProvideDisabledDiscoveryOnly(options, model);
}

void scenarioAutoProvideDisabledExplicitRequest(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Disabled Auto Provide explicit-request testing requires an adapter-supplied ordinary FOM");
  Session owner(options, model, "auto-provide-disabled-request-owner");
  Session requester(options, model, "auto-provide-disabled-request-requester");
  auto const federation = federationName(
      options,
      "auto-provide-disabled-explicit-request");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Disabled Auto Provide explicit-request lookup returned an invalid standard handle");
  require(
      !owner.rtiAmbassador().getAutoProvideSwitch() &&
          !requester.rtiAmbassador().getAutoProvideSwitch(),
      "ordinary adapter FOM unexpectedly enabled the standard Auto Provide switch");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  owner.recorder().clearProvidedUpdates();
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Disabled Auto Provide explicit-request registration returned an invalid object handle");

  waitFor(
      owner,
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "disabled Auto Provide explicit-request discovery");
  require(
      owner.recorder().providedUpdates().empty(),
      "disabled Auto Provide emitted a solicitation before the explicit request");

  auto const discoveredObject = requester.recorder().discovery().object;
  std::vector<std::uint8_t> const requestTagBytes{0x51U, 0x52U, 0x54U};
  rti::VariableLengthData const requestTag(
      requestTagBytes.data(),
      requestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      discoveredObject,
      requesterAttributes,
      requestTag);
  waitFor(
      owner,
      requester,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "disabled Auto Provide explicit provider request");

  auto const providedUpdates = owner.recorder().providedUpdates();
  require(
      providedUpdates.size() == 1U,
      "disabled Auto Provide explicit request produced an unexpected callback count");
  require(
      providedUpdates.front().object == object &&
          providedUpdates.front().attributes == ownerAttributes &&
          providedUpdates.front().tag == requestTagBytes,
      "disabled Auto Provide explicit request returned the wrong object, attributes, or tag");

  owner.recorder().clearProvidedUpdates();
  std::vector<std::uint8_t> const classRequestTagBytes{0x43U, 0x4CU, 0x53U};
  rti::VariableLengthData const classRequestTag(
      classRequestTagBytes.data(),
      classRequestTagBytes.size());
  requester.rtiAmbassador().requestAttributeValueUpdate(
      requesterClass,
      requesterAttributes,
      classRequestTag);
  waitFor(
      owner,
      requester,
      [&] { return owner.recorder().providedUpdates().size() == 1U; },
      options,
      "disabled Auto Provide explicit class request");
  auto const classProvidedUpdates = owner.recorder().providedUpdates();
  require(
      classProvidedUpdates.size() == 1U &&
          classProvidedUpdates.front().object == object &&
          classProvidedUpdates.front().attributes == ownerAttributes &&
          classProvidedUpdates.front().tag == classRequestTagBytes,
      "disabled Auto Provide explicit class request returned the wrong callback data");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAutoProvideDisabledExplicitRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutoProvideDisabledExplicitRequest(options, model);
}

void scenarioFederationAutoProvideMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM Auto Provide testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "federation-auto-provide-mom-switch");
  owner.connect();
  requester.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(options.ownerFederateName, options.federateType, federation);
  requester.join(options.memberFederateName, options.federateType, federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  auto const ownerSetSwitches = owner.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
  auto const requesterSetSwitches =
      requester.rtiAmbassador().getInteractionClassHandle(
          L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAadjust.HLAsetSwitches");
  auto const ownerAutoProvide = owner.rtiAmbassador().getParameterHandle(
      ownerSetSwitches,
      L"HLAautoProvide");
  auto const requesterAutoProvide = requester.rtiAmbassador().getParameterHandle(
      requesterSetSwitches,
      L"HLAautoProvide");
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid() &&
          ownerSetSwitches.isValid() && requesterSetSwitches.isValid() &&
          ownerAutoProvide.isValid() && requesterAutoProvide.isValid(),
      "Standard application or federation-MOM lookup returned an invalid handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto sendAutoProvideSwitch = [](
      Session& setter,
      auto const& setSwitches,
      auto const& autoProvideParameter,
      bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {autoProvideParameter, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    setter.rtiAmbassador().sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
  };
  auto registerAndObserveDiscovery = [&](char const* stage) {
    requester.recorder().clearDiscovery();
    auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
    require(object.isValid(), std::string(stage) + " returned an invalid object handle");
    waitFor(
        owner,
        requester,
        [&] { return requester.recorder().hasDiscovery(object); },
        options,
        stage);
    auto const discovery = requester.recorder().discovery();
    require(
        discovery.present && discovery.object == object &&
            discovery.objectClass == requesterClass,
        std::string(stage) + " delivered the wrong standard discovery data");
    return object;
  };
  auto drainOwnerCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      static_cast<void>(owner.evokeMultipleCallbacks(0.0, 1.0));
    } else {
      static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
          options.objectClassName));
    }
  };

  // A joined non-owner federate sets the switch. Discovery remains active, but
  // the registered publisher must not receive a provide-value request.
  sendAutoProvideSwitch(
      requester,
      requesterSetSwitches,
      requesterAutoProvide,
      false);
  static_cast<void>(registerAndObserveDiscovery(
      "federation MOM Auto Provide disabled discovery"));
  drainOwnerCallbacks();
  require(
      owner.recorder().providedUpdates().empty(),
      "federation MOM Auto Provide false still solicited the provider");

  // The member enables the federation-wide switch. The next ordinary object
  // registration must request exactly its published attribute from the owner.
  sendAutoProvideSwitch(
      requester,
      requesterSetSwitches,
      requesterAutoProvide,
      true);
  auto const enabledObject = registerAndObserveDiscovery(
      "federation MOM Auto Provide enabled discovery");
  waitFor(
      owner,
      requester,
      [&] {
        return requester.recorder().hasDiscovery(enabledObject) &&
            owner.recorder().providedUpdates().size() == 1U;
      },
      options,
      "federation MOM Auto Provide provider request");
  auto const enabledRequests = owner.recorder().providedUpdates();
  require(
      enabledRequests.size() == 1U &&
          enabledRequests.front().object == enabledObject &&
          enabledRequests.front().attributes == ownerAttributes &&
          enabledRequests.front().tag.empty(),
      "federation MOM Auto Provide true returned the wrong provider callback data");

  // The owner disables it; a third object is still discovered without another
  // provider request, demonstrating federation-wide off behavior as well.
  sendAutoProvideSwitch(
      owner,
      ownerSetSwitches,
      ownerAutoProvide,
      false);
  static_cast<void>(registerAndObserveDiscovery(
      "federation MOM Auto Provide disabled-again discovery"));
  drainOwnerCallbacks();
  require(
      owner.recorder().providedUpdates().size() == 1U,
      "federation MOM Auto Provide false generated an additional provider request");

  requester.rtiAmbassador().unsubscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioFederationAutoProvideMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationAutoProvideMomSwitch(options, model);
}

void scenarioFederateServiceReportingMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM Service Reporting testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-service-reporting-mom-switch");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-service-reporting-mom-switch",
      options.federateType,
      federation);

  auto const setSwitches = federate.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const serviceReporting = federate.rtiAmbassador().getParameterHandle(
      setSwitches,
      L"HLAserviceReporting");
  require(
      setSwitches.isValid() && serviceReporting.isValid(),
      "Standard federate-MOM Service Reporting lookup returned an invalid handle");
  require(
      !federate.rtiAmbassador().getServiceReportingSwitch(),
      "Standard MIM Service Reporting switch did not start false");

  auto setReportingThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {serviceReporting, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    federate.rtiAmbassador().sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        federate.rtiAmbassador().getServiceReportingSwitch() == enabled,
        std::string("HLAsetSwitches HLAserviceReporting did not set the switch to ") +
            (enabled ? "true" : "false"));
  };
  setReportingThroughMom(true);
  setReportingThroughMom(false);

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateServiceReportingMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateServiceReportingMomSwitch(options, model);
}

void scenarioFederateExceptionReportingMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM Exception Reporting testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-exception-reporting-mom-switch");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-exception-reporting-mom-switch",
      options.federateType,
      federation);

  auto const setSwitches = federate.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const exceptionReporting = federate.rtiAmbassador().getParameterHandle(
      setSwitches,
      L"HLAexceptionReporting");
  require(
      setSwitches.isValid() && exceptionReporting.isValid(),
      "Standard federate-MOM Exception Reporting lookup returned an invalid handle");
  require(
      !federate.rtiAmbassador().getExceptionReportingSwitch(),
      "Standard MIM Exception Reporting switch did not start false");

  auto const initialServiceReporting =
      federate.rtiAmbassador().getServiceReportingSwitch();
  auto const initialResignAction =
      federate.rtiAmbassador().getAutomaticResignDirective();
  auto const initialFileReporting =
      federate.rtiAmbassador().getSendServiceReportsToFileSwitch();
  auto setReportingThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {exceptionReporting, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    federate.rtiAmbassador().sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        federate.rtiAmbassador().getExceptionReportingSwitch() == enabled,
        std::string("HLAsetSwitches HLAexceptionReporting did not set the switch to ") +
            (enabled ? "true" : "false"));
    require(
        federate.rtiAmbassador().getServiceReportingSwitch() ==
                initialServiceReporting &&
            federate.rtiAmbassador().getAutomaticResignDirective() ==
                initialResignAction &&
            federate.rtiAmbassador().getSendServiceReportsToFileSwitch() ==
                initialFileReporting,
        "HLAsetSwitches HLAexceptionReporting changed a neighboring federate switch");
  };
  setReportingThroughMom(true);
  setReportingThroughMom(false);

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateExceptionReportingMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateExceptionReportingMomSwitch(options, model);
}

template <typename Getter, typename Setter>
void runBooleanFederateMomAttributeScenario(
    Options const& options,
    rti::CallbackModel model,
    char const* federationSuffix,
    wchar_t const* attributeName,
    char const* attributeDisplayName,
    Getter getter,
    Setter setter) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      std::string(attributeDisplayName) +
          " MOM attribute testing requires adapter-supplied FOM and standard MIM");

  Session monitor(options, model, "boolean-federate-mom-attribute-monitor");
  Session subject(options, model, "boolean-federate-mom-attribute-subject");
  auto const federation = federationName(options, federationSuffix);
  monitor.connect();
  subject.connect();
  if (options.logicalTimeImplementationName.empty()) {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  monitor.join(
      options.memberFederateName + L"-boolean-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const valueAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      attributeName);
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          valueAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, valueAttribute) ==
              attributeName &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      std::string(attributeDisplayName) +
          " MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, valueAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-boolean-mom-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      std::string(attributeDisplayName) + " subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      std::string(attributeDisplayName) +
          " discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(valueAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto validateValueReflection = [&](ReflectionRecord const& reflection,
                                     bool const expected,
                                     char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) + " had non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(valueAttribute));
    require(
        value.get() == (expected ? 1 : 0),
        std::string(description) + " returned the wrong HLAswitch value");
  };
  auto requestValue = [&](bool const expected, char const* description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{valueAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " delivered an unexpected number of direct MOM reflections");
    validateValueReflection(reflections.front(), expected, description);
  };

  auto& subjectAmbassador = subject.rtiAmbassador();
  bool const initialValue = getter(subjectAmbassador);
  requestValue(initialValue, "initial direct MOM attribute value request");

  bool const changedValue = !initialValue;
  auto beforeChange = monitor.recorder().reflections().size();
  setter(subjectAmbassador, changedValue);
  require(
      getter(subjectAmbassador) == changedValue,
      std::string(attributeDisplayName) +
          " public setter did not retain the changed value");
  waitFor(
      monitor,
      [&] { return !valueReflectionsSince(beforeChange).empty(); },
      options,
      std::string(attributeDisplayName) + " conditional MOM reflection");
  auto const changedReflections = valueReflectionsSince(beforeChange);
  require(
      changedReflections.size() == 1U,
      std::string(attributeDisplayName) +
          " change did not produce exactly one conditional MOM reflection");
  validateValueReflection(
      changedReflections.front(),
      changedValue,
      "conditional MOM attribute reflection");
  requestValue(changedValue, "changed direct MOM attribute value request");

  beforeChange = monitor.recorder().reflections().size();
  setter(subjectAmbassador, initialValue);
  require(
      getter(subjectAmbassador) == initialValue,
      std::string(attributeDisplayName) +
          " public setter did not restore the initial value");
  waitFor(
      monitor,
      [&] { return !valueReflectionsSince(beforeChange).empty(); },
      options,
      std::string(attributeDisplayName) + " restored conditional MOM reflection");
  auto const restoredReflections = valueReflectionsSince(beforeChange);
  require(
      restoredReflections.size() == 1U,
      std::string(attributeDisplayName) +
          " restoration did not produce exactly one conditional MOM reflection");
  validateValueReflection(
      restoredReflections.front(),
      initialValue,
      "restored conditional MOM attribute reflection");
  requestValue(initialValue, "restored direct MOM attribute value request");

  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void runAsynchronousDeliveryMomAttributeScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Asynchronous Delivery MOM attribute testing requires adapter-supplied FOM and standard MIM");

  Session monitor(options, model, "asynchronous-delivery-mom-attribute-monitor");
  Session subject(options, model, "asynchronous-delivery-mom-attribute-subject");
  auto const federation = federationName(
      options,
      "federate-asynchronous-delivery-mim-attribute");
  monitor.connect();
  subject.connect();
  if (options.logicalTimeImplementationName.empty()) {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  monitor.join(
      options.memberFederateName + L"-async-delivery-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const valueAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAasynchronousDelivery");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          valueAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, valueAttribute) ==
              L"HLAasynchronousDelivery" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Asynchronous Delivery MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, valueAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-async-delivery-mom-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Asynchronous Delivery subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Asynchronous Delivery discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(valueAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto validateValueReflection = [&](ReflectionRecord const& reflection,
                                     bool const expected,
                                     char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard asynchronous-delivery MOM reflection metadata");
    rti::HLAboolean value;
    value.decode(reflection.values.at(valueAttribute));
    require(
        value.get() == expected,
        std::string(description) + " returned the wrong HLAboolean value");
  };
  auto requestValue = [&](char const* description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{valueAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " delivered an unexpected number of direct MOM reflections");
    rti::HLAboolean value;
    value.decode(reflections.front().values.at(valueAttribute));
    validateValueReflection(reflections.front(), value.get(), description);
    return value.get();
  };

  auto transitionTo = [&](bool const enabled, char const* description) {
    auto const before = monitor.recorder().reflections().size();
    if (enabled) {
      subject.rtiAmbassador().enableAsynchronousDelivery();
    } else {
      subject.rtiAmbassador().disableAsynchronousDelivery();
    }
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " did not produce exactly one conditional MOM reflection");
    validateValueReflection(
        reflections.front(),
        enabled,
        "conditional asynchronous-delivery MOM reflection");
    require(
        requestValue("direct asynchronous-delivery MOM value request") ==
            enabled,
        "Direct HLAasynchronousDelivery value disagreed with the service transition");
  };

  bool const initialValue =
      requestValue("initial asynchronous-delivery MOM value request");
  transitionTo(
      !initialValue,
      "asynchronous-delivery state-change MOM reflection");
  transitionTo(
      initialValue,
      "restored asynchronous-delivery MOM reflection");

  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void runTimeConstrainedMomAttributeScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Time Constrained MOM attribute testing requires adapter-supplied FOM and standard MIM");

  Session monitor(options, model, "time-constrained-mom-attribute-monitor");
  Session subject(options, model, "time-constrained-mom-attribute-subject");
  auto const federation = federationName(
      options,
      "federate-time-constrained-mim-attribute");
  monitor.connect();
  subject.connect();
  if (options.logicalTimeImplementationName.empty()) {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  monitor.join(
      options.memberFederateName + L"-time-constrained-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const valueAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAtimeConstrained");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          valueAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, valueAttribute) ==
              L"HLAtimeConstrained" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Time Constrained MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, valueAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-time-constrained-mom-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Time Constrained subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Time Constrained discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(valueAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto validateValueReflection = [&](ReflectionRecord const& reflection,
                                     bool const expected,
                                     char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard time-constrained MOM reflection metadata");
    rti::HLAboolean value;
    value.decode(reflection.values.at(valueAttribute));
    require(
        value.get() == expected,
        std::string(description) + " returned the wrong HLAboolean value");
  };
  auto requestValue = [&](char const* description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{valueAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " delivered an unexpected number of direct MOM reflections");
    rti::HLAboolean value;
    value.decode(reflections.front().values.at(valueAttribute));
    validateValueReflection(reflections.front(), value.get(), description);
    return value.get();
  };

  auto transitionTo = [&](bool const enabled, char const* description) {
    auto const before = monitor.recorder().reflections().size();
    auto const callbacksBefore =
        subject.recorder().timeConstrainedEnabled().size();
    if (enabled) {
      subject.rtiAmbassador().enableTimeConstrained();
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeConstrainedEnabled().size() >=
                callbacksBefore + 1U;
          },
          options,
          "timeConstrainedEnabled callback after Enable Time Constrained");
      require(
          subject.recorder().timeConstrainedEnabled().size() ==
              callbacksBefore + 1U,
          "Enable Time Constrained delivered an unexpected number of callbacks");
    } else {
      subject.rtiAmbassador().disableTimeConstrained();
      require(
          subject.recorder().timeConstrainedEnabled().size() == callbacksBefore,
          "Disable Time Constrained delivered a timeConstrainedEnabled callback");
    }
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " did not produce exactly one conditional MOM reflection");
    validateValueReflection(
        reflections.front(),
        enabled,
        "conditional time-constrained MOM reflection");
    require(
        requestValue("direct time-constrained MOM value request") == enabled,
        "Direct HLAtimeConstrained value disagreed with the service transition");
  };

  bool const initialValue =
      requestValue("initial time-constrained MOM value request");
  transitionTo(
      !initialValue,
      "time-constrained state-change MOM reflection");
  transitionTo(
      initialValue,
      "restored time-constrained MOM reflection");

  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void runTimeRegulatingMomAttributeScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Time Regulation MOM attribute testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session monitor(options, model, "time-regulating-mom-attribute-monitor");
  Session subject(options, model, "time-regulating-mom-attribute-subject");
  auto const federation = federationName(
      options,
      "federate-time-regulating-mim-attribute");
  monitor.connect();
  subject.connect();
  monitor.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  monitor.join(
      options.memberFederateName + L"-time-regulating-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const valueAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAtimeRegulating");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          valueAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, valueAttribute) ==
              L"HLAtimeRegulating" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Time Regulation MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, valueAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-time-regulating-mom-subject",
      options.federateType,
      federation);
  auto const subjectTime = makeTimeContext(subject);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Time Regulation subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Time Regulation discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(valueAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto validateValueReflection = [&](ReflectionRecord const& reflection,
                                     bool const expected,
                                     char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard time-regulating MOM reflection metadata");
    rti::HLAboolean value;
    value.decode(reflection.values.at(valueAttribute));
    require(
        value.get() == expected,
        std::string(description) + " returned the wrong HLAboolean value");
  };
  auto requestValue = [&](char const* description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{valueAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " delivered an unexpected number of direct MOM reflections");
    rti::HLAboolean value;
    value.decode(reflections.front().values.at(valueAttribute));
    validateValueReflection(reflections.front(), value.get(), description);
    return value.get();
  };

  auto transitionTo = [&](bool const enabled, char const* description) {
    auto const before = monitor.recorder().reflections().size();
    auto const callbacksBefore = subject.recorder().timeRegulationEnabled().size();
    if (enabled) {
      subject.rtiAmbassador().enableTimeRegulation(*subjectTime.epsilon);
      waitFor(
          subject,
          [&] {
            return subject.recorder().timeRegulationEnabled().size() >=
                callbacksBefore + 1U;
          },
          options,
          "timeRegulationEnabled callback after Enable Time Regulation");
      require(
          subject.recorder().timeRegulationEnabled().size() ==
              callbacksBefore + 1U,
          "Enable Time Regulation delivered an unexpected number of callbacks");
    } else {
      subject.rtiAmbassador().disableTimeRegulation();
      require(
          subject.recorder().timeRegulationEnabled().size() == callbacksBefore,
          "Disable Time Regulation delivered a timeRegulationEnabled callback");
    }
    waitFor(
        monitor,
        [&] { return !valueReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = valueReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " did not produce exactly one conditional MOM reflection");
    validateValueReflection(
        reflections.front(),
        enabled,
        "conditional time-regulating MOM reflection");
    require(
        requestValue("direct time-regulating MOM value request") == enabled,
        "Direct HLAtimeRegulating value disagreed with the service transition");
  };

  bool const initialValue =
      requestValue("initial time-regulating MOM value request");
  transitionTo(
      !initialValue,
      "time-regulating state-change MOM reflection");
  transitionTo(
      initialValue,
      "restored time-regulating MOM reflection");

  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void runFederateLookaheadMomPeriodicScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate HLAlookahead MOM testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session monitor(options, model, "federate-lookahead-mom-periodic-monitor");
  Session subject(options, model, "federate-lookahead-mom-periodic-subject");
  auto const federation = federationName(
      options,
      "federate-lookahead-mim-periodic");
  monitor.connect();
  subject.connect();
  monitor.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  monitor.join(
      options.memberFederateName + L"-lookahead-mom-periodic-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const lookaheadAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAlookahead");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          lookaheadAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, lookaheadAttribute) ==
              L"HLAlookahead" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federate HLAlookahead MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, lookaheadAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-lookahead-mom-periodic-subject",
      options.federateType,
      federation);
  auto const subjectTime = makeTimeContext(subject);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Federate HLAlookahead subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Federate HLAlookahead discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(lookaheadAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto matchesLookahead = [&](ReflectionRecord const& reflection,
                              rti::LogicalTimeInterval const& expected) {
    auto const value = reflection.values.find(lookaheadAttribute);
    if (!reflection.present || reflection.object != *subjectMomObject ||
        value == reflection.values.end()) {
      return false;
    }
    auto const decodedInterval =
        subjectTime.factory->decodeLogicalTimeInterval(value->second);
    return decodedInterval != nullptr && *decodedInterval == expected;
  };
  auto validateLookaheadReflection = [&](ReflectionRecord const& reflection,
                                         rti::LogicalTimeInterval const& expected,
                                         char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard HLAlookahead reflection metadata");
    require(
        matchesLookahead(reflection, expected),
        std::string(description) +
            " did not contain the encoded standard logical-time interval");
  };
  auto waitForLookaheadReflection = [&](std::size_t firstReflection,
                                       rti::LogicalTimeInterval const& expected,
                                       char const* description) {
    waitFor(
        monitor,
        [&] {
          auto const reflections = valueReflectionsSince(firstReflection);
          return std::any_of(
              reflections.begin(),
              reflections.end(),
              [&](ReflectionRecord const& reflection) {
                return matchesLookahead(reflection, expected);
              });
        },
        options,
        description);
    auto const reflections = valueReflectionsSince(firstReflection);
    auto const matched = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return matchesLookahead(reflection, expected);
        });
    require(
        matched != reflections.end(),
        std::string(description) + " was not recorded after callback servicing");
    validateLookaheadReflection(*matched, expected, description);
  };

  auto const enableCallbacksBefore =
      subject.recorder().timeRegulationEnabled().size();
  subject.rtiAmbassador().enableTimeRegulation(*subjectTime.epsilon);
  waitFor(
      subject,
      [&] {
        return subject.recorder().timeRegulationEnabled().size() >=
            enableCallbacksBefore + 1U;
      },
      options,
      "Federate HLAlookahead timeRegulationEnabled callback");
  require(
      subject.recorder().timeRegulationEnabled().size() ==
          enableCallbacksBefore + 1U,
      "Enable Time Regulation delivered an unexpected number of callbacks");

  auto initialLookahead = subjectTime.factory->makeZero();
  require(
      initialLookahead != nullptr,
      "Federate HLAlookahead could not allocate an initial Query Lookahead interval");
  subject.rtiAmbassador().queryLookahead(*initialLookahead);
  require(
      *initialLookahead == *subjectTime.epsilon,
      "initial Query Lookahead did not match the enabled epsilon");

  auto const setTiming = monitor.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          monitor.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          monitor.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          monitor.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Federate HLAlookahead HLAsetTiming lookup did not round-trip standard MIM handles");
  auto setReportPeriod = [&](std::int32_t seconds) {
    monitor.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, subjectFederate.encode()},
            {reportPeriod, rti::HLAinteger32BE{seconds}.encode()}},
        rti::VariableLengthData{});
  };

  auto const beforeInitialReport = monitor.recorder().reflections().size();
  setReportPeriod(1);
  waitForLookaheadReflection(
      beforeInitialReport,
      *subjectTime.epsilon,
      "periodic HLAlookahead reflection after HLAsetTiming");

  auto const beforeChangedReport = monitor.recorder().reflections().size();
  auto changedLookaheadTime = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      3U);
  auto changedLookahead = subjectTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "Federate HLAlookahead could not allocate the changed interval");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *subjectTime.initial);
  subject.rtiAmbassador().modifyLookahead(*changedLookahead);

  auto queriedChanged = subjectTime.factory->makeZero();
  require(
      queriedChanged != nullptr,
      "Federate HLAlookahead could not allocate the changed Query Lookahead interval");
  subject.rtiAmbassador().queryLookahead(*queriedChanged);
  require(
      *queriedChanged == *changedLookahead,
      "Modify Lookahead did not cross the Query Lookahead boundary");
  waitForLookaheadReflection(
      beforeChangedReport,
      *changedLookahead,
      "periodic HLAlookahead reflection after Modify Lookahead");

  setReportPeriod(0);
  subject.rtiAmbassador().disableTimeRegulation();
  monitor.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, lookaheadAttribute});
  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void scenarioFederateExceptionReportingMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-exception-reporting-mim-attribute",
      L"HLAexceptionReporting",
      "HLAexceptionReporting",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getExceptionReportingSwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setExceptionReportingSwitch(value);
      });
}

void scenarioFederateServiceReportingMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-service-reporting-mim-attribute",
      L"HLAserviceReporting",
      "HLAserviceReporting",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getServiceReportingSwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setServiceReportingSwitch(value);
      });
}

void scenarioFederateServiceReportingMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateServiceReportingMomAttribute(options, model);
}

void scenarioFederateObjectClassRelevanceAdvisoryMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-object-class-relevance-advisory-mim-attribute",
      L"HLAobjectClassRelevanceAdvisory",
      "HLAobjectClassRelevanceAdvisory",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getObjectClassRelevanceAdvisorySwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setObjectClassRelevanceAdvisorySwitch(value);
      });
}

void scenarioFederateObjectClassRelevanceAdvisoryMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateObjectClassRelevanceAdvisoryMomAttribute(options, model);
}

void scenarioFederateAttributeRelevanceAdvisoryMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-attribute-relevance-advisory-mim-attribute",
      L"HLAattributeRelevanceAdvisory",
      "HLAattributeRelevanceAdvisory",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getAttributeRelevanceAdvisorySwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setAttributeRelevanceAdvisorySwitch(value);
      });
}

void scenarioFederateAttributeRelevanceAdvisoryMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateAttributeRelevanceAdvisoryMomAttribute(options, model);
}

void scenarioFederateAttributeScopeAdvisoryMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-attribute-scope-advisory-mim-attribute",
      L"HLAattributeScopeAdvisory",
      "HLAattributeScopeAdvisory",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getAttributeScopeAdvisorySwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setAttributeScopeAdvisorySwitch(value);
      });
}

void scenarioFederateAttributeScopeAdvisoryMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateAttributeScopeAdvisoryMomAttribute(options, model);
}

void scenarioFederateInteractionRelevanceAdvisoryMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runBooleanFederateMomAttributeScenario(
      options,
      model,
      "federate-interaction-relevance-advisory-mim-attribute",
      L"HLAinteractionRelevanceAdvisory",
      "HLAinteractionRelevanceAdvisory",
      [](rti::RTIambassador& ambassador) {
        return ambassador.getInteractionRelevanceAdvisorySwitch();
      },
      [](rti::RTIambassador& ambassador, bool const value) {
        ambassador.setInteractionRelevanceAdvisorySwitch(value);
      });
}

void scenarioFederateInteractionRelevanceAdvisoryMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateInteractionRelevanceAdvisoryMomAttribute(options, model);
}

void scenarioFederateAsynchronousDeliveryMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runAsynchronousDeliveryMomAttributeScenario(options, model);
}

void scenarioFederateAsynchronousDeliveryMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateAsynchronousDeliveryMomAttribute(options, model);
}

void scenarioFederateTimeConstrainedMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runTimeConstrainedMomAttributeScenario(options, model);
}

void scenarioFederateTimeConstrainedMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateTimeConstrainedMomAttribute(options, model);
}

void scenarioFederateTimeRegulatingMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runTimeRegulatingMomAttributeScenario(options, model);
}

void scenarioFederateTimeRegulatingMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateTimeRegulatingMomAttribute(options, model);
}

void runFederateLogicalTimeMomPeriodicScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate HLAlogicalTime MOM testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session monitor(options, model, "federate-logical-time-mom-periodic-monitor");
  Session subject(options, model, "federate-logical-time-mom-periodic-subject");
  auto const federation = federationName(
      options,
      "federate-logical-time-mim-periodic");
  monitor.connect();
  subject.connect();
  monitor.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  monitor.join(
      options.memberFederateName + L"-logical-time-mom-periodic-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const logicalTimeAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAlogicalTime");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          logicalTimeAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(momClass, logicalTimeAttribute) ==
              L"HLAlogicalTime" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federate HLAlogicalTime MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, logicalTimeAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-logical-time-mom-periodic-subject",
      options.federateType,
      federation);
  auto const monitorTime = makeTimeContext(monitor);
  auto const subjectTime = makeTimeContext(subject);
  require(
      monitorTime.factory->getName() == subjectTime.factory->getName() &&
          *monitorTime.initial == *subjectTime.initial,
      "Federate HLAlogicalTime participants selected incompatible standard logical-time configurations");
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Federate HLAlogicalTime subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Federate HLAlogicalTime discovery returned the wrong standard MOM object");

  auto valueReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(logicalTimeAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto matchesLogicalTime = [&](ReflectionRecord const& reflection,
                                rti::LogicalTime const& expected) {
    auto const value = reflection.values.find(logicalTimeAttribute);
    if (!reflection.present || reflection.object != *subjectMomObject ||
        value == reflection.values.end()) {
      return false;
    }
    auto const decodedTime = subjectTime.factory->decodeLogicalTime(value->second);
    return decodedTime != nullptr && *decodedTime == expected;
  };
  auto validateLogicalTimeReflection = [&](ReflectionRecord const& reflection,
                                           rti::LogicalTime const& expected,
                                           char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard HLAlogicalTime reflection metadata");
    require(
        matchesLogicalTime(reflection, expected),
        std::string(description) +
            " did not contain the encoded standard logical time");
  };
  auto waitForLogicalTimeReflection = [&](std::size_t firstReflection,
                                         rti::LogicalTime const& expected,
                                         char const* description) {
    waitFor(
        monitor,
        [&] {
          auto const reflections = valueReflectionsSince(firstReflection);
          return std::any_of(
              reflections.begin(),
              reflections.end(),
              [&](ReflectionRecord const& reflection) {
                return matchesLogicalTime(reflection, expected);
              });
        },
        options,
        description);
    auto const reflections = valueReflectionsSince(firstReflection);
    auto const matched = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return matchesLogicalTime(reflection, expected);
        });
    require(
        matched != reflections.end(),
        std::string(description) + " was not recorded after callback servicing");
    validateLogicalTimeReflection(*matched, expected, description);
  };

  auto const constrainedCallbacksBefore =
      subject.recorder().timeConstrainedEnabled().size();
  subject.rtiAmbassador().enableTimeConstrained();
  waitFor(
      subject,
      [&] {
        return subject.recorder().timeConstrainedEnabled().size() >=
            constrainedCallbacksBefore + 1U;
      },
      options,
      "Federate HLAlogicalTime timeConstrainedEnabled callback");
  require(
      subject.recorder().timeConstrainedEnabled().size() ==
          constrainedCallbacksBefore + 1U,
      "Enable Time Constrained delivered an unexpected number of callbacks");

  auto const regulationCallbacksBefore =
      monitor.recorder().timeRegulationEnabled().size();
  monitor.rtiAmbassador().enableTimeRegulation(*monitorTime.epsilon);
  waitFor(
      monitor,
      [&] {
        return monitor.recorder().timeRegulationEnabled().size() >=
            regulationCallbacksBefore + 1U;
      },
      options,
      "Federate HLAlogicalTime monitor timeRegulationEnabled callback");
  require(
      monitor.recorder().timeRegulationEnabled().size() ==
          regulationCallbacksBefore + 1U,
      "Enable Time Regulation delivered an unexpected number of monitor callbacks");

  auto queryLogicalTime = [&] {
    auto result = subjectTime.factory->makeInitial();
    require(
        result != nullptr,
        "Federate HLAlogicalTime could not allocate a Query Logical Time result");
    subject.rtiAmbassador().queryLogicalTime(*result);
    return result;
  };
  auto const initialLogicalTime = queryLogicalTime();
  require(
      *initialLogicalTime == *subjectTime.initial,
      "initial Query Logical Time did not match the federation initial time");

  auto const setTiming = monitor.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = monitor.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          monitor.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          monitor.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          monitor.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Federate HLAlogicalTime HLAsetTiming lookup did not round-trip standard MIM handles");
  auto setReportPeriod = [&](std::int32_t seconds) {
    monitor.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, subjectFederate.encode()},
            {reportPeriod, rti::HLAinteger32BE{seconds}.encode()}},
        rti::VariableLengthData{});
  };

  auto const beforeInitialReport = monitor.recorder().reflections().size();
  setReportPeriod(1);
  waitForLogicalTimeReflection(
      beforeInitialReport,
      *initialLogicalTime,
      "periodic HLAlogicalTime reflection after HLAsetTiming");

  auto const firstMonitorTarget = timeAfter(
      *monitorTime.factory,
      *monitorTime.initial,
      *monitorTime.epsilon,
      2U);
  auto const firstTarget = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      2U);
  require(
      *firstMonitorTarget == *firstTarget,
      "first HLAlogicalTime regulator and subject targets did not agree");
  auto const beforeFirstGrantReport = monitor.recorder().reflections().size();
  auto const firstMonitorGrantCount =
      monitor.recorder().timeAdvanceGrants().size();
  auto const firstGrantCount = subject.recorder().timeAdvanceGrants().size();
  monitor.rtiAmbassador().timeAdvanceRequest(*firstMonitorTarget);
  subject.rtiAmbassador().timeAdvanceRequest(*firstTarget);
  waitForSessions(
      {&monitor, &subject},
      [&] {
        return monitor.recorder().timeAdvanceGrants().size() >=
                firstMonitorGrantCount + 1U &&
            subject.recorder().timeAdvanceGrants().size() >= firstGrantCount + 1U;
      },
      options,
      "first HLAlogicalTime regulator and subject Time Advance Grants");
  auto const firstMonitorGrants = monitor.recorder().timeAdvanceGrants();
  auto const firstGrants = subject.recorder().timeAdvanceGrants();
  require(
      firstMonitorGrants.size() == firstMonitorGrantCount + 1U &&
          firstMonitorGrants.back().encoded == encodeTime(*firstMonitorTarget) &&
          firstGrants.size() == firstGrantCount + 1U &&
          firstGrants.back().encoded == encodeTime(*firstTarget),
      "first HLAlogicalTime regulator or subject grant returned the wrong time");
  auto const firstQueriedTime = queryLogicalTime();
  require(
      *firstQueriedTime == *firstTarget,
      "Query Logical Time did not match the first successful grant");
  waitForLogicalTimeReflection(
      beforeFirstGrantReport,
      *firstQueriedTime,
      "periodic HLAlogicalTime reflection after the first grant");

  auto const secondMonitorTarget = timeAfter(
      *monitorTime.factory,
      *monitorTime.initial,
      *monitorTime.epsilon,
      4U);
  auto const secondTarget = timeAfter(
      *subjectTime.factory,
      *subjectTime.initial,
      *subjectTime.epsilon,
      4U);
  require(
      *secondMonitorTarget == *secondTarget && *secondTarget > *firstTarget,
      "second HLAlogicalTime regulator and subject targets did not agree or advance");
  auto const beforeSecondGrantReport = monitor.recorder().reflections().size();
  auto const secondMonitorGrantCount =
      monitor.recorder().timeAdvanceGrants().size();
  auto const secondGrantCount = subject.recorder().timeAdvanceGrants().size();
  monitor.rtiAmbassador().timeAdvanceRequest(*secondMonitorTarget);
  subject.rtiAmbassador().timeAdvanceRequest(*secondTarget);
  waitForSessions(
      {&monitor, &subject},
      [&] {
        return monitor.recorder().timeAdvanceGrants().size() >=
                secondMonitorGrantCount + 1U &&
            subject.recorder().timeAdvanceGrants().size() >= secondGrantCount + 1U;
      },
      options,
      "second HLAlogicalTime regulator and subject Time Advance Grants");
  auto const secondMonitorGrants = monitor.recorder().timeAdvanceGrants();
  auto const secondGrants = subject.recorder().timeAdvanceGrants();
  require(
      secondMonitorGrants.size() == secondMonitorGrantCount + 1U &&
          secondMonitorGrants.back().encoded == encodeTime(*secondMonitorTarget) &&
          secondGrants.size() == secondGrantCount + 1U &&
          secondGrants.back().encoded == encodeTime(*secondTarget),
      "second HLAlogicalTime regulator or subject grant returned the wrong time");
  auto const secondQueriedTime = queryLogicalTime();
  require(
      *secondQueriedTime == *secondTarget,
      "Query Logical Time did not match the second successful grant");
  waitForLogicalTimeReflection(
      beforeSecondGrantReport,
      *secondQueriedTime,
      "periodic HLAlogicalTime reflection after the second grant");

  setReportPeriod(0);
  subject.rtiAmbassador().disableTimeConstrained();
  monitor.rtiAmbassador().disableTimeRegulation();
  monitor.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, logicalTimeAttribute});
  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void scenarioFederateLookaheadMomPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  runFederateLookaheadMomPeriodicScenario(options, model);
}

void scenarioFederateLookaheadMomPeriodicContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLookaheadMomPeriodic(options, model);
}

void scenarioFederateLogicalTimeMomPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  runFederateLogicalTimeMomPeriodicScenario(options, model);
}

void scenarioFederateLogicalTimeMomPeriodicContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateLogicalTimeMomPeriodic(options, model);
}

void runFederateTimeManagerStateMomAttributeScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate HLAtimeManagerState MOM testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session monitor(options, model, "federate-time-manager-state-mom-monitor");
  Session subject(options, model, "federate-time-manager-state-mom-subject");
  auto const federation = federationName(
      options,
      "federate-time-manager-state-mom");
  monitor.connect();
  subject.connect();
  monitor.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  monitor.join(
      options.memberFederateName + L"-time-manager-state-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const timeManagerStateAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAtimeManagerState");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          timeManagerStateAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              timeManagerStateAttribute) == L"HLAtimeManagerState" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federate HLAtimeManagerState MOM lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          timeManagerStateAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-time-manager-state-mom-subject",
      options.federateType,
      federation);
  auto const subjectTime = makeTimeContext(subject);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "Federate HLAtimeManagerState subject MOM discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Federate HLAtimeManagerState discovery returned the wrong standard MOM object");

  auto stateReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(timeManagerStateAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto decodeTimeManagerState = [&](ReflectionRecord const& reflection,
                                    char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard HLAtimeManagerState reflection metadata");
    rti::HLAinteger32BE state;
    state.decode(reflection.values.at(timeManagerStateAttribute));
    return state.get();
  };
  auto applyTimeAdvanceAndCheck = [&](auto request,
                                     rti::LogicalTime const& target,
                                     bool flushQueueGrantExpected,
                                     char const* description) {
    auto const beforeReflection = monitor.recorder().reflections().size();
    auto const beforeTimeAdvanceGrant =
        subject.recorder().timeAdvanceGrants().size();
    auto const beforeFlushQueueGrant =
        subject.recorder().flushQueueGrants().size();
    request(target);
    waitForSessions(
        {&subject, &monitor},
        [&] {
          auto const grantReceived = flushQueueGrantExpected
              ? subject.recorder().flushQueueGrants().size() >=
                  beforeFlushQueueGrant + 1U
              : subject.recorder().timeAdvanceGrants().size() >=
                  beforeTimeAdvanceGrant + 1U;
          return grantReceived &&
              stateReflectionsSince(beforeReflection).size() >= 2U;
        },
        options,
        description);

    if (flushQueueGrantExpected) {
      auto const flushGrants = subject.recorder().flushQueueGrants();
      require(
          flushGrants.size() == beforeFlushQueueGrant + 1U &&
              subject.recorder().timeAdvanceGrants().size() ==
                  beforeTimeAdvanceGrant &&
              !flushGrants.back().time.empty() &&
              !flushGrants.back().optimisticTime.empty(),
          std::string(description) +
              " did not return exactly one standard Flush Queue Grant");
    } else {
      auto const grants = subject.recorder().timeAdvanceGrants();
      require(
          grants.size() == beforeTimeAdvanceGrant + 1U &&
              grants.back().encoded == encodeTime(target),
          std::string(description) + " returned the wrong Time Advance Grant");
    }

    bool sawTimeAdvancing = false;
    bool sawTimeGranted = false;
    for (auto const& reflection : stateReflectionsSince(beforeReflection)) {
      auto const state = decodeTimeManagerState(reflection, description);
      require(
          state == 0 || state == 1,
          std::string(description) +
              " returned a value outside the standard HLAtimeState enumeration");
      sawTimeAdvancing = sawTimeAdvancing || state == 1;
      sawTimeGranted = sawTimeGranted || state == 0;
    }
    require(
        sawTimeAdvancing && sawTimeGranted,
        std::string(description) +
            " did not report both TimeAdvancing and TimeGranted");
  };

  for (std::size_t step = 1U; step <= 5U; ++step) {
    auto const target = timeAfter(
        *subjectTime.factory,
        *subjectTime.initial,
        *subjectTime.epsilon,
        step);
    switch (step) {
      case 1U:
        applyTimeAdvanceAndCheck(
            [&](rti::LogicalTime const& time) {
              subject.rtiAmbassador().timeAdvanceRequest(time);
            },
            *target,
            false,
            "HLAtimeManagerState after Time Advance Request and grant");
        break;
      case 2U:
        applyTimeAdvanceAndCheck(
            [&](rti::LogicalTime const& time) {
              subject.rtiAmbassador().timeAdvanceRequestAvailable(time);
            },
            *target,
            false,
            "HLAtimeManagerState after Time Advance Request Available and grant");
        break;
      case 3U:
        applyTimeAdvanceAndCheck(
            [&](rti::LogicalTime const& time) {
              subject.rtiAmbassador().nextMessageRequest(time);
            },
            *target,
            false,
            "HLAtimeManagerState after Next Message Request and grant");
        break;
      case 4U:
        applyTimeAdvanceAndCheck(
            [&](rti::LogicalTime const& time) {
              subject.rtiAmbassador().nextMessageRequestAvailable(time);
            },
            *target,
            false,
            "HLAtimeManagerState after Next Message Request Available and grant");
        break;
      case 5U:
        applyTimeAdvanceAndCheck(
            [&](rti::LogicalTime const& time) {
              subject.rtiAmbassador().flushQueueRequest(time);
            },
            *target,
            true,
            "HLAtimeManagerState after Flush Queue Request and grant");
        break;
      default:
        throw std::logic_error("Unexpected HLAtimeManagerState request selector");
    }
  }

  monitor.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          timeManagerStateAttribute});
  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  monitor.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void runFederationMomStaticIdentityScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM static-identity testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session creator(options, model, "federation-mom-static-identity-creator");
  Session observer(options, model, "federation-mom-static-identity-observer");
  auto const federation = federationName(options, "federation-mom-static-identity");
  creator.connect();
  observer.connect();
  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(
      options.ownerFederateName + L"-static-identity-creator",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-static-identity-observer",
      options.federateType,
      federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const federationNameAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAfederationName");
  auto const rtiVersionAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLARTIversion");
  auto const mimDesignatorAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAMIMdesignator");
  auto const timeImplementationAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAtimeImplementationName");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && federationNameAttribute.isValid() &&
          rtiVersionAttribute.isValid() && mimDesignatorAttribute.isValid() &&
          timeImplementationAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              federationNameAttribute) == L"HLAfederationName" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              rtiVersionAttribute) == L"HLARTIversion" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              mimDesignatorAttribute) == L"HLAMIMdesignator" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              timeImplementationAttribute) == L"HLAtimeImplementationName" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federation MOM static-identity lookup did not round-trip standard MIM handles");

  rti::AttributeHandleSet const identityAttributes{
      federationNameAttribute,
      rtiVersionAttribute,
      mimDesignatorAttribute,
      timeImplementationAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      identityAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the standard federation MOM object discovery");

  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM static-identity discovery did not preserve standard object identity");

  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      federationObject,
      identityAttributes,
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        return reflection.present && reflection.object == federationObject &&
            reflection.values.count(federationNameAttribute) == 1U &&
            reflection.values.count(rtiVersionAttribute) == 1U &&
            reflection.values.count(mimDesignatorAttribute) == 1U &&
            reflection.values.count(timeImplementationAttribute) == 1U;
      },
      options,
      "the requested standard federation identity reflection");

  auto const reflection = observer.recorder().reflection();
  require(
      reflection.transportation == reliable && reflection.tag.empty() &&
          !reflection.producer.isValid() && !reflection.regions.has_value(),
      "Federation MOM static-identity reflection returned non-standard callback metadata");
  rti::HLAunicodeString federationNameValue;
  federationNameValue.decode(reflection.values.at(federationNameAttribute));
  rti::HLAunicodeString rtiVersionValue;
  rtiVersionValue.decode(reflection.values.at(rtiVersionAttribute));
  rti::HLAunicodeString mimDesignator;
  mimDesignator.decode(reflection.values.at(mimDesignatorAttribute));
  rti::HLAunicodeString timeImplementationName;
  timeImplementationName.decode(reflection.values.at(timeImplementationAttribute));
  require(
      federationNameValue.get() == federation,
      "Federation MOM federation name did not match the created federation execution");
  require(
      !rtiVersionValue.get().empty(),
      "Federation MOM RTI version was empty despite being a required static value");
  require(
      mimDesignator.get() == L"HLAstandardMIM",
      "Federation MOM reported a MIM designator other than HLAstandardMIM for the supplied standard MIM");
  require(
      timeImplementationName.get() == options.logicalTimeImplementationName,
      "Federation MOM time-implementation name did not match the Create Federation Execution argument");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      identityAttributes);
  observer.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  creator.disconnect();
}

void runFederationMomStaticSwitchesScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM static-switch testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session creator(options, model, "federation-mom-static-switches-creator");
  Session observer(options, model, "federation-mom-static-switches-observer");
  auto const federation = federationName(options, "federation-mom-static-switches");
  creator.connect();
  observer.connect();
  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  creator.join(
      options.ownerFederateName + L"-static-switches-creator",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-static-switches-observer",
      options.federateType,
      federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const advisoriesUseKnownClassAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federationClass,
          L"HLAadvisoriesUseKnownClass");
  auto const delaySubscriptionEvaluationAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federationClass,
          L"HLAdelaySubscriptionEvaluation");
  auto const nonRegulatedGrantAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federationClass,
          L"HLAnonRegulatedGrant");
  auto const allowRelaxedDdmAttribute = observer.rtiAmbassador().getAttributeHandle(
      federationClass,
      L"HLAallowRelaxedDDM");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && advisoriesUseKnownClassAttribute.isValid() &&
          delaySubscriptionEvaluationAttribute.isValid() &&
          nonRegulatedGrantAttribute.isValid() && allowRelaxedDdmAttribute.isValid() &&
          reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              advisoriesUseKnownClassAttribute) == L"HLAadvisoriesUseKnownClass" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              delaySubscriptionEvaluationAttribute) ==
              L"HLAdelaySubscriptionEvaluation" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              nonRegulatedGrantAttribute) == L"HLAnonRegulatedGrant" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              allowRelaxedDdmAttribute) == L"HLAallowRelaxedDDM" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federation MOM static-switch lookup did not round-trip standard MIM handles");

  rti::AttributeHandleSet const switchAttributes{
      advisoriesUseKnownClassAttribute,
      delaySubscriptionEvaluationAttribute,
      nonRegulatedGrantAttribute,
      allowRelaxedDdmAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      switchAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the standard federation MOM object discovery for static switches");

  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM static-switch discovery did not preserve standard object identity");

  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      federationObject,
      switchAttributes,
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflection = observer.recorder().reflection();
        return reflection.present && reflection.object == federationObject &&
            reflection.values.count(advisoriesUseKnownClassAttribute) == 1U &&
            reflection.values.count(delaySubscriptionEvaluationAttribute) == 1U &&
            reflection.values.count(nonRegulatedGrantAttribute) == 1U &&
            reflection.values.count(allowRelaxedDdmAttribute) == 1U;
      },
      options,
      "the requested standard federation static-switch reflection");

  auto const reflection = observer.recorder().reflection();
  require(
      reflection.transportation == reliable && reflection.tag.empty() &&
          !reflection.producer.isValid() && !reflection.regions.has_value(),
      "Federation MOM static-switch reflection returned non-standard callback metadata");
  auto checkSwitch = [&](rti::AttributeHandle const& attribute,
                         std::string const& name) {
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(attribute));
    require(
        value.get() == 0 || value.get() == 1,
        "Federation MOM " + name +
            " was not a standard HLAswitch enumerator (Disabled=0, Enabled=1)");
  };
  checkSwitch(advisoriesUseKnownClassAttribute, "HLAadvisoriesUseKnownClass");
  checkSwitch(delaySubscriptionEvaluationAttribute, "HLAdelaySubscriptionEvaluation");
  checkSwitch(nonRegulatedGrantAttribute, "HLAnonRegulatedGrant");
  checkSwitch(allowRelaxedDdmAttribute, "HLAallowRelaxedDDM");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      switchAttributes);
  observer.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  creator.disconnect();
}

void runFederationMomFederatesInFederationScenario(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM roster testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session creator(options, model, "federation-mom-roster-creator");
  Session observer(options, model, "federation-mom-roster-observer");
  Session entrant(options, model, "federation-mom-roster-entrant");
  auto const federation = federationName(options, "federation-mom-roster");
  creator.connect();
  observer.connect();
  entrant.connect();
  creator.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const creatorName = options.ownerFederateName + L"-roster-creator";
  auto const observerName = options.memberFederateName + L"-roster-observer";
  auto const entrantName = options.memberFederateName + L"-roster-entrant";
  creator.join(creatorName, options.federateType, federation);
  observer.join(observerName, options.federateType, federation);

  auto const federationClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederation");
  auto const federatesInFederationAttribute =
      observer.rtiAmbassador().getAttributeHandle(
          federationClass,
          L"HLAfederatesInFederation");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      federationClass.isValid() && federatesInFederationAttribute.isValid() &&
          reliable.isValid() &&
          observer.rtiAmbassador().getObjectClassName(federationClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederation" &&
          observer.rtiAmbassador().getAttributeName(
              federationClass,
              federatesInFederationAttribute) == L"HLAfederatesInFederation" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federation MOM roster lookup did not round-trip standard MIM handles");
  rti::AttributeHandleSet const rosterAttributes{
      federatesInFederationAttribute};
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      federationClass,
      rosterAttributes,
      true,
      L"");
  waitFor(
      observer,
      [&] {
        auto const discovery = observer.recorder().discovery();
        return discovery.present && discovery.objectClass == federationClass &&
            discovery.name == L"HLAfederation";
      },
      options,
      "the standard federation MOM object discovery for the roster");
  auto const federationObject = observer.recorder().discovery().object;
  require(
      observer.rtiAmbassador().getKnownObjectClassHandle(federationObject) ==
              federationClass &&
          observer.rtiAmbassador().getObjectInstanceName(federationObject) ==
              L"HLAfederation" &&
          observer.rtiAmbassador().getObjectInstanceHandle(L"HLAfederation") ==
              federationObject,
      "Federation MOM roster discovery did not preserve standard object identity");

  using EncodedFederateHandle = rti::HLAvariableArrayT<rti::HLAbyte>;
  auto decodeRoster = [&](rti::VariableLengthData const& encoded) {
    auto const rawBytes = copyBytes(encoded);
    std::vector<rti::Octet> bytes(rawBytes.begin(), rawBytes.end());
    rti::HLAinteger32BE referenceCount;
    auto cursor = referenceCount.decodeFrom(bytes, 0U);
    require(
        referenceCount.get() >= 0 &&
            static_cast<std::size_t>(referenceCount.get()) <= bytes.size() / 4U,
        "HLAfederatesInFederation has an invalid standard array count");

    EncodedFederateHandle referencePrototype;
    auto const elementBoundary = referencePrototype.getOctetBoundary();
    require(
        elementBoundary != 0U,
        "The standard federate-handle array reported an invalid octet boundary");
    auto const arrayBoundary = std::max(4U, elementBoundary);
    auto const leadingPadding =
        (arrayBoundary - (4U % arrayBoundary)) % arrayBoundary;
    auto skipAndCheckPadding = [&](std::size_t padding, std::string const& description) {
      require(
          cursor <= bytes.size() && padding <= bytes.size() - cursor,
          description + " is truncated");
      for (std::size_t index = 0; index < padding; ++index) {
        require(bytes[cursor + index] == 0U, description + " is nonzero");
      }
      cursor += padding;
    };
    skipAndCheckPadding(
        leadingPadding,
        "HLAfederatesInFederation leading array padding");

    std::vector<rti::FederateHandle> result;
    auto const count = static_cast<std::size_t>(referenceCount.get());
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
      auto const elementStart = cursor;
      EncodedFederateHandle reference;
      cursor = reference.decodeFrom(bytes, cursor);
      result.push_back(
          observer.rtiAmbassador().decodeFederateHandle(reference.encode()));
      if (index + 1U != count) {
        auto const encodedLength = cursor - elementStart;
        auto const padding =
            (elementBoundary - (encodedLength % elementBoundary)) % elementBoundary;
        skipAndCheckPadding(
            padding,
            "HLAfederatesInFederation reference-array element padding");
      }
    }
    require(
        cursor == bytes.size(),
        "HLAfederatesInFederation contains trailing bytes after its standard array");
    return result;
  };
  auto const hasExactRoster = [](
      std::vector<rti::FederateHandle> const& actual,
      std::vector<rti::FederateHandle> const& expected) {
    if (actual.size() != expected.size()) {
      return false;
    }
    return std::all_of(
        expected.begin(),
        expected.end(),
        [&](rti::FederateHandle const& handle) {
          return std::find(actual.begin(), actual.end(), handle) != actual.end();
        });
  };
  auto verifyRosterReflection = [&](std::vector<rti::FederateHandle> const& expected,
                                    std::string const& description) {
    waitFor(
        observer,
        [&] {
          auto const reflection = observer.recorder().reflection();
          if (!reflection.present || reflection.object != federationObject) {
            return false;
          }
          auto const value = reflection.values.find(federatesInFederationAttribute);
          return value != reflection.values.end() &&
              hasExactRoster(decodeRoster(value->second), expected);
        },
        options,
        description);
    auto const reflection = observer.recorder().reflection();
    require(
        reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM callback metadata");
    return decodeRoster(reflection.values.at(federatesInFederationAttribute));
  };

  observer.recorder().clearReflection();
  observer.rtiAmbassador().requestAttributeValueUpdate(
      federationObject,
      rosterAttributes,
      rti::VariableLengthData{});
  auto const initialRoster = verifyRosterReflection(
      {creator.federateHandle(), observer.federateHandle()},
      "the requested standard federation roster");
  require(
      initialRoster.size() == 2U,
      "The requested federation roster contained an unexpected number of joined federates");

  observer.recorder().clearReflection();
  entrant.join(entrantName, options.federateType, federation);
  auto const joinedRoster = verifyRosterReflection(
      {creator.federateHandle(), observer.federateHandle(), entrant.federateHandle()},
      "the conditional federation roster reflection after Join Federation Execution");
  require(
      joinedRoster.size() == 3U,
      "The join-triggered federation roster reflection had the wrong membership count");

  observer.recorder().clearReflection();
  entrant.resign(rti::NO_ACTION);
  auto const resignedRoster = verifyRosterReflection(
      {creator.federateHandle(), observer.federateHandle()},
      "the conditional federation roster reflection after Resign Federation Execution");
  require(
      resignedRoster.size() == 2U,
      "The resign-triggered federation roster reflection retained the resigned federate");

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      federationClass,
      rosterAttributes);
  observer.resign(rti::NO_ACTION);
  creator.resign(rti::NO_ACTION);
  creator.rtiAmbassador().destroyFederationExecution(federation);
  entrant.disconnect();
  observer.disconnect();
  creator.disconnect();
}

void scenarioFederationMomStaticIdentity(
    Options const& options,
    rti::CallbackModel model) {
  runFederationMomStaticIdentityScenario(options, model);
}

void scenarioFederationMomStaticIdentityContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomStaticIdentity(options, model);
}

void scenarioFederationMomStaticSwitches(
    Options const& options,
    rti::CallbackModel model) {
  runFederationMomStaticSwitchesScenario(options, model);
}

void scenarioFederationMomStaticSwitchesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomStaticSwitches(options, model);
}

void scenarioFederationMomFederatesInFederation(
    Options const& options,
    rti::CallbackModel model) {
  runFederationMomFederatesInFederationScenario(options, model);
}

void scenarioFederationMomFederatesInFederationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFederatesInFederation(options, model);
}

void scenarioFederationMomFomModuleDesignatorListPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFomModuleDesignatorList(options, model);
}

void scenarioFederationMomFomModuleDesignatorListContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomFomModuleDesignatorListContract(options, model);
}

void scenarioFederateTimeManagerStateMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  runFederateTimeManagerStateMomAttributeScenario(options, model);
}

void scenarioFederateTimeManagerStateMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateTimeManagerStateMomAttribute(options, model);
}

void scenarioFederateExceptionReportingMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateExceptionReportingMomAttribute(options, model);
}

void scenarioFederateExceptionReportDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate Exception Reporting delivery testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-exception-report-delivery");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const federateName =
      options.ownerFederateName + L"-exception-report-delivery";
  federate.join(federateName, options.federateType, federation);

  auto& ambassador = federate.rtiAmbassador();
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportException");
  auto const serviceParameter =
      ambassador.getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter =
      ambassador.getParameterHandle(reportClass, L"HLAexception");
  auto const federateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  auto const joinedFederate = ambassador.getFederateHandle(federateName);
  require(
      reportClass.isValid() && serviceParameter.isValid() &&
          exceptionParameter.isValid() && federateParameter.isValid() &&
          reliable.isValid() && joinedFederate.isValid(),
      "Standard MIM HLAreportException lookups returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass, true);

  auto drainCallbacks = [&] {
    while (ambassador.evokeCallback(0.0)) {
    }
  };
  auto failObjectClassLookup = [&] {
    std::wstring exceptionDescription;
    try {
      static_cast<void>(
          ambassador.getObjectClassHandle(L"HLAobjectRoot.UndefinedReportProbe"));
    } catch (rti::NameNotFound const& exception) {
      exceptionDescription = exception.name();
      if (!exception.what().empty()) {
        exceptionDescription += L": " + exception.what();
      }
    }
    require(
        !exceptionDescription.empty(),
        "An undefined object-class lookup did not preserve its standard NameNotFound exception");
    return exceptionDescription;
  };
  auto verifyReport = [&](InteractionRecord const& report) {
    require(
        report.present && report.interaction == reportClass &&
            report.parameters.size() == 3U &&
            report.transportation == reliable && !report.producer.isValid(),
        "HLAreportException callback metadata or parameter set was not standard");
    rti::HLAunicodeString serviceName;
    rti::HLAunicodeString exceptionText;
    require(
        report.parameters.find(serviceParameter) != report.parameters.end() &&
            report.parameters.find(exceptionParameter) != report.parameters.end() &&
            report.parameters.find(federateParameter) != report.parameters.end(),
        "HLAreportException omitted a required standard parameter");
    serviceName.decode(report.parameters.at(serviceParameter));
    exceptionText.decode(report.parameters.at(exceptionParameter));
    require(
        !serviceName.get().empty() && !exceptionText.get().empty(),
        "HLAreportException service or exception text was empty");
    auto const expectedFederateEncoding = copyBytes(joinedFederate.encode());
    require(
        copyBytes(report.parameters.at(federateParameter)) ==
            expectedFederateEncoding,
        "HLAreportException identified a different joined federate");
  };

  require(
      !ambassador.getExceptionReportingSwitch() &&
          !ambassador.getServiceReportingSwitch(),
      "Standard MIM Exception Reporting or Service Reporting default switch value was not false");
  static_cast<void>(failObjectClassLookup());
  drainCallbacks();
  require(
      federate.recorder().interactions().empty(),
      "HLAreportException was delivered while Exception Reporting was disabled");

  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const exceptionReportingParameter =
      ambassador.getParameterHandle(setSwitches, L"HLAexceptionReporting");
  require(
      setSwitches.isValid() && exceptionReportingParameter.isValid(),
      "Standard federate HLAsetSwitches Exception Reporting lookup returned an invalid handle");
  auto setExceptionReportingThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {exceptionReportingParameter,
         rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    ambassador.sendInteraction(setSwitches, parameters, rti::VariableLengthData{});
    require(
        ambassador.getExceptionReportingSwitch() == enabled,
        "HLAsetSwitches did not update the Exception Reporting switch");
  };

  setExceptionReportingThroughMom(true);
  static_cast<void>(failObjectClassLookup());
  drainCallbacks();
  auto reports = federate.recorder().interactions();
  require(reports.size() == 1U, "Enabled Exception Reporting did not deliver exactly one HLAreportException");
  verifyReport(reports.front());

  setExceptionReportingThroughMom(false);
  static_cast<void>(failObjectClassLookup());
  drainCallbacks();
  reports = federate.recorder().interactions();
  require(
      reports.size() == 1U,
      "HLAreportException was delivered after Exception Reporting was disabled");
  verifyReport(reports.front());

  ambassador.unsubscribeInteractionClass(reportClass);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateExceptionReportDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateExceptionReportDelivery(options, model);
}

void scenarioFederateMomExceptionReportDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM exception-report testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-mom-exception-report-delivery");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const federateName =
      options.ownerFederateName + L"-mom-exception-report-delivery";
  federate.join(federateName, options.federateType, federation);

  auto& ambassador = federate.rtiAmbassador();
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const serviceParameter =
      ambassador.getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter =
      ambassador.getParameterHandle(reportClass, L"HLAexception");
  auto const parameterErrorParameter =
      ambassador.getParameterHandle(reportClass, L"HLAparameterError");
  auto const federateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  auto const joinedFederate = ambassador.getFederateHandle(federateName);
  require(
      reportClass.isValid() && serviceParameter.isValid() &&
          exceptionParameter.isValid() && parameterErrorParameter.isValid() &&
          federateParameter.isValid() && reliable.isValid() &&
          joinedFederate.isValid(),
      "Standard MIM HLAreportMOMexception lookups returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass, true);

  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const automaticResignActionParameter =
      ambassador.getParameterHandle(setSwitches, L"HLAautomaticResignAction");
  auto const serviceReportingParameter =
      ambassador.getParameterHandle(setSwitches, L"HLAserviceReporting");
  require(
      setSwitches.isValid() && automaticResignActionParameter.isValid() &&
          serviceReportingParameter.isValid(),
      "Standard federate HLAsetSwitches malformed-value parameter lookup returned an invalid handle");
  auto const initialResignAction = ambassador.getAutomaticResignDirective();
  auto const initialServiceReporting = ambassador.getServiceReportingSwitch();

  auto sendMalformedValue = [&](auto const& parameter, std::int32_t value) {
    rti::ParameterHandleValueMap const malformedParameters{
        {parameter, rti::HLAinteger32BE{value}.encode()}};
    try {
      ambassador.sendInteraction(
          setSwitches,
          malformedParameters,
          rti::VariableLengthData{});
    } catch (rti::Exception const&) {
      // A synchronous RTI exception is permitted; the standard MIM still
      // requires the subscribed HLAreportMOMexception notification.
    }
    while (ambassador.evokeCallback(0.0)) {
    }
  };

  // HLAresignAction enumerates 0 through 5, while HLAswitch enumerates 0 and
  // 1.  Each out-of-domain HLAinteger32BE value is malformed for its declared
  // MIM parameter type.
  sendMalformedValue(automaticResignActionParameter, 6);
  auto reports = federate.recorder().interactions();
  require(
      reports.size() == 1U,
      "Malformed HLAresignAction did not deliver one HLAreportMOMexception");
  sendMalformedValue(serviceReportingParameter, 2);
  reports = federate.recorder().interactions();
  require(
      reports.size() == 2U,
      "Malformed HLAserviceReporting did not deliver a second HLAreportMOMexception");

  for (auto const& report : reports) {
    require(
        report.present && report.interaction == reportClass &&
            report.parameters.size() == 4U && report.transportation == reliable,
        "HLAreportMOMexception callback metadata or parameter set was not standard");
    require(
        report.parameters.find(serviceParameter) != report.parameters.end() &&
            report.parameters.find(exceptionParameter) != report.parameters.end() &&
            report.parameters.find(parameterErrorParameter) != report.parameters.end() &&
            report.parameters.find(federateParameter) != report.parameters.end(),
        "HLAreportMOMexception omitted a required standard parameter");

    rti::HLAunicodeString serviceName;
    rti::HLAunicodeString exceptionText;
    rti::HLAboolean parameterError;
    serviceName.decode(report.parameters.at(serviceParameter));
    exceptionText.decode(report.parameters.at(exceptionParameter));
    parameterError.decode(report.parameters.at(parameterErrorParameter));
    require(
        serviceName.get() ==
            L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
        "HLAreportMOMexception did not identify the fully qualified standard MOM interaction");
    require(
        !exceptionText.get().empty(),
        "HLAreportMOMexception did not provide a textual problem description");
    require(
        parameterError.get(),
        "HLAreportMOMexception did not mark the incorrectly formatted parameter");
    require(
        copyBytes(report.parameters.at(federateParameter)) ==
            copyBytes(joinedFederate.encode()),
        "HLAreportMOMexception identified a different joined federate");
  }
  require(
      ambassador.getAutomaticResignDirective() == initialResignAction,
      "Malformed HLAautomaticResignAction changed the joined federate's directive");
  require(
      ambassador.getServiceReportingSwitch() == initialServiceReporting,
      "Malformed HLAserviceReporting changed the joined federate's switch");

  ambassador.unsubscribeInteractionClass(reportClass);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateMomExceptionReportDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomExceptionReportDelivery(options, model);
}

void scenarioFederateMomExceptionMissingParameter(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM missing-parameter reporting requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-mom-exception-missing-parameter");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const federateName =
      options.ownerFederateName + L"-mom-exception-missing-parameter";
  federate.join(federateName, options.federateType, federation);

  auto& ambassador = federate.rtiAmbassador();
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const serviceParameter =
      ambassador.getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter =
      ambassador.getParameterHandle(reportClass, L"HLAexception");
  auto const parameterErrorParameter =
      ambassador.getParameterHandle(reportClass, L"HLAparameterError");
  auto const federateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  auto const joinedFederate = ambassador.getFederateHandle(federateName);
  auto const requestClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus");
  auto const statusReportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportSynchronizationPointStatus");
  require(
      reportClass.isValid() && serviceParameter.isValid() &&
          exceptionParameter.isValid() && parameterErrorParameter.isValid() &&
          federateParameter.isValid() && reliable.isValid() &&
          joinedFederate.isValid() && requestClass.isValid() &&
          statusReportClass.isValid(),
      "Standard MIM synchronization-status exception-report lookups returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass, true);
  ambassador.subscribeInteractionClass(statusReportClass, true);

  // HLArequestSynchronizationPointStatus declares HLAsyncPointName. Sending
  // the interaction with no parameters exercises the standard MIM missing-
  // parameter report without depending on provider-specific exception text.
  rti::ParameterHandleValueMap const noParameters;
  try {
    ambassador.sendInteraction(
        requestClass,
        noParameters,
        rti::VariableLengthData{});
  } catch (rti::Exception const&) {
    // The required HLAreportMOMexception callback is the portable oracle.
  }
  while (ambassador.evokeCallback(0.0)) {
  }

  auto const reports = federate.recorder().interactions();
  require(
      reports.size() == 1U,
      "A request missing HLAsyncPointName did not deliver exactly one HLAreportMOMexception");
  auto const& report = reports.front();
  require(
      report.present && report.interaction == reportClass &&
          report.parameters.size() == 4U && report.transportation == reliable,
      "HLAreportMOMexception callback metadata or parameter set was not standard");
  require(
      report.parameters.find(serviceParameter) != report.parameters.end() &&
          report.parameters.find(exceptionParameter) != report.parameters.end() &&
          report.parameters.find(parameterErrorParameter) != report.parameters.end() &&
          report.parameters.find(federateParameter) != report.parameters.end(),
      "HLAreportMOMexception omitted a required standard parameter");

  rti::HLAunicodeString serviceName;
  rti::HLAunicodeString exceptionText;
  rti::HLAboolean parameterError;
  serviceName.decode(report.parameters.at(serviceParameter));
  exceptionText.decode(report.parameters.at(exceptionParameter));
  parameterError.decode(report.parameters.at(parameterErrorParameter));
  require(
      serviceName.get() ==
          L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestSynchronizationPointStatus",
      "HLAreportMOMexception did not identify the fully qualified standard MOM interaction");
  require(
      !exceptionText.get().empty(),
      "HLAreportMOMexception did not provide a textual problem description");
  require(
      parameterError.get(),
      "HLAreportMOMexception did not mark the missing interaction parameter");
  require(
      copyBytes(report.parameters.at(federateParameter)) ==
          copyBytes(joinedFederate.encode()),
      "HLAreportMOMexception identified a different joined federate");

  ambassador.unsubscribeInteractionClass(statusReportClass);
  ambassador.unsubscribeInteractionClass(reportClass);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateMomExceptionMissingParameterContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomExceptionMissingParameter(options, model);
}

void scenarioFederationMomExceptionMissingFomModuleIndicator(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federation MOM missing-module-indicator reporting requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federation-mom-exception-missing-fom-module-indicator");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const federateName =
      options.ownerFederateName + L"-mom-missing-fom-module-indicator";
  federate.join(federateName, options.federateType, federation);

  auto& ambassador = federate.rtiAmbassador();
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const serviceParameter =
      ambassador.getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter =
      ambassador.getParameterHandle(reportClass, L"HLAexception");
  auto const parameterErrorParameter =
      ambassador.getParameterHandle(reportClass, L"HLAparameterError");
  auto const federateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  auto const joinedFederate = ambassador.getFederateHandle(federateName);
  auto const requestClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData");
  auto const moduleIndicatorParameter =
      ambassador.getParameterHandle(requestClass, L"HLAFOMmoduleIndicator");
  auto const moduleReportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederation.HLAreport.HLAreportFOMmoduleData");
  require(
      reportClass.isValid() && serviceParameter.isValid() &&
          exceptionParameter.isValid() && parameterErrorParameter.isValid() &&
          federateParameter.isValid() && reliable.isValid() &&
          joinedFederate.isValid() && requestClass.isValid() &&
          moduleIndicatorParameter.isValid() && moduleReportClass.isValid(),
      "Standard MIM FOM-module exception-report lookups returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass, true);
  ambassador.subscribeInteractionClass(moduleReportClass, true);

  // The standard MIM identifies one FOM module by HLAFOMmoduleIndicator;
  // without that parameter, the RTI cannot form the requested data report.
  rti::ParameterHandleValueMap const noParameters;
  try {
    ambassador.sendInteraction(
        requestClass,
        noParameters,
        rti::VariableLengthData{});
  } catch (rti::Exception const&) {
    // The required HLAreportMOMexception callback is the portable oracle.
  }
  while (ambassador.evokeCallback(0.0)) {
  }

  auto const reports = federate.recorder().interactions();
  require(
      reports.size() == 1U,
      "A request missing HLAFOMmoduleIndicator did not deliver exactly one HLAreportMOMexception");
  auto const& report = reports.front();
  require(
      report.present && report.interaction == reportClass &&
          report.parameters.size() == 4U && report.transportation == reliable,
      "HLAreportMOMexception callback metadata or parameter set was not standard");
  require(
      report.parameters.find(serviceParameter) != report.parameters.end() &&
          report.parameters.find(exceptionParameter) != report.parameters.end() &&
          report.parameters.find(parameterErrorParameter) != report.parameters.end() &&
          report.parameters.find(federateParameter) != report.parameters.end(),
      "HLAreportMOMexception omitted a required standard parameter");

  rti::HLAunicodeString serviceName;
  rti::HLAunicodeString exceptionText;
  rti::HLAboolean parameterError;
  serviceName.decode(report.parameters.at(serviceParameter));
  exceptionText.decode(report.parameters.at(exceptionParameter));
  parameterError.decode(report.parameters.at(parameterErrorParameter));
  require(
      serviceName.get() ==
          L"HLAinteractionRoot.HLAmanager.HLAfederation.HLArequest.HLArequestFOMmoduleData",
      "HLAreportMOMexception did not identify the fully qualified standard MOM interaction");
  require(
      !exceptionText.get().empty(),
      "HLAreportMOMexception did not provide a textual problem description");
  require(
      parameterError.get(),
      "HLAreportMOMexception did not mark the missing FOM-module indicator");
  require(
      copyBytes(report.parameters.at(federateParameter)) ==
          copyBytes(joinedFederate.encode()),
      "HLAreportMOMexception identified a different joined federate");

  ambassador.unsubscribeInteractionClass(moduleReportClass);
  ambassador.unsubscribeInteractionClass(reportClass);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederationMomExceptionMissingFomModuleIndicatorContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationMomExceptionMissingFomModuleIndicator(options, model);
}

void scenarioFederateMomFomModuleContentReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.additionalFomModules.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM FOM-module reporting requires adapter-supplied FOM, standard MIM, an additional FOM module, and logical-time configuration");

  Session subject(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "federate-mom-fom-module-content-report");
  subject.connect();
  requester.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const subjectName =
      options.ownerFederateName + L"-federate-mom-fom-module-subject";
  auto const requesterName =
      options.memberFederateName + L"-federate-mom-fom-module-requester";
  subject.join(
      subjectName,
      options.federateType,
      federation,
      fomModuleNames(options.additionalFomModules));
  requester.join(requesterName, options.federateType, federation);

  auto& ambassador = requester.rtiAmbassador();
  auto const requestClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestFOMmoduleData");
  auto const requestFederateParameter =
      ambassador.getParameterHandle(requestClass, L"HLAfederate");
  auto const requestIndicatorParameter =
      ambassador.getParameterHandle(requestClass, L"HLAFOMmoduleIndicator");
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportFOMmoduleData");
  auto const reportFederateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reportIndicatorParameter =
      ambassador.getParameterHandle(reportClass, L"HLAFOMmoduleIndicator");
  auto const reportDataParameter =
      ambassador.getParameterHandle(reportClass, L"HLAFOMmoduleData");
  auto const targetFederate = ambassador.getFederateHandle(subjectName);
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      requestClass.isValid() && requestFederateParameter.isValid() &&
          requestIndicatorParameter.isValid() && reportClass.isValid() &&
          reportFederateParameter.isValid() &&
          reportIndicatorParameter.isValid() && reportDataParameter.isValid() &&
          targetFederate.isValid() && reliable.isValid(),
      "Standard MIM federate FOM-module request/report lookup returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass);

  rti::ParameterHandleValueMap requestParameters;
  requestParameters.emplace(requestFederateParameter, targetFederate.encode());
  requestParameters.emplace(
      requestIndicatorParameter,
      rti::HLAinteger32BE{0}.encode());
  ambassador.sendInteraction(
      requestClass,
      requestParameters,
      rti::VariableLengthData{});
  waitFor(
      requester,
      [&] { return !requester.recorder().interactions().empty(); },
      options,
      "the federate-scoped MOM FOM-module content report");

  auto const reports = requester.recorder().interactions();
  require(
      reports.size() == 1U,
      "A federate-scoped FOM-module request did not deliver exactly one report");
  auto const& report = reports.front();
  require(
      report.present,
      "Federate MOM FOM-module callback was not marked as present");
  require(
      report.interaction == reportClass,
      "Federate MOM FOM-module callback used a different interaction class");
  require(
      report.transportation == reliable,
      "Federate MOM FOM-module report did not use reliable transportation");
  require(
      report.parameters.count(reportIndicatorParameter) == 1U &&
          report.parameters.count(reportDataParameter) == 1U,
      "Federate MOM FOM-module report omitted one of its declared child parameters");
  auto const inheritedTargetParameterCount =
      report.parameters.count(reportFederateParameter);
  require(
      report.parameters.size() == 2U + inheritedTargetParameterCount,
      "Federate MOM FOM-module report returned " +
          std::to_string(report.parameters.size()) +
          " unrecognized parameters alongside its declared child parameters");
  if (inheritedTargetParameterCount == 1U) {
    require(
        copyBytes(report.parameters.at(reportFederateParameter)) ==
            copyBytes(targetFederate.encode()),
        "Federate MOM FOM-module report identified a different joined federate");
  }

  rti::HLAinteger32BE reportedIndicator;
  reportedIndicator.decode(report.parameters.at(reportIndicatorParameter));
  require(
      reportedIndicator.get() == 0,
      "Federate MOM FOM-module report returned a different module indicator");
  rti::HLAunicodeString reportedModule;
  reportedModule.decode(report.parameters.at(reportDataParameter));
  require(
      !reportedModule.get().empty() &&
          reportedModule.get().find(L"<objectModel") != std::wstring::npos,
      "Federate MOM FOM-module report did not return standard FOM XML content");

  ambassador.unsubscribeInteractionClass(reportClass);
  requester.resign(rti::NO_ACTION);
  subject.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  subject.disconnect();
}

void scenarioFederateMomFomModuleContentReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomFomModuleContentReport(options, model);
}

void scenarioFederateMomObjectInstanceInformation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty() &&
          !options.objectClassName.empty() && !options.attributeName.empty(),
      "Federate MOM object-information reporting requires adapter-supplied FOM, standard MIM, object class, attribute, and logical-time configuration");

  Session requester(options, model, "member");
  Session owner(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-mom-object-instance-information");
  requester.connect();
  owner.connect();
  owner.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const requesterName =
      options.memberFederateName + L"-mom-object-information-requester";
  auto const ownerName = options.ownerFederateName + L"-mom-object-information-owner";
  requester.join(requesterName, options.federateType, federation);
  owner.join(ownerName, options.federateType, federation);

  auto& ambassador = requester.rtiAmbassador();
  auto const requesterClass =
      ambassador.getObjectClassHandle(options.objectClassName);
  auto const requesterAttribute =
      ambassador.getAttributeHandle(requesterClass, options.attributeName);
  auto const requesterFederate = ambassador.getFederateHandle(requesterName);
  auto const ownerClass =
      owner.rtiAmbassador().getObjectClassHandle(options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  require(
      requesterClass.isValid() && requesterAttribute.isValid() &&
          requesterFederate.isValid() && ownerClass.isValid() &&
          ownerAttribute.isValid(),
      "Adapter-supplied object class or attribute lookup returned an invalid handle");
  ambassador.publishObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute});
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      rti::AttributeHandleSet{ownerAttribute});
  ambassador.subscribeObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute},
      true);

  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportObjectInstanceInformation");
  auto const reportObjectParameter =
      ambassador.getParameterHandle(reportClass, L"HLAobjectInstance");
  auto const reportOwnedAttributesParameter =
      ambassador.getParameterHandle(reportClass, L"HLAownedInstanceAttributeList");
  auto const reportRegisteredClassParameter =
      ambassador.getParameterHandle(reportClass, L"HLAregisteredClass");
  auto const reportKnownClassParameter =
      ambassador.getParameterHandle(reportClass, L"HLAknownClass");
  auto const reportFederateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const requestClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLArequest.HLArequestObjectInstanceInformation");
  auto const requestObjectParameter =
      ambassador.getParameterHandle(requestClass, L"HLAobjectInstance");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  require(
      reportClass.isValid() && reportObjectParameter.isValid() &&
          reportOwnedAttributesParameter.isValid() &&
          reportRegisteredClassParameter.isValid() &&
          reportKnownClassParameter.isValid() &&
          reportFederateParameter.isValid() && requestClass.isValid() &&
          requestObjectParameter.isValid() && reliable.isValid(),
      "Standard MIM object-information request/report lookup returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass);

  auto decodeOwnedAttributes = [&](rti::VariableLengthData const& encoded) {
    // HLAattributeHandleList is a variable array whose elements are encoded
    // attribute-handle byte arrays, as declared by the standard MIM.
    rti::HLAvariableArray const attributeHandlePrototype{rti::HLAoctet{}};
    rti::HLAvariableArray handles{
        static_cast<rti::DataElement const&>(attributeHandlePrototype)};
    handles.decode(encoded);
    std::vector<rti::AttributeHandle> decoded;
    decoded.reserve(handles.size());
    for (std::size_t index = 0; index != handles.size(); ++index) {
      auto const& encodedAttribute =
          dynamic_cast<rti::HLAvariableArray const&>(handles.get(index));
      decoded.push_back(
          ambassador.decodeAttributeHandle(encodedAttribute.encode()));
    }
    return decoded;
  };
  auto requestInformation = [&](rti::ObjectInstanceHandle const& object,
                                std::string const& description) {
    requester.recorder().clearInteraction();
    rti::ParameterHandleValueMap const parameters{
        {requestObjectParameter, object.encode()}};
    ambassador.sendInteraction(
        requestClass,
        parameters,
        rti::VariableLengthData{});
    waitFor(
        requester,
        [&] { return !requester.recorder().interactions().empty(); },
        options,
        description);
    auto const reports = requester.recorder().interactions();
    require(
        reports.size() == 1U,
        description + " did not deliver exactly one object-information report");
    return reports.front();
  };
  auto requireReportBase = [&](InteractionRecord const& report,
                               rti::ObjectInstanceHandle const& object) {
    require(
        report.present && report.interaction == reportClass &&
            report.transportation == reliable &&
            report.parameters.find(reportObjectParameter) != report.parameters.end() &&
            report.parameters.find(reportOwnedAttributesParameter) !=
                report.parameters.end(),
        "Object-information report omitted its standard identity/list fields or used non-reliable delivery");
    auto const inheritedFederateParameterCount =
        report.parameters.count(reportFederateParameter);
    require(
        report.parameters.size() == 2U +
                (report.parameters.count(reportRegisteredClassParameter) != 0U ? 1U : 0U) +
                (report.parameters.count(reportKnownClassParameter) != 0U ? 1U : 0U) +
                inheritedFederateParameterCount,
        "Object-information report included parameters outside its MIM-defined response shape");
    require(
        ambassador.decodeObjectInstanceHandle(
            report.parameters.at(reportObjectParameter)) == object,
        "Object-information report identified a different object instance");
    if (inheritedFederateParameterCount == 1U) {
      require(
          copyBytes(report.parameters.at(reportFederateParameter)) ==
              copyBytes(requesterFederate.encode()),
          "Object-information report identified a different joined federate");
    }
  };

  auto const ownedObject = ambassador.registerObjectInstance(requesterClass);
  require(ownedObject.isValid(), "Object-information test registration returned an invalid handle");
  auto const ownedReport = requestInformation(
      ownedObject,
      "the requesting federate's owned object-information report");
  requireReportBase(ownedReport, ownedObject);
  auto const ownedAttributes = decodeOwnedAttributes(
      ownedReport.parameters.at(reportOwnedAttributesParameter));
  require(
      std::find(ownedAttributes.begin(), ownedAttributes.end(), requesterAttribute) !=
          ownedAttributes.end(),
      "Object-information report omitted the requesting federate's owned application attribute");
  require(
      ownedReport.parameters.count(reportRegisteredClassParameter) == 1U &&
          ownedReport.parameters.count(reportKnownClassParameter) == 1U &&
          ambassador.decodeObjectClassHandle(
              ownedReport.parameters.at(reportRegisteredClassParameter)) == requesterClass &&
          ambassador.decodeObjectClassHandle(
              ownedReport.parameters.at(reportKnownClassParameter)) == requesterClass,
      "Owned object-information report returned the wrong registered or known class");

  auto const observedObject = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(observedObject.isValid(), "Remote object-information registration returned an invalid handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(observedObject); },
      options,
      "discovery of the object-information query target");
  require(
      ambassador.getKnownObjectClassHandle(observedObject) == requesterClass,
      "Object-information query target was discovered with a different class");

  auto const knownReport = requestInformation(
      observedObject,
      "the known object-information report");
  requireReportBase(knownReport, observedObject);
  require(
      decodeOwnedAttributes(
          knownReport.parameters.at(reportOwnedAttributesParameter)).empty(),
      "Known-object information report included attributes not owned by the requesting federate");
  require(
      knownReport.parameters.count(reportRegisteredClassParameter) == 1U &&
          knownReport.parameters.count(reportKnownClassParameter) == 1U &&
          ambassador.decodeObjectClassHandle(
              knownReport.parameters.at(reportRegisteredClassParameter)) == requesterClass &&
          ambassador.decodeObjectClassHandle(
              knownReport.parameters.at(reportKnownClassParameter)) == requesterClass,
      "Known-object information report returned the wrong registered or known class");

  ambassador.localDeleteObjectInstance(observedObject);
  auto const nullReport = requestInformation(
      observedObject,
      "the NULL object-information report after local deletion");
  requireReportBase(nullReport, observedObject);
  require(
      decodeOwnedAttributes(
          nullReport.parameters.at(reportOwnedAttributesParameter)).empty() &&
          nullReport.parameters.count(reportRegisteredClassParameter) == 0U &&
          nullReport.parameters.count(reportKnownClassParameter) == 0U,
      "NULL object-information report did not use the standard empty-list/class-omission shape");

  ambassador.unsubscribeInteractionClass(reportClass);
  ambassador.unsubscribeObjectClassAttributes(
      requesterClass,
      rti::AttributeHandleSet{requesterAttribute});
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioFederateMomObjectInstanceInformationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomObjectInstanceInformation(options, model);
}

void scenarioFederateMomExceptionReportServicePrecondition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM service-precondition reporting requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-mom-exception-report-service-precondition");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  auto const federateName =
      options.ownerFederateName + L"-mom-exception-report-service-precondition";
  federate.join(federateName, options.federateType, federation);

  auto& ambassador = federate.rtiAmbassador();
  auto const reportClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportMOMexception");
  auto const serviceParameter =
      ambassador.getParameterHandle(reportClass, L"HLAservice");
  auto const exceptionParameter =
      ambassador.getParameterHandle(reportClass, L"HLAexception");
  auto const parameterErrorParameter =
      ambassador.getParameterHandle(reportClass, L"HLAparameterError");
  auto const federateParameter =
      ambassador.getParameterHandle(reportClass, L"HLAfederate");
  auto const reliable = ambassador.getTransportationTypeHandle(L"HLAreliable");
  auto const joinedFederate = ambassador.getFederateHandle(federateName);
  require(
      reportClass.isValid() && serviceParameter.isValid() &&
          exceptionParameter.isValid() && parameterErrorParameter.isValid() &&
          federateParameter.isValid() && reliable.isValid() &&
          joinedFederate.isValid(),
      "Standard MIM HLAreportMOMexception lookups returned an invalid handle");
  ambassador.subscribeInteractionClass(reportClass, true);

  auto const serviceInvocationClass = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      serviceInvocationClass.isValid(),
      "Standard MIM HLAreportServiceInvocation lookup returned an invalid handle");
  ambassador.subscribeInteractionClass(serviceInvocationClass, true);

  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const serviceReportingParameter =
      ambassador.getParameterHandle(setSwitches, L"HLAserviceReporting");
  require(
      setSwitches.isValid() && serviceReportingParameter.isValid(),
      "Standard federate HLAsetSwitches Service Reporting lookup returned an invalid handle");
  require(
      !ambassador.getServiceReportingSwitch(),
      "The standard Service Reporting switch did not begin disabled");

  auto setServiceReporting = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {serviceReportingParameter,
         rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    ambassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
  };
  try {
    setServiceReporting(true);
  } catch (rti::Exception const&) {
    // The MIM report, not a provider-specific synchronous exception class,
    // is the portable oracle for this rejected well-formed request.
  }
  while (ambassador.evokeCallback(0.0)) {
  }

  auto reports = federate.recorder().interactions();
  require(
      reports.size() == 1U,
      "The HLAsetSwitches precondition failure did not deliver exactly one HLAreportMOMexception");
  auto const& report = reports.front();
  require(
      report.present && report.interaction == reportClass &&
          report.parameters.size() == 4U && report.transportation == reliable,
      "HLAreportMOMexception callback metadata or parameter set was not standard");
  require(
      report.parameters.find(serviceParameter) != report.parameters.end() &&
          report.parameters.find(exceptionParameter) != report.parameters.end() &&
          report.parameters.find(parameterErrorParameter) != report.parameters.end() &&
          report.parameters.find(federateParameter) != report.parameters.end(),
      "HLAreportMOMexception omitted a required standard parameter");

  rti::HLAunicodeString serviceName;
  rti::HLAunicodeString exceptionText;
  rti::HLAboolean parameterError;
  serviceName.decode(report.parameters.at(serviceParameter));
  exceptionText.decode(report.parameters.at(exceptionParameter));
  parameterError.decode(report.parameters.at(parameterErrorParameter));
  require(
      serviceName.get() ==
          L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches",
      "HLAreportMOMexception did not identify the fully qualified standard MOM interaction");
  require(
      !exceptionText.get().empty(),
      "HLAreportMOMexception did not provide a textual problem description");
  require(
      !parameterError.get(),
      "HLAreportMOMexception marked a well-formed precondition failure as a parameter error");
  require(
      copyBytes(report.parameters.at(federateParameter)) ==
          copyBytes(joinedFederate.encode()),
      "HLAreportMOMexception identified a different joined federate");
  require(
      !ambassador.getServiceReportingSwitch(),
      "A failed Service Reporting precondition changed the switch");

  // The standard MIM permits enabling Service Reporting once this federate
  // no longer subscribes to the report interaction that would carry it.
  ambassador.unsubscribeInteractionClass(serviceInvocationClass);
  setServiceReporting(true);
  require(
      ambassador.getServiceReportingSwitch(),
      "Service Reporting did not enable after its MIM subscription precondition was removed");
  while (ambassador.evokeCallback(0.0)) {
  }
  reports = federate.recorder().interactions();
  require(
      reports.size() == 1U && reports.front().interaction == reportClass,
      "A successful Service Reporting enable produced an unexpected subscribed report");

  setServiceReporting(false);
  require(
      !ambassador.getServiceReportingSwitch(),
      "Service Reporting did not return to its initial disabled value");
  ambassador.unsubscribeInteractionClass(reportClass);
  federate.resign(rti::NO_ACTION);
  ambassador.destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateMomExceptionReportServicePreconditionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateMomExceptionReportServicePrecondition(options, model);
}

void scenarioFederateSendServiceReportsToFileMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM report-destination testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-send-service-reports-to-file-mom-switch");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-report-file-destination-mom-switch",
      options.federateType,
      federation);

  auto& ambassador = federate.rtiAmbassador();
  require(
      !ambassador.getServiceReportingSwitch(),
      "Standard MIM Service Reporting switch did not start false; report destination must remain inactive in this state-only scenario");
  auto const initialFileReporting =
      ambassador.getSendServiceReportsToFileSwitch();
  auto const initialExceptionReporting =
      ambassador.getExceptionReportingSwitch();
  auto const initialResignAction = ambassador.getAutomaticResignDirective();
  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const sendReportsToFile = ambassador.getParameterHandle(
      setSwitches,
      L"HLAsendServiceReportsToFile");
  require(
      setSwitches.isValid() && sendReportsToFile.isValid(),
      "Standard federate-MOM report-destination lookup returned an invalid handle");

  auto setReportDestinationThroughMom = [&](bool const enabled) {
    rti::ParameterHandleValueMap const parameters{
        {sendReportsToFile, rti::HLAinteger32BE{enabled ? 1 : 0}.encode()}};
    ambassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        ambassador.getSendServiceReportsToFileSwitch() == enabled,
        std::string("HLAsetSwitches HLAsendServiceReportsToFile did not set the switch to ") +
            (enabled ? "true" : "false"));
    require(
        !ambassador.getServiceReportingSwitch() &&
            ambassador.getExceptionReportingSwitch() ==
                initialExceptionReporting &&
            ambassador.getAutomaticResignDirective() == initialResignAction,
        "HLAsetSwitches HLAsendServiceReportsToFile changed a neighboring federate switch");
  };
  setReportDestinationThroughMom(false);
  setReportDestinationThroughMom(true);
  setReportDestinationThroughMom(initialFileReporting);

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateSendServiceReportsToFileMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateSendServiceReportsToFileMomSwitch(options, model);
}

void scenarioFederateAutomaticResignActionMomSwitch(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "Federate MOM Automatic Resign Action testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "owner");
  auto const federation = federationName(
      options,
      "federate-automatic-resign-action-mom-switch");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-automatic-resign-action-mom-switch",
      options.federateType,
      federation);

  auto& ambassador = federate.rtiAmbassador();
  require(
      !ambassador.getServiceReportingSwitch(),
      "Standard MIM Service Reporting switch did not start false for the Automatic Resign Action switch test");
  auto const initialResignAction =
      ambassador.getAutomaticResignDirective();
  auto const initialExceptionReporting =
      ambassador.getExceptionReportingSwitch();
  auto const initialFileReporting =
      ambassador.getSendServiceReportsToFileSwitch();
  auto const setSwitches = ambassador.getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetSwitches");
  auto const automaticResignAction = ambassador.getParameterHandle(
      setSwitches,
      L"HLAautomaticResignAction");
  require(
      setSwitches.isValid() && automaticResignAction.isValid(),
      "Standard federate-MOM Automatic Resign Action lookup returned an invalid handle");

  auto encodeResignAction = [](rti::ResignAction const action) {
    return rti::HLAinteger32BE{
        static_cast<std::int32_t>(action)}.encode();
  };
  auto setResignActionThroughMom = [&](rti::ResignAction const action) {
    rti::ParameterHandleValueMap const parameters{
        {automaticResignAction, encodeResignAction(action)}};
    ambassador.sendInteraction(
        setSwitches,
        parameters,
        rti::VariableLengthData{});
    require(
        ambassador.getAutomaticResignDirective() == action,
        "HLAsetSwitches HLAautomaticResignAction did not set the requested standard enum");
    require(
        !ambassador.getServiceReportingSwitch() &&
            ambassador.getExceptionReportingSwitch() ==
                initialExceptionReporting &&
            ambassador.getSendServiceReportsToFileSwitch() ==
                initialFileReporting,
        "HLAsetSwitches HLAautomaticResignAction changed a neighboring federate switch");
  };
  setResignActionThroughMom(rti::NO_ACTION);
  setResignActionThroughMom(rti::DELETE_OBJECTS);
  setResignActionThroughMom(initialResignAction);

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioFederateAutomaticResignActionMomAttribute(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Federate MOM Automatic Resign Action attribute testing requires adapter-supplied FOM and standard MIM");

  Session monitor(options, model, "automatic-resign-action-mom-attribute-monitor");
  Session subject(options, model, "automatic-resign-action-mom-attribute-subject");
  auto const federation = federationName(
      options,
      "federate-automatic-resign-action-mim-attribute");
  monitor.connect();
  subject.connect();
  if (options.logicalTimeImplementationName.empty()) {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    monitor.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }
  monitor.join(
      options.memberFederateName + L"-automatic-resign-action-mom-monitor",
      options.federateType,
      federation);

  auto const momClass = monitor.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = monitor.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const automaticResignActionAttribute =
      monitor.rtiAmbassador().getAttributeHandle(
          momClass,
          L"HLAautomaticResignAction");
  auto const reliable = monitor.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          automaticResignActionAttribute.isValid() && reliable.isValid() &&
          monitor.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          monitor.rtiAmbassador().getAttributeName(
              momClass,
              automaticResignActionAttribute) == L"HLAautomaticResignAction" &&
          monitor.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Federate MOM Automatic Resign Action lookup did not round-trip standard MIM handles");
  monitor.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{
          federateHandleAttribute,
          automaticResignActionAttribute},
      true,
      L"");

  subject.join(
      options.ownerFederateName + L"-automatic-resign-action-mom-subject",
      options.federateType,
      federation);
  auto const subjectFederate = subject.federateHandle();
  auto findSubjectMomObject = [&]()
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = monitor.recorder().reflections();
    auto const expectedHandle = copyBytes(subjectFederate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      monitor,
      [&] { return findSubjectMomObject().has_value(); },
      options,
      "federate MOM Automatic Resign Action subject discovery");
  auto const subjectMomObject = findSubjectMomObject();
  require(
      subjectMomObject.has_value() && subjectMomObject->isValid() &&
          monitor.rtiAmbassador().getKnownObjectClassHandle(*subjectMomObject) ==
              momClass,
      "Federate MOM Automatic Resign Action discovery returned the wrong standard object");

  auto actionReflectionsSince = [&](std::size_t firstReflection) {
    auto const reflections = monitor.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    std::vector<ReflectionRecord> result;
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (iterator->present && iterator->object == *subjectMomObject &&
          iterator->values.count(automaticResignActionAttribute) == 1U) {
        result.push_back(*iterator);
      }
    }
    return result;
  };
  auto requireActionReflection = [&](ReflectionRecord const& reflection,
                                     rti::ResignAction const expected,
                                     char const* description) {
    require(
        reflection.values.size() == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        std::string(description) +
            " had non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(automaticResignActionAttribute));
    require(
        value.get() == static_cast<std::int32_t>(expected),
        std::string(description) +
            " returned the wrong HLAresignAction value");
  };
  auto requestAction = [&](rti::ResignAction const expected,
                           char const* description) {
    auto const before = monitor.recorder().reflections().size();
    monitor.rtiAmbassador().requestAttributeValueUpdate(
        *subjectMomObject,
        rti::AttributeHandleSet{automaticResignActionAttribute},
        rti::VariableLengthData{});
    waitFor(
        monitor,
        [&] { return !actionReflectionsSince(before).empty(); },
        options,
        description);
    auto const reflections = actionReflectionsSince(before);
    require(
        reflections.size() == 1U,
        std::string(description) +
            " delivered an unexpected number of direct MOM reflections");
    requireActionReflection(reflections.front(), expected, description);
  };

  auto& subjectAmbassador = subject.rtiAmbassador();
  auto const initialAction = subjectAmbassador.getAutomaticResignDirective();
  requestAction(
      initialAction,
      "federate MOM Automatic Resign Action initial value request");

  auto const changedAction = initialAction == rti::NO_ACTION
      ? rti::DELETE_OBJECTS
      : rti::NO_ACTION;
  auto beforeChange = monitor.recorder().reflections().size();
  subjectAmbassador.setAutomaticResignDirective(changedAction);
  require(
      subjectAmbassador.getAutomaticResignDirective() == changedAction,
      "setAutomaticResignDirective did not retain the changed standard action");
  waitFor(
      monitor,
      [&] { return !actionReflectionsSince(beforeChange).empty(); },
      options,
      "conditional federate MOM Automatic Resign Action reflection");
  auto changedReflections = actionReflectionsSince(beforeChange);
  require(
      changedReflections.size() == 1U,
      "Changing the automatic resign action did not produce exactly one conditional MOM reflection");
  requireActionReflection(
      changedReflections.front(),
      changedAction,
      "conditional federate MOM Automatic Resign Action reflection");
  requestAction(
      changedAction,
      "federate MOM Automatic Resign Action changed value request");

  beforeChange = monitor.recorder().reflections().size();
  subjectAmbassador.setAutomaticResignDirective(initialAction);
  require(
      subjectAmbassador.getAutomaticResignDirective() == initialAction,
      "setAutomaticResignDirective did not restore the initial standard action");
  waitFor(
      monitor,
      [&] { return !actionReflectionsSince(beforeChange).empty(); },
      options,
      "restored conditional federate MOM Automatic Resign Action reflection");
  auto const restoredReflections = actionReflectionsSince(beforeChange);
  require(
      restoredReflections.size() == 1U,
      "Restoring the automatic resign action did not produce exactly one conditional MOM reflection");
  requireActionReflection(
      restoredReflections.front(),
      initialAction,
      "restored conditional federate MOM Automatic Resign Action reflection");
  requestAction(
      initialAction,
      "federate MOM Automatic Resign Action restored value request");

  subject.resign(rti::NO_ACTION);
  monitor.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  subject.disconnect();
  monitor.disconnect();
}

void scenarioFederateAutomaticResignActionMomAttributeContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateAutomaticResignActionMomAttribute(options, model);
}

void scenarioFederateAutomaticResignActionMomSwitchContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederateAutomaticResignActionMomSwitch(options, model);
}

void scenarioObjectRegistrationServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRegistration(options, model);
}

void scenarioObjectRegistrationServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRegistrationServiceBoundaries(options, model);
}

void scenarioObjectDeletionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectDeletion(options, model);
}

void scenarioObjectDeletionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectDeletionServiceBoundaries(options, model);
}

void scenarioAttributeUpdateServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeUpdate(options, model);
}

void scenarioAttributeUpdateServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeUpdateServiceBoundaries(options, model);
}

void scenarioInteractionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteraction(options, model);
}

void scenarioInteractionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionServiceBoundaries(options, model);
}

void scenarioAttributeValueRequestServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueRequest(options, model);
}

void scenarioAttributeValueRequestServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeValueRequestServiceBoundaries(options, model);
}

void scenarioConnectionServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnection(options, model);
}

void scenarioConnectionServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioConnectionServiceBoundaries(options, model);
}

void scenarioOwnershipServiceBoundaries(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnership(options, model);
}

void scenarioOwnershipServiceBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipManagementContract(options, model);
}

void scenarioJoinedFederateMomRegisteredObjectCount(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Joined-federate MOM registered-object testing requires an adapter-supplied FOM");
  require(
      !options.mimFom.empty(),
      "Joined-federate MOM registered-object testing requires an adapter-supplied standard MIM");

  Session owner(options, model, "mom-registered-owner");
  Session observer(options, model, "mom-registered-observer");
  auto const federation = federationName(
      options,
      "joined-federate-mom-registered-object-count");
  owner.connect();
  observer.connect();
  if (options.logicalTimeImplementationName.empty()) {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    owner.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  observer.join(
      options.memberFederateName + L"-mom-registered-observer",
      options.federateType,
      federation);
  auto const observerFederateClass = observer.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  require(
      observerFederateClass.isValid() &&
          observer.rtiAmbassador().getObjectClassName(observerFederateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate",
      "Joined-federate MOM registered-object lookup did not round-trip the standard class");

  auto const observerFederateHandleAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAfederateHandle");
  auto const observerRegisteredCountAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerFederateClass,
      L"HLAobjectInstancesRegistered");
  auto const reliable = observer.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      observerFederateHandleAttribute.isValid() &&
          observerRegisteredCountAttribute.isValid() && reliable.isValid() &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerFederateHandleAttribute) == L"HLAfederateHandle" &&
          observer.rtiAmbassador().getAttributeName(
              observerFederateClass,
              observerRegisteredCountAttribute) == L"HLAobjectInstancesRegistered" &&
          observer.rtiAmbassador().getTransportationTypeName(reliable) == L"HLAreliable",
      "Joined-federate MOM registered-object lookup returned inconsistent standard handles");
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerRegisteredCountAttribute},
      true,
      L"");

  owner.join(
      options.ownerFederateName + L"-mom-registered-owner",
      options.federateType,
      federation);
  auto const ownerFederateClass = owner.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const ownerFederateHandleAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAfederateHandle");
  auto const ownerRegisteredCountAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerFederateClass,
      L"HLAobjectInstancesRegistered");
  require(
      ownerFederateClass == observerFederateClass &&
          ownerFederateHandleAttribute == observerFederateHandleAttribute &&
          ownerRegisteredCountAttribute == observerRegisteredCountAttribute,
      "Joined-federate MOM registered-object lookup returned different handles to the joined federates");

  auto findOwnerMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = observer.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(owner.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(observerFederateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      observer,
      [&] { return findOwnerMomReflection().has_value(); },
      options,
      "joined-federate MOM registered-object owner object discovery");
  auto const ownerMomReflection = findOwnerMomReflection();
  require(
      ownerMomReflection.has_value() &&
          observer.rtiAmbassador().getKnownObjectClassHandle(
              ownerMomReflection->object) == observerFederateClass,
      "Joined-federate MOM registered-object discovery returned the wrong standard object");
  auto const ownerMomObject = ownerMomReflection->object;

  auto requestRegisteredCount = [&](std::int32_t expected,
                                    std::string const& description) {
    auto const before = observer.recorder().reflections().size();
    observer.rtiAmbassador().requestAttributeValueUpdate(
        ownerMomObject,
        rti::AttributeHandleSet{observerRegisteredCountAttribute},
        rti::VariableLengthData{});
    waitFor(
        observer,
        [&] {
          auto const reflections = observer.recorder().reflections();
          return reflections.size() > before &&
              std::any_of(
                  reflections.begin() + static_cast<std::ptrdiff_t>(before),
                  reflections.end(),
                  [&](ReflectionRecord const& reflection) {
                    return reflection.object == ownerMomObject &&
                        reflection.values.count(observerRegisteredCountAttribute) == 1U;
                  });
        },
        options,
        description);
    auto const reflections = observer.recorder().reflections();
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(before),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.object == ownerMomObject &&
              reflection.values.count(observerRegisteredCountAttribute) == 1U;
        });
    require(iterator != reflections.end(), description + " produced no registered-object value");
    auto const& reflection = *iterator;
    require(
        reflection.values.size() == 1U && reflection.transportation == reliable &&
            reflection.tag.empty() && !reflection.producer.isValid() &&
            !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE decoded;
    decoded.decode(reflection.values.at(observerRegisteredCountAttribute));
    require(
        decoded.get() == expected,
        description + " returned the wrong registered-object count");
  };

  requestRegisteredCount(
      0,
      "joined-federate MOM registered-object initial value request");

  auto const ownerObjectClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerObjectClass,
      options.attributeName);
  require(
      ownerObjectClass.isValid() && ownerAttribute.isValid(),
      "Joined-federate MOM registered-object application lookup returned an invalid handle");
  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  auto const firstObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      firstObject.isValid(),
      "Joined-federate MOM registered-object first registration returned an invalid handle");
  requestRegisteredCount(
      1,
      "joined-federate MOM registered-object first value request");
  auto const secondObject = owner.rtiAmbassador().registerObjectInstance(ownerObjectClass);
  require(
      secondObject.isValid(),
      "Joined-federate MOM registered-object second registration returned an invalid handle");
  requestRegisteredCount(
      2,
      "joined-federate MOM registered-object second value request");

  auto const setTiming = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = observer.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          observer.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          observer.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          observer.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM registered-object count HLAsetTiming lookup did not round-trip");
  auto const beforePeriodic = observer.recorder().reflections().size();
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, owner.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{1}.encode()}},
      rti::VariableLengthData{});
  waitFor(
      observer,
      [&] {
        auto const reflections = observer.recorder().reflections();
        auto const begin = reflections.begin() +
            static_cast<std::ptrdiff_t>(std::min(beforePeriodic, reflections.size()));
        return std::any_of(
            begin,
            reflections.end(),
            [&](ReflectionRecord const& reflection) {
              return reflection.present && reflection.object == ownerMomObject &&
                  reflection.values.count(observerRegisteredCountAttribute) == 1U;
            });
      },
      options,
      "joined-federate MOM registered-object periodic reflection");
  auto const periodicReflections = observer.recorder().reflections();
  auto const periodicBegin = periodicReflections.begin() +
      static_cast<std::ptrdiff_t>(std::min(beforePeriodic, periodicReflections.size()));
  auto const periodicReflection = std::find_if(
      periodicBegin,
      periodicReflections.end(),
      [&](ReflectionRecord const& reflection) {
        return reflection.present && reflection.object == ownerMomObject &&
            reflection.values.count(observerRegisteredCountAttribute) == 1U;
      });
  require(
      periodicReflection != periodicReflections.end() &&
          periodicReflection->values.size() == 1U &&
          periodicReflection->transportation == reliable &&
          periodicReflection->tag.empty() && !periodicReflection->producer.isValid() &&
          !periodicReflection->regions.has_value(),
      "Joined-federate MOM registered-object periodic reflection metadata was incorrect");
  rti::HLAinteger32BE periodicRegisteredCount;
  periodicRegisteredCount.decode(
      periodicReflection->values.at(observerRegisteredCountAttribute));
  require(
      periodicRegisteredCount.get() == 2,
      "Joined-federate MOM registered-object periodic report returned the wrong count");
  observer.rtiAmbassador().sendInteraction(
      setTiming,
      rti::ParameterHandleValueMap{
          {timingFederate, owner.federateHandle().encode()},
          {reportPeriod, rti::HLAinteger32BE{0}.encode()}},
      rti::VariableLengthData{});

  observer.rtiAmbassador().unsubscribeObjectClassAttributes(
      observerFederateClass,
      rti::AttributeHandleSet{
          observerFederateHandleAttribute,
          observerRegisteredCountAttribute});
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerObjectClass,
      ownerAttributes);
  owner.resign(rti::DELETE_OBJECTS);
  observer.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioJoinedFederateMomRegisteredObjectCountContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomRegisteredObjectCount(options, model);
}

void scenarioJoinedFederateMomRoLengthPeriodic(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Joined-federate MOM RO-length testing requires an adapter-supplied FOM and standard MIM");
  require(
      !options.interactionClassName.empty() && !options.parameterName.empty(),
      "Joined-federate MOM RO-length testing requires adapter-supplied interaction and parameter names");

  Session receiver(options, model, "member");
  Session publisher(options, model, "owner");
  auto const federation = federationName(
      options,
      "joined-federate-mom-ro-length-periodic");
  receiver.connect();
  publisher.connect();
  if (options.logicalTimeImplementationName.empty()) {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring());
  } else {
    publisher.rtiAmbassador().createFederationExecutionWithMIM(
        federation,
        std::vector<std::wstring>{options.fom.wstring()},
        options.mimFom.wstring(),
        options.logicalTimeImplementationName);
  }

  receiver.join(
      options.memberFederateName + L"-mom-ro-length-receiver",
      options.federateType,
      federation);
  auto const momClass = receiver.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAfederateHandle");
  auto const roLengthAttribute = receiver.rtiAmbassador().getAttributeHandle(
      momClass,
      L"HLAROlength");
  auto const reliable = receiver.rtiAmbassador().getTransportationTypeHandle(
      L"HLAreliable");
  require(
      momClass.isValid() && federateHandleAttribute.isValid() &&
          roLengthAttribute.isValid() && reliable.isValid() &&
          receiver.rtiAmbassador().getObjectClassName(momClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              federateHandleAttribute) == L"HLAfederateHandle" &&
          receiver.rtiAmbassador().getAttributeName(
              momClass,
              roLengthAttribute) == L"HLAROlength" &&
          receiver.rtiAmbassador().getTransportationTypeName(reliable) ==
              L"HLAreliable",
      "Joined-federate MOM RO-length lookup did not round-trip the standard MIM handles");
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, roLengthAttribute},
      true,
      L"");

  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      receiverInteraction.isValid() && receiverParameter.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Joined-federate MOM RO-length application interaction lookup did not round-trip");
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction);

  publisher.join(
      options.ownerFederateName + L"-mom-ro-length-publisher",
      options.federateType,
      federation);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  require(
      publisherInteraction == receiverInteraction &&
          publisherParameter == receiverParameter,
      "Joined-federate MOM RO-length application handles differed between federates");
  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);

  auto findMomObject = [&](rti::FederateHandle const& federate)
      -> std::optional<rti::ObjectInstanceHandle> {
    auto const reflections = receiver.recorder().reflections();
    auto const expectedHandle = copyBytes(federate.encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return iterator->object;
  };
  waitFor(
      receiver,
      [&] {
        return findMomObject(receiver.federateHandle()).has_value() &&
            findMomObject(publisher.federateHandle()).has_value();
      },
      options,
      "joined-federate MOM RO-length object discovery and reflection");
  auto const receiverMomObject = findMomObject(receiver.federateHandle());
  require(
      receiverMomObject.has_value() &&
          receiver.rtiAmbassador().getKnownObjectClassHandle(
              *receiverMomObject) == momClass,
      "Joined-federate MOM RO-length lookup found the wrong receiver object");

  auto findRoLengthReflection = [&](std::size_t firstReflection)
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(firstReflection, reflections.size());
    auto const iterator = std::find_if(
        reflections.begin() + static_cast<std::ptrdiff_t>(begin),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          return reflection.present && reflection.object == *receiverMomObject &&
              reflection.values.count(roLengthAttribute) == 1U;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  auto decodeRoLength = [&](ReflectionRecord const& reflection,
                            std::string const& description) {
    require(
        reflection.values.count(roLengthAttribute) == 1U &&
            reflection.transportation == reliable && reflection.tag.empty() &&
            !reflection.producer.isValid() && !reflection.regions.has_value(),
        description + " returned non-standard MOM reflection metadata");
    rti::HLAinteger32BE value;
    value.decode(reflection.values.at(roLengthAttribute));
    require(value.get() >= 0, description + " returned a negative RO length");
    return value.get();
  };
  auto requestRoLength = [&](std::string const& description) {
    auto const before = receiver.recorder().reflections().size();
    receiver.rtiAmbassador().requestAttributeValueUpdate(
        *receiverMomObject,
        rti::AttributeHandleSet{roLengthAttribute},
        rti::VariableLengthData{});
    waitFor(
        receiver,
        [&] { return findRoLengthReflection(before).has_value(); },
        options,
        description);
    auto const reflection = findRoLengthReflection(before);
    require(reflection.has_value(), description + " produced no RO-length value");
    return decodeRoLength(*reflection, description);
  };
  auto drainEvokedCallbacks = [&] {
    if (model != rti::HLA_EVOKED) {
      return;
    }
    for (std::size_t pass = 0; pass != 512U; ++pass) {
      if (!receiver.rtiAmbassador().evokeCallback(0.0)) {
        return;
      }
    }
    throw std::runtime_error(
        "Joined-federate MOM RO-length callback queue did not drain");
  };
  auto requireEmptyQueue = [&](std::string const& description) {
    for (std::size_t attempt = 0; attempt != 5U; ++attempt) {
      auto const count = requestRoLength(
          description + " request " + std::to_string(attempt + 1U));
      if (count == 0) {
        return;
      }
      drainEvokedCallbacks();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    throw std::runtime_error(description + " did not reach an empty RO queue");
  };
  drainEvokedCallbacks();
  requireEmptyQueue("Joined-federate MOM initial RO-length value");

  auto const setTiming = receiver.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming");
  auto const timingFederate = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAfederate");
  auto const reportPeriod = receiver.rtiAmbassador().getParameterHandle(
      setTiming,
      L"HLAreportPeriod");
  require(
      setTiming.isValid() && timingFederate.isValid() && reportPeriod.isValid() &&
          receiver.rtiAmbassador().getInteractionClassName(setTiming) ==
              L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAadjust.HLAsetTiming" &&
          receiver.rtiAmbassador().getParameterName(setTiming, timingFederate) ==
              L"HLAfederate" &&
          receiver.rtiAmbassador().getParameterName(setTiming, reportPeriod) ==
              L"HLAreportPeriod",
      "Joined-federate MOM RO-length HLAsetTiming lookup did not round-trip");
  auto sendSetTiming = [&](std::int32_t period) {
    receiver.rtiAmbassador().sendInteraction(
        setTiming,
        rti::ParameterHandleValueMap{
            {timingFederate, receiver.federateHandle().encode()},
            {reportPeriod, rti::HLAinteger32BE{period}.encode()}},
        rti::VariableLengthData{});
  };

  std::vector<std::uint8_t> const applicationPayload{0x52U, 0x4fU};
  rti::VariableLengthData const encodedApplicationPayload(
      applicationPayload.data(),
      applicationPayload.size());
  auto sendApplicationInteraction = [&] {
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        rti::ParameterHandleValueMap{
            {publisherParameter, encodedApplicationPayload}},
        rti::VariableLengthData{});
  };
  auto countApplicationInteractions = [&](std::size_t firstInteraction) {
    auto const interactions = receiver.recorder().interactions();
    auto const begin = std::min(firstInteraction, interactions.size());
    return static_cast<std::size_t>(std::count_if(
        interactions.begin() + static_cast<std::ptrdiff_t>(begin),
        interactions.end(),
        [&](InteractionRecord const& interaction) {
          auto const value = interaction.parameters.find(receiverParameter);
          return interaction.present && interaction.interaction == receiverInteraction &&
              interaction.producer == publisher.federateHandle() &&
              value != interaction.parameters.end() &&
              copyBytes(value->second) == applicationPayload &&
              interaction.tag.empty() && !interaction.regions.has_value();
        }));
  };

  std::size_t const directInteractionStart =
      receiver.recorder().interactions().size();
  if (model == rti::HLA_IMMEDIATE) {
    receiver.rtiAmbassador().disableCallbacks();
  }
  for (std::size_t message = 0; message != 3U; ++message) {
    sendApplicationInteraction();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  static_cast<void>(receiver.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate"));
  auto const directReflectionStart = receiver.recorder().reflections().size();
  receiver.rtiAmbassador().requestAttributeValueUpdate(
      *receiverMomObject,
      rti::AttributeHandleSet{roLengthAttribute},
      rti::VariableLengthData{});
  if (model == rti::HLA_IMMEDIATE) {
    receiver.rtiAmbassador().enableCallbacks();
  }
  waitFor(
      receiver,
      [&] {
        return countApplicationInteractions(directInteractionStart) >= 3U &&
            findRoLengthReflection(directReflectionStart).has_value();
      },
      options,
      "joined-federate MOM positive direct RO-length value and queued interactions");
  auto const directReflection = findRoLengthReflection(directReflectionStart);
  require(
      directReflection.has_value() &&
          decodeRoLength(*directReflection,
                         "Joined-federate MOM direct RO-length reflection") >= 1,
      "Joined-federate MOM direct RO-length value did not report queued receive-order messages");
  require(
      countApplicationInteractions(directInteractionStart) == 3U,
      "Joined-federate MOM direct RO-length phase delivered an unexpected interaction count");
  requireEmptyQueue("Joined-federate MOM direct RO-length after delivery");

  std::size_t const periodicReflectionStart =
      receiver.recorder().reflections().size();
  auto const periodicInteractionStart =
      receiver.recorder().interactions().size();
  sendSetTiming(1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  if (model == rti::HLA_IMMEDIATE) {
    receiver.rtiAmbassador().disableCallbacks();
  }
  sendApplicationInteraction();
  std::this_thread::sleep_for(std::chrono::milliseconds(2200));
  if (model == rti::HLA_IMMEDIATE) {
    receiver.rtiAmbassador().enableCallbacks();
  }

  auto findPositivePeriodicReflection = [&]()
      -> std::optional<ReflectionRecord> {
    auto const reflections = receiver.recorder().reflections();
    auto const begin = std::min(periodicReflectionStart, reflections.size());
    for (auto iterator = reflections.begin() +
                         static_cast<std::ptrdiff_t>(begin);
         iterator != reflections.end();
         ++iterator) {
      if (!iterator->present || iterator->object != *receiverMomObject ||
          iterator->values.count(roLengthAttribute) != 1U) {
        continue;
      }
      rti::HLAinteger32BE value;
      value.decode(iterator->values.at(roLengthAttribute));
      if (value.get() > 0) {
        return *iterator;
      }
    }
    return std::nullopt;
  };
  waitFor(
      receiver,
      [&] {
        return countApplicationInteractions(periodicInteractionStart) >= 1U &&
            findPositivePeriodicReflection().has_value();
      },
      options,
      "joined-federate MOM periodic positive RO-length value and queued interaction");
  auto const periodicReflection = findPositivePeriodicReflection();
  require(
      periodicReflection.has_value() &&
          decodeRoLength(*periodicReflection,
                         "Joined-federate MOM periodic RO-length reflection") >= 1,
      "Joined-federate MOM periodic RO-length value did not report a queued receive-order message");
  sendSetTiming(0);
  require(
      countApplicationInteractions(periodicInteractionStart) == 1U,
      "Joined-federate MOM periodic RO-length phase delivered an unexpected interaction count");
  drainEvokedCallbacks();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  requireEmptyQueue("Joined-federate MOM periodic RO-length after delivery");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      momClass,
      rti::AttributeHandleSet{federateHandleAttribute, roLengthAttribute});
  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioJoinedFederateMomRoLengthPeriodicContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioJoinedFederateMomRoLengthPeriodic(options, model);
}

void scenarioAttributeUpdateOrderClassification(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Attribute-update order classification requires an adapter-supplied FOM");

  Session publisher(options, model, "attribute-update-order-classification-publisher");
  Session receiver(options, model, "attribute-update-order-classification-receiver");
  auto const federation = federationName(options, "attribute-update-order-classification");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Attribute-update order classification lookup returned an invalid handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, receiverAttributes, true, L"");

  auto const receiveOrderObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      receiveOrderObject.isValid(),
      "Receive-order attribute-update registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(receiveOrderObject); },
      options,
      "receive-order object discovery in attribute-update order classification");
  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "Attribute-update order classification");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  receiver.recorder().clearReflection();
  publisher.recorder().clearReflection();

  std::vector<std::uint8_t> const receiveOrderValue{0x52U, 0x4FU};
  std::vector<std::uint8_t> const receiveOrderTag{0x52U, 0x4FU};
  rti::AttributeHandleValueMap receiveOrderValues;
  receiveOrderValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          receiveOrderValue.data(), receiveOrderValue.size()));
  rti::VariableLengthData receiveOrderUserTag(
      receiveOrderTag.data(), receiveOrderTag.size());
  publisher.rtiAmbassador().updateAttributeValues(
      receiveOrderObject, receiveOrderValues, receiveOrderUserTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().reflections().size() >= 1U; },
      options,
      "receive-order reflection in attribute-update order classification");

  auto const receiveOrderRecords = receiver.recorder().reflections();
  require(
      receiveOrderRecords.size() == 1U &&
          receiver.recorder().timedReflections().empty(),
      "ordinary Update Attribute Values did not use only the receive-order callback path");
  auto const& receiveOrderRecord = receiveOrderRecords.front();
  require(
      receiveOrderRecord.present &&
          receiveOrderRecord.object == receiveOrderObject &&
          receiveOrderRecord.values.size() == 1U &&
          receiveOrderRecord.values.count(receiverAttribute) == 1U &&
          copyBytes(receiveOrderRecord.values.at(receiverAttribute)) ==
              receiveOrderValue &&
          receiveOrderRecord.tag == receiveOrderTag &&
          receiveOrderRecord.producer == publisher.federateHandle(),
      "ordinary reflection returned incorrect object, value, tag, or producer");
  require(
      receiveOrderRecord.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              receiveOrderRecord.transportation).empty() &&
          !receiveOrderRecord.regions.has_value(),
      "ordinary reflection returned invalid standard metadata");

  publisher.rtiAmbassador().changeDefaultAttributeOrderType(
      publisherClass, publisherAttributes, rti::TIMESTAMP);
  auto const timestampedObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      timestampedObject.isValid(),
      "Timestamped attribute-update registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(timestampedObject); },
      options,
      "timestamped object discovery in attribute-update order classification");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  std::vector<std::uint8_t> const timestampedValue{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const timestampedTag{0x54U, 0x53U};
  rti::AttributeHandleValueMap timestampedValues;
  timestampedValues.emplace(
      publisherAttribute,
      rti::VariableLengthData(
          timestampedValue.data(), timestampedValue.size()));
  rti::VariableLengthData timestampedUserTag(
      timestampedTag.data(), timestampedTag.size());
  auto const retraction = publisher.rtiAmbassador().updateAttributeValues(
      timestampedObject,
      timestampedValues,
      timestampedUserTag,
      *timestamp);
  require(
      retraction.isValid(),
      "timestamped Update Attribute Values returned an invalid retraction handle");
  require(
      receiver.recorder().reflections().size() == 1U &&
          receiver.recorder().timedReflections().empty(),
      "timestamped Update Attribute Values was delivered before the receiver's advance request");

  receiver.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().timeAdvanceRequest(*timestamp);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedReflections().size() >= 1U;
      },
      options,
      "timestamped reflection and matching grants in attribute-update order classification");

  auto const timedRecords = receiver.recorder().timedReflections();
  require(
      receiver.recorder().reflections().size() == 1U && timedRecords.size() == 1U,
      "same receiver did not keep RO and TSO updates on their distinct callback paths");
  auto const& timedRecord = timedRecords.front();
  require(
      timedRecord.object == timestampedObject &&
          timedRecord.values.size() == 1U &&
          timedRecord.values.count(receiverAttribute) == 1U &&
          copyBytes(timedRecord.values.at(receiverAttribute)) == timestampedValue &&
          timedRecord.tag == timestampedTag &&
          timedRecord.producer == publisher.federateHandle(),
      "timestamped reflection returned incorrect object, value, tag, or producer");
  require(
      timedRecord.time == encodeTime(*timestamp) &&
          timedRecord.sentOrder == rti::TIMESTAMP &&
          timedRecord.receivedOrder == rti::TIMESTAMP &&
          timedRecord.retractionPresent &&
          timedRecord.retraction == copyBytes(retraction.encode()),
      "timestamped reflection returned incorrect time, order, or retraction metadata");
  require(
      timedRecord.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              timedRecord.transportation).empty() &&
          !timedRecord.regions.has_value(),
      "timestamped reflection returned invalid standard metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"reflect", "grant"},
      "timestamped reflection was not delivered before its matching grant");
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  auto const receiverGrants = receiver.recorder().timeAdvanceGrants();
  require(
      publisherGrants.size() == 1U && receiverGrants.size() == 1U &&
          publisherGrants.front().encoded == encodeTime(*timestamp) &&
          receiverGrants.front().encoded == encodeTime(*receiverTarget),
      "time advance grants did not report their requested logical times");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass, receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioAttributeUpdateOrderClassificationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeUpdateOrderClassification(options, model);
}

void scenarioReceiveOrderInteraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order interaction testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-interaction-publisher");
  Session receiver(options, model, "receive-order-interaction-receiver");
  auto const federation = federationName(
      options,
      "receive-order-interaction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Receive-order interaction lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getInteractionClassName(publisherInteraction) ==
              options.interactionClassName &&
          receiver.rtiAmbassador().getInteractionClassName(receiverInteraction) ==
              options.interactionClassName &&
          publisher.rtiAmbassador().getParameterName(
              publisherInteraction,
              publisherParameter) == options.parameterName &&
          receiver.rtiAmbassador().getParameterName(
              receiverInteraction,
              receiverParameter) == options.parameterName,
      "Receive-order interaction lookup did not round-trip adapter names");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  std::vector<std::uint8_t> const firstValue{0x50U, 0x31U};
  std::vector<std::uint8_t> const firstTag{0x49U, 0x31U};
  std::vector<std::uint8_t> const secondValue{0x50U, 0x32U};
  std::vector<std::uint8_t> const secondTag{0x49U, 0x32U};
  auto send = [&](std::vector<std::uint8_t> const& value,
                  std::vector<std::uint8_t> const& tagBytes) {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag);
  };
  send(firstValue, firstTag);
  send(secondValue, secondTag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().interactions().empty(),
        "Receive-order interaction delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 2U; },
      options,
      "receive-order interaction callbacks");

  auto const received = receiver.recorder().interactions();
  require(
      received.size() == 2U,
      "Receive-order interaction delivered an unexpected callback count");
  auto assertInteraction = [&](InteractionRecord const& interaction,
                               std::vector<std::uint8_t> const& expectedValue,
                               std::vector<std::uint8_t> const& expectedTag,
                               std::string const& description) {
    require(
        interaction.present &&
            interaction.interaction == receiverInteraction &&
            interaction.parameters.size() == 1U &&
            interaction.parameters.count(receiverParameter) == 1U &&
            copyBytes(interaction.parameters.at(receiverParameter)) ==
                expectedValue,
        description + " returned the wrong class, parameter, or value");
    require(
        interaction.tag == expectedTag &&
            interaction.producer == publisher.federateHandle(),
        description + " returned the wrong tag or producing federate");
    require(
        interaction.transportation.isValid() &&
            !receiver.rtiAmbassador().getTransportationTypeName(
                interaction.transportation).empty() &&
            !interaction.regions.has_value(),
        description + " returned invalid receive-order interaction metadata");
  };
  assertInteraction(
      received.front(),
      firstValue,
      firstTag,
      "first receive-order interaction");
  assertInteraction(
      received.back(),
      secondValue,
      secondTag,
      "second receive-order interaction");
  require(
      publisher.recorder().interactions().empty(),
      "Receive-order interaction looped a callback back to its publisher");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderInteraction(options, model);
}

void scenarioInteractionOrderClassification(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Interaction-order classification requires an adapter-supplied FOM");

  Session publisher(options, model, "interaction-order-classification-publisher");
  Session receiver(options, model, "interaction-order-classification-receiver");
  auto const federation = federationName(options, "interaction-order-classification");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction, options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction, options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Interaction-order classification lookup returned an invalid handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  std::vector<std::uint8_t> const receiveOrderValue{0x52U, 0x4FU};
  std::vector<std::uint8_t> const receiveOrderTag{0x52U, 0x4FU};
  rti::ParameterHandleValueMap receiveOrderParameters;
  receiveOrderParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          receiveOrderValue.data(), receiveOrderValue.size()));
  rti::VariableLengthData receiveOrderUserTag(
      receiveOrderTag.data(), receiveOrderTag.size());
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      receiveOrderParameters,
      receiveOrderUserTag);
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 1U; },
      options,
      "receive-order callback in interaction-order classification");

  auto const receiveOrderRecords = receiver.recorder().interactions();
  require(
      receiveOrderRecords.size() == 1U &&
          receiver.recorder().timedInteractions().empty(),
      "ordinary Send Interaction did not use only the receive-order callback path");
  auto const& receiveOrderRecord = receiveOrderRecords.front();
  require(
      receiveOrderRecord.present &&
          receiveOrderRecord.interaction == receiverInteraction &&
          receiveOrderRecord.parameters.size() == 1U &&
          receiveOrderRecord.parameters.count(receiverParameter) == 1U &&
          copyBytes(receiveOrderRecord.parameters.at(receiverParameter)) ==
              receiveOrderValue &&
          receiveOrderRecord.tag == receiveOrderTag &&
          receiveOrderRecord.producer == publisher.federateHandle(),
      "ordinary Send Interaction callback returned incorrect class, value, tag, or producer");
  require(
      receiveOrderRecord.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              receiveOrderRecord.transportation).empty() &&
          !receiveOrderRecord.regions.has_value(),
      "ordinary Send Interaction callback returned invalid standard metadata");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "Interaction-order classification");
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction, rti::TIMESTAMP);
  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  std::vector<std::uint8_t> const timestampedValue{0x54U, 0x53U, 0x4FU};
  std::vector<std::uint8_t> const timestampedTag{0x54U, 0x53U};
  rti::ParameterHandleValueMap timestampedParameters;
  timestampedParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          timestampedValue.data(), timestampedValue.size()));
  rti::VariableLengthData timestampedUserTag(
      timestampedTag.data(), timestampedTag.size());
  auto const retraction = publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      timestampedParameters,
      timestampedUserTag,
      *timestamp);
  require(
      retraction.isValid(),
      "timestamped Send Interaction returned an invalid retraction handle");
  require(
      receiver.recorder().interactions().size() == 1U &&
          receiver.recorder().timedInteractions().empty(),
      "timestamped Send Interaction was delivered before the receiver's advance request");

  receiver.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().timeAdvanceRequest(*timestamp);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      options,
      "timestamp-order callback and matching grants");

  auto const timedRecords = receiver.recorder().timedInteractions();
  require(
      receiver.recorder().interactions().size() == 1U && timedRecords.size() == 1U,
      "same receiver did not keep RO and TSO messages on their distinct callback paths");
  auto const& timedRecord = timedRecords.front();
  require(
      timedRecord.interaction == receiverInteraction &&
          timedRecord.parameters.size() == 1U &&
          timedRecord.parameters.count(receiverParameter) == 1U &&
          copyBytes(timedRecord.parameters.at(receiverParameter)) == timestampedValue &&
          timedRecord.tag == timestampedTag &&
          timedRecord.producer == publisher.federateHandle(),
      "timestamp-order callback returned incorrect class, value, tag, or producer");
  require(
      timedRecord.time == encodeTime(*timestamp) &&
          timedRecord.sentOrder == rti::TIMESTAMP &&
          timedRecord.receivedOrder == rti::TIMESTAMP &&
          timedRecord.retractionPresent &&
          timedRecord.retraction == copyBytes(retraction.encode()),
      "timestamp-order callback returned incorrect time, order, or retraction metadata");
  require(
      timedRecord.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              timedRecord.transportation).empty() &&
          !timedRecord.regions.has_value(),
      "timestamp-order callback returned invalid standard metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"interaction", "grant"},
      "timestamped interaction was not delivered before its matching grant");
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  auto const receiverGrants = receiver.recorder().timeAdvanceGrants();
  require(
      publisherGrants.size() == 1U && receiverGrants.size() == 1U &&
          publisherGrants.front().encoded == encodeTime(*timestamp) &&
          receiverGrants.front().encoded == encodeTime(*receiverTarget),
      "time advance grants did not report their requested logical times");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioInteractionOrderClassificationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionOrderClassification(options, model);
}

void scenarioInteractionPublicationSendTransition(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Interaction publication/send transition testing requires an adapter-supplied FOM");

  Session publisher(options, model, "interaction-publication-transition-publisher");
  Session receiver(options, model, "interaction-publication-transition-receiver");
  auto const federation = federationName(
      options,
      "interaction-publication-send-transition");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const receiverInteraction =
      receiver.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Interaction publication/send transition lookup returned an invalid standard handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  receiver.rtiAmbassador().subscribeInteractionClass(receiverInteraction, true);

  std::vector<std::uint8_t> const value{0x50U, 0x54U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x31U};
  auto send = [&] {
    rti::ParameterHandleValueMap parameters;
    parameters.emplace(
        publisherParameter,
        rti::VariableLengthData(value.data(), value.size()));
    rti::VariableLengthData tag(tagBytes.data(), tagBytes.size());
    publisher.rtiAmbassador().sendInteraction(
        publisherInteraction,
        parameters,
        tag);
  };

  send();
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().interactions().empty(),
        "Published interaction arrived before callback servicing in evoked mode");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 1U; },
      options,
      "published interaction delivery before unpublication");
  auto const delivered = receiver.recorder().interactions();
  require(
      delivered.size() == 1U && delivered.front().present &&
          delivered.front().interaction == receiverInteraction &&
          delivered.front().parameters.size() == 1U &&
          delivered.front().parameters.count(receiverParameter) == 1U &&
          copyBytes(delivered.front().parameters.at(receiverParameter)) == value &&
          delivered.front().tag == tagBytes &&
          delivered.front().producer == publisher.federateHandle(),
      "Published interaction was not delivered with the expected class, parameter, payload, tag, and producer");

  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  requireException(
      send,
      L"InteractionClassNotPublished",
      "sending an ordinary interaction after unpublishing its class");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  send();
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().interactions().size() == 1U,
        "Republished interaction arrived before callback servicing in evoked mode");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().interactions().size() >= 2U; },
      options,
      "republished interaction delivery after publication recovery");
  auto const recovered = receiver.recorder().interactions();
  require(
      recovered.size() == 2U && recovered.back().present &&
          recovered.back().interaction == receiverInteraction &&
          recovered.back().parameters.size() == 1U &&
          recovered.back().parameters.count(receiverParameter) == 1U &&
          copyBytes(recovered.back().parameters.at(receiverParameter)) == value &&
          recovered.back().tag == tagBytes &&
          recovered.back().producer == publisher.federateHandle(),
      "Republishing did not restore ordinary interaction delivery with the expected payload metadata");
  require(
      publisher.recorder().interactions().empty(),
      "Republished interaction looped a callback back to its publisher");

  receiver.rtiAmbassador().unsubscribeInteractionClass(receiverInteraction);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioInteractionPublicationSendTransitionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionPublicationSendTransition(options, model);
}

void scenarioInteractionActiveAndPassiveSubscriptionModes(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Active/passive interaction-subscription testing requires an adapter-supplied FOM");

  Session publisher(options, model, "interaction-subscription-modes-publisher");
  Session activeSubscriber(options, model, "interaction-subscription-modes-active");
  Session passiveSubscriber(options, model, "interaction-subscription-modes-passive");
  auto const federation = federationName(
      options,
      "interaction-active-and-passive-subscription-modes");
  connectAndJoin(
      publisher,
      activeSubscriber,
      options,
      federation,
      options.fom);
  passiveSubscriber.connect();
  passiveSubscriber.join(
      options.memberFederateName + L"-passive-interaction-subscription",
      options.federateType,
      federation);

  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const activeInteraction =
      activeSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const passiveInteraction =
      passiveSubscriber.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const activeParameter = activeSubscriber.rtiAmbassador().getParameterHandle(
      activeInteraction,
      options.parameterName);
  auto const passiveParameter = passiveSubscriber.rtiAmbassador().getParameterHandle(
      passiveInteraction,
      options.parameterName);
  require(
      publisherInteraction.isValid() && activeInteraction.isValid() &&
          passiveInteraction.isValid() && publisherParameter.isValid() &&
          activeParameter.isValid() && passiveParameter.isValid(),
      "Active/passive interaction lookup returned an invalid standard handle");

  publisher.rtiAmbassador().publishInteractionClass(publisherInteraction);
  activeSubscriber.rtiAmbassador().subscribeInteractionClass(
      activeInteraction,
      true);
  passiveSubscriber.rtiAmbassador().subscribeInteractionClass(
      passiveInteraction,
      false);

  std::vector<std::uint8_t> const value{0x41U, 0x50U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      publisherParameter,
      rti::VariableLengthData(value.data(), value.size()));
  publisher.rtiAmbassador().sendInteraction(
      publisherInteraction,
      parameters,
      rti::VariableLengthData{});

  if (model == rti::HLA_EVOKED) {
    require(
        activeSubscriber.recorder().interactions().empty() &&
            passiveSubscriber.recorder().interactions().empty(),
        "Active/passive interaction callbacks arrived before evoked callback servicing");
  }
  waitFor(
      activeSubscriber,
      [&] { return activeSubscriber.recorder().interactions().size() >= 1U; },
      options,
      "active ordinary interaction delivery");
  auto assertDelivery = [&](Session& receiver,
                            rti::InteractionClassHandle const& expectedInteraction,
                            rti::ParameterHandle const& expectedParameter,
                            std::string const& description) {
    auto const received = receiver.recorder().interactions();
    require(
        received.size() == 1U && received.front().present &&
            received.front().interaction == expectedInteraction &&
            received.front().parameters.size() == 1U &&
            received.front().parameters.count(expectedParameter) == 1U &&
            copyBytes(received.front().parameters.at(expectedParameter)) == value,
        description + " received the wrong class, parameter, or parameter value");
  };
  assertDelivery(
      activeSubscriber,
      activeInteraction,
      activeParameter,
      "active interaction subscriber");
  require(
      publisher.recorder().interactions().empty(),
      "Ordinary interaction callback looped back to its publisher");

  passiveSubscriber.rtiAmbassador().unsubscribeInteractionClass(
      passiveInteraction);
  activeSubscriber.rtiAmbassador().unsubscribeInteractionClass(activeInteraction);
  passiveSubscriber.resign(rti::NO_ACTION);
  activeSubscriber.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().unpublishInteractionClass(publisherInteraction);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  passiveSubscriber.disconnect();
  activeSubscriber.disconnect();
  publisher.disconnect();
}

void scenarioInteractionActiveAndPassiveSubscriptionModesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioInteractionActiveAndPassiveSubscriptionModes(options, model);
}

void scenarioReceiveOrderObjectRemoval(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Receive-order object-removal testing requires an adapter-supplied FOM");

  Session publisher(options, model, "receive-order-object-removal-publisher");
  Session receiver(options, model, "receive-order-object-removal-receiver");
  auto const federation = federationName(
      options,
      "receive-order-object-removal");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Receive-order object-removal lookup returned an invalid standard handle");
  require(
      publisher.rtiAmbassador().getObjectClassName(publisherClass) ==
              options.objectClassName &&
          receiver.rtiAmbassador().getObjectClassName(receiverClass) ==
              options.objectClassName &&
          publisher.rtiAmbassador().getAttributeName(
              publisherClass,
              publisherAttribute) == options.attributeName &&
          receiver.rtiAmbassador().getAttributeName(
              receiverClass,
              receiverAttribute) == options.attributeName,
      "Receive-order object-removal lookup did not round-trip adapter names");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");

  auto const object = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      object.isValid(),
      "Receive-order object-removal registration returned an invalid object handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(object); },
      options,
      "receive-order object-removal discovery");

  std::vector<std::uint8_t> const removalTag{0x44U, 0x31U};
  rti::VariableLengthData tag(removalTag.data(), removalTag.size());
  publisher.rtiAmbassador().deleteObjectInstance(object, tag);

  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().removals().empty(),
        "Receive-order object-removal delivered before callback servicing");
  }
  waitFor(
      receiver,
      [&] { return receiver.recorder().removals().size() >= 1U; },
      options,
      "receive-order object-removal callback");

  auto const removals = receiver.recorder().removals();
  require(
      removals.size() == 1U,
      "Receive-order object-removal delivered an unexpected callback count");
  require(
      removals.front().object == object &&
          removals.front().tag == removalTag &&
          removals.front().producer == publisher.federateHandle(),
      "Receive-order object-removal returned the wrong object, tag, or producer");
  require(
      publisher.recorder().removals().empty(),
      "Receive-order object-removal looped a callback back to its publisher");

  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioReceiveOrderObjectRemovalContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioReceiveOrderObjectRemoval(options, model);
}

void scenarioObjectRemovalOrderClassification(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.logicalTimeImplementationName.empty(),
      "Object-removal order classification requires adapter-supplied FOM and logical-time implementation");

  Session publisher(options, model, "object-removal-order-classification-publisher");
  Session receiver(options, model, "object-removal-order-classification-receiver");
  auto const federation =
      federationName(options, "object-removal-order-classification");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass, options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass, options.attributeName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid(),
      "Object-removal order classification lookup returned an invalid standard handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass, publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass, receiverAttributes, true, L"");

  auto const receiveOrderObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      receiveOrderObject.isValid(),
      "Receive-order object-removal registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(receiveOrderObject); },
      options,
      "receive-order object discovery in object-removal classification");

  auto const timestampedObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      timestampedObject.isValid(),
      "Timestamped object-removal registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(timestampedObject); },
      options,
      "timestamped object discovery in object-removal classification");
  auto const timeAdvanceReleasedObject =
      publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      timeAdvanceReleasedObject.isValid(),
      "Time-advance-released object-removal registration returned an invalid handle");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(timeAdvanceReleasedObject); },
      options,
      "time-advance-released object discovery in object-removal classification");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "Object-removal order classification");

  std::vector<std::uint8_t> const receiveOrderTag{0x52U, 0x4FU};
  rti::VariableLengthData receiveOrderUserTag(
      receiveOrderTag.data(), receiveOrderTag.size());
  publisher.rtiAmbassador().deleteObjectInstance(
      receiveOrderObject, receiveOrderUserTag);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().removals().empty(),
        "receive-order removal callback arrived before callback servicing");
  }
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().removals().empty() &&
          receiver.recorder().timedRemovals().empty(),
      "time-constrained receive-order removal bypassed disabled asynchronous delivery");
  receiver.rtiAmbassador().enableAsynchronousDelivery();
  waitFor(
      receiver,
      [&] { return receiver.recorder().removals().size() >= 1U; },
      options,
      "asynchronous release of receive-order object removal");
  auto const receiveOrderRemovals = receiver.recorder().removals();
  require(
      receiveOrderRemovals.size() == 1U &&
          receiver.recorder().timedRemovals().empty(),
      "ordinary Delete Object Instance did not use only the receive-order callback path after asynchronous release");
  require(
      receiveOrderRemovals.front().object == receiveOrderObject &&
          receiveOrderRemovals.front().tag == receiveOrderTag &&
          receiveOrderRemovals.front().producer == publisher.federateHandle(),
      "receive-order removal returned incorrect object, tag, or producer");

  receiver.rtiAmbassador().disableAsynchronousDelivery();
  std::vector<std::uint8_t> const timeAdvanceReleasedTag{0x54U, 0x41U};
  rti::VariableLengthData timeAdvanceReleasedUserTag(
      timeAdvanceReleasedTag.data(), timeAdvanceReleasedTag.size());
  publisher.rtiAmbassador().deleteObjectInstance(
      timeAdvanceReleasedObject, timeAdvanceReleasedUserTag);
  if (model == rti::HLA_EVOKED) {
    require(
        receiver.recorder().removals().size() == 1U,
        "second receive-order removal arrived before callback servicing while asynchronous delivery was disabled");
  }
  for (int pass = 0; pass != 8; ++pass) {
    receiver.pump();
  }
  require(
      receiver.recorder().removals().size() == 1U &&
          receiver.recorder().timedRemovals().empty(),
      "disabling asynchronous delivery did not retain receive-order object removal");

  auto const timestamp = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  std::vector<std::uint8_t> const timestampedTag{0x54U, 0x53U};
  rti::VariableLengthData timestampedUserTag(
      timestampedTag.data(), timestampedTag.size());
  auto const retraction = publisher.rtiAmbassador().deleteObjectInstance(
      timestampedObject,
      timestampedUserTag,
      *timestamp);
  require(
      retraction.isValid(),
      "Timestamped Delete Object Instance returned an invalid retraction handle");
  require(
      receiver.recorder().removals().size() == 1U &&
          receiver.recorder().timedRemovals().empty(),
      "timestamped Delete Object Instance was delivered before the receiver advance");

  receiver.recorder().clearCallbackOrder();
  publisher.rtiAmbassador().timeAdvanceRequest(*timestamp);
  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  waitFor(
      publisher,
      receiver,
      [&] {
        return publisher.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().removals().size() >= 2U &&
            receiver.recorder().timedRemovals().size() >= 1U;
      },
      options,
      "timestamped removal and grants in object-removal classification");

  auto const timedRemovals = receiver.recorder().timedRemovals();
  require(
      receiver.recorder().removals().size() == 2U &&
          timedRemovals.size() == 1U,
      "time advance did not release the held receive-order removal and timestamped removal");
  auto const receiveOrderRemovalsAfterAdvance = receiver.recorder().removals();
  require(
      receiveOrderRemovalsAfterAdvance.back().object == timeAdvanceReleasedObject &&
          receiveOrderRemovalsAfterAdvance.back().tag == timeAdvanceReleasedTag &&
          receiveOrderRemovalsAfterAdvance.back().producer == publisher.federateHandle(),
      "time-advance-released removal returned incorrect object, tag, or producer");
  auto const& timedRemoval = timedRemovals.front();
  require(
      timedRemoval.object == timestampedObject &&
          timedRemoval.tag == timestampedTag &&
          timedRemoval.producer == publisher.federateHandle(),
      "timestamped removal returned incorrect object, tag, or producer");
  require(
      timedRemoval.time == encodeTime(*timestamp) && !timedRemoval.timeText.empty() &&
          timedRemoval.sentOrder == rti::TIMESTAMP &&
          timedRemoval.receivedOrder == rti::TIMESTAMP &&
          timedRemoval.retractionPresent &&
          timedRemoval.retraction == copyBytes(retraction.encode()),
      "timestamped removal returned incorrect time, order, or retraction metadata");
  require(
      receiver.recorder().callbackOrder() ==
          std::vector<std::string>{"remove", "grant"},
      "timestamped removal was not delivered before its matching grant");
  auto const publisherGrants = publisher.recorder().timeAdvanceGrants();
  auto const receiverGrants = receiver.recorder().timeAdvanceGrants();
  require(
      publisherGrants.size() == 1U && receiverGrants.size() == 1U &&
          publisherGrants.front().encoded == encodeTime(*timestamp) &&
          receiverGrants.front().encoded == encodeTime(*receiverTarget),
      "object-removal classification grants did not report their requested times");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass, receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass, publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::NO_ACTION);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioObjectRemovalOrderClassificationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioObjectRemovalOrderClassification(options, model);
}

void scenarioFederationRestoreAbort(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federation restore-abort testing requires an adapter-supplied FOM");

  Session owner(options, model, "federation-restore-abort-owner");
  Session peer(options, model, "federation-restore-abort-peer");
  auto const federation = federationName(
      options,
      "federation-restore-abort");
  connectAndJoin(owner, peer, options, federation, options.fom);

  std::wstring const saveLabel = L"tck-restore-abort-save";
  auto const ownerSaveBefore = owner.federateSaveInitiations().size();
  auto const peerSaveBefore = peer.federateSaveInitiations().size();
  owner.rtiAmbassador().requestFederationSave(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federateSaveInitiations().size() > ownerSaveBefore &&
            peer.federateSaveInitiations().size() > peerSaveBefore;
      },
      options,
      "restore-abort save initiation callbacks");
  require(
      owner.federateSaveInitiations().back() == saveLabel &&
          peer.federateSaveInitiations().back() == saveLabel,
      "restore-abort save initiation returned the wrong label");

  owner.rtiAmbassador().federateSaveBegun();
  peer.rtiAmbassador().federateSaveBegun();
  auto const ownerSavedBefore = owner.federationSavedCount();
  auto const peerSavedBefore = peer.federationSavedCount();
  owner.rtiAmbassador().federateSaveComplete();
  peer.rtiAmbassador().federateSaveComplete();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationSavedCount() > ownerSavedBefore &&
            peer.federationSavedCount() > peerSavedBefore;
      },
      options,
      "restore-abort save completion callbacks");
  require(
      owner.federationNotSavedReasons().empty() &&
          peer.federationNotSavedReasons().empty(),
      "restore-abort setup reported a save failure");

  auto const ownerRestoreBefore = owner.federateRestoreInitiations().size();
  auto const peerRestoreBefore = peer.federateRestoreInitiations().size();
  auto const ownerRestoreBegunBefore = owner.federationRestoreBegunCount();
  auto const peerRestoreBegunBefore = peer.federationRestoreBegunCount();
  owner.rtiAmbassador().requestFederationRestore(saveLabel);
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationRestoreRequestsSucceeded().size() >= 1U &&
            owner.federationRestoreBegunCount() > ownerRestoreBegunBefore &&
            peer.federationRestoreBegunCount() > peerRestoreBegunBefore &&
            owner.federateRestoreInitiations().size() > ownerRestoreBefore &&
            peer.federateRestoreInitiations().size() > peerRestoreBefore;
      },
      options,
      "restore-abort restore initiation callbacks");
  require(
      owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty() &&
          peer.federationRestoreRequestsFailed().empty(),
      "restore-abort restore request returned an unexpected result");

  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const peerRestore = peer.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && peerRestore.label == saveLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          peerRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          peerRestore.postRestoreFederateHandle.isValid(),
      "restore-abort initiation returned incomplete restore metadata");

  auto const ownerNotRestoredBefore = owner.federationNotRestoredReasons().size();
  auto const peerNotRestoredBefore = peer.federationNotRestoredReasons().size();
  auto const ownerRestoredBefore = owner.federationRestoredCount();
  auto const peerRestoredBefore = peer.federationRestoredCount();
  owner.rtiAmbassador().abortFederationRestore();
  waitForSessions(
      {&owner, &peer},
      [&] {
        return owner.federationNotRestoredReasons().size() > ownerNotRestoredBefore &&
            peer.federationNotRestoredReasons().size() > peerNotRestoredBefore;
      },
      options,
      "restore-abort failure callbacks");
  require(
      owner.federationNotRestoredReasons().back() == rti::RESTORE_ABORTED &&
          peer.federationNotRestoredReasons().back() == rti::RESTORE_ABORTED &&
          owner.federationRestoredCount() == ownerRestoredBefore &&
          peer.federationRestoredCount() == peerRestoredBefore,
      "abortFederationRestore returned the wrong terminal callback result");

  auto const statusBefore = owner.federationRestoreStatusResponses().size();
  owner.rtiAmbassador().queryFederationRestoreStatus();
  waitFor(
      owner,
      [&] {
        return owner.federationRestoreStatusResponses().size() > statusBefore;
      },
      options,
      "restore-abort terminal restore status");
  auto const status = owner.federationRestoreStatusResponses().back();
  require(
      status.size() == 2U,
      "restore-abort terminal status omitted a joined federate");
  for (auto const& entry : status) {
    require(
        entry.status == rti::NO_RESTORE_IN_PROGRESS,
        "restore-abort left a restore in progress");
  }

  peer.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  peer.disconnect();
  owner.disconnect();
}

void scenarioFederationRestoreAbortContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationRestoreAbort(options, model);
}

void scenarioFederationRestoreOwnershipAssumption(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Federation restore ownership-assumption testing requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session candidate(options, model, "member");
  auto const federation = federationName(
      options,
      "federation-restore-work-item-ownership-assumption");
  connectAndJoin(owner, candidate, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const candidateClass = candidate.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const candidateAttribute = candidate.rtiAmbassador().getAttributeHandle(
      candidateClass,
      options.attributeName);
  require(
      ownerClass.isValid() && candidateClass.isValid() &&
          ownerAttribute.isValid() && candidateAttribute.isValid(),
      "restore ownership-assumption lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const candidateAttributes{candidateAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  candidate.rtiAmbassador().subscribeObjectClassAttributes(
      candidateClass,
      candidateAttributes,
      true,
      L"");
  candidate.rtiAmbassador().publishObjectClassAttributes(
      candidateClass,
      candidateAttributes);

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "restore ownership-assumption registration returned an invalid object handle");
  if (model == rti::HLA_IMMEDIATE) {
    waitFor(
        candidate,
        [&] { return candidate.recorder().hasDiscovery(object); },
        options,
        "restore ownership-assumption object discovery");
  }

  std::vector<std::uint8_t> const assumptionTagBytes{
      0xD4U, 0x31U, 0x7BU, 0x0AU};
  rti::VariableLengthData const assumptionTag(
      assumptionTagBytes.data(),
      assumptionTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  candidate.recorder().clearOwnershipRecords();
  owner.recorder().clearCallbackOrder();
  candidate.recorder().clearCallbackOrder();

  if (model == rti::HLA_IMMEDIATE) {
    owner.rtiAmbassador().disableCallbacks();
    candidate.rtiAmbassador().disableCallbacks();
  }
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      assumptionTag);
  if (model == rti::HLA_EVOKED) {
    // A standard public service call admits pending pushed frames to the
    // HLA_EVOKED callback queue without invoking them.  Keep that queue
    // untouched through save and restore so the test proves preservation of
    // an unconsumed discovery/ownership-assumption work item.
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value(),
        "restore ownership-assumption work was delivered before callback servicing");
  }
  require(
      !candidate.recorder().ownershipAssumption().has_value(),
      "restore ownership-assumption work was delivered before the save");

  std::wstring const saveLabel =
      L"tck-federation-restore-work-item-ownership-assumption-save";
  auto serviceCallbacks = [&] {
    if (model == rti::HLA_EVOKED) {
      owner.pump();
      candidate.pump();
      return;
    }
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
  };
  auto waitForCallbacks = [&](auto predicate, std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      serviceCallbacks();
      if (predicate()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("Timed out waiting for " + description);
  };
  auto waitForOwnerCallbacks = [&](auto predicate, std::string const& description) {
    auto const deadline = Clock::now() +
        std::chrono::milliseconds(options.timeoutMilliseconds);
    while (Clock::now() < deadline) {
      owner.pump();
      if (predicate()) {
        return;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    throw std::runtime_error("Timed out waiting for " + description);
  };

  if (model == rti::HLA_EVOKED) {
    owner.rtiAmbassador().requestFederationSave(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    waitForOwnerCallbacks(
        [&] {
          return owner.federateSaveInitiations().size() >= 1U;
        },
        "restore ownership-assumption owner save initiation callback");
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty(),
        "restore ownership-assumption candidate callback queue was serviced during save");
    owner.rtiAmbassador().federateSaveBegun();
    candidate.rtiAmbassador().federateSaveBegun();
    owner.rtiAmbassador().federateSaveComplete();
    candidate.rtiAmbassador().federateSaveComplete();
    waitForOwnerCallbacks(
        [&] {
          return owner.federationSavedCount() >= 1U;
        },
        "restore ownership-assumption owner save completion callback");
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty() &&
            candidate.federationSavedCount() == 0U,
        "restore ownership-assumption candidate callback queue was serviced before restore");

    owner.rtiAmbassador().requestFederationRestore(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    owner.rtiAmbassador().federateRestoreComplete();
    candidate.rtiAmbassador().federateRestoreComplete();
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    static_cast<void>(candidate.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        !candidate.recorder().hasDiscovery(object) &&
            !candidate.recorder().ownershipAssumption().has_value() &&
            candidate.federateSaveInitiations().empty() &&
            candidate.federationSavedCount() == 0U &&
            candidate.federationRestoreBegunCount() == 0U &&
            candidate.federateRestoreInitiations().empty() &&
            candidate.federationRestoredCount() == 0U,
        "restore ownership-assumption callback queue was serviced before federation restore completion");
    waitForOwnerCallbacks(
        [&] {
          return owner.federationRestoreRequestsSucceeded().size() >= 1U &&
              owner.federationRestoreBegunCount() >= 1U &&
              owner.federateRestoreInitiations().size() >= 1U &&
              owner.federationRestoredCount() >= 1U;
        },
        "restore ownership-assumption owner restore callbacks");

    auto waitForCandidateCallbacks = [&](auto predicate,
                                         std::string const& description) {
      auto const deadline = Clock::now() +
          std::chrono::milliseconds(options.timeoutMilliseconds);
      while (Clock::now() < deadline) {
        candidate.pump();
        if (predicate()) {
          return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      throw std::runtime_error("Timed out waiting for " + description);
    };
    waitForCandidateCallbacks(
        [&] { return candidate.recorder().hasDiscovery(object); },
        "restore ownership-assumption discovery callback");
    waitForCandidateCallbacks(
        [&] { return candidate.recorder().ownershipAssumption().has_value(); },
        "restore ownership-assumption callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federateSaveInitiations().size() >= 1U; },
        "restore ownership-assumption save initiation callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationSavedCount() >= 1U; },
        "restore ownership-assumption save completion callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationRestoreBegunCount() >= 1U; },
        "restore ownership-assumption restore-begun callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federateRestoreInitiations().size() >= 1U; },
        "restore ownership-assumption restore initiation callback");
    waitForCandidateCallbacks(
        [&] { return candidate.federationRestoredCount() >= 1U; },
        "restore ownership-assumption federation-restored callback");
  } else {
    owner.rtiAmbassador().requestFederationSave(saveLabel);
    owner.rtiAmbassador().federateSaveBegun();
    candidate.rtiAmbassador().federateSaveBegun();
    owner.rtiAmbassador().federateSaveComplete();
    candidate.rtiAmbassador().federateSaveComplete();
    require(
        owner.federationSavedCount() == 0U &&
            candidate.federationSavedCount() == 0U,
        "disabled callbacks exposed save completion");

    owner.rtiAmbassador().requestFederationRestore(saveLabel);
    static_cast<void>(owner.rtiAmbassador().getObjectClassHandle(
        options.objectClassName));
    require(
        owner.federationRestoredCount() == 0U &&
            candidate.federationRestoredCount() == 0U &&
            !candidate.recorder().ownershipAssumption().has_value(),
        "disabled callbacks exposed restore completion or ownership work");
    owner.rtiAmbassador().federateRestoreComplete();
    candidate.rtiAmbassador().federateRestoreComplete();
    owner.rtiAmbassador().enableCallbacks();
    candidate.rtiAmbassador().enableCallbacks();
    waitForCallbacks(
        [&] {
          return owner.federationRestoredCount() >= 1U &&
              candidate.federationRestoredCount() >= 1U &&
              candidate.recorder().ownershipAssumption().has_value();
        },
        "restored ownership-assumption immediate callbacks");
  }
  require(
      owner.federationRestoreRequestsSucceeded().size() >= 1U &&
          owner.federationRestoreRequestsSucceeded().back() == saveLabel &&
          owner.federationRestoreRequestsFailed().empty() &&
          candidate.federationRestoreRequestsFailed().empty(),
      "restore ownership-assumption request returned an unexpected result");
  auto const ownerRestore = owner.federateRestoreInitiations().back();
  auto const candidateRestore = candidate.federateRestoreInitiations().back();
  require(
      ownerRestore.label == saveLabel && candidateRestore.label == saveLabel &&
          ownerRestore.federateName == options.ownerFederateName &&
          candidateRestore.federateName == options.memberFederateName &&
          ownerRestore.postRestoreFederateHandle.isValid() &&
          candidateRestore.postRestoreFederateHandle.isValid(),
      "restore ownership-assumption initiation returned incomplete metadata");
  require(
      owner.federationNotRestoredReasons().empty() &&
          candidate.federationNotRestoredReasons().empty(),
      "restore ownership-assumption restore reported a failure");

  auto const assumption = candidate.recorder().ownershipAssumption();
  require(
      assumption->object == object &&
          assumption->attributes == candidateAttributes &&
          assumption->tag == assumptionTagBytes,
      "restored ownership-assumption callback returned the wrong metadata");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !candidate.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              candidateAttribute),
      "restored ownership-assumption work transferred ownership prematurely");
  require(
      owner.federationRestoredCount() >= 1U &&
          candidate.federationRestoredCount() >= 1U,
      "restored ownership-assumption work did not complete federation restore first");

  candidate.resign(rti::NO_ACTION);
  owner.resign(rti::NO_ACTION);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  candidate.disconnect();
  owner.disconnect();
}

void scenarioFederationRestoreOwnershipAssumptionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationRestoreOwnershipAssumption(options, model);
}

void scenarioOwnershipAcquisitionIfAvailable(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "If Available ownership-acquisition testing requires an adapter-supplied FOM");

  Session owner(options, model, "owner");
  Session requester(options, model, "member");
  auto const federation = federationName(
      options,
      "ownership-acquisition-if-available");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "If Available ownership-acquisition lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const unavailableObject = owner.rtiAmbassador().registerObjectInstance(
      ownerClass);
  auto const availableObject = owner.rtiAmbassador().registerObjectInstance(
      ownerClass);
  require(
      unavailableObject.isValid() && availableObject.isValid(),
      "If Available ownership-acquisition registration returned an invalid object handle");
  waitFor(
      requester,
      [&] {
        return requester.recorder().hasDiscovery(unavailableObject) &&
            requester.recorder().hasDiscovery(availableObject);
      },
      options,
      "If Available ownership-acquisition object discovery");

  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(
          unavailableObject,
          ownerAttribute) &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              requesterAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const unavailableTagBytes{
      0x51U,
      0xA7U,
      0x0CU};
  rti::VariableLengthData const unavailableTag(
      unavailableTagBytes.data(),
      unavailableTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      unavailableObject,
      requesterAttributes,
      unavailableTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options,
      "If Available ownership-acquisition unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == unavailableObject &&
          unavailable->attributes == requesterAttributes &&
          unavailable->tag == unavailableTagBytes,
      "If Available ownership-acquisition unavailable callback returned the wrong metadata");
  require(
      !requester.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              unavailableObject,
              requesterAttribute),
      "If Available ownership-acquisition unavailable changed ownership");

  std::vector<std::uint8_t> const acquisitionTagBytes{
      0x3CU,
      0xA1U,
      0x7EU};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  requester.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      availableObject,
      ownerAttributes,
      rti::VariableLengthData{});
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(
          availableObject,
          ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition divestiture transferred ownership before the request");

  requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      availableObject,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipAcquisition().has_value(); },
      options,
      "If Available ownership-acquisition notification");
  auto const acquisition = requester.recorder().ownershipAcquisition();
  require(
      acquisition->object == availableObject &&
          acquisition->attributes == requesterAttributes &&
          acquisition->tag == acquisitionTagBytes,
      "If Available ownership-acquisition notification returned the wrong metadata");
  require(
      !requester.recorder().ownershipUnavailable().has_value() &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              ownerAttribute) &&
          requester.rtiAmbassador().isAttributeOwnedByFederate(
              availableObject,
              requesterAttribute),
      "If Available ownership-acquisition notification did not establish ownership");
  requireException(
      [&] {
        requester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
            availableObject,
            requesterAttributes,
            acquisitionTag);
      },
      L"FederateOwnsAttributes",
      "repeating an If Available ownership-acquisition request after ownership transfer");

  requester.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      availableObject,
      requesterAttributes,
      rti::VariableLengthData{});
  require(
      !requester.rtiAmbassador().isAttributeOwnedByFederate(
          availableObject,
          requesterAttribute),
      "If Available ownership-acquisition cleanup did not release the transferred attribute");
  requester.resign(rti::DELETE_OBJECTS);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioOwnershipAcquisitionIfAvailableContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipAcquisitionIfAvailable(options, model);
}

void scenarioAttributeOwnershipAcquisitionCancellation(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Attribute ownership-acquisition cancellation testing requires an adapter-supplied model FOM");

  Session owner(options, model, "acquisition-cancellation-owner");
  Session requester(options, model, "acquisition-cancellation-requester");
  auto const federation = federationName(
      options,
      "attribute-ownership-acquisition-cancellation");
  connectAndJoin(owner, requester, options, federation, options.modelFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedIdentityAttributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.typedIdentityAttributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Attribute ownership-acquisition cancellation lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");
  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Attribute ownership-acquisition cancellation registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "attribute ownership-acquisition cancellation discovery");
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);

  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();
  std::vector<std::uint8_t> const acquisitionTagBytes{0xC0U, 0xDEU, 0x25U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());

  requireException(
      [&] {
        requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            requesterAttributes);
      },
      L"AttributeAcquisitionWasNotRequested",
      "canceling an ownership acquisition before requesting it");
  requireException(
      [&] {
        owner.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            ownerAttributes);
      },
      L"AttributeAlreadyOwned",
      "canceling an ownership acquisition for an owned attribute");

  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
      object,
      requesterAttributes);
  waitFor(
      requester,
      [&] {
        return requester.recorder().ownershipAcquisitionCancellation().has_value();
      },
      options,
      "attribute ownership-acquisition cancellation confirmation");
  auto const cancellation = requester.recorder().ownershipAcquisitionCancellation();
  require(
      cancellation->object == object &&
          cancellation->attributes == requesterAttributes &&
          cancellation->tag.empty(),
      "attribute ownership-acquisition cancellation returned the wrong standard metadata");
  require(
      !requester.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "attribute ownership-acquisition cancellation changed ownership or delivered acquisition");
  requireException(
      [&] {
        requester.rtiAmbassador().cancelAttributeOwnershipAcquisition(
            object,
            requesterAttributes);
      },
      L"AttributeAcquisitionWasNotRequested",
      "repeating an ownership-acquisition cancellation after confirmation");

  requester.rtiAmbassador().unpublishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.resign(rti::NO_ACTION);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAttributeOwnershipAcquisitionCancellationContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipAcquisitionCancellation(options, model);
}

void scenarioNegotiatedDivestitureConfirmationFlow(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Negotiated-divestiture testing requires an adapter-supplied FOM");

  Session owner(options, model, "negotiated-divestiture-owner");
  Session acquirer(options, model, "negotiated-divestiture-acquirer");
  auto const federation = federationName(
      options,
      "negotiated-divestiture-confirmation-flow");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const acquirerClass = acquirer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const acquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      acquirerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && acquirerClass.isValid() &&
          ownerAttribute.isValid() && acquirerAttribute.isValid(),
      "Negotiated-divestiture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass,
      acquirerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Negotiated-divestiture registration returned an invalid object handle");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "negotiated-divestiture object discovery");

  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "Negotiated-divestiture setup did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const divestitureTagBytes{0x10U, 0xD1U};
  std::vector<std::uint8_t> const acquisitionRequestTagBytes{0x20U, 0xA2U};
  std::vector<std::uint8_t> const confirmationTagBytes{0x30U, 0xC3U};
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  rti::VariableLengthData const acquisitionRequestTag(
      acquisitionRequestTagBytes.data(),
      acquisitionRequestTagBytes.size());
  rti::VariableLengthData const confirmationTag(
      confirmationTagBytes.data(),
      confirmationTagBytes.size());

  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().negotiatedAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      divestitureTag);
  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      acquirerAttributes,
      acquisitionRequestTag);

  waitFor(
      owner,
      [&] { return owner.recorder().divestitureConfirmation().has_value(); },
      options,
      "negotiated-divestiture confirmation request");
  auto const confirmationRequest = owner.recorder().divestitureConfirmation();
  require(
      confirmationRequest->object == object &&
          confirmationRequest->attributes == ownerAttributes &&
          confirmationRequest->tag == acquisitionRequestTagBytes,
      "Negotiated-divestiture confirmation request returned the wrong object, attributes, or acquisition tag");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute) &&
          !acquirer.recorder().ownershipAcquisition().has_value(),
      "Negotiated divestiture transferred ownership before confirmation");

  owner.rtiAmbassador().confirmDivestiture(
      object,
      ownerAttributes,
      confirmationTag);
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "negotiated-divestiture acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == confirmationTagBytes,
      "Negotiated-divestiture acquisition notification returned the wrong object, attributes, or confirmation tag");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "Confirm Divestiture did not transfer ownership to the acquirer");

  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      acquirerAttributes,
      rti::VariableLengthData{});
  acquirer.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}

void scenarioNegotiatedDivestitureConfirmationFlowContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioNegotiatedDivestitureConfirmationFlow(options, model);
}

void scenarioAttributeOwnershipReleaseDenied(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Ownership-release-denial testing requires an adapter-supplied FOM");

  Session owner(options, model, "ownership-release-denial-owner");
  Session requester(options, model, "ownership-release-denial-requester");
  auto const federation = federationName(
      options,
      "attribute-ownership-release-denied");
  connectAndJoin(owner, requester, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const requesterClass = requester.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const requesterAttribute = requester.rtiAmbassador().getAttributeHandle(
      requesterClass,
      options.attributeName);
  require(
      ownerClass.isValid() && requesterClass.isValid() &&
          ownerAttribute.isValid() && requesterAttribute.isValid(),
      "Ownership-release-denial lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const requesterAttributes{requesterAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  requester.rtiAmbassador().publishObjectClassAttributes(
      requesterClass,
      requesterAttributes);
  requester.rtiAmbassador().subscribeObjectClassAttributes(
      requesterClass,
      requesterAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Ownership-release-denial registration returned an invalid object handle");
  waitFor(
      requester,
      [&] { return requester.recorder().hasDiscovery(object); },
      options,
      "ownership-release-denial object discovery");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "Ownership-release-denial setup did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xA5U, 0x01U};
  std::vector<std::uint8_t> const denialTagBytes{0xD3U, 0x02U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const denialTag(
      denialTagBytes.data(),
      denialTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  requester.recorder().clearOwnershipRecords();

  requester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      requesterAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "ownership-release-denial release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object &&
          releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "Ownership-release request returned the wrong object, attributes, or acquisition tag");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute) &&
          !requester.recorder().ownershipAcquisition().has_value(),
      "A pending ownership release request changed ownership before denial");

  owner.rtiAmbassador().attributeOwnershipReleaseDenied(
      object,
      ownerAttributes,
      denialTag);
  waitFor(
      requester,
      [&] { return requester.recorder().ownershipUnavailable().has_value(); },
      options,
      "ownership-release-denial unavailable callback");
  auto const unavailable = requester.recorder().ownershipUnavailable();
  require(
      unavailable->object == object &&
          unavailable->attributes == requesterAttributes &&
          unavailable->tag == denialTagBytes,
      "Ownership-unavailable callback returned the wrong object, attributes, or denial tag");
  require(
      !requester.recorder().ownershipAcquisition().has_value() &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !requester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              requesterAttribute),
      "Ownership-release denial transferred ownership or delivered an acquisition");

  requester.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  requester.disconnect();
  owner.disconnect();
}

void scenarioAttributeOwnershipReleaseDeniedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipReleaseDenied(options, model);
}

void scenarioAttributeOwnershipDivestitureIfWanted(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "If Wanted divestiture testing requires an adapter-supplied FOM");

  Session owner(options, model, "if-wanted-divestiture-owner");
  Session acquirer(options, model, "if-wanted-divestiture-acquirer");
  auto const federation = federationName(
      options,
      "attribute-ownership-divestiture-if-wanted");
  connectAndJoin(owner, acquirer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const acquirerClass = acquirer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const acquirerAttribute = acquirer.rtiAmbassador().getAttributeHandle(
      acquirerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && acquirerClass.isValid() &&
          ownerAttribute.isValid() && acquirerAttribute.isValid(),
      "If Wanted divestiture lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const acquirerAttributes{acquirerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  acquirer.rtiAmbassador().publishObjectClassAttributes(
      acquirerClass,
      acquirerAttributes);
  acquirer.rtiAmbassador().subscribeObjectClassAttributes(
      acquirerClass,
      acquirerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "If Wanted divestiture registration returned an invalid object handle");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().hasDiscovery(object); },
      options,
      "If Wanted divestiture object discovery");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "If Wanted divestiture setup did not establish the standard initial ownership state");

  std::vector<std::uint8_t> const acquisitionTagBytes{0xA4U, 0x61U};
  std::vector<std::uint8_t> const divestitureTagBytes{0xD4U, 0x62U};
  rti::VariableLengthData const acquisitionTag(
      acquisitionTagBytes.data(),
      acquisitionTagBytes.size());
  rti::VariableLengthData const divestitureTag(
      divestitureTagBytes.data(),
      divestitureTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();

  acquirer.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      acquirerAttributes,
      acquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipReleaseRequest().has_value(); },
      options,
      "If Wanted divestiture release request");
  auto const releaseRequest = owner.recorder().ownershipReleaseRequest();
  require(
      releaseRequest->object == object &&
          releaseRequest->attributes == ownerAttributes &&
          releaseRequest->tag == acquisitionTagBytes,
      "If Wanted divestiture release request returned the wrong metadata");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "A pending If Wanted acquisition changed ownership before divestiture");

  rti::AttributeHandleSet divestedAttributes;
  owner.rtiAmbassador().attributeOwnershipDivestitureIfWanted(
      object,
      ownerAttributes,
      divestitureTag,
      divestedAttributes);
  require(
      divestedAttributes == ownerAttributes,
      "If Wanted divestiture did not return the exact transferred attribute set");
  waitFor(
      acquirer,
      [&] { return acquirer.recorder().ownershipAcquisition().has_value(); },
      options,
      "If Wanted divestiture acquisition notification");
  auto const acquisition = acquirer.recorder().ownershipAcquisition();
  require(
      acquisition->object == object &&
          acquisition->attributes == acquirerAttributes &&
          acquisition->tag == divestitureTagBytes,
      "If Wanted acquisition notification returned the wrong metadata or divestiture tag");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          acquirer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              acquirerAttribute),
      "If Wanted divestiture did not transfer ownership to the pending acquirer");

  std::vector<std::uint8_t> const cleanupDivestitureTagBytes{0xC1U, 0x01U};
  std::vector<std::uint8_t> const cleanupAcquisitionTagBytes{0xC2U, 0x02U};
  rti::VariableLengthData const cleanupDivestitureTag(
      cleanupDivestitureTagBytes.data(),
      cleanupDivestitureTagBytes.size());
  rti::VariableLengthData const cleanupAcquisitionTag(
      cleanupAcquisitionTagBytes.data(),
      cleanupAcquisitionTagBytes.size());
  owner.recorder().clearOwnershipRecords();
  acquirer.recorder().clearOwnershipRecords();
  acquirer.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      acquirerAttributes,
      cleanupDivestitureTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipAssumption().has_value(); },
      options,
      "If Wanted divestiture cleanup ownership-assumption offer");
  owner.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      ownerAttributes,
      cleanupAcquisitionTag);
  waitFor(
      owner,
      [&] { return owner.recorder().ownershipAcquisition().has_value(); },
      options,
      "If Wanted divestiture cleanup ownership reacquisition");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute),
      "If Wanted divestiture cleanup did not restore owner ownership");

  acquirer.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.resign(rti::CANCEL_THEN_DELETE_THEN_DIVEST);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  acquirer.disconnect();
  owner.disconnect();
}

void scenarioAttributeOwnershipDivestitureIfWantedContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipDivestitureIfWanted(options, model);
}

void scenarioAttributeOwnershipQueryOwnerReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty(),
      "Attribute-ownership query testing requires an adapter-supplied FOM");

  Session owner(options, model, "ownership-query-owner");
  Session observer(options, model, "ownership-query-observer");
  auto const federation = federationName(
      options,
      "attribute-ownership-query-owner-report");
  connectAndJoin(owner, observer, options, federation, options.fom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const ownerAttribute = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.attributeName);
  auto const observerAttribute = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.attributeName);
  require(
      ownerClass.isValid() && observerClass.isValid() &&
          ownerAttribute.isValid() && observerAttribute.isValid(),
      "Attribute-ownership query lookup returned an invalid standard handle");

  rti::AttributeHandleSet const ownerAttributes{ownerAttribute};
  rti::AttributeHandleSet const observerAttributes{observerAttribute};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Attribute-ownership query registration returned an invalid object handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "attribute-ownership query object discovery");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !observer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              observerAttribute),
      "Attribute-ownership query setup did not establish the standard initial ownership state");

  observer.recorder().clearOwnershipRecords();
  observer.rtiAmbassador().queryAttributeOwnership(
      object,
      observerAttributes);
  waitFor(
      observer,
      [&] {
        return !observer.recorder().ownershipInformationEvents().empty();
      },
      options,
      "attribute-ownership query Inform Attribute Ownership callback");
  auto const reports = observer.recorder().ownershipInformationEvents();
  require(
      reports.size() == 1U &&
          reports.front().object == object &&
          reports.front().attributes == observerAttributes &&
          reports.front().owner == owner.federateHandle(),
      "Inform Attribute Ownership returned the wrong object, attributes, or owner handle");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAttribute) &&
          !observer.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              observerAttribute),
      "Attribute-ownership query changed the reported ownership state");

  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioAttributeOwnershipQueryOwnerReportContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipQueryOwnerReport(options, model);
}

void scenarioAttributeOwnershipQueryUnownedResult(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.multiAttributeFom.empty(),
      "Attribute-ownership query testing requires an adapter-supplied multi-attribute FOM");

  Session owner(options, model, "ownership-query-unowned-owner");
  Session observer(options, model, "ownership-query-unowned-observer");
  auto const federation = federationName(
      options,
      "attribute-ownership-query-unowned-result");
  connectAndJoin(
      owner,
      observer,
      options,
      federation,
      options.multiAttributeFom);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const observerClass = observer.rtiAmbassador().getObjectClassHandle(
      options.multiAttributeObjectClassName);
  auto const ownerFirst = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeFirstName);
  auto const ownerSecond = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.multiAttributeSecondName);
  auto const observerFirst = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.multiAttributeFirstName);
  auto const observerSecond = observer.rtiAmbassador().getAttributeHandle(
      observerClass,
      options.multiAttributeSecondName);
  require(
      ownerClass.isValid() && observerClass.isValid() && ownerFirst.isValid() &&
          ownerSecond.isValid() && observerFirst.isValid() &&
          observerSecond.isValid(),
      "Attribute-ownership query lookup returned an invalid standard handle");
  require(
      ownerFirst != ownerSecond,
      "Attribute-ownership query adapter supplied duplicate attribute names");

  rti::AttributeHandleSet const ownerAttributes{ownerFirst};
  rti::AttributeHandleSet const observerAttributes{observerFirst, observerSecond};
  owner.rtiAmbassador().publishObjectClassAttributes(
      ownerClass,
      ownerAttributes);
  observer.rtiAmbassador().subscribeObjectClassAttributes(
      observerClass,
      observerAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Attribute-ownership query registration returned an invalid object handle");
  waitFor(
      observer,
      [&] { return observer.recorder().hasDiscovery(object); },
      options,
      "attribute-ownership query object discovery");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond) &&
          !observer.rtiAmbassador().isAttributeOwnedByFederate(object, observerFirst) &&
          !observer.rtiAmbassador().isAttributeOwnedByFederate(object, observerSecond),
      "Attribute-ownership query setup did not establish one owned and one unowned attribute");

  observer.recorder().clearOwnershipRecords();
  observer.rtiAmbassador().queryAttributeOwnership(
      object,
      observerAttributes);
  waitFor(
      observer,
      [&] {
        return observer.recorder().ownershipInformationEvents().size() == 1U &&
            observer.recorder().ownershipNotOwnedEvents().size() == 1U;
      },
      options,
      "the partitioned ownership query callbacks");

  auto const ownershipInformation =
      observer.recorder().ownershipInformationEvents();
  auto const ownershipNotOwned = observer.recorder().ownershipNotOwnedEvents();
  require(
      ownershipInformation.front().object == object &&
          ownershipInformation.front().attributes ==
              rti::AttributeHandleSet{observerFirst} &&
          ownershipInformation.front().owner == owner.federateHandle(),
      "Inform Attribute Ownership did not report the exact owned attribute and owner handle");
  require(
      ownershipNotOwned.front().object == object &&
          ownershipNotOwned.front().attributes ==
              rti::AttributeHandleSet{observerSecond},
      "Attribute Is Not Owned did not report the exact unowned attribute");
  require(
      observer.recorder().ownershipOwnedByRtiEvents().empty(),
      "an ordinary unowned attribute was incorrectly reported as RTI-owned");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerFirst) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerSecond),
      "the ownership query changed the object's ownership state");

  observer.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  owner.disconnect();
}

void scenarioAttributeOwnershipQueryUnownedResultContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipQueryUnownedResult(options, model);
}

void scenarioAttributeOwnershipQueryRtiOwnedResult(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty() && !options.mimFom.empty() &&
          !options.logicalTimeImplementationName.empty(),
      "RTI-owned attribute-query testing requires adapter-supplied FOM, standard MIM, and logical-time configuration");

  Session federate(options, model, "ownership-query-rti-owned-federate");
  auto const federation = federationName(
      options,
      "attribute-ownership-query-rti-owned-result");
  federate.connect();
  federate.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.modelFom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  federate.join(
      options.ownerFederateName + L"-ownership-query-rti-owned-federate",
      options.federateType,
      federation);

  auto const federateClass = federate.rtiAmbassador().getObjectClassHandle(
      L"HLAobjectRoot.HLAmanager.HLAfederate");
  auto const federateHandleAttribute = federate.rtiAmbassador().getAttributeHandle(
      federateClass,
      L"HLAfederateHandle");
  require(
      federateClass.isValid() && federateHandleAttribute.isValid() &&
          federate.rtiAmbassador().getObjectClassName(federateClass) ==
              L"HLAobjectRoot.HLAmanager.HLAfederate" &&
          federate.rtiAmbassador().getAttributeName(
              federateClass,
              federateHandleAttribute) == L"HLAfederateHandle",
      "Standard-MIM HLAfederate ownership-query lookup returned inconsistent handles");

  federate.recorder().clearDiscovery();
  federate.recorder().clearReflection();
  federate.rtiAmbassador().subscribeObjectClassAttributes(
      federateClass,
      rti::AttributeHandleSet{federateHandleAttribute},
      true,
      L"");

  auto findOwnMomReflection = [&]() -> std::optional<ReflectionRecord> {
    auto const reflections = federate.recorder().reflections();
    auto const expectedFederateHandle = copyBytes(federate.federateHandle().encode());
    auto const iterator = std::find_if(
        reflections.begin(),
        reflections.end(),
        [&](ReflectionRecord const& reflection) {
          auto const value = reflection.values.find(federateHandleAttribute);
          return reflection.present && value != reflection.values.end() &&
              copyBytes(value->second) == expectedFederateHandle;
        });
    if (iterator == reflections.end()) {
      return std::nullopt;
    }
    return *iterator;
  };
  waitFor(
      federate,
      [&] { return findOwnMomReflection().has_value(); },
      options,
      "standard-MIM self object reflection before ownership query");
  auto const ownMomReflection = findOwnMomReflection();
  require(
      ownMomReflection.has_value() &&
          federate.rtiAmbassador().getKnownObjectClassHandle(
              ownMomReflection->object) == federateClass,
      "Standard-MIM ownership-query setup discovered the wrong joined-federate object");

  federate.recorder().clearOwnershipRecords();
  federate.rtiAmbassador().queryAttributeOwnership(
      ownMomReflection->object,
      rti::AttributeHandleSet{federateHandleAttribute});
  waitFor(
      federate,
      [&] {
        return federate.recorder().ownershipOwnedByRtiEvents().size() == 1U;
      },
      options,
      "FederateAmbassador::attributeIsOwnedByRTI for HLAfederateHandle");
  auto const rtiOwned = federate.recorder().ownershipOwnedByRtiEvents();
  require(
      rtiOwned.size() == 1U &&
          rtiOwned.front().object == ownMomReflection->object &&
          rtiOwned.front().attributes ==
              rti::AttributeHandleSet{federateHandleAttribute} &&
          federate.recorder().ownershipInformationEvents().empty() &&
          federate.recorder().ownershipNotOwnedEvents().empty(),
      "The standard-MIM ownership query did not report exactly the RTI-owned attribute");
  require(
      !federate.rtiAmbassador().isAttributeOwnedByFederate(
          ownMomReflection->object,
          federateHandleAttribute),
      "A joined federate was reported as owner of the RTI-owned standard-MIM attribute");

  federate.resign(rti::NO_ACTION);
  federate.rtiAmbassador().destroyFederationExecution(federation);
  federate.disconnect();
}

void scenarioAttributeOwnershipQueryRtiOwnedResultContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAttributeOwnershipQueryRtiOwnedResult(options, model);
}

void scenarioUnconditionalAttributeOwnershipDivestiture(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Unconditional ownership-divestiture testing requires an adapter-supplied model FOM");

  Session owner(options, model, "unconditional-divestiture-owner");
  Session regularRequester(options, model, "unconditional-divestiture-regular");
  Session ifAvailableRequester(options, model, "unconditional-divestiture-if-available");
  Session invitedCandidate(options, model, "unconditional-divestiture-invited");
  Session unpublishedCandidate(options, model, "unconditional-divestiture-unpublished");
  auto const federation = federationName(
      options,
      "unconditional-attribute-ownership-divestiture");

  owner.connect();
  regularRequester.connect();
  ifAvailableRequester.connect();
  invitedCandidate.connect();
  unpublishedCandidate.connect();
  owner.rtiAmbassador().createFederationExecution(
      federation,
      options.modelFom.wstring(),
      options.logicalTimeImplementationName);
  owner.join(
      options.ownerFederateName + L"-unconditional-divestiture-owner",
      options.federateType,
      federation);
  regularRequester.join(
      options.memberFederateName + L"-unconditional-divestiture-regular",
      options.federateType,
      federation);
  ifAvailableRequester.join(
      options.memberFederateName + L"-unconditional-divestiture-if-available",
      options.federateType,
      federation);
  invitedCandidate.join(
      options.memberFederateName + L"-unconditional-divestiture-invited",
      options.federateType,
      federation);
  unpublishedCandidate.join(
      options.memberFederateName + L"-unconditional-divestiture-unpublished",
      options.federateType,
      federation);

  auto const ownerClass = owner.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const regularClass = regularRequester.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const ifAvailableClass =
      ifAvailableRequester.rtiAmbassador().getObjectClassHandle(
          options.typedObjectClassName);
  auto const invitedClass = invitedCandidate.rtiAmbassador().getObjectClassHandle(
      options.typedObjectClassName);
  auto const unpublishedClass =
      unpublishedCandidate.rtiAmbassador().getObjectClassHandle(
          options.typedObjectClassName);
  auto const ownerIdentity = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedIdentityAttributeName);
  auto const ownerInteger = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedIntegerAttributeName);
  auto const ownerAscii = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedAsciiAttributeName);
  auto const ownerUnicode = owner.rtiAmbassador().getAttributeHandle(
      ownerClass,
      options.typedUnicodeAttributeName);
  auto const regularIdentity = regularRequester.rtiAmbassador().getAttributeHandle(
      regularClass,
      options.typedIdentityAttributeName);
  auto const ifAvailableInteger =
      ifAvailableRequester.rtiAmbassador().getAttributeHandle(
          ifAvailableClass,
          options.typedIntegerAttributeName);
  auto const invitedAscii = invitedCandidate.rtiAmbassador().getAttributeHandle(
      invitedClass,
      options.typedAsciiAttributeName);
  auto const invitedUnicode = invitedCandidate.rtiAmbassador().getAttributeHandle(
      invitedClass,
      options.typedUnicodeAttributeName);
  auto const unpublishedAscii =
      unpublishedCandidate.rtiAmbassador().getAttributeHandle(
          unpublishedClass,
          options.typedAsciiAttributeName);
  auto const unpublishedUnicode =
      unpublishedCandidate.rtiAmbassador().getAttributeHandle(
          unpublishedClass,
          options.typedUnicodeAttributeName);
  require(
      ownerClass.isValid() && regularClass.isValid() &&
          ifAvailableClass.isValid() && invitedClass.isValid() &&
          unpublishedClass.isValid(),
      "Unconditional ownership-divestiture object-class lookup returned an invalid handle");
  require(
      ownerIdentity.isValid() && ownerInteger.isValid() && ownerAscii.isValid() &&
          ownerUnicode.isValid() && regularIdentity.isValid() &&
          ifAvailableInteger.isValid() && invitedAscii.isValid() &&
          invitedUnicode.isValid() && unpublishedAscii.isValid() &&
          unpublishedUnicode.isValid(),
      "Unconditional ownership-divestiture attribute lookup returned an invalid handle");
  require(
      ownerIdentity != ownerInteger && ownerInteger != ownerAscii &&
          ownerAscii != ownerUnicode,
      "Unconditional ownership-divestiture adapter supplied duplicate attributes");

  rti::AttributeHandleSet const ownerAttributes{
      ownerIdentity,
      ownerInteger,
      ownerAscii,
      ownerUnicode};
  rti::AttributeHandleSet const regularAttributes{regularIdentity};
  rti::AttributeHandleSet const ifAvailableAttributes{ifAvailableInteger};
  rti::AttributeHandleSet const invitedAttributes{invitedAscii, invitedUnicode};
  rti::AttributeHandleSet const unpublishedAttributes{
      unpublishedAscii,
      unpublishedUnicode};
  owner.rtiAmbassador().publishObjectClassAttributes(ownerClass, ownerAttributes);
  regularRequester.rtiAmbassador().publishObjectClassAttributes(
      regularClass,
      regularAttributes);
  ifAvailableRequester.rtiAmbassador().publishObjectClassAttributes(
      ifAvailableClass,
      ifAvailableAttributes);
  invitedCandidate.rtiAmbassador().publishObjectClassAttributes(
      invitedClass,
      invitedAttributes);
  regularRequester.rtiAmbassador().subscribeObjectClassAttributes(
      regularClass,
      regularAttributes,
      true,
      L"");
  ifAvailableRequester.rtiAmbassador().subscribeObjectClassAttributes(
      ifAvailableClass,
      ifAvailableAttributes,
      true,
      L"");
  invitedCandidate.rtiAmbassador().subscribeObjectClassAttributes(
      invitedClass,
      invitedAttributes,
      true,
      L"");
  unpublishedCandidate.rtiAmbassador().subscribeObjectClassAttributes(
      unpublishedClass,
      unpublishedAttributes,
      true,
      L"");

  auto const object = owner.rtiAmbassador().registerObjectInstance(ownerClass);
  require(
      object.isValid(),
      "Unconditional ownership-divestiture registration returned an invalid object handle");
  waitForSessions(
      {&regularRequester, &ifAvailableRequester, &invitedCandidate,
       &unpublishedCandidate},
      [&] {
        return regularRequester.recorder().hasDiscovery(object) &&
            ifAvailableRequester.recorder().hasDiscovery(object) &&
            invitedCandidate.recorder().hasDiscovery(object) &&
            unpublishedCandidate.recorder().hasDiscovery(object);
      },
      options,
      "unconditional ownership-divestiture object discovery");
  require(
      owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerIdentity) &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerInteger) &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAscii) &&
          owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerUnicode),
      "Unconditional ownership-divestiture did not establish initial ownership");

  std::vector<std::uint8_t> const regularTagBytes{0xA2U, 0x07U, 0x20U};
  std::vector<std::uint8_t> const ifAvailableTagBytes{0xB2U, 0x07U, 0x20U};
  std::vector<std::uint8_t> const assumptionTagBytes{0xD2U, 0x07U, 0x20U};
  std::vector<std::uint8_t> const invitedTagBytes{0xC2U, 0x07U, 0x20U};
  rti::VariableLengthData const regularTag(
      regularTagBytes.data(),
      regularTagBytes.size());
  rti::VariableLengthData const ifAvailableTag(
      ifAvailableTagBytes.data(),
      ifAvailableTagBytes.size());
  rti::VariableLengthData const assumptionTag(
      assumptionTagBytes.data(),
      assumptionTagBytes.size());
  rti::VariableLengthData const invitedTag(
      invitedTagBytes.data(),
      invitedTagBytes.size());
  regularRequester.recorder().clearOwnershipRecords();
  ifAvailableRequester.recorder().clearOwnershipRecords();
  invitedCandidate.recorder().clearOwnershipRecords();
  unpublishedCandidate.recorder().clearOwnershipRecords();
  regularRequester.rtiAmbassador().attributeOwnershipAcquisition(
      object,
      regularAttributes,
      regularTag);
  ifAvailableRequester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      ifAvailableAttributes,
      ifAvailableTag);

  auto serviceImmediateCallbacks = [&] {
    if (model != rti::HLA_IMMEDIATE) {
      return;
    }
    static_cast<void>(regularRequester.rtiAmbassador().evokeCallback(0.0));
    static_cast<void>(ifAvailableRequester.rtiAmbassador().evokeCallback(0.0));
    static_cast<void>(invitedCandidate.rtiAmbassador().evokeCallback(0.0));
    static_cast<void>(unpublishedCandidate.rtiAmbassador().evokeCallback(0.0));
  };
  waitForSessions(
      {&regularRequester, &ifAvailableRequester},
      [&] {
        serviceImmediateCallbacks();
        return ifAvailableRequester.recorder().ownershipUnavailable().has_value();
      },
      options,
      "unconditional ownership-divestiture initial If Available rejection");
  auto const unavailable = ifAvailableRequester.recorder().ownershipUnavailable();
  require(
      unavailable->object == object &&
          unavailable->attributes == ifAvailableAttributes &&
          unavailable->tag == ifAvailableTagBytes,
      "Unconditional ownership-divestiture initial If Available rejection returned the wrong metadata");
  ifAvailableRequester.recorder().clearOwnershipRecords();
  owner.rtiAmbassador().unconditionalAttributeOwnershipDivestiture(
      object,
      ownerAttributes,
      assumptionTag);
  waitForSessions(
      {&regularRequester, &ifAvailableRequester, &invitedCandidate},
      [&] {
        serviceImmediateCallbacks();
        return regularRequester.recorder().ownershipAcquisition().has_value();
      },
      options,
      "unconditional ownership-divestiture regular acquisition callback");
  waitForSessions(
      {&regularRequester, &ifAvailableRequester, &invitedCandidate},
      [&] {
        serviceImmediateCallbacks();
        return invitedCandidate.recorder().ownershipAssumption().has_value();
      },
      options,
      "unconditional ownership-divestiture ownership-assumption callback");
  auto const regularAcquisition = regularRequester.recorder().ownershipAcquisition();
  auto const assumption = invitedCandidate.recorder().ownershipAssumption();
  require(
      regularAcquisition->object == object &&
          regularAcquisition->attributes == regularAttributes &&
          regularAcquisition->tag == regularTagBytes,
      "Unconditional ownership-divestiture regular acquisition returned the wrong metadata");
  require(
      assumption->object == object && assumption->attributes == invitedAttributes &&
          assumption->tag == assumptionTagBytes,
      "Unconditional ownership-divestiture assumption returned the wrong metadata");
  require(
      !ifAvailableRequester.recorder().ownershipAcquisition().has_value(),
      "Unconditional ownership-divestiture bypassed the initial If Available rejection");
  unpublishedCandidate.pump();
  require(
      !unpublishedCandidate.recorder().ownershipAssumption().has_value(),
      "Unconditional ownership-divestiture offered an unpublished candidate");
  require(
      !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerIdentity) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerInteger) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerAscii) &&
          !owner.rtiAmbassador().isAttributeOwnedByFederate(object, ownerUnicode) &&
          regularRequester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              regularIdentity) &&
          !ifAvailableRequester.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              ifAvailableInteger) &&
          !invitedCandidate.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              invitedAscii) &&
          !invitedCandidate.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              invitedUnicode),
      "Unconditional ownership-divestiture established the wrong ownership split");

  ifAvailableRequester.recorder().clearOwnershipRecords();
  ifAvailableRequester.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      ifAvailableAttributes,
      ifAvailableTag);
  waitForSessions(
      {&ifAvailableRequester},
      [&] {
        serviceImmediateCallbacks();
        return ifAvailableRequester.recorder().ownershipAcquisition().has_value();
      },
      options,
      "unconditional ownership-divestiture retried If Available acquisition");
  auto const ifAvailableAcquisition =
      ifAvailableRequester.recorder().ownershipAcquisition();
  require(
      ifAvailableAcquisition->object == object &&
          ifAvailableAcquisition->attributes == ifAvailableAttributes &&
          ifAvailableAcquisition->tag == ifAvailableTagBytes,
      "Unconditional ownership-divestiture retried If Available acquisition returned the wrong metadata");
  require(
      ifAvailableRequester.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          ifAvailableInteger),
      "Unconditional ownership-divestiture retried If Available acquisition did not establish ownership");

  invitedCandidate.recorder().clearOwnershipRecords();
  invitedCandidate.rtiAmbassador().attributeOwnershipAcquisitionIfAvailable(
      object,
      invitedAttributes,
      invitedTag);
  waitFor(
      invitedCandidate,
      [&] { return invitedCandidate.recorder().ownershipAcquisition().has_value(); },
      options,
      "unconditional ownership-divestiture invited acquisition");
  auto const invitedAcquisition = invitedCandidate.recorder().ownershipAcquisition();
  require(
      invitedAcquisition->object == object &&
          invitedAcquisition->attributes == invitedAttributes &&
          invitedAcquisition->tag == invitedTagBytes,
      "Unconditional ownership-divestiture invited acquisition returned the wrong metadata");
  require(
      invitedCandidate.rtiAmbassador().isAttributeOwnedByFederate(
          object,
          invitedAscii) &&
          invitedCandidate.rtiAmbassador().isAttributeOwnedByFederate(
              object,
              invitedUnicode),
      "Unconditional ownership-divestiture invited acquisition did not establish ownership");

  unpublishedCandidate.rtiAmbassador().unsubscribeObjectClassAttributes(
      unpublishedClass,
      unpublishedAttributes);
  invitedCandidate.rtiAmbassador().unsubscribeObjectClassAttributes(
      invitedClass,
      invitedAttributes);
  ifAvailableRequester.rtiAmbassador().unsubscribeObjectClassAttributes(
      ifAvailableClass,
      ifAvailableAttributes);
  regularRequester.rtiAmbassador().unsubscribeObjectClassAttributes(
      regularClass,
      regularAttributes);
  owner.rtiAmbassador().unpublishObjectClassAttributes(ownerClass, ownerAttributes);
  invitedCandidate.rtiAmbassador().unpublishObjectClassAttributes(
      invitedClass,
      invitedAttributes);
  ifAvailableRequester.rtiAmbassador().unpublishObjectClassAttributes(
      ifAvailableClass,
      ifAvailableAttributes);
  regularRequester.rtiAmbassador().unpublishObjectClassAttributes(
      regularClass,
      regularAttributes);
  invitedCandidate.resign(rti::NO_ACTION);
  ifAvailableRequester.resign(rti::NO_ACTION);
  regularRequester.resign(rti::NO_ACTION);
  unpublishedCandidate.resign(rti::NO_ACTION);
  owner.resign(rti::DELETE_OBJECTS);
  owner.rtiAmbassador().destroyFederationExecution(federation);
  invitedCandidate.disconnect();
  ifAvailableRequester.disconnect();
  regularRequester.disconnect();
  unpublishedCandidate.disconnect();
  owner.disconnect();
}

void scenarioUnconditionalAttributeOwnershipDivestitureContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioUnconditionalAttributeOwnershipDivestiture(options, model);
}

void scenarioAutomaticResignDirectiveDeleteObjects(
    Options const& options,
    rti::CallbackModel model) {
  require(
      options.connectionLossServerManaged,
      "Automatic resignation on connection loss requires an adapter-managed fault fixture");
  Session survivor(options, model, "owner");
  Session lost(options, model, "member");
  auto const federation = federationName(
      options,
      "automatic-resign-directive-delete-objects");
  connectAndJoin(survivor, lost, options, federation, options.fom);

  auto const lostClass = lost.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const survivorClass = survivor.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const lostAttribute = lost.rtiAmbassador().getAttributeHandle(
      lostClass,
      options.attributeName);
  auto const survivorAttribute = survivor.rtiAmbassador().getAttributeHandle(
      survivorClass,
      options.attributeName);
  require(
      survivorClass.isValid() && lostClass.isValid() &&
          survivorAttribute.isValid() && lostAttribute.isValid(),
      "automatic resign directive lookup returned an invalid standard handle");

  rti::AttributeHandleSet const survivorAttributes{survivorAttribute};
  rti::AttributeHandleSet const lostAttributes{lostAttribute};
  lost.rtiAmbassador().publishObjectClassAttributes(
      lostClass,
      lostAttributes);
  survivor.rtiAmbassador().subscribeObjectClassAttributes(
      survivorClass,
      survivorAttributes,
      true,
      L"");

  auto const object = lost.rtiAmbassador().registerObjectInstance(lostClass);
  require(
      object.isValid(),
      "automatic resign directive registration returned an invalid object handle");
  auto const objectName = lost.rtiAmbassador().getObjectInstanceName(object);
  require(
      !objectName.empty(),
      "automatic resign directive registration returned an empty object name");
  waitFor(
      survivor,
      [&] { return survivor.recorder().hasDiscovery(object); },
      options,
      "automatic resign directive object discovery");
  lost.rtiAmbassador().setAutomaticResignDirective(rti::DELETE_OBJECTS);
  require(
      lost.rtiAmbassador().getAutomaticResignDirective() == rti::DELETE_OBJECTS,
      "automatic resign directive did not round-trip before connection loss");
  require(
      survivor.rtiAmbassador().getObjectInstanceHandle(objectName) == object,
      "automatic resign directive discovery did not preserve object-name lookup");

  survivor.recorder().clearRemovals();
  waitForConnectionLossAndSignal(
      lost,
      options,
      "automatic resign directive delete-object cleanup");
  waitFor(
      survivor,
      [&] {
        return survivor.recorder().hasRemoval(
            object,
            {},
            lost.federateHandle());
      },
      options,
      "automatic resign directive delete-object removal");

  auto const removals = survivor.recorder().removals();
  require(
      removals.size() == 1U,
      "automatic resign directive delivered a duplicate object removal callback");
  require(
      removals.front().object == object &&
          removals.front().tag.empty() &&
          removals.front().producer == lost.federateHandle(),
      "automatic resign directive returned the wrong removal metadata");
  requireException(
      [&] {
        static_cast<void>(survivor.rtiAmbassador().getObjectInstanceHandle(objectName));
      },
      L"ObjectInstanceNotKnown",
      "looking up an automatically deleted object");

  survivor.resign(rti::NO_ACTION);
  survivor.disconnect();
}

void scenarioAutomaticResignDirectiveDeleteObjectsContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAutomaticResignDirectiveDeleteObjects(options, model);
}

void scenarioPublicHandleDecoding(Options const& options, rti::CallbackModel model) {
  scenarioHandleLookups(options, model);
  scenarioHandleWireFormats(options, model);
}

void scenarioPublicHandleDecodingContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioPublicHandleDecoding(options, model);
}

void scenarioTimestampedDirectedInteractionRetraction(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped directed-interaction retraction testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-directed-retraction-publisher");
  Session receiver(options, model, "timestamped-directed-retraction-receiver");
  auto const federation = federationName(
      options,
      "timestamped-directed-interaction-retraction");
  connectAndJoin(publisher, receiver, options, federation, options.fom);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const receiverClass = receiver.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const receiverAttribute = receiver.rtiAmbassador().getAttributeHandle(
      receiverClass,
      options.attributeName);
  auto const publisherInteraction = publisher.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const receiverInteraction = receiver.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const receiverParameter = receiver.rtiAmbassador().getParameterHandle(
      receiverInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && receiverClass.isValid() &&
          publisherAttribute.isValid() && receiverAttribute.isValid() &&
          publisherInteraction.isValid() && receiverInteraction.isValid() &&
          publisherParameter.isValid() && receiverParameter.isValid(),
      "Timestamped directed-interaction retraction lookup returned an invalid standard handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const receiverAttributes{receiverAttribute};
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  rti::InteractionClassHandleSet const receiverDirected{receiverInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.rtiAmbassador().subscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  receiver.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected,
      true);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

  auto const target = publisher.rtiAmbassador().registerObjectInstance(publisherClass);
  require(
      target.isValid(),
      "Timestamped directed-interaction retraction registration returned an invalid target");
  waitFor(
      receiver,
      [&] { return receiver.recorder().hasDiscovery(target); },
      options,
      "timestamped directed-interaction retraction target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      publisher,
      receiver,
      publisherTime,
      receiverTime,
      options,
      "timestamped directed-interaction retraction");
  publisher.recorder().clearTimeCallbacks();
  receiver.recorder().clearTimeCallbacks();
  publisher.recorder().clearTimedDirectedInteractions();
  receiver.recorder().clearTimedDirectedInteractions();
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();

  auto const firstMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const firstParameterBytes{0x52U, 0x45U, 0x54U};
  std::vector<std::uint8_t> const firstTagBytes{0x52U, 0x31U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          firstParameterBytes.data(),
          firstParameterBytes.size()));
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(),
      firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      firstParameters,
      firstTag,
      *firstMessageTime);
  require(
      firstRetraction.isValid(),
      "Timestamped directed-interaction retraction returned an invalid first handle");
  require(
      receiver.recorder().timedDirectedInteractions().empty(),
      "Timestamped directed-interaction retraction delivered before the advance");

  receiver.rtiAmbassador().timeAdvanceRequest(*firstMessageTime);
  publisher.rtiAmbassador().retract(firstRetraction);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstMessageTime);
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return receiver.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped directed-interaction retraction grant after cancellation");
  require(
      receiver.recorder().timedDirectedInteractions().empty(),
      "Timestamped directed-interaction retraction delivered a canceled message");
  require(
      receiver.recorder().timeAdvanceGrants().front().encoded ==
          encodeTime(*firstMessageTime),
      "Timestamped directed-interaction retraction returned the wrong receiver grant time");
  publisher.recorder().clearCallbackOrder();
  receiver.recorder().clearCallbackOrder();
  auto const secondMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  std::vector<std::uint8_t> const secondParameterBytes{0x44U, 0x45U, 0x4CU};
  std::vector<std::uint8_t> const secondTagBytes{0x52U, 0x32U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          secondParameterBytes.data(),
          secondParameterBytes.size()));
  rti::VariableLengthData const secondTag(
      secondTagBytes.data(),
      secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      secondParameters,
      secondTag,
      *secondMessageTime);
  require(
      secondRetraction.isValid(),
      "Timestamped directed-interaction retraction returned an invalid delivery handle");
  receiver.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  if (publisher.recorder().timeAdvanceGrants().size() >= 1U) {
    publisher.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  }
  waitForSessions(
      {&publisher, &receiver},
      [&] {
        return receiver.recorder().timeAdvanceGrants().size() >= 2U &&
            receiver.recorder().timedDirectedInteractions().size() >= 1U;
      },
      options,
      "timestamped directed-interaction delivery after retraction control");

  auto const delivered = receiver.recorder().timedDirectedInteractions().front();
  require(
      delivered.interaction == receiverInteraction && delivered.object == target &&
          delivered.parameters.size() == 1U &&
          delivered.parameters.count(receiverParameter) == 1U &&
          copyBytes(delivered.parameters.at(receiverParameter)) == secondParameterBytes,
      "Timestamped directed-interaction delivery returned the wrong target or parameters");
  require(
      delivered.tag == secondTagBytes &&
          delivered.producer == publisher.federateHandle(),
      "Timestamped directed-interaction delivery changed its tag or producer");
  require(
      delivered.transportation.isValid() &&
          !receiver.rtiAmbassador().getTransportationTypeName(
              delivered.transportation).empty(),
      "Timestamped directed-interaction delivery returned invalid transportation metadata");
  require(
      delivered.time == encodeTime(*secondMessageTime) &&
          !delivered.timeText.empty() &&
          delivered.sentOrder == rti::TIMESTAMP &&
          delivered.receivedOrder == rti::TIMESTAMP &&
          delivered.retractionPresent &&
          delivered.retraction == copyBytes(secondRetraction.encode()),
      "Timestamped directed-interaction delivery returned the wrong time, order, or retraction metadata");
  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped directed interaction");

  receiver.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      receiverClass,
      receiverDirected);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  receiver.rtiAmbassador().unsubscribeObjectClassAttributes(
      receiverClass,
      receiverAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  receiver.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDirectedInteractionRetractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionRetraction(options, model);
}

void scenarioTimestampedDirectedInteractionRetractionFanout(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Timestamped directed-interaction retraction fan-out testing requires an adapter-supplied logical-time implementation");

  Session publisher(options, model, "timestamped-directed-retraction-fanout-publisher");
  Session first(options, model, "timestamped-directed-retraction-fanout-first");
  Session second(options, model, "timestamped-directed-retraction-fanout-second");
  auto const federation = federationName(
      options,
      "timestamped-directed-interaction-retraction-fanout");
  connectAndJoin(publisher, first, options, federation, options.fom);
  second.connect();
  second.join(
      options.memberFederateName +
          L"-timestamped-directed-retraction-fanout-second",
      options.federateType,
      federation);

  auto const publisherClass = publisher.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const firstClass = first.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const secondClass = second.rtiAmbassador().getObjectClassHandle(
      options.objectClassName);
  auto const publisherAttribute = publisher.rtiAmbassador().getAttributeHandle(
      publisherClass,
      options.attributeName);
  auto const firstAttribute = first.rtiAmbassador().getAttributeHandle(
      firstClass,
      options.attributeName);
  auto const secondAttribute = second.rtiAmbassador().getAttributeHandle(
      secondClass,
      options.attributeName);
  auto const publisherInteraction =
      publisher.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const firstInteraction = first.rtiAmbassador().getInteractionClassHandle(
      options.interactionClassName);
  auto const secondInteraction =
      second.rtiAmbassador().getInteractionClassHandle(
          options.interactionClassName);
  auto const publisherParameter = publisher.rtiAmbassador().getParameterHandle(
      publisherInteraction,
      options.parameterName);
  auto const firstParameter = first.rtiAmbassador().getParameterHandle(
      firstInteraction,
      options.parameterName);
  auto const secondParameter = second.rtiAmbassador().getParameterHandle(
      secondInteraction,
      options.parameterName);
  require(
      publisherClass.isValid() && firstClass.isValid() && secondClass.isValid() &&
          publisherAttribute.isValid() && firstAttribute.isValid() &&
          secondAttribute.isValid() && publisherInteraction.isValid() &&
          firstInteraction.isValid() && secondInteraction.isValid() &&
          publisherParameter.isValid() && firstParameter.isValid() &&
          secondParameter.isValid(),
      "Timestamped directed-interaction retraction fan-out lookup returned an invalid standard handle");

  rti::AttributeHandleSet const publisherAttributes{publisherAttribute};
  rti::AttributeHandleSet const firstAttributes{firstAttribute};
  rti::AttributeHandleSet const secondAttributes{secondAttribute};
  rti::InteractionClassHandleSet const publisherDirected{publisherInteraction};
  rti::InteractionClassHandleSet const firstDirected{firstInteraction};
  rti::InteractionClassHandleSet const secondDirected{secondInteraction};
  publisher.rtiAmbassador().publishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.rtiAmbassador().subscribeObjectClassAttributes(
      firstClass,
      firstAttributes,
      true,
      L"");
  second.rtiAmbassador().subscribeObjectClassAttributes(
      secondClass,
      secondAttributes,
      true,
      L"");
  publisher.rtiAmbassador().publishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  first.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      firstClass,
      firstDirected,
      true);
  second.rtiAmbassador().subscribeObjectClassDirectedInteractions(
      secondClass,
      secondDirected,
      true);
  publisher.rtiAmbassador().changeInteractionOrderType(
      publisherInteraction,
      rti::TIMESTAMP);

  auto const target = publisher.rtiAmbassador().registerObjectInstance(
      publisherClass);
  require(
      target.isValid(),
      "Timestamped directed-interaction retraction fan-out registration returned an invalid target");
  waitForSessions(
      {&first, &second},
      [&] {
        return first.recorder().hasDiscovery(target) &&
            second.recorder().hasDiscovery(target);
      },
      options,
      "timestamped directed-interaction retraction fan-out target discovery");

  auto publisherTime = makeTimeContext(publisher);
  auto firstTime = makeTimeContext(first);
  auto secondTime = makeTimeContext(second);
  require(
      publisherTime.factory->getName() == firstTime.factory->getName() &&
          publisherTime.factory->getName() == secondTime.factory->getName(),
      "Timestamped directed-interaction retraction fan-out members selected different logical-time factories");
  first.rtiAmbassador().enableTimeConstrained();
  waitFor(
      first,
      [&] { return first.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped directed-interaction retraction fan-out first time-constrained callback");
  second.rtiAmbassador().enableTimeConstrained();
  waitFor(
      second,
      [&] { return second.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "timestamped directed-interaction retraction fan-out second time-constrained callback");
  publisher.rtiAmbassador().enableTimeRegulation(*publisherTime.epsilon);
  waitFor(
      publisher,
      [&] { return publisher.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "timestamped directed-interaction retraction fan-out time-regulation callback");
  publisher.recorder().clearTimeCallbacks();
  first.recorder().clearTimeCallbacks();
  second.recorder().clearTimeCallbacks();
  publisher.recorder().clearTimedDirectedInteractions();
  first.recorder().clearTimedDirectedInteractions();
  second.recorder().clearTimedDirectedInteractions();
  publisher.recorder().clearCallbackOrder();
  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();

  auto const firstMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      6U);
  std::vector<std::uint8_t> const firstParameterBytes{
      0x46U, 0x41U, 0x4EU, 0x31U};
  std::vector<std::uint8_t> const firstTagBytes{0x46U, 0x52U, 0x31U};
  rti::ParameterHandleValueMap firstParameters;
  firstParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          firstParameterBytes.data(),
          firstParameterBytes.size()));
  rti::VariableLengthData const firstTag(
      firstTagBytes.data(),
      firstTagBytes.size());
  auto const firstRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      firstParameters,
      firstTag,
      *firstMessageTime);
  require(
      firstRetraction.isValid(),
      "Timestamped directed-interaction retraction fan-out returned an invalid first handle");
  require(
      first.recorder().timedDirectedInteractions().empty() &&
          second.recorder().timedDirectedInteractions().empty(),
      "Timestamped directed-interaction retraction fan-out delivered before either advance");

  first.rtiAmbassador().timeAdvanceRequest(*firstMessageTime);
  second.rtiAmbassador().timeAdvanceRequest(*firstMessageTime);
  publisher.rtiAmbassador().retract(firstRetraction);
  publisher.rtiAmbassador().timeAdvanceRequest(*firstMessageTime);
  waitForSessions(
      {&publisher, &first, &second},
      [&] {
        return first.recorder().timeAdvanceGrants().size() >= 1U &&
            second.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "timestamped directed-interaction retraction fan-out canceled grant");
  require(
      first.recorder().timedDirectedInteractions().empty() &&
          second.recorder().timedDirectedInteractions().empty() &&
          first.recorder().retractions().empty() &&
          second.recorder().retractions().empty(),
      "Timestamped directed-interaction retraction fan-out leaked a canceled copy");
  require(
      first.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstMessageTime) &&
          second.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstMessageTime),
      "Timestamped directed-interaction retraction fan-out returned the wrong first grant time");

  publisher.recorder().clearCallbackOrder();
  first.recorder().clearCallbackOrder();
  second.recorder().clearCallbackOrder();
  auto const secondMessageTime = timeAfter(
      *publisherTime.factory,
      *publisherTime.initial,
      *publisherTime.epsilon,
      8U);
  std::vector<std::uint8_t> const secondParameterBytes{
      0x46U, 0x41U, 0x4EU, 0x32U};
  std::vector<std::uint8_t> const secondTagBytes{0x46U, 0x52U, 0x32U};
  rti::ParameterHandleValueMap secondParameters;
  secondParameters.emplace(
      publisherParameter,
      rti::VariableLengthData(
          secondParameterBytes.data(),
          secondParameterBytes.size()));
  rti::VariableLengthData const secondTag(
      secondTagBytes.data(),
      secondTagBytes.size());
  auto const secondRetraction = publisher.rtiAmbassador().sendDirectedInteraction(
      publisherInteraction,
      target,
      secondParameters,
      secondTag,
      *secondMessageTime);
  require(
      secondRetraction.isValid(),
      "Timestamped directed-interaction retraction fan-out returned an invalid delivery handle");
  first.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  second.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  if (publisher.recorder().timeAdvanceGrants().size() >= 1U) {
    publisher.rtiAmbassador().timeAdvanceRequest(*secondMessageTime);
  }
  waitForSessions(
      {&publisher, &first, &second},
      [&] {
        return first.recorder().timeAdvanceGrants().size() >= 2U &&
            second.recorder().timeAdvanceGrants().size() >= 2U &&
            first.recorder().timedDirectedInteractions().size() >= 1U &&
            second.recorder().timedDirectedInteractions().size() >= 1U;
      },
      options,
      "timestamped directed-interaction retraction fan-out delivery");

  auto assertDelivery = [&](Session& recipient,
                            TimedDirectedInteractionRecord const& delivery,
                            rti::InteractionClassHandle const& interaction,
                            rti::ParameterHandle const& parameter,
                            std::string const& description) {
    require(
        delivery.interaction == interaction && delivery.object == target &&
            delivery.parameters.size() == 1U &&
            delivery.parameters.count(parameter) == 1U &&
            copyBytes(delivery.parameters.at(parameter)) == secondParameterBytes,
        description + " returned the wrong target or parameter payload");
    require(
        delivery.tag == secondTagBytes &&
            delivery.producer == publisher.federateHandle(),
        description + " changed the tag or producer metadata");
    require(
        delivery.transportation.isValid() &&
            !recipient.rtiAmbassador().getTransportationTypeName(
                delivery.transportation).empty(),
        description + " returned an unresolvable transportation handle");
    require(
        delivery.time == encodeTime(*secondMessageTime) &&
            !delivery.timeText.empty() &&
            delivery.sentOrder == rti::TIMESTAMP &&
            delivery.receivedOrder == rti::TIMESTAMP &&
            delivery.retractionPresent &&
            delivery.retraction == copyBytes(secondRetraction.encode()),
        description + " changed the time, order, or retraction metadata");
  };

  auto const firstDeliveries = first.recorder().timedDirectedInteractions();
  auto const secondDeliveries = second.recorder().timedDirectedInteractions();
  require(
      firstDeliveries.size() == 1U && secondDeliveries.size() == 1U,
      "Timestamped directed-interaction retraction fan-out duplicated a delivery");
  assertDelivery(
      first,
      firstDeliveries.front(),
      firstInteraction,
      firstParameter,
      "Timestamped directed-interaction retraction fan-out first recipient");
  assertDelivery(
      second,
      secondDeliveries.front(),
      secondInteraction,
      secondParameter,
      "Timestamped directed-interaction retraction fan-out second recipient");
  require(
      first.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstMessageTime) &&
          first.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*secondMessageTime) &&
          second.recorder().timeAdvanceGrants().front().encoded ==
              encodeTime(*firstMessageTime) &&
          second.recorder().timeAdvanceGrants().back().encoded ==
              encodeTime(*secondMessageTime),
      "Timestamped directed-interaction retraction fan-out returned the wrong recipient grant sequence");

  requireException(
      [&] { publisher.rtiAmbassador().retract(secondRetraction); },
      L"MessageCanNoLongerBeRetracted",
      "retracting a delivered timestamped directed-interaction fan-out copy");
  require(
      first.recorder().retractions().empty() &&
          second.recorder().retractions().empty(),
      "Timestamped directed-interaction retraction fan-out emitted a stale retraction callback after delivery");

  first.rtiAmbassador().disableTimeConstrained();
  second.rtiAmbassador().disableTimeConstrained();
  publisher.rtiAmbassador().disableTimeRegulation();
  first.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      firstClass,
      firstDirected);
  second.rtiAmbassador().unsubscribeObjectClassDirectedInteractions(
      secondClass,
      secondDirected);
  publisher.rtiAmbassador().unpublishObjectClassDirectedInteractions(
      publisherClass,
      publisherDirected);
  first.rtiAmbassador().unsubscribeObjectClassAttributes(
      firstClass,
      firstAttributes);
  second.rtiAmbassador().unsubscribeObjectClassAttributes(
      secondClass,
      secondAttributes);
  publisher.rtiAmbassador().unpublishObjectClassAttributes(
      publisherClass,
      publisherAttributes);
  first.resign(rti::NO_ACTION);
  second.resign(rti::NO_ACTION);
  publisher.resign(rti::DELETE_OBJECTS);
  publisher.rtiAmbassador().destroyFederationExecution(federation);
  first.disconnect();
  second.disconnect();
  publisher.disconnect();
}

void scenarioTimestampedDirectedInteractionRetractionFanoutContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimestampedDirectedInteractionRetractionFanout(options, model);
}

void scenarioModifyLookahead(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.logicalTimeImplementationName.empty(),
      "Modify Lookahead testing requires an adapter-supplied logical-time implementation");

  Session regulator(options, model, "modify-lookahead-regulator");
  Session constrained(options, model, "modify-lookahead-constrained");
  auto const federation = federationName(options, "modify-lookahead");
  connectAndJoin(regulator, constrained, options, federation, options.fom);

  auto regulatorTime = makeTimeContext(regulator);
  auto constrainedTime = makeTimeContext(constrained);
  require(
      regulatorTime.factory->getName() == constrainedTime.factory->getName(),
      "Modify Lookahead members selected different logical-time factories");

  auto beforeRegulation = regulatorTime.factory->makeZero();
  require(
      beforeRegulation != nullptr,
      "Modify Lookahead could not allocate a pre-regulation query interval");
  requireException(
      [&] { regulator.rtiAmbassador().queryLookahead(*beforeRegulation); },
      L"TimeRegulationIsNotEnabled",
      "querying lookahead before enabling time regulation");
  requireException(
      [&] {
        regulator.rtiAmbassador().modifyLookahead(*regulatorTime.zero);
      },
      L"TimeRegulationIsNotEnabled",
      "modifying lookahead before enabling time regulation");

  constrained.rtiAmbassador().enableTimeConstrained();
  waitFor(
      constrained,
      [&] { return constrained.recorder().timeConstrainedEnabled().size() >= 1U; },
      options,
      "Modify Lookahead time-constrained callback");
  regulator.rtiAmbassador().enableTimeRegulation(*regulatorTime.epsilon);
  waitFor(
      regulator,
      [&] { return regulator.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Modify Lookahead time-regulation callback");

  auto queriedInitial = regulatorTime.factory->makeZero();
  require(
      queriedInitial != nullptr,
      "Modify Lookahead could not allocate an initial query interval");
  regulator.rtiAmbassador().queryLookahead(*queriedInitial);
  require(
      *queriedInitial == *regulatorTime.epsilon,
      "initial Query Lookahead did not match the enabled epsilon");

  auto changedLookaheadTime = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      3U);
  auto changedLookahead = regulatorTime.factory->makeZero();
  require(
      changedLookahead != nullptr,
      "Modify Lookahead could not allocate the increased interval");
  changedLookahead->setToDifference(
      *changedLookaheadTime,
      *regulatorTime.initial);
  regulator.rtiAmbassador().modifyLookahead(*changedLookahead);

  auto queriedChanged = regulatorTime.factory->makeZero();
  require(
      queriedChanged != nullptr,
      "Modify Lookahead could not allocate the changed query interval");
  regulator.rtiAmbassador().queryLookahead(*queriedChanged);
  require(
      *queriedChanged == *changedLookahead,
      "Modify Lookahead increase did not cross the Query Lookahead boundary");

  std::unique_ptr<rti::LogicalTimeInterval> incompatibleLookahead;
  if (regulatorTime.factory->getName() == L"HLAinteger64Time") {
    incompatibleLookahead = std::make_unique<rti::HLAfloat64Interval>(1.0);
  } else {
    incompatibleLookahead = std::make_unique<rti::HLAinteger64Interval>(1);
  }
  requireException(
      [&] { regulator.rtiAmbassador().modifyLookahead(*incompatibleLookahead); },
      L"InvalidLookahead",
      "modifying lookahead with an incompatible standard interval");
  auto queriedAfterInvalid = regulatorTime.factory->makeZero();
  regulator.rtiAmbassador().queryLookahead(*queriedAfterInvalid);
  require(
      *queriedAfterInvalid == *changedLookahead,
      "an invalid Modify Lookahead changed the active interval");

  regulator.rtiAmbassador().modifyLookahead(*regulatorTime.epsilon);
  auto queriedDeferred = regulatorTime.factory->makeZero();
  regulator.rtiAmbassador().queryLookahead(*queriedDeferred);
  require(
      *queriedDeferred == *changedLookahead,
      "a decreased lookahead took effect before the next logical-time grant");

  auto const regulatorTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      1U);
  auto const constrainedTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      1U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorTarget);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedTarget);
  if (model == rti::HLA_EVOKED) {
    requireException(
        [&] { regulator.rtiAmbassador().modifyLookahead(*changedLookahead); },
        L"InTimeAdvancingState",
        "modifying lookahead while a time advance is pending");
    auto queriedDuringAdvance = regulatorTime.factory->makeZero();
    require(
        queriedDuringAdvance != nullptr,
        "Modify Lookahead could not allocate a pending-advance query interval");
    regulator.rtiAmbassador().queryLookahead(*queriedDuringAdvance);
    require(
        *queriedDuringAdvance == *changedLookahead,
        "a rejected pending-advance Modify Lookahead changed the active interval");
  }
  waitFor(
      regulator,
      constrained,
      [&] {
        return regulator.recorder().timeAdvanceGrants().size() >= 1U &&
            constrained.recorder().timeAdvanceGrants().size() >= 1U;
      },
      options,
      "Modify Lookahead grant after deferred decrease");
  auto const regulatorGrants = regulator.recorder().timeAdvanceGrants();
  auto const constrainedGrants = constrained.recorder().timeAdvanceGrants();
  require(
      regulatorGrants.size() == 1U && constrainedGrants.size() == 1U,
      "Modify Lookahead delivered duplicate grants");
  require(
      regulatorGrants.front().encoded == encodeTime(*regulatorTarget) &&
          constrainedGrants.front().encoded == encodeTime(*constrainedTarget),
      "Modify Lookahead returned the wrong grant time");

  auto partiallyDecreasedLookahead = regulatorTime.factory->makeZero();
  require(
      partiallyDecreasedLookahead != nullptr,
      "Modify Lookahead could not allocate an intermediate query interval");
  *partiallyDecreasedLookahead = *regulatorTime.epsilon;
  *partiallyDecreasedLookahead += *regulatorTime.epsilon;
  regulator.rtiAmbassador().queryLookahead(*partiallyDecreasedLookahead);
  auto expectedIntermediateLookahead = regulatorTime.factory->makeZero();
  require(
      expectedIntermediateLookahead != nullptr,
      "Modify Lookahead could not allocate its expected intermediate interval");
  *expectedIntermediateLookahead = *regulatorTime.epsilon;
  *expectedIntermediateLookahead += *regulatorTime.epsilon;
  require(
      !(*partiallyDecreasedLookahead == *changedLookahead),
      "the first grant left the old three-epsilon lookahead unchanged");
  require(
      !(*partiallyDecreasedLookahead == *regulatorTime.epsilon),
      "the first grant applied the full decrease before reaching its requested value");
  require(
      *partiallyDecreasedLookahead == *expectedIntermediateLookahead,
      "the decreased lookahead did not fall by the granted logical-time amount");

  auto const regulatorIntermediateTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      2U);
  auto const constrainedIntermediateTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      2U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorIntermediateTarget);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedIntermediateTarget);
  waitFor(
      regulator,
      constrained,
      [&] {
        return regulator.recorder().timeAdvanceGrants().size() >= 2U &&
            constrained.recorder().timeAdvanceGrants().size() >= 2U;
      },
      options,
      "Modify Lookahead intermediate gradual-decrease grant");
  auto const regulatorGrantsAfterIntermediate = regulator.recorder().timeAdvanceGrants();
  auto const constrainedGrantsAfterIntermediate = constrained.recorder().timeAdvanceGrants();
  require(
      regulatorGrantsAfterIntermediate.size() == 2U &&
          constrainedGrantsAfterIntermediate.size() == 2U,
      "Modify Lookahead intermediate grant was delivered more than once");
  require(
      regulatorGrantsAfterIntermediate.back().encoded ==
              encodeTime(*regulatorIntermediateTarget) &&
          constrainedGrantsAfterIntermediate.back().encoded ==
              encodeTime(*constrainedIntermediateTarget),
      "Modify Lookahead intermediate grant returned the wrong time");
  auto appliedLookahead = regulatorTime.factory->makeZero();
  require(
      appliedLookahead != nullptr,
      "Modify Lookahead could not allocate a final decreased query interval");
  regulator.rtiAmbassador().queryLookahead(*appliedLookahead);
  require(
      *appliedLookahead == *regulatorTime.epsilon,
      "the decreased lookahead did not reach its requested value after the second grant");

  regulator.rtiAmbassador().modifyLookahead(*regulatorTime.zero);
  auto queriedDeferredZero = regulatorTime.factory->makeZero();
  require(
      queriedDeferredZero != nullptr,
      "Modify Lookahead could not allocate a deferred zero query interval");
  regulator.rtiAmbassador().queryLookahead(*queriedDeferredZero);
  require(
      *queriedDeferredZero == *regulatorTime.epsilon,
      "a zero lookahead took effect before its next logical-time grant");

  auto const regulatorZeroTarget = timeAfter(
      *regulatorTime.factory,
      *regulatorTime.initial,
      *regulatorTime.epsilon,
      4U);
  auto const constrainedZeroTarget = timeAfter(
      *constrainedTime.factory,
      *constrainedTime.initial,
      *constrainedTime.epsilon,
      3U);
  regulator.rtiAmbassador().timeAdvanceRequest(*regulatorZeroTarget);
  constrained.rtiAmbassador().timeAdvanceRequest(*constrainedZeroTarget);
  waitFor(
      regulator,
      constrained,
      [&] {
        return regulator.recorder().timeAdvanceGrants().size() >= 3U &&
            constrained.recorder().timeAdvanceGrants().size() >= 3U;
      },
      options,
      "Modify Lookahead zero boundary grant");
  auto const regulatorZeroGrants = regulator.recorder().timeAdvanceGrants();
  auto const constrainedZeroGrants = constrained.recorder().timeAdvanceGrants();
  require(
      regulatorZeroGrants.size() == 3U && constrainedZeroGrants.size() == 3U,
      "Modify Lookahead zero boundary delivered duplicate grants");
  require(
      regulatorZeroGrants.back().encoded == encodeTime(*regulatorZeroTarget) &&
          constrainedZeroGrants.back().encoded == encodeTime(*constrainedZeroTarget),
      "Modify Lookahead zero boundary returned the wrong grant time");
  auto appliedZeroLookahead = regulatorTime.factory->makeZero();
  require(
      appliedZeroLookahead != nullptr,
      "Modify Lookahead could not allocate an applied zero query interval");
  regulator.rtiAmbassador().queryLookahead(*appliedZeroLookahead);
  require(
      *appliedZeroLookahead == *regulatorTime.zero,
      "the zero lookahead did not apply after the logical-time grant");

  constrained.rtiAmbassador().disableTimeConstrained();
  regulator.rtiAmbassador().disableTimeRegulation();
  constrained.resign(rti::NO_ACTION);
  regulator.resign(rti::NO_ACTION);
  regulator.rtiAmbassador().destroyFederationExecution(federation);
  constrained.disconnect();
  regulator.disconnect();
}

void scenarioModifyLookaheadContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioModifyLookahead(options, model);
}

void scenarioModifyLookaheadServiceReport(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.fom.empty() && !options.mimFom.empty(),
      "Modify Lookahead service-report testing requires adapter-supplied FOM and standard MIM modules");

  Session subject(options, model, "modify-lookahead-report-subject");
  Session observer(options, model, "modify-lookahead-report-observer");
  auto const federation = federationName(options, "modify-lookahead-service-report");
  subject.connect();
  observer.connect();
  subject.rtiAmbassador().createFederationExecutionWithMIM(
      federation,
      std::vector<std::wstring>{options.fom.wstring()},
      options.mimFom.wstring(),
      options.logicalTimeImplementationName);
  subject.join(
      options.ownerFederateName + L"-modify-lookahead-report-subject",
      options.federateType,
      federation);
  observer.join(
      options.memberFederateName + L"-modify-lookahead-report-observer",
      options.federateType,
      federation);

  auto const reportClass = observer.rtiAmbassador().getInteractionClassHandle(
      L"HLAinteractionRoot.HLAmanager.HLAfederate.HLAreport.HLAreportServiceInvocation");
  require(
      reportClass.isValid(),
      "Modify Lookahead report lookup returned an invalid HLAreportServiceInvocation handle");
  std::vector<rti::ParameterHandle> const reportParameters = [&] {
    std::vector<rti::ParameterHandle> result;
    for (auto const& name : {
             L"HLAservice",
             L"HLAserviceType",
             L"HLAsuccessIndicator",
             L"HLAsuppliedArguments",
             L"HLAreturnedArgument",
             L"HLAexception",
             L"HLAserialNumber",
             L"HLAfederate"}) {
      auto const parameter = observer.rtiAmbassador().getParameterHandle(
          reportClass,
          name);
      require(
          parameter.isValid(),
          "Modify Lookahead report lookup returned an invalid parameter handle");
      result.push_back(parameter);
    }
    return result;
  }();

  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().setServiceReportingSwitch(false);
  observer.rtiAmbassador().setSendServiceReportsToFileSwitch(false);
  observer.rtiAmbassador().subscribeInteractionClass(reportClass);

  auto const time = makeTimeContext(subject);
  auto const currentTime = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      3U);
  auto currentLookahead = time.factory->makeZero();
  currentLookahead->setToDifference(*currentTime, *time.initial);
  auto requestedTime = timeAfter(
      *time.factory,
      *time.initial,
      *time.epsilon,
      1U);
  auto requestedLookahead = time.factory->makeZero();
  requestedLookahead->setToDifference(*requestedTime, *time.initial);

  subject.rtiAmbassador().enableTimeRegulation(*currentLookahead);
  waitFor(
      subject,
      [&] { return subject.recorder().timeRegulationEnabled().size() >= 1U; },
      options,
      "Modify Lookahead service-report time-regulation callback");
  auto actualLookahead = time.factory->makeZero();
  subject.rtiAmbassador().queryLookahead(*actualLookahead);
  require(
      *actualLookahead == *currentLookahead,
      "Modify Lookahead service-report setup did not establish the initial interval");

  observer.recorder().clearInteraction();
  subject.rtiAmbassador().setServiceReportingSwitch(true);
  subject.rtiAmbassador().modifyLookahead(*requestedLookahead);
  auto deferredLookahead = time.factory->makeZero();
  subject.rtiAmbassador().queryLookahead(*deferredLookahead);
  require(
      *deferredLookahead == *currentLookahead,
      "accepted lower Modify Lookahead request changed the actual interval before logical-time advancement");

  waitFor(
      observer,
      [&] {
        for (auto const& candidate : observer.recorder().interactions()) {
          auto const serviceParameter = candidate.parameters.find(reportParameters[0]);
          if (serviceParameter == candidate.parameters.end()) {
            continue;
          }
          rti::HLAunicodeString candidateService;
          candidateService.decode(serviceParameter->second);
          if (candidateService.get() == L"ModifyLookahead") {
            return true;
          }
        }
        return false;
      },
      options,
      "ModifyLookahead HLAreportServiceInvocation callback");
  auto const reports = observer.recorder().interactions();
  auto const reportIterator = std::find_if(
      reports.begin(),
      reports.end(),
      [&](InteractionRecord const& candidate) {
        auto const serviceParameter = candidate.parameters.find(reportParameters[0]);
        if (serviceParameter == candidate.parameters.end()) {
          return false;
        }
        rti::HLAunicodeString candidateService;
        candidateService.decode(serviceParameter->second);
        return candidateService.get() == L"ModifyLookahead";
      });
  require(
      reportIterator != reports.end(),
      "ModifyLookahead service report was not recorded");
  auto const report = *reportIterator;
  auto const quoted = [](std::wstring const& value) {
    return std::wstring{L"\""} + value + L"\"";
  };
  requirePortableServiceReport(
      observer,
      report,
      reportClass,
      reportParameters,
      L"ModifyLookahead",
      {{32, L"Requested lookahead", quoted(requestedLookahead->toString())}},
      {34, L"", L"null"},
      0,
      "ModifyLookahead service report",
      4);
  require(
      observer.rtiAmbassador().decodeFederateHandle(
          report.parameters.at(reportParameters.at(7U))) == subject.federateHandle(),
      "ModifyLookahead service report identified the wrong joined federate");

  observer.rtiAmbassador().unsubscribeInteractionClass(reportClass);
  subject.rtiAmbassador().setServiceReportingSwitch(false);
  subject.rtiAmbassador().disableTimeRegulation();
  subject.resign(rti::NO_ACTION);
  observer.resign(rti::NO_ACTION);
  subject.rtiAmbassador().destroyFederationExecution(federation);
  observer.disconnect();
  subject.disconnect();
}

void scenarioModifyLookaheadServiceReportContractPortable(
    Options const& options,
    rti::CallbackModel model) {
  scenarioModifyLookaheadServiceReport(options, model);
}

void scenarioCustomTransportationTimestampedRegionalInteractionDelivery(
    Options const& options,
    rti::CallbackModel model) {
  require(
      !options.modelFom.empty(),
      "Custom transportation timestamped regional interaction testing requires an adapter-supplied rich FOM");
  require(
      !options.logicalTimeImplementationName.empty(),
      "Custom transportation timestamped regional interaction testing requires an adapter-supplied logical-time implementation");
  require(
      options.ddmDimensionNames.size() >= 2U,
      "Custom transportation timestamped regional interaction testing requires two adapter-supplied dimensions");

  auto regional = options;
  regional.ddmFom = regional.modelFom;
  regional.objectClassName = regional.customRegionalObjectClassName;
  regional.attributeName = regional.customRegionalAttributeName;
  regional.interactionClassName = regional.customRegionalInteractionClassName;
  regional.parameterName = regional.customRegionalParameterName;
  regional.requireCustomTransportation = true;

  // Keep the complete standard timestamped regional interaction oracle as the
  // first half of this extension. The focused probe below then verifies that
  // the same send-with-regions operation retains the adapter-declared custom
  // transportation identity.
  scenarioTimestampedRegionalInteraction(regional, model);

  Session producer(regional, model, "custom-transportation-timestamped-regional-producer");
  Session receiver(regional, model, "custom-transportation-timestamped-regional-receiver");
  auto const federation = federationName(
      regional,
      "custom-transportation-timestamped-regional-interaction-delivery");
  connectAndJoin(producer, receiver, regional, federation, regional.ddmFom);

  auto const producerHandles = ddmHandles(producer, regional);
  auto const receiverHandles = ddmHandles(receiver, regional);
  verifyDdmClassDimensions(
      producer,
      regional,
      producerHandles,
      "custom transportation timestamped regional interaction");
  auto const producerTransportation = producer.rtiAmbassador().getTransportationTypeHandle(
      regional.fomCustomTransportationName);
  auto const receiverTransportation = receiver.rtiAmbassador().getTransportationTypeHandle(
      regional.fomCustomTransportationName);
  require(
      producerTransportation.isValid() && receiverTransportation.isValid(),
      "custom transportation timestamped regional interaction lookup returned an invalid handle");
  require(
      producer.rtiAmbassador().getTransportationTypeName(producerTransportation) ==
              regional.fomCustomTransportationName &&
          receiver.rtiAmbassador().getTransportationTypeName(receiverTransportation) ==
              regional.fomCustomTransportationName,
      "custom transportation timestamped regional interaction name lookup did not round-trip");

  producer.rtiAmbassador().publishInteractionClass(producerHandles.interactionClass);
  producer.rtiAmbassador().changeInteractionOrderType(
      producerHandles.interactionClass,
      rti::TIMESTAMP);
  auto const sourceRegion = createDdmRegion(
      producer,
      producerHandles,
      1UL,
      5UL,
      1UL,
      5UL);
  auto const receiverRegion = createDdmRegion(
      receiver,
      receiverHandles,
      2UL,
      6UL,
      2UL,
      6UL);
  auto const sourceRegionSet = rti::RegionHandleSet{sourceRegion};
  auto const receiverRegionSet = rti::RegionHandleSet{receiverRegion};
  receiver.rtiAmbassador().setConveyRegionDesignatorSetsSwitch(true);
  require(
      receiver.rtiAmbassador().getConveyRegionDesignatorSetsSwitch(),
      "custom transportation timestamped regional interaction did not enable region designator callbacks");
  receiver.rtiAmbassador().subscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet,
      true);

  auto producerTime = makeTimeContext(producer);
  auto receiverTime = makeTimeContext(receiver);
  enableTimestampedRoles(
      producer,
      receiver,
      producerTime,
      receiverTime,
      regional,
      "custom transportation timestamped regional interaction");
  auto const messageTime = timeAfter(
      *producerTime.factory,
      *producerTime.initial,
      *producerTime.epsilon,
      3U);
  auto const receiverTarget = timeAfter(
      *receiverTime.factory,
      *receiverTime.initial,
      *receiverTime.epsilon,
      3U);
  std::vector<std::uint8_t> const parameterBytes{0xC5U, 0x52U, 0x49U};
  std::vector<std::uint8_t> const tagBytes{0x54U, 0x52U, 0x49U};
  rti::ParameterHandleValueMap parameters;
  parameters.emplace(
      producerHandles.parameter,
      rti::VariableLengthData(parameterBytes.data(), parameterBytes.size()));
  rti::VariableLengthData const tag(tagBytes.data(), tagBytes.size());
  auto const retraction = producer.rtiAmbassador().sendInteractionWithRegions(
      producerHandles.interactionClass,
      parameters,
      sourceRegionSet,
      tag,
      *messageTime);
  require(
      retraction.isValid(),
      "custom transportation timestamped regional interaction returned an invalid retraction handle");
  require(
      receiver.recorder().timedInteractions().empty(),
      "custom transportation timestamped regional interaction delivered before its grant");

  receiver.rtiAmbassador().timeAdvanceRequest(*receiverTarget);
  producer.rtiAmbassador().timeAdvanceRequest(*messageTime);
  waitForSessions(
      {&producer, &receiver},
      [&] {
        return producer.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timeAdvanceGrants().size() >= 1U &&
            receiver.recorder().timedInteractions().size() >= 1U;
      },
      regional,
      "custom transportation timestamped regional interaction delivery and grants");

  auto const deliveries = receiver.recorder().timedInteractions();
  require(
      deliveries.size() == 1U,
      "custom transportation timestamped regional interaction produced duplicate callbacks");
  auto const& delivery = deliveries.front();
  require(
      delivery.interaction == receiverHandles.interactionClass &&
          delivery.parameters.size() == 1U &&
          delivery.parameters.count(receiverHandles.parameter) == 1U &&
          copyBytes(delivery.parameters.at(receiverHandles.parameter)) == parameterBytes,
      "custom transportation timestamped regional interaction changed the payload");
  require(
      delivery.tag == tagBytes &&
          delivery.producer == producer.federateHandle() &&
          delivery.time == encodeTime(*messageTime) &&
          !delivery.timeText.empty() &&
          delivery.sentOrder == rti::TIMESTAMP &&
          delivery.receivedOrder == rti::TIMESTAMP &&
          delivery.retractionPresent &&
          delivery.retraction == copyBytes(retraction.encode()),
      "custom transportation timestamped regional interaction changed standard callback metadata");
  require(
      delivery.transportation.isValid() &&
          receiver.rtiAmbassador().getTransportationTypeName(delivery.transportation) ==
              regional.fomCustomTransportationName,
      "custom transportation timestamped regional interaction changed transportation identity");
  require(
      delivery.regions.has_value() &&
          delivery.regions->size() == 1U &&
          delivery.regions->count(sourceRegion) == 1U,
      "custom transportation timestamped regional interaction omitted the source region designator set");

  producer.rtiAmbassador().disableTimeRegulation();
  receiver.rtiAmbassador().disableTimeConstrained();
  receiver.rtiAmbassador().unsubscribeInteractionClassWithRegions(
      receiverHandles.interactionClass,
      receiverRegionSet);
  producer.rtiAmbassador().unpublishInteractionClass(producerHandles.interactionClass);
  producer.rtiAmbassador().deleteRegion(sourceRegion);
  receiver.rtiAmbassador().deleteRegion(receiverRegion);
  receiver.resign(rti::NO_ACTION);
  producer.resign(rti::NO_ACTION);
  producer.rtiAmbassador().destroyFederationExecution(federation);
  receiver.disconnect();
  producer.disconnect();
}

void scenarioCustomTransportationTimestampedRegionalInteractionDeliveryContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioCustomTransportationTimestampedRegionalInteractionDelivery(options, model);
}

void scenarioDelaySubscriptionEvaluationInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationDirectedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationDirectedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationAttributeUpdate(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedDirectedInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdate(
      options,
      model);
}

void scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate(options, model);
}

void scenarioDelaySubscriptionEvaluationTimestampedRegionalInteractionContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioDelaySubscriptionEvaluationTimestampedRegionalInteraction(
      options,
      model);
}

void scenarioFederationSaveRestoreInterlocksContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFederationSaveRestoreInterlocks(options, model);
}

void scenarioTimedFederationSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedFederationSaveRestore(options, model);
}

void scenarioFqrTimedFederationSaveCallbackContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioFqrTimedFederationSaveCallback(options, model);
}

void scenarioTimedRegionalInteractionSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedRegionalInteractionSaveRestore(options, model);
}

void scenarioTimedDefaultRegionInteractionSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedDefaultRegionInteractionSaveRestore(options, model);
}

void scenarioTimedDefaultRegionAttributeSaveRestoreContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioTimedDefaultRegionAttributeSaveRestore(options, model);
}

void scenarioRegionalMultiAttributeUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalMultiAttributeUpdate(options, model);
}

void scenarioRegionalThreeDimensionalOverlapContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalThreeDimensionalOverlap(options, model);
}

void scenarioAllowRelaxedDdmContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioAllowRelaxedDdm(options, model);
}

void scenarioRegionalBoundariesContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioRegionalBoundaries(options, model);
}

void scenarioOwnershipTransferRegionalUpdateContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioOwnershipTransferRegionalUpdate(options, model);
}

void scenarioMomTransportationTypeChangeRequestContract(
    Options const& options,
    rti::CallbackModel model) {
  scenarioMomTransportationTypeChangeRequest(options, model);
}

using PortableScenario = void (*)(Options const&, rti::CallbackModel);

int runPortableScenarioPair(
    int argc,
    char** argv,
    char const* scenarioId,
    char const* contractId,
    PortableScenario scenario,
    PortableScenario contract) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != scenarioId && selected != contractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      try {
        if (selected == scenarioId) {
          scenario(options, callback.second);
        } else {
          contract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runModifyLookaheadScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      modifyLookaheadScenario,
      modifyLookaheadContractId,
      scenarioModifyLookahead,
      scenarioModifyLookaheadContract);
}

int runModifyLookaheadServiceReportScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      modifyLookaheadServiceReportContractId,
      modifyLookaheadServiceReportContractId,
      scenarioModifyLookaheadServiceReportContractPortable,
      scenarioModifyLookaheadServiceReportContractPortable);
}

int runTransportationTypeChangeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      transportationTypeChangeScenario,
      transportationTypeChangeContractId,
      scenarioTransportationTypeChange,
      scenarioTransportationTypeChangeContract);
}

int runOrderTypeChangeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      orderTypeChangeScenario,
      orderTypeChangeContractId,
      scenarioOrderTypeChange,
      scenarioOrderTypeChangeContract);
}

int runTransportOrderScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      transportOrderScenario,
      transportOrderContractId,
      scenarioTransportOrderPortable,
      scenarioTransportOrderPortableContract);
}

int runStandardOrderAndTransportationLookupScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      standardOrderAndTransportationLookupsScenario,
      standardOrderAndTransportationLookupsContractId,
      scenarioStandardOrderAndTransportationLookupsPortable,
      scenarioStandardOrderAndTransportationLookupsPortableContract);
}

int runFactoryDiscoveryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      factoryDiscoveryScenario,
      rtiAmbassadorFactoryContractScenario,
      scenarioFactoryDiscoveryPortable,
      scenarioRTIambassadorFactoryPortableContract);
}

int runLogicalTimeFactoryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      logicalTimeFactoryScenario,
      logicalTimeFactoryScenario,
      scenarioLogicalTimeFactoryPortable,
      scenarioLogicalTimeFactoryPortable);
}

int runVariableLengthDataScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      variableLengthDataContractScenario,
      variableLengthDataContractScenario,
      scenarioVariableLengthDataPortable,
      scenarioVariableLengthDataPortable);
}

int runLogicalTimeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      logicalTimeContractScenario,
      logicalTimeContractScenario,
      scenarioLogicalTimePortable,
      scenarioLogicalTimePortable);
}

int runLogicalTimeFactoryFactoryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      logicalTimeFactoryFactoryContractScenario,
      logicalTimeFactoryFactoryContractScenario,
      scenarioLogicalTimeFactoryFactoryPortable,
      scenarioLogicalTimeFactoryFactoryPortable);
}

int runLogicalTimeDataElementsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      logicalTimeDataElementsContractScenario,
      logicalTimeDataElementsContractScenario,
      scenarioLogicalTimeDataElementsPortable,
      scenarioLogicalTimeDataElementsPortable);
}

int runExceptionHierarchyScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      exceptionHierarchyContractScenario,
      exceptionHierarchyContractScenario,
      scenarioExceptionHierarchyPortable,
      scenarioExceptionHierarchyPortable);
}

int runEnumScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      enumContractScenario,
      enumContractScenario,
      scenarioEnumPortable,
      scenarioEnumPortable);
}

int runHandleAndCollectionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      handleAndCollectionContractScenario,
      handleAndCollectionContractScenario,
      scenarioHandleAndCollectionPortable,
      scenarioHandleAndCollectionPortable);
}

int runDataElementScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      dataElementContractScenario,
      dataElementContractScenario,
      scenarioDataElementPortable,
      scenarioDataElementPortable);
}

int runBasicDataElementsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      basicDataElementsContractScenario,
      basicDataElementsContractScenario,
      scenarioBasicDataElementsPortable,
      scenarioBasicDataElementsPortable);
}

int runCompositeDataElementsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      compositeDataElementsContractScenario,
      compositeDataElementsContractScenario,
      scenarioCompositeDataElementsPortable,
      scenarioCompositeDataElementsPortable);
}

int runConfigurationAndAuthorizationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      configurationAndAuthorizationContractScenario,
      configurationAndAuthorizationContractScenario,
      scenarioConfigurationAndAuthorizationPortable,
      scenarioConfigurationAndAuthorizationPortable);
}

int runRuntimeIdentityScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      runtimeIdentityContractScenario,
      runtimeIdentityContractScenario,
      scenarioRuntimeIdentityPortable,
      scenarioRuntimeIdentityPortable);
}

int runAuthorizerFactoryFactoryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      authorizerFactoryFactoryContractScenario,
      authorizerFactoryFactoryContractScenario,
      scenarioAuthorizerFactoryFactoryPortable,
      scenarioAuthorizerFactoryFactoryPortable);
}

int runNullFederateAmbassadorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nullFederateAmbassadorContractScenario,
      nullFederateAmbassadorContractScenario,
      scenarioNullFederateAmbassadorPortable,
      scenarioNullFederateAmbassadorPortable);
}

int runConnectionCallbackFoundationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      connectionScenario,
      connectionCallbackContractScenario,
      scenarioConnectionPortable,
      scenarioConnectionCallbackContractPortable);
}

int runCallbackReentrancyScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      callbackReentrancyScenario,
      callbackReentrancyContractScenario,
      scenarioCallbackReentrancyPortable,
      scenarioCallbackReentrancyContractPortable);
}

int runMalformedInputsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      malformedInputsScenario,
      malformedInputsContractScenario,
      scenarioMalformedInputsPortable,
      scenarioMalformedInputsContractPortable);
}

int runFederationLifecycleScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationLifecycleScenario,
      federationLifecycleContractScenario,
      scenarioFederationLifecyclePortable,
      scenarioFederationLifecycleContractPortable);
}

int runFederationManagementRequiresConnectionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationManagementRequiresConnectionScenario,
      federationManagementRequiresConnectionScenario,
      scenarioFederationManagementRequiresConnectionPortable,
      scenarioFederationManagementRequiresConnectionPortable);
}

int runUnnamedJoinOverloadScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      unnamedJoinOverloadScenario,
      unnamedJoinOverloadContractScenario,
      scenarioUnnamedJoinOverloadPortable,
      scenarioUnnamedJoinOverloadContractPortable);
}

int runFederationListServicesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationListServicesScenario,
      federationListServicesContractScenario,
      scenarioFederationListServicesPortable,
      scenarioFederationListServicesContractPortable);
}

int runFederateLookupLifecycleScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateLookupLifecycleScenario,
      federateLookupLifecycleContractScenario,
      scenarioFederateLookupLifecyclePortable,
      scenarioFederateLookupLifecycleContractPortable);
}

int runExplicitMimCreationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      explicitMimCreationScenario,
      explicitMimCreationContractScenario,
      scenarioExplicitMimCreationPortable,
      scenarioExplicitMimCreationContractPortable);
}

int runFederationMomCurrentFddScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomCurrentFddScenario,
      federationMomCurrentFddContractScenario,
      scenarioFederationMomCurrentFddPortable,
      scenarioFederationMomCurrentFddContractPortable);
}

int runFederationMomStaticIdentityScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomStaticIdentityScenario,
      federationMomStaticIdentityContractScenario,
      scenarioFederationMomStaticIdentity,
      scenarioFederationMomStaticIdentityContract);
}

int runFederationMomStaticSwitchesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomStaticSwitchesScenario,
      federationMomStaticSwitchesContractScenario,
      scenarioFederationMomStaticSwitches,
      scenarioFederationMomStaticSwitchesContract);
}

int runFederationMomFederatesInFederationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomFederatesInFederationScenario,
      federationMomFederatesInFederationContractScenario,
      scenarioFederationMomFederatesInFederation,
      scenarioFederationMomFederatesInFederationContract);
}

int runFederationMomFomModuleDesignatorListScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomFomModuleDesignatorListScenario,
      federationMomFomModuleDesignatorListContractScenario,
      scenarioFederationMomFomModuleDesignatorListPortable,
      scenarioFederationMomFomModuleDesignatorListContractPortable);
}

int runFederationMomContentReportsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomContentReportsScenario,
      federationMomContentReportsContractScenario,
      scenarioFederationMomContentReportsPortable,
      scenarioFederationMomContentReportsContractPortable);
}

int runServiceReportOrderTransportationLookupsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportOrderTransportationLookupsScenario,
      serviceReportOrderTransportationLookupsContractScenario,
      scenarioServiceReportOrderTransportationLookupsPortable,
      scenarioServiceReportOrderTransportationLookupsContractPortable);
}

int runServiceReportDimensionUpperBoundScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDimensionUpperBoundScenario,
      serviceReportDimensionUpperBoundContractScenario,
      scenarioServiceReportDimensionUpperBoundPortable,
      scenarioServiceReportDimensionUpperBoundContractPortable);
}

int runServiceReportDimensionHandleSetScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDimensionHandleSetScenario,
      serviceReportDimensionHandleSetContractScenario,
      scenarioServiceReportDimensionHandleSetPortable,
      scenarioServiceReportDimensionHandleSetContractPortable);
}

int runServiceReportRangeBoundsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportRangeBoundsScenario,
      serviceReportRangeBoundsContractScenario,
      scenarioServiceReportRangeBoundsPortable,
      scenarioServiceReportRangeBoundsContractPortable);
}

int runServiceReportSetRangeBoundsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportSetRangeBoundsScenario,
      serviceReportSetRangeBoundsContractScenario,
      scenarioServiceReportSetRangeBoundsPortable,
      scenarioServiceReportSetRangeBoundsContractPortable);
}

int runServiceReportCommitRegionModificationsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportCommitRegionModificationsScenario,
      serviceReportCommitRegionModificationsContractScenario,
      scenarioServiceReportCommitRegionModificationsPortable,
      scenarioServiceReportCommitRegionModificationsContractPortable);
}

int runServiceReportDeleteRegionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDeleteRegionScenario,
      serviceReportDeleteRegionContractScenario,
      scenarioServiceReportDeleteRegionPortable,
      scenarioServiceReportDeleteRegionContractPortable);
}

int runServiceReportCreateRegionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportCreateRegionScenario,
      serviceReportCreateRegionContractScenario,
      scenarioServiceReportCreateRegionPortable,
      scenarioServiceReportCreateRegionContractPortable);
}

int runRegionRangeValidationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionRangeValidationScenario,
      regionRangeValidationContractScenario,
      scenarioRegionRangeValidationPortable,
      scenarioRegionRangeValidationContractPortable);
}

int runRegionRangeDimensionValidationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionRangeDimensionValidationScenario,
      regionRangeDimensionValidationContractScenario,
      scenarioRegionRangeDimensionValidationPortable,
      scenarioRegionRangeDimensionValidationContractPortable);
}

int runServiceReportDimensionNameScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDimensionNameScenario,
      serviceReportDimensionNameContractScenario,
      scenarioServiceReportDimensionNamePortable,
      scenarioServiceReportDimensionNameContractPortable);
}

int runServiceReportDimensionHandleScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDimensionHandleScenario,
      serviceReportDimensionHandleContractScenario,
      scenarioServiceReportDimensionHandlePortable,
      scenarioServiceReportDimensionHandleContractPortable);
}

int runServiceReportInteractionDimensionsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionDimensionsScenario,
      serviceReportInteractionDimensionsContractScenario,
      scenarioServiceReportInteractionDimensionsPortable,
      scenarioServiceReportInteractionDimensionsContractPortable);
}

int runServiceReportObjectDimensionsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportObjectDimensionsScenario,
      serviceReportObjectDimensionsContractScenario,
      scenarioServiceReportObjectDimensionsPortable,
      scenarioServiceReportObjectDimensionsContractPortable);
}

int runServiceReportOrderTransportationLookupFailuresScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportOrderTransportationLookupFailuresScenario,
      serviceReportOrderTransportationLookupFailuresContractScenario,
      scenarioServiceReportOrderTransportationLookupFailuresPortable,
      scenarioServiceReportOrderTransportationLookupFailuresContractPortable);
}

int runServiceReportFederateObjectClassLookupsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportFederateObjectClassLookupsScenario,
      serviceReportFederateObjectClassLookupsContractScenario,
      scenarioServiceReportFederateObjectClassLookupsPortable,
      scenarioServiceReportFederateObjectClassLookupsContractPortable);
}

int runServiceReportFederateObjectClassLookupFailuresScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportFederateObjectClassLookupFailuresScenario,
      serviceReportFederateObjectClassLookupFailuresContractScenario,
      scenarioServiceReportFederateObjectClassLookupFailuresPortable,
      scenarioServiceReportFederateObjectClassLookupFailuresContractPortable);
}

int runServiceReportInteractionParameterLookupsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionParameterLookupsScenario,
      serviceReportInteractionParameterLookupsContractScenario,
      scenarioServiceReportInteractionParameterLookupsPortable,
      scenarioServiceReportInteractionParameterLookupsContractPortable);
}

int runServiceReportInteractionParameterLookupFailuresScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionParameterLookupFailuresScenario,
      serviceReportInteractionParameterLookupFailuresContractScenario,
      scenarioServiceReportInteractionParameterLookupFailuresPortable,
      scenarioServiceReportInteractionParameterLookupFailuresContractPortable);
}

int runServiceReportInteractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionScenario,
      serviceReportInteractionContractScenario,
      scenarioServiceReportInteractionPortable,
      scenarioServiceReportInteractionContractPortable);
}

int runServiceReportResignFederationExecutionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportResignFederationExecutionScenario,
      serviceReportResignFederationExecutionContractScenario,
      scenarioServiceReportResignFederationExecutionPortable,
      scenarioServiceReportResignFederationExecutionContractPortable);
}

int runFederateLostMomReportScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateLostReportScenario,
      federateLostReportContractScenario,
      scenarioFederateLostMomReportPortable,
      scenarioFederateLostMomReportContractPortable);
}

int runHandleNormalizationMembershipBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      handleNormalizationMembershipBoundariesScenario,
      handleNormalizationMembershipBoundariesContractScenario,
      scenarioHandleNormalizationMembershipBoundariesPortable,
      scenarioHandleNormalizationMembershipBoundariesContractPortable);
}

int runDimensionLookupValidHandlesAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      dimensionLookupValidHandlesAfterResignationScenario,
      dimensionLookupValidHandlesAfterResignationContractScenario,
      scenarioDimensionLookupValidHandlesAfterResignationPortable,
      scenarioDimensionLookupValidHandlesAfterResignationContractPortable);
}

int runUpdateRateLookupValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      updateRateLookupValidInputsAfterResignationScenario,
      updateRateLookupValidInputsAfterResignationContractScenario,
      scenarioUpdateRateLookupValidInputsAfterResignationPortable,
      scenarioUpdateRateLookupValidInputsAfterResignationContractPortable);
}

int runSupportLookupValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      supportLookupValidInputsAfterResignationScenario,
      supportLookupValidInputsAfterResignationContractScenario,
      scenarioSupportLookupValidInputsAfterResignationPortable,
      scenarioSupportLookupValidInputsAfterResignationContractPortable);
}

int runSupportSwitchGetterLifecycleBoundariesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      supportSwitchGetterLifecycleBoundariesScenario,
      supportSwitchGetterLifecycleBoundariesContractScenario,
      scenarioSupportSwitchGetterLifecycleBoundariesPortable,
      scenarioSupportSwitchGetterLifecycleBoundariesContractPortable);
}

int runSupportSwitchSetterLifecycleBoundariesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      supportSwitchSetterLifecycleBoundariesScenario,
      supportSwitchSetterLifecycleBoundariesContractScenario,
      scenarioSupportSwitchSetterLifecycleBoundariesPortable,
      scenarioSupportSwitchSetterLifecycleBoundariesContractPortable);
}

int runAutomaticResignDirectiveLifecycleBoundariesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      automaticResignDirectiveLifecycleBoundariesScenario,
      automaticResignDirectiveLifecycleBoundariesContractScenario,
      scenarioAutomaticResignDirectiveLifecycleBoundariesPortable,
      scenarioAutomaticResignDirectiveLifecycleBoundariesContractPortable);
}

int runAutomaticResignDirectiveEnumValuesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      automaticResignDirectiveEnumValuesScenario,
      automaticResignDirectiveEnumValuesContractScenario,
      scenarioAutomaticResignDirectiveEnumValuesPortable,
      scenarioAutomaticResignDirectiveEnumValuesContractPortable);
}

int runAdvisorySwitchValueRoundTripsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      advisorySwitchValueRoundTripsScenario,
      advisorySwitchValueRoundTripsContractScenario,
      scenarioAdvisorySwitchValueRoundTripsPortable,
      scenarioAdvisorySwitchValueRoundTripsContractPortable);
}

int runDuplicateFederateNameJoinBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      duplicateFederateNameJoinScenario,
      duplicateFederateNameJoinContractScenario,
      scenarioDuplicateFederateNameJoinBoundaryPortable,
      scenarioDuplicateFederateNameJoinBoundaryContractPortable);
}

int runRepeatedJoinByMemberBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      repeatedJoinByMemberScenario,
      repeatedJoinByMemberContractScenario,
      scenarioRepeatedJoinByMemberBoundaryPortable,
      scenarioRepeatedJoinByMemberBoundaryContractPortable);
}

int runForeignFederateNameLookupBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      foreignFederateNameLookupScenario,
      foreignFederateNameLookupContractScenario,
      scenarioForeignFederateNameLookupBoundaryPortable,
      scenarioForeignFederateNameLookupBoundaryContractPortable);
}

int runForeignObjectInstanceNameLookupBoundaryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      foreignObjectInstanceNameLookupScenario,
      foreignObjectInstanceNameLookupContractScenario,
      scenarioForeignObjectInstanceNameLookupBoundaryPortable,
      scenarioForeignObjectInstanceNameLookupBoundaryContractPortable);
}

int runForeignObjectInstanceHandleLookupBoundaryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      foreignObjectInstanceHandleLookupScenario,
      foreignObjectInstanceHandleLookupContractScenario,
      scenarioForeignObjectInstanceHandleLookupBoundaryPortable,
      scenarioForeignObjectInstanceHandleLookupBoundaryContractPortable);
}

int runLocalDeleteObjectInstanceNameLookupBoundaryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      localDeleteObjectInstanceNameLookupScenario,
      localDeleteObjectInstanceNameLookupContractScenario,
      scenarioLocalDeleteObjectInstanceNameLookupBoundaryPortable,
      scenarioLocalDeleteObjectInstanceNameLookupBoundaryContractPortable);
}

int runRemoteDeleteKnownObjectClassLookupBoundaryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      remoteDeleteKnownObjectClassLookupScenario,
      remoteDeleteKnownObjectClassLookupContractScenario,
      scenarioRemoteDeleteKnownObjectClassLookupBoundaryPortable,
      scenarioRemoteDeleteKnownObjectClassLookupBoundaryContractPortable);
}

int runRemoteDeleteObjectInstanceNameLookupBoundaryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      remoteDeleteObjectInstanceNameLookupScenario,
      remoteDeleteObjectInstanceNameLookupContractScenario,
      scenarioRemoteDeleteObjectInstanceNameLookupBoundaryPortable,
      scenarioRemoteDeleteObjectInstanceNameLookupBoundaryContractPortable);
}

int runResignDeleteObjectLookupBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignDeleteObjectLookupScenario,
      resignDeleteObjectLookupContractScenario,
      scenarioResignDeleteObjectLookupBoundaryPortable,
      scenarioResignDeleteObjectLookupBoundaryContractPortable);
}

int runResignNoActionOwnedAttributesBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignNoActionOwnedAttributesScenario,
      resignNoActionOwnedAttributesContractScenario,
      scenarioResignNoActionOwnedAttributesBoundaryPortable,
      scenarioResignNoActionOwnedAttributesBoundaryContractPortable);
}

int runResignDeleteObjectsThenDivestCrossObjectEffectsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignDeleteObjectsThenDivestCrossObjectEffectsScenario,
      resignDeleteObjectsThenDivestCrossObjectEffectsContractScenario,
      scenarioResignDeleteObjectsThenDivestCrossObjectEffectsPortable,
      scenarioResignDeleteObjectsThenDivestCrossObjectEffectsContractPortable);
}

int runResignUnconditionallyDivestAttributesPreservesObjectsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignUnconditionallyDivestAttributesPreservesObjectsScenario,
      resignUnconditionallyDivestAttributesPreservesObjectsContractScenario,
      scenarioResignUnconditionallyDivestAttributesPreservesObjectsPortable,
      scenarioResignUnconditionallyDivestAttributesPreservesObjectsContractPortable);
}

int runResignCancelPendingOwnershipAcquisitionsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignCancelPendingOwnershipAcquisitionsScenario,
      resignCancelPendingOwnershipAcquisitionsContractScenario,
      scenarioResignCancelPendingOwnershipAcquisitionsPortable,
      scenarioResignCancelPendingOwnershipAcquisitionsContractPortable);
}

int runResignCancelThenDeleteThenDivestCrossObjectEffectsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      resignCancelThenDeleteThenDivestCrossObjectEffectsScenario,
      resignCancelThenDeleteThenDivestCrossObjectEffectsContractScenario,
      scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsPortable,
      scenarioResignCancelThenDeleteThenDivestCrossObjectEffectsContractPortable);
}

int runStandardOrderTransportLookupsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      standardOrderTransportLookupsAfterResignationScenario,
      standardOrderTransportLookupsAfterResignationContractScenario,
      scenarioStandardOrderTransportLookupsAfterResignationPortable,
      scenarioStandardOrderTransportLookupsAfterResignationContractPortable);
}

int runObjectAttributeDeclarationsValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectAttributeDeclarationsValidInputsAfterResignationScenario,
      objectAttributeDeclarationsValidInputsAfterResignationContractScenario,
      scenarioObjectAttributeDeclarationsValidInputsAfterResignationPortable,
      scenarioObjectAttributeDeclarationsValidInputsAfterResignationContractPortable);
}

int runInteractionDeclarationsValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionDeclarationsValidInputsAfterResignationScenario,
      interactionDeclarationsValidInputsAfterResignationContractScenario,
      scenarioInteractionDeclarationsValidInputsAfterResignationPortable,
      scenarioInteractionDeclarationsValidInputsAfterResignationContractPortable);
}

int runDirectedInteractionDeclarationsValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      directedInteractionDeclarationsValidInputsAfterResignationScenario,
      directedInteractionDeclarationsValidInputsAfterResignationContractScenario,
      scenarioDirectedInteractionDeclarationsValidInputsAfterResignationPortable,
      scenarioDirectedInteractionDeclarationsValidInputsAfterResignationContractPortable);
}

int runInteractionTransportationQueryValidInputsAfterResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionTransportationQueryValidInputsAfterResignationScenario,
      interactionTransportationQueryValidInputsAfterResignationContractScenario,
      scenarioInteractionTransportationQueryValidInputsAfterResignationPortable,
      scenarioInteractionTransportationQueryValidInputsAfterResignationContractPortable);
}

int runServiceReportReceiveOrderInteractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportReceiveOrderInteractionScenario,
      serviceReportReceiveOrderInteractionContractScenario,
      scenarioServiceReportReceiveOrderInteractionPortable,
      scenarioServiceReportReceiveOrderInteractionContractPortable);
}

int runServiceReportTimestampedDirectedInteractionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedDirectedInteractionScenario,
      serviceReportTimestampedDirectedInteractionContractScenario,
      scenarioServiceReportTimestampedDirectedInteractionPortable,
      scenarioServiceReportTimestampedDirectedInteractionContractPortable);
}

int runServiceReportObjectAttributeUpdateRateLookupsScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportObjectAttributeUpdateRateLookupsScenario,
      serviceReportObjectAttributeUpdateRateLookupsContractScenario,
      scenarioServiceReportObjectAttributeUpdateRateLookupsPortable,
      scenarioServiceReportObjectAttributeUpdateRateLookupsContractPortable);
}

int runServiceReportObjectAttributeUpdateRateLookupFailuresScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportObjectAttributeUpdateRateLookupFailuresScenario,
      serviceReportObjectAttributeUpdateRateLookupFailuresContractScenario,
      scenarioServiceReportObjectAttributeUpdateRateLookupFailuresPortable,
      scenarioServiceReportObjectAttributeUpdateRateLookupFailuresContractPortable);
}

int runServiceReportObjectAttributeDeclarationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportObjectAttributeDeclarationScenario,
      serviceReportObjectAttributeDeclarationContractScenario,
      scenarioServiceReportObjectAttributeDeclarationPortable,
      scenarioServiceReportObjectAttributeDeclarationContractPortable);
}

int runServiceReportDirectedInteractionDeclarationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDirectedInteractionDeclarationScenario,
      serviceReportDirectedInteractionDeclarationContractScenario,
      scenarioServiceReportDirectedInteractionDeclarationPortable,
      scenarioServiceReportDirectedInteractionDeclarationContractPortable);
}

int runServiceReportDirectedInteractionPublicationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDirectedInteractionPublicationScenario,
      serviceReportDirectedInteractionPublicationContractScenario,
      scenarioServiceReportDirectedInteractionPublicationPortable,
      scenarioServiceReportDirectedInteractionPublicationContractPortable);
}

int runServiceReportInteractionPublicationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionPublicationScenario,
      serviceReportInteractionPublicationContractScenario,
      scenarioServiceReportInteractionPublicationPortable,
      scenarioServiceReportInteractionPublicationContractPortable);
}

int runServiceReportInteractionSubscriptionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionSubscriptionScenario,
      serviceReportInteractionSubscriptionContractScenario,
      scenarioServiceReportInteractionSubscriptionPortable,
      scenarioServiceReportInteractionSubscriptionContractPortable);
}

int runServiceReportAttributeUpdateScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportAttributeUpdateScenario,
      serviceReportAttributeUpdateContractScenario,
      scenarioServiceReportAttributeUpdatePortable,
      scenarioServiceReportAttributeUpdateContractPortable);
}

int runServiceReportRequestAttributeValueUpdateScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportRequestAttributeValueUpdateScenario,
      serviceReportRequestAttributeValueUpdateContractScenario,
      scenarioServiceReportRequestAttributeValueUpdatePortable,
      scenarioServiceReportRequestAttributeValueUpdateContractPortable);
}

int runServiceReportRegisterObjectInstanceScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportRegisterObjectInstanceScenario,
      serviceReportRegisterObjectInstanceContractScenario,
      scenarioServiceReportRegisterObjectInstancePortable,
      scenarioServiceReportRegisterObjectInstanceContractPortable);
}

int runServiceReportDeleteObjectInstanceScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDeleteObjectInstanceScenario,
      serviceReportDeleteObjectInstanceContractScenario,
      scenarioServiceReportDeleteObjectInstancePortable,
      scenarioServiceReportDeleteObjectInstanceContractPortable);
}

int runServiceReportInteractionFailureScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInteractionFailureScenario,
      serviceReportInteractionFailureContractScenario,
      scenarioServiceReportInteractionFailurePortable,
      scenarioServiceReportInteractionFailureContractPortable);
}

int runServiceReportAttributeUpdateFailureScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportAttributeUpdateFailureScenario,
      serviceReportAttributeUpdateFailureContractScenario,
      scenarioServiceReportAttributeUpdateFailurePortable,
      scenarioServiceReportAttributeUpdateFailureContractPortable);
}

int runServiceReportDeleteObjectInstanceFailureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportDeleteObjectInstanceFailureScenario,
      serviceReportDeleteObjectInstanceFailureContractScenario,
      scenarioServiceReportDeleteObjectInstanceFailurePortable,
      scenarioServiceReportDeleteObjectInstanceFailureContractPortable);
}

int runServiceReportReserveObjectInstanceNameScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportReserveObjectInstanceNameScenario,
      serviceReportReserveObjectInstanceNameContractScenario,
      scenarioServiceReportReserveObjectInstanceNamePortable,
      scenarioServiceReportReserveObjectInstanceNameContractPortable);
}

int runServiceReportReleaseObjectInstanceNameScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportReleaseObjectInstanceNameScenario,
      serviceReportReleaseObjectInstanceNameContractScenario,
      scenarioServiceReportReleaseObjectInstanceNamePortable,
      scenarioServiceReportReleaseObjectInstanceNameContractPortable);
}

int runServiceReportReleaseMultipleObjectInstanceNamesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportReleaseMultipleObjectInstanceNamesScenario,
      serviceReportReleaseMultipleObjectInstanceNamesContractScenario,
      scenarioServiceReportReleaseMultipleObjectInstanceNamesPortable,
      scenarioServiceReportReleaseMultipleObjectInstanceNamesContractPortable);
}

int runServiceReportQueryAttributeOwnershipScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportQueryAttributeOwnershipScenario,
      serviceReportQueryAttributeOwnershipContractScenario,
      scenarioServiceReportQueryAttributeOwnershipPortable,
      scenarioServiceReportQueryAttributeOwnershipContractPortable);
}

int runServiceReportConfirmDivestitureScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportConfirmDivestitureScenario,
      serviceReportConfirmDivestitureContractScenario,
      scenarioServiceReportConfirmDivestiturePortable,
      scenarioServiceReportConfirmDivestitureContractPortable);
}

int runServiceReportCancelAttributeOwnershipAcquisitionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportCancelAttributeOwnershipAcquisitionScenario,
      serviceReportCancelAttributeOwnershipAcquisitionContractScenario,
      scenarioServiceReportCancelAttributeOwnershipAcquisitionPortable,
      scenarioServiceReportCancelAttributeOwnershipAcquisitionContractPortable);
}

int runServiceReportLocalDeleteObjectInstanceScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportLocalDeleteObjectInstanceScenario,
      serviceReportLocalDeleteObjectInstanceContractScenario,
      scenarioServiceReportLocalDeleteObjectInstancePortable,
      scenarioServiceReportLocalDeleteObjectInstanceContractPortable);
}

int runServiceReportLocalDeleteObjectInstanceFailureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportLocalDeleteObjectInstanceFailureScenario,
      serviceReportLocalDeleteObjectInstanceFailureContractScenario,
      scenarioServiceReportLocalDeleteObjectInstanceFailurePortable,
      scenarioServiceReportLocalDeleteObjectInstanceFailureContractPortable);
}

int runServiceReportInterlockScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportInterlockScenario,
      serviceReportInterlockContractScenario,
      scenarioServiceReportInterlockPortable,
      scenarioServiceReportInterlockContractPortable);
}

int runServiceReportSynchronizationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportSynchronizationScenario,
      serviceReportSynchronizationContractScenario,
      scenarioServiceReportSynchronizationPortable,
      scenarioServiceReportSynchronizationContractPortable);
}

int runServiceReportFederateSaveBegunScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportFederateSaveBegunScenario,
      serviceReportFederateSaveBegunContractScenario,
      scenarioServiceReportFederateSaveBegunPortable,
      scenarioServiceReportFederateSaveBegunContractPortable);
}

int runServiceReportTimestampedAttributeUpdateScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedAttributeUpdateScenario,
      serviceReportTimestampedAttributeUpdateContractScenario,
      scenarioServiceReportTimestampedAttributeUpdatePortable,
      scenarioServiceReportTimestampedAttributeUpdateContractPortable);
}

int runServiceReportTimestampedDeleteObjectInstanceScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedDeleteObjectInstanceScenario,
      serviceReportTimestampedDeleteObjectInstanceContractScenario,
      scenarioServiceReportTimestampedDeleteObjectInstancePortable,
      scenarioServiceReportTimestampedDeleteObjectInstanceContractPortable);
}

int runServiceReportTimestampedInteractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedInteractionScenario,
      serviceReportTimestampedInteractionContractScenario,
      scenarioServiceReportTimestampedInteractionPortable,
      scenarioServiceReportTimestampedInteractionContractPortable);
}

int runServiceReportTimestampedAttributeUpdateFailureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedAttributeUpdateFailureScenario,
      serviceReportTimestampedAttributeUpdateFailureContractScenario,
      scenarioServiceReportTimestampedAttributeUpdateFailurePortable,
      scenarioServiceReportTimestampedAttributeUpdateFailureContractPortable);
}

int runServiceReportTimestampedInteractionFailureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedInteractionFailureScenario,
      serviceReportTimestampedInteractionFailureContractScenario,
      scenarioServiceReportTimestampedInteractionFailurePortable,
      scenarioServiceReportTimestampedInteractionFailureContractPortable);
}

int runServiceReportTimestampedDeleteObjectInstanceFailureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      serviceReportTimestampedDeleteObjectInstanceFailureScenario,
      serviceReportTimestampedDeleteObjectInstanceFailureContractScenario,
      scenarioServiceReportTimestampedDeleteObjectInstanceFailurePortable,
      scenarioServiceReportTimestampedDeleteObjectInstanceFailureContractPortable);
}

int runSynchronizationPointScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != javaSynchronizationScenario &&
          selected != synchronizationPointsScenario &&
          selected != synchronizationPointContractScenario &&
          selected != synchronizationExplicitSetLateJoinScenario &&
          selected != synchronizationExplicitSetLateJoinContractScenario) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      try {
        if (selected == synchronizationExplicitSetLateJoinScenario) {
          scenarioSynchronizationExplicitSetLateJoinPortable(options, callback.second);
        } else if (selected == synchronizationExplicitSetLateJoinContractScenario) {
          scenarioSynchronizationExplicitSetLateJoinContractPortable(
              options,
              callback.second);
        } else if (selected == synchronizationPointContractScenario) {
          scenarioSynchronizationPointPortableContract(options, callback.second);
        } else {
          scenarioSynchronizationPointsPortable(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runMomTransportationTypeChangeRequestScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      momTransportationTypeChangeRequestScenario,
      momTransportationTypeChangeRequestContractId,
      scenarioMomTransportationTypeChangeRequest,
      scenarioMomTransportationTypeChangeRequestContract);
}

int runJoinedFederateMomRegisteredObjectCountScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomRegisteredObjectCountScenario,
      joinedFederateMomRegisteredObjectCountContractId,
      scenarioJoinedFederateMomRegisteredObjectCount,
      scenarioJoinedFederateMomRegisteredObjectCountContract);
}

int runJoinedFederateMomRoLengthPeriodicScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomRoLengthPeriodicScenario,
      joinedFederateMomRoLengthPeriodicContractId,
      scenarioJoinedFederateMomRoLengthPeriodic,
      scenarioJoinedFederateMomRoLengthPeriodicContract);
}

int runJoinedFederateMomTsoLengthPeriodicScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomTsoLengthPeriodicScenario,
      joinedFederateMomTsoLengthPeriodicContractId,
      scenarioJoinedFederateMomTsoLengthPeriodic,
      scenarioJoinedFederateMomTsoLengthPeriodicContract);
}

int runJoinedFederateMomInteractionsReceivedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomInteractionsReceivedAttributeScenario,
      joinedFederateMomInteractionsReceivedAttributeContractId,
      scenarioJoinedFederateMomInteractionsReceivedAttribute,
      scenarioJoinedFederateMomInteractionsReceivedAttributeContract);
}

int runJoinedFederateMomInteractionsSentAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomInteractionsSentAttributeScenario,
      joinedFederateMomInteractionsSentAttributeContractId,
      scenarioJoinedFederateMomInteractionsSentAttribute,
      scenarioJoinedFederateMomInteractionsSentAttributeContract);
}

int runJoinedFederateMomDirectedInteractionsReceivedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDirectedInteractionsReceivedAttributeScenario,
      joinedFederateMomDirectedInteractionsReceivedAttributeContractId,
      scenarioJoinedFederateMomDirectedInteractionsReceivedAttribute,
      scenarioJoinedFederateMomDirectedInteractionsReceivedAttributeContract);
}

int runJoinedFederateMomDirectedInteractionsSentAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDirectedInteractionsSentAttributeScenario,
      joinedFederateMomDirectedInteractionsSentAttributeContractId,
      scenarioJoinedFederateMomDirectedInteractionsSentAttribute,
      scenarioJoinedFederateMomDirectedInteractionsSentAttributeContract);
}

int runJoinedFederateMomUpdatesSentAttributeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomUpdatesSentAttributeScenario,
      joinedFederateMomUpdatesSentAttributeContractId,
      scenarioJoinedFederateMomUpdatesSentAttribute,
      scenarioJoinedFederateMomUpdatesSentAttributeContract);
}

int runJoinedFederateMomObjectInstancesUpdatedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesUpdatedAttributeScenario,
      joinedFederateMomObjectInstancesUpdatedAttributeContractId,
      scenarioJoinedFederateMomObjectInstancesUpdatedAttribute,
      scenarioJoinedFederateMomObjectInstancesUpdatedAttributeContract);
}

int runJoinedFederateMomObjectInstancesReflectedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesReflectedAttributeScenario,
      joinedFederateMomObjectInstancesReflectedAttributeContractId,
      scenarioJoinedFederateMomObjectInstancesReflectedAttribute,
      scenarioJoinedFederateMomObjectInstancesReflectedAttributeContract);
}

int runJoinedFederateMomReflectionsReceivedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomReflectionsReceivedAttributeScenario,
      joinedFederateMomReflectionsReceivedAttributeContractId,
      scenarioJoinedFederateMomReflectionsReceivedAttribute,
      scenarioJoinedFederateMomReflectionsReceivedAttributeContract);
}

int runJoinedFederateMomObjectInstancesDiscoveredAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesDiscoveredAttributeScenario,
      joinedFederateMomObjectInstancesDiscoveredAttributeContractId,
      scenarioJoinedFederateMomObjectInstancesDiscoveredAttribute,
      scenarioJoinedFederateMomObjectInstancesDiscoveredAttributeContract);
}

int runJoinedFederateMomObjectInstancesRemovedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesRemovedAttributeScenario,
      joinedFederateMomObjectInstancesRemovedAttributeContractId,
      scenarioJoinedFederateMomObjectInstancesRemovedAttribute,
      scenarioJoinedFederateMomObjectInstancesRemovedAttributeContract);
}

int runJoinedFederateMomObjectInstancesDeletedAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesDeletedAttributeScenario,
      joinedFederateMomObjectInstancesDeletedAttributeContractId,
      scenarioJoinedFederateMomObjectInstancesDeletedAttribute,
      scenarioJoinedFederateMomObjectInstancesDeletedAttributeContract);
}

int runJoinedFederateMomObjectLifecycleScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectLifecycleScenario,
      joinedFederateMomObjectLifecycleContractId,
      scenarioJoinedFederateMomObjectLifecycle,
      scenarioJoinedFederateMomObjectLifecycleContract);
}

int runJoinedFederateMomDeletableObjectReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDeletableObjectReportScenario,
      joinedFederateMomDeletableObjectReportContractId,
      scenarioJoinedFederateMomDeletableObjectReport,
      scenarioJoinedFederateMomDeletableObjectReportContract);
}

int runJoinedFederateMomObjectInstancesUpdatedReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesUpdatedReportScenario,
      joinedFederateMomObjectInstancesUpdatedReportContractId,
      scenarioJoinedFederateMomObjectInstancesUpdatedReport,
      scenarioJoinedFederateMomObjectInstancesUpdatedReportContract);
}

int runJoinedFederateMomObjectInstancesUpdatedTimestampedReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesUpdatedTimestampedReportScenario,
      joinedFederateMomObjectInstancesUpdatedTimestampedReportContractId,
      scenarioJoinedFederateMomObjectInstancesUpdatedTimestampedReport,
      scenarioJoinedFederateMomObjectInstancesUpdatedTimestampedReportContract);
}

int runJoinedFederateMomObjectInstancesUpdatedMultiClassReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesUpdatedMultiClassReportScenario,
      joinedFederateMomObjectInstancesUpdatedMultiClassReportContractId,
      scenarioJoinedFederateMomObjectInstancesUpdatedMultiClassReport,
      scenarioJoinedFederateMomObjectInstancesUpdatedMultiClassReportContract);
}

int runJoinedFederateMomObjectInstancesUpdatedNullReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesUpdatedNullReportScenario,
      joinedFederateMomObjectInstancesUpdatedNullReportContractId,
      scenarioJoinedFederateMomObjectInstancesUpdatedNullReport,
      scenarioJoinedFederateMomObjectInstancesUpdatedNullReportContract);
}

int runJoinedFederateMomObjectInstancesReflectedNullReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesReflectedNullReportScenario,
      joinedFederateMomObjectInstancesReflectedNullReportContractId,
      scenarioJoinedFederateMomObjectInstancesReflectedNullReport,
      scenarioJoinedFederateMomObjectInstancesReflectedNullReportContract);
}

int runJoinedFederateMomObjectInstancesReflectedReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesReflectedReportScenario,
      joinedFederateMomObjectInstancesReflectedReportContractId,
      scenarioJoinedFederateMomObjectInstancesReflectedReport,
      scenarioJoinedFederateMomObjectInstancesReflectedReportContract);
}

int runJoinedFederateMomObjectInstancesReflectedTimestampedReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesReflectedTimestampedReportScenario,
      joinedFederateMomObjectInstancesReflectedTimestampedReportContractId,
      scenarioJoinedFederateMomObjectInstancesReflectedTimestampedReport,
      scenarioJoinedFederateMomObjectInstancesReflectedTimestampedReportContract);
}

int runJoinedFederateMomObjectInstancesReflectedMultiClassReportScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomObjectInstancesReflectedMultiClassReportScenario,
      joinedFederateMomObjectInstancesReflectedMultiClassReportContractId,
      scenarioJoinedFederateMomObjectInstancesReflectedMultiClassReport,
      scenarioJoinedFederateMomObjectInstancesReflectedMultiClassReportContract);
}

int runJoinedFederateMomDeletableObjectCountScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      joinedFederateMomDeletableObjectCountScenario,
      joinedFederateMomDeletableObjectCountContractId,
      scenarioJoinedFederateMomDeletableObjectCount,
      scenarioJoinedFederateMomDeletableObjectCountContract);
}

int runReceiveOrderAttributeUpdateScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderAttributeUpdateScenario,
      receiveOrderAttributeUpdateContractId,
      scenarioReceiveOrderAttributeUpdate,
      scenarioReceiveOrderAttributeUpdateContract);
}

int runAttributeUpdateOrderClassificationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeUpdateOrderClassificationScenario,
      attributeUpdateOrderClassificationContractId,
      scenarioAttributeUpdateOrderClassification,
      scenarioAttributeUpdateOrderClassificationContract);
}

int runReceiveOrderInteractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderInteractionScenario,
      receiveOrderInteractionContractId,
      scenarioReceiveOrderInteraction,
      scenarioReceiveOrderInteractionContract);
}

int runInteractionOrderClassificationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionOrderClassificationScenario,
      interactionOrderClassificationContractId,
      scenarioInteractionOrderClassification,
      scenarioInteractionOrderClassificationContract);
}

int runInteractionPublicationSendTransitionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionPublicationSendTransitionScenario,
      interactionPublicationSendTransitionContractId,
      scenarioInteractionPublicationSendTransition,
      scenarioInteractionPublicationSendTransitionContract);
}

int runInteractionActiveAndPassiveSubscriptionModesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionActiveAndPassiveSubscriptionModesScenario,
      interactionActiveAndPassiveSubscriptionModesContractId,
      scenarioInteractionActiveAndPassiveSubscriptionModes,
      scenarioInteractionActiveAndPassiveSubscriptionModesContract);
}

int runReceiveOrderObjectRemovalScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      receiveOrderObjectRemovalScenario,
      receiveOrderObjectRemovalContractId,
      scenarioReceiveOrderObjectRemoval,
      scenarioReceiveOrderObjectRemovalContract);
}

int runObjectRemovalOrderClassificationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectRemovalOrderClassificationScenario,
      objectRemovalOrderClassificationContractId,
      scenarioObjectRemovalOrderClassification,
      scenarioObjectRemovalOrderClassificationContract);
}

int runFederationRestoreAbortScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationRestoreAbortScenario,
      federationRestoreAbortContractId,
      scenarioFederationRestoreAbort,
      scenarioFederationRestoreAbortContract);
}

int runFederationRestoreOwnershipAssumptionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationRestoreOwnershipAssumptionScenario,
      federationRestoreOwnershipAssumptionContractId,
      scenarioFederationRestoreOwnershipAssumption,
      scenarioFederationRestoreOwnershipAssumptionContract);
}

int runPendingNegotiatedRestorePackageScenario(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  hla_rti_cpp_tck::PendingNegotiatedRestoreConfiguration configuration;
  configuration.applicationFom = options.fom;
  configuration.standardMim = options.mimFom;
  configuration.serviceReportDirectory = options.serviceReportDirectory;
  configuration.logicalTimeImplementation = options.logicalTimeImplementationName;
  configuration.configurationName = options.configurationName;
  configuration.rtiAddress = options.rtiAddress;
  configuration.timeout = std::chrono::milliseconds(options.timeoutMilliseconds);
  if (options.callbackModel == "evoked") {
    configuration.callbackModel = rti::HLA_EVOKED;
  } else if (options.callbackModel == "immediate") {
    configuration.callbackModel = rti::HLA_IMMEDIATE;
  } else {
    throw std::runtime_error(
        "Package restore scenario must select one callback model per CTest case");
  }
  configuration.objectClassName = options.objectClassName;
  configuration.attributeName = options.attributeName;
  hla_rti_cpp_tck::runPendingNegotiatedRestore(configuration);
  std::cout << "passed " << pendingNegotiatedRestorePackageScenario << " ["
            << options.callbackModel << "]\n";
  return 0;
}

int runServiceReportCreationFailureScenario(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  hla_rti_cpp_tck::ServiceReportCreationFailureConfiguration configuration;
  configuration.applicationFom = options.fom;
  configuration.standardMim = options.mimFom;
  configuration.serviceReportDirectory = options.serviceReportDirectory;
  configuration.logicalTimeImplementation = options.logicalTimeImplementationName;
  configuration.configurationName = options.configurationName;
  if (options.callbackModel == "evoked") {
    configuration.callbackModel = rti::HLA_EVOKED;
  } else if (options.callbackModel == "immediate") {
    configuration.callbackModel = rti::HLA_IMMEDIATE;
  } else {
    throw std::runtime_error(
        "Service-report failure scenario must select one callback model per CTest case");
  }
  hla_rti_cpp_tck::runServiceReportCreationFailure(configuration);
  std::cout << "passed " << serviceReportCreationFailureScenario << " ["
            << options.callbackModel << "]\n";
  return 0;
}

int runServiceReportConnectFailureScenario(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  hla_rti_cpp_tck::ServiceReportCreationFailureConfiguration configuration;
  configuration.applicationFom = options.fom;
  configuration.standardMim = options.mimFom;
  configuration.serviceReportDirectory = options.serviceReportDirectory;
  configuration.logicalTimeImplementation = options.logicalTimeImplementationName;
  configuration.configurationName = options.configurationName;
  if (options.callbackModel == "evoked") {
    configuration.callbackModel = rti::HLA_EVOKED;
  } else if (options.callbackModel == "immediate") {
    configuration.callbackModel = rti::HLA_IMMEDIATE;
  } else {
    throw std::runtime_error(
        "Service-report Connect failure scenario must select one callback model per CTest case");
  }
  hla_rti_cpp_tck::runServiceReportConnectFailure(configuration);
  std::cout << "passed " << serviceReportConnectFailureScenario << " ["
            << options.callbackModel << "]\n";
  return 0;
}

int runOwnershipAcquisitionIfAvailableScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      ownershipAcquisitionIfAvailableScenario,
      ownershipAcquisitionIfAvailableContractId,
      scenarioOwnershipAcquisitionIfAvailable,
      scenarioOwnershipAcquisitionIfAvailableContract);
}

int runAttributeOwnershipAcquisitionCancellationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipAcquisitionCancellationScenario,
      attributeOwnershipAcquisitionCancellationContractId,
      scenarioAttributeOwnershipAcquisitionCancellation,
      scenarioAttributeOwnershipAcquisitionCancellationContract);
}

int runNegotiatedDivestitureConfirmationFlowScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      negotiatedDivestitureConfirmationFlowScenario,
      negotiatedDivestitureConfirmationFlowContractId,
      scenarioNegotiatedDivestitureConfirmationFlow,
      scenarioNegotiatedDivestitureConfirmationFlowContract);
}

int runAttributeOwnershipReleaseDeniedScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipReleaseDeniedScenario,
      attributeOwnershipReleaseDeniedContractId,
      scenarioAttributeOwnershipReleaseDenied,
      scenarioAttributeOwnershipReleaseDeniedContract);
}

int runAttributeOwnershipDivestitureIfWantedScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipDivestitureIfWantedScenario,
      attributeOwnershipDivestitureIfWantedContractId,
      scenarioAttributeOwnershipDivestitureIfWanted,
      scenarioAttributeOwnershipDivestitureIfWantedContract);
}

int runAttributeOwnershipQueryOwnerReportScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipQueryOwnerReportScenario,
      attributeOwnershipQueryOwnerReportContractId,
      scenarioAttributeOwnershipQueryOwnerReport,
      scenarioAttributeOwnershipQueryOwnerReportContract);
}

int runAttributeOwnershipQueryUnownedResultScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipQueryUnownedResultScenario,
      attributeOwnershipQueryUnownedResultContractId,
      scenarioAttributeOwnershipQueryUnownedResult,
      scenarioAttributeOwnershipQueryUnownedResultContract);
}

int runAttributeOwnershipQueryRtiOwnedResultScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeOwnershipQueryRtiOwnedResultScenario,
      attributeOwnershipQueryRtiOwnedResultContractId,
      scenarioAttributeOwnershipQueryRtiOwnedResult,
      scenarioAttributeOwnershipQueryRtiOwnedResultContract);
}

int runTimestampedDirectedInteractionRetractionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionRetractionScenario,
      timestampedDirectedInteractionRetractionContractId,
      scenarioTimestampedDirectedInteractionRetraction,
      scenarioTimestampedDirectedInteractionRetractionContract);
}

int runTimestampedDirectedInteractionRetractionFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionRetractionFanoutScenario,
      timestampedDirectedInteractionRetractionFanoutContractId,
      scenarioTimestampedDirectedInteractionRetractionFanout,
      scenarioTimestampedDirectedInteractionRetractionFanoutContract);
}

int runTimestampedInteractionsScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionsScenario,
      timestampedInteractionsContractId,
      scenarioTimestampedInteractionsPortable,
      scenarioTimestampedInteractionsContractPortable);
}

int runUnregulatedSenderTimestampedInteractionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      unregulatedSenderTimestampedInteractionScenario,
      unregulatedSenderTimestampedInteractionContractId,
      scenarioUnregulatedSenderTimestampedInteractionPortable,
      scenarioUnregulatedSenderTimestampedInteractionContractPortable);
}

int runTimestampedInteractionPreferredReceiveConversionScenarios(
    int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionPreferredReceiveConversionScenario,
      timestampedInteractionPreferredReceiveConversionContractId,
      scenarioTimestampedInteractionPreferredReceiveConversionPortable,
      scenarioTimestampedInteractionPreferredReceiveConversionContractPortable);
}

int runTimestampedInteractionSenderRegulationTransitionScenarios(
    int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionSenderRegulationTransitionScenario,
      timestampedInteractionSenderRegulationTransitionContractId,
      scenarioTimestampedInteractionSenderRegulationTransitionPortable,
      scenarioTimestampedInteractionSenderRegulationTransitionContractPortable);
}

int runInteractionFddPreferredOrderDefaultScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionFddPreferredOrderDefaultScenario,
      interactionFddPreferredOrderDefaultContractId,
      scenarioInteractionFddPreferredOrderDefaultPortable,
      scenarioInteractionFddPreferredOrderDefaultContractPortable);
}

int runAttributeFddPreferredOrderDefaultScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeFddPreferredOrderDefaultScenario,
      attributeFddPreferredOrderDefaultContractId,
      scenarioAttributeFddPreferredOrderDefaultPortable,
      scenarioAttributeFddPreferredOrderDefaultContractPortable);
}

int runTimestampedDeletePrivilegePreferredOrderScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDeletePrivilegePreferredOrderScenario,
      timestampedDeletePrivilegePreferredOrderContractId,
      scenarioTimestampedDeletePrivilegePreferredOrder,
      scenarioTimestampedDeletePrivilegePreferredOrderContract);
}

int runTimestampedDeletePrivilegeTimestampOrderScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDeletePrivilegeTimestampOrderScenario,
      timestampedDeletePrivilegeTimestampOrderContractId,
      scenarioTimestampedDeletePrivilegeTimestampOrder,
      scenarioTimestampedDeletePrivilegeTimestampOrderContract);
}

int runTimestampedAttributeUpdateScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateScenario,
      timestampedAttributeUpdateContractId,
      scenarioTimestampedAttributeUpdatePortable,
      scenarioTimestampedAttributeUpdateContractPortable);
}

int runTimestampedAttributeTarPostGrantCutoffScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeTarPostGrantCutoffScenario,
      timestampedAttributeTarPostGrantCutoffContractId,
      scenarioTimestampedAttributeUpdateTarPostGrantCutoffPortable,
      scenarioTimestampedAttributeUpdateTarPostGrantCutoffContractPortable);
}

int runTimestampedAttributeTimeAdvancingCutoffScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeTimeAdvancingCutoffScenario,
      timestampedAttributeTimeAdvancingCutoffContractId,
      scenarioTimestampedAttributeUpdateTimeAdvancingCutoffPortable,
      scenarioTimestampedAttributeUpdateTimeAdvancingCutoffContractPortable);
}

int runJavaTimestampedObjectRemovalScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      javaTimestampedObjectRemovalScenario,
      "",
      scenarioObjectRemovalOrderClassification,
      scenarioObjectRemovalOrderClassification);
}

int runJavaTimestampedAttributeUpdateScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      javaTimestampedAttributeUpdateScenario,
      "",
      scenarioTimestampedAttributeUpdatePortable,
      scenarioTimestampedAttributeUpdatePortable);
}

int runTimestampedAttributeUpdateReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateReenableScenario,
      timestampedAttributeUpdateReenableContractId,
      scenarioTimestampedAttributeUpdateReenablePortable,
      scenarioTimestampedAttributeUpdateReenableContractPortable);
}

int runTimestampedAttributeUpdateRegulationReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateRegulationReenableScenario,
      timestampedAttributeUpdateRegulationReenableContractId,
      scenarioTimestampedAttributeUpdateRegulationReenablePortable,
      scenarioTimestampedAttributeUpdateRegulationReenableContractPortable);
}

int runTimestampedAttributeSourceResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeSourceResignationScenario,
      timestampedAttributeSourceResignationContractId,
      scenarioTimestampedAttributeSourceResignationPortable,
      scenarioTimestampedAttributeSourceResignationContractPortable);
}

int runTimestampedAttributeSourceResignationFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeSourceResignationFanoutScenario,
      timestampedAttributeSourceResignationFanoutContractId,
      scenarioTimestampedAttributeSourceResignationFanoutPortable,
      scenarioTimestampedAttributeSourceResignationFanoutContractPortable);
}

int runTimestampedAttributeUpdateOwnershipTransferScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateOwnershipTransferScenario,
      timestampedAttributeUpdateOwnershipTransferContractId,
      scenarioTimestampedAttributeUpdateOwnershipTransferPortable,
      scenarioTimestampedAttributeUpdateOwnershipTransferContractPortable);
}

int runTimestampedAttributeUpdateQueuedPasselRetractionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateQueuedPasselRetractionScenario,
      timestampedAttributeUpdateQueuedPasselRetractionContractId,
      scenarioTimestampedAttributeUpdateQueuedPasselRetractionPortable,
      scenarioTimestampedAttributeUpdateQueuedPasselRetractionContractPortable);
}

int runMixedOrderAttributePasselRetractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      mixedOrderAttributePasselRetractionScenario,
      mixedOrderAttributePasselRetractionContractId,
      scenarioMixedOrderAttributePasselRetractionPortable,
      scenarioMixedOrderAttributePasselRetractionContractPortable);
}

int runTimestampedAttributeImmediateRecipientMultiTransportRetractionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeImmediateRecipientMultiTransportRetractionScenario,
      timestampedAttributeImmediateRecipientMultiTransportRetractionContractId,
      scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionPortable,
      scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionContractPortable);
}

int runTimestampedAttributeSenderRegulationTransitionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeSenderRegulationTransitionScenario,
      timestampedAttributeSenderRegulationTransitionContractId,
      scenarioTimestampedAttributeSenderRegulationTransitionPortable,
      scenarioTimestampedAttributeSenderRegulationTransitionContractPortable);
}

int runTimestampedAttributeUpdateNoFanoutScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateNoFanoutScenario,
      timestampedAttributeUpdateNoFanoutContractId,
      scenarioTimestampedAttributeUpdateNoFanoutPortable,
      scenarioTimestampedAttributeUpdateNoFanoutContractPortable);
}

int runTimestampedAttributeUpdateFlushQueueFutureInputScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateFlushQueueFutureInputScenario,
      timestampedAttributeUpdateFlushQueueFutureInputContractId,
      scenarioTimestampedAttributeUpdateFlushQueueFutureInputPortable,
      scenarioTimestampedAttributeUpdateFlushQueueFutureInputContractPortable);
}

int runTimestampedInteractionFlushQueueFutureInputScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionFlushQueueFutureInputScenario,
      timestampedInteractionFlushQueueFutureInputContractId,
      scenarioTimestampedInteractionFlushQueueFutureInputPortable,
      scenarioTimestampedInteractionFlushQueueFutureInputContractPortable);
}

int runFqrPostgrantLowerTsoRetractionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      fqrPostgrantLowerTsoRetractionScenario,
      fqrPostgrantLowerTsoRetractionContractId,
      scenarioFqrPostgrantLowerTsoRetractionPortable,
      scenarioFqrPostgrantLowerTsoRetractionContractPortable);
}

int runFqgPostgrantDeliveryCutoffScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      fqgPostgrantDeliveryCutoffScenario,
      fqgPostgrantDeliveryCutoffContractId,
      scenarioFqgPostgrantDeliveryCutoffPortable,
      scenarioFqgPostgrantDeliveryCutoffContractPortable);
}

int runTarQueuedTsoBeforeGrantScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      tarQueuedTsoBeforeGrantScenario,
      tarQueuedTsoBeforeGrantContractId,
      scenarioTarQueuedTsoBeforeGrantPortable,
      scenarioTarQueuedTsoBeforeGrantContractPortable);
}

int runTimestampedAttributeOrderCohortScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeOrderCohortScenario,
      timestampedAttributeOrderCohortContractId,
      scenarioTimestampedAttributeOrderCohortPortable,
      scenarioTimestampedAttributeOrderCohortContractPortable);
}

int runTimestampedObjectDeletionNoFanoutScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionNoFanoutScenario,
      timestampedObjectDeletionNoFanoutContractId,
      scenarioTimestampedObjectDeletionNoFanoutPortable,
      scenarioTimestampedObjectDeletionNoFanoutContractPortable);
}

int runTimestampedObjectDeletionTombstoneScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionTombstoneScenario,
      timestampedObjectDeletionTombstoneContractId,
      scenarioTimestampedObjectDeletionTombstonePortable,
      scenarioTimestampedObjectDeletionTombstoneContractPortable);
}

int runTimestampedObjectDeletionRetractionJoinedOwnersScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionRetractionJoinedOwnersScenario,
      timestampedObjectDeletionRetractionJoinedOwnersContractId,
      scenarioTimestampedObjectDeletionJoinedOwnerRetractionPortable,
      scenarioTimestampedObjectDeletionJoinedOwnerRetractionContractPortable);
}

int runTimestampedObjectDeletionSourceResignationFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionSourceResignationFanoutScenario,
      timestampedObjectDeletionSourceResignationFanoutContractId,
      scenarioTimestampedObjectDeletionSourceResignationFanoutPortable,
      scenarioTimestampedObjectDeletionSourceResignationFanoutContractPortable);
}

int runTimestampedObjectDeletionScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionScenario,
      timestampedObjectDeletionContractId,
      scenarioTimestampedObjectDeletionPortable,
      scenarioTimestampedObjectDeletionContractPortable);
}

int runTimestampedObjectDeletionRegulationReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionRegulationReenableScenario,
      timestampedObjectDeletionRegulationReenableContractId,
      scenarioTimestampedObjectDeletionRegulationReenablePortable,
      scenarioTimestampedObjectDeletionRegulationReenableContractPortable);
}

int runAlternateTimeAdvancesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      alternateTimeAdvancesScenario,
      alternateTimeAdvancesContractId,
      scenarioAlternateTimeAdvancesPortable,
      scenarioAlternateTimeAdvancesContractPortable);
}

int runPendingAlternateAdvanceStateScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      pendingAlternateAdvanceStateScenario,
      pendingAlternateAdvanceStateContractId,
      scenarioPendingAlternateAdvanceState,
      scenarioPendingAlternateAdvanceStateContract);
}

int runNextMessageRequestScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nextMessageRequestScenario,
      nextMessageRequestContractId,
      scenarioNextMessageRequestPortable,
      scenarioNextMessageRequestContractPortable);
}

int runNmraNoPregrantTsoFloorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmraNoPregrantTsoFloorScenario,
      nmraNoPregrantTsoFloorContractId,
      scenarioNmraNoPregrantTsoFloorPortable,
      scenarioNmraNoPregrantTsoFloorContractPortable);
}

int runNmrNoPregrantTsoFloorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrNoPregrantTsoFloorScenario,
      nmrNoPregrantTsoFloorContractId,
      scenarioNmrNoPregrantTsoFloorPortable,
      scenarioNmrNoPregrantTsoFloorContractPortable);
}

int runNmrReceivedTsoFloorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrReceivedTsoFloorScenario,
      nmrReceivedTsoFloorContractId,
      scenarioNmrReceivedTsoFloorPortable,
      scenarioNmrReceivedTsoFloorContractPortable);
}

int runNmrReceivedTsoAttributeUpdateFloorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrReceivedTsoAttributeUpdateFloorScenario,
      nmrReceivedTsoAttributeUpdateFloorContractId,
      scenarioNmrReceivedTsoAttributeUpdateFloorPortable,
      scenarioNmrReceivedTsoAttributeUpdateFloorContractPortable);
}

int runNmrReceivedTsoDirectedInteractionFloorScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrReceivedTsoDirectedInteractionFloorScenario,
      nmrReceivedTsoDirectedInteractionFloorContractId,
      scenarioNmrReceivedTsoDirectedInteractionFloorPortable,
      scenarioNmrReceivedTsoDirectedInteractionFloorContractPortable);
}

int runNmrEmptyMessageSetGrantTimeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrEmptyMessageSetGrantTimeScenario,
      nmrEmptyMessageSetGrantTimeContractId,
      scenarioNmrEmptyMessageSetGrantTimePortable,
      scenarioNmrEmptyMessageSetGrantTimeContractPortable);
}

int runNmrRequestBelowGaltImmediateGrantScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrRequestBelowGaltImmediateGrantScenario,
      nmrRequestBelowGaltImmediateGrantContractId,
      scenarioNmrRequestBelowGaltImmediateGrantPortable,
      scenarioNmrRequestBelowGaltImmediateGrantContractPortable);
}

int runNmrLitsBelowGaltImmediateGrantScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      nmrLitsBelowGaltImmediateGrantScenario,
      nmrLitsBelowGaltImmediateGrantContractId,
      scenarioNmrLitsBelowGaltImmediateGrantPortable,
      scenarioNmrLitsBelowGaltImmediateGrantContractPortable);
}

int runSendMessageTimeStatePreservationGrantedScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      sendMessageTimeStatePreservationGrantedScenario,
      sendMessageTimeStatePreservationGrantedContractId,
      scenarioSendMessageTimeStatePreservationGrantedPortable,
      scenarioSendMessageTimeStatePreservationGrantedContractPortable);
}

int runAvailableTimeAdvancesInclusiveGaltScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      availableTimeAdvancesInclusiveGaltScenario,
      availableTimeAdvancesInclusiveGaltContractId,
      scenarioAvailableTimeAdvancesInclusiveGaltPortable,
      scenarioAvailableTimeAdvancesInclusiveGaltContractPortable);
}

int runTimeConstrainedEnableAtGaltEqualityScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timeConstrainedEnableAtGaltEqualityScenario,
      timeConstrainedEnableAtGaltEqualityContractId,
      scenarioTimeConstrainedEnableAtGaltEqualityPortable,
      scenarioTimeConstrainedEnableAtGaltEqualityContractPortable);
}

int runTimeRegulationImplicitAdvanceScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timeRegulationImplicitAdvanceScenario,
      timeRegulationImplicitAdvanceContractId,
      scenarioTimeRegulationImplicitAdvancePortable,
      scenarioTimeRegulationImplicitAdvanceContractPortable);
}

int runTimestampedInteractionMixedAdvancesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionMixedAdvancesScenario,
      timestampedInteractionMixedAdvancesContractId,
      scenarioTimestampedInteractionMixedAdvancesPortable,
      scenarioTimestampedInteractionMixedAdvancesContractPortable);
}

int runTimeCoordinatedInteractionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timeCoordinatedInteractionScenario,
      timeCoordinatedInteractionScenario,
      scenarioTimeCoordinatedInteractionPortable,
      scenarioTimeCoordinatedInteractionPortable);
}

int runEmptyAttributeSubscriptionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      emptyAttributeSubscriptionScenario,
      emptyAttributeSubscriptionScenario,
      scenarioEmptyAttributeSubscriptionIsNoOpPortable,
      scenarioEmptyAttributeSubscriptionIsNoOpPortable);
}

int runNamedUpdateRateSubscriptionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      namedUpdateRateSubscriptionScenario,
      namedUpdateRateSubscriptionScenario,
      scenarioNamedUpdateRateSubscriptionPortable,
      scenarioNamedUpdateRateSubscriptionPortable);
}

int runDefaultRateNoReductionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      defaultRateNoReductionScenario,
      defaultRateNoReductionScenario,
      scenarioDefaultRateSubscriptionDoesNotReducePortable,
      scenarioDefaultRateSubscriptionDoesNotReducePortable);
}

int runAccumulatingAttributeSubscriptionsScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      accumulatingAttributeSubscriptionsScenario,
      accumulatingAttributeSubscriptionsScenario,
      scenarioAttributeSubscriptionInvocationsAccumulatePortable,
      scenarioAttributeSubscriptionInvocationsAccumulatePortable);
}

int runAttributeSubscriptionModeTransitionsScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeSubscriptionModeTransitionsScenario,
      attributeSubscriptionModeTransitionsContractId,
      scenarioAttributeSubscriptionPerAttributeModeTransitionsPortable,
      scenarioAttributeSubscriptionPerAttributeModeTransitionsContractPortable);
}

int runPassiveSubscriptionNoStartRegistrationScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      passiveSubscriptionNoStartRegistrationScenario,
      passiveSubscriptionNoStartRegistrationScenario,
      scenarioPassiveSubscriptionDoesNotStartRegistrationPortable,
      scenarioPassiveSubscriptionDoesNotStartRegistrationPortable);
}

int runSelectiveAttributeUnsubscriptionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      selectiveAttributeUnsubscriptionScenario,
      selectiveAttributeUnsubscriptionScenario,
      scenarioSelectiveAttributeUnsubscriptionPortable,
      scenarioSelectiveAttributeUnsubscriptionPortable);
}

int runWholeClassAttributeUnsubscriptionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      wholeClassAttributeUnsubscriptionScenario,
      wholeClassAttributeUnsubscriptionScenario,
      scenarioWholeClassAttributeUnsubscriptionPortable,
      scenarioWholeClassAttributeUnsubscriptionPortable);
}

int runEmptyAttributeUnsubscriptionScenario(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      emptyAttributeUnsubscriptionScenario,
      emptyAttributeUnsubscriptionScenario,
      scenarioEmptyAttributeUnsubscriptionIsNoOpPortable,
      scenarioEmptyAttributeUnsubscriptionIsNoOpPortable);
}

int runTimestampedInteractionTaraEqualBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionTaraEqualBoundaryScenario,
      timestampedInteractionTaraEqualBoundaryContractId,
      scenarioTimestampedInteractionTaraEqualBoundaryPortable,
      scenarioTimestampedInteractionTaraEqualBoundaryContractPortable);
}

int runTimestampedInteractionTarEqualBoundaryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionTarEqualBoundaryScenario,
      timestampedInteractionTarEqualBoundaryContractId,
      scenarioTimestampedInteractionTarEqualBoundaryPortable,
      scenarioTimestampedInteractionTarEqualBoundaryContractPortable);
}

int runTimestampedInteractionTarPostGrantCutoffScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionTarPostGrantCutoffScenario,
      timestampedInteractionTarPostGrantCutoffContractId,
      scenarioTimestampedInteractionTarPostGrantCutoffPortable,
      scenarioTimestampedInteractionTarPostGrantCutoffContractPortable);
}

int runTimestampedInteractionPublicationSendTransitionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionPublicationSendTransitionScenario,
      timestampedInteractionPublicationSendTransitionContractId,
      scenarioTimestampedInteractionPublicationSendTransitionPortable,
      scenarioTimestampedInteractionPublicationSendTransitionContractPortable);
}

int runTimestampedInteractionSourceResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionSourceResignationScenario,
      timestampedInteractionSourceResignationContractId,
      scenarioTimestampedInteractionSourceResignationPortable,
      scenarioTimestampedInteractionSourceResignationContractPortable);
}

int runTimestampedInteractionSourceResignationFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionSourceResignationFanoutScenario,
      timestampedInteractionSourceResignationFanoutContractId,
      scenarioTimestampedInteractionSourceResignationFanoutPortable,
      scenarioTimestampedInteractionSourceResignationFanoutContractPortable);
}

int runTimestampedInteractionTsoDesignatorTerminalizationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionTsoDesignatorTerminalizationScenario,
      timestampedInteractionTsoDesignatorTerminalizationContractId,
      scenarioTimestampedInteractionTsoDesignatorTerminalizationPortable,
      scenarioTimestampedInteractionTsoDesignatorTerminalizationContractPortable);
}

int runTimestampedInteractionCrossProducerOrderScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionCrossProducerOrderScenario,
      timestampedInteractionCrossProducerOrderContractId,
      scenarioTimestampedInteractionCrossProducerOrderPortable,
      scenarioTimestampedInteractionCrossProducerOrderContractPortable);
}

int runTimestampedInteractionNoFanoutScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionNoFanoutScenario,
      timestampedInteractionNoFanoutContractId,
      scenarioTimestampedInteractionNoFanoutPortable,
      scenarioTimestampedInteractionNoFanoutContractPortable);
}

int runTimestampedInteractionRetractionFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionRetractionFanoutScenario,
      timestampedInteractionRetractionFanoutContractId,
      scenarioTimestampedInteractionRetractionFanoutPortable,
      scenarioTimestampedInteractionRetractionFanoutContractPortable);
}

int runTimestampedInteractionReenableScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionReenableScenario,
      timestampedInteractionReenableContractId,
      scenarioTimestampedInteractionReenablePortable,
      scenarioTimestampedInteractionReenableContractPortable);
}

int runTimestampedDirectedInteractionReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionReenableScenario,
      timestampedDirectedInteractionReenableContractId,
      scenarioTimestampedDirectedInteractionReenablePortable,
      scenarioTimestampedDirectedInteractionReenableContractPortable);
}

int runTimestampedInteractionRegulationReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedInteractionRegulationReenableScenario,
      timestampedInteractionRegulationReenableContractId,
      scenarioTimestampedInteractionRegulationReenablePortable,
      scenarioTimestampedInteractionRegulationReenableContractPortable);
}

int runTimestampedDirectedInteractionRegulationReenableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionRegulationReenableScenario,
      timestampedDirectedInteractionRegulationReenableContractId,
      scenarioTimestampedDirectedInteractionRegulationReenablePortable,
      scenarioTimestampedDirectedInteractionRegulationReenableContractPortable);
}

int runTimestampedDirectedAlternateAdvancesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedAlternateAdvancesScenario,
      timestampedDirectedAlternateAdvancesContractId,
      scenarioTimestampedDirectedAlternateAdvancesPortable,
      scenarioTimestampedDirectedAlternateAdvancesContractPortable);
}

int runTimestampedAttributeUpdateAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedAttributeUpdateAlternateAdvancesScenario,
      timestampedAttributeUpdateAlternateAdvancesContractId,
      scenarioTimestampedAttributeUpdateAlternateAdvancesPortable,
      scenarioTimestampedAttributeUpdateAlternateAdvancesContractPortable);
}

int runTimestampedObjectDeletionMixedAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionMixedAdvancesScenario,
      timestampedObjectDeletionMixedAdvancesContractId,
      scenarioTimestampedObjectDeletionMixedAdvancesPortable,
      scenarioTimestampedObjectDeletionMixedAdvancesContractPortable);
}

int runTimestampedObjectDeletionFutureNmrArrivalScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionFutureNmrArrivalScenario,
      timestampedObjectDeletionFutureNmrArrivalContractId,
      scenarioTimestampedObjectDeletionFutureNmrArrivalPortable,
      scenarioTimestampedObjectDeletionFutureNmrArrivalContractPortable);
}

int runTimestampedObjectDeletionFutureNmraArrivalScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedObjectDeletionFutureNmraArrivalScenario,
      timestampedObjectDeletionFutureNmraArrivalContractId,
      scenarioTimestampedObjectDeletionFutureNmraArrivalPortable,
      scenarioTimestampedObjectDeletionFutureNmraArrivalContractPortable);
}

int runTimestampedDirectedInteractionTarNmrScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionTarNmrScenario,
      timestampedDirectedInteractionTarNmrContractId,
      scenarioTimestampedDirectedInteractionTarNmrPortable,
      scenarioTimestampedDirectedInteractionTarNmrContractPortable);
}

int runTimestampedDirectedInteractionSourceResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionSourceResignationScenario,
      timestampedDirectedInteractionSourceResignationContractId,
      scenarioTimestampedDirectedInteractionSourceResignationPortable,
      scenarioTimestampedDirectedInteractionSourceResignationContractPortable);
}

int runTimestampedDirectedInteractionSourceResignationFanoutScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionSourceResignationFanoutScenario,
      timestampedDirectedInteractionSourceResignationFanoutContractId,
      scenarioTimestampedDirectedInteractionSourceResignationFanoutPortable,
      scenarioTimestampedDirectedInteractionSourceResignationFanoutContractPortable);
}

int runTimestampedDirectedInteractionImmediateSourceResignationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      timestampedDirectedInteractionImmediateSourceResignationScenario,
      timestampedDirectedInteractionImmediateSourceResignationContractId,
      scenarioTimestampedDirectedInteractionImmediateSourceResignationPortable,
      scenarioTimestampedDirectedInteractionImmediateSourceResignationContractPortable);
}

int runUnconditionalAttributeOwnershipDivestitureScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      unconditionalAttributeOwnershipDivestitureScenario,
      unconditionalAttributeOwnershipDivestitureContractId,
      scenarioUnconditionalAttributeOwnershipDivestiture,
      scenarioUnconditionalAttributeOwnershipDivestitureContract);
}

int runAutoProvideDisabledDiscoveryOnlyScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      autoProvideDisabledDiscoveryOnlyScenario,
      autoProvideDisabledDiscoveryOnlyContractId,
      scenarioAutoProvideDisabledDiscoveryOnly,
      scenarioAutoProvideDisabledDiscoveryOnlyContract);
}

int runAutoProvideDisabledExplicitRequestScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      autoProvideDisabledExplicitRequestScenario,
      autoProvideDisabledExplicitRequestContractId,
      scenarioAutoProvideDisabledExplicitRequest,
      scenarioAutoProvideDisabledExplicitRequestContract);
}

int runFederationAutoProvideMomSwitchScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationAutoProvideMomSwitchScenario,
      federationAutoProvideMomSwitchContractId,
      scenarioFederationAutoProvideMomSwitch,
      scenarioFederationAutoProvideMomSwitchContract);
}

int runFederateServiceReportingMomSwitchScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateServiceReportingMomSwitchScenario,
      federateServiceReportingMomSwitchContractId,
      scenarioFederateServiceReportingMomSwitch,
      scenarioFederateServiceReportingMomSwitchContract);
}

int runFederateServiceReportingMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateServiceReportingMomAttributeScenario,
      federateServiceReportingMomAttributeContractId,
      scenarioFederateServiceReportingMomAttribute,
      scenarioFederateServiceReportingMomAttributeContract);
}

int runFederateObjectClassRelevanceAdvisoryMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateObjectClassRelevanceAdvisoryMomAttributeScenario,
      federateObjectClassRelevanceAdvisoryMomAttributeContractId,
      scenarioFederateObjectClassRelevanceAdvisoryMomAttribute,
      scenarioFederateObjectClassRelevanceAdvisoryMomAttributeContract);
}

int runFederateAttributeRelevanceAdvisoryMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateAttributeRelevanceAdvisoryMomAttributeScenario,
      federateAttributeRelevanceAdvisoryMomAttributeContractId,
      scenarioFederateAttributeRelevanceAdvisoryMomAttribute,
      scenarioFederateAttributeRelevanceAdvisoryMomAttributeContract);
}

int runFederateAttributeScopeAdvisoryMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateAttributeScopeAdvisoryMomAttributeScenario,
      federateAttributeScopeAdvisoryMomAttributeContractId,
      scenarioFederateAttributeScopeAdvisoryMomAttribute,
      scenarioFederateAttributeScopeAdvisoryMomAttributeContract);
}

int runFederateInteractionRelevanceAdvisoryMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateInteractionRelevanceAdvisoryMomAttributeScenario,
      federateInteractionRelevanceAdvisoryMomAttributeContractId,
      scenarioFederateInteractionRelevanceAdvisoryMomAttribute,
      scenarioFederateInteractionRelevanceAdvisoryMomAttributeContract);
}

int runFederateAsynchronousDeliveryMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateAsynchronousDeliveryMomAttributeScenario,
      federateAsynchronousDeliveryMomAttributeContractId,
      scenarioFederateAsynchronousDeliveryMomAttribute,
      scenarioFederateAsynchronousDeliveryMomAttributeContract);
}

int runFederateTimeConstrainedMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateTimeConstrainedMomAttributeScenario,
      federateTimeConstrainedMomAttributeContractId,
      scenarioFederateTimeConstrainedMomAttribute,
      scenarioFederateTimeConstrainedMomAttributeContract);
}

int runFederateTimeRegulatingMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateTimeRegulatingMomAttributeScenario,
      federateTimeRegulatingMomAttributeContractId,
      scenarioFederateTimeRegulatingMomAttribute,
      scenarioFederateTimeRegulatingMomAttributeContract);
}

int runFederateLookaheadMomPeriodicScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateLookaheadMomPeriodicScenario,
      federateLookaheadMomPeriodicContractId,
      scenarioFederateLookaheadMomPeriodic,
      scenarioFederateLookaheadMomPeriodicContract);
}

int runFederateLogicalTimeMomPeriodicScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateLogicalTimeMomPeriodicScenario,
      federateLogicalTimeMomPeriodicContractId,
      scenarioFederateLogicalTimeMomPeriodic,
      scenarioFederateLogicalTimeMomPeriodicContract);
}

int runFederateTimeManagerStateMomAttributeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateTimeManagerStateMomAttributeScenario,
      federateTimeManagerStateMomAttributeContractId,
      scenarioFederateTimeManagerStateMomAttribute,
      scenarioFederateTimeManagerStateMomAttributeContract);
}

int runFederateExceptionReportingMomSwitchScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateExceptionReportingMomSwitchScenario,
      federateExceptionReportingMomSwitchContractId,
      scenarioFederateExceptionReportingMomSwitch,
      scenarioFederateExceptionReportingMomSwitchContract);
}

int runFederateExceptionReportingMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateExceptionReportingMomAttributeScenario,
      federateExceptionReportingMomAttributeContractId,
      scenarioFederateExceptionReportingMomAttribute,
      scenarioFederateExceptionReportingMomAttributeContract);
}

int runFederateExceptionReportDeliveryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateExceptionReportDeliveryScenario,
      federateExceptionReportDeliveryContractId,
      scenarioFederateExceptionReportDelivery,
      scenarioFederateExceptionReportDeliveryContract);
}

int runFederateMomExceptionReportDeliveryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomExceptionReportDeliveryScenario,
      federateMomExceptionReportDeliveryContractId,
      scenarioFederateMomExceptionReportDelivery,
      scenarioFederateMomExceptionReportDeliveryContract);
}

int runFederateMomExceptionMissingParameterScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomExceptionMissingParameterScenario,
      federateMomExceptionMissingParameterContractId,
      scenarioFederateMomExceptionMissingParameter,
      scenarioFederateMomExceptionMissingParameterContract);
}

int runFederationMomExceptionMissingFomModuleIndicatorScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federationMomExceptionMissingFomModuleIndicatorScenario,
      federationMomExceptionMissingFomModuleIndicatorContractId,
      scenarioFederationMomExceptionMissingFomModuleIndicator,
      scenarioFederationMomExceptionMissingFomModuleIndicatorContract);
}

int runFederateMomFomModuleContentReportScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomFomModuleContentReportScenario,
      federateMomFomModuleContentReportContractId,
      scenarioFederateMomFomModuleContentReport,
      scenarioFederateMomFomModuleContentReportContract);
}

int runFederateMomObjectInstanceInformationScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectInstanceInformationScenario,
      federateMomObjectInstanceInformationContractId,
      scenarioFederateMomObjectInstanceInformation,
      scenarioFederateMomObjectInstanceInformationContract);
}

int runFederateMomPublicationQueryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomPublicationQueryScenario,
      federateMomPublicationQueryContractId,
      scenarioFederateMomPublicationQuery,
      scenarioFederateMomPublicationQueryContract);
}

int runFederateMomCompleteObjectClassPublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomCompleteObjectClassPublicationQueryScenario,
      federateMomCompleteObjectClassPublicationQueryContractId,
      scenarioFederateMomCompleteObjectClassPublicationQuery,
      scenarioFederateMomCompleteObjectClassPublicationQueryContract);
}

int runFederateMomObjectClassAttributeUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassAttributeUnpublicationQueryScenario,
      federateMomObjectClassAttributeUnpublicationQueryContractId,
      scenarioFederateMomObjectClassAttributeUnpublicationQuery,
      scenarioFederateMomObjectClassAttributeUnpublicationQueryContract);
}

int runFederateMomObjectClassFullAttributeUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassFullAttributeUnpublicationQueryScenario,
      federateMomObjectClassFullAttributeUnpublicationQueryContractId,
      scenarioFederateMomObjectClassFullAttributeUnpublicationQuery,
      scenarioFederateMomObjectClassFullAttributeUnpublicationQueryContract);
}

int runFederateMomObjectClassWholeUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassWholeUnpublicationQueryScenario,
      federateMomObjectClassWholeUnpublicationQueryContractId,
      scenarioFederateMomObjectClassWholeUnpublicationQuery,
      scenarioFederateMomObjectClassWholeUnpublicationQueryContract);
}

int runFederateMomInteractionClassWholeUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomInteractionClassWholeUnpublicationQueryScenario,
      federateMomInteractionClassWholeUnpublicationQueryContractId,
      scenarioFederateMomInteractionClassWholeUnpublicationQuery,
      scenarioFederateMomInteractionClassWholeUnpublicationQueryContract);
}

int runFederateMomInteractionClassPartialUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomInteractionClassPartialUnpublicationQueryScenario,
      federateMomInteractionClassPartialUnpublicationQueryContractId,
      scenarioFederateMomInteractionClassPartialUnpublicationQuery,
      scenarioFederateMomInteractionClassPartialUnpublicationQueryContract);
}

int runFederateMomMultipleInteractionClassFullUnpublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleInteractionClassFullUnpublicationQueryScenario,
      federateMomMultipleInteractionClassFullUnpublicationQueryContractId,
      scenarioFederateMomMultipleInteractionClassFullUnpublicationQuery,
      scenarioFederateMomMultipleInteractionClassFullUnpublicationQueryContract);
}

int runFederateMomMultipleDirectedPublicationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleDirectedPublicationQueryScenario,
      federateMomMultipleDirectedPublicationQueryContractId,
      scenarioFederateMomMultipleDirectedPublicationQuery,
      scenarioFederateMomMultipleDirectedPublicationQueryContract);
}

int runFederateMomMultipleDirectedPublicationPartialUnpublishQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleDirectedPublicationPartialUnpublishQueryScenario,
      federateMomMultipleDirectedPublicationPartialUnpublishQueryContractId,
      scenarioFederateMomMultipleDirectedPublicationPartialUnpublishQuery,
      scenarioFederateMomMultipleDirectedPublicationPartialUnpublishQueryContract);
}

int runFederateMomMultipleDirectedPublicationFullUnpublishQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleDirectedPublicationFullUnpublishQueryScenario,
      federateMomMultipleDirectedPublicationFullUnpublishQueryContractId,
      scenarioFederateMomMultipleDirectedPublicationFullUnpublishQuery,
      scenarioFederateMomMultipleDirectedPublicationFullUnpublishQueryContract);
}

int runFederateMomSubscriptionQueryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomSubscriptionQueryScenario,
      federateMomSubscriptionQueryContractId,
      scenarioFederateMomSubscriptionQuery,
      scenarioFederateMomSubscriptionQueryContract);
}

int runFederateMomPassiveSubscriptionQueryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomPassiveSubscriptionQueryScenario,
      federateMomPassiveSubscriptionQueryContractId,
      scenarioFederateMomPassiveSubscriptionQuery,
      scenarioFederateMomPassiveSubscriptionQueryContract);
}

int runFederateMomPassiveInteractionSubscriptionQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomPassiveInteractionSubscriptionQueryScenario,
      federateMomPassiveInteractionSubscriptionQueryContractId,
      scenarioFederateMomPassiveInteractionSubscriptionQuery,
      scenarioFederateMomPassiveInteractionSubscriptionQueryContract);
}

int runFederateMomPassiveInteractionSubscriptionActivationQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomPassiveInteractionSubscriptionActivationQueryScenario,
      federateMomPassiveInteractionSubscriptionActivationQueryContractId,
      scenarioFederateMomPassiveInteractionSubscriptionActivationQuery,
      scenarioFederateMomPassiveInteractionSubscriptionActivationQueryContract);
}

int runFederateMomMultipleInteractionSubscriptionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleInteractionSubscriptionQueryScenario,
      federateMomMultipleInteractionSubscriptionQueryContractId,
      scenarioFederateMomMultipleInteractionSubscriptionQuery,
      scenarioFederateMomMultipleInteractionSubscriptionQueryContract);
}

int runFederateMomMultipleInteractionSubscriptionUnsubscribeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleInteractionSubscriptionUnsubscribeQueryScenario,
      federateMomMultipleInteractionSubscriptionUnsubscribeQueryContractId,
      scenarioFederateMomMultipleInteractionSubscriptionUnsubscribeQuery,
      scenarioFederateMomMultipleInteractionSubscriptionUnsubscribeQueryContract);
}

int runFederateMomMultipleInteractionSubscriptionFullUnsubscribeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryScenario,
      federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryContractId,
      scenarioFederateMomMultipleInteractionSubscriptionFullUnsubscribeQuery,
      scenarioFederateMomMultipleInteractionSubscriptionFullUnsubscribeQueryContract);
}

int runFederateMomObjectAttributePartialUnsubscribeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectAttributePartialUnsubscribeQueryScenario,
      federateMomObjectAttributePartialUnsubscribeQueryContractId,
      scenarioFederateMomObjectAttributePartialUnsubscribeQuery,
      scenarioFederateMomObjectAttributePartialUnsubscribeQueryContract);
}

int runFederateMomObjectAttributeFullUnsubscribeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectAttributeFullUnsubscribeQueryScenario,
      federateMomObjectAttributeFullUnsubscribeQueryContractId,
      scenarioFederateMomObjectAttributeFullUnsubscribeQuery,
      scenarioFederateMomObjectAttributeFullUnsubscribeQueryContract);
}

int runFederateMomObjectClassFullUnsubscribeScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassFullUnsubscribeQueryScenario,
      federateMomObjectClassFullUnsubscribeQueryContractId,
      scenarioFederateMomObjectClassFullUnsubscribeQuery,
      scenarioFederateMomObjectClassFullUnsubscribeQueryContract);
}

int runFederateMomObjectClassCompleteAttributeSubscriptionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassCompleteAttributeSubscriptionQueryScenario,
      federateMomObjectClassCompleteAttributeSubscriptionQueryContractId,
      scenarioFederateMomObjectClassCompleteAttributeSubscriptionQuery,
      scenarioFederateMomObjectClassCompleteAttributeSubscriptionQueryContract);
}

int runFederateMomObjectClassPassiveCompleteAttributeSubscriptionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryScenario,
      federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryContractId,
      scenarioFederateMomObjectClassPassiveCompleteAttributeSubscriptionQuery,
      scenarioFederateMomObjectClassPassiveCompleteAttributeSubscriptionQueryContract);
}

int runFederateMomObjectClassCompleteAttributeSubscriptionActivationScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomObjectClassCompleteAttributeSubscriptionActivationQueryScenario,
      federateMomObjectClassCompleteAttributeSubscriptionActivationQueryContractId,
      scenarioFederateMomObjectClassCompleteAttributeSubscriptionActivationQuery,
      scenarioFederateMomObjectClassCompleteAttributeSubscriptionActivationQueryContract);
}

int runFederateMomEmptySubscriptionQueryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomEmptySubscriptionQueryScenario,
      federateMomEmptySubscriptionQueryContractId,
      scenarioFederateMomEmptySubscriptionQuery,
      scenarioFederateMomEmptySubscriptionQueryContract);
}

int runFederateMomDirectedSubscriptionQueryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomDirectedSubscriptionQueryScenario,
      federateMomDirectedSubscriptionQueryContractId,
      scenarioFederateMomDirectedSubscriptionQuery,
      scenarioFederateMomDirectedSubscriptionQueryContract);
}

int runFederateMomMultipleDirectedSubscriptionQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleDirectedSubscriptionQueryScenario,
      federateMomMultipleDirectedSubscriptionQueryContractId,
      scenarioFederateMomMultipleDirectedSubscriptionQuery,
      scenarioFederateMomMultipleDirectedSubscriptionQueryContract);
}

int runFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenario,
      federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryContractId,
      scenarioFederateMomMultipleDirectedSubscriptionFullUnsubscribeQuery,
      scenarioFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryContract);
}

int runFederateMomExceptionReportServicePreconditionScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateMomExceptionReportServicePreconditionScenario,
      federateMomExceptionReportServicePreconditionContractId,
      scenarioFederateMomExceptionReportServicePrecondition,
      scenarioFederateMomExceptionReportServicePreconditionContract);
}

int runFederateSendServiceReportsToFileMomSwitchScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateSendServiceReportsToFileMomSwitchScenario,
      federateSendServiceReportsToFileMomSwitchContractId,
      scenarioFederateSendServiceReportsToFileMomSwitch,
      scenarioFederateSendServiceReportsToFileMomSwitchContract);
}

int runFederateAutomaticResignActionMomSwitchScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateAutomaticResignActionMomSwitchScenario,
      federateAutomaticResignActionMomSwitchContractId,
      scenarioFederateAutomaticResignActionMomSwitch,
      scenarioFederateAutomaticResignActionMomSwitchContract);
}

int runFederateAutomaticResignActionMomAttributeScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      federateAutomaticResignActionMomAttributeScenario,
      federateAutomaticResignActionMomAttributeContractId,
      scenarioFederateAutomaticResignActionMomAttribute,
      scenarioFederateAutomaticResignActionMomAttributeContract);
}

int runRegionalAutoProvideResponseScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideResponseScenario,
      regionalAutoProvideResponseContractId,
      scenarioRegionalAutoProvideResponse,
      scenarioRegionalAutoProvideResponseContract);
}

int runRegionalAutoProvideTimestampedResponseScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideTimestampedResponseScenario,
      regionalAutoProvideTimestampedResponseContractId,
      scenarioRegionalAutoProvideTimestampedResponse,
      scenarioRegionalAutoProvideTimestampedResponseContract);
}

int runRegionalAutoProvideOverlapScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideOverlapScenario,
      regionalAutoProvideOverlapContractId,
      scenarioRegionalAutoProvideOverlap,
      scenarioRegionalAutoProvideOverlapContract);
}

int runRegionalAutoProvideMultiSourceScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideMultiSourceScenario,
      regionalAutoProvideMultiSourceContractId,
      scenarioRegionalAutoProvideMultiSource,
      scenarioRegionalAutoProvideMultiSourceContract);
}

int runRegionalAutoProvideMultiProviderScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideMultiProviderScenario,
      regionalAutoProvideMultiProviderContractId,
      scenarioRegionalAutoProvideMultiProvider,
      scenarioRegionalAutoProvideMultiProviderContract);
}

int runRegionalAutoProvideCallbackControlScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      regionalAutoProvideCallbackControlScenario,
      regionalAutoProvideCallbackControlContractId,
      scenarioRegionalAutoProvideCallbackControl,
      scenarioRegionalAutoProvideCallbackControlContract);
}

int runObjectRegistrationServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectRegistrationServiceBoundariesScenario,
      objectRegistrationServiceBoundariesContractId,
      scenarioObjectRegistrationServiceBoundaries,
      scenarioObjectRegistrationServiceBoundariesContract);
}

int runObjectDeletionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      objectDeletionServiceBoundariesScenario,
      objectDeletionServiceBoundariesContractId,
      scenarioObjectDeletionServiceBoundaries,
      scenarioObjectDeletionServiceBoundariesContract);
}

int runAttributeUpdateServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeUpdateServiceBoundariesScenario,
      attributeUpdateServiceBoundariesContractId,
      scenarioAttributeUpdateServiceBoundaries,
      scenarioAttributeUpdateServiceBoundariesContract);
}

int runInteractionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      interactionServiceBoundariesScenario,
      interactionServiceBoundariesContractId,
      scenarioInteractionServiceBoundaries,
      scenarioInteractionServiceBoundariesContract);
}

int runAttributeValueRequestServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      attributeValueRequestServiceBoundariesScenario,
      attributeValueRequestServiceBoundariesContractId,
      scenarioAttributeValueRequestServiceBoundaries,
      scenarioAttributeValueRequestServiceBoundariesContract);
}

int runConnectionServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      connectionServiceBoundariesScenario,
      connectionServiceBoundariesContractId,
      scenarioConnectionServiceBoundaries,
      scenarioConnectionServiceBoundariesContract);
}

int runOwnershipServiceBoundariesScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      ownershipServiceBoundariesScenario,
      ownershipServiceBoundariesContractId,
      scenarioOwnershipServiceBoundaries,
      scenarioOwnershipServiceBoundariesContract);
}

PortableScenario delaySubscriptionEvaluationScenario(std::string const& id) {
  if (id == delaySubscriptionEvaluationInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationInteraction;
  }
  if (id == delaySubscriptionEvaluationInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationInteractionContract;
  }
  if (id == delaySubscriptionEvaluationDirectedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationDirectedInteraction;
  }
  if (id == delaySubscriptionEvaluationDirectedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationDirectedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationAttributeUpdateContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedInteraction;
  }
  if (id == delaySubscriptionEvaluationTimestampedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedDirectedInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction;
  }
  if (id == delaySubscriptionEvaluationTimestampedDirectedInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedDirectedInteractionContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationTimestampedAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdateContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateScenario) {
    return scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate;
  }
  if (id == delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContract;
  }
  if (id == delaySubscriptionEvaluationTimestampedRegionalInteractionScenario) {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalInteraction;
  }
  if (id == delaySubscriptionEvaluationTimestampedRegionalInteractionContractId) {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalInteractionContract;
  }
  return nullptr;
}

bool isDelaySubscriptionEvaluationTimestamped(std::string const& id) {
  return id == delaySubscriptionEvaluationTimestampedInteractionScenario ||
      id == delaySubscriptionEvaluationTimestampedInteractionContractId ||
      id == delaySubscriptionEvaluationTimestampedDirectedInteractionScenario ||
      id == delaySubscriptionEvaluationTimestampedDirectedInteractionContractId ||
      id == delaySubscriptionEvaluationTimestampedAttributeUpdateScenario ||
      id == delaySubscriptionEvaluationTimestampedAttributeUpdateContractId ||
      id == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateScenario ||
      id == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContractId ||
      id == delaySubscriptionEvaluationTimestampedRegionalInteractionScenario ||
      id == delaySubscriptionEvaluationTimestampedRegionalInteractionContractId;
}

int runDelaySubscriptionEvaluationScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = delaySubscriptionEvaluationScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.switchesFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied switch-declaration FOM";
      } else if ((selected == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateScenario ||
                  selected == delaySubscriptionEvaluationTimestampedRegionalAttributeUpdateContractId ||
                  selected == delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateScenario ||
                  selected == delaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdateContractId ||
                  selected == delaySubscriptionEvaluationTimestampedRegionalInteractionScenario ||
                  selected == delaySubscriptionEvaluationTimestampedRegionalInteractionContractId) &&
                 options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else if (isDelaySubscriptionEvaluationTimestamped(selected) &&
                 options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runFederationSaveRestoreInterlocksScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      if (selected != federationSaveRestoreInterlocksScenario &&
          selected != federationSaveRestoreInterlocksContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else if (options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else {
        try {
          if (selected == federationSaveRestoreInterlocksScenario) {
            scenarioFederationSaveRestoreInterlocks(options, callback.second);
          } else {
            scenarioFederationSaveRestoreInterlocksContract(
                options,
                callback.second);
          }
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

PortableScenario timedSaveRestoreScenario(std::string const& id) {
  if (id == fqrTimedFederationSaveCallbackScenario) {
    return scenarioFqrTimedFederationSaveCallback;
  }
  if (id == fqrTimedFederationSaveCallbackContractId) {
    return scenarioFqrTimedFederationSaveCallbackContract;
  }
  if (id == timedFederationSaveRestoreScenario) {
    return scenarioTimedFederationSaveRestore;
  }
  if (id == timedFederationSaveRestoreContractId) {
    return scenarioTimedFederationSaveRestoreContract;
  }
  if (id == timedRegionalInteractionSaveRestoreScenario) {
    return scenarioTimedRegionalInteractionSaveRestore;
  }
  if (id == timedRegionalInteractionSaveRestoreContractId) {
    return scenarioTimedRegionalInteractionSaveRestoreContract;
  }
  if (id == timedDefaultRegionInteractionSaveRestoreScenario) {
    return scenarioTimedDefaultRegionInteractionSaveRestore;
  }
  if (id == timedDefaultRegionInteractionSaveRestoreContractId) {
    return scenarioTimedDefaultRegionInteractionSaveRestoreContract;
  }
  if (id == timedDefaultRegionAttributeSaveRestoreScenario) {
    return scenarioTimedDefaultRegionAttributeSaveRestore;
  }
  if (id == timedDefaultRegionAttributeSaveRestoreContractId) {
    return scenarioTimedDefaultRegionAttributeSaveRestoreContract;
  }
  return nullptr;
}

bool timedSaveRestoreRequiresDdm(std::string const& id) {
  return id == timedRegionalInteractionSaveRestoreScenario ||
      id == timedRegionalInteractionSaveRestoreContractId ||
      id == timedDefaultRegionInteractionSaveRestoreScenario ||
      id == timedDefaultRegionInteractionSaveRestoreContractId ||
      id == timedDefaultRegionAttributeSaveRestoreScenario ||
      id == timedDefaultRegionAttributeSaveRestoreContractId;
}

int runTimedSaveRestoreScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = timedSaveRestoreScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if (options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied logical-time implementation";
      } else if (timedSaveRestoreRequiresDdm(selected) && options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

PortableScenario regionalDdmScenario(std::string const& id) {
  if (id == timestampedAttributeReflectRegionSnapshotScenario) {
    return scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshot;
  }
  if (id == timestampedAttributeReflectRegionSnapshotContractId) {
    return scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshotContract;
  }
  if (id == regionalMultiAttributeUpdateScenario) {
    return scenarioRegionalMultiAttributeUpdate;
  }
  if (id == regionalMultiAttributeUpdateContractId) {
    return scenarioRegionalMultiAttributeUpdateContract;
  }
  if (id == regionalThreeDimensionalOverlapScenario) {
    return scenarioRegionalThreeDimensionalOverlap;
  }
  if (id == regionalThreeDimensionalOverlapContractId) {
    return scenarioRegionalThreeDimensionalOverlapContract;
  }
  if (id == regionalAttributeUpdateCallbackDdmRecheckScenario) {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheck;
  }
  if (id == regionalAttributeUpdateCallbackDdmRecheckContractScenario) {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheckContract;
  }
  if (id == regionalAttributeValueRequestFilteringScenario) {
    return scenarioRegionalAttributeValueRequestFiltering;
  }
  if (id == regionalAttributeValueRequestFilteringContractScenario) {
    return scenarioRegionalAttributeValueRequestFilteringContract;
  }
  if (id == regionalAttributeValueUpdateResponseRecheckScenario) {
    return scenarioRegionalAttributeValueUpdateResponseRecheck;
  }
  if (id == regionalAttributeValueUpdateResponseRecheckContractScenario) {
    return scenarioRegionalAttributeValueUpdateResponseRecheckContract;
  }
  if (id == allowRelaxedDdmScenario) {
    return scenarioAllowRelaxedDdm;
  }
  if (id == allowRelaxedDdmContractId) {
    return scenarioAllowRelaxedDdmContract;
  }
  if (id == regionalBoundariesScenario) {
    return scenarioRegionalBoundaries;
  }
  if (id == regionalBoundariesContractId) {
    return scenarioRegionalBoundariesContract;
  }
  if (id == ownershipTransferRegionalUpdateScenario) {
    return scenarioOwnershipTransferRegionalUpdate;
  }
  if (id == ownershipTransferRegionalUpdateContractId) {
    return scenarioOwnershipTransferRegionalUpdateContract;
  }
  return nullptr;
}

int runRegionalDdmScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& selected : options.scenarios) {
      auto const scenario = regionalDdmScenario(selected);
      if (scenario == nullptr) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{selected, callback.first, "passed", "", 0};
      if ((selected == timestampedAttributeReflectRegionSnapshotScenario ||
           selected == timestampedAttributeReflectRegionSnapshotContractId) &&
          callback.second != rti::HLA_EVOKED) {
        result.status = "skipped";
        result.message =
            "requires HLA_EVOKED for callback-time region-designator queries";
      } else if ((selected == timestampedAttributeReflectRegionSnapshotScenario ||
                  selected == timestampedAttributeReflectRegionSnapshotContractId) &&
                 options.multiAttributeFom.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied multi-attribute dimensional FOM";
      } else if ((selected == timestampedAttributeReflectRegionSnapshotScenario ||
                  selected == timestampedAttributeReflectRegionSnapshotContractId) &&
                 options.logicalTimeImplementationName.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied logical-time implementation";
      } else if ((selected == allowRelaxedDdmScenario ||
           selected == allowRelaxedDdmContractId) &&
          (options.ddmFom.empty() || options.switchesFom.empty())) {
        result.status = "skipped";
        result.message =
            "requires adapter-supplied dimensional and switch-declaration FOMs";
      } else if ((selected == regionalBoundariesScenario ||
                  selected == regionalBoundariesContractId ||
                  selected == ownershipTransferRegionalUpdateScenario ||
                  selected == ownershipTransferRegionalUpdateContractId) &&
                 options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      } else if ((selected == regionalMultiAttributeUpdateScenario ||
                  selected == regionalMultiAttributeUpdateContractId) &&
                 options.multiAttributeFom.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied multi-attribute dimensional FOM";
      } else if ((selected == regionalThreeDimensionalOverlapScenario ||
                  selected == regionalThreeDimensionalOverlapContractId) &&
                 options.threeDimensionalFom.empty()) {
        result.status = "skipped";
        result.message =
            "requires an adapter-supplied three-dimensional DDM FOM";
      } else if ((selected == regionalAttributeUpdateCallbackDdmRecheckScenario ||
                  selected == regionalAttributeUpdateCallbackDdmRecheckContractScenario ||
                  selected == regionalAttributeValueRequestFilteringScenario ||
                  selected == regionalAttributeValueRequestFilteringContractScenario ||
                  selected == regionalAttributeValueUpdateResponseRecheckScenario ||
                  selected == regionalAttributeValueUpdateResponseRecheckContractScenario) &&
                 options.ddmFom.empty()) {
        result.status = "skipped";
        result.message = "requires an adapter-supplied dimensional FOM";
      }
      if (result.status == "passed") {
        try {
          scenario(options, callback.second);
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

int runCustomTransportationTimestampedRegionalInteractionDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedRegionalInteractionDeliveryId,
      customTransportationTimestampedRegionalInteractionDeliveryContractId,
      scenarioCustomTransportationTimestampedRegionalInteractionDelivery,
      scenarioCustomTransportationTimestampedRegionalInteractionDeliveryContract);
}

int runCustomTransportationAttributeDeliveryScenarios(int argc, char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationAttributeDeliveryId,
      customTransportationAttributeDeliveryContractId,
      scenarioCustomTransportationAttributeDelivery,
      scenarioCustomTransportationAttributeDeliveryContract);
}

int runCustomTransportationTimestampedAttributeDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedAttributeDeliveryId,
      customTransportationTimestampedAttributeDeliveryContractId,
      scenarioCustomTransportationTimestampedAttributeDelivery,
      scenarioCustomTransportationTimestampedAttributeDeliveryContract);
}

int runCustomTransportationTimestampedAttributeAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedAttributeAlternateAdvancesId,
      customTransportationTimestampedAttributeAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedAttributeAlternateAdvances,
      scenarioCustomTransportationTimestampedAttributeAlternateAdvancesContract);
}

int runCustomTransportationTimestampedInteractionAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedInteractionAlternateAdvancesId,
      customTransportationTimestampedInteractionAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedInteractionAlternateAdvances,
      scenarioCustomTransportationTimestampedInteractionAlternateAdvancesContract);
}

int runCustomTransportationTimestampedDirectedDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-timestamped-directed-delivery",
      "cpp-tck.custom-transportation-timestamped-directed-delivery-contract",
      scenarioCustomTransportationTimestampedDirectedDeliveryPortable,
      scenarioCustomTransportationTimestampedDirectedDeliveryPortableContract);
}

int runCustomTransportationInteractionDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-interaction-delivery",
      "cpp-tck.custom-transportation-interaction-delivery-contract",
      scenarioCustomTransportationInteractionDeliveryPortable,
      scenarioCustomTransportationInteractionDeliveryPortableContract);
}

int runCustomTransportationTimestampedDeliveryPortableScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      "cpp-tck.custom-transportation-timestamped-delivery",
      "cpp-tck.custom-transportation-timestamped-delivery-contract",
      scenarioCustomTransportationTimestampedDeliveryPortable,
      scenarioCustomTransportationTimestampedDeliveryPortableContract);
}

int runCustomTransportationDirectedInteractionDeliveryScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationDirectedInteractionDeliveryId,
      customTransportationDirectedInteractionDeliveryContractId,
      scenarioCustomTransportationDirectedInteractionDelivery,
      scenarioCustomTransportationDirectedInteractionDeliveryContract);
}

int runCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenarios(
    int argc,
    char** argv) {
  return runPortableScenarioPair(
      argc,
      argv,
      customTransportationTimestampedDirectedInteractionAlternateAdvancesId,
      customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId,
      scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvances,
      scenarioCustomTransportationTimestampedDirectedInteractionAlternateAdvancesContract);
}

int runAutomaticResignDirectiveScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != automaticResignDirectiveDeleteObjectsId &&
          scenario != automaticResignDirectiveDeleteObjectsContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      if (!options.connectionLossServerManaged) {
        result.status = "skipped";
        result.message =
            "requires an adapter-managed connection-loss fixture";
      } else {
        try {
          if (scenario == automaticResignDirectiveDeleteObjectsId) {
            scenarioAutomaticResignDirectiveDeleteObjects(options, callback.second);
          } else {
            scenarioAutomaticResignDirectiveDeleteObjectsContract(
                options,
                callback.second);
          }
        } catch (rti::Exception const& error) {
          result.status = "failed";
          result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
        } catch (std::exception const& error) {
          result.status = "failed";
          result.message = error.what();
        } catch (...) {
          result.status = "failed";
          result.message = "unknown non-standard exception";
        }
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasAutomaticResignDirectiveScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == automaticResignDirectiveDeleteObjectsId ||
        scenario == automaticResignDirectiveDeleteObjectsContractId) {
      return true;
    }
  }
  return false;
}

int runPublicHandleDecodingScenarios(int argc, char** argv) {
  auto const options = parseOptions(argc, argv);
  std::vector<ScenarioResult> results;
  for (auto const& callback : callbackModels(options)) {
    for (auto const& scenario : options.scenarios) {
      if (scenario != publicHandleDecodingId &&
          scenario != publicHandleDecodingContractId) {
        continue;
      }
      auto const started = Clock::now();
      ScenarioResult result{scenario, callback.first, "passed", "", 0};
      try {
        if (scenario == publicHandleDecodingId) {
          scenarioPublicHandleDecoding(options, callback.second);
        } else {
          scenarioPublicHandleDecodingContract(options, callback.second);
        }
      } catch (rti::Exception const& error) {
        result.status = "failed";
        result.message = toNarrow(error.name()) + ": " + toNarrow(error.what());
      } catch (std::exception const& error) {
        result.status = "failed";
        result.message = error.what();
      } catch (...) {
        result.status = "failed";
        result.message = "unknown non-standard exception";
      }
      result.durationMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
          Clock::now() - started).count();
      results.push_back(result);
      std::cout << result.status << " " << result.id << " ["
                << result.callbackModel << "]";
      if (!result.message.empty()) {
        std::cout << ": " << result.message;
      }
      std::cout << '\n';
    }
  }
  if (!options.results.empty()) {
    writeResults(options.results, options, results);
  }
  if (!options.junit.empty()) {
    writeJUnit(options.junit, results);
  }
  auto const failures = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "failed"; });
  auto const skipped = std::count_if(
      results.begin(),
      results.end(),
      [](auto const& result) { return result.status == "skipped"; });
  std::cout << "summary passed=" << results.size() - failures - skipped
            << " skipped=" << skipped << " failed=" << failures << '\n';
  return failures == 0 ? 0 : 1;
}

bool hasPublicHandleDecodingScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == publicHandleDecodingId ||
        scenario == publicHandleDecodingContractId) {
      return true;
    }
  }
  return false;
}

bool hasMomTransportationTypeChangeRequestScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == momTransportationTypeChangeRequestScenario ||
        scenario == momTransportationTypeChangeRequestContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomRegisteredObjectCountScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomRegisteredObjectCountScenario ||
        scenario == joinedFederateMomRegisteredObjectCountContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomRoLengthPeriodicScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomRoLengthPeriodicScenario ||
        scenario == joinedFederateMomRoLengthPeriodicContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomTsoLengthPeriodicScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomTsoLengthPeriodicScenario ||
        scenario == joinedFederateMomTsoLengthPeriodicContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomInteractionsReceivedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomInteractionsReceivedAttributeScenario ||
        scenario == joinedFederateMomInteractionsReceivedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomInteractionsSentAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomInteractionsSentAttributeScenario ||
        scenario == joinedFederateMomInteractionsSentAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDirectedInteractionsReceivedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDirectedInteractionsReceivedAttributeScenario ||
        scenario == joinedFederateMomDirectedInteractionsReceivedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDirectedInteractionsSentAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDirectedInteractionsSentAttributeScenario ||
        scenario == joinedFederateMomDirectedInteractionsSentAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomUpdatesSentAttributeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomUpdatesSentAttributeScenario ||
        scenario == joinedFederateMomUpdatesSentAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesUpdatedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesUpdatedAttributeScenario ||
        scenario == joinedFederateMomObjectInstancesUpdatedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesReflectedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesReflectedAttributeScenario ||
        scenario == joinedFederateMomObjectInstancesReflectedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomReflectionsReceivedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomReflectionsReceivedAttributeScenario ||
        scenario == joinedFederateMomReflectionsReceivedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesDiscoveredAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesDiscoveredAttributeScenario ||
        scenario ==
            joinedFederateMomObjectInstancesDiscoveredAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesRemovedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesRemovedAttributeScenario ||
        scenario == joinedFederateMomObjectInstancesRemovedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesDeletedAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesDeletedAttributeScenario ||
        scenario == joinedFederateMomObjectInstancesDeletedAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectLifecycleScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectLifecycleScenario ||
        scenario == joinedFederateMomObjectLifecycleContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDeletableObjectReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDeletableObjectReportScenario ||
        scenario == joinedFederateMomDeletableObjectReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesUpdatedReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesUpdatedReportScenario ||
        scenario == joinedFederateMomObjectInstancesUpdatedReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesUpdatedTimestampedReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesUpdatedTimestampedReportScenario ||
        scenario == joinedFederateMomObjectInstancesUpdatedTimestampedReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesUpdatedMultiClassReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesUpdatedMultiClassReportScenario ||
        scenario == joinedFederateMomObjectInstancesUpdatedMultiClassReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesUpdatedNullReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesUpdatedNullReportScenario ||
        scenario == joinedFederateMomObjectInstancesUpdatedNullReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesReflectedNullReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesReflectedNullReportScenario ||
        scenario == joinedFederateMomObjectInstancesReflectedNullReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesReflectedReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesReflectedReportScenario ||
        scenario == joinedFederateMomObjectInstancesReflectedReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesReflectedTimestampedReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesReflectedTimestampedReportScenario ||
        scenario == joinedFederateMomObjectInstancesReflectedTimestampedReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomObjectInstancesReflectedMultiClassReportScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomObjectInstancesReflectedMultiClassReportScenario ||
        scenario == joinedFederateMomObjectInstancesReflectedMultiClassReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasJoinedFederateMomDeletableObjectCountScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == joinedFederateMomDeletableObjectCountScenario ||
        scenario == joinedFederateMomDeletableObjectCountContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderAttributeUpdateScenario ||
        scenario == receiveOrderAttributeUpdateContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeUpdateOrderClassificationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeUpdateOrderClassificationScenario ||
        scenario == attributeUpdateOrderClassificationContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderInteractionScenario ||
        scenario == receiveOrderInteractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionOrderClassificationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionOrderClassificationScenario ||
        scenario == interactionOrderClassificationContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionPublicationSendTransitionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionPublicationSendTransitionScenario ||
        scenario == interactionPublicationSendTransitionContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionActiveAndPassiveSubscriptionModesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionActiveAndPassiveSubscriptionModesScenario ||
        scenario == interactionActiveAndPassiveSubscriptionModesContractId) {
      return true;
    }
  }
  return false;
}

bool hasReceiveOrderObjectRemovalScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == receiveOrderObjectRemovalScenario ||
        scenario == receiveOrderObjectRemovalContractId) {
      return true;
    }
  }
  return false;
}

bool hasObjectRemovalOrderClassificationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectRemovalOrderClassificationScenario ||
        scenario == objectRemovalOrderClassificationContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationRestoreAbortScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationRestoreAbortScenario ||
        scenario == federationRestoreAbortContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationRestoreOwnershipAssumptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationRestoreOwnershipAssumptionScenario ||
        scenario == federationRestoreOwnershipAssumptionContractId) {
      return true;
    }
  }
  return false;
}

bool hasPendingNegotiatedRestorePackageScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == pendingNegotiatedRestorePackageScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportCreationFailureScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == serviceReportCreationFailureScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportConnectFailureScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == serviceReportConnectFailureScenario) {
      return true;
    }
  }
  return false;
}

bool hasOwnershipAcquisitionIfAvailableScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == ownershipAcquisitionIfAvailableScenario ||
        scenario == ownershipAcquisitionIfAvailableContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipAcquisitionCancellationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipAcquisitionCancellationScenario ||
        scenario == attributeOwnershipAcquisitionCancellationContractId) {
      return true;
    }
  }
  return false;
}

bool hasNegotiatedDivestitureConfirmationFlowScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == negotiatedDivestitureConfirmationFlowScenario ||
        scenario == negotiatedDivestitureConfirmationFlowContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipReleaseDeniedScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipReleaseDeniedScenario ||
        scenario == attributeOwnershipReleaseDeniedContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipDivestitureIfWantedScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipDivestitureIfWantedScenario ||
        scenario == attributeOwnershipDivestitureIfWantedContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipQueryOwnerReportScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipQueryOwnerReportScenario ||
        scenario == attributeOwnershipQueryOwnerReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipQueryUnownedResultScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipQueryUnownedResultScenario ||
        scenario == attributeOwnershipQueryUnownedResultContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeOwnershipQueryRtiOwnedResultScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeOwnershipQueryRtiOwnedResultScenario ||
        scenario == attributeOwnershipQueryRtiOwnedResultContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionRetractionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionRetractionScenario ||
        scenario == timestampedDirectedInteractionRetractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionRetractionFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionRetractionFanoutScenario ||
        scenario == timestampedDirectedInteractionRetractionFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionsScenario ||
        scenario == timestampedInteractionsContractId) {
      return true;
    }
  }
  return false;
}

bool hasUnregulatedSenderTimestampedInteractionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == unregulatedSenderTimestampedInteractionScenario ||
        scenario == unregulatedSenderTimestampedInteractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionPreferredReceiveConversionScenario(
    int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionPreferredReceiveConversionScenario ||
        scenario == timestampedInteractionPreferredReceiveConversionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionSenderRegulationTransitionScenario(
    int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionSenderRegulationTransitionScenario ||
        scenario == timestampedInteractionSenderRegulationTransitionContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionFddPreferredOrderDefaultScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionFddPreferredOrderDefaultScenario ||
        scenario == interactionFddPreferredOrderDefaultContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeFddPreferredOrderDefaultScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeFddPreferredOrderDefaultScenario ||
        scenario == attributeFddPreferredOrderDefaultContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDeletePrivilegePreferredOrderScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDeletePrivilegePreferredOrderScenario ||
        scenario == timestampedDeletePrivilegePreferredOrderContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDeletePrivilegeTimestampOrderScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDeletePrivilegeTimestampOrderScenario ||
        scenario == timestampedDeletePrivilegeTimestampOrderContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateScenario ||
        scenario == timestampedAttributeUpdateContractId) {
      return true;
    }
  }
  return false;
}

bool hasJavaTimestampedAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == javaTimestampedAttributeUpdateScenario) {
      return true;
    }
  }
  return false;
}

bool hasJavaTimestampedObjectRemovalScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == javaTimestampedObjectRemovalScenario) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateReenableScenario ||
        scenario == timestampedAttributeUpdateReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateRegulationReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateRegulationReenableScenario ||
        scenario == timestampedAttributeUpdateRegulationReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeSourceResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeSourceResignationScenario ||
        scenario == timestampedAttributeSourceResignationContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeSourceResignationFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeSourceResignationFanoutScenario ||
        scenario == timestampedAttributeSourceResignationFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateOwnershipTransferScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateOwnershipTransferScenario ||
        scenario == timestampedAttributeUpdateOwnershipTransferContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateQueuedPasselRetractionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateQueuedPasselRetractionScenario ||
        scenario == timestampedAttributeUpdateQueuedPasselRetractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasMixedOrderAttributePasselRetractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == mixedOrderAttributePasselRetractionScenario ||
        scenario == mixedOrderAttributePasselRetractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeImmediateRecipientMultiTransportRetractionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeImmediateRecipientMultiTransportRetractionScenario ||
        scenario == timestampedAttributeImmediateRecipientMultiTransportRetractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeSenderRegulationTransitionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeSenderRegulationTransitionScenario ||
        scenario == timestampedAttributeSenderRegulationTransitionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateNoFanoutScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateNoFanoutScenario ||
        scenario == timestampedAttributeUpdateNoFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateFlushQueueFutureInputScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateFlushQueueFutureInputScenario ||
        scenario == timestampedAttributeUpdateFlushQueueFutureInputContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionFlushQueueFutureInputScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionFlushQueueFutureInputScenario ||
        scenario == timestampedInteractionFlushQueueFutureInputContractId) {
      return true;
    }
  }
  return false;
}

bool hasFqrPostgrantLowerTsoRetractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == fqrPostgrantLowerTsoRetractionScenario ||
        scenario == fqrPostgrantLowerTsoRetractionContractId) {
      return true;
    }
  }
  return false;
}

bool hasFqgPostgrantDeliveryCutoffScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == fqgPostgrantDeliveryCutoffScenario ||
        scenario == fqgPostgrantDeliveryCutoffContractId) {
      return true;
    }
  }
  return false;
}

bool hasTarQueuedTsoBeforeGrantScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == tarQueuedTsoBeforeGrantScenario ||
        scenario == tarQueuedTsoBeforeGrantContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeOrderCohortScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeOrderCohortScenario ||
        scenario == timestampedAttributeOrderCohortContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionNoFanoutScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionNoFanoutScenario ||
        scenario == timestampedObjectDeletionNoFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionTombstoneScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionTombstoneScenario ||
        scenario == timestampedObjectDeletionTombstoneContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionRetractionJoinedOwnersScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionRetractionJoinedOwnersScenario ||
        scenario == timestampedObjectDeletionRetractionJoinedOwnersContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionSourceResignationFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionSourceResignationFanoutScenario ||
        scenario == timestampedObjectDeletionSourceResignationFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionScenario ||
        scenario == timestampedObjectDeletionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionRegulationReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionRegulationReenableScenario ||
        scenario == timestampedObjectDeletionRegulationReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasAlternateTimeAdvancesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == alternateTimeAdvancesScenario ||
        scenario == alternateTimeAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasPendingAlternateAdvanceStateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == pendingAlternateAdvanceStateScenario ||
        scenario == pendingAlternateAdvanceStateContractId) {
      return true;
    }
  }
  return false;
}

bool hasNextMessageRequestScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nextMessageRequestScenario ||
        scenario == nextMessageRequestContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmraNoPregrantTsoFloorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmraNoPregrantTsoFloorScenario ||
        scenario == nmraNoPregrantTsoFloorContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrNoPregrantTsoFloorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrNoPregrantTsoFloorScenario ||
        scenario == nmrNoPregrantTsoFloorContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrReceivedTsoFloorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrReceivedTsoFloorScenario ||
        scenario == nmrReceivedTsoFloorContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrReceivedTsoAttributeUpdateFloorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrReceivedTsoAttributeUpdateFloorScenario ||
        scenario == nmrReceivedTsoAttributeUpdateFloorContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrReceivedTsoDirectedInteractionFloorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrReceivedTsoDirectedInteractionFloorScenario ||
        scenario == nmrReceivedTsoDirectedInteractionFloorContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrEmptyMessageSetGrantTimeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrEmptyMessageSetGrantTimeScenario ||
        scenario == nmrEmptyMessageSetGrantTimeContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrRequestBelowGaltImmediateGrantScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrRequestBelowGaltImmediateGrantScenario ||
        scenario == nmrRequestBelowGaltImmediateGrantContractId) {
      return true;
    }
  }
  return false;
}

bool hasNmrLitsBelowGaltImmediateGrantScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == nmrLitsBelowGaltImmediateGrantScenario ||
        scenario == nmrLitsBelowGaltImmediateGrantContractId) {
      return true;
    }
  }
  return false;
}

bool hasSendMessageTimeStatePreservationGrantedScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == sendMessageTimeStatePreservationGrantedScenario ||
        scenario == sendMessageTimeStatePreservationGrantedContractId) {
      return true;
    }
  }
  return false;
}

bool hasAvailableTimeAdvancesInclusiveGaltScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == availableTimeAdvancesInclusiveGaltScenario ||
        scenario == availableTimeAdvancesInclusiveGaltContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimeConstrainedEnableAtGaltEqualityScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const selected = std::string(argv[index + 1]);
    if (selected == timeConstrainedEnableAtGaltEqualityScenario ||
        selected == timeConstrainedEnableAtGaltEqualityContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimeRegulationImplicitAdvanceScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const selected = std::string(argv[index + 1]);
    if (selected == timeRegulationImplicitAdvanceScenario ||
        selected == timeRegulationImplicitAdvanceContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionMixedAdvancesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionMixedAdvancesScenario ||
        scenario == timestampedInteractionMixedAdvancesContractId ||
        scenario == timestampedInteractionTaraEqualBoundaryScenario ||
        scenario == timestampedInteractionTaraEqualBoundaryContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimeCoordinatedInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == timeCoordinatedInteractionScenario) {
      return true;
    }
  }
  return false;
}

bool hasEmptyAttributeSubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == emptyAttributeSubscriptionScenario) {
      return true;
    }
  }
  return false;
}

bool hasNamedUpdateRateSubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == namedUpdateRateSubscriptionScenario) {
      return true;
    }
  }
  return false;
}

bool hasDefaultRateNoReductionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == defaultRateNoReductionScenario) {
      return true;
    }
  }
  return false;
}

bool hasAccumulatingAttributeSubscriptionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) ==
            accumulatingAttributeSubscriptionsScenario) {
      return true;
    }
  }
  return false;
}

bool hasAttributeSubscriptionModeTransitionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeSubscriptionModeTransitionsScenario ||
        scenario == attributeSubscriptionModeTransitionsContractId) {
      return true;
    }
  }
  return false;
}

bool hasPassiveSubscriptionNoStartRegistrationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) ==
            passiveSubscriptionNoStartRegistrationScenario) {
      return true;
    }
  }
  return false;
}

bool hasSelectiveAttributeUnsubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == selectiveAttributeUnsubscriptionScenario) {
      return true;
    }
  }
  return false;
}

bool hasWholeClassAttributeUnsubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == wholeClassAttributeUnsubscriptionScenario) {
      return true;
    }
  }
  return false;
}

bool hasEmptyAttributeUnsubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == emptyAttributeUnsubscriptionScenario) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionTarEqualBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionTarEqualBoundaryScenario ||
        scenario == timestampedInteractionTarEqualBoundaryContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionTarPostGrantCutoffScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionTarPostGrantCutoffScenario ||
        scenario == timestampedInteractionTarPostGrantCutoffContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeTarPostGrantCutoffScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeTarPostGrantCutoffScenario ||
        scenario == timestampedAttributeTarPostGrantCutoffContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeTimeAdvancingCutoffScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeTimeAdvancingCutoffScenario ||
        scenario == timestampedAttributeTimeAdvancingCutoffContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionPublicationSendTransitionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionPublicationSendTransitionScenario ||
        scenario == timestampedInteractionPublicationSendTransitionContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionSourceResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionSourceResignationScenario ||
        scenario == timestampedInteractionSourceResignationContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionSourceResignationFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionSourceResignationFanoutScenario ||
        scenario == timestampedInteractionSourceResignationFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionTsoDesignatorTerminalizationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionTsoDesignatorTerminalizationScenario ||
        scenario == timestampedInteractionTsoDesignatorTerminalizationContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionCrossProducerOrderScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionCrossProducerOrderScenario ||
        scenario == timestampedInteractionCrossProducerOrderContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionNoFanoutScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionNoFanoutScenario ||
        scenario == timestampedInteractionNoFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionRetractionFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionRetractionFanoutScenario ||
        scenario == timestampedInteractionRetractionFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionReenableScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionReenableScenario ||
        scenario == timestampedInteractionReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionReenableScenario ||
        scenario == timestampedDirectedInteractionReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedInteractionRegulationReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedInteractionRegulationReenableScenario ||
        scenario == timestampedInteractionRegulationReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionRegulationReenableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionRegulationReenableScenario ||
        scenario == timestampedDirectedInteractionRegulationReenableContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedAlternateAdvancesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedAlternateAdvancesScenario ||
        scenario == timestampedDirectedAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedAttributeUpdateAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedAttributeUpdateAlternateAdvancesScenario ||
        scenario == timestampedAttributeUpdateAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionMixedAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionMixedAdvancesScenario ||
        scenario == timestampedObjectDeletionMixedAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionFutureNmrArrivalScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionFutureNmrArrivalScenario ||
        scenario == timestampedObjectDeletionFutureNmrArrivalContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedObjectDeletionFutureNmraArrivalScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedObjectDeletionFutureNmraArrivalScenario ||
        scenario == timestampedObjectDeletionFutureNmraArrivalContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionTarNmrScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionTarNmrScenario ||
        scenario == timestampedDirectedInteractionTarNmrContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionSourceResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionSourceResignationScenario ||
        scenario == timestampedDirectedInteractionSourceResignationContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionSourceResignationFanoutScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionSourceResignationFanoutScenario ||
        scenario == timestampedDirectedInteractionSourceResignationFanoutContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimestampedDirectedInteractionImmediateSourceResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == timestampedDirectedInteractionImmediateSourceResignationScenario ||
        scenario == timestampedDirectedInteractionImmediateSourceResignationContractId) {
      return true;
    }
  }
  return false;
}

bool hasModifyLookaheadScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == modifyLookaheadScenario ||
        scenario == modifyLookaheadContractId) {
      return true;
    }
  }
  return false;
}

bool hasModifyLookaheadServiceReportScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == modifyLookaheadServiceReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasTransportationTypeChangeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == transportationTypeChangeScenario ||
        scenario == transportationTypeChangeContractId) {
      return true;
    }
  }
  return false;
}

bool hasOrderTypeChangeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == orderTypeChangeScenario ||
        scenario == orderTypeChangeContractId) {
      return true;
    }
  }
  return false;
}

bool hasTransportOrderScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == transportOrderScenario ||
        scenario == transportOrderContractId) {
      return true;
    }
  }
  return false;
}

bool hasStandardOrderAndTransportationLookupScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == standardOrderAndTransportationLookupsScenario ||
        scenario == standardOrderAndTransportationLookupsContractId) {
      return true;
    }
  }
  return false;
}

bool hasFactoryDiscoveryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == factoryDiscoveryScenario ||
        scenario == rtiAmbassadorFactoryContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasLogicalTimeFactoryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == logicalTimeFactoryScenario) {
      return true;
    }
  }
  return false;
}

bool hasVariableLengthDataScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == variableLengthDataContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasLogicalTimeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == logicalTimeContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasLogicalTimeFactoryFactoryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == logicalTimeFactoryFactoryContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasLogicalTimeDataElementsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == logicalTimeDataElementsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasExceptionHierarchyScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == exceptionHierarchyContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasEnumScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == enumContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasHandleAndCollectionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == handleAndCollectionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasDataElementScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == dataElementContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasBasicDataElementsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == basicDataElementsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasCompositeDataElementsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == compositeDataElementsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasConfigurationAndAuthorizationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) ==
        configurationAndAuthorizationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRuntimeIdentityScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) == runtimeIdentityContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasAuthorizerFactoryFactoryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) ==
        authorizerFactoryFactoryContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasNullFederateAmbassadorScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (std::string(argv[index + 1]) ==
        nullFederateAmbassadorContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasConnectionCallbackFoundationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == connectionScenario ||
        scenario == connectionCallbackContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasCallbackReentrancyScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == callbackReentrancyScenario ||
        scenario == callbackReentrancyContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasMalformedInputsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == malformedInputsScenario ||
        scenario == malformedInputsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationLifecycleScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationLifecycleScenario ||
        scenario == federationLifecycleContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationManagementRequiresConnectionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) == "--scenario" &&
        std::string(argv[index + 1]) == federationManagementRequiresConnectionScenario) {
      return true;
    }
  }
  return false;
}

bool hasUnnamedJoinOverloadScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == unnamedJoinOverloadScenario ||
        scenario == unnamedJoinOverloadContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationListServicesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationListServicesScenario ||
        scenario == federationListServicesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederateLookupLifecycleScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateLookupLifecycleScenario ||
        scenario == federateLookupLifecycleContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasExplicitMimCreationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == explicitMimCreationScenario ||
        scenario == explicitMimCreationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomCurrentFddScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomCurrentFddScenario ||
        scenario == federationMomCurrentFddContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomStaticIdentityScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomStaticIdentityScenario ||
        scenario == federationMomStaticIdentityContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomStaticSwitchesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomStaticSwitchesScenario ||
        scenario == federationMomStaticSwitchesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomFederatesInFederationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomFederatesInFederationScenario ||
        scenario == federationMomFederatesInFederationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomFomModuleDesignatorListScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomFomModuleDesignatorListScenario ||
        scenario == federationMomFomModuleDesignatorListContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomContentReportsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomContentReportsScenario ||
        scenario == federationMomContentReportsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportOrderTransportationLookupsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportOrderTransportationLookupsScenario ||
        scenario == serviceReportOrderTransportationLookupsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDimensionUpperBoundScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDimensionUpperBoundScenario ||
        scenario == serviceReportDimensionUpperBoundContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDimensionHandleSetScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDimensionHandleSetScenario ||
        scenario == serviceReportDimensionHandleSetContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportRangeBoundsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportRangeBoundsScenario ||
        scenario == serviceReportRangeBoundsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportSetRangeBoundsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportSetRangeBoundsScenario ||
        scenario == serviceReportSetRangeBoundsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportCommitRegionModificationsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportCommitRegionModificationsScenario ||
        scenario == serviceReportCommitRegionModificationsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDeleteRegionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDeleteRegionScenario ||
        scenario == serviceReportDeleteRegionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportCreateRegionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportCreateRegionScenario ||
        scenario == serviceReportCreateRegionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRegionRangeValidationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionRangeValidationScenario ||
        scenario == regionRangeValidationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRegionRangeDimensionValidationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionRangeDimensionValidationScenario ||
        scenario == regionRangeDimensionValidationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDimensionNameScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDimensionNameScenario ||
        scenario == serviceReportDimensionNameContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDimensionHandleScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDimensionHandleScenario ||
        scenario == serviceReportDimensionHandleContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionDimensionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionDimensionsScenario ||
        scenario == serviceReportInteractionDimensionsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportObjectDimensionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportObjectDimensionsScenario ||
        scenario == serviceReportObjectDimensionsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportOrderTransportationLookupFailuresScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportOrderTransportationLookupFailuresScenario ||
        scenario == serviceReportOrderTransportationLookupFailuresContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportFederateObjectClassLookupsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportFederateObjectClassLookupsScenario ||
        scenario == serviceReportFederateObjectClassLookupsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportFederateObjectClassLookupFailuresScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportFederateObjectClassLookupFailuresScenario ||
        scenario == serviceReportFederateObjectClassLookupFailuresContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionParameterLookupsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionParameterLookupsScenario ||
        scenario == serviceReportInteractionParameterLookupsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionParameterLookupFailuresScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionParameterLookupFailuresScenario ||
        scenario == serviceReportInteractionParameterLookupFailuresContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionScenario ||
        scenario == serviceReportInteractionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportResignFederationExecutionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportResignFederationExecutionScenario ||
        scenario == serviceReportResignFederationExecutionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasFederateLostMomReportScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateLostReportScenario ||
        scenario == federateLostReportContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasHandleNormalizationMembershipBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == handleNormalizationMembershipBoundariesScenario ||
        scenario == handleNormalizationMembershipBoundariesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasDimensionLookupValidHandlesAfterResignationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == dimensionLookupValidHandlesAfterResignationScenario ||
        scenario == dimensionLookupValidHandlesAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasUpdateRateLookupValidInputsAfterResignationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == updateRateLookupValidInputsAfterResignationScenario ||
        scenario == updateRateLookupValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasSupportLookupValidInputsAfterResignationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == supportLookupValidInputsAfterResignationScenario ||
        scenario == supportLookupValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasSupportSwitchGetterLifecycleBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == supportSwitchGetterLifecycleBoundariesScenario ||
        scenario == supportSwitchGetterLifecycleBoundariesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasSupportSwitchSetterLifecycleBoundariesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == supportSwitchSetterLifecycleBoundariesScenario ||
        scenario == supportSwitchSetterLifecycleBoundariesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasAutomaticResignDirectiveLifecycleBoundariesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == automaticResignDirectiveLifecycleBoundariesScenario ||
        scenario == automaticResignDirectiveLifecycleBoundariesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasAutomaticResignDirectiveEnumValuesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == automaticResignDirectiveEnumValuesScenario ||
        scenario == automaticResignDirectiveEnumValuesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasAdvisorySwitchValueRoundTripsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == advisorySwitchValueRoundTripsScenario ||
        scenario == advisorySwitchValueRoundTripsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasDuplicateFederateNameJoinBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == duplicateFederateNameJoinScenario ||
        scenario == duplicateFederateNameJoinContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRepeatedJoinByMemberBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == repeatedJoinByMemberScenario ||
        scenario == repeatedJoinByMemberContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasForeignFederateNameLookupBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == foreignFederateNameLookupScenario ||
        scenario == foreignFederateNameLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasForeignObjectInstanceNameLookupBoundaryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == foreignObjectInstanceNameLookupScenario ||
        scenario == foreignObjectInstanceNameLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasForeignObjectInstanceHandleLookupBoundaryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == foreignObjectInstanceHandleLookupScenario ||
        scenario == foreignObjectInstanceHandleLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasLocalDeleteObjectInstanceNameLookupBoundaryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == localDeleteObjectInstanceNameLookupScenario ||
        scenario == localDeleteObjectInstanceNameLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRemoteDeleteKnownObjectClassLookupBoundaryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == remoteDeleteKnownObjectClassLookupScenario ||
        scenario == remoteDeleteKnownObjectClassLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasRemoteDeleteObjectInstanceNameLookupBoundaryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == remoteDeleteObjectInstanceNameLookupScenario ||
        scenario == remoteDeleteObjectInstanceNameLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignDeleteObjectLookupBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignDeleteObjectLookupScenario ||
        scenario == resignDeleteObjectLookupContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignNoActionOwnedAttributesBoundaryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignNoActionOwnedAttributesScenario ||
        scenario == resignNoActionOwnedAttributesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignDeleteObjectsThenDivestCrossObjectEffectsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignDeleteObjectsThenDivestCrossObjectEffectsScenario ||
        scenario == resignDeleteObjectsThenDivestCrossObjectEffectsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignUnconditionallyDivestAttributesPreservesObjectsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignUnconditionallyDivestAttributesPreservesObjectsScenario ||
        scenario == resignUnconditionallyDivestAttributesPreservesObjectsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignCancelPendingOwnershipAcquisitionsScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignCancelPendingOwnershipAcquisitionsScenario ||
        scenario == resignCancelPendingOwnershipAcquisitionsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasResignCancelThenDeleteThenDivestCrossObjectEffectsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == resignCancelThenDeleteThenDivestCrossObjectEffectsScenario ||
        scenario == resignCancelThenDeleteThenDivestCrossObjectEffectsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasStandardOrderTransportLookupsAfterResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == standardOrderTransportLookupsAfterResignationScenario ||
        scenario == standardOrderTransportLookupsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasObjectAttributeDeclarationsValidInputsAfterResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectAttributeDeclarationsValidInputsAfterResignationScenario ||
        scenario == objectAttributeDeclarationsValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasInteractionDeclarationsValidInputsAfterResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionDeclarationsValidInputsAfterResignationScenario ||
        scenario == interactionDeclarationsValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasDirectedInteractionDeclarationsValidInputsAfterResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == directedInteractionDeclarationsValidInputsAfterResignationScenario ||
        scenario == directedInteractionDeclarationsValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasInteractionTransportationQueryValidInputsAfterResignationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionTransportationQueryValidInputsAfterResignationScenario ||
        scenario == interactionTransportationQueryValidInputsAfterResignationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportReceiveOrderInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportReceiveOrderInteractionScenario ||
        scenario == serviceReportReceiveOrderInteractionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedDirectedInteractionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedDirectedInteractionScenario ||
        scenario == serviceReportTimestampedDirectedInteractionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportObjectAttributeUpdateRateLookupsScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportObjectAttributeUpdateRateLookupsScenario ||
        scenario == serviceReportObjectAttributeUpdateRateLookupsContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportObjectAttributeUpdateRateLookupFailuresScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportObjectAttributeUpdateRateLookupFailuresScenario ||
        scenario == serviceReportObjectAttributeUpdateRateLookupFailuresContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportObjectAttributeDeclarationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportObjectAttributeDeclarationScenario ||
        scenario == serviceReportObjectAttributeDeclarationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDirectedInteractionDeclarationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDirectedInteractionDeclarationScenario ||
        scenario == serviceReportDirectedInteractionDeclarationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDirectedInteractionPublicationScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDirectedInteractionPublicationScenario ||
        scenario == serviceReportDirectedInteractionPublicationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionPublicationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionPublicationScenario ||
        scenario == serviceReportInteractionPublicationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionSubscriptionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionSubscriptionScenario ||
        scenario == serviceReportInteractionSubscriptionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportAttributeUpdateScenario ||
        scenario == serviceReportAttributeUpdateContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportRequestAttributeValueUpdateScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportRequestAttributeValueUpdateScenario ||
        scenario == serviceReportRequestAttributeValueUpdateContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportRegisterObjectInstanceScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportRegisterObjectInstanceScenario ||
        scenario == serviceReportRegisterObjectInstanceContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDeleteObjectInstanceScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDeleteObjectInstanceScenario ||
        scenario == serviceReportDeleteObjectInstanceContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInteractionFailureScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInteractionFailureScenario ||
        scenario == serviceReportInteractionFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportAttributeUpdateFailureScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportAttributeUpdateFailureScenario ||
        scenario == serviceReportAttributeUpdateFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportDeleteObjectInstanceFailureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportDeleteObjectInstanceFailureScenario ||
        scenario == serviceReportDeleteObjectInstanceFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportReserveObjectInstanceNameScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportReserveObjectInstanceNameScenario ||
        scenario == serviceReportReserveObjectInstanceNameContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportReleaseObjectInstanceNameScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportReleaseObjectInstanceNameScenario ||
        scenario == serviceReportReleaseObjectInstanceNameContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportReleaseMultipleObjectInstanceNamesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportReleaseMultipleObjectInstanceNamesScenario ||
        scenario == serviceReportReleaseMultipleObjectInstanceNamesContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportQueryAttributeOwnershipScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportQueryAttributeOwnershipScenario ||
        scenario == serviceReportQueryAttributeOwnershipContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportConfirmDivestitureScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportConfirmDivestitureScenario ||
        scenario == serviceReportConfirmDivestitureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportCancelAttributeOwnershipAcquisitionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportCancelAttributeOwnershipAcquisitionScenario ||
        scenario == serviceReportCancelAttributeOwnershipAcquisitionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportLocalDeleteObjectInstanceScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportLocalDeleteObjectInstanceScenario ||
        scenario == serviceReportLocalDeleteObjectInstanceContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportLocalDeleteObjectInstanceFailureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportLocalDeleteObjectInstanceFailureScenario ||
        scenario == serviceReportLocalDeleteObjectInstanceFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportInterlockScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportInterlockScenario ||
        scenario == serviceReportInterlockContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportSynchronizationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportSynchronizationScenario ||
        scenario == serviceReportSynchronizationContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportFederateSaveBegunScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportFederateSaveBegunScenario ||
        scenario == serviceReportFederateSaveBegunContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedAttributeUpdateScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedAttributeUpdateScenario ||
        scenario == serviceReportTimestampedAttributeUpdateContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedDeleteObjectInstanceScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedDeleteObjectInstanceScenario ||
        scenario == serviceReportTimestampedDeleteObjectInstanceContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedInteractionScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedInteractionScenario ||
        scenario == serviceReportTimestampedInteractionContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedAttributeUpdateFailureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedAttributeUpdateFailureScenario ||
        scenario == serviceReportTimestampedAttributeUpdateFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedInteractionFailureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedInteractionFailureScenario ||
        scenario == serviceReportTimestampedInteractionFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasServiceReportTimestampedDeleteObjectInstanceFailureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == serviceReportTimestampedDeleteObjectInstanceFailureScenario ||
        scenario == serviceReportTimestampedDeleteObjectInstanceFailureContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasSynchronizationPointScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == javaSynchronizationScenario ||
        scenario == synchronizationPointsScenario ||
        scenario == synchronizationPointContractScenario ||
        scenario == synchronizationExplicitSetLateJoinScenario ||
        scenario == synchronizationExplicitSetLateJoinContractScenario) {
      return true;
    }
  }
  return false;
}

bool hasUnconditionalAttributeOwnershipDivestitureScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == unconditionalAttributeOwnershipDivestitureScenario ||
        scenario == unconditionalAttributeOwnershipDivestitureContractId) {
      return true;
    }
  }
  return false;
}

bool hasAutoProvideDisabledDiscoveryOnlyScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == autoProvideDisabledDiscoveryOnlyScenario ||
        scenario == autoProvideDisabledDiscoveryOnlyContractId) {
      return true;
    }
  }
  return false;
}

bool hasAutoProvideDisabledExplicitRequestScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == autoProvideDisabledExplicitRequestScenario ||
        scenario == autoProvideDisabledExplicitRequestContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationAutoProvideMomSwitchScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationAutoProvideMomSwitchScenario ||
        scenario == federationAutoProvideMomSwitchContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateServiceReportingMomSwitchScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateServiceReportingMomSwitchScenario ||
        scenario == federateServiceReportingMomSwitchContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateServiceReportingMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateServiceReportingMomAttributeScenario ||
        scenario == federateServiceReportingMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateObjectClassRelevanceAdvisoryMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateObjectClassRelevanceAdvisoryMomAttributeScenario ||
        scenario == federateObjectClassRelevanceAdvisoryMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateAttributeRelevanceAdvisoryMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateAttributeRelevanceAdvisoryMomAttributeScenario ||
        scenario == federateAttributeRelevanceAdvisoryMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateAttributeScopeAdvisoryMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateAttributeScopeAdvisoryMomAttributeScenario ||
        scenario == federateAttributeScopeAdvisoryMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateInteractionRelevanceAdvisoryMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateInteractionRelevanceAdvisoryMomAttributeScenario ||
        scenario == federateInteractionRelevanceAdvisoryMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateAsynchronousDeliveryMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateAsynchronousDeliveryMomAttributeScenario ||
        scenario == federateAsynchronousDeliveryMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateTimeConstrainedMomAttributeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateTimeConstrainedMomAttributeScenario ||
        scenario == federateTimeConstrainedMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateTimeRegulatingMomAttributeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateTimeRegulatingMomAttributeScenario ||
        scenario == federateTimeRegulatingMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateLookaheadMomPeriodicScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateLookaheadMomPeriodicScenario ||
        scenario == federateLookaheadMomPeriodicContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateLogicalTimeMomPeriodicScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateLogicalTimeMomPeriodicScenario ||
        scenario == federateLogicalTimeMomPeriodicContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateTimeManagerStateMomAttributeScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateTimeManagerStateMomAttributeScenario ||
        scenario == federateTimeManagerStateMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateExceptionReportingMomSwitchScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateExceptionReportingMomSwitchScenario ||
        scenario == federateExceptionReportingMomSwitchContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateExceptionReportingMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateExceptionReportingMomAttributeScenario ||
        scenario == federateExceptionReportingMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateExceptionReportDeliveryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateExceptionReportDeliveryScenario ||
        scenario == federateExceptionReportDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomExceptionReportDeliveryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomExceptionReportDeliveryScenario ||
        scenario == federateMomExceptionReportDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomExceptionMissingParameterScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomExceptionMissingParameterScenario ||
        scenario == federateMomExceptionMissingParameterContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederationMomExceptionMissingFomModuleIndicatorScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationMomExceptionMissingFomModuleIndicatorScenario ||
        scenario == federationMomExceptionMissingFomModuleIndicatorContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomFomModuleContentReportScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomFomModuleContentReportScenario ||
        scenario == federateMomFomModuleContentReportContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectInstanceInformationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectInstanceInformationScenario ||
        scenario == federateMomObjectInstanceInformationContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomPublicationQueryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomPublicationQueryScenario ||
        scenario == federateMomPublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomCompleteObjectClassPublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomCompleteObjectClassPublicationQueryScenario ||
        scenario == federateMomCompleteObjectClassPublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassAttributeUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectClassAttributeUnpublicationQueryScenario ||
        scenario == federateMomObjectClassAttributeUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassFullAttributeUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomObjectClassFullAttributeUnpublicationQueryScenario ||
        scenario ==
            federateMomObjectClassFullAttributeUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassWholeUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectClassWholeUnpublicationQueryScenario ||
        scenario == federateMomObjectClassWholeUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomInteractionClassWholeUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomInteractionClassWholeUnpublicationQueryScenario ||
        scenario == federateMomInteractionClassWholeUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomInteractionClassPartialUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomInteractionClassPartialUnpublicationQueryScenario ||
        scenario ==
            federateMomInteractionClassPartialUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleInteractionClassFullUnpublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleInteractionClassFullUnpublicationQueryScenario ||
        scenario ==
            federateMomMultipleInteractionClassFullUnpublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleDirectedPublicationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomMultipleDirectedPublicationQueryScenario ||
        scenario == federateMomMultipleDirectedPublicationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleDirectedPublicationPartialUnpublishQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleDirectedPublicationPartialUnpublishQueryScenario ||
        scenario ==
            federateMomMultipleDirectedPublicationPartialUnpublishQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleDirectedPublicationFullUnpublishQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleDirectedPublicationFullUnpublishQueryScenario ||
        scenario ==
            federateMomMultipleDirectedPublicationFullUnpublishQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomSubscriptionQueryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomSubscriptionQueryScenario ||
        scenario == federateMomSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomPassiveSubscriptionQueryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomPassiveSubscriptionQueryScenario ||
        scenario == federateMomPassiveSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomPassiveInteractionSubscriptionQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomPassiveInteractionSubscriptionQueryScenario ||
        scenario == federateMomPassiveInteractionSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomPassiveInteractionSubscriptionActivationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomPassiveInteractionSubscriptionActivationQueryScenario ||
        scenario ==
            federateMomPassiveInteractionSubscriptionActivationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleInteractionSubscriptionQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomMultipleInteractionSubscriptionQueryScenario ||
        scenario == federateMomMultipleInteractionSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleInteractionSubscriptionUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleInteractionSubscriptionUnsubscribeQueryScenario ||
        scenario ==
            federateMomMultipleInteractionSubscriptionUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleInteractionSubscriptionFullUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryScenario ||
        scenario ==
            federateMomMultipleInteractionSubscriptionFullUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectAttributePartialUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectAttributePartialUnsubscribeQueryScenario ||
        scenario == federateMomObjectAttributePartialUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectAttributeFullUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectAttributeFullUnsubscribeQueryScenario ||
        scenario == federateMomObjectAttributeFullUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassFullUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomObjectClassFullUnsubscribeQueryScenario ||
        scenario == federateMomObjectClassFullUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassCompleteAttributeSubscriptionQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomObjectClassCompleteAttributeSubscriptionQueryScenario ||
        scenario ==
            federateMomObjectClassCompleteAttributeSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassPassiveCompleteAttributeSubscriptionQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryScenario ||
        scenario ==
            federateMomObjectClassPassiveCompleteAttributeSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomObjectClassCompleteAttributeSubscriptionActivationQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomObjectClassCompleteAttributeSubscriptionActivationQueryScenario ||
        scenario ==
            federateMomObjectClassCompleteAttributeSubscriptionActivationQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomEmptySubscriptionQueryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomEmptySubscriptionQueryScenario ||
        scenario == federateMomEmptySubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomDirectedSubscriptionQueryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomDirectedSubscriptionQueryScenario ||
        scenario == federateMomDirectedSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleDirectedSubscriptionQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomMultipleDirectedSubscriptionQueryScenario ||
        scenario == federateMomMultipleDirectedSubscriptionQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario ==
            federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenario ||
        scenario ==
            federateMomMultipleDirectedSubscriptionFullUnsubscribeQueryContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateMomExceptionReportServicePreconditionScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateMomExceptionReportServicePreconditionScenario ||
        scenario == federateMomExceptionReportServicePreconditionContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateSendServiceReportsToFileMomSwitchScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateSendServiceReportsToFileMomSwitchScenario ||
        scenario == federateSendServiceReportsToFileMomSwitchContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateAutomaticResignActionMomSwitchScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateAutomaticResignActionMomSwitchScenario ||
        scenario == federateAutomaticResignActionMomSwitchContractId) {
      return true;
    }
  }
  return false;
}

bool hasFederateAutomaticResignActionMomAttributeScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federateAutomaticResignActionMomAttributeScenario ||
        scenario == federateAutomaticResignActionMomAttributeContractId) {
      return true;
    }
  }
  return false;
}

bool hasObjectRegistrationServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectRegistrationServiceBoundariesScenario ||
        scenario == objectRegistrationServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasObjectDeletionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == objectDeletionServiceBoundariesScenario ||
        scenario == objectDeletionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeUpdateServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeUpdateServiceBoundariesScenario ||
        scenario == attributeUpdateServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasInteractionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == interactionServiceBoundariesScenario ||
        scenario == interactionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasAttributeValueRequestServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == attributeValueRequestServiceBoundariesScenario ||
        scenario == attributeValueRequestServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasConnectionServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == connectionServiceBoundariesScenario ||
        scenario == connectionServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasOwnershipServiceBoundariesScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == ownershipServiceBoundariesScenario ||
        scenario == ownershipServiceBoundariesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedRegionalInteractionDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedRegionalInteractionDeliveryId ||
        scenario == customTransportationTimestampedRegionalInteractionDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationAttributeDeliveryScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationAttributeDeliveryId ||
        scenario == customTransportationAttributeDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedAttributeDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedAttributeDeliveryId ||
        scenario == customTransportationTimestampedAttributeDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedAttributeAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedAttributeAlternateAdvancesId ||
        scenario == customTransportationTimestampedAttributeAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedInteractionAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedInteractionAlternateAdvancesId ||
        scenario == customTransportationTimestampedInteractionAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDirectedDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-timestamped-directed-delivery" ||
        scenario == "cpp-tck.custom-transportation-timestamped-directed-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationInteractionDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-interaction-delivery" ||
        scenario == "cpp-tck.custom-transportation-interaction-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDeliveryPortableScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == "cpp-tck.custom-transportation-timestamped-delivery" ||
        scenario == "cpp-tck.custom-transportation-timestamped-delivery-contract") {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationDirectedInteractionDeliveryScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationDirectedInteractionDeliveryId ||
        scenario == customTransportationDirectedInteractionDeliveryContractId) {
      return true;
    }
  }
  return false;
}

bool hasCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenario(
    int argc,
    char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == customTransportationTimestampedDirectedInteractionAlternateAdvancesId ||
        scenario == customTransportationTimestampedDirectedInteractionAlternateAdvancesContractId) {
      return true;
    }
  }
  return false;
}

bool hasDelaySubscriptionEvaluationScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (delaySubscriptionEvaluationScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

bool hasFederationSaveRestoreInterlocksScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == federationSaveRestoreInterlocksScenario ||
        scenario == federationSaveRestoreInterlocksContractId) {
      return true;
    }
  }
  return false;
}

bool hasTimedSaveRestoreScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (timedSaveRestoreScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

bool hasRegionalDdmScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    if (regionalDdmScenario(std::string(argv[index + 1])) != nullptr) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideResponseScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideResponseScenario ||
        scenario == regionalAutoProvideResponseContractId) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideTimestampedResponseScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideTimestampedResponseScenario ||
        scenario == regionalAutoProvideTimestampedResponseContractId) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideOverlapScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideOverlapScenario ||
        scenario == regionalAutoProvideOverlapContractId) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideMultiSourceScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideMultiSourceScenario ||
        scenario == regionalAutoProvideMultiSourceContractId) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideMultiProviderScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideMultiProviderScenario ||
        scenario == regionalAutoProvideMultiProviderContractId) {
      return true;
    }
  }
  return false;
}

bool hasRegionalAutoProvideCallbackControlScenario(int argc, char** argv) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string(argv[index]) != "--scenario") {
      continue;
    }
    auto const scenario = std::string(argv[index + 1]);
    if (scenario == regionalAutoProvideCallbackControlScenario ||
        scenario == regionalAutoProvideCallbackControlContractId) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (hasServiceReportConnectFailureScenario(argc, argv)) {
      return runServiceReportConnectFailureScenario(argc, argv);
    }
    if (hasServiceReportCreationFailureScenario(argc, argv)) {
      return runServiceReportCreationFailureScenario(argc, argv);
    }
    if (hasPendingNegotiatedRestorePackageScenario(argc, argv)) {
      return runPendingNegotiatedRestorePackageScenario(argc, argv);
    }
    if (hasAutomaticResignDirectiveScenario(argc, argv)) {
      return runAutomaticResignDirectiveScenarios(argc, argv);
    }
    if (hasPublicHandleDecodingScenario(argc, argv)) {
      return runPublicHandleDecodingScenarios(argc, argv);
    }
    if (hasMomTransportationTypeChangeRequestScenario(argc, argv)) {
      return runMomTransportationTypeChangeRequestScenarios(argc, argv);
    }
    if (hasJoinedFederateMomRegisteredObjectCountScenario(argc, argv)) {
      return runJoinedFederateMomRegisteredObjectCountScenarios(argc, argv);
    }
    if (hasJoinedFederateMomRoLengthPeriodicScenario(argc, argv)) {
      return runJoinedFederateMomRoLengthPeriodicScenarios(argc, argv);
    }
    if (hasJoinedFederateMomTsoLengthPeriodicScenario(argc, argv)) {
      return runJoinedFederateMomTsoLengthPeriodicScenarios(argc, argv);
    }
    if (hasJoinedFederateMomInteractionsReceivedAttributeScenario(argc, argv)) {
      return runJoinedFederateMomInteractionsReceivedAttributeScenarios(argc, argv);
    }
    if (hasJoinedFederateMomInteractionsSentAttributeScenario(argc, argv)) {
      return runJoinedFederateMomInteractionsSentAttributeScenarios(argc, argv);
    }
    if (hasJoinedFederateMomDirectedInteractionsReceivedAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomDirectedInteractionsReceivedAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomDirectedInteractionsSentAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomDirectedInteractionsSentAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomUpdatesSentAttributeScenario(argc, argv)) {
      return runJoinedFederateMomUpdatesSentAttributeScenarios(argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesUpdatedAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomObjectInstancesUpdatedAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomObjectInstancesReflectedAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomObjectInstancesReflectedAttributeScenarios(
          argc,
          argv);
    }
     if (hasJoinedFederateMomReflectionsReceivedAttributeScenario(argc, argv)) {
       return runJoinedFederateMomReflectionsReceivedAttributeScenarios(argc, argv);
     }
    if (hasJoinedFederateMomObjectInstancesDiscoveredAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomObjectInstancesDiscoveredAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomObjectInstancesRemovedAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomObjectInstancesRemovedAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomObjectInstancesDeletedAttributeScenario(
            argc,
            argv)) {
      return runJoinedFederateMomObjectInstancesDeletedAttributeScenarios(
          argc,
          argv);
    }
    if (hasJoinedFederateMomObjectLifecycleScenario(argc, argv)) {
      return runJoinedFederateMomObjectLifecycleScenarios(argc, argv);
    }
    if (hasJoinedFederateMomDeletableObjectReportScenario(argc, argv)) {
      return runJoinedFederateMomDeletableObjectReportScenarios(argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesUpdatedReportScenario(argc, argv)) {
      return runJoinedFederateMomObjectInstancesUpdatedReportScenarios(argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesUpdatedTimestampedReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesUpdatedTimestampedReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesUpdatedMultiClassReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesUpdatedMultiClassReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesUpdatedNullReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesUpdatedNullReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesReflectedNullReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesReflectedNullReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesReflectedReportScenario(argc, argv)) {
      return runJoinedFederateMomObjectInstancesReflectedReportScenarios(argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesReflectedTimestampedReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesReflectedTimestampedReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomObjectInstancesReflectedMultiClassReportScenario(
            argc, argv)) {
      return runJoinedFederateMomObjectInstancesReflectedMultiClassReportScenarios(
          argc, argv);
    }
    if (hasJoinedFederateMomDeletableObjectCountScenario(argc, argv)) {
      return runJoinedFederateMomDeletableObjectCountScenarios(argc, argv);
    }
    if (hasReceiveOrderAttributeUpdateScenario(argc, argv)) {
      return runReceiveOrderAttributeUpdateScenarios(argc, argv);
    }
    if (hasAttributeUpdateOrderClassificationScenario(argc, argv)) {
      return runAttributeUpdateOrderClassificationScenarios(argc, argv);
    }
    if (hasReceiveOrderInteractionScenario(argc, argv)) {
      return runReceiveOrderInteractionScenarios(argc, argv);
    }
    if (hasInteractionOrderClassificationScenario(argc, argv)) {
      return runInteractionOrderClassificationScenarios(argc, argv);
    }
    if (hasInteractionPublicationSendTransitionScenario(argc, argv)) {
      return runInteractionPublicationSendTransitionScenarios(argc, argv);
    }
    if (hasInteractionActiveAndPassiveSubscriptionModesScenario(argc, argv)) {
      return runInteractionActiveAndPassiveSubscriptionModesScenarios(argc, argv);
    }
    if (hasReceiveOrderObjectRemovalScenario(argc, argv)) {
      return runReceiveOrderObjectRemovalScenarios(argc, argv);
    }
    if (hasObjectRemovalOrderClassificationScenario(argc, argv)) {
      return runObjectRemovalOrderClassificationScenarios(argc, argv);
    }
    if (hasFederationRestoreAbortScenario(argc, argv)) {
      return runFederationRestoreAbortScenarios(argc, argv);
    }
    if (hasFederationRestoreOwnershipAssumptionScenario(argc, argv)) {
      return runFederationRestoreOwnershipAssumptionScenarios(argc, argv);
    }
    if (hasOwnershipAcquisitionIfAvailableScenario(argc, argv)) {
      return runOwnershipAcquisitionIfAvailableScenarios(argc, argv);
    }
    if (hasAttributeOwnershipAcquisitionCancellationScenario(argc, argv)) {
      return runAttributeOwnershipAcquisitionCancellationScenarios(argc, argv);
    }
    if (hasNegotiatedDivestitureConfirmationFlowScenario(argc, argv)) {
      return runNegotiatedDivestitureConfirmationFlowScenarios(argc, argv);
    }
    if (hasAttributeOwnershipReleaseDeniedScenario(argc, argv)) {
      return runAttributeOwnershipReleaseDeniedScenarios(argc, argv);
    }
    if (hasAttributeOwnershipDivestitureIfWantedScenario(argc, argv)) {
      return runAttributeOwnershipDivestitureIfWantedScenarios(argc, argv);
    }
    if (hasAttributeOwnershipQueryOwnerReportScenario(argc, argv)) {
      return runAttributeOwnershipQueryOwnerReportScenarios(argc, argv);
    }
    if (hasAttributeOwnershipQueryUnownedResultScenario(argc, argv)) {
      return runAttributeOwnershipQueryUnownedResultScenarios(argc, argv);
    }
    if (hasAttributeOwnershipQueryRtiOwnedResultScenario(argc, argv)) {
      return runAttributeOwnershipQueryRtiOwnedResultScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionRetractionScenario(argc, argv)) {
      return runTimestampedDirectedInteractionRetractionScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionRetractionFanoutScenario(argc, argv)) {
      return runTimestampedDirectedInteractionRetractionFanoutScenarios(argc, argv);
    }
    if (hasTimestampedInteractionsScenario(argc, argv)) {
      return runTimestampedInteractionsScenarios(argc, argv);
    }
    if (hasUnregulatedSenderTimestampedInteractionScenario(argc, argv)) {
      return runUnregulatedSenderTimestampedInteractionScenarios(argc, argv);
    }
    if (hasTimestampedInteractionPreferredReceiveConversionScenario(argc, argv)) {
      return runTimestampedInteractionPreferredReceiveConversionScenarios(
          argc, argv);
    }
    if (hasTimestampedInteractionSenderRegulationTransitionScenario(argc, argv)) {
      return runTimestampedInteractionSenderRegulationTransitionScenarios(
          argc, argv);
    }
    if (hasInteractionFddPreferredOrderDefaultScenario(argc, argv)) {
      return runInteractionFddPreferredOrderDefaultScenarios(argc, argv);
    }
    if (hasAttributeFddPreferredOrderDefaultScenario(argc, argv)) {
      return runAttributeFddPreferredOrderDefaultScenarios(argc, argv);
    }
    if (hasTimestampedDeletePrivilegePreferredOrderScenario(argc, argv)) {
      return runTimestampedDeletePrivilegePreferredOrderScenarios(argc, argv);
    }
    if (hasTimestampedDeletePrivilegeTimestampOrderScenario(argc, argv)) {
      return runTimestampedDeletePrivilegeTimestampOrderScenarios(argc, argv);
    }
    if (hasJavaTimestampedAttributeUpdateScenario(argc, argv)) {
      return runJavaTimestampedAttributeUpdateScenario(argc, argv);
    }
    if (hasJavaTimestampedObjectRemovalScenario(argc, argv)) {
      return runJavaTimestampedObjectRemovalScenario(argc, argv);
    }
    if (hasTimestampedAttributeUpdateScenario(argc, argv)) {
      return runTimestampedAttributeUpdateScenarios(argc, argv);
    }
    if (hasTimestampedAttributeTarPostGrantCutoffScenario(argc, argv)) {
      return runTimestampedAttributeTarPostGrantCutoffScenarios(argc, argv);
    }
    if (hasTimestampedAttributeTimeAdvancingCutoffScenario(argc, argv)) {
      return runTimestampedAttributeTimeAdvancingCutoffScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateReenableScenario(argc, argv)) {
      return runTimestampedAttributeUpdateReenableScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateRegulationReenableScenario(argc, argv)) {
      return runTimestampedAttributeUpdateRegulationReenableScenarios(
          argc, argv);
    }
    if (hasTimestampedAttributeSourceResignationScenario(argc, argv)) {
      return runTimestampedAttributeSourceResignationScenarios(argc, argv);
    }
    if (hasTimestampedAttributeSourceResignationFanoutScenario(argc, argv)) {
      return runTimestampedAttributeSourceResignationFanoutScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateOwnershipTransferScenario(argc, argv)) {
      return runTimestampedAttributeUpdateOwnershipTransferScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateQueuedPasselRetractionScenario(argc, argv)) {
      return runTimestampedAttributeUpdateQueuedPasselRetractionScenarios(argc, argv);
    }
    if (hasMixedOrderAttributePasselRetractionScenario(argc, argv)) {
      return runMixedOrderAttributePasselRetractionScenarios(argc, argv);
    }
    if (hasTimestampedAttributeImmediateRecipientMultiTransportRetractionScenario(
            argc,
            argv)) {
      return runTimestampedAttributeImmediateRecipientMultiTransportRetractionScenarios(
          argc,
          argv);
    }
    if (hasTimestampedAttributeSenderRegulationTransitionScenario(argc, argv)) {
      return runTimestampedAttributeSenderRegulationTransitionScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateNoFanoutScenario(argc, argv)) {
      return runTimestampedAttributeUpdateNoFanoutScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateFlushQueueFutureInputScenario(argc, argv)) {
      return runTimestampedAttributeUpdateFlushQueueFutureInputScenarios(argc, argv);
    }
    if (hasTimestampedInteractionFlushQueueFutureInputScenario(argc, argv)) {
      return runTimestampedInteractionFlushQueueFutureInputScenarios(argc, argv);
    }
    if (hasFqrPostgrantLowerTsoRetractionScenario(argc, argv)) {
      return runFqrPostgrantLowerTsoRetractionScenarios(argc, argv);
    }
    if (hasFqgPostgrantDeliveryCutoffScenario(argc, argv)) {
      return runFqgPostgrantDeliveryCutoffScenarios(argc, argv);
    }
    if (hasTarQueuedTsoBeforeGrantScenario(argc, argv)) {
      return runTarQueuedTsoBeforeGrantScenarios(argc, argv);
    }
    if (hasTimestampedAttributeOrderCohortScenario(argc, argv)) {
      return runTimestampedAttributeOrderCohortScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionNoFanoutScenario(argc, argv)) {
      return runTimestampedObjectDeletionNoFanoutScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionTombstoneScenario(argc, argv)) {
      return runTimestampedObjectDeletionTombstoneScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionRetractionJoinedOwnersScenario(argc, argv)) {
      return runTimestampedObjectDeletionRetractionJoinedOwnersScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionSourceResignationFanoutScenario(argc, argv)) {
      return runTimestampedObjectDeletionSourceResignationFanoutScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionScenario(argc, argv)) {
      return runTimestampedObjectDeletionScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionRegulationReenableScenario(argc, argv)) {
      return runTimestampedObjectDeletionRegulationReenableScenarios(
          argc, argv);
    }
    if (hasAlternateTimeAdvancesScenario(argc, argv)) {
      return runAlternateTimeAdvancesScenarios(argc, argv);
    }
    if (hasPendingAlternateAdvanceStateScenario(argc, argv)) {
      return runPendingAlternateAdvanceStateScenarios(argc, argv);
    }
    if (hasNextMessageRequestScenario(argc, argv)) {
      return runNextMessageRequestScenarios(argc, argv);
    }
    if (hasNmrNoPregrantTsoFloorScenario(argc, argv)) {
      return runNmrNoPregrantTsoFloorScenarios(argc, argv);
    }
    if (hasNmrReceivedTsoFloorScenario(argc, argv)) {
      return runNmrReceivedTsoFloorScenarios(argc, argv);
    }
    if (hasNmrReceivedTsoAttributeUpdateFloorScenario(argc, argv)) {
      return runNmrReceivedTsoAttributeUpdateFloorScenarios(argc, argv);
    }
    if (hasNmrReceivedTsoDirectedInteractionFloorScenario(argc, argv)) {
      return runNmrReceivedTsoDirectedInteractionFloorScenarios(argc, argv);
    }
    if (hasNmrEmptyMessageSetGrantTimeScenario(argc, argv)) {
      return runNmrEmptyMessageSetGrantTimeScenarios(argc, argv);
    }
    if (hasNmrRequestBelowGaltImmediateGrantScenario(argc, argv)) {
      return runNmrRequestBelowGaltImmediateGrantScenarios(argc, argv);
    }
    if (hasNmrLitsBelowGaltImmediateGrantScenario(argc, argv)) {
      return runNmrLitsBelowGaltImmediateGrantScenarios(argc, argv);
    }
    if (hasNmraNoPregrantTsoFloorScenario(argc, argv)) {
      return runNmraNoPregrantTsoFloorScenarios(argc, argv);
    }
    if (hasSendMessageTimeStatePreservationGrantedScenario(argc, argv)) {
      return runSendMessageTimeStatePreservationGrantedScenarios(argc, argv);
    }
    if (hasAvailableTimeAdvancesInclusiveGaltScenario(argc, argv)) {
      return runAvailableTimeAdvancesInclusiveGaltScenarios(argc, argv);
    }
    if (hasTimeConstrainedEnableAtGaltEqualityScenario(argc, argv)) {
      return runTimeConstrainedEnableAtGaltEqualityScenarios(argc, argv);
    }
    if (hasTimeRegulationImplicitAdvanceScenario(argc, argv)) {
      return runTimeRegulationImplicitAdvanceScenarios(argc, argv);
    }
    if (hasTimestampedInteractionTarEqualBoundaryScenario(argc, argv)) {
      return runTimestampedInteractionTarEqualBoundaryScenarios(argc, argv);
    }
    if (hasTimestampedInteractionTarPostGrantCutoffScenario(argc, argv)) {
      return runTimestampedInteractionTarPostGrantCutoffScenarios(argc, argv);
    }
    if (hasTimestampedInteractionPublicationSendTransitionScenario(argc, argv)) {
      return runTimestampedInteractionPublicationSendTransitionScenarios(
          argc, argv);
    }
    if (hasEmptyAttributeSubscriptionScenario(argc, argv)) {
      return runEmptyAttributeSubscriptionScenario(argc, argv);
    }
    if (hasNamedUpdateRateSubscriptionScenario(argc, argv)) {
      return runNamedUpdateRateSubscriptionScenario(argc, argv);
    }
    if (hasDefaultRateNoReductionScenario(argc, argv)) {
      return runDefaultRateNoReductionScenario(argc, argv);
    }
    if (hasAccumulatingAttributeSubscriptionsScenario(argc, argv)) {
      return runAccumulatingAttributeSubscriptionsScenario(argc, argv);
    }
    if (hasAttributeSubscriptionModeTransitionsScenario(argc, argv)) {
      return runAttributeSubscriptionModeTransitionsScenario(argc, argv);
    }
    if (hasPassiveSubscriptionNoStartRegistrationScenario(argc, argv)) {
      return runPassiveSubscriptionNoStartRegistrationScenario(argc, argv);
    }
    if (hasSelectiveAttributeUnsubscriptionScenario(argc, argv)) {
      return runSelectiveAttributeUnsubscriptionScenario(argc, argv);
    }
    if (hasWholeClassAttributeUnsubscriptionScenario(argc, argv)) {
      return runWholeClassAttributeUnsubscriptionScenario(argc, argv);
    }
    if (hasEmptyAttributeUnsubscriptionScenario(argc, argv)) {
      return runEmptyAttributeUnsubscriptionScenario(argc, argv);
    }
    if (hasTimeCoordinatedInteractionScenario(argc, argv)) {
      return runTimeCoordinatedInteractionScenario(argc, argv);
    }
    if (hasTimestampedInteractionMixedAdvancesScenario(argc, argv)) {
      if (std::find_if(
              argv + 1,
              argv + argc,
              [](char const* argument) {
                return std::string(argument) ==
                    "cpp-tck.timestamped-interaction-tara-equal-boundary" ||
                    std::string(argument) ==
                    "cpp-tck.timestamped-interaction-tara-equal-boundary-contract";
              }) != argv + argc) {
        return runTimestampedInteractionTaraEqualBoundaryScenarios(argc, argv);
      }
      return runTimestampedInteractionMixedAdvancesScenarios(argc, argv);
    }
    if (hasTimestampedInteractionSourceResignationScenario(argc, argv)) {
      return runTimestampedInteractionSourceResignationScenarios(argc, argv);
    }
    if (hasTimestampedInteractionSourceResignationFanoutScenario(argc, argv)) {
      return runTimestampedInteractionSourceResignationFanoutScenarios(argc, argv);
    }
    if (hasTimestampedInteractionTsoDesignatorTerminalizationScenario(argc, argv)) {
      return runTimestampedInteractionTsoDesignatorTerminalizationScenarios(argc, argv);
    }
    if (hasTimestampedInteractionCrossProducerOrderScenario(argc, argv)) {
      return runTimestampedInteractionCrossProducerOrderScenarios(argc, argv);
    }
    if (hasTimestampedInteractionNoFanoutScenario(argc, argv)) {
      return runTimestampedInteractionNoFanoutScenarios(argc, argv);
    }
    if (hasTimestampedInteractionRetractionFanoutScenario(argc, argv)) {
      return runTimestampedInteractionRetractionFanoutScenarios(argc, argv);
    }
    if (hasTimestampedInteractionReenableScenario(argc, argv)) {
      return runTimestampedInteractionReenableScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionReenableScenario(argc, argv)) {
      return runTimestampedDirectedInteractionReenableScenarios(argc, argv);
    }
    if (hasTimestampedInteractionRegulationReenableScenario(argc, argv)) {
      return runTimestampedInteractionRegulationReenableScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionRegulationReenableScenario(argc, argv)) {
      return runTimestampedDirectedInteractionRegulationReenableScenarios(
          argc, argv);
    }
    if (hasTimestampedDirectedAlternateAdvancesScenario(argc, argv)) {
      return runTimestampedDirectedAlternateAdvancesScenarios(argc, argv);
    }
    if (hasTimestampedAttributeUpdateAlternateAdvancesScenario(argc, argv)) {
      return runTimestampedAttributeUpdateAlternateAdvancesScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionMixedAdvancesScenario(argc, argv)) {
      return runTimestampedObjectDeletionMixedAdvancesScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionFutureNmrArrivalScenario(argc, argv)) {
      return runTimestampedObjectDeletionFutureNmrArrivalScenarios(argc, argv);
    }
    if (hasTimestampedObjectDeletionFutureNmraArrivalScenario(argc, argv)) {
      return runTimestampedObjectDeletionFutureNmraArrivalScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionTarNmrScenario(argc, argv)) {
      return runTimestampedDirectedInteractionTarNmrScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionSourceResignationScenario(argc, argv)) {
      return runTimestampedDirectedInteractionSourceResignationScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionSourceResignationFanoutScenario(argc, argv)) {
      return runTimestampedDirectedInteractionSourceResignationFanoutScenarios(argc, argv);
    }
    if (hasTimestampedDirectedInteractionImmediateSourceResignationScenario(argc, argv)) {
      return runTimestampedDirectedInteractionImmediateSourceResignationScenarios(argc, argv);
    }
    if (hasModifyLookaheadServiceReportScenario(argc, argv)) {
      return runModifyLookaheadServiceReportScenarios(argc, argv);
    }
    if (hasModifyLookaheadScenario(argc, argv)) {
      return runModifyLookaheadScenarios(argc, argv);
    }
    if (hasTransportationTypeChangeScenario(argc, argv)) {
      return runTransportationTypeChangeScenarios(argc, argv);
    }
    if (hasOrderTypeChangeScenario(argc, argv)) {
      return runOrderTypeChangeScenarios(argc, argv);
    }
    if (hasTransportOrderScenario(argc, argv)) {
      return runTransportOrderScenarios(argc, argv);
    }
    if (hasStandardOrderAndTransportationLookupScenario(argc, argv)) {
      return runStandardOrderAndTransportationLookupScenarios(argc, argv);
    }
    if (hasFactoryDiscoveryScenario(argc, argv)) {
      return runFactoryDiscoveryScenarios(argc, argv);
    }
    if (hasLogicalTimeFactoryScenario(argc, argv)) {
      return runLogicalTimeFactoryScenarios(argc, argv);
    }
    if (hasVariableLengthDataScenario(argc, argv)) {
      return runVariableLengthDataScenarios(argc, argv);
    }
    if (hasLogicalTimeFactoryFactoryScenario(argc, argv)) {
      return runLogicalTimeFactoryFactoryScenarios(argc, argv);
    }
    if (hasLogicalTimeDataElementsScenario(argc, argv)) {
      return runLogicalTimeDataElementsScenarios(argc, argv);
    }
    if (hasExceptionHierarchyScenario(argc, argv)) {
      return runExceptionHierarchyScenarios(argc, argv);
    }
    if (hasEnumScenario(argc, argv)) {
      return runEnumScenarios(argc, argv);
    }
    if (hasHandleAndCollectionScenario(argc, argv)) {
      return runHandleAndCollectionScenarios(argc, argv);
    }
    if (hasDataElementScenario(argc, argv)) {
      return runDataElementScenarios(argc, argv);
    }
    if (hasBasicDataElementsScenario(argc, argv)) {
      return runBasicDataElementsScenarios(argc, argv);
    }
    if (hasCompositeDataElementsScenario(argc, argv)) {
      return runCompositeDataElementsScenarios(argc, argv);
    }
    if (hasConfigurationAndAuthorizationScenario(argc, argv)) {
      return runConfigurationAndAuthorizationScenarios(argc, argv);
    }
    if (hasRuntimeIdentityScenario(argc, argv)) {
      return runRuntimeIdentityScenarios(argc, argv);
    }
    if (hasAuthorizerFactoryFactoryScenario(argc, argv)) {
      return runAuthorizerFactoryFactoryScenarios(argc, argv);
    }
    if (hasNullFederateAmbassadorScenario(argc, argv)) {
      return runNullFederateAmbassadorScenarios(argc, argv);
    }
    if (hasConnectionCallbackFoundationScenario(argc, argv)) {
      return runConnectionCallbackFoundationScenarios(argc, argv);
    }
    if (hasCallbackReentrancyScenario(argc, argv)) {
      return runCallbackReentrancyScenarios(argc, argv);
    }
    if (hasMalformedInputsScenario(argc, argv)) {
      return runMalformedInputsScenarios(argc, argv);
    }
    if (hasFederationLifecycleScenario(argc, argv)) {
      return runFederationLifecycleScenarios(argc, argv);
    }
    if (hasFederationManagementRequiresConnectionScenario(argc, argv)) {
      return runFederationManagementRequiresConnectionScenarios(argc, argv);
    }
    if (hasUnnamedJoinOverloadScenario(argc, argv)) {
      return runUnnamedJoinOverloadScenarios(argc, argv);
    }
    if (hasFederationListServicesScenario(argc, argv)) {
      return runFederationListServicesScenarios(argc, argv);
    }
    if (hasFederateLookupLifecycleScenario(argc, argv)) {
      return runFederateLookupLifecycleScenarios(argc, argv);
    }
    if (hasExplicitMimCreationScenario(argc, argv)) {
      return runExplicitMimCreationScenarios(argc, argv);
    }
    if (hasFederationMomCurrentFddScenario(argc, argv)) {
      return runFederationMomCurrentFddScenarios(argc, argv);
    }
    if (hasFederationMomStaticIdentityScenario(argc, argv)) {
      return runFederationMomStaticIdentityScenarios(argc, argv);
    }
    if (hasFederationMomStaticSwitchesScenario(argc, argv)) {
      return runFederationMomStaticSwitchesScenarios(argc, argv);
    }
    if (hasFederationMomFederatesInFederationScenario(argc, argv)) {
      return runFederationMomFederatesInFederationScenarios(argc, argv);
    }
    if (hasFederationMomFomModuleDesignatorListScenario(argc, argv)) {
      return runFederationMomFomModuleDesignatorListScenarios(argc, argv);
    }
    if (hasFederationMomContentReportsScenario(argc, argv)) {
      return runFederationMomContentReportsScenarios(argc, argv);
    }
    if (hasServiceReportOrderTransportationLookupsScenario(argc, argv)) {
      return runServiceReportOrderTransportationLookupsScenarios(argc, argv);
    }
    if (hasServiceReportDimensionUpperBoundScenario(argc, argv)) {
      return runServiceReportDimensionUpperBoundScenarios(argc, argv);
    }
    if (hasServiceReportDimensionHandleSetScenario(argc, argv)) {
      return runServiceReportDimensionHandleSetScenarios(argc, argv);
    }
    if (hasServiceReportRangeBoundsScenario(argc, argv)) {
      return runServiceReportRangeBoundsScenarios(argc, argv);
    }
    if (hasServiceReportSetRangeBoundsScenario(argc, argv)) {
      return runServiceReportSetRangeBoundsScenarios(argc, argv);
    }
    if (hasServiceReportCommitRegionModificationsScenario(argc, argv)) {
      return runServiceReportCommitRegionModificationsScenarios(argc, argv);
    }
    if (hasServiceReportDeleteRegionScenario(argc, argv)) {
      return runServiceReportDeleteRegionScenarios(argc, argv);
    }
    if (hasServiceReportCreateRegionScenario(argc, argv)) {
      return runServiceReportCreateRegionScenarios(argc, argv);
    }
    if (hasRegionRangeValidationScenario(argc, argv)) {
      return runRegionRangeValidationScenarios(argc, argv);
    }
    if (hasRegionRangeDimensionValidationScenario(argc, argv)) {
      return runRegionRangeDimensionValidationScenarios(argc, argv);
    }
    if (hasServiceReportDimensionNameScenario(argc, argv)) {
      return runServiceReportDimensionNameScenarios(argc, argv);
    }
    if (hasServiceReportDimensionHandleScenario(argc, argv)) {
      return runServiceReportDimensionHandleScenarios(argc, argv);
    }
    if (hasServiceReportInteractionDimensionsScenario(argc, argv)) {
      return runServiceReportInteractionDimensionsScenarios(argc, argv);
    }
    if (hasServiceReportObjectDimensionsScenario(argc, argv)) {
      return runServiceReportObjectDimensionsScenarios(argc, argv);
    }
    if (hasServiceReportOrderTransportationLookupFailuresScenario(argc, argv)) {
      return runServiceReportOrderTransportationLookupFailuresScenarios(argc, argv);
    }
    if (hasServiceReportFederateObjectClassLookupsScenario(argc, argv)) {
      return runServiceReportFederateObjectClassLookupsScenarios(argc, argv);
    }
    if (hasServiceReportFederateObjectClassLookupFailuresScenario(argc, argv)) {
      return runServiceReportFederateObjectClassLookupFailuresScenarios(argc, argv);
    }
    if (hasServiceReportInteractionParameterLookupsScenario(argc, argv)) {
      return runServiceReportInteractionParameterLookupsScenarios(argc, argv);
    }
    if (hasServiceReportInteractionParameterLookupFailuresScenario(argc, argv)) {
      return runServiceReportInteractionParameterLookupFailuresScenarios(argc, argv);
    }
    if (hasServiceReportInteractionScenario(argc, argv)) {
      return runServiceReportInteractionScenarios(argc, argv);
    }
    if (hasServiceReportResignFederationExecutionScenario(argc, argv)) {
      return runServiceReportResignFederationExecutionScenarios(argc, argv);
    }
    if (hasFederateLostMomReportScenario(argc, argv)) {
      return runFederateLostMomReportScenarios(argc, argv);
    }
    if (hasHandleNormalizationMembershipBoundariesScenario(argc, argv)) {
      return runHandleNormalizationMembershipBoundariesScenarios(argc, argv);
    }
    if (hasDimensionLookupValidHandlesAfterResignationScenario(argc, argv)) {
      return runDimensionLookupValidHandlesAfterResignationScenarios(argc, argv);
    }
    if (hasUpdateRateLookupValidInputsAfterResignationScenario(argc, argv)) {
      return runUpdateRateLookupValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasSupportLookupValidInputsAfterResignationScenario(argc, argv)) {
      return runSupportLookupValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasSupportSwitchGetterLifecycleBoundariesScenario(argc, argv)) {
      return runSupportSwitchGetterLifecycleBoundariesScenarios(argc, argv);
    }
    if (hasSupportSwitchSetterLifecycleBoundariesScenario(argc, argv)) {
      return runSupportSwitchSetterLifecycleBoundariesScenarios(argc, argv);
    }
    if (hasAutomaticResignDirectiveLifecycleBoundariesScenario(argc, argv)) {
      return runAutomaticResignDirectiveLifecycleBoundariesScenarios(argc, argv);
    }
    if (hasAutomaticResignDirectiveEnumValuesScenario(argc, argv)) {
      return runAutomaticResignDirectiveEnumValuesScenarios(argc, argv);
    }
    if (hasAdvisorySwitchValueRoundTripsScenario(argc, argv)) {
      return runAdvisorySwitchValueRoundTripsScenarios(argc, argv);
    }
    if (hasDuplicateFederateNameJoinBoundaryScenario(argc, argv)) {
      return runDuplicateFederateNameJoinBoundaryScenarios(argc, argv);
    }
    if (hasRepeatedJoinByMemberBoundaryScenario(argc, argv)) {
      return runRepeatedJoinByMemberBoundaryScenarios(argc, argv);
    }
    if (hasForeignFederateNameLookupBoundaryScenario(argc, argv)) {
      return runForeignFederateNameLookupBoundaryScenarios(argc, argv);
    }
    if (hasForeignObjectInstanceNameLookupBoundaryScenario(argc, argv)) {
      return runForeignObjectInstanceNameLookupBoundaryScenarios(argc, argv);
    }
    if (hasForeignObjectInstanceHandleLookupBoundaryScenario(argc, argv)) {
      return runForeignObjectInstanceHandleLookupBoundaryScenarios(argc, argv);
    }
    if (hasLocalDeleteObjectInstanceNameLookupBoundaryScenario(argc, argv)) {
      return runLocalDeleteObjectInstanceNameLookupBoundaryScenarios(argc, argv);
    }
    if (hasRemoteDeleteKnownObjectClassLookupBoundaryScenario(argc, argv)) {
      return runRemoteDeleteKnownObjectClassLookupBoundaryScenarios(argc, argv);
    }
    if (hasRemoteDeleteObjectInstanceNameLookupBoundaryScenario(argc, argv)) {
      return runRemoteDeleteObjectInstanceNameLookupBoundaryScenarios(argc, argv);
    }
    if (hasResignDeleteObjectLookupBoundaryScenario(argc, argv)) {
      return runResignDeleteObjectLookupBoundaryScenarios(argc, argv);
    }
    if (hasResignNoActionOwnedAttributesBoundaryScenario(argc, argv)) {
      return runResignNoActionOwnedAttributesBoundaryScenarios(argc, argv);
    }
    if (hasResignDeleteObjectsThenDivestCrossObjectEffectsScenario(argc, argv)) {
      return runResignDeleteObjectsThenDivestCrossObjectEffectsScenarios(argc, argv);
    }
    if (hasResignUnconditionallyDivestAttributesPreservesObjectsScenario(
            argc,
            argv)) {
      return runResignUnconditionallyDivestAttributesPreservesObjectsScenarios(
          argc,
          argv);
    }
    if (hasResignCancelPendingOwnershipAcquisitionsScenario(argc, argv)) {
      return runResignCancelPendingOwnershipAcquisitionsScenarios(argc, argv);
    }
    if (hasResignCancelThenDeleteThenDivestCrossObjectEffectsScenario(argc, argv)) {
      return runResignCancelThenDeleteThenDivestCrossObjectEffectsScenarios(
          argc,
          argv);
    }
    if (hasStandardOrderTransportLookupsAfterResignationScenario(argc, argv)) {
      return runStandardOrderTransportLookupsAfterResignationScenarios(argc, argv);
    }
    if (hasObjectAttributeDeclarationsValidInputsAfterResignationScenario(argc, argv)) {
      return runObjectAttributeDeclarationsValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasInteractionDeclarationsValidInputsAfterResignationScenario(argc, argv)) {
      return runInteractionDeclarationsValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasDirectedInteractionDeclarationsValidInputsAfterResignationScenario(argc, argv)) {
      return runDirectedInteractionDeclarationsValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasInteractionTransportationQueryValidInputsAfterResignationScenario(argc, argv)) {
      return runInteractionTransportationQueryValidInputsAfterResignationScenarios(argc, argv);
    }
    if (hasServiceReportReceiveOrderInteractionScenario(argc, argv)) {
      return runServiceReportReceiveOrderInteractionScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedDirectedInteractionScenario(argc, argv)) {
      return runServiceReportTimestampedDirectedInteractionScenarios(argc, argv);
    }
    if (hasServiceReportObjectAttributeUpdateRateLookupsScenario(argc, argv)) {
      return runServiceReportObjectAttributeUpdateRateLookupsScenarios(argc, argv);
    }
    if (hasServiceReportObjectAttributeUpdateRateLookupFailuresScenario(argc, argv)) {
      return runServiceReportObjectAttributeUpdateRateLookupFailuresScenarios(argc, argv);
    }
    if (hasServiceReportObjectAttributeDeclarationScenario(argc, argv)) {
      return runServiceReportObjectAttributeDeclarationScenarios(argc, argv);
    }
    if (hasServiceReportDirectedInteractionDeclarationScenario(argc, argv)) {
      return runServiceReportDirectedInteractionDeclarationScenarios(argc, argv);
    }
    if (hasServiceReportDirectedInteractionPublicationScenario(argc, argv)) {
      return runServiceReportDirectedInteractionPublicationScenarios(argc, argv);
    }
    if (hasServiceReportInteractionPublicationScenario(argc, argv)) {
      return runServiceReportInteractionPublicationScenarios(argc, argv);
    }
    if (hasServiceReportInteractionSubscriptionScenario(argc, argv)) {
      return runServiceReportInteractionSubscriptionScenarios(argc, argv);
    }
    if (hasServiceReportAttributeUpdateScenario(argc, argv)) {
      return runServiceReportAttributeUpdateScenarios(argc, argv);
    }
    if (hasServiceReportRequestAttributeValueUpdateScenario(argc, argv)) {
      return runServiceReportRequestAttributeValueUpdateScenarios(argc, argv);
    }
    if (hasServiceReportRegisterObjectInstanceScenario(argc, argv)) {
      return runServiceReportRegisterObjectInstanceScenarios(argc, argv);
    }
    if (hasServiceReportDeleteObjectInstanceScenario(argc, argv)) {
      return runServiceReportDeleteObjectInstanceScenarios(argc, argv);
    }
    if (hasServiceReportInteractionFailureScenario(argc, argv)) {
      return runServiceReportInteractionFailureScenarios(argc, argv);
    }
    if (hasServiceReportAttributeUpdateFailureScenario(argc, argv)) {
      return runServiceReportAttributeUpdateFailureScenarios(argc, argv);
    }
    if (hasServiceReportDeleteObjectInstanceFailureScenario(argc, argv)) {
      return runServiceReportDeleteObjectInstanceFailureScenarios(argc, argv);
    }
    if (hasServiceReportReserveObjectInstanceNameScenario(argc, argv)) {
      return runServiceReportReserveObjectInstanceNameScenarios(argc, argv);
    }
    if (hasServiceReportReleaseObjectInstanceNameScenario(argc, argv)) {
      return runServiceReportReleaseObjectInstanceNameScenarios(argc, argv);
    }
    if (hasServiceReportReleaseMultipleObjectInstanceNamesScenario(argc, argv)) {
      return runServiceReportReleaseMultipleObjectInstanceNamesScenarios(argc, argv);
    }
    if (hasServiceReportQueryAttributeOwnershipScenario(argc, argv)) {
      return runServiceReportQueryAttributeOwnershipScenarios(argc, argv);
    }
    if (hasServiceReportConfirmDivestitureScenario(argc, argv)) {
      return runServiceReportConfirmDivestitureScenarios(argc, argv);
    }
    if (hasServiceReportCancelAttributeOwnershipAcquisitionScenario(argc, argv)) {
      return runServiceReportCancelAttributeOwnershipAcquisitionScenarios(argc, argv);
    }
    if (hasServiceReportLocalDeleteObjectInstanceScenario(argc, argv)) {
      return runServiceReportLocalDeleteObjectInstanceScenarios(argc, argv);
    }
    if (hasServiceReportLocalDeleteObjectInstanceFailureScenario(argc, argv)) {
      return runServiceReportLocalDeleteObjectInstanceFailureScenarios(argc, argv);
    }
    if (hasServiceReportInterlockScenario(argc, argv)) {
      return runServiceReportInterlockScenarios(argc, argv);
    }
    if (hasServiceReportSynchronizationScenario(argc, argv)) {
      return runServiceReportSynchronizationScenarios(argc, argv);
    }
    if (hasServiceReportFederateSaveBegunScenario(argc, argv)) {
      return runServiceReportFederateSaveBegunScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedAttributeUpdateScenario(argc, argv)) {
      return runServiceReportTimestampedAttributeUpdateScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedDeleteObjectInstanceScenario(argc, argv)) {
      return runServiceReportTimestampedDeleteObjectInstanceScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedInteractionScenario(argc, argv)) {
      return runServiceReportTimestampedInteractionScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedAttributeUpdateFailureScenario(argc, argv)) {
      return runServiceReportTimestampedAttributeUpdateFailureScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedInteractionFailureScenario(argc, argv)) {
      return runServiceReportTimestampedInteractionFailureScenarios(argc, argv);
    }
    if (hasServiceReportTimestampedDeleteObjectInstanceFailureScenario(argc, argv)) {
      return runServiceReportTimestampedDeleteObjectInstanceFailureScenarios(argc, argv);
    }
    if (hasLogicalTimeScenario(argc, argv)) {
      return runLogicalTimeScenarios(argc, argv);
    }
    if (hasSynchronizationPointScenario(argc, argv)) {
      return runSynchronizationPointScenarios(argc, argv);
    }
    if (hasUnconditionalAttributeOwnershipDivestitureScenario(argc, argv)) {
      return runUnconditionalAttributeOwnershipDivestitureScenarios(argc, argv);
    }
    if (hasAutoProvideDisabledDiscoveryOnlyScenario(argc, argv)) {
      return runAutoProvideDisabledDiscoveryOnlyScenarios(argc, argv);
    }
    if (hasAutoProvideDisabledExplicitRequestScenario(argc, argv)) {
      return runAutoProvideDisabledExplicitRequestScenarios(argc, argv);
    }
    if (hasFederationAutoProvideMomSwitchScenario(argc, argv)) {
      return runFederationAutoProvideMomSwitchScenarios(argc, argv);
    }
    if (hasFederateServiceReportingMomSwitchScenario(argc, argv)) {
      return runFederateServiceReportingMomSwitchScenarios(argc, argv);
    }
    if (hasFederateServiceReportingMomAttributeScenario(argc, argv)) {
      return runFederateServiceReportingMomAttributeScenarios(argc, argv);
    }
    if (hasFederateObjectClassRelevanceAdvisoryMomAttributeScenario(
            argc,
            argv)) {
      return runFederateObjectClassRelevanceAdvisoryMomAttributeScenarios(
          argc,
          argv);
    }
    if (hasFederateAttributeRelevanceAdvisoryMomAttributeScenario(
            argc,
            argv)) {
      return runFederateAttributeRelevanceAdvisoryMomAttributeScenarios(
          argc,
          argv);
    }
    if (hasFederateAttributeScopeAdvisoryMomAttributeScenario(argc, argv)) {
      return runFederateAttributeScopeAdvisoryMomAttributeScenarios(
          argc,
          argv);
    }
    if (hasFederateInteractionRelevanceAdvisoryMomAttributeScenario(
            argc,
            argv)) {
      return runFederateInteractionRelevanceAdvisoryMomAttributeScenarios(
          argc,
          argv);
    }
    if (hasFederateAsynchronousDeliveryMomAttributeScenario(argc, argv)) {
      return runFederateAsynchronousDeliveryMomAttributeScenarios(
          argc,
          argv);
    }
    if (hasFederateTimeConstrainedMomAttributeScenario(argc, argv)) {
      return runFederateTimeConstrainedMomAttributeScenarios(argc, argv);
    }
    if (hasFederateTimeRegulatingMomAttributeScenario(argc, argv)) {
      return runFederateTimeRegulatingMomAttributeScenarios(argc, argv);
    }
    if (hasFederateLookaheadMomPeriodicScenario(argc, argv)) {
      return runFederateLookaheadMomPeriodicScenarios(argc, argv);
    }
    if (hasFederateLogicalTimeMomPeriodicScenario(argc, argv)) {
      return runFederateLogicalTimeMomPeriodicScenarios(argc, argv);
    }
    if (hasFederateTimeManagerStateMomAttributeScenario(argc, argv)) {
      return runFederateTimeManagerStateMomAttributeScenarios(argc, argv);
    }
    if (hasFederateExceptionReportingMomSwitchScenario(argc, argv)) {
      return runFederateExceptionReportingMomSwitchScenarios(argc, argv);
    }
    if (hasFederateExceptionReportingMomAttributeScenario(argc, argv)) {
      return runFederateExceptionReportingMomAttributeScenarios(argc, argv);
    }
    if (hasFederateExceptionReportDeliveryScenario(argc, argv)) {
      return runFederateExceptionReportDeliveryScenarios(argc, argv);
    }
    if (hasFederateMomExceptionReportDeliveryScenario(argc, argv)) {
      return runFederateMomExceptionReportDeliveryScenarios(argc, argv);
    }
    if (hasFederateMomExceptionMissingParameterScenario(argc, argv)) {
      return runFederateMomExceptionMissingParameterScenarios(argc, argv);
    }
    if (hasFederationMomExceptionMissingFomModuleIndicatorScenario(argc, argv)) {
      return runFederationMomExceptionMissingFomModuleIndicatorScenarios(argc, argv);
    }
    if (hasFederateMomFomModuleContentReportScenario(argc, argv)) {
      return runFederateMomFomModuleContentReportScenarios(argc, argv);
    }
    if (hasFederateMomObjectInstanceInformationScenario(argc, argv)) {
      return runFederateMomObjectInstanceInformationScenarios(argc, argv);
    }
    if (hasFederateMomPublicationQueryScenario(argc, argv)) {
      return runFederateMomPublicationQueryScenarios(argc, argv);
    }
    if (hasFederateMomCompleteObjectClassPublicationQueryScenario(argc, argv)) {
      return runFederateMomCompleteObjectClassPublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectClassAttributeUnpublicationQueryScenario(
            argc,
            argv)) {
      return runFederateMomObjectClassAttributeUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectClassFullAttributeUnpublicationQueryScenario(
            argc,
            argv)) {
      return runFederateMomObjectClassFullAttributeUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectClassWholeUnpublicationQueryScenario(argc, argv)) {
      return runFederateMomObjectClassWholeUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomInteractionClassWholeUnpublicationQueryScenario(
            argc,
            argv)) {
      return runFederateMomInteractionClassWholeUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomInteractionClassPartialUnpublicationQueryScenario(
            argc,
            argv)) {
      return runFederateMomInteractionClassPartialUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleInteractionClassFullUnpublicationQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleInteractionClassFullUnpublicationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleDirectedPublicationQueryScenario(argc, argv)) {
      return runFederateMomMultipleDirectedPublicationQueryScenarios(argc, argv);
    }
    if (hasFederateMomMultipleDirectedPublicationPartialUnpublishQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleDirectedPublicationPartialUnpublishQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleDirectedPublicationFullUnpublishQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleDirectedPublicationFullUnpublishQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomSubscriptionQueryScenario(argc, argv)) {
      return runFederateMomSubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomPassiveSubscriptionQueryScenario(argc, argv)) {
      return runFederateMomPassiveSubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomPassiveInteractionSubscriptionQueryScenario(argc, argv)) {
      return runFederateMomPassiveInteractionSubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomPassiveInteractionSubscriptionActivationQueryScenario(
            argc,
            argv)) {
      return runFederateMomPassiveInteractionSubscriptionActivationQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleInteractionSubscriptionQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleInteractionSubscriptionScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleInteractionSubscriptionUnsubscribeQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleInteractionSubscriptionUnsubscribeScenarios(
          argc,
          argv);
    }
    if (hasFederateMomMultipleInteractionSubscriptionFullUnsubscribeQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleInteractionSubscriptionFullUnsubscribeScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectAttributePartialUnsubscribeQueryScenario(argc, argv)) {
      return runFederateMomObjectAttributePartialUnsubscribeScenarios(argc, argv);
    }
    if (hasFederateMomObjectAttributeFullUnsubscribeQueryScenario(argc, argv)) {
      return runFederateMomObjectAttributeFullUnsubscribeScenarios(argc, argv);
    }
    if (hasFederateMomObjectClassFullUnsubscribeQueryScenario(argc, argv)) {
      return runFederateMomObjectClassFullUnsubscribeScenarios(argc, argv);
    }
    if (hasFederateMomObjectClassCompleteAttributeSubscriptionQueryScenario(
            argc,
            argv)) {
      return runFederateMomObjectClassCompleteAttributeSubscriptionScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectClassPassiveCompleteAttributeSubscriptionQueryScenario(
            argc,
            argv)) {
      return runFederateMomObjectClassPassiveCompleteAttributeSubscriptionScenarios(
          argc,
          argv);
    }
    if (hasFederateMomObjectClassCompleteAttributeSubscriptionActivationQueryScenario(
            argc,
            argv)) {
      return runFederateMomObjectClassCompleteAttributeSubscriptionActivationScenarios(
          argc,
          argv);
    }
    if (hasFederateMomEmptySubscriptionQueryScenario(argc, argv)) {
      return runFederateMomEmptySubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomDirectedSubscriptionQueryScenario(argc, argv)) {
      return runFederateMomDirectedSubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomMultipleDirectedSubscriptionQueryScenario(argc, argv)) {
      return runFederateMomMultipleDirectedSubscriptionQueryScenarios(argc, argv);
    }
    if (hasFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenario(
            argc,
            argv)) {
      return runFederateMomMultipleDirectedSubscriptionFullUnsubscribeQueryScenarios(
          argc,
          argv);
    }
    if (hasFederateMomExceptionReportServicePreconditionScenario(argc, argv)) {
      return runFederateMomExceptionReportServicePreconditionScenarios(argc, argv);
    }
    if (hasFederateSendServiceReportsToFileMomSwitchScenario(argc, argv)) {
      return runFederateSendServiceReportsToFileMomSwitchScenarios(argc, argv);
    }
    if (hasFederateAutomaticResignActionMomSwitchScenario(argc, argv)) {
      return runFederateAutomaticResignActionMomSwitchScenarios(argc, argv);
    }
    if (hasFederateAutomaticResignActionMomAttributeScenario(argc, argv)) {
      return runFederateAutomaticResignActionMomAttributeScenarios(argc, argv);
    }
    if (hasObjectRegistrationServiceBoundariesScenario(argc, argv)) {
      return runObjectRegistrationServiceBoundariesScenarios(argc, argv);
    }
    if (hasObjectDeletionServiceBoundariesScenario(argc, argv)) {
      return runObjectDeletionServiceBoundariesScenarios(argc, argv);
    }
    if (hasAttributeUpdateServiceBoundariesScenario(argc, argv)) {
      return runAttributeUpdateServiceBoundariesScenarios(argc, argv);
    }
    if (hasInteractionServiceBoundariesScenario(argc, argv)) {
      return runInteractionServiceBoundariesScenarios(argc, argv);
    }
    if (hasAttributeValueRequestServiceBoundariesScenario(argc, argv)) {
      return runAttributeValueRequestServiceBoundariesScenarios(argc, argv);
    }
    if (hasConnectionServiceBoundariesScenario(argc, argv)) {
      return runConnectionServiceBoundariesScenarios(argc, argv);
    }
    if (hasOwnershipServiceBoundariesScenario(argc, argv)) {
      return runOwnershipServiceBoundariesScenarios(argc, argv);
    }
    if (hasCustomTransportationAttributeDeliveryScenario(argc, argv)) {
      return runCustomTransportationAttributeDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedAttributeDeliveryScenario(argc, argv)) {
      return runCustomTransportationTimestampedAttributeDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedAttributeAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedAttributeAlternateAdvancesScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedInteractionAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedInteractionAlternateAdvancesScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDirectedDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationTimestampedDirectedDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationInteractionDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationInteractionDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDeliveryPortableScenario(argc, argv)) {
      return runCustomTransportationTimestampedDeliveryPortableScenarios(argc, argv);
    }
    if (hasCustomTransportationDirectedInteractionDeliveryScenario(argc, argv)) {
      return runCustomTransportationDirectedInteractionDeliveryScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenario(argc, argv)) {
      return runCustomTransportationTimestampedDirectedInteractionAlternateAdvancesScenarios(argc, argv);
    }
    if (hasDelaySubscriptionEvaluationScenario(argc, argv)) {
      return runDelaySubscriptionEvaluationScenarios(argc, argv);
    }
    if (hasFederationSaveRestoreInterlocksScenario(argc, argv)) {
      return runFederationSaveRestoreInterlocksScenarios(argc, argv);
    }
    if (hasTimedSaveRestoreScenario(argc, argv)) {
      return runTimedSaveRestoreScenarios(argc, argv);
    }
    if (hasRegionalDdmScenario(argc, argv)) {
      return runRegionalDdmScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideResponseScenario(argc, argv)) {
      return runRegionalAutoProvideResponseScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideTimestampedResponseScenario(argc, argv)) {
      return runRegionalAutoProvideTimestampedResponseScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideOverlapScenario(argc, argv)) {
      return runRegionalAutoProvideOverlapScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideMultiSourceScenario(argc, argv)) {
      return runRegionalAutoProvideMultiSourceScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideMultiProviderScenario(argc, argv)) {
      return runRegionalAutoProvideMultiProviderScenarios(argc, argv);
    }
    if (hasRegionalAutoProvideCallbackControlScenario(argc, argv)) {
      return runRegionalAutoProvideCallbackControlScenarios(argc, argv);
    }
    if (hasCustomTransportationTimestampedRegionalInteractionDeliveryScenario(argc, argv)) {
      return runCustomTransportationTimestampedRegionalInteractionDeliveryScenarios(argc, argv);
    }
    return hla_rti_cpp_tck_original_main(argc, argv);
  } catch (std::exception const& error) {
    std::cerr << "error: " << error.what() << '\n';
    return 2;
  }
}
