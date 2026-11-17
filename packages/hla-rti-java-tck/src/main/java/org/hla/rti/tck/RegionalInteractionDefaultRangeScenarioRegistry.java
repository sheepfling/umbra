package org.hla.rti.tck;

/** Owns scenario registration for regional interaction FDD default-range checks. */
final class RegionalInteractionDefaultRangeScenarioRegistry {
   private RegionalInteractionDefaultRangeScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-interaction-partial-default-range",
            "apply the FDD default dimension when a non-empty interaction region omits it",
            "regional-interaction-management",
            "RTIambassador.connect(callback model supplied by adapter);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getInteractionClassHandle;RTIambassador.getParameterHandle;RTIambassador.getDimensionHandle;RTIambassador.getAvailableDimensionsForInteractionClass;RTIambassador.getDimensionUpperBound;RTIambassador.publishInteractionClass;RTIambassador.getDimensionHandleSetFactory;DimensionHandleSetFactory.create;RTIambassador.createRegion;RegionHandle.encodedLength();RTIambassador.getDimensionHandleSet;RTIambassador.setRangeBounds;RTIambassador.getRangeBounds;RTIambassador.getRegionHandleSetFactory;RegionHandleSetFactory.create;RTIambassador.commitRegionModifications;RTIambassador.subscribeInteractionClassWithRegions;RTIambassador.getParameterHandleValueMapFactory;ParameterHandleValueMapFactory.create;RTIambassador.sendInteractionWithRegions;RTIambassador.evokeCallback;RTIambassador.unsubscribeInteractionClassWithRegions;RTIambassador.deleteRegion;RTIambassador.unpublishInteractionClass;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.receiveInteraction",
            new String[] {
               "requirement-candidate-content-clauses-08-time-management-page-217-l122-39",
               "requirement-candidate-content-clauses-08-time-management-page-217-l143-46"
            }, false), RegionalInteractionPartialDefaultRangeTck::runFromAdapter);
   }
}
