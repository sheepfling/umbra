package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiConfiguration;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Verifies the joined handle after a public connection-fault replay scenario. */
final class ConnectionFaultFederateRequestReplayTck {
   private ConnectionFaultFederateRequestReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration,
         RtiConfiguration recoveringConfiguration) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel,
         ownerConfiguration, recoveringConfiguration, false);
   }

   static void runJoinResponse(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration,
         RtiConfiguration recoveringConfiguration) throws Exception {
      run(factory, fom, mim, timeImplementation, callbackModel,
         ownerConfiguration, recoveringConfiguration, true);
   }

   private static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration,
         RtiConfiguration recoveringConfiguration, boolean joinResponseReplay)
         throws Exception {
      Member owner = new Member(factory.getRtiAmbassador());
      Member recovering = new Member(factory.getRtiAmbassador());
      String suffix = UUID.randomUUID().toString();
      String replayName = joinResponseReplay ? "Join" : "GetFederateHandle";
      String federation = "java-tck-" + (joinResponseReplay
         ? "join-response-replay-" : "federate-request-replay-") + suffix;
      String ownerName = "java-tck-owner-" + suffix;
      String recoveringName = "java-tck-recovering-member-" + suffix;
      boolean created = false;
      try {
         owner.connect(callbackModel, ownerConfiguration);
         owner.rti.createFederationExecutionWithMIM(federation,
            new String[] {fom}, mim, timeImplementation);
         created = true;
         owner.join(ownerName, federation);

         recovering.connect(callbackModel, recoveringConfiguration);
         FederateHandle joinedHandle = recovering.join(recoveringName, federation);
         FederateHandle recoveredHandle = recovering.rti.getFederateHandle(recoveringName);
         JavaTckSupport.check(recoveredHandle != null && recoveredHandle.equals(joinedHandle),
            "replayed " + replayName + " result did not identify the joined federate");

         recovering.resign();
         owner.resign();
         if (!joinResponseReplay) owner.rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         recovering.resignQuietly();
         owner.resignQuietly();
         if (created) {
            try { owner.rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         recovering.disconnectQuietly();
         owner.disconnectQuietly();
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK request-replay callbacks";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }

   private static final class Member {
      private final RTIambassador rti;
      private boolean connected;
      private boolean joined;

      private Member(RTIambassador rti) { this.rti = rti; }

      private void connect(CallbackModel callbackModel,
            RtiConfiguration connectionConfiguration) throws Exception {
         rti.connect(callbacks(), callbackModel, connectionConfiguration);
         connected = true;
      }

      private FederateHandle join(String name, String federation) throws Exception {
         FederateHandle handle = rti.joinFederationExecution(
            name, "java-tck-federate", federation);
         joined = true;
         return handle;
      }

      private void resign() throws Exception {
         if (joined) {
            rti.resignFederationExecution(ResignAction.NO_ACTION);
            joined = false;
         }
      }

      private void resignQuietly() {
         try { resign(); }
         catch (Exception ignored) { joined = false; }
      }

      private void disconnectQuietly() {
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
            connected = false;
         }
      }
   }
}
