package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.RtiFactory;

/** Registers the adapter-backed Java TLS 1.3 encryption-only witness. */
final class Tls13EncryptionOnlyScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String ADDITIONAL_SETTINGS_PROPERTY =
      "hla.rti.tck.additionalSettings";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l51-16";
   private static final String ABSENT_FEDERATION =
      "__java_tck_tls_encryption_only_absent_federation__";

   private Tls13EncryptionOnlyScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.tls13-encryption-only-mode",
            "Connect over TLS 1.3 in encryption-only mode",
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
         Tls13EncryptionOnlyScenarioRegistry::tls13EncryptionOnlyMode);
   }

   static void tls13EncryptionOnlyMode() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      String additionalSettings = System.getProperty(ADDITIONAL_SETTINGS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TLS endpoint");
      JavaTckSupport.check(!additionalSettings.isEmpty(),
         "set -D" + ADDITIONAL_SETTINGS_PROPERTY + " to adapter-supplied TLS mode settings");
      Tls13TransportSupportTck.run(RtiTckMain.factory(), RtiTckMain.callbackModel(),
         rtiAddress, additionalSettings,
         Long.getLong("hla.rti.tck.tlsTransportTimeoutMillis", 10000L), ABSENT_FEDERATION);
   }
}
