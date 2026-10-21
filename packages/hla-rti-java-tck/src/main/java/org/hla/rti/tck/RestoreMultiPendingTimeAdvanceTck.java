package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.BooleanSupplier;

/** Preserve two independently pending constrained TAR requests across restore. */
final class RestoreMultiPendingTimeAdvanceTck {
   private RestoreMultiPendingTimeAdvanceTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "multi-member pending-TAR restore has native evidence only for evoked callbacks");
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      CountDownLatch firstSaved = new CountDownLatch(1);
      CountDownLatch secondSaved = new CountDownLatch(1);
      Recorder firstEvents = new Recorder(first, firstSaved, secondSaved);
      Recorder secondEvents = new Recorder(second, secondSaved, null);
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-restore-multi-pending-tar-" + UUID.randomUUID();
      String saveLabel = "tck-multi-pending-time-advance";
      boolean created = false;
      try {
         first.connect(firstEvents.proxy(), callbackModel);
         connected.add(first);
         second.connect(secondEvents.proxy(), callbackModel);
         connected.add(second);
         first.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         first.joinFederationExecution("multi-pending-member", federation);
         joined.add(first);
         second.joinFederationExecution("multi-pending-member", federation);
         joined.add(second);

         LogicalTimeFactory<?, ?> firstFactory = first.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = second.getTimeFactory();
         JavaTckSupport.check(firstFactory.getName().equals(secondFactory.getName()),
            "multi-member pending-TAR members selected different time factories");
         LogicalTime<?, ?> firstInitial = first.queryLogicalTime();
         LogicalTime<?, ?> secondInitial = second.queryLogicalTime();
         LogicalTime<?, ?> firstFive = offset(firstInitial, firstFactory.makeEpsilon(), 5);
         LogicalTime<?, ?> secondFive = offset(secondInitial, secondFactory.makeEpsilon(), 5);
         LogicalTime<?, ?> firstNine = offset(firstInitial, firstFactory.makeEpsilon(), 9);
         LogicalTime<?, ?> secondNine = offset(secondInitial, secondFactory.makeEpsilon(), 9);

         first.enableTimeConstrained();
         second.enableTimeConstrained();
         awaitBoth(first, second,
            () -> firstEvents.constrainedEnabled == 1 && secondEvents.constrainedEnabled == 1,
            "both pending-TAR members constrained");
         firstEvents.order.clear();
         secondEvents.order.clear();
         firstEvents.completeSaveInCallback = true;
         secondEvents.completeSaveInCallback = true;
         first.requestFederationSave(saveLabel);
         JavaTckSupport.check(firstEvents.saveLabels.isEmpty()
               && secondEvents.saveLabels.isEmpty(),
            "evoked multi-member save initiation arrived before servicing");
         first.timeAdvanceRequest(firstFive);
         second.timeAdvanceRequest(secondFive);

         AtomicReference<Throwable> secondError = new AtomicReference<>();
         Thread secondRoute = new Thread(() -> {
            try {
               if (!firstSaved.await(10, TimeUnit.SECONDS))
                  throw new IllegalStateException("first save callback missed boundary");
               second.evokeCallback(0.0);
            } catch (Throwable error) {
               secondError.set(error);
               secondSaved.countDown();
            }
         }, "java-tck-second-save-route");
         secondRoute.start();
         Throwable firstError = null;
         try { first.evokeCallback(0.0); }
         catch (Throwable error) { firstError = error; }
         secondRoute.join(10_000);
         JavaTckSupport.check(!secondRoute.isAlive(),
            "second pending-TAR save callback did not finish");
         if (firstError != null)
            throw new IllegalStateException("first pending-TAR save callback failed", firstError);
         if (secondError.get() != null)
            throw new IllegalStateException("second pending-TAR save callback failed",
               secondError.get());
         JavaTckSupport.check(firstEvents.saveLabels.equals(List.of(saveLabel))
               && secondEvents.saveLabels.equals(List.of(saveLabel)),
            "both independent save callback routes did not initiate");

         drain(first);
         drain(second);
         JavaTckSupport.check(firstEvents.savedCount == 1 && secondEvents.savedCount == 1
               && firstEvents.notSavedCount == 0 && secondEvents.notSavedCount == 0,
            "multi-member pending-TAR save did not complete for both members");
         JavaTckSupport.check(firstEvents.grants.equals(List.of(firstFive))
               && secondEvents.grants.equals(List.of(secondFive)),
            "initial pending-TAR grants did not reach both saved targets");

         firstEvents.completeSaveInCallback = false;
         secondEvents.completeSaveInCallback = false;
         first.timeAdvanceRequest(firstNine);
         second.timeAdvanceRequest(secondNine);
         awaitBoth(first, second,
            () -> firstEvents.grants.size() == 2 && secondEvents.grants.size() == 2,
            "post-save grants on both routes");
         JavaTckSupport.check(firstEvents.grants.get(1).equals(firstNine)
               && secondEvents.grants.get(1).equals(secondNine)
               && firstNine.equals(first.queryLogicalTime())
               && secondNine.equals(second.queryLogicalTime()),
            "post-save time advances did not reach both later targets");

         firstEvents.order.clear();
         secondEvents.order.clear();
         first.requestFederationRestore(saveLabel);
         awaitBoth(first, second,
            () -> firstEvents.restoreLabels.size() == 1
               && secondEvents.restoreLabels.size() == 1,
            "multi-member pending-TAR restore initiation");
         JavaTckSupport.check(firstEvents.restoreSuccesses.equals(List.of(saveLabel))
               && firstEvents.restoreLabels.equals(List.of(saveLabel))
               && secondEvents.restoreLabels.equals(List.of(saveLabel)),
            "multi-member pending-TAR restore used an unexpected label");
         firstEvents.order.clear();
         secondEvents.order.clear();
         first.federateRestoreComplete();
         second.federateRestoreComplete();

         drain(first);
         JavaTckSupport.check(firstEvents.restoredCount == 1
               && firstEvents.grants.size() == 3
               && secondEvents.restoredCount == 0
               && secondEvents.grants.size() == 2
               && secondEvents.order.isEmpty(),
            "first restored pending TAR consumed or released the second route");
         drain(second);
         JavaTckSupport.check(secondEvents.restoredCount == 1
               && secondEvents.grants.size() == 3,
            "second restored pending TAR did not complete independently");
         for (int pass = 0; pass < 3; ++pass) { drain(first); drain(second); }
         JavaTckSupport.check(firstEvents.grants.get(2).equals(firstFive)
               && secondEvents.grants.get(2).equals(secondFive)
               && firstFive.equals(first.queryLogicalTime())
               && secondFive.equals(second.queryLogicalTime())
               && trailingRestoreGrant(firstEvents.order)
               && trailingRestoreGrant(secondEvents.order)
               && firstEvents.notRestoredCount == 0
               && secondEvents.notRestoredCount == 0,
            "restored pending-TAR grants did not follow Federation Restored independently");

         second.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(second);
         first.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(first);
         first.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { first.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static boolean trailingRestoreGrant(List<String> order) {
      int length = order.size();
      return length >= 2 && "restore-complete".equals(order.get(length - 2))
         && "grant".equals(order.get(length - 1));
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         first.evokeCallback(0.0);
         second.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void drain(RTIambassador member) throws Exception {
      while (member.evokeCallback(0.0)) { }
   }

   private static final class Recorder {
      private final RTIambassador member;
      private final CountDownLatch saved;
      private final CountDownLatch waitForOther;
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private volatile boolean completeSaveInCallback;
      private volatile int constrainedEnabled;
      private volatile int savedCount;
      private volatile int notSavedCount;
      private volatile int restoredCount;
      private volatile int notRestoredCount;

      private Recorder(RTIambassador member, CountDownLatch saved,
            CountDownLatch waitForOther) {
         this.member = member;
         this.saved = saved;
         this.waitForOther = waitForOther;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) args[0]);
                  order.add("save-initiate");
                  if (completeSaveInCallback) {
                     member.federateSaveBegun();
                     member.federateSaveComplete();
                     saved.countDown();
                     if (waitForOther != null && !waitForOther.await(10, TimeUnit.SECONDS))
                        throw new IllegalStateException("second save callback missed boundary");
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) args[0]);
                  order.add("grant");
               } else if ("federationSaved".equals(name)) ++savedCount;
               else if ("federationNotSaved".equals(name)) ++notSavedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  restoreSuccesses.add((String) args[0]);
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) {
                  ++restoredCount;
                  order.add("restore-complete");
               } else if ("federationNotRestored".equals(name)) ++notRestoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK multi-pending callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
