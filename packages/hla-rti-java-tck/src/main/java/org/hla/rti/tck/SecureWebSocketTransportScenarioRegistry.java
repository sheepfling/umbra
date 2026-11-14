package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;

/** Registers the adapter-backed Java secure WebSocket transport witness. */
final class SecureWebSocketTransportScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String ADDITIONAL_SETTINGS_PROPERTY =
      "hla.rti.tck.additionalSettings";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-411-l20-6-secure-websocket";

   private SecureWebSocketTransportScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.secure-websocket-transport-support",
            "Connect and complete a federation query over Secure WebSocket TLS 1.3",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RtiConfiguration.withAdditionalSettings;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConfigurationResult.addressUsed;"
               + "ConfigurationResult.additionalSettingsResultCode;"
               + "AdditionalSettingsResultCode.SETTINGS_APPLIED;"
               + "RTIambassador.listFederationExecutions;RTIambassador.evokeMultipleCallbacks;"
               + "RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            new String[] {REQUIREMENT_ID}, false),
         SecureWebSocketTransportScenarioRegistry::secureWebSocketTransportSupport);
   }

   static void secureWebSocketTransportSupport() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      String additionalSettings = System.getProperty(ADDITIONAL_SETTINGS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied secure WebSocket endpoint");
      JavaTckSupport.check(!additionalSettings.isEmpty(),
         "set -D" + ADDITIONAL_SETTINGS_PROPERTY + " to adapter-supplied TLS trust settings");
      WebSocketTransportSupportTck.runSecure(RtiTckMain.factory(),
         RtiTckMain.callbackModel(), rtiAddress, additionalSettings,
         Long.getLong("hla.rti.tck.webSocketTransportTimeoutMillis", 10000L));
   }
}
