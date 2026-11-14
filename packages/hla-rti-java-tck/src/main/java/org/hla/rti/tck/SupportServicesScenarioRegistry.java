package org.hla.rti.tck;

/** Owns focused Java counterparts for IEEE 1516.1 support services. */
final class SupportServicesScenarioRegistry {
   private SupportServicesScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.attribute-name-lookup",
            "return the configured attribute name for a valid object-class and attribute handle pair",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-250-l109-27"
            }, true), SupportServicesScenarioRegistry::attributeNameLookup);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.privilege-attribute-availability",
            "resolve HLAprivilegeToDeleteObject on every object class in the P0 TCK FOM",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;ObjectClassHandle.encodedLength;RTIambassador.getAttributeHandle;AttributeHandle.encodedLength;RTIambassador.getAttributeName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-07-ownership-management-page-153-l84-26",
               "requirement-candidate-sections-semantic-clauses-5-6-page-092-l94-20",
               "requirement-candidate-sections-semantic-clause-4b-page-051-l146-1"
            }, true), SupportServicesScenarioRegistry::privilegeAttributeAvailability);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.interaction-class-handle-lookup",
            "round-trip a configured interaction-class name through a valid handle",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;InteractionClassHandle.encodedLength;RTIambassador.getInteractionClassName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-252-l140-35",
               "requirement-candidate-content-clauses-10-support-services-page-253-l89-21"
            }, true), SupportServicesScenarioRegistry::interactionClassHandleNameLookup);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.parameter-handle-name-lookup",
            "round-trip a configured interaction parameter name through a valid handle",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;InteractionClassHandle.encodedLength;RTIambassador.getParameterHandle;ParameterHandle.encodedLength;RTIambassador.getParameterName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-254-l26-3",
               "requirement-candidate-content-clauses-10-support-services-page-254-l143-35"
            }, true), SupportServicesScenarioRegistry::parameterHandleNameLookup);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.fom-name-case-sensitivity",
            "preserve case-sensitive FOM names across public handle lookups",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getObjectClassName;RTIambassador.getAttributeHandle;RTIambassador.getAttributeName;RTIambassador.getInteractionClassHandle;RTIambassador.getInteractionClassName;RTIambassador.getParameterHandle;RTIambassador.getParameterName;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-243-l50-13",
               "requirement-candidate-content-clauses-10-support-services-page-243-l47-12",
               "requirement-candidate-content-clauses-10-support-services-page-243-l59-16",
               "requirement-candidate-content-clauses-11-management-object-model-page-353-l66-21"
            }, true), SupportServicesScenarioRegistry::fomNameCaseSensitivity);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.unconveyed-peer-region-dimension-handle-set-invalid-region",
            "reject dimension lookup for a peer-owned region whose designator was not conveyed",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet(InvalidRegion);RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-262-l29-7"
            }, true), SupportServicesScenarioRegistry::unconveyedPeerRegionDimensionHandleSetInvalidRegion);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.range-bounds-for-region",
            "return the current lower and upper bounds for a region dimension",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.getRangeBounds;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-262-l113-35",
               "requirement-candidate-content-clauses-10-support-services-page-263-l125-39"
            }, true), SupportServicesScenarioRegistry::rangeBoundsForRegion);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.unconveyed-peer-region-range-bounds-invalid-region",
            "reject range lookup for a peer-owned region whose designator was not conveyed",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.getRangeBounds(InvalidRegion);RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-262-l203-65"
            }, true), SupportServicesScenarioRegistry::unconveyedPeerRegionRangeBoundsInvalidRegion);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.invalid-region-designator-range-bounds-invalid-region",
            "reject range lookup for a deleted region designator",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.deleteRegion;RTIambassador.getRangeBounds(InvalidRegion);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-263-l20-4"
            }, true), SupportServicesScenarioRegistry::invalidRegionDesignatorRangeBoundsInvalidRegion);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.available-object-class-dimensions",
            "return the exact available object-class dimensions and an empty root set",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForObjectClass;DimensionHandle.equals;DimensionHandleSet.size;DimensionHandleSet.contains;DimensionHandleSet.isEmpty;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-258-l64-18",
               "requirement-candidate-content-clauses-10-support-services-page-258-l67-19"
            }, true), SupportServicesScenarioRegistry::availableObjectClassDimensions);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.available-interaction-class-dimensions",
            "return the exact available interaction-class dimensions and an empty root set",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForInteractionClass;DimensionHandle.equals;DimensionHandleSet.size;DimensionHandleSet.contains;DimensionHandleSet.isEmpty;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-259-l30-6",
               "requirement-candidate-content-clauses-10-support-services-page-259-l36-8"
            }, true), SupportServicesScenarioRegistry::availableInteractionClassDimensions);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.handle-normalization-equal-instances",
            "equal standard handles produce equal normalized values",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.getFederateHandleFactory;FederateHandleFactory.decode;RTIambassador.getObjectClassHandleFactory;ObjectClassHandleFactory.decode;RTIambassador.getInteractionClassHandleFactory;InteractionClassHandleFactory.decode;RTIambassador.getObjectInstanceHandleFactory;ObjectInstanceHandleFactory.decode;Handle.encode;Handle.equals;RTIambassador.normalizeFederateHandle;RTIambassador.normalizeObjectClassHandle;RTIambassador.normalizeInteractionClassHandle;RTIambassador.normalizeObjectInstanceHandle;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-244-l161-52"
            }, true), SupportServicesScenarioRegistry::handleNormalizationEqualInstances);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.normalize-object-class-handle-region-coordinate",
            "use the normalized object-class handle as an HLAobjectClass point range",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.normalizeObjectClassHandle;RTIambassador.getDimensionHandle;RTIambassador.getDimensionUpperBound;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.getRangeBounds;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-265-l150-46"
            }, true), SupportServicesScenarioRegistry::normalizeObjectClassHandleRegionCoordinate);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.normalize-interaction-class-handle-region-coordinate",
            "use the normalized interaction-class handle as an HLAinteractionClass point range",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.normalizeInteractionClassHandle;RTIambassador.getDimensionHandle;RTIambassador.getDimensionUpperBound;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.getRangeBounds;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-266-l95-28"
            }, true), SupportServicesScenarioRegistry::normalizeInteractionClassHandleRegionCoordinate);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.normalize-object-instance-handle-region-coordinate",
            "use the normalized object-instance handle as an HLAobjectInstance point range",
            "handle-lookup",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecutionWithMIM;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.getAttributeHandleSetFactory;AttributeHandleSetFactory.create;AttributeHandleSet.add;RTIambassador.publishObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.normalizeObjectInstanceHandle;RTIambassador.getDimensionHandle;RTIambassador.getDimensionUpperBound;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;DimensionHandleSet.add;RTIambassador.createRegion;RTIambassador.setRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RegionHandleSet.add;RTIambassador.commitRegionModifications;RTIambassador.getRangeBounds;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-267-l39-9"
            }, true), SupportServicesScenarioRegistry::normalizeObjectInstanceHandleRegionCoordinate);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.disable-callbacks-gates-evoke-services",
            "gate evoke services and serialize concurrent Disable Callbacks until in-flight callback completion",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(HLA_EVOKED);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.disableCallbacks;RTIambassador.listFederationExecutions;RTIambassador.evokeCallback;RTIambassador.evokeMultipleCallbacks;RTIambassador.enableCallbacks;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-286-l20-3",
               "requirement-candidate-content-clauses-10-support-services-page-286-l125-38",
               "requirement-candidate-content-clauses-10-support-services-page-286-l167-52",
               "requirement-candidate-content-clauses-11-management-object-model-page-357-l199-65",
               "requirement-candidate-content-clauses-11-management-object-model-page-358-l10-2"
            }, true), SupportServicesScenarioRegistry::disableCallbacksGatesEvokeServices);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.evoke-multiple-callbacks-pending-indicator",
            "report pending state in HLA_EVOKED and preserve no-effect behavior in HLA_IMMEDIATE",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callbackModel);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.evokeMultipleCallbacks;RTIambassador.disableCallbacks;RTIambassador.listFederationExecutions;RTIambassador.enableCallbacks;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-285-l14-1",
               "requirement-candidate-content-clauses-10-support-services-page-285-l14-1-false",
               "requirement-candidate-content-clauses-10-support-services-page-285-l14-1-true",
               "requirement-candidate-content-clauses-10-support-services-page-285-l17-2"
            }, true), SupportServicesScenarioRegistry::evokeMultipleCallbacksPendingIndicator);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.evoke-callback-immediate-no-effect",
            "leave a queued callback untouched and ignore the wait argument in HLA_IMMEDIATE",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(HLA_IMMEDIATE);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.disableCallbacks;RTIambassador.listFederationExecutions;RTIambassador.evokeCallback;RTIambassador.enableCallbacks;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-284-l80-23",
               "requirement-candidate-content-clauses-10-support-services-page-286-l152-47"
            }, true), SupportServicesScenarioRegistry::evokeCallbackImmediateNoEffect);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.evoke-callback-wait-duration-precision",
            "preserve the 1 ms difference between adjacent evokeCallback wait arguments",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(HLA_EVOKED);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-284-l71-20-precision"
            }, true), SupportServicesScenarioRegistry::evokeCallbackWaitDurationPrecision);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.evoke-callback-empty-queue-timeout",
            "wait the requested minimum duration when evokeCallback has no pending callback",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(HLA_EVOKED);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-284-l71-20"
            }, true), SupportServicesScenarioRegistry::evokeCallbackEmptyQueueTimeout);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.evoke-callback-single-dispatch-pending-indicator",
            "invoke one federation-list callback and report when another remains pending",
            "callback-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(HLA_EVOKED);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.listFederationExecutions;RTIambassador.evokeCallback;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-284-l74-21",
               "requirement-candidate-content-clauses-10-support-services-page-284-l77-22-true"
            }, true), SupportServicesScenarioRegistry::evokeCallbackSingleDispatchPendingIndicator);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.dimension-handle-name-lookup",
            "round-trip a defined dimension name and reject an undefined dimension name",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;DimensionHandle.encodedLength;RTIambassador.getDimensionName;RTIambassador.getDimensionHandle(NameNotFound);RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-259-l144-44",
               "requirement-service-flow-get-dimension-handle-name-not-found",
               "requirement-candidate-content-clauses-10-support-services-page-260-l80-23"
            }, true), SupportServicesScenarioRegistry::dimensionHandleNameLookup);
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.dimension-handle-set-for-region",
            "return the exact one- and two-dimension sets configured for regions",
            "support-services",
            "RtiFactory.getRtiAmbassador;RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getDimensionHandle;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;RTIambassador.createRegion;RTIambassador.getDimensionHandleSet;DimensionHandleSet.size;DimensionHandleSet.contains;RTIambassador.deleteRegion;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect",
            new String[] {
               "requirement-candidate-content-clauses-10-support-services-page-261-l141-43"
            }, true), SupportServicesScenarioRegistry::dimensionHandleSetForRegion);
   }

   static void availableObjectClassDimensions() throws Exception {
      AvailableObjectClassDimensionsTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmObjectClass"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmSecondDimension"),
         RtiTckMain.callbackModel());
   }

   static void attributeNameLookup() throws Exception {
      AttributeNameLookupTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.objectClassName(),
         RtiTckMain.attributeName(), RtiTckMain.callbackModel());
   }

   static void privilegeAttributeAvailability() throws Exception {
      PrivilegeAttributeAvailabilityTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void interactionClassHandleNameLookup() throws Exception {
      InteractionClassHandleNameLookupTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.interactionClassName(),
         RtiTckMain.callbackModel());
   }

   static void parameterHandleNameLookup() throws Exception {
      ParameterHandleNameLookupTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.interactionClassName(),
         RtiTckMain.parameterName(), RtiTckMain.callbackModel());
   }

   static void fomNameCaseSensitivity() throws Exception {
      FomNameCaseSensitivityTck.run(RtiTckMain.factory(), RtiTckMain.requireMim(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void unconveyedPeerRegionDimensionHandleSetInvalidRegion() throws Exception {
      UnconveyedPeerRegionDimensionHandleSetInvalidRegionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"), RtiTckMain.callbackModel());
   }

   static void rangeBoundsForRegion() throws Exception {
      RangeBoundsForRegionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"), RtiTckMain.callbackModel());
   }

   static void unconveyedPeerRegionRangeBoundsInvalidRegion() throws Exception {
      UnconveyedPeerRegionRangeBoundsInvalidRegionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"), RtiTckMain.callbackModel());
   }

   static void invalidRegionDesignatorRangeBoundsInvalidRegion() throws Exception {
      DeletedRegionRangeBoundsInvalidRegionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"), RtiTckMain.callbackModel());
   }

   static void availableInteractionClassDimensions() throws Exception {
      AvailableInteractionClassDimensionsTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.interactionClass"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmSecondDimension"),
         RtiTckMain.callbackModel());
   }

   static void handleNormalizationEqualInstances() throws Exception {
      HandleNormalizationEqualInstancesTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.objectClassName(),
         RtiTckMain.attributeName(), RtiTckMain.interactionClassName(),
         RtiTckMain.parameterName(), RtiTckMain.callbackModel());
   }

   static void normalizeObjectClassHandleRegionCoordinate() throws Exception {
      NormalizeObjectClassHandleRegionCoordinateTck.run(RtiTckMain.factory(),
         RtiTckMain.requireFom(), RtiTckMain.requireMim(), RtiTckMain.timeImplementation(),
         RtiTckMain.callbackModel());
   }

   static void normalizeInteractionClassHandleRegionCoordinate() throws Exception {
      NormalizeInteractionClassHandleRegionCoordinateTck.run(RtiTckMain.factory(),
         RtiTckMain.requireFom(), RtiTckMain.requireMim(), RtiTckMain.timeImplementation(),
         RtiTckMain.callbackModel());
   }

   static void normalizeObjectInstanceHandleRegionCoordinate() throws Exception {
      NormalizeObjectInstanceHandleRegionCoordinateTck.run(RtiTckMain.factory(),
         RtiTckMain.requireFom(), RtiTckMain.requireMim(), RtiTckMain.timeImplementation(),
         RtiTckMain.objectClassName(), RtiTckMain.attributeName(), RtiTckMain.callbackModel());
   }

   static void evokeCallbackSingleDispatchPendingIndicator() throws Exception {
      EvokeCallbackSingleDispatchPendingIndicatorTck.run(RtiTckMain.factory(),
         RtiTckMain.requireFom(), RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void evokeCallbackEmptyQueueTimeout() throws Exception {
      EvokeCallbackEmptyQueueTimeoutTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void evokeCallbackWaitDurationPrecision() throws Exception {
      EvokeCallbackWaitDurationPrecisionTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void evokeCallbackImmediateNoEffect() throws Exception {
      EvokeCallbackImmediateNoEffectTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void evokeMultipleCallbacksPendingIndicator() throws Exception {
      EvokeMultipleCallbacksPendingIndicatorTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void disableCallbacksGatesEvokeServices() throws Exception {
      DisableCallbacksGatesEvokeServicesTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), RtiTckMain.callbackModel());
   }

   static void dimensionHandleNameLookup() throws Exception {
      DimensionHandleNameLookupTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"), RtiTckMain.callbackModel());
   }

   static void dimensionHandleSetForRegion() throws Exception {
      DimensionHandleSetForRegionTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"), RtiTckMain.timeImplementation(),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmSecondDimension"),
         RtiTckMain.callbackModel());
   }
}
