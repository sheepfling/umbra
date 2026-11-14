package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.Unauthorized;
import java.util.UUID;

/** Checks federation survival and authorized retry after a denied Destroy. */
final class AuthorizerDestroyServiceDecisionTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-destroy";
   private AuthorizerDestroyServiceDecisionTck() { }

   static void run(RtiFactory factory, String fomPath, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      String federation = "java-tck-authorizer-destroy-" + UUID.randomUUID();
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean federationCreated = false;
      boolean joined = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel, configuration);
         connected = true;
         ambassador.createFederationExecution(federation, fomPath, timeImplementation);
         federationCreated = true;

         boolean destroyDenied = false;
         try {
            ambassador.destroyFederationExecution(federation);
            federationCreated = false;
         } catch (Unauthorized expected) {
            destroyDenied = true;
         }
         JavaTckSupport.check(destroyDenied,
            "Destroy Federation Execution did not apply the custom Authorizer denial");

         FederateHandle verifier = ambassador.joinFederationExecution(
            "tck-authorizer-destroy-verifier", "tck-type", federation);
         joined = true;
         JavaTckSupport.check(verifier != null,
            "Join returned no FederateHandle after the denied Destroy");
         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;

         ambassador.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (joined) {
            try {
               ambassador.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (federationCreated) {
            try {
               ambassador.destroyFederationExecution(federation);
            } catch (Exception ignored) {
            }
         }
         if (connected) ambassador.disconnect();
      }
   }
}
