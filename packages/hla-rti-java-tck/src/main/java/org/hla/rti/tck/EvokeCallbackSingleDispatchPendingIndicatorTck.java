package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/** Verifies one-callback dispatch and the pending-callback result of evokeCallback. */
final class EvokeCallbackSingleDispatchPendingIndicatorTck {
   private EvokeCallbackSingleDispatchPendingIndicatorTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "evokeCallback pending-indicator testing requires HLA_EVOKED");

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder(ambassador);
      String federation = "java-tck-evoke-callback-pending-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "pending-indicator-federate", "pending-indicator-type", federation);
         joined = true;

         ambassador.listFederationExecutions();
         JavaTckSupport.check(recorder.reportCount.get() == 0,
            "listFederationExecutions invoked a report callback before an evokeCallback boundary");

         boolean callbackPending = ambassador.evokeCallback(1.0);
         JavaTckSupport.check(recorder.reportCount.get() == 1,
            "one evokeCallback invocation did not invoke exactly one federation-list callback");
         JavaTckSupport.check(recorder.secondReportRequested.get(),
            "the first federation-list callback did not queue the second report callback");
         JavaTckSupport.check(callbackPending,
            "evokeCallback returned FALSE while the second report callback was pending");

         callbackPending = ambassador.evokeCallback(0.0);
         JavaTckSupport.check(recorder.reportCount.get() == 2,
            "the second evokeCallback invocation did not invoke exactly the remaining callback");
         JavaTckSupport.check(!callbackPending,
            "evokeCallback returned TRUE after the only remaining callback was consumed");
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

   private static final class Recorder {
      private final RTIambassador ambassador;
      private final AtomicInteger reportCount = new AtomicInteger();
      private final AtomicBoolean secondReportRequested = new AtomicBoolean();

      private Recorder(RTIambassador ambassador) {
         this.ambassador = ambassador;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) {
                  if (reportCount.incrementAndGet() == 1) {
                     ambassador.listFederationExecutions();
                     secondReportRequested.set(true);
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK evokeCallback pending-indicator recorder";
                  }
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) {
                     return proxy == (arguments == null ? null : arguments[0]);
                  }
               }
               return null;
            });
      }
   }
}
