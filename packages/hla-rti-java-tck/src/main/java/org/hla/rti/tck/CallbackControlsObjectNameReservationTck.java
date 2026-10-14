package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback-enable gating for ordinary object-name reservation. */
final class CallbackControlsObjectNameReservationTck {
   private CallbackControlsObjectNameReservationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador ambassador = factory.getRtiAmbassador();
      Recorder recorder = new Recorder();
      String federation = "java-tck-callback-name-reservation-" + UUID.randomUUID();
      String objectName = federation + "-callback-controlled-name";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      boolean reserved = false;
      try {
         ambassador.connect(recorder.proxy(), callbackModel);
         connected = true;
         ambassador.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         ambassador.joinFederationExecution("java-tck-name-reserver", federation);
         joined = true;

         recorder.clear();
         ambassador.disableCallbacks();
         ambassador.reserveObjectInstanceName(objectName);
         reserved = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(ambassador.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending name-reservation event");
         } else {
            service(ambassador, objectClassName);
         }
         JavaTckSupport.check(!recorder.succeeded(objectName),
            "disabled callbacks exposed an object-name reservation success callback");

         ambassador.enableCallbacks();
         await(ambassador, callbackModel, objectClassName,
            () -> recorder.succeeded(objectName),
            "re-enabled object-name reservation callback did not arrive");
         JavaTckSupport.check(recorder.successCount(objectName) == 1
               && !recorder.failed(objectName),
            "object-name reservation returned an unexpected callback result");
         ambassador.releaseObjectInstanceName(objectName);
         reserved = false;

         ambassador.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         ambassador.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (reserved) {
            try { ambassador.releaseObjectInstanceName(objectName); }
            catch (Exception ignored) { }
         }
         if (joined) {
            try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { ambassador.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { ambassador.disconnect(); }
            catch (Exception ignored) { }
         }
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         String objectClassName, Condition condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.isTrue()) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 1.0);
         } else {
            service(ambassador, objectClassName);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.isTrue(), "timed out waiting for " + description);
   }

   private static void service(RTIambassador ambassador, String objectClassName)
         throws Exception {
      ambassador.getObjectClassHandle(objectClassName);
   }

   private interface Condition {
      boolean isTrue();
   }

   private static final class Recorder {
      private final List<String> successes = new CopyOnWriteArrayList<>();
      private final List<String> failures = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("objectInstanceNameReservationSucceeded".equals(name)) {
                  successes.add((String) arguments[0]);
               } else if ("objectInstanceNameReservationFailed".equals(name)) {
                  failures.add((String) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK name-reservation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean succeeded(String name) { return successes.contains(name); }

      private boolean failed(String name) { return failures.contains(name); }

      private long successCount(String name) {
         return successes.stream().filter(name::equals).count();
      }

      private void clear() {
         successes.clear();
         failures.clear();
      }
   }
}
