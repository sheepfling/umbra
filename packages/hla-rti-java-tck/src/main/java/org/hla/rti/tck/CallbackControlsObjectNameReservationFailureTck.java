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

/** Portable callback-enable gating for a contended object-name reservation. */
final class CallbackControlsObjectNameReservationFailureTck {
   private CallbackControlsObjectNameReservationFailureTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder requesterRecorder = new Recorder();
      String federation = "java-tck-callback-name-reservation-failure-" + UUID.randomUUID();
      String objectName = federation + "-callback-controlled-contended-name";
      boolean ownerConnected = false;
      boolean requesterConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean requesterJoined = false;
      boolean ownerReserved = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         requester.connect(requesterRecorder.proxy(), callbackModel);
         requesterConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-reservation-owner", federation);
         ownerJoined = true;
         requester.joinFederationExecution("java-tck-reservation-requester", federation);
         requesterJoined = true;

         owner.reserveObjectInstanceName(objectName);
         ownerReserved = true;
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.succeeded(objectName),
            "baseline object-name reservation did not succeed");
         JavaTckSupport.check(ownerRecorder.successCount(objectName) == 1
               && !ownerRecorder.failed(objectName),
            "owner reservation returned an unexpected callback result");

         requesterRecorder.clear();
         requester.disableCallbacks();
         requester.reserveObjectInstanceName(objectName);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(requester.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the pending reservation failure event");
         } else {
            service(requester, objectClassName);
         }
         JavaTckSupport.check(!requesterRecorder.failed(objectName),
            "disabled callbacks exposed an object-name reservation failure callback");

         requester.enableCallbacks();
         await(requester, callbackModel, objectClassName,
            () -> requesterRecorder.failed(objectName),
            "re-enabled object-name reservation failure callback did not arrive");
         JavaTckSupport.check(requesterRecorder.failureCount(objectName) == 1
               && !requesterRecorder.succeeded(objectName),
            "contended name reservation returned an unexpected callback result");

         owner.releaseObjectInstanceName(objectName);
         ownerReserved = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (ownerReserved) {
            try { owner.releaseObjectInstanceName(objectName); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) {
            try { requester.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) {
            try { requester.disconnect(); }
            catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); }
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
                  if ("toString".equals(name)) return "Java TCK reservation-failure callbacks";
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

      private long failureCount(String name) {
         return failures.stream().filter(name::equals).count();
      }

      private void clear() {
         successes.clear();
         failures.clear();
      }
   }
}
