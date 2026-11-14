package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.UUID;
import java.util.concurrent.atomic.AtomicInteger;

/** Checks that evokeCallback has no effect in HLA_IMMEDIATE. */
final class EvokeCallbackImmediateNoEffectTck {
   private static final double REQUESTED_WAIT_SECONDS = 0.25;

   private EvokeCallbackImmediateNoEffectTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_IMMEDIATE,
         "evokeCallback no-effect testing requires HLA_IMMEDIATE");

      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-evoke-callback-immediate-" + UUID.randomUUID();
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution(
            "immediate-callback-federate", "immediate-callback-type", federation);
         joined = true;

         ambassador.disableCallbacks();
         ambassador.listFederationExecutions();
         JavaTckSupport.check(recorder.reportCount.get() == 0,
            "disabled HLA_IMMEDIATE callbacks ran before the evokeCallback no-effect check");

         long started = System.nanoTime();
         ambassador.evokeCallback(REQUESTED_WAIT_SECONDS);
         long elapsed = System.nanoTime() - started;
         JavaTckSupport.check(recorder.reportCount.get() == 0,
            "evokeCallback invoked a queued HLA_IMMEDIATE callback while callbacks were disabled");
         JavaTckSupport.check(elapsed < 125_000_000L,
            "evokeCallback applied its wait argument in HLA_IMMEDIATE despite having no effect");

         ambassador.enableCallbacks();
         JavaTckSupport.check(recorder.reportCount.get() == 1,
            "enabling HLA_IMMEDIATE callbacks did not deliver the report retained by evokeCallback");
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
      private final AtomicInteger reportCount = new AtomicInteger();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) reportCount.incrementAndGet();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK HLA_IMMEDIATE recorder";
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
