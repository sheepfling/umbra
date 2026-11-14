package org.hla.rti.tck;

/** Registers the adapter-backed Java TLS 1.3 alternate-port witness. */
final class Tls13AlternateAddressPortScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-411-l50-16";

   private Tls13AlternateAddressPortScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.tls13-alternate-address-port-configuration",
            "Connect using an explicitly configured non-default RTI address and port",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConfigurationResult.addressUsed;RTIambassador.listFederationExecutions;"
               + "RTIambassador.evokeMultipleCallbacks;RTIambassador.disconnect;"
               + "FederateAmbassador.reportFederationExecutions",
            new String[] {REQUIREMENT_ID}, false),
         Tls13AlternateAddressPortScenarioRegistry::tls13AlternateAddressPortConfiguration);
   }

   static void tls13AlternateAddressPortConfiguration() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TLS address with a port");
      Tls13TransportSupportTck.runAlternateAddressPort(RtiTckMain.factory(),
         RtiTckMain.callbackModel(), rtiAddress,
         Long.getLong("hla.rti.tck.tlsTransportTimeoutMillis", 10000L));
   }
}
