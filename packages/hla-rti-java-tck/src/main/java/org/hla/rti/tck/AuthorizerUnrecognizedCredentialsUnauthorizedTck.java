package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.Credentials;
import hla.rti1516_2025.exceptions.Unauthorized;

/** Checks that Connect reports unknown credential types as Unauthorized. */
final class AuthorizerUnrecognizedCredentialsUnauthorizedTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer";
   private static final String UNKNOWN_CREDENTIAL_TYPE = "TckUnknownCredentialType";

   private AuthorizerUnrecognizedCredentialsUnauthorizedTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean unauthorized = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel, configuration,
            new Credentials(UNKNOWN_CREDENTIAL_TYPE, new byte[0]));
         connected = true;
      } catch (Unauthorized expected) {
         unauthorized = true;
      } finally {
         if (connected) ambassador.disconnect();
      }

      JavaTckSupport.check(unauthorized,
         "HLAauthorizer did not treat an unrecognized credential type as unauthorized");
   }
}
