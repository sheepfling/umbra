package org.hla.rti.tck;

/** Registers the public API witness for application-selected attribute update thresholds. */
final class FederateUpdateThresholdsScenarioRegistry {
   private FederateUpdateThresholdsScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.federate-selected-update-thresholds",
            "Deliver updates selected by two federates with different thresholds",
            "object-management",
            "RTIambassador.connect;RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getObjectClassHandle;RTIambassador.getAttributeHandle;RTIambassador.publishObjectClassAttributes;RTIambassador.subscribeObjectClassAttributes;RTIambassador.registerObjectInstance;RTIambassador.updateAttributeValues;RTIambassador.resignFederationExecution;RTIambassador.destroyFederationExecution;RTIambassador.disconnect;FederateAmbassador.discoverObjectInstance;FederateAmbassador.reflectAttributeValues",
            new String[] {"summary-rule-9", "detailed-rule-9"}, false),
         FederateUpdateThresholdsScenarioRegistry::federateSelectedUpdateThresholds);
   }

   static void federateSelectedUpdateThresholds() throws Exception {
      FederateSelectedUpdateThresholdsTck.run(RtiTckMain.factory(), RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(), System.getProperty("hla.rti.tck.federateType", "tck-type"),
         RtiTckMain.objectClassName(), RtiTckMain.attributeName(), RtiTckMain.callbackModel());
   }
}
