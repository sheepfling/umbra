package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;

/** Checks that adjacent millisecond evokeCallback waits remain distinguishable. */
final class EvokeCallbackWaitDurationPrecisionTck {
   private static final int SAMPLE_COUNT = 200;
   private static final long EXPECTED_STEP_NANOS = 1_000_000L;
   private static final long MEASUREMENT_TOLERANCE_NANOS = 750_000L;

   private EvokeCallbackWaitDurationPrecisionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "evokeCallback wait-duration precision testing requires HLA_EVOKED");

      RTIambassador ambassador = factory.getRtiAmbassador();
      String federation = "java-tck-evoke-callback-wait-precision-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(noopFederateAmbassador(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "wait-precision-federate", "wait-precision-type", federation);
         joined = true;

         long fifteenMillisecondElapsed = 0L;
         long sixteenMillisecondElapsed = 0L;
         for (int sample = 0; sample < SAMPLE_COUNT; sample++) {
            if ((sample & 1) == 0) {
               fifteenMillisecondElapsed += measureCall(ambassador, 0.015);
               sixteenMillisecondElapsed += measureCall(ambassador, 0.016);
            } else {
               sixteenMillisecondElapsed += measureCall(ambassador, 0.016);
               fifteenMillisecondElapsed += measureCall(ambassador, 0.015);
            }
         }

         long observedStep = (sixteenMillisecondElapsed - fifteenMillisecondElapsed)
            / SAMPLE_COUNT;
         JavaTckSupport.check(
            observedStep + MEASUREMENT_TOLERANCE_NANOS >= EXPECTED_STEP_NANOS
               && observedStep <= EXPECTED_STEP_NANOS + MEASUREMENT_TOLERANCE_NANOS,
            "evokeCallback did not preserve a 1 ms wait-argument change; average accumulated "
               + "step was " + observedStep / 1_000_000.0 + " ms");
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

   private static long measureCall(RTIambassador ambassador, double waitSeconds)
         throws Exception {
      long started = System.nanoTime();
      boolean callbackPending = ambassador.evokeCallback(waitSeconds);
      long elapsed = System.nanoTime() - started;
      JavaTckSupport.check(!callbackPending,
         "evokeCallback reported a pending callback on the empty-queue precision path");
      return elapsed;
   }

   private static FederateAmbassador noopFederateAmbassador() {
      return (FederateAmbassador) Proxy.newProxyInstance(
         FederateAmbassador.class.getClassLoader(),
         new Class<?>[] {FederateAmbassador.class},
         (proxy, method, arguments) -> {
            String name = method.getName();
            if (method.getDeclaringClass() == Object.class) {
               if ("toString".equals(name)) return "Java TCK evokeCallback precision recorder";
               if ("hashCode".equals(name)) return System.identityHashCode(proxy);
               if ("equals".equals(name)) {
                  return proxy == (arguments == null ? null : arguments[0]);
               }
            }
            return null;
         });
   }
}
