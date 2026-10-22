package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import java.lang.reflect.Proxy;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Exercises single and multiple object-name reservation lifecycle services. */
final class ObjectNameReservationLifecycleTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private ObjectNameReservationLifecycleTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador unjoined = factory.getRtiAmbassador();
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador peer = factory.getRtiAmbassador();
      Recorder unjoinedCallbacks = new Recorder();
      Recorder ownerCallbacks = new Recorder();
      Recorder peerCallbacks = new Recorder();
      String federation = "java-tck-object-name-reservation-lifecycle-" + UUID.randomUUID();
      boolean unjoinedConnected = false, ownerConnected = false, peerConnected = false;
      boolean created = false, ownerJoined = false, peerJoined = false;
      Set<String> lifecycleNames = Set.of("tck-lifecycle-reservation-a",
         "tck-lifecycle-reservation-b");
      try {
         expectReservationLifecycle(unjoined, lifecycleNames, "NotConnected", "before connect");
         unjoined.connect(unjoinedCallbacks.proxy(), callbackModel);
         unjoinedConnected = true;
         expectReservationLifecycle(unjoined, lifecycleNames, "FederateNotExecutionMember",
            "after connect before join");

         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         peer.connect(peerCallbacks.proxy(), callbackModel);
         peerConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-name-lifecycle-owner", federation);
         ownerJoined = true;
         peer.joinFederationExecution("java-tck-name-lifecycle-peer", federation);
         peerJoined = true;

         JavaTckSupport.expectFailure(() -> owner.reserveObjectInstanceName(""), "IllegalName",
            "reserving an empty object name");
         JavaTckSupport.expectFailure(() -> owner.reserveObjectInstanceName("HLA.TckReserved"),
            "IllegalName", "reserving an HLA-namespace object name");
         JavaTckSupport.expectFailure(() -> owner.reserveMultipleObjectInstanceNames(Set.of()),
            "NameSetWasEmpty", "reserving an empty object-name set");
         JavaTckSupport.expectFailure(() -> owner.reserveMultipleObjectInstanceNames(
            Set.of("tck-valid-object-name", "HLA.TckInvalidObjectName")), "IllegalName",
            "reserving a set containing an HLA-namespace object name");

         String singleName = federation + "-single-reservation";
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(singleName);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!ownerCallbacks.singleSucceeded.contains(singleName),
               "evoked reservation callback arrived before callback servicing");
         }
         await(owner, callbackModel, () -> ownerCallbacks.singleSucceeded.contains(singleName),
            "single object-name reservation success");

         peerCallbacks.clear();
         peer.reserveObjectInstanceName(singleName);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!peerCallbacks.singleFailed.contains(singleName),
               "evoked reservation failure callback arrived before callback servicing");
         }
         await(peer, callbackModel, () -> peerCallbacks.singleFailed.contains(singleName),
            "contended object-name reservation failure");

         owner.releaseObjectInstanceName(singleName);
         JavaTckSupport.expectFailure(() -> owner.releaseObjectInstanceName(singleName),
            "ObjectInstanceNameNotReserved", "releasing an object name twice");
         peerCallbacks.clear();
         peer.reserveObjectInstanceName(singleName);
         await(peer, callbackModel, () -> peerCallbacks.singleSucceeded.contains(singleName),
            "reusing a released object name");
         peer.releaseObjectInstanceName(singleName);

         String multipleA = federation + "-multiple-a";
         String multipleB = federation + "-multiple-b";
         String multipleC = federation + "-multiple-c";
         Set<String> ownerNames = Set.of(multipleA, multipleB);
         Set<String> peerNames = Set.of(multipleB, multipleC);
         Set<String> peerSucceeded = Set.of(multipleC);
         Set<String> peerFailed = Set.of(multipleB);
         ownerCallbacks.clear();
         owner.reserveMultipleObjectInstanceNames(ownerNames);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!ownerCallbacks.multipleSucceeded.contains(ownerNames),
               "evoked multiple-reservation callback arrived before callback servicing");
         }
         await(owner, callbackModel,
            () -> ownerCallbacks.multipleSucceeded.contains(ownerNames),
            "multiple object-name reservation success");

         peerCallbacks.clear();
         peer.reserveMultipleObjectInstanceNames(peerNames);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(!peerCallbacks.multipleSucceeded.contains(peerSucceeded)
                  && !peerCallbacks.multipleFailed.contains(peerFailed),
               "evoked mixed multiple-reservation callbacks arrived before callback servicing");
         }
         await(peer, callbackModel,
            () -> peerCallbacks.multipleSucceeded.contains(peerSucceeded)
               && peerCallbacks.multipleFailed.contains(peerFailed),
            "mixed multiple object-name reservation results");

         JavaTckSupport.expectFailure(() -> owner.releaseMultipleObjectInstanceNames(
            Set.of(multipleA, multipleC)), "ObjectInstanceNameNotReserved",
            "releasing a mixed-owner object-name set");
         peer.releaseObjectInstanceName(multipleC);
         owner.releaseMultipleObjectInstanceNames(ownerNames);
         owner.releaseMultipleObjectInstanceNames(Set.of());

         String resignedName = federation + "-released-on-resign";
         ownerCallbacks.clear();
         owner.reserveObjectInstanceName(resignedName);
         await(owner, callbackModel, () -> ownerCallbacks.singleSucceeded.contains(resignedName),
            "object-name reservation before resignation");
         owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         ownerJoined = false;

         peerCallbacks.clear();
         peer.reserveObjectInstanceName(resignedName);
         await(peer, callbackModel, () -> peerCallbacks.singleSucceeded.contains(resignedName),
            "object-name reuse after owner resignation");
         peer.releaseObjectInstanceName(resignedName);
         peer.resignFederationExecution(ResignAction.NO_ACTION);
         peerJoined = false;
         peer.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (peerJoined) {
            try { peer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (peerConnected) {
            try { peer.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
         if (unjoinedConnected) {
            try { unjoined.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void expectReservationLifecycle(RTIambassador ambassador,
         Set<String> names, String expected, String phase) throws Exception {
      JavaTckSupport.expectFailure(() -> ambassador.reserveObjectInstanceName(
         names.iterator().next()), expected, "reserving an object name " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.releaseObjectInstanceName(
         names.iterator().next()), expected, "releasing an object name " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.reserveMultipleObjectInstanceNames(names),
         expected, "reserving multiple object names " + phase);
      JavaTckSupport.expectFailure(() -> ambassador.releaseMultipleObjectInstanceNames(names),
         expected, "releasing multiple object names " + phase);
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final List<String> singleSucceeded = new CopyOnWriteArrayList<>();
      private final List<String> singleFailed = new CopyOnWriteArrayList<>();
      private final List<Set<String>> multipleSucceeded = new CopyOnWriteArrayList<>();
      private final List<Set<String>> multipleFailed = new CopyOnWriteArrayList<>();

      private void clear() {
         singleSucceeded.clear();
         singleFailed.clear();
         multipleSucceeded.clear();
         multipleFailed.clear();
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if (arguments != null && arguments.length == 1) {
                  if ("objectInstanceNameReservationSucceeded".equals(name)) {
                     singleSucceeded.add((String) arguments[0]);
                  } else if ("objectInstanceNameReservationFailed".equals(name)) {
                     singleFailed.add((String) arguments[0]);
                  } else if ("multipleObjectInstanceNameReservationSucceeded".equals(name)) {
                     multipleSucceeded.add(copyNames(arguments[0]));
                  } else if ("multipleObjectInstanceNameReservationFailed".equals(name)) {
                     multipleFailed.add(copyNames(arguments[0]));
                  }
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-name reservation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }

      private static Set<String> copyNames(Object value) {
         Set<String> copy = new LinkedHashSet<>();
         for (Object name : (Set<?>) value) {
            if (name instanceof String) copy.add((String) name);
         }
         return Set.copyOf(copy);
      }
   }
}
