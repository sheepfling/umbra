package org.hla.rti.tck;

/** Registers the focused mixed present/absent regional unsubscribe witness. */
final class RegionalInteractionMixedAbsentUnsubscribeScenarioRegistry {
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-09-data-distribution-management-page-238-l187-56";

   private RegionalInteractionMixedAbsentUnsubscribeScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.regional-interaction-mixed-absent-unsubscribe",
            "Preserve the full interaction subscription set when unsubscription mixes an active and absent region",
            "regional-interaction-management",
            "RTIambassador.connect;RTIambassador.createFederationExecution;"
               + "RTIambassador.joinFederationExecution;FederateHandle.encodedLength;"
               + "RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;"
               + "RTIambassador.getInteractionClassHandle;"
               + "RTIambassador.getParameterHandle;RTIambassador.getDimensionHandle;"
               + "ObjectClassHandle.encodedLength;AttributeHandle.encodedLength;"
               + "InteractionClassHandle.encodedLength;ParameterHandle.encodedLength;"
               + "DimensionHandle.encodedLength;"
               + "RTIambassador.getAvailableDimensionsForObjectClass;"
               + "RTIambassador.getAvailableDimensionsForInteractionClass;"
               + "RTIambassador.getDimensionName;RTIambassador.getDimensionUpperBound;"
               + "RTIambassador.publishInteractionClass;"
               + "RTIambassador.createRegion;RegionHandle.encodedLength;"
               + "RTIambassador.setRangeBounds;"
               + "RTIambassador.commitRegionModifications;"
               + "RTIambassador.getRegionHandleSetFactory;"
               + "RTIambassador.subscribeInteractionClassWithRegions;"
               + "RTIambassador.getParameterHandleValueMapFactory;"
               + "RTIambassador.sendInteractionWithRegions;"
               + "RTIambassador.evokeCallback;RTIambassador.unsubscribeInteractionClassWithRegions;"
               + "RTIambassador.deleteRegion;RTIambassador.unpublishInteractionClass;"
               + "RTIambassador.resignFederationExecution;"
               + "RTIambassador.destroyFederationExecution;RTIambassador.disconnect;"
               + "FederateAmbassador.receiveInteraction",
            new String[] {REQUIREMENT_ID}, false),
         RegionalInteractionMixedAbsentUnsubscribeScenarioRegistry::run);
   }

   static void run() throws Exception {
      RegionalInteractionMixedAbsentUnsubscribeTck.run(RtiTckMain.factory(),
         RtiTckMain.requireAdapterFile("hla.rti.tck.ddmFom"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmObjectClass"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmAttribute"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.interactionClass"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.parameter"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.dimension"),
         RtiTckMain.requireAdapterValue("hla.rti.tck.ddmSecondDimension"),
         RtiTckMain.callbackModel());
   }
}
