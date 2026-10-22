package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.exceptions.CallNotAllowedFromWithinCallback;
import java.lang.reflect.Proxy;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/** Checks the standard callback-servicing re-entry exception boundary. */
final class CallbackReentrancyTck {
   private CallbackReentrancyTck() { }

   static void run(RtiFactory factory, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder(ambassador);
      boolean connected = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.listFederationExecutions();
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(10);
            while (!recorder.hasReport() && System.nanoTime() < deadline) {
               ambassador.evokeMultipleCallbacks(0.0, 0.01);
            }
         }
         JavaTckSupport.check(recorder.awaitReport(),
            "callback re-entrancy test timed out waiting for a federation-list callback");

         JavaTckSupport.check(recorder.reportCount.get() == 1,
            "callback re-entrancy test did not receive exactly one federation-list callback");
         JavaTckSupport.check(recorder.evokeRejected.get()
               && recorder.multipleRejected.get(),
            "callback re-entrancy test admitted callback-service re-entry");
      } finally {
         if (connected) {
            try { ambassador.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private final AtomicInteger reportCount = new AtomicInteger();
      private final AtomicBoolean evokeRejected = new AtomicBoolean();
      private final AtomicBoolean multipleRejected = new AtomicBoolean();
      private final CountDownLatch reportDelivered = new CountDownLatch(1);

      private Recorder(RTIambassador ambassador) {
         this.ambassador = ambassador;
      }

      private boolean awaitReport() throws InterruptedException {
         return reportDelivered.await(10, TimeUnit.SECONDS);
      }

      private boolean hasReport() {
         return reportDelivered.getCount() == 0;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("reportFederationExecutions".equals(name)) {
                  try {
                     reportCount.incrementAndGet();
                     try {
                        ambassador.evokeCallback(0.0);
                     } catch (CallNotAllowedFromWithinCallback expected) {
                        evokeRejected.set(true);
                     }
                     try {
                        ambassador.evokeMultipleCallbacks(0.0, 0.0);
                     } catch (CallNotAllowedFromWithinCallback expected) {
                        multipleRejected.set(true);
                     }
                  } finally {
                     reportDelivered.countDown();
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK callback-reentrancy recorder";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
