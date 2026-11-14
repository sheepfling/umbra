package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.HLAplainTextPassword;
import hla.rti1516_2025.exceptions.Unauthorized;

/** Checks that supplied credentials are rejected while RID authorization is disabled. */
final class AuthorizerDisabledServiceRejectsCredentialsTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-disabled-credentials";
   private static final String PASSWORD = "tck-disabled-authorization-password";

   private AuthorizerDisabledServiceRejectsCredentialsTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean unauthorized = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel, configuration,
            new HLAplainTextPassword(PASSWORD));
         connected = true;
      } catch (Unauthorized expected) {
         unauthorized = true;
      } finally {
         if (connected) ambassador.disconnect();
      }

      JavaTckSupport.check(unauthorized,
         "Connect accepted supplied credentials while RID authorization was disabled");
   }
}
