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

/** Checks that a denied Join does not prevent an authorized retry. */
final class AuthorizerJoinServiceDecisionTck {
   private static final String CONFIGURATION_NAME = "tck-authorizer-join";
   private static final String FEDERATE_NAME = "tck-authorizer-join-retry";

   private AuthorizerJoinServiceDecisionTck() { }

   static void run(RtiFactory factory, String fomPath, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RtiConfiguration configuration = RtiConfiguration.createConfiguration()
         .withConfigurationName(CONFIGURATION_NAME);
      String federation = "java-tck-authorizer-join-" + UUID.randomUUID();
      RTIambassador ambassador = factory.getRtiAmbassador();
      boolean connected = false;
      boolean federationCreated = false;
      boolean joined = false;
      try {
         ambassador.connect(new NullFederateAmbassador(), callbackModel, configuration);
         connected = true;
         ambassador.createFederationExecution(federation, fomPath, timeImplementation);
         federationCreated = true;

         boolean joinDenied = false;
         try {
            ambassador.joinFederationExecution(FEDERATE_NAME, "tck-type", federation);
            joined = true;
         } catch (Unauthorized expected) {
            joinDenied = true;
         }
         JavaTckSupport.check(joinDenied,
            "Join Federation Execution did not apply the custom Authorizer denial");

         FederateHandle joinedFederate = ambassador.joinFederationExecution(
            FEDERATE_NAME, "tck-type", federation);
         joined = true;
         JavaTckSupport.check(joinedFederate != null,
            "Join denial prevented a subsequent authorized join");
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
