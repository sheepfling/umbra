package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.auth.HLAnoCredentials;
import hla.rti1516_2025.exceptions.RTIexception;

/** Checks Connect rejection for a RID profile with multiple authorization services. */
final class AuthorizerSingleServicePerRidProfileTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-rid-multiple-services";

   private AuthorizerSingleServicePerRidProfileTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean connectRejected = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel,
            configuration, new HLAnoCredentials());
         connected = true;
      } catch (RTIexception expected) {
         connectRejected = true;
      } finally {
         if (connected) ambassador.disconnect();
      }

      JavaTckSupport.check(connectRejected,
         "Connect accepted a RID profile with two authorization services");
   }
}
