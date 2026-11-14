package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.RtiFactory;

/** Registers the adapter-backed Java rejection case for an untrusted TLS server. */
final class Tls13ServerCertificateValidationRejectsUntrustedScenarioRegistry {
   private static final String RTI_ADDRESS_PROPERTY = "hla.rti.tck.rtiAddress";
   private static final String ADDITIONAL_SETTINGS_PROPERTY =
      "hla.rti.tck.additionalSettings";
   private static final String REQUIREMENT_ID =
      "requirement-candidate-content-clauses-12-programming-language-mappings-page-412-l51-16-federate-validation";

   private Tls13ServerCertificateValidationRejectsUntrustedScenarioRegistry() { }

   static void register() {
      RtiTckMain.run(new RtiTckMain.Scenario(
            "java-tck.tls13-server-certificate-validation-rejects-untrusted",
            "Reject an untrusted TLS 1.3 server certificate",
            "connection",
            "RtiConfiguration.createConfiguration;RtiConfiguration.withRtiAddress;"
               + "RtiConfiguration.withAdditionalSettings;"
               + "RTIambassador.connect(FederateAmbassador, CallbackModel, RtiConfiguration);"
               + "ConnectionFailed",
            new String[] {REQUIREMENT_ID}, false),
         Tls13ServerCertificateValidationRejectsUntrustedScenarioRegistry::
            tls13ServerCertificateValidationRejectsUntrusted);
   }

   static void tls13ServerCertificateValidationRejectsUntrusted() throws Exception {
      String rtiAddress = System.getProperty(RTI_ADDRESS_PROPERTY, "");
      String additionalSettings = System.getProperty(ADDITIONAL_SETTINGS_PROPERTY, "");
      JavaTckSupport.check(!rtiAddress.isEmpty(),
         "set -D" + RTI_ADDRESS_PROPERTY + " to the adapter-supplied TLS endpoint");
      JavaTckSupport.check(!additionalSettings.isEmpty(),
         "set -D" + ADDITIONAL_SETTINGS_PROPERTY + " to adapter-supplied validation settings");
      Tls13TransportSupportTck.expectUntrustedServerFailure(RtiTckMain.factory(),
         RtiTckMain.callbackModel(), rtiAddress, additionalSettings);
   }
}
