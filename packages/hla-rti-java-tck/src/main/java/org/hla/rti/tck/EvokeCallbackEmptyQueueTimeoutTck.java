package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

/** Checks the minimum wait and empty-queue result of evokeCallback. */
final class EvokeCallbackEmptyQueueTimeoutTck {
   private static final double REQUESTED_WAIT_SECONDS = 0.05;

   private EvokeCallbackEmptyQueueTimeoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "evokeCallback empty-queue timeout testing requires HLA_EVOKED");

      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-evoke-callback-empty-queue-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(noopFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "empty-queue-federate", "empty-queue-type", federation);
         joined = true;

         long started = System.nanoTime();
         boolean callbackPending = ambassador.evokeCallback(REQUESTED_WAIT_SECONDS);
         long elapsed = System.nanoTime() - started;
         long requestedWait = TimeUnit.MILLISECONDS.toNanos(50);
         long precision = TimeUnit.MILLISECONDS.toNanos(1);

         JavaTckSupport.check(!callbackPending,
            "evokeCallback reported pending callbacks when the joined federate had none");
         JavaTckSupport.check(elapsed + precision >= requestedWait,
            "evokeCallback returned before its requested empty-queue timeout");
      } finally {
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK empty-queue recorder";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
