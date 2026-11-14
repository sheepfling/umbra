package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.HLAnoCredentials;
import hla.rti1516_2025.exceptions.Unauthorized;

/** Checks that protected RID profiles select their configured authorization policy. */
final class AuthorizerRidConfigurationDecisionTck {
   private AuthorizerRidConfigurationDecisionTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(!connectWithProfile(factory, callbackModel,
            "tck-authorizer-rid-denied"),
         "the RID-selected denying service did not reject Connect");
      JavaTckSupport.check(connectWithProfile(factory, callbackModel,
            "tck-authorizer-rid-allowed"),
         "the RID-selected allowing service did not authorize Connect");
      JavaTckSupport.check(connectWithProfile(factory, callbackModel,
            "tck-authorizer-rid-disabled"),
         "the RID disabled profile did not allow no-credentials Connect");
   }

   private static boolean connectWithProfile(RtiFactory factory,
         CallbackModel callbackModel, String profile) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(profile);
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel,
            configuration, new HLAnoCredentials());
         connected = true;
         return true;
      } catch (Unauthorized denied) {
         return false;
      } finally {
         if (connected) ambassador.disconnect();
      }
   }
}
