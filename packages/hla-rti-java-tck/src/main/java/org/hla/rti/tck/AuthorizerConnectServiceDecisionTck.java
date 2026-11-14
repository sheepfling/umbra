package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.auth.HLAnoCredentials;
import hla.rti1516_2025.auth.HLAplainTextPassword;
import hla.rti1516_2025.exceptions.Unauthorized;

/** Checks configured HLAauthorizer decisions through the standard Java Connect service. */
final class AuthorizerConnectServiceDecisionTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer";

   private AuthorizerConnectServiceDecisionTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);

      boolean noCredentialsDenied = connectDenied(factory, configuration,
         new HLAnoCredentials(), callbackModel);
      JavaTckSupport.check(noCredentialsDenied,
         "configured HLAauthorizer did not deny HLAnoCredentials under its password policy");

      boolean wrongPasswordDenied = connectDenied(factory, configuration,
         new HLAplainTextPassword("wrong-tck-authorizer-password"), callbackModel);
      JavaTckSupport.check(wrongPasswordDenied,
         "Connect accepted a password denied by the configured HLAauthorizer");

      RTIambassador accepted = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         accepted.connect(new NullFederateAmbassador(), callbackModel, configuration,
            new HLAplainTextPassword("tck-authorizer-password"));
         connected = true;
      } finally {
         if (connected) accepted.disconnect();
      }
   }

   private static boolean connectDenied(RtiFactory factory, RtiConfiguration configuration,
         Credentials credentials, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel,
            configuration, credentials);
         connected = true;
         return false;
      } catch (Unauthorized expected) {
         return true;
      } finally {
         if (connected) ambassador.disconnect();
      }
   }
}
