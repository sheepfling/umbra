package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.NullFederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.util.UUID;

/** Portable connection-state guards around federation creation and resignation. */
final class ConnectBeforeFederationDisconnectAfterResignTck {
   private ConnectBeforeFederationDisconnectAfterResignTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String federateType, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador federate = factory.getRtiAmbassador();
      String federation = "java-tck-connect-disconnect-resign-" + UUID.randomUUID();
      String federateName = "java-tck-connect-disconnect-member-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean federateConnected = false;
      boolean created = false;
      boolean federateJoined = false;
      try {
         JavaTckSupport.expectFailure(
            () -> federate.createFederationExecution(federation, fom),
            "NotConnected", "creating a federation execution before connect");

         owner.connect(new NullFederateAmbassador(), callbackModel);
         ownerConnected = true;
         federate.connect(new NullFederateAmbassador(), callbackModel);
         federateConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         federate.joinFederationExecution(federateName, federateType, federation);
         federateJoined = true;
         federate.resignFederationExecution(ResignAction.NO_ACTION);
         federateJoined = false;
         federate.disconnect();
         federateConnected = false;

         JavaTckSupport.expectFailure(
            () -> federate.createFederationExecution(federation, fom),
            "NotConnected", "creating a federation execution after resign and disconnect");

         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (federateJoined) {
            try { federate.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (federateConnected) {
            try { federate.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }
}
