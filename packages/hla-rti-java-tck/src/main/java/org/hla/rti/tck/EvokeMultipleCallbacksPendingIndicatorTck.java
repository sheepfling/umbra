package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

/** Verifies pending-state reporting and the HLA_IMMEDIATE no-effect behavior. */
final class EvokeMultipleCallbacksPendingIndicatorTck {
   private static final double REQUESTED_MINIMUM_WAIT_SECONDS = 0.25;
   private static final double REQUESTED_MAXIMUM_WAIT_SECONDS = 0.50;
   private static final long IMMEDIATE_NO_EFFECT_LIMIT_NANOS =
      TimeUnit.MILLISECONDS.toNanos(125);

   private EvokeMultipleCallbacksPendingIndicatorTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED
            || callbackModel == CallbackModel.HLA_IMMEDIATE,
         "evokeMultipleCallbacks pending-indicator testing requires a supported callback model");

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-evoke-multiple-pending-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "multiple-pending-federate", "multiple-pending-type", federation);
         joined = true;

         if (callbackModel == CallbackModel.HLA_IMMEDIATE) {
            verifyImmediateNoEffect(ambassador, recorder);
            return;
         }

         boolean callbackPending = ambassador.evokeMultipleCallbacks(0.0, 0.0);
         JavaTckSupport.check(!callbackPending,
            "evokeMultipleCallbacks reported pending callbacks for an empty queue");

         ambassador.disableCallbacks();
         ambassador.listFederationExecutions();
         long pendingDeadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(1);
         do {
            callbackPending = ambassador.evokeMultipleCallbacks(0.0, 0.0);
            if (callbackPending) break;
            Thread.sleep(1);
         } while (System.nanoTime() < pendingDeadline);

         JavaTckSupport.check(recorder.reportCount.get() == 0,
            "disabled HLA_EVOKED callbacks ran before the pending-indicator check");
         JavaTckSupport.check(callbackPending,
            "evokeMultipleCallbacks did not report TRUE while a federation-list callback was pending");

         ambassador.enableCallbacks();
         callbackPending = ambassador.evokeMultipleCallbacks(0.001, 0.050);
         JavaTckSupport.check(recorder.reportCount.get() == 1,
            "evokeMultipleCallbacks did not deliver the queued report after callbacks were enabled");
         JavaTckSupport.check(!callbackPending,
            "evokeMultipleCallbacks returned TRUE after the only callback was consumed");
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

   private static void verifyImmediateNoEffect(
         RTIambassador ambassador, Recorder recorder) throws Exception {
      ambassador.disableCallbacks();
      ambassador.listFederationExecutions();
      JavaTckSupport.check(recorder.reportCount.get() == 0,
         "disabled HLA_IMMEDIATE callbacks ran before the evokeMultipleCallbacks no-effect check");

      long started = System.nanoTime();
      ambassador.evokeMultipleCallbacks(
         REQUESTED_MINIMUM_WAIT_SECONDS, REQUESTED_MAXIMUM_WAIT_SECONDS);
      long elapsed = System.nanoTime() - started;
      JavaTckSupport.check(recorder.reportCount.get() == 0,
         "evokeMultipleCallbacks invoked a queued HLA_IMMEDIATE callback while callbacks were disabled");
      JavaTckSupport.check(elapsed < IMMEDIATE_NO_EFFECT_LIMIT_NANOS,
         "evokeMultipleCallbacks applied its wait arguments in HLA_IMMEDIATE despite having no effect");

      ambassador.enableCallbacks();
      JavaTckSupport.check(recorder.reportCount.get() == 1,
         "enabling HLA_IMMEDIATE callbacks did not deliver the report retained by evokeMultipleCallbacks");
   }

   private static final class Recorder {
      private final AtomicInteger reportCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) reportCount.incrementAndGet();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK evokeMultipleCallbacks recorder";
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
