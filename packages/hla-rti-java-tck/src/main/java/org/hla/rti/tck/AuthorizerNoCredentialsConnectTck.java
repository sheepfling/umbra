package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.HLAnoCredentials;

/** Checks the standard HLAnoCredentials handoff through Java Connect. */
final class AuthorizerNoCredentialsConnectTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-no-credentials";

   private AuthorizerNoCredentialsConnectTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel,
            configuration, new HLAnoCredentials());
         connected = true;
      } finally {
         if (connected) ambassador.disconnect();
      }
   }
}
