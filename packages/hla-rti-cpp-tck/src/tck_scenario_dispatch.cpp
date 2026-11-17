#include "tck_harness.hpp"
#include "tck_receive_order_callback_cancellation.hpp"
#include "tck_attribute_value_update_request_response.hpp"
#include "tck_update_rate_scenarios.hpp"
#include "tck_federate_update_threshold_scenarios.hpp"
#include "tck_federation_mom_content_scenarios.hpp"
#include "tck_federation_synchronization_mom_queries.hpp"
#include "tck_multi_attribute_delivery_scenarios.hpp"
#include "tck_api_foundation.hpp"
#include "tck_language_api_transparency.hpp"
#include "tck_configuration_authorization.hpp"
#include "tck_federation_mom_save_conditionals.hpp"
#include "tck_federate_save_timestamp_eligibility.hpp"
#include "tck_federation_save_request_replacement.hpp"
#include "tck_federation_mom_save_status.hpp"
#include "tck_joined_federate_mom_state_save_restore.hpp"
#include "tck_joined_federate_mom_galt_lits_periodic.hpp"
#include "tck_joined_federate_mom_tso_length_periodic.hpp"
#include "tck_joined_federate_mom_removed_object_count.hpp"
#include "tck_joined_federate_mom_updates_sent_counts.hpp"
#include "tck_joined_federate_mom_interactions_received_counts.hpp"
#include "tck_joined_federate_mom_interactions_sent_counts.hpp"
#include "tck_joined_federate_mom_directed_interactions_sent_counts.hpp"
#include "tck_mom_transportation_type_change_request.hpp"
#include "tck_joined_federate_mom_directed_interactions_received_counts.hpp"
#include "tck_joined_federate_mom_reflection_counts.hpp"
#include "tck_joined_federate_mom_updated_object_count_periodic.hpp"
#include "tck_joined_federate_mom_discovered_object_count_periodic.hpp"
#include "tck_joined_federate_mom_deleted_object_count_periodic.hpp"
#include "tck_joined_federate_mom_time_state_durations.hpp"
#include "tck_order_type_controls.hpp"
#include "tck_attribute_relevance_scenarios.hpp"
#include "tck_joined_federate_mom_reflections_received_counts.hpp"
#include "tck_factory_identity.hpp"
#include "tck_main_scenarios.hpp"
#include "tck_restore_save_after_federation_destruction.hpp"
#include "tck_object_removal_fanout_scenarios.hpp"
#include "tck_custom_transportation_interaction_scenarios.hpp"
#include "tck_scenario_dispatch.hpp"
#include "tck_lookup_lifecycle_scenarios.hpp"
#include "tck_fom_module_scenarios.hpp"
#include "tck_inherited_derived_delivery_scenarios.hpp"
#include "tck_directed_retraction_mixed_fanout.hpp"
#include "tck_ownership_cancellation_idle.hpp"
#include "tck_ownership_denial_idle.hpp"
#include "tck_ownership_denial_query.hpp"
#include "tck_ownership_transition_query.hpp"
#include "tck_ownership_reacquisition_query.hpp"
#include "tck_ownership_competing_if_available.hpp"
#include "tck_ownership_mixed_if_available.hpp"
#include "tck_ownership_unpublish_owned.hpp"
#include "tck_timestamped_interaction_boundary_scenarios.hpp"
#include "tck_region_lifecycle_scenarios.hpp"
#include "tck_data_encoding.hpp"
#include "tck_logical_time.hpp"

namespace hla_rti_cpp_tck::detail {

constexpr char federateSelectedUpdateThresholdsScenario[] =
    "cpp-tck.federate-selected-update-thresholds";
constexpr char federateSelectedUpdateThresholdsContractScenario[] =
    "cpp-tck.federate-selected-update-thresholds-contract";

constexpr char callbackDiscoveryTransportDirectionScenario[] =
    "cpp-tck.callback-controls-object-discovery-transport-direction";

constexpr char attributeScopeOwnershipAcquisitionNoOutOfScopeScenario[] =
    "cpp-tck.attribute-scope-ownership-acquisition-no-out-of-scope";

constexpr char fomEmptyDomainTablesAcceptedScenario[] =
    "cpp-tck.fom-empty-domain-tables-accepted";
constexpr char fomEmptyDomainTablesAcceptedContractScenario[] =
    "cpp-tck.fom-empty-domain-tables-accepted-contract";
constexpr char momTableClassStructureScenario[] =
    "cpp-tck.mom-table-class-structure";
constexpr char momPredefinedObjectAndInteractionScenario[] =
    "cpp-tck.mom-predefined-object-and-interaction";
constexpr char momModifyAttributeStateScenario[] =
    "cpp-tck.mom-modify-attribute-state";
constexpr char momTableInformationNotAlteredScenario[] =
    "cpp-tck.mom-table-information-not-altered";
constexpr char momTableAnnexGPrecedenceServiceReportFileScenario[] =
    "cpp-tck.mom-table-annex-g-precedence-service-report-file";
constexpr char serviceReportFileSwitchLifecycleScenario[] =
    "cpp-tck.service-report-file-switch-lifecycle";
constexpr char objectClassAbstractionSubscriptionScenario[] =
    "cpp-tck.object-class-abstraction-subscription";
constexpr char objectClassAbstractionSubscriptionContractScenario[] =
    "cpp-tck.object-class-abstraction-subscription-contract";
constexpr char nmraFutureTsoArrivalPendingRequestScenario[] =
    "cpp-tck.nmra-future-tso-arrival-pending-request";
constexpr char authorizerConnectServiceDecisionScenario[] =
    "cpp-tck.authorizer-connect-service-decision";
constexpr char authorizerUnrecognizedCredentialsUnauthorizedScenario[] =
    "cpp-tck.authorizer-unrecognized-credentials-unauthorized";
constexpr char authorizerDisabledServiceRejectsCredentialsScenario[] =
    "cpp-tck.authorizer-disabled-service-rejects-credentials";
constexpr char authorizerFederationServiceDecisionScenario[] =
    "cpp-tck.authorizer-federation-service-decision";
constexpr char authorizerDestroyServiceDecisionScenario[] =
    "cpp-tck.authorizer-destroy-service-decision";
constexpr char authorizerJoinServiceDecisionScenario[] =
    "cpp-tck.authorizer-join-service-decision";
constexpr char authorizerNoCredentialsConnectScenario[] =
    "cpp-tck.authorizer-no-credentials-connect";
constexpr char authorizerRidConfigurationDecisionScenario[] =
    "cpp-tck.authorizer-rid-configuration-decision";
constexpr char authorizerSingleServicePerRidProfileScenario[] =
    "cpp-tck.authorizer-single-service-per-rid-profile";
constexpr char basicDataElementValueOperationsContractScenario[] =
    "cpp-tck.basic-data-element-value-operations-contract";
constexpr char basicDataElementEncodeDecodeContractScenario[] =
    "cpp-tck.basic-data-element-encode-decode-contract";
constexpr char fixedRecordAlignmentScenario[] =
    "cpp-tck.fixed-record-alignment";
constexpr char variantRecordDiscriminantPaddingScenario[] =
    "cpp-tck.variant-record-discriminant-padding";
constexpr char variantRecordUnmappedDiscriminantScenario[] =
    "cpp-tck.variant-record-unmapped-discriminant";
constexpr char extendableVariantRecordLengthPaddingScenario[] =
    "cpp-tck.extendable-variant-record-length-padding";
constexpr char extendableVariantRecordAlternativePaddingScenario[] =
    "cpp-tck.extendable-variant-record-alternative-padding";
constexpr char extendableVariantRecordUnmappedDiscriminantScenario[] =
    "cpp-tck.extendable-variant-record-unmapped-discriminant";
constexpr char extendableVariantRecordRejectsBoundaryOverEightScenario[] =
    "cpp-tck.extendable-variant-record-rejects-boundary-over-eight";
constexpr char fixedArrayElementSequenceScenario[] =
    "cpp-tck.fixed-array-element-sequence";
constexpr char variableArrayElementSequenceScenario[] =
    "cpp-tck.variable-array-element-sequence";
constexpr char variableArrayFirstElementPaddingScenario[] =
    "cpp-tck.variable-array-first-element-padding";
constexpr char variableArrayInterElementPaddingScenario[] =
    "cpp-tck.variable-array-inter-element-padding";
constexpr char complexDataElementConstituentAccessScenario[] =
    "cpp-tck.complex-data-element-constituent-access";
constexpr char startRegistrationAdvisorySwitchGatingScenario[] =
    "cpp-tck.start-registration-advisory-switch-gating";

void scenarioTimestampedDirectedZeroLookaheadSendAtGrant(
    Options const&, rti::CallbackModel);
void scenarioSentTsoAttributePerRecipientOrderConversion(Options const&,
                                                         rti::CallbackModel);
void scenarioSentTsoAttributePerRecipientOrderConversionContract(
    Options const&, rti::CallbackModel);
void scenarioFqrPostgrantLowerTsoRetraction(Options const&, rti::CallbackModel);
void scenarioFqrPostgrantLowerTsoRetractionContract(Options const&, rti::CallbackModel);
void scenarioFqgPostgrantDeliveryCutoff(Options const&, rti::CallbackModel);
void scenarioFqgPostgrantDeliveryCutoffContract(Options const&, rti::CallbackModel);
void scenarioTarQueuedTsoBeforeGrant(Options const&, rti::CallbackModel);
void scenarioTarQueuedTsoBeforeGrantContract(Options const&, rti::CallbackModel);
void scenarioNmrLitsBelowGaltImmediateGrant(Options const&, rti::CallbackModel);
void scenarioNmrLitsBelowGaltImmediateGrantContract(Options const&, rti::CallbackModel);
void scenarioTimeConstrainedEnableRetainsQueuedTsoAttribute(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedEnableRetainsQueuedTsoAttributeContract(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrder(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrderContract(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrder(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrderContract(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrder(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrderContract(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrder(
    Options const&, rti::CallbackModel);
void scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrderContract(
    Options const&, rti::CallbackModel);

void scenarioActiveSubscriptionObjectDiscovery(Options const&, rti::CallbackModel);

void scenarioBaseSubscriptionRejectsDerivedOnlyAttribute(Options const&, rti::CallbackModel);
void scenarioBaseSubscriptionRejectsDerivedOnlyAttributeContract(Options const&, rti::CallbackModel);
void scenarioBasePublicationRejectsDerivedOnlyAttribute(Options const&, rti::CallbackModel);
void scenarioDdmAvailableAttributeDeclarationServices(Options const&, rti::CallbackModel);
void scenarioDdmAvailableAttributeDeclarationServicesContract(
    Options const&, rti::CallbackModel);
void scenarioStartRegistrationAdvisorySwitchGating(
    Options const&, rti::CallbackModel);
void scenarioDdmUnsubscribePreservesOtherClassSubscription(
    Options const&, rti::CallbackModel);
void scenarioDdmUnsubscribePreservesOtherClassSubscriptionContract(
    Options const&, rti::CallbackModel);
void scenarioUnsubscribeLastAttributeStopsDiscovery(Options const&, rti::CallbackModel);
void scenarioAttributeUnsubscribeStopsReflection(Options const&, rti::CallbackModel);
void scenarioAttributeUnsubscribeStopsReflectionContract(Options const&, rti::CallbackModel);
void scenarioPrivilegeToDeletePublicationEligibility(
    Options const&, rti::CallbackModel);
void scenarioPrivilegeToDeletePublicationEligibilityContract(
    Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryTimeAdvancingRo(
    Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryDisabledTimeAdvancingRo(
    Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryTsoExclusion(
    Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryTsoReenableGate(
    Options const&, rti::CallbackModel);
void scenarioAsynchronousDeliveryTsoExclusionContract(
    Options const&, rti::CallbackModel);
void scenarioWebSocketTransportSupport(
    Options const&, rti::CallbackModel);
void scenarioOneTransportConnectionPerFederateSession(
    Options const&, rti::CallbackModel);
void scenarioPublicTransportNewSessionInteroperability(
    Options const&, rti::CallbackModel);
void scenarioRejectsInvalidTransportSessionMetadata(
    Options const&, rti::CallbackModel);
void scenarioConnectionAdditionalSettingsParseFailure(
    Options const&, rti::CallbackModel);
void scenarioSecureWebSocketTransportSupport(
    Options const&, rti::CallbackModel);
void scenarioTls13TransportSupport(Options const&, rti::CallbackModel);
void scenarioTls13EncryptionOnlyMode(Options const&, rti::CallbackModel);
void scenarioTls13ServerCertificateValidation(
    Options const&, rti::CallbackModel);
void scenarioTls13ServerCertificateValidationRejectsUntrusted(
    Options const&, rti::CallbackModel);
void scenarioTls13DefaultPortSupport(Options const&, rti::CallbackModel);
void scenarioTls13AlternateAddressPortConfiguration(
    Options const&, rti::CallbackModel);

constexpr char restoreSaveAfterFederationDestructionScenario[] =
    "cpp-tck.restore-save-after-federation-destruction";
constexpr char restoreSaveAfterFederationDestructionContract[] =
    "cpp-tck.restore-save-after-federation-destruction-contract";
constexpr char saveRestoreQueuedReceiveOrderInteractionScenario[] =
    "cpp-tck.save-restore-queued-ro-interaction";
constexpr char restoreFederateTypeCountMismatchScenario[] =
    "cpp-tck.restore-federate-type-count-mismatch";
constexpr char restoreFederateTypeIdentityRebindingScenario[] =
    "cpp-tck.restore-federate-type-identity-rebinding";
constexpr char webSocketTransportSupportScenario[] =
    "cpp-tck.websocket-transport-support";
constexpr char oneTransportConnectionPerFederateSessionScenario[] =
    "cpp-tck.one-transport-connection-per-federate-session";
constexpr char publicTransportNewSessionInteroperabilityScenario[] =
    "cpp-tck.transport-public-new-session-interoperability";
constexpr char invalidTransportSessionMetadataScenario[] =
    "cpp-tck.reject-invalid-transport-session-metadata";
constexpr char connectionAdditionalSettingsParseFailureScenario[] =
    "cpp-tck.connection-additional-settings-parse-failure";
constexpr char connectionDefaultConfigurationFallbackScenario[] =
    "cpp-tck.connection-default-configuration-fallback";
constexpr char tls13TransportSupportScenario[] =
    "cpp-tck.tls13-transport-support";
constexpr char tls13EncryptionOnlyModeScenario[] =
    "cpp-tck.tls13-encryption-only-mode";
constexpr char tls13ServerCertificateValidationScenario[] =
    "cpp-tck.tls13-server-certificate-validation";
constexpr char tls13ServerCertificateValidationRejectsUntrustedScenario[] =
    "cpp-tck.tls13-server-certificate-validation-rejects-untrusted";
constexpr char tls13DefaultPortSupportScenario[] =
    "cpp-tck.tls13-default-port-support";
constexpr char tls13AlternateAddressPortConfigurationScenario[] =
    "cpp-tck.tls13-alternate-address-port-configuration";
constexpr char secureWebSocketTransportSupportScenario[] =
    "cpp-tck.secure-websocket-transport-support";

ScenarioFunction scenarioFunction(std::string const& id) {
  constexpr char negotiatedDivestitureDuplicateRequestScenario[] =
      "cpp-tck.negotiated-divestiture-duplicate-request";
  constexpr char negotiatedDivestitureIfWantedCompletionScenario[] =
      "cpp-tck.negotiated-divestiture-if-wanted-completion";
  if (id == restoreSaveAfterFederationDestructionScenario) {
    return scenarioRestoreSaveAfterFederationDestruction;
  }
  if (id == restoreSaveAfterFederationDestructionContract) {
    return scenarioRestoreSaveAfterFederationDestructionContract;
  }
  if (id == saveRestoreQueuedReceiveOrderInteractionScenario) {
    return scenarioSaveRestoreQueuedReceiveOrderInteraction;
  }
  if (id == "java-tck.factory-discovery") return scenarioFactoryDiscovery;
  if (id == "cpp-tck.rti-ambassador-factory-contract") {
    return scenarioRTIambassadorFactoryContract;
  }
  if (id == "cpp-tck.three-library-set-contract") {
    return scenarioThreeLibrarySetContract;
  }
  if (id == "java-tck.api-surface-inventory") return scenarioApiSurfaceInventory;
  if (id == "cpp-tck.annex-c-api-header-inventory") {
    return scenarioAnnexCApiHeaderInventory;
  }
  if (id == "cpp-tck.language-api-transparency") {
    return scenarioLanguageApiTransparency;
  }
  if (id == "cpp-tck.variable-length-data-contract") {
    return scenarioVariableLengthDataContract;
  }
  if (id == "cpp-tck.logical-time-contract") {
    return scenarioLogicalTimeContract;
  }
  if (id == "cpp-tck.logical-time-header-layout") {
    return scenarioLogicalTimeHeaderLayoutContract;
  }
  if (id == "cpp-tck.logical-time-interval-nonnegative") {
    return scenarioLogicalTimeIntervalNonnegativeContract;
  }
  if (id == "cpp-tck.logical-time-factory-factory-contract") {
    return scenarioLogicalTimeFactoryFactoryContract;
  }
  if (id == "cpp-tck.federation-supplied-custom-tradt") {
    return scenarioFederationSuppliedCustomTradt;
  }
  if (id == "cpp-tck.logical-time-data-elements-contract") {
    return scenarioLogicalTimeDataElementsContract;
  }
  if (id == "cpp-tck.connection-callback-contract") {
    return scenarioConnectionCallbackContract;
  }
  if (id == webSocketTransportSupportScenario) {
    return scenarioWebSocketTransportSupport;
  }
  if (id == oneTransportConnectionPerFederateSessionScenario) {
    return scenarioOneTransportConnectionPerFederateSession;
  }
  if (id == publicTransportNewSessionInteroperabilityScenario) {
    return scenarioPublicTransportNewSessionInteroperability;
  }
  if (id == invalidTransportSessionMetadataScenario) {
    return scenarioRejectsInvalidTransportSessionMetadata;
  }
  if (id == connectionAdditionalSettingsParseFailureScenario) {
    return scenarioConnectionAdditionalSettingsParseFailure;
  }
  if (id == connectionDefaultConfigurationFallbackScenario) {
    return scenarioConnectionDefaultConfigurationFallback;
  }
  if (id == tls13TransportSupportScenario) {
    return scenarioTls13TransportSupport;
  }
  if (id == tls13EncryptionOnlyModeScenario) {
    return scenarioTls13EncryptionOnlyMode;
  }
  if (id == tls13ServerCertificateValidationScenario) {
    return scenarioTls13ServerCertificateValidation;
  }
  if (id == tls13ServerCertificateValidationRejectsUntrustedScenario) {
    return scenarioTls13ServerCertificateValidationRejectsUntrusted;
  }
  if (id == tls13DefaultPortSupportScenario) {
    return scenarioTls13DefaultPortSupport;
  }
  if (id == tls13AlternateAddressPortConfigurationScenario) {
    return scenarioTls13AlternateAddressPortConfiguration;
  }
  if (id == secureWebSocketTransportSupportScenario) {
    return scenarioSecureWebSocketTransportSupport;
  }
  if (id == "cpp-tck.callback-reentrancy") {
    return scenarioCallbackReentrancy;
  }
  if (id == "cpp-tck.callback-reentrancy-contract") {
    return scenarioCallbackReentrancyContract;
  }
  if (id == "cpp-tck.evoke-multiple-callbacks-minimum-duration") {
    return scenarioEvokeMultipleCallbacksMinimumDuration;
  }
  if (id == "cpp-tck.evoke-multiple-callbacks-maximum-duration") {
    return scenarioEvokeMultipleCallbacksMaximumDuration;
  }
  if (id == "cpp-tck.evoke-multiple-callbacks-millisecond-precision") {
    return scenarioEvokeMultipleCallbacksMillisecondPrecision;
  }
  if (id == "cpp-tck.federation-lifecycle-contract") {
    return scenarioFederationLifecycleContract;
  }
  if (id == "cpp-tck.connect-before-federation-disconnect-after-resign") {
    return scenarioConnectBeforeFederationAndDisconnectAfterResign;
  }
  if (id == "cpp-tck.federation-management-requires-connection") {
    return scenarioFederationManagementRequiresConnection;
  }
  if (id == "cpp-tck.declaration-management-contract") {
    return scenarioDeclarationManagementContract;
  }
  if (id == "cpp-tck.interaction-full-unpublication") {
    return scenarioInteractionFullUnpublication;
  }
  if (id == "cpp-tck.declaration-publish-before-send") {
    return scenarioDeclarationPublicationGate;
  }
  if (id == "cpp-tck.declaration-time-position-independence") {
    return scenarioDeclarationTimePositionIndependence;
  }
  if (id == "cpp-tck.declaration-register-before-publish") {
    return scenarioDeclarationObjectRegistrationGate;
  }
  if (id == "cpp-tck.declaration-directed-send-before-publish") {
    return scenarioDeclarationDirectedPublicationGate;
  }
  if (id == "cpp-tck.unsubscribe-last-attribute-stops-discovery") {
    return scenarioUnsubscribeLastAttributeStopsDiscovery;
  }
  if (id == "cpp-tck.attribute-unsubscribe-stops-reflection") {
    return scenarioAttributeUnsubscribeStopsReflection;
  }
  if (id == "cpp-tck.attribute-unsubscribe-stops-reflection-contract") {
    return scenarioAttributeUnsubscribeStopsReflectionContract;
  }
  if (id == "cpp-tck.base-subscription-rejects-derived-only-attribute") {
    return scenarioBaseSubscriptionRejectsDerivedOnlyAttribute;
  }
  if (id == "cpp-tck.base-subscription-rejects-derived-only-attribute-contract") {
    return scenarioBaseSubscriptionRejectsDerivedOnlyAttributeContract;
  }
  if (id == "cpp-tck.base-publication-rejects-derived-only-attribute") {
    return scenarioBasePublicationRejectsDerivedOnlyAttribute;
  }
  if (id == "cpp-tck.ddm-available-attribute-services") {
    return scenarioDdmAvailableAttributeDeclarationServices;
  }
  if (id == "cpp-tck.ddm-available-attribute-services-contract") {
    return scenarioDdmAvailableAttributeDeclarationServicesContract;
  }
  if (id == "cpp-tck.ddm-unsubscribe-preserves-other-class-subscription") {
    return scenarioDdmUnsubscribePreservesOtherClassSubscription;
  }
  if (id == "cpp-tck.ddm-unsubscribe-preserves-other-class-subscription-contract") {
    return scenarioDdmUnsubscribePreservesOtherClassSubscriptionContract;
  }
  if (id == "cpp-tck.object-management-contract") {
    return scenarioObjectManagementContract;
  }
  if (id == "cpp-tck.attribute-interaction-contract") {
    return scenarioAttributeInteractionContract;
  }
  if (id == "cpp-tck.directed-interaction-contract") {
    return scenarioDirectedInteractionContract;
  }
  if (id == "cpp-tck.ownership-management-contract") {
    return scenarioOwnershipManagementContract;
  }
  if (id == "cpp-tck.unconditional-divestiture-mixed-set-atomicity") {
    return scenarioUnconditionalDivestitureMixedSetAtomicity;
  }
  if (id == "cpp-tck.unconditional-divestiture-mixed-set-atomicity-contract") {
    return scenarioUnconditionalDivestitureMixedSetAtomicityContract;
  }
  if (id == "cpp-tck.unconditional-divestiture-assumption-coalescing") {
    return scenarioUnconditionalDivestitureAssumptionCoalescing;
  }
  if (id == "cpp-tck.unconditional-divestiture-assumption-coalescing-contract") {
    return scenarioUnconditionalDivestitureAssumptionCoalescingContract;
  }
  if (id == "cpp-tck.negotiated-divestiture-confirmation-coalescing") {
    return scenarioNegotiatedDivestitureConfirmationCoalescing;
  }
  if (id == "cpp-tck.negotiated-divestiture-confirmation-coalescing-contract") {
    return scenarioNegotiatedDivestitureConfirmationCoalescingContract;
  }
  if (id == "cpp-tck.negotiated-divestiture-waiting-recovery") {
    return scenarioNegotiatedDivestitureReturnsToWaitingForNewOwner;
  }
  if (id == "cpp-tck.negotiated-divestiture-waiting-recovery-contract") {
    return scenarioNegotiatedDivestitureReturnsToWaitingForNewOwnerContract;
  }
  if (id == negotiatedDivestitureDuplicateRequestScenario) {
    return scenarioNegotiatedDivestitureDuplicateRequest;
  }
  if (id == negotiatedDivestitureIfWantedCompletionScenario) {
    return scenarioNegotiatedDivestitureIfWantedCompletion;
  }
  if (id == "cpp-tck.attribute-ownership-release-coalescing") {
    return scenarioAttributeOwnershipReleaseCoalescing;
  }
  if (id == "cpp-tck.attribute-ownership-release-coalescing-contract") {
    return scenarioAttributeOwnershipReleaseCoalescingContract;
  }
  if (id == "cpp-tck.privilege-to-delete-transfer") {
    return scenarioPrivilegeToDeleteTransfer;
  }
  if (id == "cpp-tck.privilege-to-delete-transfer-contract") {
    return scenarioPrivilegeToDeleteTransferContract;
  }
  if (id == "cpp-tck.privilege-to-delete-publication-eligibility") {
    return scenarioPrivilegeToDeletePublicationEligibility;
  }
  if (id == "cpp-tck.privilege-to-delete-publication-eligibility-contract") {
    return scenarioPrivilegeToDeletePublicationEligibilityContract;
  }
  if (id == "cpp-tck.ownership-state-save-restore-contract") {
    return scenarioOwnershipSaveRestoreContract;
  }
  if (id == "cpp-tck.pending-negotiated-ownership-save-restore-contract") {
    return scenarioPendingNegotiatedOwnershipSaveRestoreContract;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-save-restore-contract") {
    return scenarioOwnershipAcquisitionCancellationSaveRestoreContract;
  }
  if (id == "cpp-tck.if-available-ownership-restore-unavailable-callback-contract") {
    return scenarioIfAvailableOwnershipRestoreUnavailableCallbackContract;
  }
  if (id == "cpp-tck.synchronization-point-contract") {
    return scenarioSynchronizationPointContract;
  }
  if (id == "cpp-tck.federation-save-restore-contract") {
    return scenarioFederationSaveRestoreContract;
  }
  if (id == "cpp-tck.asynchronous-delivery-contract") {
    return scenarioAsynchronousDeliveryContract;
  }
  if (id == "cpp-tck.asynchronous-attribute-delivery") {
    return scenarioAsynchronousAttributeDelivery;
  }
  if (id == "cpp-tck.asynchronous-delivery-tso-exclusion") {
    return scenarioAsynchronousDeliveryTsoExclusion;
  }
  if (id == "cpp-tck.asynchronous-delivery-time-advancing-ro") {
    return scenarioAsynchronousDeliveryTimeAdvancingRo;
  }
  if (id == "cpp-tck.asynchronous-delivery-disabled-time-advancing-ro") {
    return scenarioAsynchronousDeliveryDisabledTimeAdvancingRo;
  }
  if (id == "cpp-tck.asynchronous-delivery-reenable-tso-gate") {
    return scenarioAsynchronousDeliveryTsoReenableGate;
  }
  if (id == "cpp-tck.asynchronous-delivery-tso-exclusion-contract") {
    return scenarioAsynchronousDeliveryTsoExclusionContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-contract") {
    return scenarioTimestampedAttributeUpdateContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-contract") {
    return scenarioTimestampedObjectDeletionContract;
  }
  if (id == "cpp-tck.alternate-time-advances-contract") {
    return scenarioAlternateTimeAdvancesContract;
  }
  if (id == "cpp-tck.float64-time-reference-contract") {
    return scenarioFloat64TimeReferenceContract;
  }
  if (id == "cpp-tck.non-regulating-federate-peer-advance-independence-contract") {
    return scenarioNonRegulatingFederatePeerAdvanceIndependenceContract;
  }
  if (id == "cpp-tck.support-services-contract") {
    return scenarioSupportServicesContract;
  }
  if (id == "cpp-tck.attribute-name-lookup") {
    return scenarioAttributeNameLookup;
  }
  if (id == "cpp-tck.interaction-class-handle-lookup") {
    return scenarioInteractionClassHandleLookup;
  }
  if (id == "cpp-tck.parameter-handle-name-lookup") {
    return scenarioParameterHandleNameLookup;
  }
  if (id == "cpp-tck.available-object-class-dimensions") {
    return scenarioAvailableObjectClassDimensions;
  }
  if (id == "cpp-tck.available-interaction-class-dimensions") {
    return scenarioAvailableInteractionClassDimensions;
  }
  if (id == "cpp-tck.dimension-handle-name-lookup") {
    return scenarioDimensionHandleNameLookup;
  }
  if (id == "cpp-tck.dimension-handle-set-for-region") {
    return scenarioDimensionHandleSetForRegion;
  }
  if (id == "cpp-tck.unconveyed-peer-region-dimension-handle-set-invalid-region") {
    return scenarioUnconveyedPeerRegionDimensionHandleSetInvalidRegion;
  }
  if (id == "cpp-tck.range-bounds-for-region") {
    return scenarioRangeBoundsForRegion;
  }
  if (id == "cpp-tck.unconveyed-peer-region-range-bounds-invalid-region") {
    return scenarioUnconveyedPeerRegionRangeBoundsInvalidRegion;
  }
  if (id == "cpp-tck.invalid-region-designator-range-bounds-invalid-region") {
    return scenarioInvalidRegionDesignatorRangeBoundsInvalidRegion;
  }
  if (id == "cpp-tck.handle-normalization-equal-instances") {
    return scenarioHandleNormalizationEqualInstances;
  }
  if (id == "cpp-tck.normalize-object-class-handle-region-coordinate") {
    return scenarioNormalizeObjectClassHandleRegionCoordinate;
  }
  if (id == "cpp-tck.normalize-interaction-class-handle-region-coordinate") {
    return scenarioNormalizeInteractionClassHandleRegionCoordinate;
  }
  if (id == "cpp-tck.normalize-object-instance-handle-region-coordinate") {
    return scenarioNormalizeObjectInstanceHandleRegionCoordinate;
  }
  if (id == "cpp-tck.evoke-callback-empty-queue-timeout") {
    return scenarioEvokeCallbackWaitsForTimeoutWithNoPendingCallback;
  }
  if (id == "cpp-tck.evoke-callback-single-dispatch-pending-indicator") {
    return scenarioEvokeCallbackSingleDispatchPendingIndicator;
  }
  if (id == "cpp-tck.evoke-callback-wait-duration-precision") {
    return scenarioEvokeCallbackWaitDurationPrecision;
  }
  if (id == "cpp-tck.evoke-callback-immediate-no-effect") {
    return scenarioEvokeCallbackImmediateNoEffect;
  }
  if (id == "cpp-tck.evoke-multiple-callbacks-pending-indicator") {
    return scenarioEvokeMultipleCallbacksPendingIndicator;
  }
  if (id == "cpp-tck.disable-callbacks-gates-evoke-services") {
    return scenarioDisableCallbacksGateEvokeServices;
  }
  if (id == "cpp-tck.evoked-callback-no-concurrency") {
    return scenarioEvokedCallbacksAreNotConcurrent;
  }
  if (id == "cpp-tck.federate-initiated-concurrency-different-ambassadors") {
    return scenarioConcurrentCallsDifferentAmbassadors;
  }
  if (id == "cpp-tck.handle-encoding-mim-data-types") {
    return scenarioHandleEncodingMimDataTypes;
  }
  if (id == "cpp-tck.handle-parameter-value-round-trip") {
    return scenarioHandleParameterValueRoundTrip;
  }
  if (id == "cpp-tck.handle-decoder-semantic-validity") {
    return scenarioHandleDecoderDoesNotCheckSemanticValidity;
  }
  if (id == "cpp-tck.transport-order-contract") {
    return scenarioTransportOrderContract;
  }
  if (id == "cpp-tck.relevance-advisories-contract") {
    return scenarioRelevanceAdvisoriesContract;
  }
  if (id == "cpp-tck.attribute-relevance-known-class-contract") {
    return scenarioKnownClassAttributeRelevanceContract;
  }
  if (id == "cpp-tck.attribute-relevance-known-class-disabled-contract") {
    return scenarioKnownClassAttributeRelevanceDisabledContract;
  }
  if (id == "cpp-tck.attribute-relevance-rate-reissue-contract") {
    return scenarioAttributeRelevanceRateReissueContract;
  }
  if (id == "cpp-tck.fom-model-contract") {
    return scenarioFomModelContract;
  }
  if (id == "cpp-tck.fom-module-composition-contract") {
    return scenarioFomModuleCompositionContract;
  }
  if (id == "cpp-tck.fom-additional-module-join-atomicity-contract") {
    return scenarioFomAdditionalModuleJoinAtomicityContract;
  }
  if (id == "cpp-tck.fom-transportation-handle-stability-contract") {
    return scenarioFomTransportationHandleStabilityContract;
  }
  if (id == "cpp-tck.fom-dimension-handle-stability-contract") {
    return scenarioFomDimensionHandleStabilityContract;
  }
  if (id == "cpp-tck.fom-update-rate-value-stability-contract") {
    return scenarioFomUpdateRateValueStabilityContract;
  }
  if (id == "cpp-tck.interaction-class-lookup-lifecycle-contract") {
    return scenarioInteractionClassLookupLifecycleContract;
  }
  if (id == "cpp-tck.object-class-lookup-lifecycle-contract") {
    return scenarioObjectClassLookupLifecycleContract;
  }
  if (id == "cpp-tck.attribute-lookup-lifecycle-contract") {
    return scenarioAttributeLookupLifecycleContract;
  }
  if (id == "cpp-tck.parameter-lookup-lifecycle-contract") {
    return scenarioParameterLookupLifecycleContract;
  }
  if (id == "cpp-tck.dimension-lookup-lifecycle-contract") {
    return scenarioDimensionLookupLifecycleContract;
  }
  if (id == "cpp-tck.fom-basic-data-representation-endian-values-contract") {
    return scenarioFomBasicDataRepresentationEndianValuesContract;
  }
  if (id == "cpp-tck.fom-standard-invalid-input-corpus-contract") {
    return scenarioFomStandardInvalidInputCorpusContract;
  }
  if (id == "cpp-tck.fom-invalid-create-atomicity-contract") {
    return scenarioFomInvalidCreateAtomicityContract;
  }
  if (id == "cpp-tck.fom-invalid-composite-join-atomicity-contract") {
    return scenarioFomInvalidCompositeJoinAtomicityContract;
  }
  if (id == "cpp-tck.fom-invalid-mim-create-atomicity-contract") {
    return scenarioFomInvalidMimCreateAtomicityContract;
  }
  if (id == "cpp-tck.fom-invalid-composite-mim-create-atomicity-contract") {
    return scenarioFomInvalidCompositeMimCreateAtomicityContract;
  }
  if (id == "cpp-tck.fom-empty-mim-create-atomicity-contract") {
    return scenarioFomEmptyMimCreateAtomicityContract;
  }
  if (id == "cpp-tck.fom-empty-module-validation-contract") {
    return scenarioFomEmptyModuleValidationContract;
  }
  if (id == fomEmptyDomainTablesAcceptedContractScenario) {
    return scenarioFomEmptyDomainTablesAcceptedContract;
  }
  if (id == "cpp-tck.fom-data-type-forward-reference-contract") {
    return scenarioFomDataTypeForwardReferenceContract;
  }
  if (id == "cpp-tck.fom-data-type-conflicting-definition-contract") {
    return scenarioFomDataTypeConflictingDefinitionContract;
  }
  if (id == "cpp-tck.fom-variant-alternative-overlap-contract") {
    return scenarioFomVariantAlternativeOverlapContract;
  }
  if (id == "cpp-tck.fom-interaction-parameter-merge-conflict-contract") {
    return scenarioFomInteractionParameterMergeConflictContract;
  }
  if (id == "cpp-tck.fom-object-attribute-merge-conflict-contract") {
    return scenarioFomObjectAttributeMergeConflictContract;
  }
  if (id == "cpp-tck.fom-object-class-semantics-conflict-contract") {
    return scenarioFomObjectClassSemanticsConflictContract;
  }
  if (id == "cpp-tck.fom-matching-data-types-contract") {
    return scenarioFomMatchingDataTypesContract;
  }
  if (id == "cpp-tck.fom-object-class-dimension-merge-contract") {
    return scenarioFomObjectClassDimensionMergeContract;
  }
  if (id == "cpp-tck.fom-interaction-class-dimension-merge-contract") {
    return scenarioFomInteractionClassDimensionMergeContract;
  }
  if (id == "cpp-tck.fom-named-variant-alternative-merge-contract") {
    return scenarioFomNamedVariantAlternativeMergeContract;
  }
  if (id == "cpp-tck.fom-extendable-variant-hlaother-contract") {
    return scenarioFomExtendableVariantHlaOtherContract;
  }
  if (id == "cpp-tck.fom-enumerated-value-collision-contract") {
    return scenarioFomEnumeratedValueCollisionContract;
  }
  if (id == "cpp-tck.fom-nonextendable-variant-alternative-conflict-contract") {
    return scenarioFomNonextendableVariantAlternativeConflictContract;
  }
  if (id == "cpp-tck.fom-array-cardinality-forms-contract") {
    return scenarioFomArrayCardinalityFormsContract;
  }
  if (id == "cpp-tck.fom-array-cardinality-rejections-contract") {
    return scenarioFomArrayCardinalityRejectionsContract;
  }
  if (id == "cpp-tck.fom-time-representation-valid-contract") {
    return scenarioFomTimeRepresentationValidContract;
  }
  if (id == "cpp-tck.fom-time-representation-rejections-contract") {
    return scenarioFomTimeRepresentationRejectionsContract;
  }
  if (id == "cpp-tck.fom-default-logical-time-selection-contract") {
    return scenarioFomDefaultLogicalTimeSelectionContract;
  }
  if (id == "cpp-tck.fom-unavailable-logical-time-factory-contract") {
    return scenarioFomUnavailableLogicalTimeFactoryContract;
  }
  if (id == "cpp-tck.fom-member-data-type-categories-contract") {
    return scenarioFomMemberDataTypeCategoriesContract;
  }
  if (id == "cpp-tck.fom-attribute-na-companion-completion-contract") {
    return scenarioFomAttributeNaCompanionCompletionContract;
  }
  if (id == "cpp-tck.fom-attribute-na-companion-rejections-contract") {
    return scenarioFomAttributeNaCompanionRejectionsContract;
  }
  if (id == "cpp-tck.fom-dynamic-update-condition-valid-contract") {
    return scenarioFomDynamicUpdateConditionValidContract;
  }
  if (id == "cpp-tck.fom-dynamic-update-condition-rejections-contract") {
    return scenarioFomDynamicUpdateConditionRejectionsContract;
  }
  if (id == "cpp-tck.fom-attribute-value-required-sharing-contract") {
    return scenarioFomAttributeValueRequiredSharingContract;
  }
  if (id == "cpp-tck.fom-attribute-table-nonblank-semantics-contract") {
    return scenarioFomAttributeTableNonblankSemanticsContract;
  }
  if (id == "cpp-tck.fom-reference-data-type-name-convention-contract") {
    return scenarioFomReferenceDataTypeNameConventionContract;
  }
  if (id == "cpp-tck.fom-simple-data-type-name-convention-contract") {
    return scenarioFomSimpleDataTypeNameConventionContract;
  }
  if (id == "cpp-tck.fom-model-identification-name-valid-contract") {
    return scenarioFomModelIdentificationNameValidContract;
  }
  if (id == "cpp-tck.fom-enumerated-data-type-name-convention-contract") {
    return scenarioFomEnumeratedDataTypeNameConventionContract;
  }
  if (id == "cpp-tck.fom-enumerator-name-convention-contract") {
    return scenarioFomEnumeratorNameConventionContract;
  }
  if (id == "cpp-tck.fom-array-data-type-name-convention-contract") {
    return scenarioFomArrayDataTypeNameConventionContract;
  }
  if (id == "cpp-tck.fom-array-element-type-reference-contract") {
    return scenarioFomArrayElementTypeReferenceContract;
  }
  if (id == "cpp-tck.fom-root-hierarchy-contract") {
    return scenarioFomRootHierarchyContract;
  }
  if (id == "cpp-tck.fom-tag-data-type-categories-contract") {
    return scenarioFomTagDataTypeCategoriesContract;
  }
  if (id == "cpp-tck.fom-inherited-member-names-contract") {
    return scenarioFomInheritedMemberNamesContract;
  }
  if (id == "cpp-tck.fom-equivalent-class-conflict-contract") {
    return scenarioFomEquivalentClassConflictContract;
  }
  if (id == "cpp-tck.fom-reference-class-resolution-contract") {
    return scenarioFomReferenceClassResolutionContract;
  }
  if (id == "cpp-tck.fom-enumerated-representation-contract") {
    return scenarioFomEnumeratedRepresentationContract;
  }
  if (id == "cpp-tck.fom-array-element-category-contract") {
    return scenarioFomArrayElementCategoryContract;
  }
  if (id == "cpp-tck.fom-record-member-category-contract") {
    return scenarioFomRecordMemberCategoryContract;
  }
  if (id == "cpp-tck.fom-variant-discriminant-category-contract") {
    return scenarioFomVariantDiscriminantCategoryContract;
  }
  if (id == "cpp-tck.fom-variant-record-encoding-contract") {
    return scenarioFomVariantRecordEncodingContract;
  }
  if (id == "cpp-tck.fom-variant-alternative-type-category-contract") {
    return scenarioFomVariantAlternativeTypeCategoryContract;
  }
  if (id == "cpp-tck.fom-variant-alternative-name-convention-contract") {
    return scenarioFomVariantAlternativeNameConventionContract;
  }
  if (id == "cpp-tck.fom-variant-discriminant-name-convention-contract") {
    return scenarioFomVariantDiscriminantNameConventionContract;
  }
  if (id == "cpp-tck.fom-variant-record-name-convention-contract") {
    return scenarioFomVariantRecordNameConventionContract;
  }
  if (id == "cpp-tck.fom-variant-enumerator-syntax-contract") {
    return scenarioFomVariantEnumeratorSyntaxContract;
  }
  if (id == "cpp-tck.fom-variant-enumerator-membership-contract") {
    return scenarioFomVariantEnumeratorMembershipContract;
  }
  if (id == "cpp-tck.fom-variant-range-semantics-contract") {
    return scenarioFomVariantRangeSemanticsContract;
  }
  if (id == "cpp-tck.fom-file-url-designator-contract") {
    return scenarioFomFileUrlDesignatorContract;
  }
  if (id == "cpp-tck.fom-reference-attribute-resolution-contract") {
    return scenarioFomReferenceAttributeResolutionContract;
  }
  if (id == "cpp-tck.fom-data-representation-resolution-contract") {
    return scenarioFomDataRepresentationResolutionContract;
  }
  if (id == "cpp-tck.fom-noncompliant-module-contract") {
    return scenarioFomNoncompliantModuleContract;
  }
  if (id == "cpp-tck.fom-incomplete-class-definition-contract") {
    return scenarioFomIncompleteClassDefinitionContract;
  }
  if (id == "cpp-tck.fom-transportation-reference-resolution-contract") {
    return scenarioFomTransportationReferenceResolutionContract;
  }
  if (id == "cpp-tck.fom-directed-reference-resolution-contract") {
    return scenarioFomDirectedReferenceResolutionContract;
  }
  if (id == "cpp-tck.fom-dimension-reference-resolution-contract") {
    return scenarioFomDimensionReferenceResolutionContract;
  }
  if (id == "cpp-tck.fom-dimension-duplicate-union-contract") {
    return scenarioFomDimensionDuplicateUnionContract;
  }
  if (id == "cpp-tck.fom-dimension-duplicate-conflict-contract") {
    return scenarioFomDimensionDuplicateConflictContract;
  }
  if (id == "cpp-tck.fom-dimension-input-type-contract") {
    return scenarioFomDimensionInputTypeContract;
  }
  if (id == "cpp-tck.fom-dimension-input-description-contract") {
    return scenarioFomDimensionInputDescriptionContract;
  }
  if (id == "cpp-tck.fom-dimension-input-na-exclusivity-contract") {
    return scenarioFomDimensionInputNaExclusivityContract;
  }
  if (id == "cpp-tck.fom-dimension-name-uniqueness-contract") {
    return scenarioFomDimensionNameUniquenessContract;
  }
  if (id == "cpp-tck.fom-object-class-name-conventions-contract") {
    return scenarioFomObjectClassNameConventionsContract;
  }
  if (id == "cpp-tck.fom-model-modification-date-contract") {
    return scenarioFomModelModificationDateContract;
  }
  if (id == "cpp-tck.fom-positive-update-rate-contract") {
    return scenarioFomPositiveUpdateRateContract;
  }
  if (id == "cpp-tck.fom-dimension-default-range-contract") {
    return scenarioFomDimensionDefaultRangeContract;
  }
  if (id == "cpp-tck.fom-synchronization-duplicate-equal-contract") {
    return scenarioFomSynchronizationDuplicateEqualContract;
  }
  if (id == "cpp-tck.fom-synchronization-duplicate-note-conflict-contract") {
    return scenarioFomSynchronizationDuplicateNoteConflictContract;
  }
  if (id == "cpp-tck.fom-synchronization-table-validation-contract") {
    return scenarioFomSynchronizationTableValidationContract;
  }
  if (id == "cpp-tck.fom-transportation-duplicate-equal-contract") {
    return scenarioFomTransportationDuplicateEqualContract;
  }
  if (id == "cpp-tck.fom-transportation-duplicate-note-conflict-contract") {
    return scenarioFomTransportationDuplicateNoteConflictContract;
  }
  if (id == "cpp-tck.fom-transportation-description-ignored-contract") {
    return scenarioFomTransportationDescriptionIgnoredContract;
  }
  if (id == "cpp-tck.fom-update-rate-duplicate-equal-contract") {
    return scenarioFomUpdateRateDuplicateEqualContract;
  }
  if (id == "cpp-tck.fom-update-rate-duplicate-note-conflict-contract") {
    return scenarioFomUpdateRateDuplicateNoteConflictContract;
  }
  if (id == "cpp-tck.fom-switch-duplicate-equal-contract") {
    return scenarioFomSwitchDuplicateEqualContract;
  }
  if (id == "cpp-tck.fom-switch-duplicate-order-contract") {
    return scenarioFomSwitchDuplicateOrderContract;
  }
  if (id == "cpp-tck.fom-switch-duplicate-omitted-contract") {
    return scenarioFomSwitchDuplicateOmittedContract;
  }
  if (id == "cpp-tck.fom-automatic-resign-action-setting-contract") {
    return scenarioFomAutomaticResignActionSettingContract;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-equal-contract") {
    return scenarioFomAdvisorySwitchDuplicateEqualContract;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-order-contract") {
    return scenarioFomAdvisorySwitchDuplicateOrderContract;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-omitted-contract") {
    return scenarioFomAdvisorySwitchDuplicateOmittedContract;
  }
  if (id == "cpp-tck.service-report-interaction-contract") {
    return scenarioServiceReportInteractionContract;
  }
  if (id == "cpp-tck.service-report-attribute-update-contract") {
    return scenarioServiceReportAttributeUpdateContract;
  }
  if (id == "cpp-tck.service-report-register-object-instance-contract") {
    return scenarioServiceReportRegisterObjectInstanceContract;
  }
  if (id == "cpp-tck.service-report-delete-object-instance-contract") {
    return scenarioServiceReportDeleteObjectInstanceContract;
  }
  if (id == "cpp-tck.service-report-request-attribute-value-update-contract") {
    return scenarioServiceReportRequestAttributeValueUpdateContract;
  }
  if (id == "cpp-tck.service-report-release-multiple-object-instance-names-contract") {
    return scenarioServiceReportReleaseMultipleObjectInstanceNamesContract;
  }
  if (id == "cpp-tck.service-report-release-object-instance-name-contract") {
    return scenarioServiceReportReleaseObjectInstanceNameContract;
  }
  if (id == "cpp-tck.service-report-reserve-object-instance-name-contract") {
    return scenarioServiceReportReserveObjectInstanceNameContract;
  }
  if (id == "cpp-tck.service-report-local-delete-object-instance-contract") {
    return scenarioServiceReportLocalDeleteObjectInstanceContract;
  }
  if (id == "cpp-tck.service-report-local-delete-object-instance-failure-contract") {
    return scenarioServiceReportLocalDeleteObjectInstanceFailureContract;
  }
  if (id == "cpp-tck.service-report-delete-object-instance-failure-contract") {
    return scenarioServiceReportDeleteObjectInstanceFailureContract;
  }
  if (id == "cpp-tck.service-report-interaction-failure-contract") {
    return scenarioServiceReportInteractionFailureContract;
  }
  if (id == "cpp-tck.service-report-attribute-update-failure-contract") {
    return scenarioServiceReportAttributeUpdateFailureContract;
  }
  if (id == "cpp-tck.service-report-regional-interaction-failure-contract") {
    return scenarioServiceReportRegionalInteractionFailureContract;
  }
  if (id == "cpp-tck.service-report-timestamped-attribute-update-failure-contract") {
    return scenarioServiceReportTimestampedAttributeUpdateFailureContract;
  }
  if (id == "cpp-tck.service-report-timestamped-interaction-failure-contract") {
    return scenarioServiceReportTimestampedInteractionFailureContract;
  }
  if (id == "cpp-tck.service-report-timestamped-delete-object-instance-failure-contract") {
    return scenarioServiceReportTimestampedDeleteObjectInstanceFailureContract;
  }
  if (id == "cpp-tck.service-report-timestamped-interaction-contract") {
    return scenarioServiceReportTimestampedInteractionContract;
  }
  if (id == "cpp-tck.exception-hierarchy-contract") {
    return scenarioExceptionHierarchyContract;
  }
  if (id == "cpp-tck.exception-name-string") {
    return scenarioExceptionNameString;
  }
  if (id == "cpp-tck.standard-exception-boundaries-contract") {
    return scenarioStandardExceptionBoundariesContract;
  }
  if (id == "cpp-tck.federate-application-multiple-executions-contract") {
    return scenarioFederateApplicationMultipleExecutionsContract;
  }
  if (id == "cpp-tck.post-resignation-service-boundaries-contract") {
    return scenarioPostResignationServiceBoundariesContract;
  }
  if (id == "cpp-tck.enum-contract") {
    return scenarioEnumContract;
  }
  if (id == "cpp-tck.handle-and-collection-contract") {
    return scenarioHandleAndCollectionContract;
  }
  if (id == "cpp-tck.configuration-and-authorization-contract") {
    return scenarioConfigurationAndAuthorizationContract;
  }
  if (id == authorizerConnectServiceDecisionScenario) {
    return scenarioAuthorizerConnectServiceDecision;
  }
  if (id == authorizerUnrecognizedCredentialsUnauthorizedScenario) {
    return scenarioAuthorizerUnrecognizedCredentialsUnauthorized;
  }
  if (id == authorizerDisabledServiceRejectsCredentialsScenario) {
    return scenarioAuthorizerDisabledServiceRejectsCredentials;
  }
  if (id == authorizerFederationServiceDecisionScenario) {
    return scenarioAuthorizerFederationServiceDecision;
  }
  if (id == authorizerDestroyServiceDecisionScenario) {
    return scenarioAuthorizerDestroyServiceDecision;
  }
  if (id == authorizerJoinServiceDecisionScenario) {
    return scenarioAuthorizerJoinServiceDecision;
  }
  if (id == authorizerNoCredentialsConnectScenario) {
    return scenarioAuthorizerNoCredentialsConnect;
  }
  if (id == authorizerRidConfigurationDecisionScenario) {
    return scenarioAuthorizerRidConfigurationDecision;
  }
  if (id == authorizerSingleServicePerRidProfileScenario) {
    return scenarioAuthorizerSingleServicePerRidProfile;
  }
  if (id == "cpp-tck.authorizer-factory-factory-contract") {
    return scenarioAuthorizerFactoryFactoryContract;
  }
  if (id == "cpp-tck.runtime-identity-contract") {
    return scenarioRuntimeIdentityContract;
  }
  if (id == "cpp-tck.null-federate-ambassador-contract") {
    return scenarioNullFederateAmbassadorContract;
  }
  if (id == "cpp-tck.data-element-contract") {
    return scenarioDataElementContract;
  }
  if (id == "java-tck.overloads-and-exceptions") return scenarioConnection;
  if (id == "java-tck.encoder-round-trip") return scenarioEncoderRoundTrip;
  if (id == "cpp-tck.basic-data-elements-contract") {
    return scenarioBasicDataElementsContract;
  }
  if (id == basicDataElementEncodeDecodeContractScenario) {
    return scenarioBasicDataElementEncodeDecodeContract;
  }
  if (id == basicDataElementValueOperationsContractScenario) {
    return scenarioBasicDataElementValueOperationsContract;
  }
  if (id == complexDataElementConstituentAccessScenario) {
    return scenarioComplexDataElementConstituentAccess;
  }
  if (id == "cpp-tck.composite-data-elements-contract") {
    return scenarioCompositeDataElementsContract;
  }
  if (id == fixedRecordAlignmentScenario) {
    return scenarioFixedRecordAlignment;
  }
  if (id == variantRecordDiscriminantPaddingScenario) {
    return scenarioVariantRecordDiscriminantPadding;
  }
  if (id == variantRecordUnmappedDiscriminantScenario) {
    return scenarioVariantRecordUnmappedDiscriminant;
  }
  if (id == extendableVariantRecordLengthPaddingScenario) {
    return scenarioExtendableVariantRecordLengthPadding;
  }
  if (id == extendableVariantRecordAlternativePaddingScenario) {
    return scenarioExtendableVariantRecordAlternativePadding;
  }
  if (id == extendableVariantRecordUnmappedDiscriminantScenario) {
    return scenarioExtendableVariantRecordUnmappedDiscriminant;
  }
  if (id == extendableVariantRecordRejectsBoundaryOverEightScenario) {
    return scenarioExtendableVariantRecordRejectsBoundaryOverEight;
  }
  if (id == fixedArrayElementSequenceScenario) {
    return scenarioFixedArrayElementSequence;
  }
  if (id == variableArrayElementSequenceScenario) {
    return scenarioVariableArrayElementSequence;
  }
  if (id == variableArrayFirstElementPaddingScenario) {
    return scenarioVariableArrayFirstElementPadding;
  }
  if (id == variableArrayInterElementPaddingScenario) {
    return scenarioVariableArrayInterElementPadding;
  }
  if (id == "java-tck.malformed-inputs") return scenarioMalformedInputs;
  if (id == "cpp-tck.malformed-inputs-contract") {
    return scenarioMalformedInputsContract;
  }
  if (id == "java-tck.federation-membership") return scenarioFederationLifecycle;
  if (id == "cpp-tck.application-multiple-federates-across-executions") {
    return scenarioApplicationMultipleFederatesAcrossExecutions;
  }
  if (id == "cpp-tck.application-multiple-federates-across-executions-contract") {
    return scenarioApplicationMultipleFederatesAcrossExecutionsContract;
  }
  if (id == "cpp-tck.unnamed-join-overload") return scenarioUnnamedJoinOverload;
  if (id == "cpp-tck.unnamed-join-overload-contract") {
    return scenarioUnnamedJoinOverloadContract;
  }
  if (id == "cpp-tck.federation-list-services") return scenarioFederationListServices;
  if (id == "cpp-tck.federation-list-services-contract") {
    return scenarioFederationListServicesContract;
  }
  if (id == "cpp-tck.federate-lookup-lifecycle") return scenarioFederateLookupLifecycle;
  if (id == "cpp-tck.federate-lookup-lifecycle-contract") {
    return scenarioFederateLookupLifecycleContract;
  }
  if (id == "cpp-tck.explicit-mim-creation") return scenarioExplicitMimCreation;
  if (id == "cpp-tck.explicit-mim-creation-contract") {
    return scenarioExplicitMimCreationContract;
  }
  if (id == "cpp-tck.federation-mom-current-fdd") {
    return scenarioFederationMomCurrentFdd;
  }
  if (id == "cpp-tck.federation-mom-current-fdd-contract") {
    return scenarioFederationMomCurrentFddContract;
  }
  if (id == "cpp-tck.fom-service-usage-merge") {
    return scenarioFomServiceUsageMerge;
  }
  if (id == "cpp-tck.fom-service-usage-merge-contract") {
    return scenarioFomServiceUsageMergeContract;
  }
  if (id == "cpp-tck.fom-note-collision-remap") {
    return scenarioFomNoteCollisionRemap;
  }
  if (id == "cpp-tck.fom-note-collision-remap-contract") {
    return scenarioFomNoteCollisionRemapContract;
  }
  if (id == "cpp-tck.federation-mom-fom-module-designator-list") {
    return scenarioFederationMomFomModuleDesignatorList;
  }
  if (id == "cpp-tck.federation-mom-fom-module-designator-list-contract") {
    return scenarioFederationMomFomModuleDesignatorListContract;
  }
  if (id == "cpp-tck.federation-mom-content-reports") {
    return scenarioFederationMomContentReports;
  }
  if (id == "cpp-tck.federation-mom-content-reports-contract") {
    return scenarioFederationMomContentReportsContract;
  }
  if (id == "cpp-tck.federation-mom-s-leaf-subscription") {
    return scenarioFederationMomSLeafSubscription;
  }
  if (id == "cpp-tck.federation-mom-s-leaf-subscription-contract") {
    return scenarioFederationMomSLeafSubscriptionContract;
  }
  if (id == "cpp-tck.federate-mom-set-switches-extra-parameter") {
    return scenarioFederateMomSetSwitchesExtraParameter;
  }
  if (id == "cpp-tck.federate-mom-category-c-interaction") {
    return scenarioFederateMomCategoryCInteraction;
  }
  if (id == "cpp-tck.federate-mom-report-added-subclass-not-sent") {
    return scenarioFederateMomReportAddedSubclassNotSent;
  }
  if (id == "cpp-tck.federate-mom-report-added-subclass-full-interaction") {
    return scenarioFederateMomReportAddedSubclassFullInteraction;
  }
  if (id == "cpp-tck.federate-mom-report-added-subclass-parent-promotion-drops-extension-parameter") {
    return scenarioFederateMomReportAddedSubclassPromotionDropsExtensionParameter;
  }
  if (id == "cpp-tck.service-report-invocation-extension-parameter-not-sent") {
    return scenarioServiceReportOmitsFomAddedParameter;
  }
  if (id == "cpp-tck.federation-mom-synchronization-queries") {
    return scenarioFederationSynchronizationMomQueries;
  }
  if (id == "cpp-tck.federation-mom-synchronization-queries-contract") {
    return scenarioFederationSynchronizationMomQueriesContract;
  }
  if (id == "cpp-tck.service-report-order-transportation-lookups") {
    return scenarioServiceReportOrderTransportationLookups;
  }
  if (id == "cpp-tck.service-report-order-transportation-lookups-contract") {
    return scenarioServiceReportOrderTransportationLookupsContract;
  }
  if (id == "cpp-tck.service-report-dimension-upper-bound") {
    return scenarioServiceReportDimensionUpperBound;
  }
  if (id == "cpp-tck.service-report-dimension-upper-bound-contract") {
    return scenarioServiceReportDimensionUpperBoundContract;
  }
  if (id == "cpp-tck.service-report-dimension-name") {
    return scenarioServiceReportDimensionName;
  }
  if (id == "cpp-tck.service-report-dimension-name-contract") {
    return scenarioServiceReportDimensionNameContract;
  }
  if (id == "cpp-tck.service-report-dimension-handle") {
    return scenarioServiceReportDimensionHandle;
  }
  if (id == "cpp-tck.service-report-dimension-handle-contract") {
    return scenarioServiceReportDimensionHandleContract;
  }
  if (id == "cpp-tck.service-report-interaction-dimensions") {
    return scenarioServiceReportInteractionDimensions;
  }
  if (id == "cpp-tck.service-report-interaction-dimensions-contract") {
    return scenarioServiceReportInteractionDimensionsContract;
  }
  if (id == "cpp-tck.service-report-object-dimensions") {
    return scenarioServiceReportObjectDimensions;
  }
  if (id == "cpp-tck.service-report-object-dimensions-contract") {
    return scenarioServiceReportObjectDimensionsContract;
  }
  if (id == "cpp-tck.service-report-order-transportation-lookup-failures") {
    return scenarioServiceReportOrderTransportationLookupFailures;
  }
  if (id == "cpp-tck.service-report-order-transportation-lookup-failures-contract") {
    return scenarioServiceReportOrderTransportationLookupFailuresContract;
  }
  if (id == "cpp-tck.service-report-federate-object-class-lookups") {
    return scenarioServiceReportFederateObjectClassLookups;
  }
  if (id == "cpp-tck.service-report-federate-object-class-lookups-contract") {
    return scenarioServiceReportFederateObjectClassLookupsContract;
  }
  if (id == "cpp-tck.service-report-federate-object-class-lookup-failures") {
    return scenarioServiceReportFederateObjectClassLookupFailures;
  }
  if (id == "cpp-tck.service-report-federate-object-class-lookup-failures-contract") {
    return scenarioServiceReportFederateObjectClassLookupFailuresContract;
  }
  if (id == "cpp-tck.service-report-interaction-parameter-lookups") {
    return scenarioServiceReportInteractionParameterLookups;
  }
  if (id == "cpp-tck.service-report-interaction-parameter-lookups-contract") {
    return scenarioServiceReportInteractionParameterLookupsContract;
  }
  if (id == "cpp-tck.service-report-interaction-parameter-lookup-failures") {
    return scenarioServiceReportInteractionParameterLookupFailures;
  }
  if (id == "cpp-tck.service-report-interaction-parameter-lookup-failures-contract") {
    return scenarioServiceReportInteractionParameterLookupFailuresContract;
  }
  if (id == "cpp-tck.service-report-object-attribute-update-rate-lookups") {
    return scenarioServiceReportObjectAttributeUpdateRateLookups;
  }
  if (id == "cpp-tck.service-report-object-attribute-update-rate-lookups-contract") {
    return scenarioServiceReportObjectAttributeUpdateRateLookupsContract;
  }
  if (id == "cpp-tck.service-report-object-attribute-update-rate-lookup-failures") {
    return scenarioServiceReportObjectAttributeUpdateRateLookupFailures;
  }
  if (id == "cpp-tck.service-report-object-attribute-update-rate-lookup-failures-contract") {
    return scenarioServiceReportObjectAttributeUpdateRateLookupFailuresContract;
  }
  if (id == "cpp-tck.federation-mom-save-conditionals-contract") {
    return scenarioFederationMomSaveConditionalsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-federate-state-save-restore-contract") {
    return scenarioJoinedFederateMomFederateStateSaveRestoreContract;
  }
  if (id == "cpp-tck.joined-federate-mom-galt-lits-periodic-contract") {
    return scenarioJoinedFederateMomGaltLitsPeriodicContract;
  }
  if (id == "cpp-tck.joined-federate-mom-tso-length-periodic-contract") {
    return scenarioJoinedFederateMomTsoLengthPeriodicContract;
  }
  if (id == "cpp-tck.joined-federate-mom-removed-object-count-contract") {
    return scenarioJoinedFederateMomRemovedObjectCountContract;
  }
  if (id == "cpp-tck.joined-federate-mom-updates-sent-counts-contract") {
    return scenarioJoinedFederateMomUpdatesSentCountsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-interactions-received-counts-contract") {
    return scenarioJoinedFederateMomInteractionsReceivedCountsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-directed-interactions-received-counts-contract") {
    return scenarioJoinedFederateMomDirectedInteractionsReceivedCountsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-interactions-sent-counts-contract") {
    return scenarioJoinedFederateMomInteractionsSentCountsContract;
  }
  if (id == "cpp-tck.service-report-interaction") {
    return scenarioServiceReportInteraction;
  }
  if (id == "cpp-tck.service-report-receive-order-interaction") {
    return scenarioServiceReportReceiveOrderInteraction;
  }
  if (id == "cpp-tck.service-report-receive-order-interaction-contract") {
    return scenarioServiceReportReceiveOrderInteractionContract;
  }
  if (id == "cpp-tck.service-report-timestamped-directed-interaction") {
    return scenarioServiceReportTimestampedDirectedInteraction;
  }
  if (id == "cpp-tck.service-report-timestamped-directed-interaction-contract") {
    return scenarioServiceReportTimestampedDirectedInteractionContract;
  }
  if (id == "cpp-tck.service-report-timestamped-attribute-update") {
    return scenarioServiceReportTimestampedAttributeUpdate;
  }
  if (id == momTableAnnexGPrecedenceServiceReportFileScenario) {
    return scenarioMomTableAnnexGPrecedenceServiceReportFile;
  }
  if (id == serviceReportFileSwitchLifecycleScenario) {
    return scenarioServiceReportFileSwitchLifecycle;
  }
  if (id == "cpp-tck.service-report-timestamped-attribute-update-contract") {
    return scenarioServiceReportTimestampedAttributeUpdateContract;
  }
  if (id == "cpp-tck.service-report-timestamped-delete-object-instance") {
    return scenarioServiceReportTimestampedDeleteObjectInstance;
  }
  if (id == "cpp-tck.service-report-timestamped-delete-object-instance-contract") {
    return scenarioServiceReportTimestampedDeleteObjectInstanceContract;
  }
  if (id == "cpp-tck.service-report-interlock") {
    return scenarioServiceReportInterlock;
  }
  if (id == "cpp-tck.service-report-interlock-contract") {
    return scenarioServiceReportInterlockContract;
  }
  if (id == "cpp-tck.service-report-synchronization") {
    return scenarioServiceReportSynchronization;
  }
  if (id == "cpp-tck.service-report-synchronization-contract") {
    return scenarioServiceReportSynchronizationContract;
  }
  if (id == "cpp-tck.service-report-federate-save-begun") {
    return scenarioServiceReportFederateSaveBegun;
  }
  if (id == "cpp-tck.service-report-federate-save-begun-contract") {
    return scenarioServiceReportFederateSaveBegunContract;
  }
  if (id == "cpp-tck.service-report-save-failure-reason-file") {
    return scenarioServiceReportSaveFailureReasonFile;
  }
  if (id == "cpp-tck.service-report-save-failure-reason-file-contract") {
    return scenarioServiceReportSaveFailureReasonFileContract;
  }
  if (id == "cpp-tck.service-report-save-status-file") {
    return scenarioServiceReportSaveStatusFile;
  }
  if (id == "cpp-tck.service-report-save-status-file-contract") {
    return scenarioServiceReportSaveStatusFileContract;
  }
  if (id == "cpp-tck.service-report-service-group-file") {
    return scenarioServiceReportServiceGroupFile;
  }
  if (id == "cpp-tck.service-report-service-group-file-contract") {
    return scenarioServiceReportServiceGroupFileContract;
  }
  if (id == "cpp-tck.service-report-federation-execution-information-file") {
    return scenarioServiceReportFederationExecutionInformationFile;
  }
  if (id == "cpp-tck.service-report-federation-execution-information-file-contract") {
    return scenarioServiceReportFederationExecutionInformationFileContract;
  }
  if (id == "cpp-tck.service-report-federation-execution-members-file") {
    return scenarioServiceReportFederationExecutionMembersFile;
  }
  if (id == "cpp-tck.service-report-federation-execution-members-file-contract") {
    return scenarioServiceReportFederationExecutionMembersFileContract;
  }
  if (id == "cpp-tck.service-report-interaction-failure") {
    return scenarioServiceReportInteractionFailure;
  }
  if (id == "cpp-tck.service-report-regional-interaction") {
    return scenarioServiceReportRegionalInteraction;
  }
  if (id == "cpp-tck.service-report-regional-interaction-contract") {
    return scenarioServiceReportRegionalInteractionContract;
  }
  if (id == "cpp-tck.service-report-regional-interaction-subscription") {
    return scenarioServiceReportRegionalInteractionSubscription;
  }
  if (id == "cpp-tck.service-report-object-attribute-declaration") {
    return scenarioServiceReportObjectAttributeDeclaration;
  }
  if (id == "cpp-tck.service-report-directed-interaction-declaration") {
    return scenarioServiceReportDirectedInteractionDeclaration;
  }
  if (id == "cpp-tck.service-report-directed-interaction-publication") {
    return scenarioServiceReportDirectedInteractionPublication;
  }
  if (id == "cpp-tck.service-report-interaction-publication") {
    return scenarioServiceReportInteractionPublication;
  }
  if (id == "cpp-tck.service-report-interaction-subscription") {
    return scenarioServiceReportInteractionSubscription;
  }
  if (id == "cpp-tck.service-report-next-message-request-available-boundary") {
    return scenarioServiceReportNextMessageRequestAvailableBoundary;
  }
  if (id == "cpp-tck.service-report-query-galt-return-value") {
    return scenarioServiceReportQueryGalt;
  }
  if (id == "cpp-tck.service-report-regional-interaction-subscription-contract") {
    return scenarioServiceReportRegionalInteractionSubscriptionContract;
  }
  if (id == "cpp-tck.service-report-object-attribute-declaration-contract") {
    return scenarioServiceReportObjectAttributeDeclarationContract;
  }
  if (id == "cpp-tck.service-report-directed-interaction-declaration-contract") {
    return scenarioServiceReportDirectedInteractionDeclarationContract;
  }
  if (id == "cpp-tck.service-report-directed-interaction-publication-contract") {
    return scenarioServiceReportDirectedInteractionPublicationContract;
  }
  if (id == "cpp-tck.service-report-interaction-publication-contract") {
    return scenarioServiceReportInteractionPublicationContract;
  }
  if (id == "cpp-tck.service-report-interaction-subscription-contract") {
    return scenarioServiceReportInteractionSubscriptionContract;
  }
  if (id == "cpp-tck.service-report-regional-interaction-failure") {
    return scenarioServiceReportRegionalInteractionFailure;
  }
  if (id == "cpp-tck.service-report-attribute-update") {
    return scenarioServiceReportAttributeUpdate;
  }
  if (id == "cpp-tck.service-report-attribute-update-failure") {
    return scenarioServiceReportAttributeUpdateFailure;
  }
  if (id == "cpp-tck.service-report-request-attribute-value-update") {
    return scenarioServiceReportRequestAttributeValueUpdate;
  }
  if (id == "cpp-tck.service-report-release-multiple-object-instance-names") {
    return scenarioServiceReportReleaseMultipleObjectInstanceNames;
  }
  if (id == "cpp-tck.service-report-release-object-instance-name") {
    return scenarioServiceReportReleaseObjectInstanceName;
  }
  if (id == "cpp-tck.service-report-reserve-object-instance-name") {
    return scenarioServiceReportReserveObjectInstanceName;
  }
  if (id == "cpp-tck.service-report-register-object-instance") {
    return scenarioServiceReportRegisterObjectInstance;
  }
  if (id == "cpp-tck.service-report-local-delete-object-instance") {
    return scenarioServiceReportLocalDeleteObjectInstance;
  }
  if (id == "cpp-tck.service-report-local-delete-object-instance-failure") {
    return scenarioServiceReportLocalDeleteObjectInstanceFailure;
  }
  if (id == "cpp-tck.service-report-delete-object-instance") {
    return scenarioServiceReportDeleteObjectInstance;
  }
  if (id == "cpp-tck.service-report-delete-object-instance-failure") {
    return scenarioServiceReportDeleteObjectInstanceFailure;
  }
  if (id == "cpp-tck.service-report-timestamped-interaction") {
    return scenarioServiceReportTimestampedInteraction;
  }
  if (id == "cpp-tck.service-report-timestamped-attribute-update-failure") {
    return scenarioServiceReportTimestampedAttributeUpdateFailure;
  }
  if (id == "cpp-tck.service-report-timestamped-interaction-failure") {
    return scenarioServiceReportTimestampedInteractionFailure;
  }
  if (id == "cpp-tck.service-report-timestamped-delete-object-instance-failure") {
    return scenarioServiceReportTimestampedDeleteObjectInstanceFailure;
  }
  if (id == "java-tck.logical-time-factory") return scenarioLogicalTimeFactory;
  if (id == "java-tck.time-advance") return scenarioTimeAdvance;
  if (id == "cpp-tck.time-advance-contract") {
    return scenarioTimeAdvanceContract;
  }
  if (id == "java-tck.support-services") return scenarioHandleLookups;
  if (id == "cpp-tck.standard-order-and-transportation-lookups") {
    return scenarioStandardOrderAndTransportationLookups;
  }
  if (id == "cpp-tck.standard-order-and-transportation-lookups-contract") {
    return scenarioStandardOrderAndTransportationLookupsContract;
  }
  if (id == "cpp-tck.fom-name-case-sensitivity") {
    return scenarioFomNameCaseSensitivity;
  }
  if (id == "cpp-tck.fom-name-case-sensitivity-contract") {
    return scenarioFomNameCaseSensitivityContract;
  }
  if (id == "cpp-tck.fom-handle-uniqueness") {
    return scenarioFomHandleUniqueness;
  }
  if (id == "cpp-tck.standard-exception-boundaries") {
    return scenarioStandardExceptionBoundaries;
  }
  if (id == "cpp-tck.federate-application-multiple-executions") {
    return scenarioFederateApplicationMultipleExecutions;
  }
  if (id == "cpp-tck.federate-save-initiation-eligibility") {
    return scenarioFederateSaveInitiationEligibility;
  }
  if (id == "cpp-tck.federate-save-initiation-time-constrained-eligibility") {
    return scenarioFederateSaveInitiationTimeConstrainedEligibility;
  }
  if (id == "cpp-tck.post-resignation-service-boundaries") {
    return scenarioPostResignationServiceBoundaries;
  }
  if (id == "java-tck.declaration-management") return scenarioDeclarations;
  if (id == "java-tck.object-management") return scenarioObjectManagement;
  if (id == "cpp-tck.attribute-value-update-request-baseline") {
    return scenarioAttributeValueUpdateRequestBaseline;
  }
  if (id == "cpp-tck.federation-visible-object-state-completeness") {
    return scenarioFederationVisibleObjectStateCompleteness;
  }
  if (id == "cpp-tck.attribute-value-update-request-baseline-contract") {
    return scenarioAttributeValueUpdateRequestBaselineContract;
  }
  if (id == "cpp-tck.object-class-attribute-value-update-request-baseline") {
    return scenarioObjectClassAttributeValueUpdateRequestBaseline;
  }
  if (id == "cpp-tck.object-class-attribute-value-update-request-baseline-contract") {
    return scenarioObjectClassAttributeValueUpdateRequestBaselineContract;
  }
  if (id == "cpp-tck.object-class-timestamped-provider-response") {
    return scenarioObjectClassTimestampedProviderResponse;
  }
  if (id == "cpp-tck.object-class-timestamped-provider-response-contract") {
    return scenarioObjectClassTimestampedProviderResponseContract;
  }
  if (id == "cpp-tck.attribute-value-update-response") {
    return scenarioAttributeValueUpdateResponse;
  }
  if (id == "cpp-tck.attribute-value-update-response-contract") {
    return scenarioAttributeValueUpdateResponseContract;
  }
  if (id == "cpp-tck.attribute-value-update-request-multi-requester") {
    return scenarioAttributeValueUpdateRequestMultiRequester;
  }
  if (id == "cpp-tck.attribute-value-update-request-multi-requester-contract") {
    return scenarioAttributeValueUpdateRequestMultiRequesterContract;
  }
  if (id == "cpp-tck.resign-delete-objects") return scenarioResignDeleteObjects;
  if (id == "cpp-tck.resign-delete-objects-contract") {
    return scenarioResignDeleteObjectsContract;
  }
  if (id == "cpp-tck.resign-unconditional-divestiture") {
    return scenarioResignUnconditionalDivestiture;
  }
  if (id == "cpp-tck.resign-unconditional-divestiture-contract") {
    return scenarioResignUnconditionalDivestitureContract;
  }
  if (id == "cpp-tck.object-name-reservation-lifecycle") {
    return scenarioObjectNameReservationLifecycle;
  }
  if (id == "cpp-tck.object-name-reservation-lifecycle-contract") {
    return scenarioObjectNameReservationLifecycleContract;
  }
  if (id == "cpp-tck.reserved-object-name-no-discovery") {
    return scenarioReservedObjectNameDoesNotDiscover;
  }
  if (id == "cpp-tck.final-federate-resignation-cleanup") {
    return scenarioFinalFederateResignationCleanup;
  }
  if (id == "cpp-tck.final-federate-resignation-cleanup-contract") {
    return scenarioFinalFederateResignationCleanupContract;
  }
  if (id == "cpp-tck.object-registration-discovery-lifecycle") {
    return scenarioObjectRegistrationDiscoveryLifecycle;
  }
  if (id == "cpp-tck.object-registration-discovery-lifecycle-contract") {
    return scenarioObjectRegistrationDiscoveryLifecycleContract;
  }
  if (id == "cpp-tck.overlapping-object-class-subscriptions-single-discovery") {
    return scenarioOverlappingObjectClassSubscriptionsSingleDiscovery;
  }
  if (id == "cpp-tck.overlapping-object-class-subscriptions-single-discovery-contract") {
    return scenarioOverlappingObjectClassSubscriptionsSingleDiscoveryContract;
  }
  if (id == objectClassAbstractionSubscriptionScenario) {
    return scenarioObjectClassAbstractionSubscription;
  }
  if (id == objectClassAbstractionSubscriptionContractScenario) {
    return scenarioObjectClassAbstractionSubscriptionContract;
  }
  if (id == "cpp-tck.per-federate-discovered-class") {
    return scenarioPerFederateDiscoveredClass;
  }
  if (id == "cpp-tck.per-federate-discovered-class-contract") {
    return scenarioPerFederateDiscoveredClassContract;
  }
  if (id == "cpp-tck.registered-object-class-immutability") {
    return scenarioRegisteredObjectClassImmutability;
  }
  if (id == "cpp-tck.registered-object-class-immutability-contract") {
    return scenarioRegisteredObjectClassImmutabilityContract;
  }
  if (id == "cpp-tck.discovered-class-stability-after-specific-subscription") {
    return scenarioDiscoveredClassStabilityAfterSpecificSubscription;
  }
  if (id == "cpp-tck.discovered-class-stability-after-specific-subscription-contract") {
    return scenarioDiscoveredClassStabilityAfterSpecificSubscriptionContract;
  }
  if (id == "cpp-tck.object-registration-discovery-multi-recipient") {
    return scenarioObjectRegistrationDiscoveryMultiRecipient;
  }
  if (id == "cpp-tck.object-registration-discovery-multi-recipient-contract") {
    return scenarioObjectRegistrationDiscoveryMultiRecipientContract;
  }
  if (id == "cpp-tck.active-subscription-object-discovery") {
    return scenarioActiveSubscriptionObjectDiscovery;
  }
  if (id == "cpp-tck.named-registration") {
    return scenarioNamedRegistration;
  }
  if (id == "cpp-tck.named-registration-contract") {
    return scenarioNamedRegistrationContract;
  }
  if (id == "cpp-tck.named-registration-multi-recipient") {
    return scenarioNamedRegistrationMultiRecipient;
  }
  if (id == "cpp-tck.named-registration-multi-recipient-contract") {
    return scenarioNamedRegistrationMultiRecipientContract;
  }
  if (id == "cpp-tck.local-delete-object-instance") {
    return scenarioLocalDeleteObjectInstance;
  }
  if (id == "cpp-tck.local-delete-object-instance-contract") {
    return scenarioLocalDeleteObjectInstanceContract;
  }
  if (id == "cpp-tck.object-attribute-subscription-lifecycle") {
    return scenarioObjectAttributeSubscriptionLifecycle;
  }
  if (id == "cpp-tck.object-attribute-subscription-lifecycle-contract") {
    return scenarioObjectAttributeSubscriptionLifecycleContract;
  }
  if (id == "cpp-tck.object-publication-registration-fence") {
    return scenarioObjectPublicationRegistrationFence;
  }
  if (id == "cpp-tck.object-publication-registration-fence-contract") {
    return scenarioObjectPublicationRegistrationFenceContract;
  }
  if (id == "cpp-tck.order-type-controls") return scenarioOrderTypeControls;
  if (id == "cpp-tck.order-type-controls-contract") {
    return scenarioOrderTypeControlsContract;
  }
  if (id == "cpp-tck.order-type-inherited-attribute-default-scope") {
    return scenarioInheritedAttributeDefaultOrderScope;
  }
  if (id == "cpp-tck.order-type-inherited-attribute-default-scope-contract") {
    return scenarioInheritedAttributeDefaultOrderScopeContract;
  }
  if (id == "cpp-tck.order-type-fdd-default-initialization") {
    return scenarioFddAttributeOrderDefaultInitialization;
  }
  if (id == "cpp-tck.order-type-fdd-default-initialization-contract") {
    return scenarioFddAttributeOrderDefaultInitializationContract;
  }
  if (id == "cpp-tck.order-type-default-order-class-handle-validation") {
    return scenarioDefaultOrderAttributeClassHandleValidation;
  }
  if (id == "cpp-tck.order-type-default-order-class-handle-validation-contract") {
    return scenarioDefaultOrderAttributeClassHandleValidationContract;
  }
  if (id == "cpp-tck.regional-multi-attribute-update") {
    return scenarioRegionalMultiAttributeUpdate;
  }
  if (id == "cpp-tck.regional-object-attribute-dimension-inheritance") {
    return scenarioRegionalObjectAttributeDimensionInheritance;
  }
  if (id == "cpp-tck.regional-object-attribute-dimension-inheritance-contract") {
    return scenarioRegionalObjectAttributeDimensionInheritanceContract;
  }
  if (id == "cpp-tck.regional-object-attribute-default-region-fdd-dimensions") {
    return scenarioRegionalObjectAttributeDefaultRegionFddDimensions;
  }
  if (id == "cpp-tck.regional-object-attribute-default-region-fdd-dimensions-contract") {
    return scenarioRegionalObjectAttributeDefaultRegionFddDimensionsContract;
  }
  if (id == "cpp-tck.regional-object-attribute-unspecified-dimension-default-range") {
    return scenarioRegionalObjectAttributeUnspecifiedDimensionDefaultRange;
  }
  if (id == "cpp-tck.regional-object-attribute-unspecified-dimension-default-range-contract") {
    return scenarioRegionalObjectAttributeUnspecifiedDimensionDefaultRangeContract;
  }
  if (id == "cpp-tck.regional-object-attribute-excluded-dimension-not-realized") {
    return scenarioRegionalObjectAttributeExcludedDimensionNotRealized;
  }
  if (id == "cpp-tck.regional-object-attribute-excluded-dimension-not-realized-contract") {
    return scenarioRegionalObjectAttributeExcludedDimensionNotRealizedContract;
  }
  if (id == "cpp-tck.regional-subscription-triple-isolation") {
    return scenarioRegionalSubscriptionTripleIsolation;
  }
  if (id == "cpp-tck.regional-subscription-triple-isolation-contract") {
    return scenarioRegionalSubscriptionTripleIsolationContract;
  }
  if (id == "cpp-tck.regional-three-dimensional-overlap") {
    return scenarioRegionalThreeDimensionalOverlap;
  }
  if (id == "java-tck.attribute-interaction") return scenarioAttributeInteraction;
  if (id == "cpp-tck.receive-order-attribute-update-callback-cancellation") {
    return scenarioReceiveOrderAttributeUpdateCallbackCancellation;
  }
  if (id == "cpp-tck.receive-order-attribute-update-callback-cancellation-contract") {
    return scenarioReceiveOrderAttributeUpdateCallbackCancellationContract;
  }
  if (id == "cpp-tck.receive-order-attribute-update-receiver-resignation-fence-contract") {
    return scenarioReceiveOrderAttributeUpdateReceiverResignationFenceContract;
  }
  if (id == "cpp-tck.receive-order-object-removal-subscription-withdrawal") {
    return scenarioReceiveOrderObjectRemovalSubscriptionWithdrawal;
  }
  if (id == "cpp-tck.receive-order-object-removal-subscription-withdrawal-contract") {
    return scenarioReceiveOrderObjectRemovalSubscriptionWithdrawalContract;
  }
  if (id == "cpp-tck.receive-order-interaction-callback-cancellation") {
    return scenarioReceiveOrderInteractionCallbackCancellation;
  }
  if (id == "cpp-tck.receive-order-interaction-callback-cancellation-contract") {
    return scenarioReceiveOrderInteractionCallbackCancellationContract;
  }
  if (id == "cpp-tck.interaction-subscription-lifecycle") {
    return scenarioInteractionSubscriptionLifecycle;
  }
  if (id == "cpp-tck.interaction-subscription-lifecycle-contract") {
    return scenarioInteractionSubscriptionLifecycleContract;
  }
  if (id == "cpp-tck.interaction-publication-send-fence") {
    return scenarioInteractionPublicationSendFence;
  }
  if (id == "cpp-tck.interaction-publication-send-fence-contract") {
    return scenarioInteractionPublicationSendFenceContract;
  }
  if (id == "cpp-tck.interaction-multi-recipient-fifo") {
    return scenarioInteractionMultiRecipientFifo;
  }
  if (id == "cpp-tck.interaction-multi-recipient-fifo-contract") {
    return scenarioInteractionMultiRecipientFifoContract;
  }
  if (id == "cpp-tck.attribute-multi-recipient-fifo") {
    return scenarioAttributeMultiRecipientFifo;
  }
  if (id == "cpp-tck.attribute-multi-recipient-fifo-contract") {
    return scenarioAttributeMultiRecipientFifoContract;
  }
  if (id == "cpp-tck.ordinary-multi-attribute-subscription-projection") {
    return scenarioOrdinaryMultiAttributeSubscriptionProjection;
  }
  if (id == "cpp-tck.ordinary-multi-attribute-subscription-projection-contract") {
    return scenarioOrdinaryMultiAttributeSubscriptionProjectionContract;
  }
  if (id == "cpp-tck.attribute-subscription-mode-exclusivity") {
    return scenarioAttributeSubscriptionModeExclusivity;
  }
  if (id == "cpp-tck.attribute-subscription-mode-exclusivity-contract") {
    return scenarioAttributeSubscriptionModeExclusivityContract;
  }
  if (id == "cpp-tck.subscription-initial-reflection-projection") {
    return scenarioSubscriptionInitialReflectionProjection;
  }
  if (id == "cpp-tck.subscription-initial-reflection-projection-contract") {
    return scenarioSubscriptionInitialReflectionProjectionContract;
  }
  if (id == "cpp-tck.ordinary-multi-attribute-value-update-request-response") {
    return scenarioOrdinaryMultiAttributeValueUpdateRequestResponse;
  }
  if (id == "cpp-tck.ordinary-multi-attribute-value-update-request-response-contract") {
    return scenarioOrdinaryMultiAttributeValueUpdateRequestResponseContract;
  }
  if (id == "cpp-tck.receive-order-multi-attribute-update-callback-cancellation") {
    return scenarioReceiveOrderMultiAttributeUpdateCallbackCancellation;
  }
  if (id == "cpp-tck.receive-order-multi-attribute-update-callback-cancellation-contract") {
    return scenarioReceiveOrderMultiAttributeUpdateCallbackCancellationContract;
  }
  if (id == "cpp-tck.object-removal-multi-recipient-fifo") {
    return scenarioObjectRemovalMultiRecipientFifo;
  }
  if (id == "cpp-tck.object-removal-multi-recipient-fifo-contract") {
    return scenarioObjectRemovalMultiRecipientFifoContract;
  }
  if (id == "cpp-tck.resign-delete-objects-multi-recipient-fifo") {
    return scenarioResignDeleteObjectsMultiRecipientFifo;
  }
  if (id == "cpp-tck.resign-delete-objects-multi-recipient-fifo-contract") {
    return scenarioResignDeleteObjectsMultiRecipientFifoContract;
  }
  if (id == "java-tck.directed-interactions") return scenarioDirectedInteractions;
  if (id == "cpp-tck.directed-interaction-multi-recipient-fifo") {
    return scenarioDirectedInteractionMultiRecipientFifo;
  }
  if (id == "cpp-tck.directed-interaction-multi-recipient-fifo-contract") {
    return scenarioDirectedInteractionMultiRecipientFifoContract;
  }
  if (id == "cpp-tck.directed-interaction-mixed-subscription-fanout") {
    return scenarioDirectedInteractionMixedSubscriptionFanout;
  }
  if (id == "cpp-tck.directed-interaction-mixed-subscription-fanout-contract") {
    return scenarioDirectedInteractionMixedSubscriptionFanoutContract;
  }
  if (id == "cpp-tck.directed-interaction-publication-send-fence") {
    return scenarioDirectedInteractionPublicationSendFence;
  }
  if (id == "cpp-tck.directed-interaction-publication-send-fence-contract") {
    return scenarioDirectedInteractionPublicationSendFenceContract;
  }
  if (id == "cpp-tck.directed-interaction-target-lifecycle") {
    return scenarioDirectedInteractionTargetLifecycle;
  }
  if (id == "cpp-tck.directed-interaction-target-lifecycle-contract") {
    return scenarioDirectedInteractionTargetLifecycleContract;
  }
  if (id == "cpp-tck.directed-interaction-subscription-kind") {
    return scenarioDirectedInteractionSubscriptionKind;
  }
  if (id == "cpp-tck.directed-interaction-subscription-kind-contract") {
    return scenarioDirectedInteractionSubscriptionKindContract;
  }
  if (id == "cpp-tck.timestamped-interactions") return scenarioTimestampedInteractions;
  if (id == "cpp-tck.timestamped-interactions-contract") {
    return scenarioTimestampedInteractionsContract;
  }
  if (id == "cpp-tck.timestamped-interaction-preferred-receive-conversion") {
    return scenarioTimestampedInteractionPreferredReceiveConversion;
  }
  if (id == "cpp-tck.timestamped-interaction-preferred-receive-conversion-contract") {
    return scenarioTimestampedInteractionPreferredReceiveConversionContract;
  }
  if (id == "cpp-tck.timestamped-interaction-sender-regulation-transition") {
    return scenarioTimestampedInteractionSenderRegulationTransition;
  }
  if (id == "cpp-tck.timestamped-interaction-sender-regulation-transition-contract") {
    return scenarioTimestampedInteractionSenderRegulationTransitionContract;
  }
  if (id == "cpp-tck.interaction-fdd-preferred-order-default") {
    return scenarioInteractionFddPreferredOrderDefault;
  }
  if (id == "cpp-tck.interaction-fdd-preferred-order-default-contract") {
    return scenarioInteractionFddPreferredOrderDefaultContract;
  }
  if (id == "cpp-tck.attribute-fdd-preferred-order-default") {
    return scenarioAttributeFddPreferredOrderDefault;
  }
  if (id == "cpp-tck.attribute-fdd-preferred-order-default-contract") {
    return scenarioAttributeFddPreferredOrderDefaultContract;
  }
  if (id == "cpp-tck.timestamped-delete-privilege-preferred-order") {
    return scenarioTimestampedDeletePrivilegePreferredOrder;
  }
  if (id == "cpp-tck.timestamped-delete-privilege-preferred-order-contract") {
    return scenarioTimestampedDeletePrivilegePreferredOrderContract;
  }
  if (id == "cpp-tck.timestamped-delete-privilege-timestamp-order") {
    return scenarioTimestampedDeletePrivilegeTimestampOrder;
  }
  if (id == "cpp-tck.timestamped-delete-privilege-timestamp-order-contract") {
    return scenarioTimestampedDeletePrivilegeTimestampOrderContract;
  }
  if (id == "cpp-tck.sent-tso-per-recipient-order-conversion") {
    return scenarioSentTsoPerRecipientOrderConversion;
  }
  if (id == "cpp-tck.sent-tso-per-recipient-order-conversion-contract") {
    return scenarioSentTsoPerRecipientOrderConversionContract;
  }
  if (id == "cpp-tck.sent-tso-attribute-per-recipient-order-conversion") {
    return scenarioSentTsoAttributePerRecipientOrderConversion;
  }
  if (id == "cpp-tck.sent-tso-attribute-per-recipient-order-conversion-contract") {
    return scenarioSentTsoAttributePerRecipientOrderConversionContract;
  }
  if (id == "cpp-tck.timestamped-interaction-source-resignation-contract") {
    return scenarioTimestampedInteractionSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-interaction-source-resignation-fanout-contract") {
    return scenarioTimestampedInteractionSourceResignationFanoutContract;
  }
  if (id == "cpp-tck.timestamped-interaction-source-resignation") {
    return scenarioTimestampedInteractionSourceResignation;
  }
  if (id == "cpp-tck.timestamped-interaction-source-resignation-fanout") {
    return scenarioTimestampedInteractionSourceResignationFanout;
  }
  if (id == "cpp-tck.timestamped-interaction-mixed-advances-contract") {
    return scenarioTimestampedInteractionMixedAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tara-equal-boundary-contract") {
    return scenarioTimestampedInteractionTaraEqualBoundaryContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-equal-boundary-contract") {
    return scenarioTimestampedInteractionTarEqualBoundaryContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-inclusive-target-delivery-contract") {
    return scenarioTimestampedInteractionTarInclusiveTargetDeliveryContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-strict-timestamp-cutoff-contract") {
    return scenarioTimestampedInteractionTarStrictTimestampCutoffContract;
  }
  if (id == "cpp-tck.timestamped-interaction-zero-lookahead-tar-send-boundary-contract") {
    return scenarioTimestampedInteractionZeroLookaheadTarSendBoundaryContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-post-grant-cutoff-contract") {
    return scenarioTimestampedInteractionTarPostGrantCutoffContract;
  }
  if (id == "cpp-tck.timestamped-interaction-flush-queue-future-input-contract") {
    return scenarioTimestampedInteractionFlushQueueFutureInputContract;
  }
  if (id == "cpp-tck.fqr-postgrant-lower-tso-retraction-contract") {
    return scenarioFqrPostgrantLowerTsoRetractionContract;
  }
  if (id == "cpp-tck.fqg-postgrant-delivery-cutoff-contract") {
    return scenarioFqgPostgrantDeliveryCutoffContract;
  }
  if (id == "cpp-tck.tar-queued-tso-before-grant-contract") {
    return scenarioTarQueuedTsoBeforeGrantContract;
  }
  if (id == "cpp-tck.timestamped-interaction-tso-designator-terminalization-contract") {
    return scenarioTimestampedInteractionTsoDesignatorTerminalizationContract;
  }
  if (id == "cpp-tck.timestamped-interaction-cross-producer-order-contract") {
    return scenarioTimestampedInteractionCrossProducerOrderContract;
  }
  if (id == "cpp-tck.timestamped-interaction-no-fanout-contract") {
    return scenarioTimestampedInteractionNoFanoutContract;
  }
  if (id == "cpp-tck.timestamped-interaction-retraction-fanout-contract") {
    return scenarioTimestampedInteractionRetractionFanoutContract;
  }
  if (id == "cpp-tck.timestamped-interaction-regulation-reenable-contract") {
    return scenarioTimestampedInteractionRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-interaction-reenable-contract") {
    return scenarioTimestampedInteractionReenableContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-reenable-contract") {
    return scenarioTimestampedDirectedInteractionReenableContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-regulation-reenable-contract") {
    return scenarioTimestampedDirectedInteractionRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-attribute-order-cohort-contract") {
    return scenarioTimestampedAttributeOrderCohortContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-queued-passel-retraction-contract") {
    return scenarioTimestampedAttributeUpdateQueuedPasselRetractionContract;
  }
  if (id == "cpp-tck.mixed-order-attribute-passel-retraction-contract") {
    return scenarioMixedOrderAttributePasselRetractionContract;
  }
  if (id == "cpp-tck.mixed-order-attribute-passels-separate-messages-contract") {
    return scenarioMixedOrderAttributePasselsSeparateMessagesContract;
  }
  if (id == "cpp-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction-contract") {
    return scenarioTimestampedAttributeImmediateRecipientMultiTransportRetractionContract;
  }
  if (id == "cpp-tck.timestamped-attribute-sender-regulation-transition-contract") {
    return scenarioTimestampedAttributeSenderRegulationTransitionContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-no-fanout-contract") {
    return scenarioTimestampedAttributeUpdateNoFanoutContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-alternate-advances-contract") {
    return scenarioTimestampedAttributeUpdateAlternateAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-flush-queue-future-input-contract") {
    return scenarioTimestampedAttributeUpdateFlushQueueFutureInputContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-reenable-contract") {
    return scenarioTimestampedAttributeUpdateReenableContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff-contract") {
    return scenarioTimestampedAttributeUpdateTarPostGrantCutoffContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-time-advancing-cutoff-contract") {
    return scenarioTimestampedAttributeUpdateTimeAdvancingCutoffContract;
  }
  if (id == "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot-contract") {
    return scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshotContract;
  }
  if (id == "cpp-tck.timestamped-attribute-source-resignation-contract") {
    return scenarioTimestampedAttributeSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-attribute-source-resignation-fanout-contract") {
    return scenarioTimestampedAttributeSourceResignationFanoutContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-regulation-reenable-contract") {
    return scenarioTimestampedAttributeUpdateRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-interaction-mixed-advances") {
    return scenarioTimestampedInteractionMixedAdvances;
  }
  if (id == "cpp-tck.timestamped-interaction-tara-equal-boundary") {
    return scenarioTimestampedInteractionTaraEqualBoundary;
  }
  if (id == "cpp-tck.timestamped-interaction-tara-strict-timestamp-cutoff") {
    return scenarioTimestampedInteractionTaraStrictTimestampCutoff;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-equal-boundary") {
    return scenarioTimestampedInteractionTarEqualBoundary;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-inclusive-target-delivery") {
    return scenarioTimestampedInteractionTarInclusiveTargetDelivery;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-strict-timestamp-cutoff") {
    return scenarioTimestampedInteractionTarStrictTimestampCutoff;
  }
  if (id == "cpp-tck.timestamped-interaction-zero-lookahead-tar-send-boundary") {
    return scenarioTimestampedInteractionZeroLookaheadTarSendBoundary;
  }
  if (id == "cpp-tck.timestamped-interaction-tar-post-grant-cutoff") {
    return scenarioTimestampedInteractionTarPostGrantCutoff;
  }
  if (id == "cpp-tck.timestamped-interaction-flush-queue-future-input") {
    return scenarioTimestampedInteractionFlushQueueFutureInput;
  }
  if (id == "cpp-tck.fqr-postgrant-lower-tso-retraction") {
    return scenarioFqrPostgrantLowerTsoRetraction;
  }
  if (id == "cpp-tck.flush-queue-timestamped-object-deletion-known-state") {
    return scenarioFqrTimestampedObjectDeletionKnownState;
  }
  if (id == "cpp-tck.fqg-postgrant-delivery-cutoff") {
    return scenarioFqgPostgrantDeliveryCutoff;
  }
  if (id == "cpp-tck.tar-queued-tso-before-grant") {
    return scenarioTarQueuedTsoBeforeGrant;
  }
  if (id == "cpp-tck.timestamped-interaction-tso-designator-terminalization") {
    return scenarioTimestampedInteractionTsoDesignatorTerminalization;
  }
  if (id == "cpp-tck.timestamped-interaction-cross-producer-order") {
    return scenarioTimestampedInteractionCrossProducerOrder;
  }
  if (id == "cpp-tck.timestamped-interaction-no-fanout") {
    return scenarioTimestampedInteractionNoFanout;
  }
  if (id == "cpp-tck.timestamped-interaction-retraction-fanout") {
    return scenarioTimestampedInteractionRetractionFanout;
  }
  if (id == "cpp-tck.timestamped-interaction-regulation-reenable") {
    return scenarioTimestampedInteractionRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-interaction-reenable") {
    return scenarioTimestampedInteractionReenable;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-reenable") {
    return scenarioTimestampedDirectedInteractionReenable;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-regulation-reenable") {
    return scenarioTimestampedDirectedInteractionRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-subscription-kind") {
    return scenarioTimestampedDirectedInteractionSubscriptionKind;
  }
  if (id == "cpp-tck.timestamped-directed-zero-lookahead-tar-send-boundary") {
    return scenarioTimestampedDirectedZeroLookaheadTarSendBoundary;
  }
  if (id == "cpp-tck.timestamped-directed-zero-lookahead-send-at-grant") {
    return scenarioTimestampedDirectedZeroLookaheadSendAtGrant;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-source-resignation") {
    return scenarioTimestampedDirectedInteractionSourceResignation;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-source-resignation-fanout") {
    return scenarioTimestampedDirectedInteractionSourceResignationFanout;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-source-resignation-contract") {
    return scenarioTimestampedDirectedInteractionSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-source-resignation-fanout-contract") {
    return scenarioTimestampedDirectedInteractionSourceResignationFanoutContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-tar-nmr") {
    return scenarioTimestampedDirectedInteractionTarNmr;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-tar-nmr-contract") {
    return scenarioTimestampedDirectedInteractionTarNmrContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-immediate-source-resignation") {
    return scenarioTimestampedDirectedInteractionImmediateSourceResignation;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-immediate-source-resignation-contract") {
    return scenarioTimestampedDirectedInteractionImmediateSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update") {
    return scenarioTimestampedAttributeUpdate;
  }
  if (id == "cpp-tck.timestamped-attribute-update-tar-post-grant-cutoff") {
    return scenarioTimestampedAttributeUpdateTarPostGrantCutoff;
  }
  if (id == "cpp-tck.timestamped-attribute-update-time-advancing-cutoff") {
    return scenarioTimestampedAttributeUpdateTimeAdvancingCutoff;
  }
  if (id == "cpp-tck.timestamped-attribute-update-ownership-transfer") {
    return scenarioTimestampedAttributeUpdateOwnershipTransfer;
  }
  if (id == "cpp-tck.timestamped-attribute-update-ownership-transfer-contract") {
    return scenarioTimestampedAttributeUpdateOwnershipTransferContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-queued-passel-retraction") {
    return scenarioTimestampedAttributeUpdateQueuedPasselRetraction;
  }
  if (id == "cpp-tck.mixed-order-attribute-passel-retraction") {
    return scenarioMixedOrderAttributePasselRetraction;
  }
  if (id == "cpp-tck.mixed-order-attribute-passels-separate-messages") {
    return scenarioMixedOrderAttributePasselsSeparateMessages;
  }
  if (id == "cpp-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction") {
    return scenarioTimestampedAttributeImmediateRecipientMultiTransportRetraction;
  }
  if (id == "cpp-tck.timestamped-attribute-sender-regulation-transition") {
    return scenarioTimestampedAttributeSenderRegulationTransition;
  }
  if (id == "cpp-tck.timestamped-attribute-source-resignation") {
    return scenarioTimestampedAttributeSourceResignation;
  }
  if (id == "cpp-tck.timestamped-attribute-order-cohort") {
    return scenarioTimestampedAttributeOrderCohort;
  }
  if (id == "cpp-tck.timestamped-attribute-update-no-fanout") {
    return scenarioTimestampedAttributeUpdateNoFanout;
  }
  if (id == "cpp-tck.timestamped-attribute-update-alternate-advances") {
    return scenarioTimestampedAttributeUpdateAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-attribute-update-flush-queue-future-input") {
    return scenarioTimestampedAttributeUpdateFlushQueueFutureInput;
  }
  if (id == "cpp-tck.timestamped-attribute-update-reenable") {
    return scenarioTimestampedAttributeUpdateReenable;
  }
  if (id == "cpp-tck.timestamped-attribute-update-regulation-reenable") {
    return scenarioTimestampedAttributeUpdateRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-attribute-source-resignation-fanout") {
    return scenarioTimestampedAttributeSourceResignationFanout;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-update") {
    return scenarioTimestampedRegionalAttributeUpdate;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-update-contract") {
    return scenarioTimestampedRegionalAttributeUpdateContract;
  }
  if (id == "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot") {
    return scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshot;
  }
  if (id == "cpp-tck.timestamped-attribute-reflect-region-designator-callback-snapshot-contract") {
    return scenarioTimestampedAttributeReflectRegionDesignatorCallbackSnapshotContract;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-alternate-advances") {
    return scenarioTimestampedRegionalAttributeAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-alternate-advances-contract") {
    return scenarioTimestampedRegionalAttributeAlternateAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-association-replacement") {
    return scenarioTimestampedRegionalAttributeAssociationReplacement;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-association-replacement-contract") {
    return scenarioTimestampedRegionalAttributeAssociationReplacementContract;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-regulation-reenable") {
    return scenarioTimestampedRegionalAttributeRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-regulation-reenable-contract") {
    return scenarioTimestampedRegionalAttributeRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-source-resignation") {
    return scenarioTimestampedRegionalAttributeSourceResignation;
  }
  if (id == "cpp-tck.timestamped-regional-attribute-source-resignation-contract") {
    return scenarioTimestampedRegionalAttributeSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-alternate-advances") {
    return scenarioTimestampedDefaultRegionAttributeAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-alternate-advances-contract") {
    return scenarioTimestampedDefaultRegionAttributeAlternateAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-reenable") {
    return scenarioTimestampedDefaultRegionAttributeReenable;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-reenable-contract") {
    return scenarioTimestampedDefaultRegionAttributeReenableContract;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-regulation-reenable") {
    return scenarioTimestampedDefaultRegionAttributeRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-regulation-reenable-contract") {
    return scenarioTimestampedDefaultRegionAttributeRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-mixed-fanout") {
    return scenarioTimestampedDefaultRegionAttributeMixedFanout;
  }
  if (id == "cpp-tck.timestamped-default-region-attribute-mixed-fanout-contract") {
    return scenarioTimestampedDefaultRegionAttributeMixedFanoutContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction") {
    return scenarioTimestampedDefaultRegionInteraction;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-contract") {
    return scenarioTimestampedDefaultRegionInteractionContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-alternate-advances") {
    return scenarioTimestampedDefaultRegionInteractionAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-alternate-advances-contract") {
    return scenarioTimestampedDefaultRegionInteractionAlternateAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-mixed-fanout") {
    return scenarioTimestampedDefaultRegionInteractionMixedFanout;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-mixed-fanout-contract") {
    return scenarioTimestampedDefaultRegionInteractionMixedFanoutContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-source-resignation") {
    return scenarioTimestampedDefaultRegionInteractionSourceResignation;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-source-resignation-contract") {
    return scenarioTimestampedDefaultRegionInteractionSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-reenable") {
    return scenarioTimestampedDefaultRegionInteractionReenable;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-reenable-contract") {
    return scenarioTimestampedDefaultRegionInteractionReenableContract;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-regulation-reenable") {
    return scenarioTimestampedDefaultRegionInteractionRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-default-region-interaction-regulation-reenable-contract") {
    return scenarioTimestampedDefaultRegionInteractionRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion") {
    return scenarioTimestampedObjectDeletion;
  }
  if (id == "cpp-tck.timestamped-update-before-delete") {
    return scenarioTimestampedUpdateBeforeDelete;
  }
  if (id == "cpp-tck.timestamped-local-delete-object") {
    return scenarioTimestampedLocalDeleteObject;
  }
  if (id == "cpp-tck.timestamped-local-delete-attribute") {
    return scenarioTimestampedLocalDeleteAttribute;
  }
  if (id == "cpp-tck.timestamped-local-delete-object-contract") {
    return scenarioTimestampedLocalDeleteObjectContract;
  }
  if (id == "cpp-tck.timestamped-local-delete-attribute-contract") {
    return scenarioTimestampedLocalDeleteAttributeContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-no-fanout") {
    return scenarioTimestampedObjectDeletionNoFanout;
  }
  if (id == "cpp-tck.timestamped-object-deletion-no-fanout-contract") {
    return scenarioTimestampedObjectDeletionNoFanoutContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-tombstone") {
    return scenarioTimestampedObjectDeletionTombstone;
  }
  if (id == "cpp-tck.timestamped-object-deletion-tombstone-contract") {
    return scenarioTimestampedObjectDeletionTombstoneContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-regulation-reenable") {
    return scenarioTimestampedObjectDeletionRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-object-deletion-regulation-reenable-contract") {
    return scenarioTimestampedObjectDeletionRegulationReenableContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-source-resignation-fanout") {
    return scenarioTimestampedObjectDeletionSourceResignationFanout;
  }
  if (id == "cpp-tck.timestamped-object-deletion-source-resignation-fanout-contract") {
    return scenarioTimestampedObjectDeletionSourceResignationFanoutContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-retraction-joined-owners") {
    return scenarioTimestampedObjectDeletionJoinedOwnerRetraction;
  }
  if (id == "cpp-tck.timestamped-object-deletion-retraction-joined-owners-contract") {
    return scenarioTimestampedObjectDeletionJoinedOwnerRetractionContract;
  }
  if (id == "cpp-tck.timestamped-object-deletion-mixed-advances") {
    return scenarioTimestampedObjectDeletionMixedAdvances;
  }
  if (id == "cpp-tck.timestamped-object-deletion-mixed-advances-contract") {
    return scenarioTimestampedObjectDeletionMixedAdvancesContract;
  }
  if (id == "cpp-tck.alternate-time-advances") {
    return scenarioAlternateTimeAdvances;
  }
  if (id == "cpp-tck.non-regulating-federate-peer-advance-independence") {
    return scenarioNonRegulatingFederatePeerAdvanceIndependence;
  }
  if (id == "cpp-tck.non-regulating-federate-peer-advance-independence-contract") {
    return scenarioNonRegulatingFederatePeerAdvanceIndependenceContract;
  }
  if (id == "cpp-tck.timestamped-directed-alternate-advances") {
    return scenarioTimestampedDirectedAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-directed-alternate-advances-contract") {
    return scenarioTimestampedDirectedAlternateAdvancesContract;
  }
  if (id == "cpp-tck.next-message-request") {
    return scenarioNextMessageRequest;
  }
  if (id == "cpp-tck.next-message-request-contract") {
    return scenarioNextMessageRequestContract;
  }
  if (id == "cpp-tck.nmr-no-pregrant-tso-floor") {
    return scenarioNmrNoPregrantTsoFloor;
  }
  if (id == "cpp-tck.nmr-no-pregrant-tso-floor-contract") {
    return scenarioNmrNoPregrantTsoFloorContract;
  }
  if (id == "cpp-tck.nmr-received-tso-floor") {
    return scenarioNmrReceivedTsoFloor;
  }
  if (id == "cpp-tck.nmr-received-tso-floor-contract") {
    return scenarioNmrReceivedTsoFloorContract;
  }
  if (id == "cpp-tck.nmr-ro-earliest-tso-cohort") {
    return scenarioNmrQueuedRoAndEarliestTsoCohort;
  }
  if (id == "cpp-tck.nmr-ro-earliest-tso-cohort-contract") {
    return scenarioNmrQueuedRoAndEarliestTsoCohortContract;
  }
  if (id == "cpp-tck.nmr-future-tso-arrival-while-pending") {
    return scenarioNmrFutureTsoArrivesWhilePending;
  }
  if (id == "cpp-tck.nmr-future-tso-arrival-while-pending-contract") {
    return scenarioNmrFutureTsoArrivesWhilePendingContract;
  }
  if (id == "cpp-tck.nmr-directed-future-tso-arrival-while-pending") {
    return scenarioNmrDirectedFutureTsoArrivesWhilePending;
  }
  if (id == "cpp-tck.nmr-directed-future-tso-arrival-while-pending-contract") {
    return scenarioNmrDirectedFutureTsoArrivesWhilePendingContract;
  }
  if (id == "cpp-tck.nmra-directed-future-tso-arrival-while-pending") {
    return scenarioNmraDirectedFutureTsoArrivesWhilePending;
  }
  if (id == "cpp-tck.nmra-directed-future-tso-arrival-while-pending-contract") {
    return scenarioNmraDirectedFutureTsoArrivesWhilePendingContract;
  }
  if (id == "cpp-tck.nmr-received-tso-attribute-update-floor") {
    return scenarioNmrReceivedTsoAttributeUpdateFloor;
  }
  if (id == "cpp-tck.nmr-received-tso-attribute-update-floor-contract") {
    return scenarioNmrReceivedTsoAttributeUpdateFloorContract;
  }
  if (id == "cpp-tck.nmr-received-tso-directed-interaction-floor") {
    return scenarioNmrReceivedTsoDirectedInteractionFloor;
  }
  if (id == "cpp-tck.nmr-received-tso-directed-interaction-floor-contract") {
    return scenarioNmrReceivedTsoDirectedInteractionFloorContract;
  }
  if (id == "cpp-tck.nmr-empty-message-set-grant-time") {
    return scenarioNmrEmptyMessageSetGrantTime;
  }
  if (id == "cpp-tck.nmr-empty-message-set-grant-time-contract") {
    return scenarioNmrEmptyMessageSetGrantTimeContract;
  }
  if (id == "cpp-tck.nmr-request-below-galt-immediate-grant") {
    return scenarioNmrRequestBelowGaltImmediateGrant;
  }
  if (id == "cpp-tck.nmr-request-below-galt-immediate-grant-contract") {
    return scenarioNmrRequestBelowGaltImmediateGrantContract;
  }
  if (id == "cpp-tck.nmr-lits-below-galt-immediate-grant") {
    return scenarioNmrLitsBelowGaltImmediateGrant;
  }
  if (id == "cpp-tck.nmr-lits-below-galt-immediate-grant-contract") {
    return scenarioNmrLitsBelowGaltImmediateGrantContract;
  }
  if (id == "cpp-tck.nmr-no-tso-undefined-galt-nrg") {
    return scenarioNmrNoTsoUndefinedGaltNrg;
  }
  if (id == "cpp-tck.nmr-no-tso-undefined-galt-nrg-contract") {
    return scenarioNmrNoTsoUndefinedGaltNrgContract;
  }
  if (id == "cpp-tck.nmra-no-tso-undefined-galt-nrg") {
    return scenarioNmraNoTsoUndefinedGaltNrg;
  }
  if (id == "cpp-tck.nmra-no-tso-undefined-galt-nrg-contract") {
    return scenarioNmraNoTsoUndefinedGaltNrgContract;
  }
  if (id == "cpp-tck.nmra-no-pregrant-tso-floor") {
    return scenarioNmraNoPregrantTsoFloor;
  }
  if (id == "cpp-tck.nmra-no-pregrant-tso-floor-contract") {
    return scenarioNmraNoPregrantTsoFloorContract;
  }
  if (id == nmraFutureTsoArrivalPendingRequestScenario) {
    return scenarioNmraFutureTsoArrivalAfterPendingRequest;
  }
  if (id == "cpp-tck.nmra-received-tso-actual-lookahead-floor") {
    return scenarioNmraReceivedTsoActualLookaheadFloor;
  }
  if (id == "cpp-tck.nmra-received-tso-actual-lookahead-floor-contract") {
    return scenarioNmraReceivedTsoActualLookaheadFloorContract;
  }
  if (id == "cpp-tck.nmra-ro-earliest-tso-cohort") {
    return scenarioNmraQueuedRoAndEarliestTsoCohort;
  }
  if (id == "cpp-tck.nmra-ro-earliest-tso-cohort-contract") {
    return scenarioNmraQueuedRoAndEarliestTsoCohortContract;
  }
  if (id == "cpp-tck.send-message-time-state-preservation-granted") {
    return scenarioSendMessageTimeStatePreservationGranted;
  }
  if (id == "cpp-tck.send-message-time-state-preservation-granted-contract") {
    return scenarioSendMessageTimeStatePreservationGrantedContract;
  }
  if (id == "cpp-tck.available-time-advances-inclusive-galt") {
    return scenarioAvailableTimeAdvancesInclusiveGalt;
  }
  if (id == "cpp-tck.available-time-advances-inclusive-galt-contract") {
    return scenarioAvailableTimeAdvancesInclusiveGaltContract;
  }
  if (id == "cpp-tck.tara-undefined-galt-nrg") {
    return scenarioTaraUndefinedGaltNrg;
  }
  if (id == "cpp-tck.tara-undefined-galt-nrg-contract") {
    return scenarioTaraUndefinedGaltNrgContract;
  }
  if (id == "cpp-tck.tara-no-tso-undefined-galt-nrg") {
    return scenarioTaraNoTsoUndefinedGaltNrg;
  }
  if (id == "cpp-tck.tara-no-tso-undefined-galt-nrg-contract") {
    return scenarioTaraNoTsoUndefinedGaltNrgContract;
  }
  if (id == "cpp-tck.tar-acknowledged-tso-galt-release") {
    return scenarioTarAcknowledgedTsoGaltRelease;
  }
  if (id == "cpp-tck.tar-acknowledged-tso-galt-release-contract") {
    return scenarioTarAcknowledgedTsoGaltReleaseContract;
  }
  if (id == "cpp-tck.tar-multi-regulator-acknowledged-tso-progress") {
    return scenarioTarMultiRegulatorAcknowledgedTsoProgress;
  }
  if (id == "cpp-tck.time-constrained-enable-galt-equality") {
    return scenarioTimeConstrainedEnableAtGaltEquality;
  }
  if (id == "cpp-tck.time-constrained-enable-galt-equality-contract") {
    return scenarioTimeConstrainedEnableAtGaltEqualityContract;
  }
  if (id == "cpp-tck.time-constrained-enable-regulating-federate-current-time") {
    return scenarioTimeConstrainedEnableRegulatingFederateCurrentTime;
  }
  if (id == "cpp-tck.time-constrained-enable-regulating-federate-current-time-contract") {
    return scenarioTimeConstrainedEnableRegulatingFederateCurrentTimeContract;
  }
  if (id == "cpp-tck.time-regulation-enable-actual-lookahead-tso-send") {
    return scenarioTimeRegulationEnableActualLookaheadTsoSend;
  }
  if (id == "cpp-tck.time-regulation-enable-actual-lookahead-tso-send-contract") {
    return scenarioTimeRegulationEnableActualLookaheadTsoSendContract;
  }
  if (id == "cpp-tck.zero-lookahead-tar-tso-equality-rejection") {
    return scenarioZeroLookaheadTarTsoEqualityRejection;
  }
  if (id == "cpp-tck.zero-lookahead-tar-tso-equality-rejection-contract") {
    return scenarioZeroLookaheadTarTsoEqualityRejectionContract;
  }
  if (id == "cpp-tck.zero-lookahead-nmr-tso-equality-rejection") {
    return scenarioZeroLookaheadNmrTsoEqualityRejection;
  }
  if (id == "cpp-tck.zero-lookahead-nmr-tso-equality-rejection-contract") {
    return scenarioZeroLookaheadNmrTsoEqualityRejectionContract;
  }
  if (id == "cpp-tck.zero-lookahead-nmr-continuation-tara-equality-acceptance") {
    return scenarioZeroLookaheadNmrContinuationTaraEqualityAcceptance;
  }
  if (id == "cpp-tck.zero-lookahead-nmr-continuation-tara-equality-acceptance-contract") {
    return scenarioZeroLookaheadNmrContinuationTaraEqualityAcceptanceContract;
  }
  if (id == "cpp-tck.zero-lookahead-nmra-tso-equality-acceptance") {
    return scenarioZeroLookaheadNmraTsoEqualityAcceptance;
  }
  if (id == "cpp-tck.zero-lookahead-nmra-tso-equality-acceptance-contract") {
    return scenarioZeroLookaheadNmraTsoEqualityAcceptanceContract;
  }
  if (id == "cpp-tck.time-role-pending-enable") {
    return scenarioTimeRolePendingEnable;
  }
  if (id == "cpp-tck.time-role-pending-enable-contract") {
    return scenarioTimeRolePendingEnableContract;
  }
  if (id == "cpp-tck.disable-time-regulation-send-order") {
    return scenarioDisableTimeRegulationSendOrder;
  }
  if (id == "cpp-tck.disable-time-regulation-send-order-contract") {
    return scenarioDisableTimeRegulationSendOrderContract;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso") {
    return scenarioTimeConstrainedEnableRetainsQueuedTso;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-contract") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoContract;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-directed-interaction") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoDirectedInteraction;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-directed-interaction-contract") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoDirectedInteractionContract;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-object-deletion") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoObjectDeletion;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-object-deletion-contract") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoObjectDeletionContract;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-attribute") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoAttribute;
  }
  if (id == "cpp-tck.time-constrained-enable-retains-queued-tso-attribute-contract") {
    return scenarioTimeConstrainedEnableRetainsQueuedTsoAttributeContract;
  }
  if (id == "cpp-tck.time-constrained-enable-receive-order-boundary") {
    return scenarioTimeConstrainedEnableReceiveOrderBoundary;
  }
  if (id == "cpp-tck.time-constrained-enable-receive-order-boundary-contract") {
    return scenarioTimeConstrainedEnableReceiveOrderBoundaryContract;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-interaction-to-ro") {
    return scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrder;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-interaction-to-ro-contract") {
    return scenarioTimeConstrainedDisableQueuedInteractionToReceiveOrderContract;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-attribute-to-ro") {
    return scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrder;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-attribute-to-ro-contract") {
    return scenarioTimeConstrainedDisableQueuedAttributeToReceiveOrderContract;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-deletion-to-ro") {
    return scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrder;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-deletion-to-ro-contract") {
    return scenarioTimeConstrainedDisableQueuedDeletionToReceiveOrderContract;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-directed-to-ro") {
    return scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrder;
  }
  if (id == "cpp-tck.time-constrained-disable-queued-directed-to-ro-contract") {
    return scenarioTimeConstrainedDisableQueuedDirectedToReceiveOrderContract;
  }
  if (id == "cpp-tck.time-regulation-implicit-advance") {
    return scenarioTimeRegulationImplicitAdvance;
  }
  if (id == "cpp-tck.time-regulation-implicit-advance-contract") {
    return scenarioTimeRegulationImplicitAdvanceContract;
  }
  if (id == "cpp-tck.time-regulation-enable-tso-cutoff") {
    return scenarioTimeRegulationEnableTsoCutoff;
  }
  if (id == "cpp-tck.time-regulation-enable-tso-cutoff-contract") {
    return scenarioTimeRegulationEnableTsoCutoffContract;
  }
  if (id == "cpp-tck.time-bounds-queries") {
    return scenarioTimeBoundsQueries;
  }
  if (id == "cpp-tck.time-bounds-queries-contract") {
    return scenarioTimeBoundsQueriesContract;
  }
  if (id == "cpp-tck.query-lits-source-resignation") {
    return scenarioQueryLitsAfterSourceResignation;
  }
  if (id == "cpp-tck.query-lits-source-resignation-contract") {
    return scenarioQueryLitsAfterSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-directed-interactions") {
    return scenarioTimestampedDirectedInteractions;
  }
  if (id == "cpp-tck.timestamped-directed-interactions-contract") {
    return scenarioTimestampedDirectedInteractionsContract;
  }
  if (id == "cpp-tck.region-lifecycle") return scenarioRegionLifecycle;
  if (id == "cpp-tck.region-lifecycle-contract") return scenarioRegionLifecycleContract;
  if (id == "cpp-tck.region-commit-complete-ranges") {
    return scenarioRegionCommitCompleteRanges;
  }
  if (id == "cpp-tck.regional-object-attribute-dimension-subset") {
    return scenarioRegionalObjectAttributeDimensionSubset;
  }
  if (id == "cpp-tck.regional-unpublish-region-release") {
    return scenarioRegionalUnpublishRegionRelease;
  }
  if (id == "cpp-tck.regional-unpublish-region-release-contract") {
    return scenarioRegionalUnpublishRegionReleaseContract;
  }
  if (id == "cpp-tck.regional-object-update") return scenarioRegionalObjectUpdate;
  if (id == "java-tck.ddm") return scenarioRegionalObjectUpdate;
  if (id == "cpp-tck.regional-object-update-contract") {
    return scenarioRegionalObjectUpdateContract;
  }
  if (id == "cpp-tck.associate-regions-for-updates-replaces-default-region") {
    return scenarioAssociateRegionsForUpdatesReplacesDefaultRegion;
  }
  if (id == "cpp-tck.register-object-instance-with-regions-atomicity") {
    return scenarioRegisterObjectInstanceWithRegionsAtomicity;
  }
  if (id ==
      "cpp-tck.register-object-instance-with-regions-omitted-attributes-default-region") {
    return scenarioRegisterObjectInstanceWithRegionsOmittedAttributesDefaultRegion;
  }
  if (id ==
      "cpp-tck.register-object-instance-with-regions-empty-region-set-default-region") {
    return scenarioRegisterObjectInstanceWithRegionsEmptyRegionSetDefaultRegion;
  }
  if (id == "cpp-tck.register-object-instance-with-regions-invalid-region") {
    return scenarioRegisterObjectInstanceWithRegionsInvalidRegion;
  }
  if (id == "cpp-tck.regional-attribute-value-update-response-recheck") {
    return scenarioRegionalAttributeValueUpdateResponseRecheck;
  }
  if (id == "cpp-tck.regional-attribute-value-update-response-recheck-contract") {
    return scenarioRegionalAttributeValueUpdateResponseRecheckContract;
  }
  if (id == "cpp-tck.regional-attribute-update-callback-ddm-recheck") {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheck;
  }
  if (id == "cpp-tck.regional-attribute-update-callback-ddm-recheck-contract") {
    return scenarioRegionalAttributeUpdateCallbackDdmRecheckContract;
  }
  if (id == "cpp-tck.regional-attribute-value-request-filtering") {
    return scenarioRegionalAttributeValueRequestFiltering;
  }
  if (id == "cpp-tck.regional-attribute-value-request-filtering-contract") {
    return scenarioRegionalAttributeValueRequestFilteringContract;
  }
  if (id == "cpp-tck.default-region-object-routing") {
    return scenarioDefaultRegionObjectRouting;
  }
  if (id == "cpp-tck.default-region-object-routing-contract") {
    return scenarioDefaultRegionObjectRoutingContract;
  }
  if (id == "cpp-tck.default-region-registration-names") {
    return scenarioDefaultRegionRegistrationNames;
  }
  if (id == "cpp-tck.default-region-registration-names-contract") {
    return scenarioDefaultRegionRegistrationNamesContract;
  }
  if (id == "cpp-tck.passive-regional-subscription") {
    return scenarioPassiveRegionalSubscription;
  }
  if (id == "cpp-tck.passive-regional-subscription-contract") {
    return scenarioPassiveRegionalSubscriptionContract;
  }
  if (id == "cpp-tck.passive-regional-interaction-transition") {
    return scenarioPassiveRegionalInteractionTransition;
  }
  if (id == "cpp-tck.passive-regional-interaction-transition-contract") {
    return scenarioPassiveRegionalInteractionTransitionContract;
  }
  if (id == "cpp-tck.auto-provide") return scenarioAutoProvide;
  if (id == "cpp-tck.auto-provide-contract") return scenarioAutoProvideContract;
  if (id == "cpp-tck.allow-relaxed-ddm") return scenarioAllowRelaxedDdm;
  if (id == "cpp-tck.ownership-transfer-regional-update") {
    return scenarioOwnershipTransferRegionalUpdate;
  }
  if (id == "cpp-tck.ownership-transfer-deferred-regional-update") {
    return scenarioOwnershipTransferDeferredRegionalUpdate;
  }
  if (id == "cpp-tck.ownership-transfer-deferred-regional-update-contract") {
    return scenarioOwnershipTransferDeferredRegionalUpdateContract;
  }
  if (id == "cpp-tck.attribute-scope-advisories") {
    return scenarioAttributeScopeAdvisories;
  }
  if (id == "cpp-tck.attribute-scope-advisories-contract") {
    return scenarioAttributeScopeAdvisoriesContract;
  }
  if (id == attributeScopeOwnershipAcquisitionNoOutOfScopeScenario) {
    return scenarioAttributeScopeOwnershipAcquisitionException;
  }
  if (id == "cpp-tck.regional-declaration-relevance-advisories") {
    return scenarioRegionalDeclarationRelevanceAdvisories;
  }
  if (id == startRegistrationAdvisorySwitchGatingScenario) {
    return scenarioStartRegistrationAdvisorySwitchGating;
  }
  if (id == "cpp-tck.regional-declaration-relevance-advisories-contract") {
    return scenarioRegionalDeclarationRelevanceAdvisoriesContract;
  }
  if (id == "cpp-tck.regional-interaction-routing") {
    return scenarioRegionalInteractionRouting;
  }
  if (id == "cpp-tck.regional-interaction-routing-contract") {
    return scenarioRegionalInteractionRoutingContract;
  }
  if (id == "cpp-tck.regional-send-no-retraction-designator") {
    return scenarioRegionalSendNoRetractionDesignator;
  }
  if (id == "cpp-tck.default-region-interaction-routing") {
    return scenarioDefaultRegionInteractionRouting;
  }
  if (id == "cpp-tck.default-region-interaction-routing-contract") {
    return scenarioDefaultRegionInteractionRoutingContract;
  }
  if (id == "cpp-tck.default-region-interaction-unsubscribe-isolation") {
    return scenarioDefaultRegionInteractionUnsubscribeIsolation;
  }
  if (id == "cpp-tck.default-region-interaction-unsubscribe-isolation-contract") {
    return scenarioDefaultRegionInteractionUnsubscribeIsolationContract;
  }
  if (id == "cpp-tck.zero-dimensional-regional-interaction") {
    return scenarioZeroDimensionalRegionalInteraction;
  }
  if (id == "cpp-tck.zero-dimensional-regional-interaction-contract") {
    return scenarioZeroDimensionalRegionalInteractionContract;
  }
  if (id == "cpp-tck.regional-interaction-partial-default-range") {
    return scenarioRegionalInteractionPartialDefaultRange;
  }
  if (id == "cpp-tck.regional-interaction-partial-default-range-contract") {
    return scenarioRegionalInteractionPartialDefaultRangeContract;
  }
  if (id == "cpp-tck.multi-region-interaction-routing") {
    return scenarioMultiRegionInteractionRouting;
  }
  if (id == "cpp-tck.multi-region-interaction-routing-contract") {
    return scenarioMultiRegionInteractionRoutingContract;
  }
  if (id == "cpp-tck.regional-interaction-empty-subscription-sets") {
    return scenarioRegionalInteractionEmptySubscriptionSets;
  }
  if (id == "cpp-tck.regional-interaction-empty-subscription-sets-contract") {
    return scenarioRegionalInteractionEmptySubscriptionSetsContract;
  }
  if (id == "cpp-tck.regional-interaction-mixed-absent-unsubscribe") {
    return scenarioRegionalInteractionMixedAbsentUnsubscribeSet;
  }
  if (id == "cpp-tck.regional-interaction-mixed-absent-unsubscribe-contract") {
    return scenarioRegionalInteractionMixedAbsentUnsubscribeSetContract;
  }
  if (id == "cpp-tck.regional-interaction-source-region-snapshot") {
    return scenarioRegionalInteractionSourceRegionSnapshot;
  }
  if (id == "cpp-tck.regional-interaction-source-region-snapshot-contract") {
    return scenarioRegionalInteractionSourceRegionSnapshotContract;
  }
  if (id == "cpp-tck.regional-interaction-subscription-filtering") {
    return scenarioRegionalInteractionSubscriptionFiltering;
  }
  if (id == "cpp-tck.regional-interaction-subscription-filtering-contract") {
    return scenarioRegionalInteractionSubscriptionFilteringContract;
  }
  if (id == "cpp-tck.regional-interaction-region-validation") {
    return scenarioRegionalInteractionRegionValidation;
  }
  if (id == "cpp-tck.regional-interaction-region-validation-contract") {
    return scenarioRegionalInteractionRegionValidationContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction") {
    return scenarioTimestampedRegionalInteraction;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-contract") {
    return scenarioTimestampedRegionalInteractionContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-alternate-advances") {
    return scenarioTimestampedRegionalInteractionAlternateAdvances;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-alternate-advances-contract") {
    return scenarioTimestampedRegionalInteractionAlternateAdvancesContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-no-overlap") {
    return scenarioTimestampedRegionalInteractionNoOverlap;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-no-overlap-contract") {
    return scenarioTimestampedRegionalInteractionNoOverlapContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-subscription-replacement") {
    return scenarioTimestampedRegionalInteractionSubscriptionReplacement;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-subscription-replacement-contract") {
    return scenarioTimestampedRegionalInteractionSubscriptionReplacementContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-source-resignation") {
    return scenarioTimestampedRegionalInteractionSourceResignation;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-source-resignation-contract") {
    return scenarioTimestampedRegionalInteractionSourceResignationContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-tar-nmr") {
    return scenarioTimestampedRegionalInteractionTarNmr;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-tar-nmr-contract") {
    return scenarioTimestampedRegionalInteractionTarNmrContract;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-regulation-reenable") {
    return scenarioTimestampedRegionalInteractionRegulationReenable;
  }
  if (id == "cpp-tck.timestamped-regional-interaction-regulation-reenable-contract") {
    return scenarioTimestampedRegionalInteractionRegulationReenableContract;
  }
  if (id == "cpp-tck.regional-boundaries") return scenarioRegionalBoundaries;
  if (id == "java-tck.synchronization") return scenarioSynchronizationPoints;
  if (id == "cpp-tck.synchronization-points") return scenarioSynchronizationPoints;
  if (id == "cpp-tck.synchronization-points-independent-tracking") {
    return scenarioSynchronizationPointIndependentTracking;
  }
  if (id == "cpp-tck.synchronization-points-independent-tracking-contract") {
    return scenarioSynchronizationPointIndependentTrackingContract;
  }
  if (id == "cpp-tck.synchronization-point-explicit-set-late-join") {
    return scenarioSynchronizationPointExplicitSetLateJoin;
  }
  if (id == "cpp-tck.synchronization-point-explicit-set-late-join-contract") {
    return scenarioSynchronizationPointExplicitSetLateJoinContract;
  }
  if (id == "cpp-tck.callback-controls") return scenarioCallbackControls;
  if (id == "cpp-tck.callback-controls-contract") return scenarioCallbackControlsContract;
  if (id == "cpp-tck.callback-controls-attribute-update") {
    return scenarioCallbackControlsAttributeUpdate;
  }
  if (id == "cpp-tck.callback-controls-attribute-update-contract") {
    return scenarioCallbackControlsAttributeUpdateContract;
  }
  if (id == "cpp-tck.callback-controls-object-removal") {
    return scenarioCallbackControlsObjectRemoval;
  }
  if (id == "cpp-tck.callback-controls-object-removal-contract") {
    return scenarioCallbackControlsObjectRemovalContract;
  }
  if (id == "cpp-tck.callback-controls-object-discovery") {
    return scenarioCallbackControlsObjectDiscovery;
  }
  if (id == callbackDiscoveryTransportDirectionScenario) {
    return scenarioCallbackControlsObjectDiscovery;
  }
  if (id == "cpp-tck.callback-controls-object-discovery-contract") {
    return scenarioCallbackControlsObjectDiscoveryContract;
  }
  if (id == "cpp-tck.callback-controls-object-name-reservation") {
    return scenarioCallbackControlsObjectNameReservation;
  }
  if (id == "cpp-tck.callback-controls-object-name-reservation-contract") {
    return scenarioCallbackControlsObjectNameReservationContract;
  }
  if (id == "cpp-tck.callback-controls-object-name-reservation-failure") {
    return scenarioCallbackControlsObjectNameReservationFailure;
  }
  if (id == "cpp-tck.callback-controls-object-name-reservation-failure-contract") {
    return scenarioCallbackControlsObjectNameReservationFailureContract;
  }
  if (id == "cpp-tck.callback-controls-multiple-object-name-reservation") {
    return scenarioCallbackControlsMultipleObjectNameReservation;
  }
  if (id == "cpp-tck.callback-controls-multiple-object-name-reservation-contract") {
    return scenarioCallbackControlsMultipleObjectNameReservationContract;
  }
  if (id == "cpp-tck.callback-controls-declaration-advisories") {
    return scenarioCallbackControlsDeclarationAdvisories;
  }
  if (id == "cpp-tck.callback-controls-declaration-advisories-contract") {
    return scenarioCallbackControlsDeclarationAdvisoriesContract;
  }
  if (id == "cpp-tck.callback-controls-attribute-relevance-advisories") {
    return scenarioCallbackControlsAttributeRelevanceAdvisories;
  }
  if (id == "cpp-tck.callback-controls-attribute-relevance-advisories-contract") {
    return scenarioCallbackControlsAttributeRelevanceAdvisoriesContract;
  }
  if (id == "cpp-tck.callback-controls-attribute-value-request") {
    return scenarioCallbackControlsAttributeValueRequest;
  }
  if (id == "cpp-tck.callback-controls-attribute-value-request-contract") {
    return scenarioCallbackControlsAttributeValueRequestContract;
  }
  if (id == "cpp-tck.callback-controls-auto-provide") {
    return scenarioCallbackControlsAutoProvide;
  }
  if (id == "cpp-tck.callback-controls-auto-provide-contract") {
    return scenarioCallbackControlsAutoProvideContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-assumption") {
    return scenarioCallbackControlsOwnershipAssumption;
  }
  if (id == "cpp-tck.callback-controls-ownership-assumption-contract") {
    return scenarioCallbackControlsOwnershipAssumptionContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-unavailable") {
    return scenarioCallbackControlsOwnershipUnavailable;
  }
  if (id == "cpp-tck.callback-controls-ownership-unavailable-contract") {
    return scenarioCallbackControlsOwnershipUnavailableContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-release-request") {
    return scenarioCallbackControlsOwnershipReleaseRequest;
  }
  if (id == "cpp-tck.callback-controls-ownership-release-request-contract") {
    return scenarioCallbackControlsOwnershipReleaseRequestContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-acquisition-cancellation") {
    return scenarioCallbackControlsOwnershipAcquisitionCancellation;
  }
  if (id == "cpp-tck.callback-controls-ownership-acquisition-cancellation-contract") {
    return scenarioCallbackControlsOwnershipAcquisitionCancellationContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-release-denied") {
    return scenarioCallbackControlsOwnershipReleaseDenied;
  }
  if (id == "cpp-tck.callback-controls-ownership-release-denied-contract") {
    return scenarioCallbackControlsOwnershipReleaseDeniedContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-acquisition-notification") {
    return scenarioCallbackControlsOwnershipAcquisitionNotification;
  }
  if (id == "cpp-tck.callback-controls-ownership-acquisition-notification-contract") {
    return scenarioCallbackControlsOwnershipAcquisitionNotificationContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-divestiture-confirmation") {
    return scenarioCallbackControlsOwnershipDivestitureConfirmation;
  }
  if (id == "cpp-tck.callback-controls-ownership-divestiture-confirmation-contract") {
    return scenarioCallbackControlsOwnershipDivestitureConfirmationContract;
  }
  if (id == "cpp-tck.callback-controls-ownership-query") {
    return scenarioCallbackControlsOwnershipQuery;
  }
  if (id == "cpp-tck.callback-controls-ownership-query-contract") {
    return scenarioCallbackControlsOwnershipQueryContract;
  }
  if (id == "cpp-tck.callback-controls-synchronization") {
    return scenarioCallbackControlsSynchronization;
  }
  if (id == "cpp-tck.callback-controls-synchronization-contract") {
    return scenarioCallbackControlsSynchronizationContract;
  }
  if (id == "cpp-tck.callback-controls-time-advance") {
    return scenarioCallbackControlsTimeAdvance;
  }
  if (id == "cpp-tck.callback-controls-time-advance-contract") {
    return scenarioCallbackControlsTimeAdvanceContract;
  }
  if (id == "cpp-tck.callback-controls-save-restore") {
    return scenarioCallbackControlsSaveRestore;
  }
  if (id == "cpp-tck.callback-controls-save-restore-contract") {
    return scenarioCallbackControlsSaveRestoreContract;
  }
  if (id == "cpp-tck.callback-controls-save-restore-failures") {
    return scenarioCallbackControlsSaveRestoreFailures;
  }
  if (id == "cpp-tck.callback-controls-save-restore-failures-contract") {
    return scenarioCallbackControlsSaveRestoreFailuresContract;
  }
  if (id == "cpp-tck.callback-controls-restore-request-failure") {
    return scenarioCallbackControlsRestoreRequestFailure;
  }
  if (id == "cpp-tck.callback-controls-restore-request-failure-contract") {
    return scenarioCallbackControlsRestoreRequestFailureContract;
  }
  if (id == "cpp-tck.callback-controls-timestamped-attribute-update") {
    return scenarioCallbackControlsTimestampedAttributeUpdate;
  }
  if (id == "cpp-tck.callback-controls-timestamped-attribute-update-contract") {
    return scenarioCallbackControlsTimestampedAttributeUpdateContract;
  }
  if (id == "cpp-tck.callback-controls-timestamped-regional-interaction") {
    return scenarioCallbackControlsTimestampedRegionalInteraction;
  }
  if (id == "cpp-tck.callback-controls-timestamped-regional-interaction-contract") {
    return scenarioCallbackControlsTimestampedRegionalInteractionContract;
  }
  if (id == "cpp-tck.callback-controls-timestamped-object-removal") {
    return scenarioCallbackControlsTimestampedObjectRemoval;
  }
  if (id == "cpp-tck.callback-controls-timestamped-object-removal-contract") {
    return scenarioCallbackControlsTimestampedObjectRemovalContract;
  }
  if (id == "cpp-tck.callback-controls-timestamped-retraction") {
    return scenarioCallbackControlsTimestampedRetraction;
  }
  if (id == "cpp-tck.callback-controls-timestamped-retraction-contract") {
    return scenarioCallbackControlsTimestampedRetractionContract;
  }
  if (id == "cpp-tck.callback-controls-transportation") {
    return scenarioCallbackControlsTransportation;
  }
  if (id == "cpp-tck.callback-controls-transportation-contract") {
    return scenarioCallbackControlsTransportationContract;
  }
  if (id == "cpp-tck.callback-controls-directed-interaction") {
    return scenarioCallbackControlsDirectedInteraction;
  }
  if (id == "cpp-tck.callback-controls-directed-interaction-contract") {
    return scenarioCallbackControlsDirectedInteractionContract;
  }
  if (id == "cpp-tck.callback-controls-federation-reports") {
    return scenarioCallbackControlsFederationReports;
  }
  if (id == "cpp-tck.callback-controls-federation-reports-contract") {
    return scenarioCallbackControlsFederationReportsContract;
  }
  if (id == "cpp-tck.callback-controls-attribute-scope-advisories") {
    return scenarioCallbackControlsAttributeScopeAdvisories;
  }
  if (id == "cpp-tck.callback-controls-attribute-scope-advisories-contract") {
    return scenarioCallbackControlsAttributeScopeAdvisoriesContract;
  }
  if (id == "cpp-tck.callback-controls-flush-queue-grant") {
    return scenarioCallbackControlsFlushQueueGrant;
  }
  if (id == "cpp-tck.callback-controls-flush-queue-grant-contract") {
    return scenarioCallbackControlsFlushQueueGrantContract;
  }
  if (id == "cpp-tck.callback-controls-available-time-advance-callbacks") {
    return scenarioCallbackControlsAvailableTimeAdvanceCallbacks;
  }
  if (id == "cpp-tck.callback-controls-available-time-advance-callbacks-contract") {
    return scenarioCallbackControlsAvailableTimeAdvanceCallbacksContract;
  }
  if (id == "cpp-tck.callback-controls-time-role-enablement") {
    return scenarioCallbackControlsTimeRoleEnablement;
  }
  if (id == "cpp-tck.callback-controls-time-role-enablement-contract") {
    return scenarioCallbackControlsTimeRoleEnablementContract;
  }
  if (id == "cpp-tck.asynchronous-attribute-delivery") {
    return scenarioAsynchronousAttributeDelivery;
  }
  if (id == "cpp-tck.asynchronous-delivery") return scenarioAsynchronousDelivery;
  if (id == "cpp-tck.asynchronous-delivery-tso-exclusion") {
    return scenarioAsynchronousDeliveryTsoExclusion;
  }
  if (id == "cpp-tck.asynchronous-delivery-time-advancing-ro") {
    return scenarioAsynchronousDeliveryTimeAdvancingRo;
  }
  if (id == "cpp-tck.asynchronous-delivery-disabled-time-advancing-ro") {
    return scenarioAsynchronousDeliveryDisabledTimeAdvancingRo;
  }
  if (id == "cpp-tck.asynchronous-delivery-reenable-tso-gate") {
    return scenarioAsynchronousDeliveryTsoReenableGate;
  }
  if (id == "cpp-tck.federation-mom-save-status") return scenarioFederationMomSaveStatus;
  if (id == "cpp-tck.federation-mom-save-status-contract") {
    return scenarioFederationMomSaveStatusContract;
  }
  if (id == "java-tck.mom") return scenarioFederationMomSaveStatus;
  if (id == "cpp-tck.federation-save-restore") return scenarioFederationSaveRestore;
  if (id == restoreFederateTypeCountMismatchScenario) {
    return scenarioFederateTypeRestoreCountMismatch;
  }
  if (id == restoreFederateTypeIdentityRebindingScenario) {
    return scenarioFederateTypeRestoreIdentityRebinding;
  }
  if (id == "cpp-tck.ownership-state-save-restore") {
    return scenarioOwnershipSaveRestore;
  }
  if (id == "cpp-tck.pending-negotiated-ownership-save-restore") {
    return scenarioPendingNegotiatedOwnershipSaveRestore;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-save-restore") {
    return scenarioOwnershipAcquisitionCancellationSaveRestore;
  }
  if (id == "cpp-tck.if-available-ownership-restore-unavailable-callback") {
    return scenarioIfAvailableOwnershipRestoreUnavailableCallback;
  }
  if (id == "java-tck.save-restore") return scenarioFederationSaveRestore;
  if (id == "cpp-tck.federation-save-restore-interlocks") {
    return scenarioFederationSaveRestoreInterlocks;
  }
  if (id == "cpp-tck.fqr-timed-federation-save-callback" ||
      id == "cpp-tck.fqr-timed-federation-save-callback-contract") {
    return scenarioFqrTimedFederationSaveCallback;
  }
  if (id == "cpp-tck.timed-federation-save-restore") {
    return scenarioTimedFederationSaveRestore;
  }
  if (id == "cpp-tck.federation-save-timestamp-eligibility") {
    return scenarioFederateSaveTimestampEligibility;
  }
  if (id == "cpp-tck.federate-application-save-identity") {
    return scenarioFederateSaveIdentity;
  }
  if (id == "cpp-tck.federate-application-join-intent") {
    return scenarioJoinFederationIntent;
  }
  if (id == "cpp-tck.reliable-transport-one-copy-delivery") {
    return scenarioReliableTransportOneCopyDelivery;
  }
  if (id == "cpp-tck.unsupported-fom-transportation-fallback") {
    return scenarioUnsupportedFomTransportationFallback;
  }
  if (id == "cpp-tck.federation-save-timestamp-eligibility-contract") {
    return scenarioFederateSaveTimestampEligibilityContract;
  }
  if (id == "cpp-tck.federation-save-request-replacement") {
    return scenarioFederationSaveRequestReplacement;
  }
  if (id == "cpp-tck.federation-save-request-replacement-contract") {
    return scenarioFederationSaveRequestReplacementContract;
  }
  if (id == "cpp-tck.timed-regional-interaction-save-restore") {
    return scenarioTimedRegionalInteractionSaveRestore;
  }
  if (id == "cpp-tck.timed-default-region-interaction-save-restore") {
    return scenarioTimedDefaultRegionInteractionSaveRestore;
  }
  if (id == "cpp-tck.timed-default-region-attribute-save-restore") {
    return scenarioTimedDefaultRegionAttributeSaveRestore;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore") {
    return scenarioTimedRegionalAttributeNegotiatedRegularCandidateContinuationAfterRestore;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-candidate-continuation-after-restore-contract") {
    return scenarioTimedRegionalAttributeNegotiatedRegularCandidateContinuationAfterRestoreContract;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore") {
    return scenarioTimedRegionalAttributeNegotiatedRegularConfirmationCancelAfterRestore;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-confirmation-cancel-after-restore-contract") {
    return scenarioTimedRegionalAttributeNegotiatedRegularConfirmationCancelAfterRestoreContract;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore") {
    return scenarioTimedRegionalAttributeNegotiatedRegularPreDeliveryCancelAfterRestore;
  }
  if (id ==
      "cpp-tck.timed-live-tso-regional-attribute-update-multi-recipient-negotiated-regular-pre-delivery-cancel-after-restore-contract") {
    return scenarioTimedRegionalAttributeNegotiatedRegularPreDeliveryCancelAfterRestoreContract;
  }
  if (id == "cpp-tck.federation-mom-save-conditionals") {
    return scenarioFederationMomSaveConditionals;
  }
  if (id == "cpp-tck.joined-federate-mom-federate-state-save-restore") {
    return scenarioJoinedFederateMomFederateStateSaveRestore;
  }
  if (id == "cpp-tck.joined-federate-mom-galt-lits-periodic") {
    return scenarioJoinedFederateMomGaltLitsPeriodic;
  }
  if (id == "cpp-tck.joined-federate-mom-tso-length-periodic") {
    return scenarioJoinedFederateMomTsoLengthPeriodic;
  }
  if (id == "cpp-tck.joined-federate-mom-removed-object-count") {
    return scenarioJoinedFederateMomRemovedObjectCount;
  }
  if (id == "cpp-tck.joined-federate-mom-updates-sent-counts") {
    return scenarioJoinedFederateMomUpdatesSentCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-interactions-received-counts") {
    return scenarioJoinedFederateMomInteractionsReceivedCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-directed-interactions-received-counts") {
    return scenarioJoinedFederateMomDirectedInteractionsReceivedCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-interactions-sent-counts") {
    return scenarioJoinedFederateMomInteractionsSentCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-directed-interactions-sent-counts") {
    return scenarioJoinedFederateMomDirectedInteractionsSentCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-directed-interactions-sent-counts-contract") {
    return scenarioJoinedFederateMomDirectedInteractionsSentCountsContract;
  }
  if (id == "cpp-tck.mom-transportation-type-change-request") {
    return scenarioMomTransportationTypeChangeRequest;
  }
  if (id == "cpp-tck.joined-federate-mom-reflections-received-counts") {
    return scenarioJoinedFederateMomReflectionsReceivedCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-reflections-received-counts-contract") {
    return scenarioJoinedFederateMomReflectionsReceivedCountsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-reflection-counts") {
    return scenarioJoinedFederateMomReflectionCounts;
  }
  if (id == "cpp-tck.joined-federate-mom-reflection-counts-contract") {
    return scenarioJoinedFederateMomReflectionCountsContract;
  }
  if (id == "cpp-tck.joined-federate-mom-updated-object-count-periodic") {
    return scenarioJoinedFederateMomUpdatedObjectCountPeriodic;
  }
  if (id == "cpp-tck.joined-federate-mom-updated-object-count-periodic-contract") {
    return scenarioJoinedFederateMomUpdatedObjectCountPeriodicContract;
  }
  if (id == "cpp-tck.joined-federate-mom-discovered-object-count-periodic") {
    return scenarioJoinedFederateMomDiscoveredObjectCountPeriodic;
  }
  if (id == "cpp-tck.joined-federate-mom-discovered-object-count-periodic-contract") {
    return scenarioJoinedFederateMomDiscoveredObjectCountPeriodicContract;
  }
  if (id == "cpp-tck.joined-federate-mom-deleted-object-count-periodic") {
    return scenarioJoinedFederateMomDeletedObjectCountPeriodic;
  }
  if (id == "cpp-tck.joined-federate-mom-deleted-object-count-periodic-contract") {
    return scenarioJoinedFederateMomDeletedObjectCountPeriodicContract;
  }
  if (id == "cpp-tck.joined-federate-mom-time-state-durations") {
    return scenarioJoinedFederateMomTimeStateDurations;
  }
  if (id == "cpp-tck.joined-federate-mom-time-state-durations-contract") {
    return scenarioJoinedFederateMomTimeStateDurationsContract;
  }
  if (id == "java-tck.transport-order") return scenarioTransportOrder;
  if (id == "java-tck.relevance-advisories") return scenarioRelevanceAdvisories;
  if (id == "cpp-tck.attribute-relevance-known-class") {
    return scenarioKnownClassAttributeRelevance;
  }
  if (id == "cpp-tck.attribute-relevance-known-class-disabled") {
    return scenarioKnownClassAttributeRelevanceDisabled;
  }
  if (id == "cpp-tck.attribute-relevance-rate-reissue") {
    return scenarioAttributeRelevanceRateReissue;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-interaction") {
    return scenarioDelaySubscriptionEvaluationInteraction;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-directed-interaction") {
    return scenarioDelaySubscriptionEvaluationDirectedInteraction;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-attribute-update") {
    return scenarioDelaySubscriptionEvaluationAttributeUpdate;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-timestamped-interaction") {
    return scenarioDelaySubscriptionEvaluationTimestampedInteraction;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-timestamped-directed-interaction") {
    return scenarioDelaySubscriptionEvaluationTimestampedDirectedInteraction;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-timestamped-attribute-update") {
    return scenarioDelaySubscriptionEvaluationTimestampedAttributeUpdate;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-timestamped-regional-attribute-update") {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalAttributeUpdate;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-receive-order-regional-attribute-update") {
    return scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-receive-order-regional-attribute-update-contract") {
    return scenarioDelaySubscriptionEvaluationReceiveOrderRegionalAttributeUpdate;
  }
  if (id == "cpp-tck.delay-subscription-evaluation-timestamped-regional-interaction") {
    return scenarioDelaySubscriptionEvaluationTimestampedRegionalInteraction;
  }
  if (id == "cpp-tck.update-rate-queries") return scenarioUpdateRateQueries;
  if (id == federateSelectedUpdateThresholdsScenario) {
    return scenarioFederateSelectedUpdateThresholds;
  }
  if (id == federateSelectedUpdateThresholdsContractScenario) {
    return scenarioFederateSelectedUpdateThresholdsContract;
  }
  if (id == "cpp-tck.update-rate-queries-contract") {
    return scenarioUpdateRateQueriesContract;
  }
  if (id == "cpp-tck.federation-teardown-isolation") {
    return scenarioFederationTeardownIsolation;
  }
  if (id == "cpp-tck.federation-teardown-isolation-contract") {
    return scenarioFederationTeardownIsolationContract;
  }
  if (id == "cpp-tck.mixed-update-rate-subscriptions") {
    return scenarioMixedUpdateRateSubscriptions;
  }
  if (id == "cpp-tck.mixed-update-rate-subscriptions-contract") {
    return scenarioMixedUpdateRateSubscriptionsContract;
  }
  if (id == "cpp-tck.timestamped-attribute-update-rate-reduction") {
    return scenarioTimestampedAttributeUpdateRateReduction;
  }
  if (id == "cpp-tck.timestamped-attribute-update-rate-reduction-contract") {
    return scenarioTimestampedAttributeUpdateRateReductionContract;
  }
  if (id == "cpp-tck.update-rate-wall-clock-spacing") {
    return scenarioUpdateRateWallClockSpacing;
  }
  if (id == "cpp-tck.update-rate-wall-clock-spacing-contract") {
    return scenarioUpdateRateWallClockSpacingContract;
  }
  if (id == "cpp-tck.synchronous-update-rate-message-drop") {
    return scenarioSynchronousUpdateRateMessageDrop;
  }
  if (id == "cpp-tck.synchronous-update-rate-message-drop-contract") {
    return scenarioSynchronousUpdateRateMessageDropContract;
  }
  if (id == "cpp-tck.handle-wire-formats") return scenarioHandleWireFormats;
  if (id == "cpp-tck.handle-wire-formats-contract") {
    return scenarioHandleWireFormatsContract;
  }
  if (id == "java-tck.ownership") return scenarioOwnership;
  if (id == "cpp-tck.partial-attribute-ownership-transfer") {
    return scenarioPartialAttributeOwnershipTransfer;
  }
  if (id == "cpp-tck.partial-attribute-ownership-transfer-contract") {
    return scenarioPartialAttributeOwnershipTransferContract;
  }
  if (id == "cpp-tck.ownership-acquisition-publication-fence") {
    return scenarioOwnershipAcquisitionPublicationFence;
  }
  if (id == "cpp-tck.ownership-acquisition-publication-fence-contract") {
    return scenarioOwnershipAcquisitionPublicationFenceContract;
  }
  if (id == "cpp-tck.ownership-query-partition-cleanup") {
    return scenarioOwnershipQueryPartitionCleanup;
  }
  if (id == "cpp-tck.ownership-query-save-restore-results") {
    return scenarioOwnershipQuerySaveRestoreResults;
  }
  if (id == "cpp-tck.ownership-query-partition-cleanup-contract") {
    return scenarioOwnershipQueryPartitionCleanupContract;
  }
  if (id == "cpp-tck.ownership-query-save-restore-results-contract") {
    return scenarioOwnershipQuerySaveRestoreResultsContract;
  }
  if (id == "cpp-tck.divestiture-if-wanted-mixed-acquirers") {
    return scenarioDivestitureIfWantedMixedAcquirers;
  }
  if (id == "cpp-tck.divestiture-if-wanted-mixed-acquirers-contract") {
    return scenarioDivestitureIfWantedMixedAcquirersContract;
  }
  if (id == "cpp-tck.negotiated-divestiture-partial-acquisition-cancellation") {
    return scenarioNegotiatedDivestiturePartialAcquisitionCancellation;
  }
  if (id == "cpp-tck.negotiated-divestiture-partial-acquisition-cancellation-contract") {
    return scenarioNegotiatedDivestiturePartialAcquisitionCancellationContract;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-transfer-race") {
    return scenarioOwnershipAcquisitionCancellationTransferRace;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-transfer-race-contract") {
    return scenarioOwnershipAcquisitionCancellationTransferRaceContract;
  }
  if (id == "cpp-tck.negotiated-divestiture-cancellation") {
    return scenarioNegotiatedDivestitureCancellation;
  }
  if (id == "cpp-tck.negotiated-divestiture-cancellation-contract") {
    return scenarioNegotiatedDivestitureCancellationContract;
  }
  if (id == "cpp-tck.negotiated-divestiture-pre-delivery-cancellation") {
    return scenarioNegotiatedDivestiturePreDeliveryCancellation;
  }
  if (id == "cpp-tck.negotiated-divestiture-pre-delivery-cancellation-contract") {
    return scenarioNegotiatedDivestiturePreDeliveryCancellationContract;
  }
  if (id == "cpp-tck.negotiated-willing-to-acquire-continuation") {
    return scenarioNegotiatedWillingToAcquireContinuation;
  }
  if (id == "cpp-tck.negotiated-willing-to-acquire-continuation-contract") {
    return scenarioNegotiatedWillingToAcquireContinuationContract;
  }
  if (id == "cpp-tck.resign-pending-acquisition-rejection") {
    return scenarioResignPendingAcquisitionRejection;
  }
  if (id == "cpp-tck.resign-pending-acquisition-rejection-contract") {
    return scenarioResignPendingAcquisitionRejectionContract;
  }
  if (id == "cpp-tck.resign-cancel-pending-acquisition") {
    return scenarioResignCancelPendingAcquisition;
  }
  if (id == "cpp-tck.resign-cancel-pending-acquisition-contract") {
    return scenarioResignCancelPendingAcquisitionContract;
  }
  if (id == "cpp-tck.resign-cancel-if-available-pending") {
    return scenarioResignCancelIfAvailablePending;
  }
  if (id == "cpp-tck.resign-cancel-if-available-pending-contract") {
    return scenarioResignCancelIfAvailablePendingContract;
  }
  if (id == "cpp-tck.resign-cancel-negotiated-pending") {
    return scenarioResignCancelNegotiatedPending;
  }
  if (id == "cpp-tck.resign-cancel-negotiated-pending-contract") {
    return scenarioResignCancelNegotiatedPendingContract;
  }
  if (id == "java-tck.ordinary-edges") return scenarioOrdinaryEdges;
  if (id == "cpp-tck.ordinary-edges-contract") {
    return scenarioOrdinaryEdgesContract;
  }
  if (id == "cpp-tck.fom-model") return scenarioFomModel;
  if (id == "cpp-tck.fom-qualified-class-name-identity") {
    return scenarioFomQualifiedClassNameIdentity;
  }
  if (id == "cpp-tck.fom-interaction-class-hierarchy") {
    return scenarioFomInteractionClassHierarchy;
  }
  if (id == "cpp-tck.fom-interaction-qualified-class-name-identity") {
    return scenarioFomInteractionQualifiedClassNameIdentity;
  }
  if (id == "cpp-tck.inherited-object-attribute-projection") {
    return scenarioInheritedObjectAttributeProjection;
  }
  if (id == "cpp-tck.inherited-object-attribute-projection-contract") {
    return scenarioInheritedObjectAttributeProjectionContract;
  }
  if (id == "cpp-tck.inherited-interaction-class-delivery") {
    return scenarioInheritedInteractionClassDelivery;
  }
  if (id == "cpp-tck.inherited-interaction-class-delivery-contract") {
    return scenarioInheritedInteractionClassDeliveryContract;
  }
  if (id == "cpp-tck.interaction-closest-ancestor-transition") {
    return scenarioInteractionClosestAncestorTransition;
  }
  if (id == "cpp-tck.interaction-closest-ancestor-transition-contract") {
    return scenarioInteractionClosestAncestorTransitionContract;
  }
  if (id == "cpp-tck.timestamped-inherited-interaction-class-delivery") {
    return scenarioTimestampedInheritedInteractionClassDelivery;
  }
  if (id == "cpp-tck.timestamped-inherited-interaction-class-delivery-contract") {
    return scenarioTimestampedInheritedInteractionClassDeliveryContract;
  }
  if (id == "cpp-tck.directed-interaction-derived-object-target") {
    return scenarioDirectedDerivedObjectTargetDelivery;
  }
  if (id == "cpp-tck.directed-interaction-received-class-projection") {
    return scenarioDirectedInteractionReceivedClassProjection;
  }
  if (id == "cpp-tck.directed-known-object-class-fallback") {
    return scenarioDirectedKnownObjectClassFallback;
  }
  if (id == "cpp-tck.directed-known-object-class-fallback-contract") {
    return scenarioDirectedKnownObjectClassFallbackContract;
  }
  if (id == "cpp-tck.directed-retraction-mixed-fanout") {
    return scenarioDirectedRetractionMixedFanout;
  }
  if (id == "cpp-tck.directed-retraction-mixed-fanout-contract") {
    return scenarioDirectedRetractionMixedFanoutContract;
  }
  if (id == "cpp-tck.directed-retraction-immediate-only") {
    return scenarioDirectedRetractionImmediateOnly;
  }
  if (id == "cpp-tck.directed-retraction-immediate-only-contract") {
    return scenarioDirectedRetractionImmediateOnlyContract;
  }
  if (id == "cpp-tck.ownership-cancellation-idle") {
    return scenarioOwnershipCancellationIdle;
  }
  if (id == "cpp-tck.ownership-cancellation-idle-contract") {
    return scenarioOwnershipCancellationIdleContract;
  }
  if (id == "cpp-tck.acquisition-publication-guard-unavailable") {
    return scenarioAcquisitionPublicationGuardUnavailable;
  }
  if (id == "cpp-tck.acquisition-publication-guard-unavailable-contract") {
    return scenarioAcquisitionPublicationGuardUnavailableContract;
  }
  if (id == "cpp-tck.ownership-denial-idle") {
    return scenarioOwnershipDenialIdle;
  }
  if (id == "cpp-tck.ownership-denial-idle-contract") {
    return scenarioOwnershipDenialIdleContract;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-denial-race") {
    return scenarioOwnershipAcquisitionCancellationDenialRace;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-denial-race-contract") {
    return scenarioOwnershipAcquisitionCancellationDenialRaceContract;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-mixed-replies") {
    return scenarioOwnershipAcquisitionCancellationMixedReplies;
  }
  if (id == "cpp-tck.ownership-acquisition-cancellation-mixed-replies-contract") {
    return scenarioOwnershipAcquisitionCancellationMixedRepliesContract;
  }
  if (id == "cpp-tck.ownership-denial-query") {
    return scenarioOwnershipDenialQuery;
  }
  if (id == "cpp-tck.ownership-denial-query-contract") {
    return scenarioOwnershipDenialQueryContract;
  }
  if (id == "cpp-tck.ownership-transition-query") {
    return scenarioOwnershipTransitionQuery;
  }
  if (id == "cpp-tck.ownership-transition-query-contract") {
    return scenarioOwnershipTransitionQueryContract;
  }
  if (id == "cpp-tck.ownership-reacquisition-query") {
    return scenarioOwnershipReacquisitionQuery;
  }
  if (id == "cpp-tck.ownership-reacquisition-query-contract") {
    return scenarioOwnershipReacquisitionQueryContract;
  }
  if (id == "cpp-tck.ownership-competing-if-available-query") {
    return scenarioOwnershipCompetingIfAvailable;
  }
  if (id == "cpp-tck.ownership-competing-if-available-query-contract") {
    return scenarioOwnershipCompetingIfAvailableContract;
  }
  if (id == "cpp-tck.ownership-mixed-if-available-query") {
    return scenarioOwnershipMixedIfAvailable;
  }
  if (id == "cpp-tck.ownership-mixed-if-available-query-contract") {
    return scenarioOwnershipMixedIfAvailableContract;
  }
  if (id == "cpp-tck.ownership-unpublish-owned") {
    return scenarioOwnershipUnpublishOwned;
  }
  if (id == "cpp-tck.ownership-unpublish-owned-contract") {
    return scenarioOwnershipUnpublishOwnedContract;
  }
  if (id == "cpp-tck.directed-interaction-no-applicable-subscription") {
    return scenarioDirectedInteractionNoApplicableSubscription;
  }
  if (id == "cpp-tck.directed-interaction-subscription-accumulation") {
    return scenarioDirectedInteractionSubscriptionAccumulation;
  }
  if (id == "cpp-tck.directed-interaction-derived-object-target-contract") {
    return scenarioDirectedDerivedObjectTargetDeliveryContract;
  }
  if (id == "cpp-tck.directed-derived-object-target-eligibility") {
    return scenarioDirectedDerivedObjectTargetEligibility;
  }
  if (id == "cpp-tck.directed-derived-object-target-eligibility-contract") {
    return scenarioDirectedDerivedObjectTargetEligibilityContract;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-derived-object-target") {
    return scenarioTimestampedDirectedDerivedObjectTargetDelivery;
  }
  if (id == "cpp-tck.timestamped-directed-interaction-derived-object-target-contract") {
    return scenarioTimestampedDirectedDerivedObjectTargetDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-interaction-delivery") {
    return scenarioCustomTransportationInteractionDelivery;
  }
  if (id == "cpp-tck.custom-transportation-interaction-delivery-contract") {
    return scenarioCustomTransportationInteractionDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-regional-attribute-delivery") {
    return scenarioCustomTransportationRegionalAttributeDelivery;
  }
  if (id == "cpp-tck.custom-transportation-regional-attribute-delivery-contract") {
    return scenarioCustomTransportationRegionalAttributeDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-regional-interaction-delivery") {
    return scenarioCustomTransportationRegionalInteractionDelivery;
  }
  if (id == "cpp-tck.custom-transportation-regional-interaction-delivery-contract") {
    return scenarioCustomTransportationRegionalInteractionDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-delivery") {
    return scenarioCustomTransportationTimestampedDelivery;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-delivery-contract") {
    return scenarioCustomTransportationTimestampedDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-directed-delivery") {
    return scenarioCustomTransportationTimestampedDirectedDelivery;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-directed-delivery-contract") {
    return scenarioCustomTransportationTimestampedDirectedDeliveryContract;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-regional-attribute-delivery") {
    return scenarioCustomTransportationTimestampedRegionalAttributeDelivery;
  }
  if (id == "cpp-tck.custom-transportation-timestamped-regional-attribute-delivery-contract") {
    return scenarioCustomTransportationTimestampedRegionalAttributeDeliveryContract;
  }
  if (id == "cpp-tck.fom-module-composition") return scenarioFomModuleComposition;
  if (id == "cpp-tck.fom-additional-module-join-atomicity") {
    return scenarioFomAdditionalModuleJoinAtomicity;
  }
  if (id == "cpp-tck.fom-transportation-handle-stability") {
    return scenarioFomTransportationHandleStability;
  }
  if (id == "cpp-tck.fom-dimension-handle-stability") {
    return scenarioFomDimensionHandleStability;
  }
  if (id == "cpp-tck.fom-update-rate-value-stability") {
    return scenarioFomUpdateRateValueStability;
  }
  if (id == "cpp-tck.interaction-class-lookup-lifecycle") {
    return scenarioInteractionClassLookupLifecycle;
  }
  if (id == "cpp-tck.object-class-lookup-lifecycle") {
    return scenarioObjectClassLookupLifecycle;
  }
  if (id == "cpp-tck.known-object-class-and-instance-handle-lookup") {
    return scenarioKnownObjectClassAndInstanceHandleLookup;
  }
  if (id == "cpp-tck.attribute-lookup-lifecycle") {
    return scenarioAttributeLookupLifecycle;
  }
  if (id == "cpp-tck.parameter-lookup-lifecycle") {
    return scenarioParameterLookupLifecycle;
  }
  if (id == "cpp-tck.dimension-lookup-lifecycle") {
    return scenarioDimensionLookupLifecycle;
  }
  if (id == "cpp-tck.fom-basic-data-representation-endian-values") {
    return scenarioFomBasicDataRepresentationEndianValues;
  }
  if (id == "cpp-tck.fom-standard-invalid-input-corpus") {
    return scenarioFomStandardInvalidInputCorpus;
  }
  if (id == "cpp-tck.fom-invalid-create-atomicity") {
    return scenarioFomInvalidCreateAtomicity;
  }
  if (id == "cpp-tck.fom-invalid-composite-join-atomicity") {
    return scenarioFomInvalidCompositeJoinAtomicity;
  }
  if (id == "cpp-tck.fom-invalid-mim-create-atomicity") {
    return scenarioFomInvalidMimCreateAtomicity;
  }
  if (id == "cpp-tck.fom-invalid-composite-mim-create-atomicity") {
    return scenarioFomInvalidCompositeMimCreateAtomicity;
  }
  if (id == "cpp-tck.fom-empty-mim-create-atomicity") {
    return scenarioFomEmptyMimCreateAtomicity;
  }
  if (id == "cpp-tck.fom-empty-module-validation") {
    return scenarioFomEmptyModuleValidation;
  }
  if (id == fomEmptyDomainTablesAcceptedScenario) {
    return scenarioFomEmptyDomainTablesAccepted;
  }
  if (id == momTableClassStructureScenario) {
    return scenarioMomTableClassStructure;
  }
  if (id == momPredefinedObjectAndInteractionScenario) {
    return scenarioMomPredefinedObjectAndInteraction;
  }
  if (id == momModifyAttributeStateScenario) {
    return scenarioMomModifyAttributeState;
  }
  if (id == momTableInformationNotAlteredScenario) {
    return scenarioMomTableInformationNotAltered;
  }
  if (id == "cpp-tck.fom-data-type-forward-reference") {
    return scenarioFomDataTypeForwardReference;
  }
  if (id == "cpp-tck.fom-data-type-conflicting-definition") {
    return scenarioFomDataTypeConflictingDefinition;
  }
  if (id == "cpp-tck.fom-variant-alternative-overlap") {
    return scenarioFomVariantAlternativeOverlap;
  }
  if (id == "cpp-tck.fom-interaction-parameter-merge-conflict") {
    return scenarioFomInteractionParameterMergeConflict;
  }
  if (id == "cpp-tck.fom-object-attribute-merge-conflict") {
    return scenarioFomObjectAttributeMergeConflict;
  }
  if (id == "cpp-tck.fom-object-class-semantics-conflict") {
    return scenarioFomObjectClassSemanticsConflict;
  }
  if (id == "cpp-tck.fom-matching-data-types") {
    return scenarioFomMatchingDataTypes;
  }
  if (id == "cpp-tck.fom-object-class-dimension-merge") {
    return scenarioFomObjectClassDimensionMerge;
  }
  if (id == "cpp-tck.fom-interaction-class-dimension-merge") {
    return scenarioFomInteractionClassDimensionMerge;
  }
  if (id == "cpp-tck.fom-interaction-inherited-available-dimensions") {
    return scenarioFomInteractionInheritedAvailableDimensions;
  }
  if (id == "cpp-tck.fom-interaction-inherited-available-dimensions-contract") {
    return scenarioFomInteractionInheritedAvailableDimensionsContract;
  }
  if (id == "cpp-tck.fom-named-variant-alternative-merge") {
    return scenarioFomNamedVariantAlternativeMerge;
  }
  if (id == "cpp-tck.fom-extendable-variant-hlaother") {
    return scenarioFomExtendableVariantHlaOther;
  }
  if (id == "cpp-tck.fom-enumerated-value-collision") {
    return scenarioFomEnumeratedValueCollision;
  }
  if (id == "cpp-tck.fom-nonextendable-variant-alternative-conflict") {
    return scenarioFomNonextendableVariantAlternativeConflict;
  }
  if (id == "cpp-tck.fom-array-cardinality-forms") {
    return scenarioFomArrayCardinalityForms;
  }
  if (id == "cpp-tck.fom-array-cardinality-rejections") {
    return scenarioFomArrayCardinalityRejections;
  }
  if (id == "cpp-tck.fom-time-representation-valid") {
    return scenarioFomTimeRepresentationValid;
  }
  if (id == "cpp-tck.fom-time-representation-rejections") {
    return scenarioFomTimeRepresentationRejections;
  }
  if (id == "cpp-tck.fom-default-logical-time-selection") {
    return scenarioFomDefaultLogicalTimeSelection;
  }
  if (id == "cpp-tck.fom-unavailable-logical-time-factory") {
    return scenarioFomUnavailableLogicalTimeFactory;
  }
  if (id == "cpp-tck.fom-member-data-type-categories") {
    return scenarioFomMemberDataTypeCategories;
  }
  if (id == "cpp-tck.fom-attribute-na-companion-completion") {
    return scenarioFomAttributeNaCompanionCompletion;
  }
  if (id == "cpp-tck.fom-attribute-na-companion-rejections") {
    return scenarioFomAttributeNaCompanionRejections;
  }
  if (id == "cpp-tck.fom-dynamic-update-condition-valid") {
    return scenarioFomDynamicUpdateConditionValid;
  }
  if (id == "cpp-tck.fom-dynamic-update-condition-rejections") {
    return scenarioFomDynamicUpdateConditionRejections;
  }
  if (id == "cpp-tck.fom-attribute-value-required-sharing") {
    return scenarioFomAttributeValueRequiredSharing;
  }
  if (id == "cpp-tck.fom-attribute-table-nonblank-semantics") {
    return scenarioFomAttributeTableNonblankSemantics;
  }
  if (id == "cpp-tck.fom-reference-data-type-name-convention") {
    return scenarioFomReferenceDataTypeNameConvention;
  }
  if (id == "cpp-tck.fom-simple-data-type-name-convention") {
    return scenarioFomSimpleDataTypeNameConvention;
  }
  if (id == "cpp-tck.fom-model-identification-name-valid") {
    return scenarioFomModelIdentificationNameValid;
  }
  if (id == "cpp-tck.fom-enumerated-data-type-name-convention") {
    return scenarioFomEnumeratedDataTypeNameConvention;
  }
  if (id == "cpp-tck.fom-enumerator-name-convention") {
    return scenarioFomEnumeratorNameConvention;
  }
  if (id == "cpp-tck.fom-array-data-type-name-convention") {
    return scenarioFomArrayDataTypeNameConvention;
  }
  if (id == "cpp-tck.fom-array-element-type-reference") {
    return scenarioFomArrayElementTypeReference;
  }
  if (id == "cpp-tck.fom-root-hierarchy") {
    return scenarioFomRootHierarchy;
  }
  if (id == "cpp-tck.fom-tag-data-type-categories") {
    return scenarioFomTagDataTypeCategories;
  }
  if (id == "cpp-tck.fom-inherited-member-names") {
    return scenarioFomInheritedMemberNames;
  }
  if (id == "cpp-tck.fom-equivalent-class-conflict") {
    return scenarioFomEquivalentClassConflict;
  }
  if (id == "cpp-tck.fom-reference-class-resolution") {
    return scenarioFomReferenceClassResolution;
  }
  if (id == "cpp-tck.fom-enumerated-representation") {
    return scenarioFomEnumeratedRepresentation;
  }
  if (id == "cpp-tck.fom-array-element-category") {
    return scenarioFomArrayElementCategory;
  }
  if (id == "cpp-tck.fom-record-member-category") {
    return scenarioFomRecordMemberCategory;
  }
  if (id == "cpp-tck.fom-variant-discriminant-category") {
    return scenarioFomVariantDiscriminantCategory;
  }
  if (id == "cpp-tck.fom-variant-record-encoding") {
    return scenarioFomVariantRecordEncoding;
  }
  if (id == "cpp-tck.fom-variant-alternative-type-category") {
    return scenarioFomVariantAlternativeTypeCategory;
  }
  if (id == "cpp-tck.fom-variant-alternative-name-convention") {
    return scenarioFomVariantAlternativeNameConvention;
  }
  if (id == "cpp-tck.fom-variant-discriminant-name-convention") {
    return scenarioFomVariantDiscriminantNameConvention;
  }
  if (id == "cpp-tck.fom-variant-record-name-convention") {
    return scenarioFomVariantRecordNameConvention;
  }
  if (id == "cpp-tck.fom-variant-enumerator-syntax") {
    return scenarioFomVariantEnumeratorSyntax;
  }
  if (id == "cpp-tck.fom-variant-enumerator-membership") {
    return scenarioFomVariantEnumeratorMembership;
  }
  if (id == "cpp-tck.fom-variant-range-semantics") {
    return scenarioFomVariantRangeSemantics;
  }
  if (id == "cpp-tck.fom-file-url-designator") {
    return scenarioFomFileUrlDesignator;
  }
  if (id == "cpp-tck.fom-reference-attribute-resolution") {
    return scenarioFomReferenceAttributeResolution;
  }
  if (id == "cpp-tck.fom-data-representation-resolution") {
    return scenarioFomDataRepresentationResolution;
  }
  if (id == "cpp-tck.fom-noncompliant-module") {
    return scenarioFomNoncompliantModule;
  }
  if (id == "cpp-tck.fom-duplicate-object-class-attribute-name") {
    return scenarioFomDuplicateObjectClassAttributeName;
  }
  if (id == "cpp-tck.fom-incomplete-class-definition") {
    return scenarioFomIncompleteClassDefinition;
  }
  if (id == "cpp-tck.fom-transportation-reference-resolution") {
    return scenarioFomTransportationReferenceResolution;
  }
  if (id == "cpp-tck.fom-directed-reference-resolution") {
    return scenarioFomDirectedReferenceResolution;
  }
  if (id == "cpp-tck.fom-dimension-reference-resolution") {
    return scenarioFomDimensionReferenceResolution;
  }
  if (id == "cpp-tck.fom-dimension-duplicate-union") {
    return scenarioFomDimensionDuplicateUnion;
  }
  if (id == "cpp-tck.fom-dimension-duplicate-conflict") {
    return scenarioFomDimensionDuplicateConflict;
  }
  if (id == "cpp-tck.fom-dimension-input-type") {
    return scenarioFomDimensionInputType;
  }
  if (id == "cpp-tck.fom-dimension-input-description") {
    return scenarioFomDimensionInputDescription;
  }
  if (id == "cpp-tck.fom-dimension-input-na-exclusivity") {
    return scenarioFomDimensionInputNaExclusivity;
  }
  if (id == "cpp-tck.fom-dimension-name-uniqueness") {
    return scenarioFomDimensionNameUniqueness;
  }
  if (id == "cpp-tck.fom-object-class-name-conventions") {
    return scenarioFomObjectClassNameConventions;
  }
  if (id == "cpp-tck.fom-model-modification-date") {
    return scenarioFomModelModificationDate;
  }
  if (id == "cpp-tck.fom-positive-update-rate") {
    return scenarioFomPositiveUpdateRate;
  }
  if (id == "cpp-tck.fom-dimension-default-range") {
    return scenarioFomDimensionDefaultRange;
  }
  if (id == "cpp-tck.fom-synchronization-duplicate-equal") {
    return scenarioFomSynchronizationDuplicateEqual;
  }
  if (id == "cpp-tck.fom-synchronization-duplicate-note-conflict") {
    return scenarioFomSynchronizationDuplicateNoteConflict;
  }
  if (id == "cpp-tck.fom-synchronization-table-validation") {
    return scenarioFomSynchronizationTableValidation;
  }
  if (id == "cpp-tck.fom-transportation-duplicate-equal") {
    return scenarioFomTransportationDuplicateEqual;
  }
  if (id == "cpp-tck.fom-transportation-duplicate-note-conflict") {
    return scenarioFomTransportationDuplicateNoteConflict;
  }
  if (id == "cpp-tck.fom-transportation-description-ignored") {
    return scenarioFomTransportationDescriptionIgnored;
  }
  if (id == "cpp-tck.fom-update-rate-duplicate-equal") {
    return scenarioFomUpdateRateDuplicateEqual;
  }
  if (id == "cpp-tck.fom-update-rate-duplicate-note-conflict") {
    return scenarioFomUpdateRateDuplicateNoteConflict;
  }
  if (id == "cpp-tck.fom-switch-duplicate-equal") {
    return scenarioFomSwitchDuplicateEqual;
  }
  if (id == "cpp-tck.fom-switch-duplicate-order") {
    return scenarioFomSwitchDuplicateOrder;
  }
  if (id == "cpp-tck.fom-switch-duplicate-omitted") {
    return scenarioFomSwitchDuplicateOmitted;
  }
  if (id == "cpp-tck.fom-automatic-resign-action-setting") {
    return scenarioFomAutomaticResignActionSetting;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-equal") {
    return scenarioFomAdvisorySwitchDuplicateEqual;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-order") {
    return scenarioFomAdvisorySwitchDuplicateOrder;
  }
  if (id == "cpp-tck.fom-advisory-switch-duplicate-omitted") {
    return scenarioFomAdvisorySwitchDuplicateOmitted;
  }
  if (id == "cpp-tck.connection-loss-cleanup") return scenarioConnectionLoss;
  if (id == "cpp-tck.federate-reconnect-timeout") {
    return scenarioFederateReconnectTimeout;
  }
  if (id == "cpp-tck.federate-heartbeat-timeout-recovery") {
    return scenarioFederateHeartbeatTimeoutRecovery;
  }
  if (id == "cpp-tck.federate-missing-rti-message-timeout-recovers") {
    return scenarioFederateMissingRtiMessageTimeoutRecovers;
  }
  if (id == "cpp-tck.federate-idle-heartbeat-preserves-session") {
    return scenarioFederateIdleHeartbeatPreservesSession;
  }
  if (id == "cpp-tck.connection-loss-cleanup-contract") {
    return scenarioConnectionLossContract;
  }
  if (id == "cpp-tck.connection-fault-join-response-replay") {
    return scenarioConnectionFaultJoinResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-federate-request-replay") {
    return scenarioConnectionFaultFederateRequestReplay;
  }
  if (id == "cpp-tck.connection-fault-rti-closed-resume-fails") {
    return scenarioConnectionFaultRtiClosedResumeFails;
  }
  if (id == "cpp-tck.connection-fault-registration-response-replay") {
    return scenarioConnectionFaultRegistrationResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-create-federation-response-replay") {
    return scenarioConnectionFaultCreateFederationResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-reservation-response-replay") {
    return scenarioConnectionFaultReservationResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-delete-object-response-replay") {
    return scenarioConnectionFaultDeleteObjectResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-attribute-update-response-replay") {
    return scenarioConnectionFaultAttributeUpdateResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-send-interaction-response-replay") {
    return scenarioConnectionFaultSendInteractionResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-time-regulation-response-replay") {
    return scenarioConnectionFaultTimeRegulationResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-time-constrained-response-replay") {
    return scenarioConnectionFaultTimeConstrainedResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-time-advance-request-response-replay") {
    return scenarioConnectionFaultTimeAdvanceRequestResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-time-advance-request-available-response-replay") {
    return scenarioConnectionFaultTimeAdvanceRequestAvailableResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-next-message-request-response-replay") {
    return scenarioConnectionFaultNextMessageRequestResponseReplay;
  }
  if (id == "cpp-tck.connection-fault-inactive-session-purge") {
    return scenarioConnectionFaultInactiveSessionPurge;
  }
  if (id == "cpp-tck.connection-fault-next-message-request-available-response-replay") {
    return scenarioConnectionFaultNextMessageRequestAvailableResponseReplay;
  }
  if (id == "cpp-tck.connection-loss-automatic-unconditional-divestiture") {
    return scenarioConnectionLossAutomaticUnconditionalDivestiture;
  }
  if (id == "cpp-tck.connection-loss-automatic-unconditional-divestiture-contract") {
    return scenarioConnectionLossAutomaticUnconditionalDivestitureContract;
  }
  if (id == "cpp-tck.connection-loss-automatic-delete-then-divest") {
    return scenarioConnectionLossAutomaticDeleteThenDivest;
  }
  if (id == "cpp-tck.connection-loss-automatic-delete-then-divest-contract") {
    return scenarioConnectionLossAutomaticDeleteThenDivestContract;
  }
  if (id == "cpp-tck.connection-loss-automatic-cancel-pending-acquisition") {
    return scenarioConnectionLossAutomaticCancelPendingAcquisition;
  }
  if (id == "cpp-tck.connection-loss-automatic-cancel-pending-acquisition-contract") {
    return scenarioConnectionLossAutomaticCancelPendingAcquisitionContract;
  }
  throw std::runtime_error("Unknown scenario: " + id);
}

}  // namespace hla_rti_cpp_tck::detail
