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
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.BooleanSupplier;

/** Two restored pending time-role callbacks must return to their own federates. */
final class RestoreMixedPendingTimeRoleCallbacksTck {
   private RestoreMixedPendingTimeRoleCallbacksTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model) throws Exception {
      RTIambassador regulator = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder regulatorEvents = new Recorder("regulator");
      Recorder constrainedEvents = new Recorder("constrained");
      String federation = "java-tck-restore-mixed-pending-roles-" + UUID.randomUUID();
      String saveLabel = "tck-mixed-pending-roles-"
         + (model == CallbackModel.HLA_EVOKED ? "evoked-" : "immediate-")
         + UUID.randomUUID();
      boolean regulatorConnected = false, constrainedConnected = false;
      boolean regulatorJoined = false, constrainedJoined = false, created = false;
      boolean regulatorDisabled = false, constrainedDisabled = false;
      try {
         regulator.connect(regulatorEvents.proxy(), model);
         regulatorConnected = true;
         constrained.connect(constrainedEvents.proxy(), model);
         constrainedConnected = true;
         regulator.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         regulator.joinFederationExecution("pending-role-regulator", "time-managed",
            federation);
         regulatorJoined = true;
         constrained.joinFederationExecution("pending-role-constrained", "time-managed",
            federation);
         constrainedJoined = true;

         LogicalTimeFactory<?, ?> timeFactory = regulator.getTimeFactory();
         LogicalTime<?, ?> regulatorInitial = regulator.queryLogicalTime();
         LogicalTime<?, ?> constrainedInitial = constrained.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         if (model == CallbackModel.HLA_IMMEDIATE) {
            regulator.disableCallbacks();
            regulatorDisabled = true;
            constrained.disableCallbacks();
            constrainedDisabled = true;
         }
         regulator.enableTimeRegulation(doubleInterval(epsilon));
         constrained.enableTimeConstrained();
         JavaTckSupport.check(regulatorEvents.regulationTimes.isEmpty()
               && constrainedEvents.constrainedTimes.isEmpty(),
            "pending time-role callbacks crossed the save boundary");

         regulator.requestFederationSave(saveLabel);
         regulator.federateSaveBegun();
         regulator.federateSaveComplete();
         constrained.federateSaveBegun();
         constrained.federateSaveComplete();
         JavaTckSupport.check(regulatorEvents.savedCount.get() == 0
               && constrainedEvents.savedCount.get() == 0,
            "save callbacks arrived before the restore boundary");

         regulator.requestFederationRestore(saveLabel);
         regulator.federateRestoreComplete();
         constrained.federateRestoreComplete();

         if (regulatorDisabled) {
            regulator.enableCallbacks();
            regulatorDisabled = false;
         } else {
            drain(regulator);
         }
         await(regulator, model, () -> regulatorEvents.regulationTimes.size() == 1
               && regulatorEvents.restoredCount.get() == 1,
            "regulator's restored role and completion callbacks");
         JavaTckSupport.check(constrainedEvents.constrainedTimes.isEmpty()
               && constrainedEvents.savedCount.get() == 0
               && constrainedEvents.restoredCount.get() == 0,
            "the constrained federate received callbacks while still gated");

         if (constrainedDisabled) {
            constrained.enableCallbacks();
            constrainedDisabled = false;
         } else {
            drain(constrained);
         }
         awaitBoth(regulator, constrained, model,
            () -> regulatorEvents.savedCount.get() == 1
               && constrainedEvents.savedCount.get() == 1
               && regulatorEvents.restoredCount.get() == 1
               && constrainedEvents.restoredCount.get() == 1
               && constrainedEvents.constrainedTimes.size() == 1,
            "both federates' saved, restored, and time-role callbacks");
         JavaTckSupport.check(regulatorEvents.regulationTimes.equals(List.of(regulatorInitial))
               && regulatorEvents.constrainedTimes.isEmpty()
               && constrainedEvents.constrainedTimes.equals(List.of(constrainedInitial))
               && constrainedEvents.regulationTimes.isEmpty()
               && regulatorEvents.savedCount.get() == 1
               && constrainedEvents.savedCount.get() == 1
               && regulatorEvents.restoredCount.get() == 1
               && constrainedEvents.restoredCount.get() == 1,
            "restore lost, duplicated, or cross-routed a pending time-role callback");

         constrained.resignFederationExecution(ResignAction.NO_ACTION);
         constrainedJoined = false;
         regulator.resignFederationExecution(ResignAction.NO_ACTION);
         regulatorJoined = false;
         regulator.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (regulatorDisabled) try { regulator.enableCallbacks(); }
            catch (Exception ignored) { }
         if (constrainedDisabled) try { constrained.enableCallbacks(); }
            catch (Exception ignored) { }
         if (constrainedJoined) try {
            constrained.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (regulatorJoined) try {
            regulator.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (created) try { regulator.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (constrainedConnected) try { constrained.disconnect(); }
            catch (Exception ignored) { }
         if (regulatorConnected) try { regulator.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval doubleInterval(LogicalTimeInterval epsilon)
         throws Exception {
      return (LogicalTimeInterval) epsilon.add(epsilon);
   }

   private static void drain(RTIambassador member) throws Exception {
      while (member.evokeCallback(0.0)) { }
   }

   private static void await(RTIambassador member, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) member.evokeCallback(0.0);
         else Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.0);
            second.evokeCallback(0.0);
         } else Thread.sleep(1L);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      final String name;
      final List<LogicalTime<?, ?>> regulationTimes = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> constrainedTimes = new CopyOnWriteArrayList<>();
      final AtomicInteger savedCount = new AtomicInteger();
      final AtomicInteger restoredCount = new AtomicInteger();

      Recorder(String name) { this.name = name; }

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String methodName = method.getName();
               if ("timeRegulationEnabled".equals(methodName))
                  regulationTimes.add((LogicalTime<?, ?>) args[0]);
               else if ("timeConstrainedEnabled".equals(methodName))
                  constrainedTimes.add((LogicalTime<?, ?>) args[0]);
               else if ("federationSaved".equals(methodName)) savedCount.incrementAndGet();
               else if ("federationRestored".equals(methodName)) restoredCount.incrementAndGet();
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(methodName)) return "Java TCK " + name;
                  if ("hashCode".equals(methodName)) return System.identityHashCode(proxy);
                  if ("equals".equals(methodName)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
