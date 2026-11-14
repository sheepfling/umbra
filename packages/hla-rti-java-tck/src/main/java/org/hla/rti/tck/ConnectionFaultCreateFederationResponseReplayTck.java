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

/** Replays an accepted federation-creation result after an adapter-managed response fault. */
final class ConnectionFaultCreateFederationResponseReplayTck {
   private ConnectionFaultCreateFederationResponseReplayTck() { }

   static void run(RtiFactory factory, String fom, String mim,
         String timeImplementation, CallbackModel callbackModel,
         RtiConfiguration ownerConfiguration) throws Exception {
      RTIambassador rti = factory.getRtiAmbassador();
      String suffix = UUID.randomUUID().toString();
      String federation = "java-tck-create-response-replay-" + suffix;
      String federateName = "java-tck-create-response-owner-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         rti.connect(callbacks(), callbackModel, ownerConfiguration);
         connected = true;
         rti.createFederationExecutionWithMIM(
            federation, new String[] {fom}, mim, timeImplementation);
         created = true;

         FederateHandle joinedHandle = rti.joinFederationExecution(
            federateName, "java-tck-federate", federation);
         joined = true;
         FederateHandle queriedHandle = rti.getFederateHandle(federateName);
         JavaTckSupport.check(queriedHandle != null && queriedHandle.equals(joinedHandle),
            "replayed CreateFederationExecutionWithMIM response did not leave an execution that accepted Join");

         rti.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         rti.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { rti.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { rti.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { rti.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador callbacks() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(method.getName())) {
                  return "Java TCK federation-create-replay callbacks";
               }
               if ("hashCode".equals(method.getName())) return System.identityHashCode(proxy);
               if ("equals".equals(method.getName())) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
