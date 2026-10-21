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
import java.util.function.BooleanSupplier;

/** Restore a pending time-regulation callback through the standard Java API. */
final class RestorePendingTimeRegulationCallbackTck {
   private RestorePendingTimeRegulationCallbackTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model) throws Exception {
      runRole(factory, fom, timeImplementation, model, false);
   }

   static void runConstrained(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model) throws Exception {
      runRole(factory, fom, timeImplementation, model, true);
   }

   private static void runRole(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel model, boolean constrained) throws Exception {
      RTIambassador member = factory.getRtiAmbassador();
      Recorder events = new Recorder();
      String federation = "java-tck-restore-pending-"
         + (constrained ? "time-constrained-" : "time-regulation-") + UUID.randomUUID();
      String saveLabel = "tck-pending-" + (constrained ? "constrained-" : "regulation-")
         + (model == CallbackModel.HLA_EVOKED ? "evoked" : "immediate");
      boolean connected = false, created = false, joined = false, callbacksDisabled = false;
      try {
         member.connect(events.proxy(), model);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("pending-time-regulation-member", "time-managed",
            federation);
         joined = true;
         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();

         if (model == CallbackModel.HLA_IMMEDIATE) {
            member.disableCallbacks();
            callbacksDisabled = true;
         }
         if (constrained) member.enableTimeConstrained();
         else member.enableTimeRegulation(doubleInterval(epsilon));
         List<LogicalTime<?, ?>> roleTimes = constrained
            ? events.constrainedTimes : events.regulationTimes;
         JavaTckSupport.check(roleTimes.isEmpty(),
            "pending time-role callback was not gated");
         member.requestFederationSave(saveLabel);
         member.federateSaveBegun();
         member.federateSaveComplete();
         JavaTckSupport.check(events.savedCount == 0,
            "saved callback arrived before the restore boundary");
         member.requestFederationRestore(saveLabel);
         member.federateRestoreComplete();
         if (callbacksDisabled) {
            member.enableCallbacks();
            callbacksDisabled = false;
         }
         await(member, model, () -> !roleTimes.isEmpty()
               && events.savedCount >= 1 && events.restoredCount >= 1,
            "rebound time-role and restore callbacks");
         JavaTckSupport.check(roleTimes.equals(List.of(initial))
               && events.savedCount == 1 && events.restoredCount == 1,
            "restore duplicated or lost the pending time-role callback");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         member.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (callbacksDisabled) try { member.enableCallbacks(); }
            catch (Exception ignored) { }
         if (joined) try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (created) try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (connected) try { member.disconnect(); } catch (Exception ignored) { }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval doubleInterval(LogicalTimeInterval epsilon)
         throws Exception {
      return (LogicalTimeInterval) epsilon.add(epsilon);
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

   private static final class Recorder {
      final List<LogicalTime<?, ?>> regulationTimes = new CopyOnWriteArrayList<>();
      final List<LogicalTime<?, ?>> constrainedTimes = new CopyOnWriteArrayList<>();
      volatile int savedCount, restoredCount;

      FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class}, (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name))
                  regulationTimes.add((LogicalTime<?, ?>) args[0]);
               else if ("timeConstrainedEnabled".equals(name))
                  constrainedTimes.add((LogicalTime<?, ?>) args[0]);
               else if ("federationSaved".equals(name)) savedCount++;
               else if ("federationRestored".equals(name)) restoredCount++;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK pending time-role callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
