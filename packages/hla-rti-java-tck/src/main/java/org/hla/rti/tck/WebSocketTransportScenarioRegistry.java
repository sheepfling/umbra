package org.hla.rti.tck;

/** Registers the adapter-backed Java WebSocket transport witness. */
final class WebSocketTransportScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String REQUIREMENT_WEBSOCKET =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-411-l20-6-websocket";
   private static final String REQUIREMENT_DATA_FRAMES =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l81-26";

   private WebSocketTransportScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.websocket-transport-support",
            "Use the public Java Connect and List services over WebSocket",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConfigurationResult.addressUsed;RTIambassador.listFederationExecutions;"
               + "RTIambassador.evokeMultipleCallbacks;RTIambassador.getObjectClassHandle;"
               + "RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {REQUIREMENT_WEBSOCKET, REQUIREMENT_DATA_FRAMES}, false),
         WebSocketTransportScenarioRegistry::webSocketTransportSupport);
   }

   static void webSocketTransportSupport() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied WebSocket endpoint");
      WebSocketTransportSupportTck.run(RtiTckMain.factory(), RtiTckMain.callbackModel(),
         rtiAddress,
         Long.getLong("hla.rti.tck.webSocketTransportTimeoutMillis", 10000L));
   }
}
