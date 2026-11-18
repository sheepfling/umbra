package org.hla.rti.tck;

/** Maps Java TCK scenario IDs to their focused test entry points. */
final class JavaTckSelector {
   private JavaTckSelector() {
   }

   static String javaTestSelector(String scenarioId) {
      if ("java-tck.normalize-object-class-handle-region-coordinate".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#normalizeObjectClassHandleRegionCoordinate";
      }
      if ("java-tck.normalize-interaction-class-handle-region-coordinate".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#normalizeInteractionClassHandleRegionCoordinate";
      }
      if ("java-tck.normalize-object-instance-handle-region-coordinate".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#normalizeObjectInstanceHandleRegionCoordinate";
      }
      if ("java-tck.evoke-callback-single-dispatch-pending-indicator".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#evokeCallbackSingleDispatchPendingIndicator";
      }
      if ("java-tck.evoke-callback-empty-queue-timeout".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#evokeCallbackEmptyQueueTimeout";
      }
      if ("java-tck.evoke-callback-wait-duration-precision".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#evokeCallbackWaitDurationPrecision";
      }
      if ("java-tck.evoke-callback-immediate-no-effect".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#evokeCallbackImmediateNoEffect";
      }
      if ("java-tck.evoke-multiple-callbacks-pending-indicator".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#evokeMultipleCallbacksPendingIndicator";
      }
      if ("java-tck.disable-callbacks-gates-evoke-services".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#disableCallbacksGatesEvokeServices";
      }
      if ("java-tck.invalid-region-designator-range-bounds-invalid-region".equals(scenarioId)) {
         return "org.hla.rti.tck.SupportServicesScenarioRegistry#invalidRegionDesignatorRangeBoundsInvalidRegion";
      }
      if ("java-tck.federation-supplied-custom-tradt".equals(scenarioId)) {
         return "org.hla.rti.tck.FederationTimeScenarioRegistry#federationSuppliedCustomTradt";
      }
      if ("java-tck.fom-basic-data-representation-endian-values".equals(scenarioId)) {
         return "org.hla.rti.tck.FomAndHandleLookupScenarioRegistry#fomBasicDataRepresentationEndianValues";
      }
      if ("java-tck.federate-selected-update-thresholds".equals(scenarioId)) {
         return "org.hla.rti.tck.FederateUpdateThresholdsScenarioRegistry#federateSelectedUpdateThresholds";
      }
      if ("java-tck.federate-resigned-forced-departure".equals(scenarioId)) {
         return "org.hla.rti.tck.FederateResignedForcedDepartureScenarioRegistry#federateResignedForcedDeparture";
      }
      if ("java-tck.transport-public-new-session-interoperability".equals(scenarioId)) {
         return "org.hla.rti.tck.TransportPublicNewSessionInteropScenarioRegistry#transportPublicNewSessionInteroperability";
      }
      if ("java-tck.one-transport-connection-per-federate-session".equals(scenarioId)) {
         return "org.hla.rti.tck.TransportSessionCardinalityScenarioRegistry#oneTransportConnectionPerFederateSession";
      }
      if ("java-tck.reject-invalid-transport-session-metadata".equals(scenarioId)) {
         return "org.hla.rti.tck.InvalidTransportSessionMetadataScenarioRegistry#rejectsInvalidTransportSessionMetadata";
      }
      if ("java-tck.websocket-transport-support".equals(scenarioId)) {
         return "org.hla.rti.tck.WebSocketTransportScenarioRegistry#webSocketTransportSupport";
      }
      if ("java-tck.secure-websocket-transport-support".equals(scenarioId)) {
         return "org.hla.rti.tck.SecureWebSocketTransportScenarioRegistry#secureWebSocketTransportSupport";
      }
      if ("java-tck.tls13-transport-support".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13TransportScenarioRegistry#tls13TransportSupport";
      }
      if ("java-tck.tls13-default-port-support".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13DefaultPortScenarioRegistry#tls13DefaultPortSupport";
      }
      if ("java-tck.tls13-alternate-address-port-configuration".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13AlternateAddressPortScenarioRegistry#tls13AlternateAddressPortConfiguration";
      }
      if ("java-tck.tls13-encryption-only-mode".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13EncryptionOnlyScenarioRegistry#tls13EncryptionOnlyMode";
      }
      if ("java-tck.tls13-server-certificate-validation".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13ServerCertificateValidationScenarioRegistry#tls13ServerCertificateValidation";
      }
      if ("java-tck.tls13-server-certificate-validation-rejects-untrusted".equals(scenarioId)) {
         return "org.hla.rti.tck.Tls13ServerCertificateValidationRejectsUntrustedScenarioRegistry#tls13ServerCertificateValidationRejectsUntrusted";
      }
      if ("java-tck.callback-controls-object-discovery-transport-direction".equals(scenarioId)) {
         return "org.hla.rti.tck.RtiTckMain#callbackControlsObjectDiscoveryTransportDirection";
      }
      String method;
      switch (scenarioId) {
         case "java-tck.factory-discovery": method = "factoryDiscovery"; break;
         case "java-tck.basic-data-elements": method = "basicDataElements"; break;
         case "java-tck.basic-data-element-value-operations":
            method = "basicDataElementValueOperations"; break;
         case "java-tck.basic-data-element-encode-decode-contract":
            method = "basicDataElementEncodeDecodeContract"; break;
         case "java-tck.composite-data-element-bytewrapper-cursor":
            method = "compositeDataElementByteWrapperCursor"; break;
         case "java-tck.composite-data-element-unregistered-alternative":
            method = "compositeDataElementUnregisteredAlternative"; break;
         case "java-tck.logical-time-data-elements-contract":
            method = "logicalTimeDataElements"; break;
         case "java-tck.start-registration-advisory-switch-gating":
            method = "startRegistrationAdvisorySwitchGating"; break;
         case "java-tck.encoder-round-trip": method = "encoderRoundTrip"; break;
         case "java-tck.complex-data-element-constituent-access":
            method = "complexDataElementConstituentAccess"; break;
         case "java-tck.federate-lookup-lifecycle": method = "federateLookupLifecycle"; break;
         case "java-tck.federation-membership": method = "federationMembership"; break;
         case "java-tck.federate-application-multiple-executions":
            method = "federateApplicationMultipleExecutions"; break;
         case "java-tck.federation-list-services": method = "federationListServices"; break;
         case "java-tck.unnamed-join-overload": method = "unnamedJoinOverload"; break;
         case "java-tck.duplicate-federate-name-join-boundary":
            method = "duplicateFederateNameJoinBoundary"; break;
         case "java-tck.repeated-join-by-member-boundary":
            method = "repeatedJoinByMemberBoundary"; break;
         case "java-tck.foreign-federate-name-lookup-boundary":
            method = "foreignFederateNameLookupBoundary"; break;
         case "java-tck.foreign-object-instance-name-lookup-boundary":
            method = "foreignObjectInstanceNameLookupBoundary"; break;
         case "java-tck.foreign-object-instance-handle-lookup-boundary":
            method = "foreignObjectInstanceHandleLookupBoundary"; break;
         case "java-tck.local-delete-object-instance-name-lookup-boundary":
            method = "localDeleteObjectInstanceNameLookupBoundary"; break;
         case "java-tck.local-delete-object-instance":
            method = "localDeleteObjectInstance"; break;
         case "java-tck.remote-delete-known-object-class-lookup-boundary":
            method = "remoteDeleteKnownObjectClassLookupBoundary"; break;
         case "java-tck.remote-delete-object-instance-name-lookup-boundary":
            method = "remoteDeleteObjectInstanceNameLookupBoundary"; break;
         case "java-tck.resign-delete-object-lookup-boundary":
            method = "resignDeleteObjectLookupBoundary"; break;
         case "java-tck.resign-no-action-owned-attributes-rejection-boundary":
            method = "resignNoActionOwnedAttributesRejectionBoundary"; break;
         case "java-tck.post-resignation-service-boundaries":
            method = "postResignationServiceBoundaries"; break;
         case "java-tck.explicit-mim-creation": method = "explicitMimCreation"; break;
         case "java-tck.federation-mom-current-fdd": method = "federationMomCurrentFdd"; break;
         case "java-tck.federation-mom-fom-module-designator-list":
            method = "federationMomFomModuleDesignatorList"; break;
         case "java-tck.fom-file-url-designator":
            method = "fomFileUrlDesignator"; break;
         case "java-tck.federate-mom-fom-module-content-report":
            method = "federateMomFomModuleContentReport"; break;
         case "java-tck.federate-mom-object-instance-information":
            method = "federateMomObjectInstanceInformation"; break;
         case "java-tck.federate-mom-publication-query":
            method = "federateMomPublicationQuery"; break;
         case "java-tck.federate-mom-complete-object-class-publication-query":
            method = "federateMomCompleteObjectClassPublicationQuery"; break;
         case "java-tck.federate-mom-object-class-attribute-unpublication-query":
            method = "federateMomObjectClassAttributeUnpublicationQuery"; break;
         case "java-tck.federate-mom-object-class-full-attribute-unpublication-query":
            method = "federateMomObjectClassFullAttributeUnpublicationQuery"; break;
         case "java-tck.federate-mom-object-class-whole-unpublication-query":
            method = "federateMomObjectClassWholeUnpublicationQuery"; break;
         case "java-tck.federate-mom-interaction-class-whole-unpublication-query":
            method = "federateMomInteractionClassWholeUnpublicationQuery"; break;
         case "java-tck.federate-mom-interaction-class-partial-unpublication-query":
            method = "federateMomInteractionClassPartialUnpublicationQuery"; break;
         case "java-tck.federate-mom-multiple-interaction-class-full-unpublication-query":
            method = "federateMomMultipleInteractionClassFullUnpublicationQuery"; break;
         case "java-tck.federate-mom-multiple-directed-publication-query":
            method = "federateMomMultipleDirectedPublicationQuery"; break;
         case "java-tck.federate-mom-multiple-directed-publication-partial-unpublish-query":
            method = "federateMomMultipleDirectedPublicationPartialUnpublishQuery"; break;
         case "java-tck.federate-mom-multiple-directed-publication-full-unpublish-query":
            method = "federateMomMultipleDirectedPublicationFullUnpublishQuery"; break;
         case "java-tck.federate-mom-multiple-directed-subscription-query":
            method = "federateMomMultipleDirectedSubscriptionQuery"; break;
         case "java-tck.federate-mom-multiple-directed-subscription-full-unsubscribe-query":
            method = "federateMomMultipleDirectedSubscriptionFullUnsubscribeQuery"; break;
         case "java-tck.federate-mom-multiple-interaction-subscription-query":
            method = "federateMomMultipleInteractionSubscriptionQuery"; break;
         case "java-tck.federate-mom-multiple-interaction-subscription-unsubscribe-query":
            method = "federateMomMultipleInteractionSubscriptionUnsubscribeQuery"; break;
         case "java-tck.federate-mom-multiple-interaction-subscription-full-unsubscribe-query":
            method = "federateMomMultipleInteractionSubscriptionFullUnsubscribeQuery"; break;
         case "java-tck.federate-mom-passive-interaction-subscription-activation-query":
            method = "federateMomPassiveInteractionSubscriptionActivationQuery"; break;
         case "java-tck.federate-mom-object-class-complete-attribute-subscription-query":
            method = "federateMomCompleteAttributeSubscriptionQuery"; break;
         case "java-tck.federate-mom-object-class-passive-complete-attribute-subscription-query":
            method = "federateMomPassiveCompleteAttributeSubscriptionQuery"; break;
         case "java-tck.federate-mom-object-class-complete-attribute-subscription-activation-query":
            method = "federateMomCompleteAttributeSubscriptionActivationQuery"; break;
         case "java-tck.federate-mom-object-attribute-partial-unsubscribe-query":
            method = "federateMomObjectAttributePartialUnsubscribeQuery"; break;
         case "java-tck.federate-mom-object-attribute-full-unsubscribe-query":
            method = "federateMomObjectAttributeFullUnsubscribeQuery"; break;
         case "java-tck.federate-mom-object-class-full-unsubscribe-query":
            method = "federateMomObjectClassFullUnsubscribeQuery"; break;
         case "java-tck.federate-mom-subscription-query":
            method = "federateMomSubscriptionQuery"; break;
         case "java-tck.federate-mom-passive-subscription-query":
            method = "federateMomPassiveSubscriptionQuery"; break;
         case "java-tck.federate-mom-passive-interaction-subscription-query":
            method = "federateMomPassiveInteractionSubscriptionQuery"; break;
         case "java-tck.federate-mom-empty-subscription-query":
            method = "federateMomEmptySubscriptionQuery"; break;
         case "java-tck.federate-mom-directed-subscription-query":
            method = "federateMomDirectedSubscriptionQuery"; break;
         case "java-tck.federation-mom-static-identity":
            method = "federationMomStaticIdentity"; break;
         case "java-tck.federation-mom-static-switches":
            method = "federationMomStaticSwitches"; break;
         case "java-tck.federation-mom-federates-in-federation":
            method = "federationMomFederatesInFederation"; break;
         case "java-tck.federation-mom-content-reports":
            method = "federationMomContentReports"; break;
         case "java-tck.federation-mom-s-leaf-subscription":
            method = "federationMomSLeafSubscription"; break;
         case "java-tck.federation-mom-change-interaction-order-type":
            method = "federationMomChangeInteractionOrderType"; break;
         case "java-tck.federation-mom-change-attribute-order-type":
            method = "federationMomChangeAttributeOrderType"; break;
          case "java-tck.federation-mom-service-actor-identity":
             method = "federationMomServiceActorIdentity"; break;
         case "java-tck.callback-reentrancy": method = "callbackReentrancy"; break;
         case "java-tck.federate-initiated-concurrency-different-ambassadors":
            method = "federateInitiatedConcurrencyDifferentAmbassadors"; break;
         case "java-tck.evoked-callback-no-concurrency":
            method = "evokedCallbackNoConcurrency"; break;
         case "java-tck.evoke-multiple-callbacks-minimum-duration":
            method = "evokeMultipleCallbacksMinimumDuration"; break;
         case "java-tck.evoke-multiple-callbacks-millisecond-precision":
            method = "evokeMultipleCallbacksMillisecondPrecision"; break;
         case "java-tck.evoke-multiple-callbacks-maximum-duration":
            method = "evokeMultipleCallbacksMaximumDuration"; break;
         case "java-tck.connection-fault-federate-request-replay":
            method = "connectionFaultFederateRequestReplay"; break;
         case "java-tck.connection-fault-rti-closed-resume-fails":
            method = "connectionFaultRtiClosedResumeFails"; break;
         case "java-tck.connection-fault-join-response-replay":
            method = "connectionFaultJoinResponseReplay"; break;
         case "java-tck.connection-fault-inactive-session-purge":
            method = "connectionFaultInactiveSessionPurge"; break;
         case "java-tck.connection-fault-registration-response-replay":
            method = "connectionFaultRegistrationResponseReplay"; break;
         case "java-tck.connection-fault-create-federation-response-replay":
            method = "connectionFaultCreateFederationResponseReplay"; break;
         case "java-tck.connection-fault-reservation-response-replay":
            method = "connectionFaultReservationResponseReplay"; break;
         case "java-tck.connection-fault-delete-object-response-replay":
            method = "connectionFaultDeleteObjectResponseReplay"; break;
         case "java-tck.connection-fault-attribute-update-response-replay":
            method = "connectionFaultAttributeUpdateResponseReplay"; break;
         case "java-tck.connection-fault-send-interaction-response-replay":
            method = "connectionFaultSendInteractionResponseReplay"; break;
         case "java-tck.connection-fault-time-regulation-response-replay":
            method = "connectionFaultTimeRegulationResponseReplay"; break;
         case "java-tck.connection-fault-time-constrained-response-replay":
            method = "connectionFaultTimeConstrainedResponseReplay"; break;
         case "java-tck.connection-fault-time-advance-request-response-replay":
            method = "connectionFaultTimeAdvanceRequestResponseReplay"; break;
         case "java-tck.connection-fault-time-advance-request-available-response-replay":
            method = "connectionFaultTimeAdvanceRequestAvailableResponseReplay"; break;
         case "java-tck.connection-fault-next-message-request-response-replay":
            method = "connectionFaultNextMessageRequestResponseReplay"; break;
         case "java-tck.connection-fault-next-message-request-available-response-replay":
            method = "connectionFaultNextMessageRequestAvailableResponseReplay"; break;
         case "java-tck.connection-loss-cleanup": method = "connectionLossCleanup"; break;
         case "java-tck.federate-reconnect-timeout":
            method = "federateReconnectTimeout"; break;
         case "java-tck.federate-heartbeat-timeout-recovery":
            method = "federateHeartbeatTimeoutRecovery"; break;
         case "java-tck.automatic-resign-directive-delete-objects":
            method = "automaticResignDirectiveDeleteObjects"; break;
         case "java-tck.connection-loss-automatic-unconditional-divestiture":
            method = "connectionLossAutomaticUnconditionalDivestiture"; break;
         case "java-tck.connection-loss-automatic-cancel-pending-acquisition":
            method = "connectionLossAutomaticCancelPendingAcquisition"; break;
         case "java-tck.federate-lost-mom-report": method = "federateLostMomReport"; break;
         case "java-tck.standard-exception-boundaries":
            method = "standardExceptionBoundaries"; break;
         case "java-tck.federation-management-requires-connection":
            method = "federationManagementRequiresConnection"; break;
         case "java-tck.connect-before-federation-disconnect-after-resign":
            method = "connectBeforeFederationDisconnectAfterResign"; break;
         case "java-tck.asynchronous-delivery": method = "asynchronousDelivery"; break;
         case "java-tck.asynchronous-interaction-delivery":
            method = "asynchronousInteractionDelivery"; break;
         case "java-tck.asynchronous-delivery-tso-exclusion":
            method = "asynchronousDeliveryTsoExclusion"; break;
          case "java-tck.asynchronous-delivery-reenable-tso-gate":
            method = "asynchronousDeliveryTsoReenableGate"; break;
          case "java-tck.asynchronous-delivery-time-advancing-ro":
            method = "asynchronousDeliveryTimeAdvancingRo"; break;
          case "java-tck.asynchronous-delivery-disabled-time-advancing-ro":
            method = "asynchronousDeliveryDisabledTimeAdvancingRo"; break;
          case "java-tck.send-message-time-state-preservation-granted":
             method = "sendMessageTimeStatePreservationGranted"; break;
         case "java-tck.logical-time-factory": method = "logicalTimeFactory"; break;
         case "java-tck.logical-time-final-boundary":
            method = "logicalTimeFinalBoundary"; break;
         case "java-tck.logical-time-interval-nonnegative":
            method = "logicalTimeIntervalNonnegative"; break;
         case "java-tck.logical-time-factory-factory-contract":
            method = "logicalTimeFactoryFactoryContract"; break;
         case "java-tck.declaration-management": method = "declarationManagement"; break;
         case "java-tck.base-subscription-rejects-derived-only-attribute":
            method = "baseSubscriptionRejectsDerivedOnlyAttribute"; break;
         case "java-tck.unsubscribe-last-attribute-stops-discovery":
            method = "unsubscribeLastAttributeStopsDiscovery"; break;
         case "java-tck.attribute-unsubscribe-stops-reflection":
            method = "attributeUnsubscribeStopsReflection"; break;
         case "java-tck.base-publication-rejects-derived-only-attribute":
            method = "basePublicationRejectsDerivedOnlyAttribute"; break;
         case "java-tck.object-management": method = "objectRegistrationAndReflection"; break;
         case "java-tck.object-attribute-declarations-valid-inputs-after-resignation":
            method = "objectAttributeDeclarationsAfterResignation"; break;
          case "java-tck.object-registration-discovery-lifecycle":
             method = "objectRegistrationDiscoveryLifecycle"; break;
          case "java-tck.overlapping-object-class-subscriptions-single-discovery":
             method = "overlappingObjectClassSubscriptionsSingleDiscovery"; break;
          case "java-tck.inherited-object-attribute-projection":
             method = "inheritedObjectAttributeProjection"; break;
         case "java-tck.attribute-value-update-request-baseline":
            method = "attributeValueUpdateRequestBaseline"; break;
         case "java-tck.object-class-attribute-value-update-request-baseline":
            method = "objectClassAttributeValueUpdateRequestBaseline"; break;
         case "java-tck.object-class-timestamped-provider-response":
            method = "objectClassTimestampedProviderResponse"; break;
         case "java-tck.attribute-value-update-response":
            method = "attributeValueUpdateResponse"; break;
         case "java-tck.attribute-value-request-service-boundaries":
            method = "attributeValueRequestServiceBoundaries"; break;
         case "java-tck.attribute-value-request-service-boundaries-invalid-handle-contexts":
            method = "attributeValueRequestServiceBoundaryInvalidHandleContexts"; break;
         case "java-tck.attribute-value-update-request-multi-requester":
            method = "attributeValueUpdateMultiRequester"; break;
         case "java-tck.object-registration-discovery-multi-recipient":
            method = "objectRegistrationMultiRecipient"; break;
         case "java-tck.per-federate-discovered-class":
            method = "perFederateDiscoveredClass"; break;
         case "java-tck.registered-object-class-immutability":
            method = "registeredObjectClassImmutability"; break;
         case "java-tck.discovered-class-stability-after-specific-subscription":
            method = "discoveredClassStabilityAfterSpecificSubscription"; break;
         case "java-tck.object-class-abstraction-subscription":
            method = "objectClassAbstractionSubscription"; break;
         case "java-tck.named-registration-multi-recipient":
            method = "namedRegistrationMultiRecipient"; break;
         case "java-tck.object-attribute-subscription-lifecycle":
            method = "objectAttributeSubscriptionLifecycle"; break;
         case "java-tck.attribute-subscription-invocations-accumulate":
            method = "attributeSubscriptionInvocationsAccumulate"; break;
         case "java-tck.attribute-subscription-per-attribute-mode-transitions":
            method = "attributeSubscriptionPerAttributeModeTransitions"; break;
         case "java-tck.selective-attribute-unsubscription":
            method = "selectiveAttributeUnsubscription"; break;
         case "java-tck.whole-class-attribute-unsubscription":
            method = "wholeClassAttributeUnsubscription"; break;
         case "java-tck.empty-attribute-unsubscription-is-no-op":
            method = "emptyAttributeUnsubscriptionNoOp"; break;
         case "java-tck.passive-subscription-no-start-registration":
            method = "passiveSubscriptionDoesNotStartRegistration"; break;
         case "java-tck.empty-attribute-subscription-is-no-op":
            method = "emptyAttributeSubscriptionNoOp"; break;
         case "java-tck.object-publication-registration-fence":
            method = "objectPublicationRegistrationFence"; break;
         case "java-tck.object-registration-service-boundaries":
            method = "objectRegistrationServiceBoundaries"; break;
         case "java-tck.ordinary-multi-attribute-subscription-projection":
            method = "multiAttributeSubscriptionProjection"; break;
         case "java-tck.subscription-initial-reflection-projection":
            method = "subscriptionInitialReflectionProjection"; break;
         case "java-tck.active-subscription-object-discovery":
            method = "activeSubscriptionObjectDiscovery"; break;
         case "java-tck.ordinary-multi-attribute-value-update-request-response":
            method = "multiAttributeValueUpdateRequestResponse"; break;
         case "java-tck.object-removal-multi-recipient-fifo":
            method = "objectRemovalMultiRecipientFifo"; break;
         case "java-tck.object-deletion-service-boundaries":
            method = "objectDeletionServiceBoundaries"; break;
         case "java-tck.receive-order-object-removal":
            method = "receiveOrderObjectRemoval"; break;
         case "java-tck.receive-order-object-removal-subscription-withdrawal":
            method = "receiveOrderObjectRemovalSubscriptionWithdrawal"; break;
         case "java-tck.resign-delete-objects-multi-recipient-fifo":
            method = "resignDeleteObjectsMultiRecipientFifo"; break;
         case "java-tck.resign-delete-objects":
            method = "resignDeleteObjects"; break;
         case "java-tck.resign-delete-objects-then-divest-cross-object-effects":
            method = "resignDeleteThenDivestCrossObjectEffects"; break;
         case "java-tck.resign-delete-then-divest-privilege-epoch":
            method = "resignDeleteThenDivestPrivilegeEpoch"; break;
         case "java-tck.resign-unconditionally-divest-preserves-objects":
            method = "resignUnconditionallyDivestPreservesObjects"; break;
         case "java-tck.resign-unconditional-divestiture":
            method = "resignUnconditionalDivestiture"; break;
         case "java-tck.resign-assumption-later-publication":
            method = "resignAssumptionLaterPublication"; break;
         case "java-tck.ownership-assumption-search-after-decline":
            method = "ownershipAssumptionSearchAfterDecline"; break;
         case "java-tck.resign-multi-survivor-assumption-fanout":
            method = "resignMultiSurvivorAssumptionFanout"; break;
         case "java-tck.ownership-assumption-transfer-epoch":
            method = "ownershipAssumptionTransferEpoch"; break;
         case "java-tck.resign-assumption-later-discovery":
            method = "resignAssumptionLaterDiscovery"; break;
         case "java-tck.negotiated-divestiture-concurrent-assumption":
            method = "negotiatedDivestitureConcurrentAssumption"; break;
         case "java-tck.confirm-divestiture-from-callback":
            method = "confirmDivestitureFromCallback"; break;
         case "java-tck.confirm-divestiture-mixed-set-atomicity":
            method = "confirmDivestitureMixedSetAtomicity"; break;
         case "java-tck.resign-cancel-pending-ownership-acquisitions-action":
            method = "resignCancelPendingOwnershipAcquisitions"; break;
         case "java-tck.resign-cancel-then-delete-then-divest-cross-object-effects":
            method = "resignCancelThenDeleteThenDivestCrossObjectEffects"; break;
         case "java-tck.automatic-resign-directive-lifecycle-boundaries":
            method = "automaticResignDirectiveLifecycleBoundaries"; break;
         case "java-tck.automatic-resign-directive-standard-enumerator-values":
            method = "automaticResignDirectiveStandardEnumeratorValues"; break;
         case "java-tck.advisory-switch-value-round-trips":
            method = "advisorySwitchValueRoundTrips"; break;
         case "java-tck.support-switch-getter-lifecycle-boundaries":
            method = "supportSwitchGetterLifecycle"; break;
         case "java-tck.support-switch-setter-lifecycle-boundaries":
            method = "supportSwitchSetterLifecycle"; break;
         case "java-tck.attribute-multi-recipient-fifo":
            method = "attributeMultiRecipientFifo"; break;
         case "java-tck.receive-order-attribute-update":
            method = "receiveOrderAttributeUpdate"; break;
         case "java-tck.attribute-interaction": method = "attributeInteraction"; break;
         case "java-tck.declaration-publish-before-send":
            method = "declarationPublishBeforeSend"; break;
         case "java-tck.declaration-time-position-independence":
            method = "declarationTimePositionIndependence"; break;
         case "java-tck.attribute-update-service-boundaries":
            method = "attributeUpdateServiceBoundaries"; break;
         case "java-tck.support-services": method = "supportServices"; break;
         case "java-tck.standard-order-transportation-lookups":
            method = "standardOrderTransportationLookups"; break;
         case "java-tck.handle-wire-formats": method = "handleWireFormats"; break;
         case "java-tck.public-handle-decoding": method = "publicHandleDecoding"; break;
         case "java-tck.handle-decoder-semantic-validity":
            method = "handleDecoderSemanticValidity"; break;
         case "java-tck.handle-encoding-mim-data-types":
            method = "handleEncodingMimDataTypes"; break;
         case "java-tck.handle-parameter-value-round-trip":
            method = "handleParameterValueRoundTrip"; break;
         case "java-tck.callback-controls-attribute-scope-advisories":
            method = "callbackControlsAttributeScopeAdvisories"; break;
         case "java-tck.callback-controls-timestamped-regional-interaction":
            method = "callbackControlsTimestampedRegionalInteraction"; break;
          case "java-tck.timestamped-regional-interaction": method = "timestampedRegionalInteraction"; break;
          case "java-tck.timestamped-default-region-interaction":
             method = "timestampedDefaultRegionInteraction"; break;
          case "java-tck.timestamped-default-region-interaction-alternate-advances":
             method = "timestampedDefaultRegionInteractionAlternateAdvances"; break;
          case "java-tck.timestamped-default-region-interaction-mixed-fanout":
             method = "timestampedDefaultRegionInteractionMixedFanout"; break;
          case "java-tck.timestamped-default-region-interaction-source-resignation":
             method = "timestampedDefaultRegionInteractionSourceResignation"; break;
          case "java-tck.timestamped-regional-interaction-source-resignation":
             method = "timestampedRegionalInteractionSourceResignation"; break;
          case "java-tck.timestamped-default-region-interaction-reenable":
             method = "timestampedDefaultRegionInteractionReenable"; break;
          case "java-tck.timestamped-default-region-interaction-regulation-reenable":
             method = "timestampedDefaultRegionInteractionRegulationReenable"; break;
          case "java-tck.timestamped-regional-interaction-regulation-reenable":
             method = "timestampedRegionalInteractionRegulationReenable"; break;
          case "java-tck.timestamped-regional-interaction-no-overlap":
             method = "timestampedRegionalInteractionNoOverlap"; break;
          case "java-tck.timestamped-regional-interaction-subscription-replacement":
             method = "timestampedRegionalInteractionSubscriptionReplacement"; break;
          case "java-tck.timestamped-regional-interaction-tar-nmr":
             method = "timestampedRegionalInteractionTarNmr"; break;
          case "java-tck.nmr-regional-timestamped-interaction-message-set":
             method = "nmrRegionalTimestampedInteractionMessageSet"; break;
          case "java-tck.timestamped-regional-interaction-alternate-advances":
             method = "timestampedRegionalInteractionAlternateAdvances"; break;
          case "java-tck.regional-interaction-routing": method = "regionalInteractionRouting"; break;
          case "java-tck.service-report-regional-interaction": method = "serviceReportRegionalInteraction"; break;
          case "java-tck.service-report-regional-interaction-subscription": method = "serviceReportRegionalInteractionSubscription"; break;
          case "java-tck.service-report-associate-regions-for-updates":
             method = "serviceReportAssociateRegionsForUpdates"; break;
          case "java-tck.service-report-regional-interaction-failure": method = "serviceReportRegionalInteractionFailure"; break;
          case "java-tck.regional-interaction-empty-subscription-sets": method = "regionalInteractionEmptySubscriptionSets"; break;
          case "java-tck.regional-interaction-mixed-absent-unsubscribe":
             return "org.hla.rti.tck.RegionalInteractionMixedAbsentUnsubscribeScenarioRegistry#run";
          case "java-tck.regional-interaction-source-region-snapshot": method = "regionalInteractionSourceRegionSnapshot"; break;
          case "java-tck.regional-interaction-subscription-filtering": method = "regionalInteractionSubscriptionFiltering"; break;
          case "java-tck.regional-interaction-region-validation": method = "regionalInteractionRegionValidation"; break;
          case "java-tck.regional-unpublish-region-release": method = "regionalUnpublishRegionRelease"; break;
          case "java-tck.region-lifecycle": method = "regionLifecycle"; break;
          case "java-tck.region-range-validation": method = "regionRangeValidation"; break;
          case "java-tck.region-range-dimension-validation": method = "regionRangeDimensionValidation"; break;
          case "java-tck.regional-boundaries": method = "regionalBoundaries"; break;
          case "java-tck.regional-object-attribute-dimension-subset":
             method = "regionalObjectAttributeDimensionSubset"; break;
          case "java-tck.regional-object-attribute-dimension-inheritance":
             method = "regionalObjectAttributeDimensionInheritance"; break;
          case "java-tck.regional-object-attribute-default-region-fdd-dimensions":
             return "org.hla.rti.tck.RegionalObjectAttributeDimensionInheritanceTck"
                + "#runDefaultRegionFddDimensionsFromAdapter";
          case "java-tck.regional-object-attribute-unspecified-dimension-default-range":
             return "org.hla.rti.tck.RegionalObjectAttributeDimensionInheritanceTck"
                + "#runUnspecifiedDimensionDefaultRangeFromAdapter";
          case "java-tck.regional-object-attribute-excluded-dimension-not-realized":
             return "org.hla.rti.tck.RegionalObjectAttributeDimensionInheritanceTck"
                + "#runExcludedDimensionNotRealizedFromAdapter";
          case "java-tck.regional-interaction-partial-default-range":
             return "org.hla.rti.tck.RegionalInteractionPartialDefaultRangeTck#runFromAdapter";
          case "java-tck.fom-interaction-inherited-available-dimensions":
             return "org.hla.rti.tck.FomInteractionInheritedAvailableDimensionsTck#runFromAdapter";
          case "java-tck.joined-federate-mom-added-attribute-same-rti-instance":
             return "org.hla.rti.tck.JoinedFederateMomAddedAttributeSameRtiInstanceTck#runFromAdapter";
          case "java-tck.region-commit-complete-ranges":
             method = "regionCommitCompleteRanges"; break;
          case "java-tck.passive-regional-interaction-transition": method = "passiveRegionalInteractionTransition"; break;
          case "java-tck.passive-regional-subscription": method = "passiveRegionalSubscription"; break;
          case "java-tck.default-region-interaction-routing": method = "defaultRegionInteractionRouting"; break;
          case "java-tck.default-region-interaction-unsubscribe-isolation":
             method = "defaultRegionInteractionUnsubscribeIsolation"; break;
          case "java-tck.zero-dimensional-regional-interaction": method = "zeroDimensionalRegionalInteraction"; break;
          case "java-tck.multi-region-interaction-routing": method = "multiRegionInteractionRouting"; break;
          case "java-tck.regional-send-no-retraction-designator":
             method = "regionalSendNoRetractionDesignator"; break;
          case "java-tck.inherited-interaction-class-delivery": method = "inheritedInteractionClassDelivery"; break;
          case "java-tck.interaction-closest-ancestor-transition": method = "interactionClosestAncestorTransition"; break;
          case "java-tck.timestamped-inherited-interaction-class-delivery": method = "timestampedInheritedInteractionClassDelivery"; break;
          case "java-tck.timed-regional-interaction-save-restore": method = "timedRegionalInteractionSaveRestore"; break;
          case "java-tck.timed-default-region-interaction-save-restore": method = "timedDefaultRegionInteractionSaveRestore"; break;
          case "java-tck.timed-default-region-attribute-save-restore": method = "timedDefaultRegionAttributeSaveRestore"; break;
          case "java-tck.timed-regional-attribute-ownership-save-restore": method = "timedRegionalAttributeOwnershipSaveRestore"; break;
          case "java-tck.timed-regional-attribute-ownership-confirmation-cancel-save-restore": method = "timedRegionalAttributeOwnershipConfirmationCancelSaveRestore"; break;
          case "java-tck.timed-regional-attribute-ownership-pre-delivery-cancel-save-restore": method = "timedRegionalAttributeOwnershipPreDeliveryCancelSaveRestore"; break;
         case "java-tck.support-lookups-valid-inputs-after-resignation":
            method = "supportLookupsAfterResignation"; break;
         case "java-tck.dimension-lookup-valid-handles-after-resignation":
            method = "dimensionLookupAfterResignation"; break;
         case "java-tck.fom-model": method = "fomModel"; break;
         case "java-tck.fom-qualified-class-name-identity":
            method = "fomQualifiedClassNameIdentity"; break;
         case "java-tck.fom-interaction-qualified-class-name-identity":
            method = "fomInteractionQualifiedClassNameIdentity"; break;
         case "java-tck.fom-duplicate-object-class-attribute-name":
            method = "fomDuplicateObjectClassAttributeName"; break;
         case "java-tck.fom-transportation-handle-stability":
            method = "fomTransportationHandleStability"; break;
         case "java-tck.fom-module-composition": method = "fomModuleComposition"; break;
         case "java-tck.fom-incomplete-class-definition":
            method = "fomIncompleteClassDefinition"; break;
         case "java-tck.fom-additional-module-join-atomicity":
            method = "fomAdditionalModuleJoinAtomicity"; break;
         case "java-tck.fom-invalid-composite-join-atomicity":
            method = "fomInvalidCompositeJoinAtomicity"; break;
         case "java-tck.fom-update-rate-value-stability":
            method = "fomUpdateRateValueStability"; break;
         case "java-tck.fom-invalid-create-atomicity":
            method = "fomInvalidCreateAtomicity"; break;
         case "java-tck.fom-noncompliant-module":
            method = "fomNoncompliantModule"; break;
         case "java-tck.fom-invalid-mim-create-atomicity":
            method = "fomInvalidMimCreateAtomicity"; break;
         case "java-tck.fom-invalid-composite-mim-create-atomicity":
            method = "fomInvalidCompositeMimCreateAtomicity"; break;
         case "java-tck.fom-empty-mim-create-atomicity":
            method = "fomEmptyMimCreateAtomicity"; break;
         case "java-tck.fom-empty-module-validation":
            method = "fomEmptyModuleValidation"; break;
         case "java-tck.fom-empty-domain-tables-accepted":
            method = "fomEmptyDomainTablesAccepted"; break;
         case "java-tck.fom-data-type-forward-reference":
            method = "fomDataTypeForwardReference"; break;
         case "java-tck.fom-data-type-conflicting-definition":
            method = "fomDataTypeConflictingDefinition"; break;
         case "java-tck.fom-variant-alternative-overlap":
            method = "fomVariantAlternativeOverlap"; break;
         case "java-tck.fom-interaction-parameter-merge-conflict":
            method = "fomInteractionParameterMergeConflict"; break;
         case "java-tck.fom-object-attribute-merge-conflict":
            method = "fomObjectAttributeMergeConflict"; break;
         case "java-tck.fom-object-class-semantics-conflict":
            method = "fomObjectClassSemanticsConflict"; break;
         case "java-tck.fom-matching-data-types":
            method = "fomMatchingDataTypes"; break;
         case "java-tck.fom-object-class-dimension-merge":
            method = "fomObjectClassDimensionMerge"; break;
         case "java-tck.fom-interaction-class-dimension-merge":
            method = "fomInteractionClassDimensionMerge"; break;
         case "java-tck.fom-named-variant-alternative-merge":
            method = "fomNamedVariantAlternativeMerge"; break;
         case "java-tck.fom-extendable-variant-hlaother":
            method = "fomExtendableVariantHlaOther"; break;
         case "java-tck.fom-enumerated-value-collision":
            method = "fomEnumeratedValueCollision"; break;
         case "java-tck.fom-nonextendable-variant-alternative-conflict":
            method = "fomNonextendableVariantAlternativeConflict"; break;
         case "java-tck.fom-array-cardinality-forms":
            method = "fomArrayCardinalityForms"; break;
         case "java-tck.fom-array-cardinality-rejections":
            method = "fomArrayCardinalityRejections"; break;
         case "java-tck.fom-time-representation-valid":
            method = "fomTimeRepresentationValid"; break;
         case "java-tck.fom-default-logical-time-selection":
            method = "fomDefaultLogicalTimeSelection"; break;
         case "java-tck.fom-unavailable-logical-time-factory":
            method = "fomUnavailableLogicalTimeFactory"; break;
         case "java-tck.fom-time-representation-rejections":
            method = "fomTimeRepresentationRejections"; break;
         case "java-tck.fom-member-data-type-categories":
            method = "fomMemberDataTypeCategories"; break;
         case "java-tck.fom-attribute-na-companion-completion":
            method = "fomAttributeNaCompanionCompletion"; break;
         case "java-tck.fom-attribute-na-companion-rejections":
            method = "fomAttributeNaCompanionRejections"; break;
         case "java-tck.fom-dynamic-update-condition-valid":
            method = "fomDynamicUpdateConditionValid"; break;
         case "java-tck.fom-dynamic-update-condition-rejections":
            method = "fomDynamicUpdateConditionRejections"; break;
         case "java-tck.fom-attribute-value-required-sharing":
            method = "fomAttributeValueRequiredSharing"; break;
         case "java-tck.fom-root-hierarchy":
            method = "fomRootHierarchy"; break;
         case "java-tck.fom-interaction-class-hierarchy":
            method = "fomInteractionClassHierarchy"; break;
         case "java-tck.fom-tag-data-type-categories":
            method = "fomTagDataTypeCategories"; break;
         case "java-tck.fom-inherited-member-names":
            method = "fomInheritedMemberNames"; break;
         case "java-tck.fom-equivalent-class-conflict":
            method = "fomEquivalentClassConflict"; break;
         case "java-tck.fom-reference-class-resolution":
            method = "fomReferenceClassResolution"; break;
         case "java-tck.fom-enumerated-representation":
            method = "fomEnumeratedRepresentation"; break;
         case "java-tck.fom-array-element-category":
            method = "fomArrayElementCategory"; break;
         case "java-tck.fom-record-member-category":
            method = "fomRecordMemberCategory"; break;
         case "java-tck.fom-variant-discriminant-category":
            method = "fomVariantDiscriminantCategory"; break;
         case "java-tck.fom-variant-enumerator-syntax":
            method = "fomVariantEnumeratorSyntax"; break;
         case "java-tck.fom-variant-enumerator-membership":
            method = "fomVariantEnumeratorMembership"; break;
         case "java-tck.fom-variant-range-semantics":
            method = "fomVariantRangeSemantics"; break;
         case "java-tck.fom-reference-attribute-resolution":
            method = "fomReferenceAttributeResolution"; break;
         case "java-tck.fom-data-representation-resolution":
            method = "fomDataRepresentationResolution"; break;
         case "java-tck.fom-transportation-reference-resolution":
            method = "fomTransportationReferenceResolution"; break;
         case "java-tck.fom-directed-reference-resolution":
            method = "fomDirectedReferenceResolution"; break;
         case "java-tck.fom-dimension-reference-resolution":
            method = "fomDimensionReferenceResolution"; break;
         case "java-tck.fom-dimension-duplicate-union":
            method = "fomDimensionDuplicateUnion"; break;
         case "java-tck.fom-dimension-duplicate-conflict":
            method = "fomDimensionDuplicateConflict"; break;
         case "java-tck.fom-dimension-input-type":
            method = "fomDimensionInputType"; break;
         case "java-tck.fom-dimension-input-description":
            method = "fomDimensionInputDescription"; break;
         case "java-tck.fom-dimension-input-na-exclusivity":
            method = "fomDimensionInputNaExclusivity"; break;
         case "java-tck.fom-dimension-name-uniqueness":
            method = "fomDimensionNameUniqueness"; break;
         case "java-tck.fom-object-class-name-conventions":
            method = "fomObjectClassNameConventions"; break;
         case "java-tck.fom-model-modification-date":
            method = "fomModelModificationDate"; break;
         case "java-tck.fom-positive-update-rate":
            method = "fomPositiveUpdateRate"; break;
         case "java-tck.fom-dimension-default-range":
            method = "fomDimensionDefaultRange"; break;
         case "java-tck.fom-synchronization-duplicate-equal":
            method = "fomSynchronizationDuplicateEqual"; break;
         case "java-tck.fom-synchronization-duplicate-note-conflict":
            method = "fomSynchronizationDuplicateNoteConflict"; break;
         case "java-tck.fom-transportation-duplicate-equal":
            method = "fomTransportationDuplicateEqual"; break;
         case "java-tck.fom-transportation-duplicate-note-conflict":
            method = "fomTransportationDuplicateNoteConflict"; break;
         case "java-tck.fom-update-rate-duplicate-equal":
            method = "fomUpdateRateDuplicateEqual"; break;
         case "java-tck.fom-update-rate-duplicate-note-conflict":
            method = "fomUpdateRateDuplicateNoteConflict"; break;
         case "java-tck.fom-switch-duplicate-equal":
            method = "fomSwitchDuplicateEqual"; break;
         case "java-tck.fom-switch-duplicate-order":
            method = "fomSwitchDuplicateOrder"; break;
         case "java-tck.fom-switch-duplicate-omitted":
            method = "fomSwitchDuplicateOmitted"; break;
         case "java-tck.fom-advisory-switch-duplicate-equal":
            method = "fomAdvisorySwitchDuplicateEqual"; break;
         case "java-tck.fom-advisory-switch-duplicate-order":
            method = "fomAdvisorySwitchDuplicateOrder"; break;
         case "java-tck.fom-advisory-switch-duplicate-omitted":
            method = "fomAdvisorySwitchDuplicateOmitted"; break;
         case "java-tck.fom-service-usage-merge":
            method = "fomServiceUsageMerge"; break;
         case "java-tck.fom-note-collision-remap":
            method = "fomNoteCollisionRemap"; break;
         case "java-tck.fom-standard-invalid-input-corpus":
            method = "fomStandardInvalidInputCorpus"; break;
         case "java-tck.fom-dimension-handle-stability":
            method = "fomDimensionHandleStability"; break;
         case "java-tck.update-rate-lookups-valid-inputs-after-resignation":
            method = "updateRateLookupsAfterResignation"; break;
         case "java-tck.timestamped-attribute-update-rate-reduction":
            method = "timestampedAttributeUpdateRateReduction"; break;
         case "java-tck.mixed-update-rate-subscriptions":
            method = "mixedUpdateRateSubscriptions"; break;
         case "java-tck.update-rate-wall-clock-spacing":
            method = "updateRateWallClockSpacing"; break;
         case "java-tck.synchronous-update-rate-message-drop":
            method = "synchronousUpdateRateMessageDrop"; break;
         case "java-tck.attribute-relevance-rate-reissue":
            method = "attributeRelevanceRateReissue"; break;
         case "java-tck.attribute-relevance-known-class":
            method = "attributeRelevanceKnownClass"; break;
         case "java-tck.attribute-relevance-known-class-disabled":
            method = "attributeRelevanceKnownClassDisabled"; break;
         case "java-tck.update-rate-queries": method = "updateRateQueries"; break;
         case "java-tck.default-rate-subscription-no-reduction":
            method = "defaultRateSubscriptionNoReduction"; break;
         case "java-tck.federation-teardown-isolation":
            method = "federationTeardownIsolation"; break;
         case "java-tck.ordinary-edges": method = "ordinaryEdges"; break;
         case "java-tck.relevance-advisories": method = "relevanceAdvisories"; break;
         case "java-tck.directed-interactions": method = "directedInteractions"; break;
         case "java-tck.directed-interaction-subscription-accumulation":
            method = "directedInteractionSubscriptionAccumulation"; break;
         case "java-tck.directed-interaction-received-class-projection":
            method = "directedInteractionReceivedClassProjection"; break;
         case "java-tck.directed-known-object-class-fallback":
            method = "directedKnownObjectClassFallback"; break;
         case "java-tck.directed-interaction-no-applicable-subscription":
            method = "directedInteractionNoApplicableSubscription"; break;
         case "java-tck.delay-subscription-evaluation-interaction":
            method = "delaySubscriptionEvaluationInteraction"; break;
         case "java-tck.delay-subscription-evaluation-attribute-update":
            method = "delaySubscriptionEvaluationAttributeUpdate"; break;
         case "java-tck.delay-subscription-evaluation-receive-order-regional-attribute-update":
            method = "delaySubscriptionEvaluationRegionalAttributeUpdate"; break;
         case "java-tck.delay-subscription-evaluation-timestamped-attribute-update":
            method = "delaySubscriptionEvaluationTimestampedAttributeUpdate"; break;
         case "java-tck.delay-subscription-evaluation-timestamped-regional-attribute-update":
            method = "delaySubscriptionEvaluationTimestampedRegionalAttributeUpdate"; break;
         case "java-tck.delay-subscription-evaluation-timestamped-interaction":
            method = "delaySubscriptionEvaluationTimestampedInteraction"; break;
         case "java-tck.delay-subscription-evaluation-timestamped-regional-interaction":
            method = "delaySubscriptionEvaluationTimestampedRegionalInteraction"; break;
         case "java-tck.delay-subscription-evaluation-directed-interaction":
            method = "delaySubscriptionEvaluationDirectedInteraction"; break;
         case "java-tck.delay-subscription-evaluation-timestamped-directed-interaction":
            method = "delaySubscriptionEvaluationTimestampedDirectedInteraction"; break;
         case "java-tck.directed-interaction-declarations-valid-inputs-after-resignation":
            method = "directedInteractionDeclarationsAfterResignation"; break;
         case "java-tck.interaction-declarations-valid-inputs-after-resignation":
            method = "interactionDeclarationsAfterResignation"; break;
         case "java-tck.interaction-transportation-query-valid-inputs-after-resignation":
            method = "interactionTransportationQueryAfterResignation"; break;
         case "java-tck.directed-interaction-target-lifecycle":
            method = "directedInteractionTargetLifecycle"; break;
         case "java-tck.directed-interaction-publication-send-fence":
            method = "directedInteractionPublicationSendFence"; break;
         case "java-tck.transport-order": method = "transportOrder"; break;
         case "java-tck.mom-transportation-type-change-request":
            method = "momTransportationTypeChangeRequest"; break;
         case "java-tck.standard-order-transport-lookups-after-resignation":
            method = "standardOrderTransportLookupsAfterResignation"; break;
         case "java-tck.receive-order-attribute-update-callback-cancellation":
            method = "receiveOrderAttributeUpdateCallbackCancellation"; break;
         case "java-tck.receive-order-attribute-update-receiver-resignation-fence":
            method = "receiveOrderAttributeUpdateReceiverResignationFence"; break;
          case "java-tck.receive-order-multi-attribute-update-callback-cancellation":
             method = "receiveOrderMultiAttributeUpdateCallbackCancellation"; break;
          case "java-tck.receive-order-interaction-callback-cancellation":
             method = "receiveOrderInteractionCallbackCancellation"; break;
          case "java-tck.interaction-subscription-lifecycle":
             method = "interactionSubscriptionLifecycle"; break;
          case "java-tck.interaction-service-boundaries":
             method = "interactionServiceBoundaries"; break;
          case "java-tck.interaction-publication-send-fence":
             method = "interactionPublicationSendFence"; break;
          case "java-tck.receive-order-interaction":
             method = "receiveOrderInteraction"; break;
          case "java-tck.reliable-transport-one-copy-delivery":
             method = "reliableTransportOneCopyDelivery"; break;
          case "java-tck.unsupported-fom-transportation-fallback":
             method = "unsupportedFomTransportationFallback"; break;
          case "java-tck.interaction-order-classification":
             method = "interactionOrderClassification"; break;
          case "java-tck.custom-transportation-interaction-delivery":
             method = "customTransportationInteraction"; break;
          case "java-tck.custom-transportation-attribute-delivery":
             method = "customTransportationAttribute"; break;
          case "java-tck.custom-transportation-timestamped-attribute-delivery":
             method = "customTransportationTimestampedAttribute"; break;
          case "java-tck.custom-transportation-timestamped-interaction-delivery":
             method = "customTransportationTimestampedInteractionDelivery"; break;
          case "java-tck.custom-transportation-timestamped-attribute-alternate-advances":
             method = "customTransportationTimestampedAttributeAlternateAdvances"; break;
          case "java-tck.custom-transportation-timestamped-interaction-alternate-advances":
             method = "customTransportationTimestampedInteractionAlternateAdvances"; break;
          case "java-tck.custom-transportation-timestamped-directed-interaction-alternate-advances":
             method = "customTransportationTimestampedDirectedInteractionAlternateAdvances"; break;
          case "java-tck.custom-transportation-timestamped-regional-attribute-delivery":
             method = "customTransportationTimestampedRegionalAttribute"; break;
          case "java-tck.custom-transportation-timestamped-regional-interaction-delivery":
             method = "customTransportationTimestampedRegionalInteraction"; break;
          case "java-tck.custom-transportation-regional-interaction-delivery":
             method = "customTransportationRegionalInteraction"; break;
          case "java-tck.custom-transportation-regional-attribute-delivery":
             method = "customTransportationRegionalAttribute"; break;
          case "java-tck.custom-transportation-directed-interaction-delivery":
             method = "customTransportationDirectedInteraction"; break;
          case "java-tck.custom-transportation-timestamped-directed-delivery":
             method = "customTransportationTimestampedDirectedDelivery"; break;
          case "java-tck.interaction-publication-send-transition":
             method = "interactionPublicationSendTransition"; break;
           case "java-tck.interaction-active-and-passive-subscription-modes":
              method = "interactionActivePassiveSubscriptionModes"; break;
          case "java-tck.interaction-full-unpublication":
             method = "interactionFullUnpublication"; break;
          case "java-tck.interaction-multi-recipient-fifo":
             method = "interactionMultiRecipientFifo"; break;
          case "java-tck.directed-interaction-multi-recipient-fifo":
             method = "directedInteractionMultiRecipientFifo"; break;
          case "java-tck.directed-interaction-mixed-subscription-fanout":
             method = "directedInteractionMixedSubscriptionFanout"; break;
         case "java-tck.directed-interaction-subscription-kind":
            method = "directedInteractionSubscriptionKind"; break;
         case "java-tck.timestamped-directed-interaction-subscription-kind":
            method = "timestampedDirectedInteractionSubscriptionKind"; break;
         case "java-tck.directed-derived-object-target-eligibility":
            method = "directedDerivedObjectTargetEligibility"; break;
         case "java-tck.order-type-controls": method = "orderTypeControls"; break;
         case "java-tck.order-type-inherited-attribute-default-scope":
            method = "inheritedAttributeDefaultOrderScope"; break;
         case "java-tck.order-type-fdd-default-initialization":
            method = "fddAttributeOrderDefaultInitialization"; break;
         case "java-tck.order-type-default-order-class-handle-validation":
            method = "defaultOrderAttributeClassHandleValidation"; break;
         case "java-tck.api-surface-inventory": method = "apiSurfaceInventory"; break;
         case "java-tck.standard-enumeration-contract":
            method = "standardEnumerationContract"; break;
         case "java-tck.standard-exception-hierarchy":
            method = "standardExceptionHierarchyContract"; break;
         case "java-tck.exception-name-string": method = "exceptionNameString"; break;
         case "java-tck.null-federate-ambassador":
            method = "nullFederateAmbassadorContract"; break;
         case "java-tck.authorizer-factory-factory":
            method = "authorizerFactoryFactory"; break;
         case "java-tck.configuration-authorization-contract":
            method = "configurationAuthorizationContract"; break;
         case "java-tck.authorizer-connect-service-decision":
            method = "authorizerConnectServiceDecision"; break;
         case "java-tck.authorizer-unrecognized-credentials-unauthorized":
            method = "authorizerUnrecognizedCredentialsUnauthorized"; break;
         case "java-tck.authorizer-disabled-service-rejects-credentials":
            method = "authorizerDisabledServiceRejectsCredentials"; break;
         case "java-tck.authorizer-federation-service-decision":
            method = "authorizerFederationServiceDecision"; break;
         case "java-tck.authorizer-destroy-service-decision":
            method = "authorizerDestroyServiceDecision"; break;
         case "java-tck.authorizer-join-service-decision":
            method = "authorizerJoinServiceDecision"; break;
         case "java-tck.authorizer-no-credentials-connect":
            method = "authorizerNoCredentialsConnect"; break;
         case "java-tck.authorizer-rid-configuration-decision":
            method = "authorizerRidConfigurationDecision"; break;
         case "java-tck.authorizer-single-service-per-rid-profile":
            method = "authorizerSingleServicePerRidProfile"; break;
         case "java-tck.data-element-base-contract":
            method = "dataElementBaseContract"; break;
         case "java-tck.handle-collection-values":
            method = "handleCollectionsContract"; break;
         case "java-tck.overloads-and-exceptions": method = "overloadsAndExceptions"; break;
         case "java-tck.connection-additional-settings-parse-failure":
            method = "connectionAdditionalSettingsParseFailure"; break;
         case "java-tck.connection-default-configuration-fallback":
            method = "connectionDefaultConfigurationFallback"; break;
         case "java-tck.malformed-inputs": method = "malformedInputs"; break;
         case "java-tck.time-advance": method = "timeAdvanceAndGrants"; break;
         case "java-tck.non-regulating-federate-peer-advance-independence":
            method = "nonRegulatingFederatePeerAdvanceIndependence"; break;
         case "java-tck.pending-alternate-time-advance-state":
            method = "pendingAlternateTimeAdvanceState"; break;
         case "java-tck.time-bounds-queries": method = "timeBoundsQueries"; break;
         case "java-tck.time-constrained-enable-galt-equality":
            method = "timeConstrainedEnableGaltEquality"; break;
         case "java-tck.time-constrained-enable-regulating-federate-current-time":
            method = "timeConstrainedEnableRegulatingFederateCurrentTime"; break;
         case "java-tck.time-regulation-enable-actual-lookahead-tso-send":
            method = "timeRegulationEnableActualLookaheadTsoSend"; break;
         case "java-tck.time-regulation-enable-tso-cutoff":
            method = "timeRegulationEnableTsoCutoff"; break;
         case "java-tck.time-role-pending-enable":
            method = "timeRolePendingEnable"; break;
         case "java-tck.disable-time-regulation-send-order":
            method = "disableTimeRegulationSendOrder"; break;
         case "java-tck.time-constrained-enable-retains-queued-tso":
            method = "timeConstrainedEnableRetainsQueuedTso"; break;
         case "java-tck.time-constrained-enable-retains-queued-tso-attribute":
            method = "timeConstrainedEnableRetainsQueuedTsoAttribute"; break;
         case "java-tck.time-constrained-enable-retains-queued-tso-object-deletion":
            method = "timeConstrainedEnableRetainsQueuedTsoObjectDeletion"; break;
         case "java-tck.time-constrained-enable-receive-order-boundary":
            method = "timeConstrainedEnableReceiveOrderBoundary"; break;
         case "java-tck.time-constrained-disable-queued-interaction-to-ro":
            method = "timeConstrainedDisableQueuedInteractionToRo"; break;
         case "java-tck.time-constrained-disable-queued-attribute-to-ro":
            method = "timeConstrainedDisableQueuedAttributeToRo"; break;
         case "java-tck.time-constrained-disable-queued-deletion-to-ro":
            method = "timeConstrainedDisableQueuedDeletionToRo"; break;
         case "java-tck.time-constrained-disable-queued-directed-to-ro":
            method = "timeConstrainedDisableQueuedDirectedToRo"; break;
         case "java-tck.time-regulation-implicit-advance":
            method = "timeRegulationImplicitAdvance"; break;
         case "java-tck.query-lits-source-resignation":
            method = "queryLitsSourceResignation"; break;
         case "java-tck.next-message-request": method = "nextMessageRequest"; break;
         case "java-tck.nmra-no-pregrant-tso-floor":
            method = "nmraNoPregrantTsoFloor"; break;
         case "java-tck.nmra-ro-earliest-tso-cohort":
            method = "nmraRoEarliestTsoCohort"; break;
         case "java-tck.nmr-no-pregrant-tso-floor":
            method = "nmrNoPregrantTsoFloor"; break;
         case "java-tck.nmr-received-tso-floor":
            method = "nmrReceivedTsoFloor"; break;
         case "java-tck.nmra-received-tso-actual-lookahead-floor":
            method = "nmraReceivedTsoActualLookaheadFloor"; break;
         case "java-tck.nmr-received-tso-attribute-update-floor":
            method = "nmrReceivedTsoAttributeUpdateFloor"; break;
         case "java-tck.nmr-received-tso-directed-interaction-floor":
            method = "nmrReceivedTsoDirectedInteractionFloor"; break;
         case "java-tck.nmr-empty-message-set-grant-time":
            method = "nmrEmptyMessageSetGrantTime"; break;
         case "java-tck.nmra-no-tso-at-galt-grant-time":
            method = "nmraNoTsoAtGaltGrantTime"; break;
         case "java-tck.nmra-no-tso-undefined-galt-nrg":
            method = "nmraNoTsoUndefinedGaltNrg"; break;
         case "java-tck.nmra-future-tso-arrival-pending-request":
            method = "nmraFutureTsoArrivalPendingRequest"; break;
         case "java-tck.nmr-future-tso-arrival-while-pending":
            method = "nmrFutureTsoArrivalWhilePending"; break;
         case "java-tck.nmr-directed-future-tso-arrival-while-pending":
            method = "nmrDirectedFutureTsoArrivalWhilePending"; break;
         case "java-tck.nmra-directed-future-tso-arrival-while-pending":
            method = "nmraDirectedFutureTsoArrivalWhilePending"; break;
         case "java-tck.nmr-ro-earliest-tso-cohort":
            method = "nmrRoEarliestTsoCohort"; break;
         case "java-tck.nmr-request-below-galt-immediate-grant":
            method = "nmrRequestBelowGaltImmediateGrant"; break;
         case "java-tck.nmr-lits-below-galt-immediate-grant":
            method = "nmrLitsBelowGaltImmediateGrant"; break;
         case "java-tck.modify-lookahead-contract":
            method = "modifyLookaheadContract"; break;
         case "java-tck.modify-lookahead-service-report":
            method = "modifyLookaheadServiceReport"; break;
          case "java-tck.time-coordinated-interaction":
             method = "timeCoordinatedInteraction"; break;
         case "java-tck.unregulated-sender-timestamped-interaction":
            method = "unregulatedSenderTimestampedInteraction"; break;
         case "java-tck.unregulated-sender-timestamped-attribute-update":
            method = "unregulatedSenderTimestampedAttributeUpdate"; break;
         case "java-tck.timestamped-interaction-sender-regulation-transition":
            method = "timestampedInteractionSenderRegulationTransition"; break;
         case "java-tck.interaction-fdd-preferred-order-default":
            method = "interactionFddPreferredOrderDefault"; break;
         case "java-tck.attribute-fdd-preferred-order-default":
            method = "attributeFddPreferredOrderDefault"; break;
         case "java-tck.timestamped-interaction-mixed-advances":
            method = "timestampedInteractionMixedAdvances"; break;
          case "java-tck.timestamped-interaction-preferred-receive-conversion":
             method = "timestampedInteractionPreferredReceiveConversion"; break;
          case "java-tck.sent-tso-per-recipient-order-conversion":
             method = "sentTsoPerRecipientOrderConversion"; break;
         case "java-tck.sent-tso-attribute-per-recipient-order-conversion":
            method = "sentTsoAttributePerRecipientOrderConversion"; break;
         case "java-tck.timestamped-interaction-tar-equal-boundary":
            method = "timestampedInteractionTarEqualBoundary"; break;
         case "java-tck.timestamped-interaction-tar-post-grant-cutoff":
            method = "timestampedInteractionTarPostGrantCutoff"; break;
         case "java-tck.timestamped-attribute-update-tar-post-grant-cutoff":
            method = "timestampedAttributeUpdateTarPostGrantCutoff"; break;
         case "java-tck.timestamped-attribute-update-time-advancing-cutoff":
            method = "timestampedAttributeUpdateTimeAdvancingCutoff"; break;
         case "java-tck.timestamped-delete-privilege-preferred-order":
            method = "timestampedDeletePrivilegePreferredOrder"; break;
          case "java-tck.timestamped-delete-privilege-timestamp-order":
             method = "timestampedDeletePrivilegeTimestampOrder"; break;
         case "java-tck.timestamped-interaction-tara-equal-boundary":
            method = "timestampedInteractionTaraEqualBoundary"; break;
         case "java-tck.timestamped-interaction-tara-strict-timestamp-cutoff":
            method = "timestampedInteractionTaraStrictTimestampCutoff"; break;
         case "java-tck.timestamped-interaction-future-tara-arrival":
            method = "timestampedInteractionFutureTaraArrival"; break;
         case "java-tck.timestamped-interaction-zero-lookahead-send-at-grant":
            method = "timestampedInteractionZeroLookaheadSendAtGrant"; break;
         case "java-tck.zero-lookahead-tar-tso-equality-rejection":
            method = "zeroLookaheadTarTsoEqualityRejection"; break;
         case "java-tck.zero-lookahead-nmr-tso-equality-rejection":
            method = "zeroLookaheadNmrTsoEqualityRejection"; break;
         case "java-tck.zero-lookahead-nmr-continuation-tara-equality-acceptance":
            method = "zeroLookaheadNmrContinuationTaraEqualityAcceptance"; break;
         case "java-tck.zero-lookahead-nmra-tso-equality-acceptance":
            method = "zeroLookaheadNmraTsoEqualityAcceptance"; break;
         case "java-tck.tar-acknowledged-tso-galt-release":
            method = "tarAcknowledgedTsoGaltRelease"; break;
         case "java-tck.tar-multi-regulator-acknowledged-tso-progress":
            method = "tarMultiRegulatorAcknowledgedTsoProgress"; break;
         case "java-tck.timestamped-directed-zero-lookahead-tar-send-boundary":
            method = "timestampedDirectedZeroLookaheadTarSendBoundary"; break;
         case "java-tck.timestamped-directed-zero-lookahead-send-at-grant":
            method = "timestampedDirectedZeroLookaheadSendAtGrant"; break;
         case "java-tck.timestamped-interaction-zero-lookahead-tar-send-boundary":
            method = "timestampedInteractionZeroLookaheadTarSendBoundary"; break;
         case "java-tck.timestamped-interaction-tara-queued-equal-boundary":
            method = "timestampedInteractionTaraQueuedEqualBoundary"; break;
         case "java-tck.tara-undefined-galt-nrg":
            method = "taraUndefinedGaltNrg"; break;
         case "java-tck.tara-no-tso-undefined-galt-nrg":
            method = "taraNoTsoUndefinedGaltNrg"; break;
         case "java-tck.nmr-no-tso-undefined-galt-nrg":
            method = "nmrNoTsoUndefinedGaltNrg"; break;
          case "java-tck.fqr-undefined-galt-nrg":
             method = "fqrUndefinedGaltNrg"; break;
          case "java-tck.flush-queue-grant-optimistic-time-equal-when-galt-nonlimiting":
            method = "flushQueueGrantNonlimitingGalt"; break;
         case "java-tck.nrg-regulator-resignation-releases-constrained-tar":
            method = "nrgRegulatorResignation"; break;
         case "java-tck.nrg-regulator-disable-releases-constrained-tar":
            method = "nrgRegulatorDisable"; break;
         case "java-tck.nrg-disabled-regulator-activation-releases-constrained-tar":
            method = "nrgDisabledRegulatorActivation"; break;
         case "java-tck.constrained-tar-disable-releases-pending-grant":
            method = "constrainedTarDisableRelease"; break;
         case "java-tck.fqr-non-time-constrained-in-transit-tso":
            method = "fqrNonTimeConstrainedInTransitTso"; break;
         case "java-tck.flush-queue-timestamped-object-deletion-known-state":
            method = "fqrTimestampedObjectDeletionKnownState"; break;
         case "java-tck.timestamped-update-before-delete":
            method = "timestampedUpdateBeforeDelete"; break;
         case "java-tck.timestamped-attribute-reflect-region-designator-callback-snapshot":
            method = "timestampedAttributeReflectRegionDesignatorCallbackSnapshot"; break;
          case "java-tck.timestamped-interaction-flush-queue-future-input":
             method = "timestampedInteractionFlushQueueFutureInput"; break;
          case "java-tck.fqr-postgrant-lower-tso-retraction":
             method = "fqrPostgrantLowerTsoRetraction"; break;
          case "java-tck.fqg-postgrant-delivery-cutoff":
             method = "fqgPostgrantDeliveryCutoff"; break;
          case "java-tck.tar-queued-tso-before-grant":
             method = "tarQueuedTsoBeforeGrant"; break;
          case "java-tck.timestamped-interaction-tar-strict-timestamp-cutoff":
             method = "timestampedInteractionTarStrictTimestampCutoff"; break;
          case "java-tck.timestamped-interaction-tso-designator-terminalization":
             method = "timestampedInteractionTsoDesignatorTerminalization"; break;
          case "java-tck.timestamped-interaction-retraction":
             method = "timestampedInteractionRetraction"; break;
          case "java-tck.timestamped-interaction-publication-send-transition":
             method = "timestampedInteractionPublicationSendTransition"; break;
          case "java-tck.timestamped-interaction-cross-producer-order":
             method = "timestampedInteractionCrossProducerOrder"; break;
           case "java-tck.timestamped-interaction-no-fanout":
              method = "timestampedInteractionNoFanout"; break;
           case "java-tck.timestamped-interaction-source-resignation":
              method = "timestampedInteractionSourceResignation"; break;
           case "java-tck.timestamped-interaction-retraction-fanout":
              method = "timestampedInteractionRetractionFanout"; break;
           case "java-tck.timestamped-interaction-regulation-reenable":
              method = "timestampedInteractionRegulationReenable"; break;
           case "java-tck.timestamped-interaction-reenable":
              method = "timestampedInteractionConstrainedReenable"; break;
           case "java-tck.timestamped-directed-interaction-reenable":
              method = "timestampedDirectedInteractionConstrainedReenable"; break;
           case "java-tck.time-constrained-enable-retains-queued-tso-directed-interaction":
              method = "timeConstrainedEnableRetainsQueuedTsoDirectedInteraction"; break;
           case "java-tck.timestamped-directed-interaction-regulation-reenable":
              method = "timestampedDirectedInteractionRegulationReenable"; break;
           case "java-tck.timestamped-directed-interaction-tar-nmr":
              method = "timestampedDirectedInteractionTarNmr"; break;
           case "java-tck.timestamped-directed-interaction-source-resignation":
              method = "timestampedDirectedInteractionSourceResignation"; break;
           case "java-tck.timestamped-directed-interaction-source-resignation-fanout":
              method = "timestampedDirectedInteractionSourceResignationFanout"; break;
           case "java-tck.timestamped-directed-interaction-immediate-source-resignation":
              method = "timestampedDirectedInteractionImmediateSourceResignation"; break;
           case "java-tck.timestamped-directed-interaction-retraction-fanout":
              method = "timestampedDirectedInteractionRetractionFanout"; break;
           case "java-tck.directed-retraction-mixed-fanout":
              method = "directedRetractionMixedFanout"; break;
           case "java-tck.directed-retraction-immediate-only":
              method = "directedRetractionImmediateOnly"; break;
           case "java-tck.timestamped-directed-interaction-retraction":
              method = "timestampedDirectedInteractionRetraction"; break;
           case "java-tck.timestamped-directed-interactions":
              method = "timestampedDirectedInteractions"; break;
           case "java-tck.timestamped-directed-interaction-derived-object-target":
              method = "timestampedDirectedInteractionDerivedObjectTarget"; break;
           case "java-tck.timestamped-directed-alternate-advances":
              method = "timestampedDirectedAlternateAdvances"; break;
           case "java-tck.directed-interaction-derived-object-target":
              method = "directedInteractionDerivedObjectTarget"; break;
         case "java-tck.timestamped-attribute-update": method = "timestampedAttributeUpdate"; break;
         case "java-tck.timestamped-regional-attribute-update": method = "timestampedRegionalAttributeUpdate"; break;
         case "java-tck.timestamped-regional-attribute-alternate-advances": method = "timestampedRegionalAttributeAlternateAdvances"; break;
         case "java-tck.timestamped-default-region-attribute-alternate-advances": method = "timestampedDefaultRegionAttributeAlternateAdvances"; break;
         case "java-tck.timestamped-default-region-attribute-mixed-fanout": method = "timestampedDefaultRegionAttributeMixedFanout"; break;
         case "java-tck.timestamped-regional-attribute-association-replacement": method = "timestampedRegionalAttributeAssociationReplacement"; break;
         case "java-tck.timestamped-regional-attribute-regulation-reenable": method = "timestampedRegionalAttributeRegulationReenable"; break;
         case "java-tck.timestamped-default-region-attribute-regulation-reenable": method = "timestampedDefaultRegionAttributeRegulationReenable"; break;
         case "java-tck.timestamped-regional-attribute-source-resignation": method = "timestampedRegionalAttributeSourceResignation"; break;
         case "java-tck.regional-attribute-update-callback-ddm-recheck": method = "regionalAttributeUpdateCallbackDdmRecheck"; break;
         case "java-tck.default-region-class-request-equivalence": method = "defaultRegionClassRequestEquivalence"; break;
         case "java-tck.regional-attribute-value-request-filtering": method = "regionalAttributeValueRequestFiltering"; break;
         case "java-tck.regional-attribute-value-update-response-recheck": method = "regionalAttributeValueUpdateResponseRecheck"; break;
         case "java-tck.regional-object-update": method = "regionalObjectUpdate"; break;
         case "java-tck.regional-multi-attribute-update": method = "regionalMultiAttributeUpdate"; break;
         case "java-tck.regional-subscription-triple-isolation":
            method = "regionalSubscriptionTripleIsolation"; break;
         case "java-tck.regional-three-dimensional-overlap": method = "regionalThreeDimensionalOverlap"; break;
         case "java-tck.allow-relaxed-ddm": method = "allowRelaxedDdm"; break;
         case "java-tck.regional-declaration-relevance-advisories": method = "regionalDeclarationRelevanceAdvisories"; break;
         case "java-tck.attribute-scope-advisories": method = "attributeScopeAdvisories"; break;
         case "java-tck.attribute-scope-ownership-acquisition-no-out-of-scope":
            method = "attributeScopeOwnershipAcquisitionNoOutOfScope"; break;
         case "java-tck.default-region-object-routing": method = "defaultRegionObjectRouting"; break;
         case "java-tck.register-object-instance-with-regions-atomicity":
            method = "regionalRegistrationAtomicity"; break;
         case "java-tck.register-object-instance-with-regions-omitted-attributes-default-region":
            method = "regionalRegistrationOmittedAttributeDefaultRegion"; break;
         case "java-tck.register-object-instance-with-regions-empty-region-set-default-region":
            method = "regionalRegistrationEmptyRegionSetDefaultRegion"; break;
         case "java-tck.register-object-instance-with-regions-invalid-region":
            method = "regionalRegistrationInvalidRegion"; break;
         case "java-tck.associate-regions-for-updates-replaces-default-region":
            method = "associateRegionsForUpdatesReplacesDefaultRegion"; break;
         case "java-tck.default-region-registration-names": method = "defaultRegionRegistrationNames"; break;
         case "java-tck.timestamped-attribute-update-no-fanout":
            method = "timestampedAttributeUpdateNoFanout"; break;
         case "java-tck.timestamped-attribute-ownership-transfer":
            method = "timestampedAttributeOwnershipTransfer"; break;
         case "java-tck.timestamped-attribute-source-resignation":
            method = "timestampedAttributeSourceResignation"; break;
         case "java-tck.timestamped-attribute-source-resignation-fanout":
            method = "timestampedAttributeSourceResignationFanout"; break;
         case "java-tck.timestamped-local-delete-attribute":
            method = "timestampedLocalDeleteAttribute"; break;
         case "java-tck.timestamped-local-delete-object":
            method = "timestampedLocalDeleteObject"; break;
         case "java-tck.timestamped-attribute-order-cohort":
            method = "timestampedAttributeOrderCohort"; break;
          case "java-tck.timestamped-attribute-update-alternate-advances":
             method = "timestampedAttributeUpdateAlternateAdvances"; break;
         case "java-tck.timestamped-attribute-update-flush-queue-future-input":
            method = "timestampedAttributeUpdateFlushQueueFutureInput"; break;
         case "java-tck.timestamped-attribute-update-future-tara-arrival":
            method = "timestampedAttributeUpdateFutureTaraArrival"; break;
         case "java-tck.timestamped-attribute-update-future-nmr-arrival":
            method = "timestampedAttributeUpdateFutureNmrArrival"; break;
         case "java-tck.timestamped-attribute-update-future-nmra-arrival":
            method = "timestampedAttributeUpdateFutureNmraArrival"; break;
         case "java-tck.timestamped-attribute-update-queued-passel-retraction":
            method = "timestampedAttributeUpdateQueuedPasselRetraction"; break;
         case "java-tck.timestamped-attribute-immediate-recipient-multi-transport-retraction":
            method = "timestampedAttributeImmediateRecipientMultiTransportRetraction"; break;
         case "java-tck.mixed-order-attribute-passel-retraction":
            method = "mixedOrderAttributePasselRetraction"; break;
         case "java-tck.mixed-order-attribute-passels-separate-messages":
            method = "mixedOrderAttributePasselsSeparateMessages"; break;
         case "java-tck.timestamped-attribute-sender-regulation-transition":
            method = "timestampedAttributeSenderRegulationTransition"; break;
         case "java-tck.timestamped-attribute-update-reenable":
            method = "timestampedAttributeUpdateReenable"; break;
         case "java-tck.timestamped-attribute-update-regulation-reenable":
            method = "timestampedAttributeUpdateRegulationReenable"; break;
         case "java-tck.timestamped-object-removal": method = "timestampedObjectRemoval"; break;
         case "java-tck.timestamped-object-deletion-mixed-advances":
            method = "timestampedObjectDeletionMixedAdvances"; break;
         case "java-tck.timestamped-object-deletion-future-nmr-arrival":
            method = "timestampedObjectDeletionFutureNmrArrival"; break;
         case "java-tck.timestamped-object-deletion-future-nmra-arrival":
            method = "timestampedObjectDeletionFutureNmraArrival"; break;
         case "java-tck.timestamped-object-deletion-future-tara-arrival":
            method = "timestampedObjectDeletionFutureTaraArrival"; break;
         case "java-tck.timestamped-object-deletion-no-fanout":
            method = "timestampedObjectDeletionNoFanout"; break;
         case "java-tck.timestamped-object-deletion-tombstone":
            method = "timestampedObjectDeletionTombstone"; break;
         case "java-tck.timestamped-object-deletion-retraction":
            method = "timestampedObjectDeletionRetraction"; break;
         case "java-tck.timestamped-object-deletion-regulation-reenable":
            method = "timestampedObjectDeletionRegulationReenable"; break;
         case "java-tck.timestamped-object-deletion-retraction-joined-owners":
            method = "timestampedObjectDeletionRetractionJoinedOwners"; break;
         case "java-tck.timestamped-object-deletion-source-resignation-fanout":
            method = "timestampedObjectDeletionSourceResignationFanout"; break;
         case "java-tck.save-restore": method = "saveAndRestore"; break;
         case "java-tck.federation-save-restore-lifecycle":
            method = "federationSaveRestoreLifecycle"; break;
         case "java-tck.restore-save-after-federation-destruction":
            method = "restoreSaveAfterFederationDestruction"; break;
         case "java-tck.save-active-callback-resignation-guard":
            method = "saveActiveCallbackResignation"; break;
         case "java-tck.timed-save-active-callback-resignation-guard":
            method = "timedSaveActiveCallbackResignation"; break;
         case "java-tck.timed-save-fqr-active-callback-resignation-guard":
            method = "timedSaveFqrCallbackResignation"; break;
         case "java-tck.timed-save-tara-active-callback-resignation-guard":
            method = "timedSaveTaraCallbackResignation"; break;
         case "java-tck.timed-save-nmra-active-callback-resignation-guard":
            method = "timedSaveNmraCallbackResignation"; break;
         case "java-tck.timed-save-mixed-fqr-tar-members":
            method = "timedSaveMixedFqrTar"; break;
         case "java-tck.timed-save-ordinary-boundary-before-fqr":
            method = "timedSaveOrdinaryBeforeFqr"; break;
         case "java-tck.timed-save-all-advance-modes":
            method = "timedSaveAllAdvanceModes"; break;
         case "java-tck.timed-save-per-member-tso":
            method = "timedSavePerMemberTso"; break;
         case "java-tck.timed-save-flush-queue-admission":
            method = "timedSaveFlushQueueAdmission"; break;
         case "java-tck.timed-save-available-admission":
            method = "timedSaveAvailableAdmission"; break;
         case "java-tck.timed-save-next-message-admission":
            method = "timedSaveNextMessageAdmission"; break;
         case "java-tck.timed-save-available-next-message-member-admission":
            method = "timedSaveAvailableNextMessageMemberAdmission"; break;
         case "java-tck.timed-save-resigned-constrained-member":
            method = "timedSaveResignedConstrainedMember"; break;
         case "java-tck.timed-save-resigned-pending-fqr":
            method = "timedSaveResignedPendingFqr"; break;
         case "java-tck.restore-pending-flush-queue-request":
            method = "restorePendingFlushQueueRequest"; break;
         case "java-tck.restore-resignation-callback-lifetime":
            method = "restoreResignationCallbackLifetime"; break;
         case "java-tck.restore-pending-time-advance":
            method = "restorePendingTimeAdvance"; break;
         case "java-tck.restore-pending-time-regulation-callback":
            method = "restorePendingTimeRegulationCallback"; break;
         case "java-tck.restore-pending-time-constrained-callback":
            method = "restorePendingTimeConstrainedCallback"; break;
         case "java-tck.restore-mixed-pending-time-role-callbacks":
            method = "restoreMixedPendingTimeRoleCallbacks"; break;
         case "java-tck.restore-pending-time-advance-immediate":
            method = "restorePendingTimeAdvanceImmediate"; break;
         case "java-tck.restore-pending-advance-modes":
            method = "restorePendingAdvanceModes"; break;
         case "java-tck.restore-pending-tso-nmr":
            method = "restorePendingTsoNmr"; break;
         case "java-tck.restore-pending-tso-nmra":
            method = "restorePendingTsoNmra"; break;
         case "java-tck.restore-pending-tso-nmra-attribute-update":
            method = "restorePendingTsoNmraAttribute"; break;
         case "java-tck.restore-pending-tso-nmra-delete":
            method = "restorePendingTsoNmraDelete"; break;
         case "java-tck.restore-mixed-tso-four-member":
            method = "restoreMixedTsoFourMember"; break;
         case "java-tck.restore-terminal-tso-attribute":
            method = "restoreTerminalTsoAttribute"; break;
         case "java-tck.restore-terminal-tso-interaction":
            method = "restoreTerminalTsoInteraction"; break;
         case "java-tck.restore-terminal-tso-directed-interaction":
            method = "restoreTerminalTsoDirectedInteraction"; break;
         case "java-tck.restore-tso-retraction-uniqueness":
            method = "restoreTsoRetractionUniqueness"; break;
         case "java-tck.restore-tso-directed-multi-recipient":
            method = "restoreTsoDirectedMultiRecipient"; break;
         case "java-tck.restore-live-tso-retraction-record":
            method = "restoreLiveTsoRetractionRecord"; break;
         case "java-tck.restore-live-tso-directed-interaction":
            method = "restoreLiveTsoDirectedInteraction"; break;
         case "java-tck.restore-live-tso-interaction-multi-recipient":
            method = "restoreLiveTsoInteractionMultiRecipient"; break;
         case "java-tck.restore-live-tso-attribute-multi-recipient":
            method = "restoreLiveTsoAttributeMultiRecipient"; break;
         case "java-tck.restore-default-region-attribute-multi-recipient":
            method = "restoreDefaultRegionAttributeMultiRecipient"; break;
         case "java-tck.timed-restore-regional-attribute-multi-recipient":
            method = "timedRestoreRegionalAttributeMultiRecipient"; break;
         case "java-tck.restore-regional-interaction-multi-recipient":
            method = "restoreRegionalInteractionMultiRecipient"; break;
         case "java-tck.restore-time-window-state":
            method = "restoreTimeWindowState"; break;
         case "java-tck.restore-deferred-lookahead":
            method = "restoreDeferredLookahead"; break;
         case "java-tck.fqr-deferred-lookahead":
            method = "fqrDeferredLookahead"; break;
         case "java-tck.restore-multi-pending-time-advance":
            method = "restoreMultiPendingTimeAdvance"; break;
         case "java-tck.restore-pending-flush-queue-immediate":
            method = "restorePendingFlushQueueImmediate"; break;
         case "java-tck.restore-object-snapshot-status":
            method = "restoreObjectSnapshotStatus"; break;
         case "java-tck.federate-save-initiation-eligibility":
            method = "federateSaveInitiationEligibility"; break;
         case "java-tck.federate-save-initiation-time-constrained-eligibility":
            method = "federateSaveInitiationTimeConstrainedEligibility"; break;
         case "java-tck.federation-save-timestamp-eligibility":
            method = "federationSaveTimestampEligibility"; break;
         case "java-tck.federation-save-request-replacement":
            method = "federationSaveRequestReplacement"; break;
         case "java-tck.save-restore-queued-ro-interaction":
            method = "saveRestoreQueuedReceiveOrderInteraction"; break;
         case "java-tck.restore-federate-type-count-mismatch":
            method = "restoreFederateTypeCountMismatch"; break;
         case "java-tck.restore-federate-type-identity-rebinding":
            method = "restoreFederateTypeIdentityRebinding"; break;
         case "java-tck.timed-federation-save-restore":
            method = "timedFederationSaveRestore"; break;
         case "java-tck.fqr-timed-federation-save-callback":
            method = "fqrTimedFederationSaveCallback"; break;
          case "java-tck.federation-restore-work-item-ownership-assumption":
             method = "federationRestoreOwnershipAssumption"; break;
         case "java-tck.synchronization": method = "synchronizationPoint"; break;
         case "java-tck.synchronization-points-independent-tracking":
            method = "synchronizationPointIndependentTracking"; break;
         case "java-tck.synchronization-explicit-set-late-join":
            method = "synchronizationExplicitSetLateJoin"; break;
         case "java-tck.federation-mom-synchronization-queries":
            method = "federationMomSynchronizationQueries"; break;
         case "java-tck.callback-controls-synchronization":
            method = "callbackControlsSynchronization"; break;
         case "java-tck.callback-controls-time-advance":
            method = "callbackControlsTimeAdvance"; break;
         case "java-tck.callback-controls-save-restore":
            method = "callbackControlsSaveRestore"; break;
         case "java-tck.callback-controls-save-restore-failures":
            method = "callbackControlsSaveRestoreFailures"; break;
         case "java-tck.callback-controls-restore-request-failure":
            method = "callbackControlsRestoreRequestFailure"; break;
         case "java-tck.federation-restore-abort": method = "federationRestoreAbort"; break;
         case "java-tck.federation-save-restore-interlocks": method = "federationSaveRestoreInterlocks"; break;
         case "java-tck.federation-mom-save-status": method = "federationMomSaveStatus"; break;
         case "java-tck.federation-mom-save-conditionals": method = "federationMomSaveConditionals"; break;
         case "java-tck.joined-federate-mom-federate-state-save-restore":
            method = "joinedFederateMomFederateStateSaveRestore"; break;
         case "java-tck.mom-save-restore-interactions":
            method = "momSaveRestoreInteractions"; break;
         case "java-tck.callback-controls-timestamped-attribute-update":
            method = "callbackControlsTimestampedAttributeUpdate"; break;
         case "java-tck.callback-controls-attribute-update":
            method = "callbackControlsAttributeUpdate"; break;
         case "java-tck.callback-controls-timestamped-object-removal":
            method = "callbackControlsTimestampedObjectRemoval"; break;
         case "java-tck.callback-controls-timestamped-retraction":
            method = "callbackControlsTimestampedRetraction"; break;
         case "java-tck.callback-controls-transportation":
            method = "callbackControlsTransportation"; break;
         case "java-tck.callback-controls-directed-interaction":
            method = "callbackControlsDirectedInteraction"; break;
         case "java-tck.callback-controls-federation-reports":
            method = "callbackControlsFederationReports"; break;
         case "java-tck.callback-controls-declaration-advisories":
            method = "callbackControlsDeclarationAdvisories"; break;
         case "java-tck.callback-controls-attribute-relevance-advisories":
            method = "callbackControlsAttributeRelevanceAdvisories"; break;
         case "java-tck.callback-controls-attribute-value-request":
            method = "callbackControlsAttributeValueRequest"; break;
         case "java-tck.callback-controls-auto-provide":
            method = "callbackControlsAutoProvide"; break;
         case "java-tck.auto-provide": method = "autoProvide"; break;
         case "java-tck.federation-auto-provide-mom-switch":
            method = "federationAutoProvideMomSwitch"; break;
         case "java-tck.federate-service-reporting-mom-switch":
            method = "federateServiceReportingMomSwitch"; break;
         case "java-tck.federate-mom-set-switches-extra-parameter":
            method = "federateMomSetSwitchesExtraParameter"; break;
         case "java-tck.federate-mom-category-c-interaction":
            method = "federateMomCategoryCInteraction"; break;
         case "java-tck.mom-predefined-object-and-interaction":
            method = "momPredefinedObjectAndInteraction"; break;
         case "java-tck.mom-table-class-structure":
            method = "momTableClassStructure"; break;
         case "java-tck.mom-table-information-not-altered":
            method = "momTableInformationNotAltered"; break;
         case "java-tck.mom-table-annex-g-precedence-service-report-file":
            method = "momTableAnnexGPrecedenceServiceReportFile"; break;
         case "java-tck.service-report-file-switch-lifecycle":
            method = "serviceReportFileSwitchLifecycle"; break;
         case "java-tck.service-report-service-group-file":
            method = "serviceReportServiceGroupFile"; break;
         case "java-tck.service-report-federation-execution-information-file":
            method = "serviceReportFederationExecutionInformationFile"; break;
         case "java-tck.service-report-federation-execution-members-file":
            method = "serviceReportFederationExecutionMembersFile"; break;
         case "java-tck.service-report-save-status-file":
            method = "serviceReportSaveStatusFile"; break;
         case "java-tck.service-report-save-failure-reason-file":
            method = "serviceReportSaveFailureReasonFile"; break;
         case "java-tck.mom-service-optional-attribute-list-unsubscribe":
            method = "momServiceOptionalAttributeListUnsubscribe"; break;
         case "java-tck.mom-ownership-transfer-prohibition":
            method = "momOwnershipTransferProhibition"; break;
         case "java-tck.federate-send-service-reports-to-file-mom-switch":
            method = "federateSendServiceReportsToFileMomSwitch"; break;
         case "java-tck.federate-automatic-resign-action-mom-switch":
            method = "federateAutomaticResignActionMomSwitch"; break;
         case "java-tck.federate-automatic-resign-action-mim-attribute":
            method = "federateAutomaticResignActionMomAttribute"; break;
         case "java-tck.federate-exception-reporting-mom-switch":
            method = "federateExceptionReportingMomSwitch"; break;
         case "java-tck.federate-exception-report-delivery":
            method = "federateExceptionReportDelivery"; break;
         case "java-tck.federate-mom-exception-report-delivery":
            method = "federateMomExceptionReportDelivery"; break;
         case "java-tck.federate-mom-exception-missing-parameter":
            method = "federateMomExceptionMissingParameter"; break;
         case "java-tck.federation-mom-exception-missing-fom-module-indicator":
            method = "federationMomExceptionMissingFomModuleIndicator"; break;
         case "java-tck.federate-mom-exception-report-service-precondition":
            method = "federateMomExceptionReportServicePrecondition"; break;
         case "java-tck.mom-modify-attribute-state":
            method = "momModifyAttributeState"; break;
         case "java-tck.federate-exception-reporting-mim-attribute":
            method = "federateExceptionReportingMomAttribute"; break;
         case "java-tck.federate-service-reporting-mim-attribute":
            method = "federateServiceReportingMomAttribute"; break;
         case "java-tck.federate-object-class-relevance-advisory-mim-attribute":
            method = "federateObjectClassRelevanceAdvisoryMomAttribute"; break;
         case "java-tck.federate-attribute-relevance-advisory-mim-attribute":
            method = "federateAttributeRelevanceAdvisoryMomAttribute"; break;
         case "java-tck.federate-attribute-scope-advisory-mim-attribute":
            method = "federateAttributeScopeAdvisoryMomAttribute"; break;
         case "java-tck.federate-interaction-relevance-advisory-mim-attribute":
            method = "federateInteractionRelevanceAdvisoryMomAttribute"; break;
         case "java-tck.federate-asynchronous-delivery-mim-attribute":
            method = "federateAsynchronousDeliveryMomAttribute"; break;
         case "java-tck.federate-time-constrained-mim-attribute":
            method = "federateTimeConstrainedMomAttribute"; break;
         case "java-tck.federate-time-regulating-mim-attribute":
            method = "federateTimeRegulatingMomAttribute"; break;
         case "java-tck.federate-time-manager-state-mim-attribute":
            method = "federateTimeManagerStateMomAttribute"; break;
         case "java-tck.federate-logical-time-mim-periodic":
            method = "federateLogicalTimeMomPeriodic"; break;
         case "java-tck.federate-lookahead-mim-periodic":
            method = "federateLookaheadMomPeriodic"; break;
         case "java-tck.federate-mom-galt-lits-periodic":
            method = "federateMomGaltLitsPeriodic"; break;
         case "java-tck.federate-mom-tso-length-periodic":
            method = "federateMomTsoLengthPeriodic"; break;
         case "java-tck.federate-mom-ro-length-periodic":
            method = "federateMomRoLengthPeriodic"; break;
         case "java-tck.federate-mom-interactions-received-mim-attribute":
            method = "federateMomInteractionsReceived"; break;
          case "java-tck.federate-mom-interactions-received-counts":
            method = "federateMomInteractionsReceivedCounts"; break;
          case "java-tck.federate-mom-interactions-sent-mim-attribute":
            method = "federateMomInteractionsSent"; break;
          case "java-tck.federate-mom-interactions-sent-counts":
            method = "federateMomInteractionsSentCounts"; break;
          case "java-tck.federate-mom-directed-interactions-sent-mim-attribute":
            method = "federateMomDirectedInteractionsSent"; break;
          case "java-tck.federate-mom-directed-interactions-received-mim-attribute":
            method = "federateMomDirectedInteractionsReceived"; break;
          case "java-tck.federate-mom-directed-interactions-received-counts":
            method = "federateMomDirectedInteractionsReceivedCounts"; break;
          case "java-tck.federate-mom-directed-interactions-sent-counts":
            method = "federateMomDirectedInteractionsSentCounts"; break;
          case "java-tck.federate-mom-updates-sent-mim-attribute":
            method = "federateMomUpdatesSent"; break;
          case "java-tck.federate-mom-updates-sent-counts":
            method = "federateMomUpdatesSentCounts"; break;
          case "java-tck.federate-mom-object-instances-updated-mim-attribute":
            method = "federateMomObjectInstancesUpdated"; break;
          case "java-tck.federate-mom-updated-object-count-periodic":
            method = "federateMomUpdatedObjectCountPeriodic"; break;
          case "java-tck.federate-mom-object-instances-reflected-mim-attribute":
            method = "federateMomObjectInstancesReflected"; break;
          case "java-tck.federate-mom-object-instances-discovered-mim-attribute":
            method = "federateMomObjectInstancesDiscovered"; break;
          case "java-tck.federate-mom-object-instances-removed-mim-attribute":
            method = "federateMomObjectInstancesRemoved"; break;
          case "java-tck.federate-mom-object-instances-deleted-mim-attribute":
            method = "federateMomObjectInstancesDeleted"; break;
          case "java-tck.federate-mom-time-state-durations":
            method = "federateMomTimeStateDurations"; break;
          case "java-tck.federate-mom-object-lifecycle":
            method = "federateMomObjectLifecycle"; break;
          case "java-tck.federate-mom-report-service-file":
            method = "federateMomReportServiceFile"; break;
          case "java-tck.federate-mom-deletable-object-count":
            method = "federateMomDeletableObjectCount"; break;
          case "java-tck.federate-mom-deletable-object-report":
            method = "federateMomDeletableObjectReport"; break;
          case "java-tck.federate-mom-updated-object-report":
            method = "federateMomUpdatedObjectReport"; break;
          case "java-tck.federate-mom-reflected-object-report":
            method = "federateMomReflectedObjectReport"; break;
          case "java-tck.federate-mom-reflected-timestamped-object-report":
            method = "federateMomReflectedTimestampedObjectReport"; break;
          case "java-tck.federate-mom-updated-timestamped-object-report":
            method = "federateMomUpdatedTimestampedObjectReport"; break;
          case "java-tck.federate-mom-updated-null-object-report":
            method = "federateMomUpdatedNullObjectReport"; break;
          case "java-tck.federate-mom-reflected-null-object-report":
            method = "federateMomReflectedNullObjectReport"; break;
          case "java-tck.federate-mom-reflected-multi-class-object-report":
            method = "federateMomReflectedMultiClassObjectReport"; break;
          case "java-tck.federate-mom-updated-multi-class-object-report":
            method = "federateMomUpdatedMultiClassObjectReport"; break;
          case "java-tck.federate-mom-reflections-received-mim-attribute":
            method = "federateMomReflectionsReceived"; break;
          case "java-tck.federate-mom-reflections-received-counts":
            method = "federateMomReflectionsReceivedCounts"; break;
          case "java-tck.federate-mom-reflection-counts":
            method = "federateMomReflectionCounts"; break;
         case "java-tck.regional-auto-provide-response": method = "regionalAutoProvideResponse"; break;
         case "java-tck.regional-auto-provide-timestamped-response":
            method = "regionalAutoProvideTimestampedResponse"; break;
         case "java-tck.regional-auto-provide-overlap":
            method = "regionalAutoProvideOverlap"; break;
         case "java-tck.regional-auto-provide-multi-source":
            method = "regionalAutoProvideMultiSource"; break;
         case "java-tck.regional-auto-provide-multi-provider":
            method = "regionalAutoProvideMultiProvider"; break;
         case "java-tck.regional-auto-provide-callback-control":
            method = "regionalAutoProvideCallbackControl"; break;
         case "java-tck.auto-provide-disabled-explicit-request":
            method = "autoProvideDisabledExplicitRequest"; break;
         case "java-tck.auto-provide-disabled-discovery-only":
            method = "autoProvideDisabledDiscoveryOnly"; break;
         case "java-tck.callback-controls-ownership-assumption":
            method = "callbackControlsOwnershipAssumption"; break;
         case "java-tck.callback-controls-ownership-unavailable":
            method = "callbackControlsOwnershipUnavailable"; break;
         case "java-tck.callback-controls-ownership-release-request":
            method = "callbackControlsOwnershipReleaseRequest"; break;
         case "java-tck.callback-controls-ownership-acquisition-cancellation":
            method = "callbackControlsOwnershipAcquisitionCancellation"; break;
         case "java-tck.callback-controls-ownership-release-denied":
            method = "callbackControlsOwnershipReleaseDenied"; break;
         case "java-tck.callback-controls-ownership-acquisition-notification":
            method = "callbackControlsOwnershipAcquisitionNotification"; break;
         case "java-tck.callback-controls-ownership-divestiture-confirmation":
            method = "callbackControlsOwnershipDivestitureConfirmation"; break;
         case "java-tck.callback-controls-ownership-query":
            method = "callbackControlsOwnershipQuery"; break;
         case "java-tck.callback-controls-object-removal":
            method = "callbackControlsObjectRemoval"; break;
         case "java-tck.callback-controls-object-discovery":
            method = "callbackControlsObjectDiscovery"; break;
         case "java-tck.callback-controls-object-name-reservation":
            method = "callbackControlsObjectNameReservation"; break;
         case "java-tck.callback-controls-object-name-reservation-failure":
            method = "callbackControlsObjectNameReservationFailure"; break;
         case "java-tck.callback-controls-multiple-object-name-reservation":
            method = "callbackControlsMultipleObjectNameReservation"; break;
         case "java-tck.callback-controls-interaction":
            method = "callbackControlsInteraction"; break;
         case "java-tck.callback-controls-time-role-enablement":
            method = "callbackControlsTimeRoleEnablement"; break;
         case "java-tck.callback-controls-available-time-advance":
            method = "callbackControlsAvailableTimeAdvance"; break;
         case "java-tck.callback-controls-flush-queue-grant":
            method = "callbackControlsFlushQueueGrant"; break;
         case "java-tck.mom": method = "momStatusReport"; break;
         case "java-tck.federate-mom-report-added-subclass-not-sent":
            method = "federateMomReportAddedSubclassNotSent"; break;
         case "java-tck.federate-mom-report-added-subclass-full-interaction":
            method = "federateMomReportAddedSubclassFullInteraction"; break;
         case "java-tck.federate-mom-report-added-subclass-parent-promotion-drops-extension-parameter":
            method = "federateMomReportAddedSubclassParentPromotion"; break;
         case "java-tck.service-report-invocation-extension-parameter-not-sent":
            method = "serviceReportInvocationExtensionParameter"; break;
         case "java-tck.service-report-interaction": method = "serviceReportInteraction"; break;
          case "java-tck.service-report-query-galt": method = "serviceReportQueryGalt"; break;
         case "java-tck.service-report-next-message-request-available-boundary":
            method = "serviceReportNextMessageRequestAvailable"; break;
         case "java-tck.service-report-order-transportation-lookups":
            method = "serviceReportOrderTransportationLookups"; break;
         case "java-tck.service-report-order-transportation-lookup-failures":
            method = "serviceReportOrderTransportationLookupFailures"; break;
         case "java-tck.service-report-interaction-parameter-lookups":
            method = "serviceReportInteractionParameterLookups"; break;
         case "java-tck.service-report-object-attribute-update-rate-lookups":
            method = "serviceReportObjectAttributeUpdateRateLookups"; break;
         case "java-tck.service-report-dimension-upper-bound":
            method = "serviceReportDimensionUpperBound"; break;
         case "java-tck.service-report-dimension-name":
            method = "serviceReportDimensionName"; break;
         case "java-tck.service-report-dimension-handle":
            method = "serviceReportDimensionHandle"; break;
         case "java-tck.service-report-dimension-handle-set":
            method = "serviceReportDimensionHandleSet"; break;
         case "java-tck.service-report-range-bounds":
            method = "serviceReportRangeBounds"; break;
         case "java-tck.service-report-set-range-bounds":
            method = "serviceReportSetRangeBounds"; break;
         case "java-tck.service-report-commit-region-modifications":
            method = "serviceReportCommitRegionModifications"; break;
         case "java-tck.service-report-delete-region":
            method = "serviceReportDeleteRegion"; break;
         case "java-tck.service-report-create-region":
            method = "serviceReportCreateRegion"; break;
         case "java-tck.service-report-interaction-dimensions":
            method = "serviceReportInteractionDimensions"; break;
         case "java-tck.service-report-interaction-dimensions-invalid-handle-contexts":
            method = "serviceReportInteractionDimensionsInvalidClassContext"; break;
         case "java-tck.service-report-object-dimensions":
            method = "serviceReportObjectDimensions"; break;
         case "java-tck.service-report-object-dimensions-invalid-handle-contexts":
            method = "serviceReportObjectDimensionsInvalidClassContext"; break;
         case "java-tck.service-report-federate-object-class-lookups":
            method = "serviceReportFederateObjectClassLookups"; break;
         case "java-tck.service-report-federate-object-class-lookup-failures":
            method = "serviceReportFederateObjectClassLookupFailures"; break;
         case "java-tck.service-report-interaction-parameter-lookup-failures":
            method = "serviceReportInteractionParameterLookupFailures"; break;
         case "java-tck.service-report-object-attribute-update-rate-lookup-failures":
            method = "serviceReportObjectAttributeUpdateRateLookupFailures"; break;
         case "java-tck.service-report-interaction-publication":
            method = "serviceReportInteractionPublication"; break;
         case "java-tck.service-report-interaction-subscription":
            method = "serviceReportInteractionSubscription"; break;
         case "java-tck.service-report-directed-interaction-declaration":
            method = "serviceReportDirectedInteractionDeclaration"; break;
         case "java-tck.service-report-directed-interaction-publication":
            method = "serviceReportDirectedInteractionPublication"; break;
         case "java-tck.service-report-object-attribute-declaration":
            method = "serviceReportObjectAttributeDeclaration"; break;
         case "java-tck.service-report-request-attribute-value-update":
            method = "serviceReportRequestAttributeValueUpdate"; break;
         case "java-tck.service-report-local-delete-object-instance":
            method = "serviceReportLocalDelete"; break;
         case "java-tck.service-report-local-delete-object-instance-failure":
            method = "serviceReportLocalDeleteFailure"; break;
         case "java-tck.service-report-register-object-instance":
            method = "serviceReportObjectRegistration"; break;
         case "java-tck.service-report-delete-object-instance":
            method = "serviceReportObjectDeletion"; break;
         case "java-tck.service-report-delete-object-instance-failure":
            method = "serviceReportObjectDeletionFailure"; break;
         case "java-tck.service-report-reserve-object-instance-name":
            method = "serviceReportObjectNameReservation"; break;
         case "java-tck.object-name-reservation-lifecycle":
            method = "objectNameReservationLifecycle"; break;
         case "java-tck.named-registration":
            method = "namedRegistration"; break;
         case "java-tck.final-federate-resignation-cleanup":
            method = "finalFederateResignationCleanup"; break;
         case "java-tck.service-report-release-object-instance-name":
            method = "serviceReportObjectNameRelease"; break;
         case "java-tck.service-report-release-multiple-object-instance-names":
            method = "serviceReportMultipleObjectNameRelease"; break;
         case "java-tck.service-report-cancel-attribute-ownership-acquisition":
            method = "serviceReportCancelOwnershipAcquisition"; break;
         case "java-tck.service-report-query-attribute-ownership":
            method = "serviceReportQueryAttributeOwnership"; break;
         case "java-tck.service-report-confirm-divestiture":
            method = "serviceReportConfirmDivestiture"; break;
         case "java-tck.service-report-synchronization":
            method = "serviceReportSynchronization"; break;
         case "java-tck.service-report-interlock":
            method = "serviceReportInterlock"; break;
         case "java-tck.service-report-interaction-failure":
            method = "serviceReportInteractionFailure"; break;
         case "java-tck.service-report-timestamped-attribute-update":
            method = "serviceReportTimestampedAttributeUpdate"; break;
         case "java-tck.service-report-timestamped-directed-interaction":
            method = "serviceReportTimestampedDirectedInteraction"; break;
         case "java-tck.service-report-timestamped-object-deletion":
            method = "serviceReportTimestampedObjectDeletion"; break;
         case "java-tck.service-report-timestamped-object-deletion-failure":
            method = "serviceReportTimestampedObjectDeletionFailure"; break;
         case "java-tck.service-report-attribute-update":
            method = "serviceReportAttributeUpdate"; break;
         case "java-tck.service-report-federate-save-begun":
            method = "serviceReportFederateSaveBegun"; break;
         case "java-tck.service-report-attribute-update-failure":
            method = "serviceReportAttributeUpdateFailure"; break;
         case "java-tck.service-report-timestamped-attribute-update-failure":
            method = "serviceReportTimestampedAttributeUpdateFailure"; break;
         case "java-tck.service-report-timestamped-interaction-failure":
            method = "serviceReportTimestampedInteractionFailure"; break;
         case "java-tck.service-report-timestamped-interaction":
            method = "serviceReportTimestampedInteraction"; break;
         case "java-tck.service-report-resign-federation-execution":
            method = "serviceReportResignFederationExecution"; break;
         case "java-tck.ddm": method = "dataDistributionManagement"; break;
         case "java-tck.ddm-available-attribute-services":
            method = "ddmAvailableAttributeServices"; break;
         case "java-tck.ddm-unsubscribe-preserves-other-class-subscription":
            method = "ddmUnsubscribePreservesOtherClassSubscription"; break;
         case "java-tck.ownership": method = "ownershipManagement"; break;
         case "java-tck.privilege-to-delete-transfer": method = "privilegeToDeleteTransfer"; break;
         case "java-tck.privilege-to-delete-publication-eligibility": method = "privilegeToDeletePublicationEligibility"; break;
         case "java-tck.ownership-transfer-regional-update": method = "ownershipTransferRegionalUpdate"; break;
         case "java-tck.ownership-transfer-deferred-regional-update": method = "ownershipTransferDeferredRegionalUpdate"; break;
         case "java-tck.partial-attribute-ownership-transfer":
            method = "partialAttributeOwnershipTransfer"; break;
         case "java-tck.divestiture-if-wanted-mixed-acquirers":
            method = "divestitureIfWantedMixedAcquirers"; break;
         case "java-tck.divestiture-if-wanted-return-set":
            method = "divestitureIfWantedReturnSet"; break;
         case "java-tck.unconditional-attribute-ownership-divestiture":
            method = "unconditionalAttributeOwnershipDivestiture"; break;
         case "java-tck.unconditional-divestiture-assumption-coalescing":
            method = "unconditionalDivestitureAssumptionCoalescing"; break;
         case "java-tck.attribute-ownership-release-coalescing":
            method = "attributeOwnershipReleaseCoalescing"; break;
         case "java-tck.ownership-acquisition-publication-fence":
            method = "ownershipAcquisitionPublicationFence"; break;
         case "java-tck.acquisition-publication-guard-unavailable":
            method = "acquisitionPublicationGuardUnavailable"; break;
         case "java-tck.selective-unpublication-pending-acquisition":
            method = "selectiveUnpublicationPendingAcquisition"; break;
          case "java-tck.attribute-ownership-acquisition-cancellation":
             method = "attributeOwnershipAcquisitionCancellation"; break;
          case "java-tck.ownership-cancellation-idle":
             method = "ownershipCancellationIdle"; break;
          case "java-tck.attribute-ownership-release-denied":
             method = "attributeOwnershipReleaseDenied"; break;
          case "java-tck.ownership-denial-idle":
             method = "ownershipDenialIdle"; break;
          case "java-tck.ownership-denial-query":
             method = "ownershipDenialQuery"; break;
          case "java-tck.ownership-transition-query":
             method = "ownershipTransitionQuery"; break;
          case "java-tck.ownership-reacquisition-query":
             method = "ownershipReacquisitionQuery"; break;
          case "java-tck.ownership-competing-if-available-query":
             method = "ownershipCompetingIfAvailableQuery"; break;
          case "java-tck.ownership-mixed-if-available-query":
             method = "ownershipMixedIfAvailableQuery"; break;
          case "java-tck.ownership-unpublish-owned":
             method = "ownershipUnpublishOwned"; break;
          case "java-tck.attribute-ownership-query-owner-report":
             method = "attributeOwnershipQueryOwnerReport"; break;
         case "java-tck.ownership-acquisition-if-available":
            method = "ownershipAcquisitionIfAvailable"; break;
         case "java-tck.ownership-query-partition-cleanup":
            method = "ownershipQueryPartitionCleanup"; break;
         case "java-tck.ownership-query-result-nullification":
            method = "ownershipQueryResultNullification"; break;
         case "java-tck.ownership-state-save-restore":
            method = "ownershipStateSaveRestore"; break;
         case "java-tck.pending-negotiated-ownership-save-restore":
            method = "pendingNegotiatedOwnershipSaveRestore"; break;
         case "java-tck.ownership-acquisition-cancellation-save-restore":
            method = "ownershipAcquisitionCancellationSaveRestore"; break;
         case "java-tck.if-available-ownership-restore-unavailable-callback":
            method = "ifAvailableOwnershipRestoreUnavailable"; break;
         case "java-tck.ownership-query-save-restore-results":
            method = "ownershipQuerySaveRestoreResults"; break;
         case "java-tck.ownership-acquisition-cancellation-transfer-race":
            method = "ownershipAcquisitionCancellationTransferRace"; break;
         case "java-tck.ownership-acquisition-cancellation-denial-race":
            method = "ownershipAcquisitionCancellationDenialRace"; break;
         case "java-tck.ownership-acquisition-cancellation-mixed-replies":
            method = "ownershipAcquisitionCancellationMixedReplies"; break;
         case "java-tck.negotiated-divestiture-partial-acquisition-cancellation":
            method = "negotiatedDivestiturePartialAcquisitionCancellation"; break;
         case "java-tck.negotiated-divestiture-no-accepter":
            method = "negotiatedDivestitureNoAccepter"; break;
         case "java-tck.ownership-acquisition-while-owner-waits-for-new-owner":
            method = "ownershipAcquisitionWhileOwnerWaitsForNewOwner"; break;
         case "java-tck.ownership-repeat-acquisition-pending-preserves-state":
            method = "ownershipRepeatAcquisitionPending"; break;
         case "java-tck.negotiated-divestiture-pre-delivery-cancellation":
            method = "negotiatedDivestiturePreDeliveryCancellation"; break;
         case "java-tck.negotiated-divestiture-cancellation":
            method = "negotiatedDivestitureCancellation"; break;
         case "java-tck.negotiated-divestiture-duplicate-request":
            method = "negotiatedDivestitureDuplicateRequest"; break;
         case "java-tck.negotiated-divestiture-if-wanted-completion":
            method = "negotiatedDivestitureIfWantedCompletion"; break;
         case "java-tck.negotiated-divestiture-confirmation-coalescing":
            method = "negotiatedDivestitureConfirmationCoalescing"; break;
          case "java-tck.negotiated-divestiture-confirmation-flow":
             method = "negotiatedDivestitureConfirmationFlow"; break;
         case "java-tck.negotiated-willing-to-acquire-continuation":
            method = "negotiatedWillingToAcquireContinuation"; break;
         case "java-tck.if-available-during-negotiated-divestiture":
            method = "ifAvailableDuringNegotiatedDivestiture"; break;
         case "java-tck.if-available-deleted-object":
            method = "ifAvailableDeletedObject"; break;
         case "java-tck.ownership-after-fom-merge":
            method = "ownershipAfterFomMerge"; break;
         case "java-tck.resign-divests-all-instance-attributes":
            method = "resignDivestsAllInstanceAttributes"; break;
         case "java-tck.resign-pending-acquisition-rejection":
            method = "resignPendingAcquisitionRejection"; break;
         case "java-tck.resign-cancel-if-available-pending":
            method = "resignCancelIfAvailablePending"; break;
         case "java-tck.resign-cancel-negotiated-pending":
            method = "resignCancelNegotiatedPending"; break;
         default: method = "unsupportedPortableFamily"; break;
      }
      return "org.hla.rti.tck.RtiTckMain#" + method;
   }
}
