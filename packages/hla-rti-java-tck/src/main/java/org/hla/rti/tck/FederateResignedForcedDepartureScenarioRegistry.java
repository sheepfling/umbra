package org.hla.rti.tck;

import java.nio.file.Path;

/** Registers the Java public-API witness for one adapter-triggered forced resignation. */
final class FederateResignedForcedDepartureScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String COORDINATION_DIRECTORY_PROPERTY =
      "hla.rti.tck.federateResignationCoordinationDirectory";

   private FederateResignedForcedDepartureScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.federate-resigned-forced-departure",
            "Observe an RTI-forced Federate Resigned callback and rejoin through the Java API",
            "federation-management",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;RTIambassador.connect(FederateAmbassador,CallbackModel,RtiConfiguration);RTIambassador.createFederationExecution;RTIambassador.joinFederationExecution;RTIambassador.getFederateHandle;RTIambassador.resignFederationExecution;RTIambassador.evokeMultipleCallbacks;RTIambassador.disconnect;FederateAmbassador.federateResigned;FederateAmbassador.connectionLost",
            new String[] {
               "requirement-candidate-content-clauses-04-federation-management-page-040-l33-8",
               "req-federate-resigned-callback-behavior"
            }, false),
         FederateResignedForcedDepartureScenarioRegistry::federateResignedForcedDeparture);
   }

   static void federateResignedForcedDeparture() throws Exception {
      String address = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      String coordinationDirectory =
         System.getProperty(COORDINATION_DIRECTORY_PROPERTY, "");
      JavaTckSupport.check(!address.isEmpty(),
         "the forced-resignation adapter did not supply an RTI address");
      JavaTckSupport.check(!coordinationDirectory.isEmpty(),
         "the forced-resignation adapter did not supply a coordination directory");
      FederateResignedForcedDepartureTck.run(
         RtiTckMain.factory(),
         RtiTckMain.requireFom(),
         RtiTckMain.timeImplementation(),
         address,
         Path.of(coordinationDirectory),
         RtiTckMain.callbackModel());
   }
}
