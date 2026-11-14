package org.hla.rti.tck;

/** Owns registrations for callback enable and disable coverage. */
final class CallbackControlsScenarioRegistry {
   private CallbackControlsScenarioRegistry() { }

   static void registerSynchronizationAndLifecycle() {
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-synchronization",
            "gate synchronization callbacks with callback enable and disable", "callback-controls",
            "RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.registerFederationSynchronizationPoint;RTIambassador.synchronizationPointAchieved;FederateAmbassador.announceSynchronizationPoint;FederateAmbassador.federationSynchronized",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-federation-synchronization"
            }, false), RtiTckMain::callbackControlsSynchronization);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-time-advance",
            "gate a time-advance grant with callback enable and disable", "callback-controls",
            "RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.timeAdvanceRequest;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-time-management"
            }, false), RtiTckMain::callbackControlsTimeAdvance);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-save-restore",
            "gate federation save and restore callbacks with callback enable and disable",
            "callback-controls",
            "RTIambassador.requestFederationSave;RTIambassador.federateSaveBegun;RTIambassador.federateSaveComplete;RTIambassador.queryFederationSaveStatus;RTIambassador.requestFederationRestore;RTIambassador.queryFederationRestoreStatus;RTIambassador.federateRestoreComplete;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.initiateFederateSave;FederateAmbassador.federationSaved;FederateAmbassador.federationSaveStatusResponse;FederateAmbassador.requestFederationRestoreSucceeded;FederateAmbassador.federationRestoreBegun;FederateAmbassador.initiateFederateRestore;FederateAmbassador.federationRestoreStatusResponse;FederateAmbassador.federationRestored",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-federation-save-lifecycle",
               "cpp-1516.1-federation-save-status",
               "cpp-1516.1-federation-restore-lifecycle",
               "cpp-1516.1-federation-restore-status",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsSaveRestore);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-save-restore-failures",
            "gate federate-reported save and restore failures with callback enable and disable",
            "callback-controls",
            "RTIambassador.requestFederationSave;RTIambassador.federateSaveBegun;RTIambassador.federateSaveNotComplete;RTIambassador.federateSaveComplete;RTIambassador.requestFederationRestore;RTIambassador.federateRestoreComplete;RTIambassador.federateRestoreNotComplete;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.initiateFederateSave;FederateAmbassador.federationNotSaved;FederateAmbassador.federationSaved;FederateAmbassador.requestFederationRestoreSucceeded;FederateAmbassador.federationRestoreBegun;FederateAmbassador.initiateFederateRestore;FederateAmbassador.federationNotRestored",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-federation-save-lifecycle",
               "cpp-1516.1-federation-restore-lifecycle",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsSaveRestoreFailures);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-restore-request-failure",
            "gate a missing-save restore-request failure with callback enable and disable",
            "callback-controls",
            "RTIambassador.requestFederationRestore;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.requestFederationRestoreFailed;FederateAmbassador.requestFederationRestoreSucceeded;FederateAmbassador.federationRestoreBegun;FederateAmbassador.initiateFederateRestore",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-federation-restore-lifecycle",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsRestoreRequestFailure);
   }

   static void registerDataAndAdvisories() {
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-timestamped-attribute-update",
            "gate timestamped attribute reflection and its matching grant with callback enable and disable",
            "callback-controls",
            "RTIambassador.publishObjectClassAttributes;RTIambassador.changeDefaultAttributeOrderType;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.updateAttributeValues(time);RTIambassador.timeAdvanceRequest;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.reflectAttributeValues(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-timestamped-attribute-update",
               "cpp-1516.1-time-management",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsTimestampedAttributeUpdate);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-attribute-update",
            "gate ordinary attribute reflection with callback enable and disable",
            "callback-controls",
            "RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-attribute-update-reflect",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsAttributeUpdate);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-timestamped-object-removal",
            "gate timestamped object removal and its matching grant with callback enable and disable",
            "callback-controls",
            "RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getObjectInstanceName;RTIambassador.getObjectInstanceHandle;RTIambassador.enableTimeRegulation;RTIambassador.enableTimeConstrained;RTIambassador.deleteObjectInstance(time);RTIambassador.timeAdvanceRequest;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.disableTimeConstrained;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.timeRegulationEnabled;FederateAmbassador.timeConstrainedEnabled;FederateAmbassador.removeObjectInstance(time);FederateAmbassador.timeAdvanceGrant",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-timestamped-object-deletion",
               "cpp-1516.1-time-management",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsTimestampedObjectRemoval);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-timestamped-retraction",
            "gate the Request Retraction callback after timestamped interaction delivery",
            "callback-controls",
            "RTIambassador.publishInteractionClass;RTIambassador.changeInteractionOrderType;RTIambassador.subscribeInteractionClass;RTIambassador.enableTimeRegulation;RTIambassador.sendInteraction(time);RTIambassador.retract;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.disableTimeRegulation;RTIambassador.unsubscribeInteractionClass;RTIambassador.unpublishInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction(time);FederateAmbassador.requestRetraction",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-timestamped-interaction-delivery",
               "cpp-1516.1-interaction-retraction",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsTimestampedRetraction);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-transportation",
            "gate standard transportation reports and confirmations with callback enable and disable",
            "callback-controls",
            "RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishInteractionClass;RTIambassador.subscribeInteractionClass;RTIambassador.registerObjectInstance;RTIambassador.getTransportationTypeHandle;RTIambassador.getTransportationTypeName;RTIambassador.queryAttributeTransportationType;RTIambassador.requestAttributeTransportationTypeChange;RTIambassador.queryInteractionTransportationType;RTIambassador.requestInteractionTransportationTypeChange;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.unsubscribeInteractionClass;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishInteractionClass;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reportAttributeTransportationType;FederateAmbassador.confirmAttributeTransportationTypeChange;FederateAmbassador.reportInteractionTransportationType;FederateAmbassador.confirmInteractionTransportationTypeChange",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-transportation-type",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsTransportation);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-directed-interaction",
            "gate standard directed-interaction delivery with callback enable and disable",
            "callback-controls",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassDirectedInteractions;RTIambassador.subscribeObjectClassDirectedInteractions(universally=false);RTIambassador.registerObjectInstance;RTIambassador.sendDirectedInteraction;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.unsubscribeObjectClassDirectedInteractions;RTIambassador.unpublishObjectClassDirectedInteractions;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unpublishObjectClassAttributes;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.receiveDirectedInteraction",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-directed-interaction-publication",
               "cpp-1516.1-directed-interaction-subscription",
               "cpp-1516.1-directed-interaction-delivery",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, false), RtiTckMain::callbackControlsDirectedInteraction);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-federation-reports",
            "gate standard federation execution and member reports with callback enable and disable",
            "callback-controls",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.listFederationExecutions;RTIambassador.listFederationExecutionMembers;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions;FederateAmbassador.reportFederationExecutionMembers;FederateAmbassador.reportFederationExecutionDoesNotExist",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-federation-create-join-resign-destroy",
               "cpp-1516.1-federation-query-reports"
            }, false), RtiTckMain::callbackControlsFederationReports);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-declaration-advisories",
            "gate standard declaration-relevance advisory callbacks with callback enable and disable services",
            "callback-controls",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassAttributes;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.subscribeInteractionClass;RTIambassador.publishInteractionClass;RTIambassador.unsubscribeInteractionClass;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.unpublishObjectClassAttributes;RTIambassador.unpublishInteractionClass;RTIambassador.unsubscribeObjectClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.startRegistrationForObjectClass;FederateAmbassador.stopRegistrationForObjectClass;FederateAmbassador.turnInteractionsOn;FederateAmbassador.turnInteractionsOff",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-declaration-relevance-advisories",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, true), RtiTckMain::callbackControlsDeclarationAdvisories);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-attribute-relevance-advisories",
            "gate standard per-object attribute-relevance advisory callbacks with callback enable and disable services",
            "callback-controls",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.subscribeObjectClassAttributes;RTIambassador.publishObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.unsubscribeObjectClassAttributes;RTIambassador.unsubscribeObjectClass;RTIambassador.unpublishObjectClassAttributes;RTIambassador.unpublishObjectClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.startRegistrationForObjectClass;FederateAmbassador.discoverObjectInstance;FederateAmbassador.turnUpdatesOnForObjectInstance;FederateAmbassador.turnUpdatesOnForObjectInstance(updateRateDesignator);FederateAmbassador.turnUpdatesOffForObjectInstance",
            new String[] {
               "cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release",
               "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-attribute-relevance-advisories",
               "cpp-1516.1-update-rate-value",
               "cpp-1516.1-object-registration-discovery",
               "cpp-1516.1-federation-create-join-resign-destroy"
            }, true), RtiTckMain::callbackControlsAttributeRelevanceAdvisories);
      RtiTckMain.run(new RtiTckMain.Scenario("java-tck.callback-controls-attribute-scope-advisories",
            "gate regional attributes-in/out-of-scope advisories with callback enable and disable",
            "callback-controls",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;RTIambassador.getDimensionUpperBound;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.commitRegionModifications;RTIambassador.getAttributeHandleSetFactory;RTIambassador.getDimensionHandleSetFactory;RTIambassador.getRegionHandleSetFactory;RTIambassador.getAttributeSetRegionSetPairListFactory;RTIambassador.publishObjectClassAttributes;RTIambassador.setAttributeScopeAdvisorySwitch;RTIambassador.getAttributeScopeAdvisorySwitch;RTIambassador.subscribeObjectClassAttributesWithRegions;RTIambassador.registerObjectInstanceWithRegions;RTIambassador.associateRegionsForUpdates;RTIambassador.unassociateRegionsForUpdates;RTIambassador.disableCallbacks;RTIambassador.enableCallbacks;RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;RTIambassador.unsubscribeObjectClassAttributesWithRegions;RTIambassador.unpublishObjectClassAttributes;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.attributesInScope;FederateAmbassador.attributesOutOfScope",
            new String[] {"cpp-1516.1-callback-disable-gating",
               "cpp-1516.1-callback-enable-release", "cpp-1516.1-callback-model-servicing",
               "cpp-1516.1-ddm-attribute-scope-advisory",
               "cpp-1516.1-ddm-attribute-scope-stale-callback",
               "cpp-1516.1-federation-create-join-resign-destroy"}, true),
           RtiTckMain::callbackControlsAttributeScopeAdvisories);
   }
 }
