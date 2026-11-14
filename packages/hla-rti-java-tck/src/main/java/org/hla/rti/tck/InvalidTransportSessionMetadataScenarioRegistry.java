package org.hla.rti.tck;

/** Registers the adapter-backed Java Connect metadata rejection scenario. */
final class InvalidTransportSessionMetadataScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-408-l86-28";

   private InvalidTransportSessionMetadataScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.reject-invalid-transport-session-metadata",
            "Reject invalid Table 33 session metadata during Connect",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConnectionFailed",
            new String[] {REQUIREMENT_ID}, false),
         InvalidTransportSessionMetadataScenarioRegistry::rejectsInvalidTransportSessionMetadata);
   }

   static void rejectsInvalidTransportSessionMetadata() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TCP endpoint");
      InvalidTransportSessionMetadataTck.run(
         RtiTckMain.factory(), RtiTckMain.callbackModel(), rtiAddress);
   }
}
