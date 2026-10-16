package org.hla.rti.tck;

import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Portable pre-connect guards for federation-management services. */
final class FederationManagementRequiresConnectionTck {
   private FederationManagementRequiresConnectionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation) throws Exception {
      RTIambassador disconnected = factory.getRtiAmbassador();
      String federation = "java-tck-federation-management-requires-connection-"
         + UUID.randomUUID();
      String federate = "java-tck-disconnected-federate-" + UUID.randomUUID();

      JavaTckSupport.expectFailure(
         () -> disconnected.createFederationExecution(federation, fom, timeImplementation),
         "NotConnected", "creating a federation execution before connect");
      JavaTckSupport.expectFailure(
         () -> disconnected.destroyFederationExecution(federation),
         "NotConnected", "destroying a federation execution before connect");
      JavaTckSupport.expectFailure(
         () -> disconnected.joinFederationExecution(federate, federation),
         "NotConnected", "joining a federation execution before connect");
      JavaTckSupport.expectFailure(
         () -> disconnected.resignFederationExecution(ResignAction.NO_ACTION),
         "NotConnected", "resigning from a federation execution before connect");
   }
}
