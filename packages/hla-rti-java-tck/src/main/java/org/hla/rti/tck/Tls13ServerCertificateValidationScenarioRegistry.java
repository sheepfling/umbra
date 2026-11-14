package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.RtiFactory;

/** Registers the adapter-backed positive Java TLS server-certificate witness. */
final class Tls13ServerCertificateValidationScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String ADDITIONAL_SETTINGS_PROPERTY =
      "hla.rti.tck.additionalSettings";
   private static final String ABSENT_FEDERATION =
      "__java_tck_tls_server_validation_absent_federation__";
   private static final String[] REQUIREMENT_IDS = {
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l51-16-server-validation",
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l51-16-server-key",
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l66-21"
   };

   private Tls13ServerCertificateValidationScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.tls13-server-certificate-validation",
            "Connect over TLS 1.3 with a trusted server certificate",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RtiConfiguration.withAdditionalSettings;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConfigurationResult.addressUsed;"
               + "ConfigurationResult.additionalSettingsResultCode;"
               + "AdditionalSettingsResultCode.SETTINGS_APPLIED;"
               + "RTIambassador.listFederationExecutions;RTIambassador.evokeMultipleCallbacks;"
               + "RTIambassador.disconnect;FederateAmbassador.reportFederationExecutions",
            REQUIREMENT_IDS, false),
         Tls13ServerCertificateValidationScenarioRegistry::
            tls13ServerCertificateValidation);
   }

   static void tls13ServerCertificateValidation() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      String additionalSettings = System.getProperty(ADDITIONAL_SETTINGS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TLS endpoint");
      JavaTckSupport.check(!additionalSettings.isEmpty(),
         "set -D" + ADDITIONAL_SETTINGS_PROPERTY + " to adapter-supplied server-validation settings");
      Tls13TransportSupportTck.run(RtiTckMain.factory(), RtiTckMain.callbackModel(),
         rtiAddress, additionalSettings,
         Long.getLong("hla.rti.tck.tlsTransportTimeoutMillis", 10000L), ABSENT_FEDERATION);
   }
}
