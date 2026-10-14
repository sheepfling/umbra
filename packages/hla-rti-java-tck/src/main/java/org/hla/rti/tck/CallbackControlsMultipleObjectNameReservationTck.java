package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;

/** Portable callback gating for a mixed multiple-name reservation result. */
final class CallbackControlsMultipleObjectNameReservationTck {
   private CallbackControlsMultipleObjectNameReservationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder ownerRecorder = new Recorder();
      Recorder peerRecorder = new Recorder();
      String federation = "java-tck-callback-multiple-name-reservation-" + UUID.randomUUID();
      String firstName = federation + "-multiple-a";
      String secondName = federation + "-multiple-b";
      String thirdName = federation + "-multiple-c";
      Set<String> ownerNames = Set.of(firstName, secondName);
      Set<String> peerNames = Set.of(secondName, thirdName);
      Set<String> peerSucceeded = Set.of(thirdName);
      Set<String> peerFailed = Set.of(secondName);
      boolean ownerConnected = false;
      boolean peerConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean peerJoined = false;
      boolean ownerMayHaveReservations = false;
      boolean peerMayHaveReservation = false;
      try {
         owner.connect(ownerRecorder.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerRecorder.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-multiple-reservation-owner", federation);
         ownerJoined = true;
         peer.joinFederationExecution("java-tck-multiple-reservation-peer", federation);
         peerJoined = true;

         ownerRecorder.clear();
         owner.disableCallbacks();
         owner.reserveMultipleObjectInstanceNames(ownerNames);
         ownerMayHaveReservations = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(owner.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit the multiple-name success event");
         } else {
            service(owner, objectClassName);
         }
         JavaTckSupport.check(ownerRecorder.successes().isEmpty()
               && ownerRecorder.failures().isEmpty(),
            "disabled callbacks exposed a multiple-name reservation result");

         owner.enableCallbacks();
         await(owner, callbackModel, objectClassName,
            () -> ownerRecorder.hasSuccess(ownerNames),
            "re-enabled multiple-name reservation success callback did not arrive");
         JavaTckSupport.check(ownerRecorder.successes().equals(List.of(ownerNames))
               && ownerRecorder.failures().isEmpty(),
            "owner's multiple-name reservation returned an unexpected result partition");

         peerRecorder.clear();
         peer.disableCallbacks();
         peer.reserveMultipleObjectInstanceNames(peerNames);
         peerMayHaveReservation = true;
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(peer.evokeMultipleCallbacks(0.0, 1.0),
               "evoked callback control did not admit mixed multiple-name results");
         } else {
            service(peer, objectClassName);
         }
         JavaTckSupport.check(peerRecorder.successes().isEmpty()
               && peerRecorder.failures().isEmpty(),
            "disabled callbacks exposed mixed multiple-name reservation results");

         peer.enableCallbacks();
         await(peer, callbackModel, objectClassName,
            () -> peerRecorder.hasSuccess(peerSucceeded) && peerRecorder.hasFailure(peerFailed),
            "re-enabled multiple-name callbacks did not report both result subsets");
         JavaTckSupport.check(peerRecorder.successes().equals(List.of(peerSucceeded))
               && peerRecorder.failures().equals(List.of(peerFailed)),
            "peer's multiple-name reservation returned the wrong success/failure partition");

         peer.releaseObjectInstanceName(thirdName);
         peerMayHaveReservation = false;
         owner.releaseMultipleObjectInstanceNames(ownerNames);
         ownerMayHaveReservations = false;
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         owner.resignFederationExecution(ResignAction.NO_ACTION);
         ownerJoined = false;
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (peerMayHaveReservation) {
            try { peer.releaseObjectInstanceName(thirdName); }
            catch (Exception ignored) { }
         }
         if (ownerMayHaveReservations) {
            try { owner.releaseMultipleObjectInstanceNames(ownerNames); }
            catch (Exception ignored) { }
         }
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (peerConnected) {
            try { peer.disconnect(); }
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
      private final List<Set<String>> successes = new CopyOnWriteArrayList<>();
      private final List<Set<String>> failures = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("multipleObjectInstanceNameReservationSucceeded".equals(name)) {
                  successes.add(Set.copyOf(new HashSet<>((Set<String>) arguments[0])));
               } else if ("multipleObjectInstanceNameReservationFailed".equals(name)) {
                  failures.add(Set.copyOf(new HashSet<>((Set<String>) arguments[0])));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK multiple-name callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private boolean hasSuccess(Set<String> names) { return successes.contains(names); }

      private boolean hasFailure(Set<String> names) { return failures.contains(names); }

      private List<Set<String>> successes() { return List.copyOf(successes); }

      private List<Set<String>> failures() { return List.copyOf(failures); }

      private void clear() {
         successes.clear();
         failures.clear();
      }
   }
}
