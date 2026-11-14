package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.FederationExecutionDoesNotExist;
import hla.rti1516_2025.exceptions.Unauthorized;
import java.util.UUID;

/** Checks configured custom-authorizer decisions through Java federation services. */
final class AuthorizerFederationServiceDecisionTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-federation";
   private static final String DENIED_FEDERATION = "tck-authorizer-denied-federation";

   private AuthorizerFederationServiceDecisionTck() { }

   static void run(RtiFactory factory, String fomPath, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      String allowedFederation = "java-tck-authorizer-allowed-" + UUID.randomUUID();
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean deniedFederationCreated = false;
      boolean allowedFederationCreated = false;
      boolean joinedDeniedFederation = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel, configuration);
         connected = true;

         boolean deniedCreate = false;
         try {
            ambassador.createFederationExecution(
               DENIED_FEDERATION, fomPath, timeImplementation);
            deniedFederationCreated = true;
         } catch (Unauthorized expected) {
            deniedCreate = true;
         }
         JavaTckSupport.check(deniedCreate,
            "Create Federation Execution did not apply the custom Authorizer decision");

         boolean deniedCreateLeftNoFederation = false;
         try {
            ambassador.joinFederationExecution(
               "tck-authorizer-denied-join", "tck-type", DENIED_FEDERATION);
            joinedDeniedFederation = true;
         } catch (FederationExecutionDoesNotExist expected) {
            deniedCreateLeftNoFederation = true;
         }
         JavaTckSupport.check(deniedCreateLeftNoFederation,
            "denied Create Federation Execution left a federation that could be joined");

         ambassador.createFederationExecution(
            allowedFederation, fomPath, timeImplementation);
         allowedFederationCreated = true;
         ambassador.destroyFederationExecution(allowedFederation);
         allowedFederationCreated = false;
      } finally {
         if (joinedDeniedFederation) {
            try {
               ambassador.resignFederationExecution(ResignAction.NO_ACTION);
            } catch (Exception ignored) {
            }
         }
         if (allowedFederationCreated) {
            try {
               ambassador.destroyFederationExecution(allowedFederation);
            } catch (Exception ignored) {
            }
         }
         if (deniedFederationCreated) {
            try {
               ambassador.destroyFederationExecution(DENIED_FEDERATION);
            } catch (Exception ignored) {
            }
         }
         if (connected) ambassador.disconnect();
      }
   }
}
