package org.hla.rti.tck;

/** Registers the adapter-backed Java TLS 1.3 default-port witness. */
final class Tls13DefaultPortScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-411-l38-12";

   private Tls13DefaultPortScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.tls13-default-port-support",
            "Connect over TLS 1.3 using the normative default port",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConfigurationResult.addressUsed;RTIambassador.listFederationExecutions;"
               + "RTIambassador.evokeMultipleCallbacks;RTIambassador.disconnect;"
               + "FederateAmbassador.reportFederationExecutions",
            new String[] {REQUIREMENT_ID}, false),
         Tls13DefaultPortScenarioRegistry::tls13DefaultPortSupport);
   }

   static void tls13DefaultPortSupport() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TLS address without a port");
      Tls13TransportSupportTck.runDefaultPort(RtiTckMain.factory(),
         RtiTckMain.callbackModel(), rtiAddress,
         Long.getLong("hla.rti.tck.tlsTransportTimeoutMillis", 10000L));
   }
}
