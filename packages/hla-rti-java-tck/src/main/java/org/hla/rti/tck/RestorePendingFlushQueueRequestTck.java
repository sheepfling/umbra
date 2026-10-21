package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restores a saved pending FQR and discards post-save ordinary advance work. */
final class RestorePendingFlushQueueRequestTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestorePendingFlushQueueRequestTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "pending-FQR restore requires evoked callbacks");
      RTIambassador member = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder(member);
      String federation = "java-tck-restore-pending-fqr-" + UUID.randomUUID();
      String saveLabel = "tck-pending-flush-queue";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-pending-fqr-member", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> flushTime = offset(initial, epsilon, 6);
         LogicalTime<?, ?> invalidTime = offset(initial, epsilon, 7);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 9);

         member.enableTimeConstrained();
         await(member, () -> callbacks.constrainedEnabled == 1,
            "pending-FQR constrained-role enablement");
         member.enableTimeRegulation(epsilon);
         await(member, () -> callbacks.regulationEnabled == 1,
            "pending-FQR regulation enablement");

         callbacks.order.clear();
         callbacks.completeSaveInCallback = true;
         member.requestFederationSave(saveLabel, saveTime);
         member.flushQueueRequest(flushTime);
         await(member, () -> callbacks.saveLabels.size() == 1
               && callbacks.flushGrants.size() == 1,
            "pre-save pending FQR initiation and grant");
         JavaTckSupport.check(callbacks.saveLabels.equals(List.of(saveLabel))
               && callbacks.saveTimes.size() == 1
               && saveTime.equals(callbacks.saveTimes.get(0))
               && flushTime.equals(callbacks.flushGrants.get(0))
               && callbacks.order.size() >= 2
               && callbacks.order.subList(0, 2).equals(List.of("save-initiate", "flush-grant")),
            "pending FQR did not initiate save before its first grant");
         await(member, () -> callbacks.savedCount == 1,
            "pending-FQR federation save completion");
         JavaTckSupport.check(callbacks.saveFailures.isEmpty(),
            "pending-FQR federation save failed");
         while (member.evokeCallback(0.0)) { }

         callbacks.completeSaveInCallback = false;
         member.timeAdvanceRequest(laterTime);
         await(member, () -> callbacks.timeGrants.size() == 1,
            "post-save TAR grant");
         JavaTckSupport.check(laterTime.equals(callbacks.timeGrants.get(0))
               && laterTime.equals(member.queryLogicalTime()),
            "post-save TAR did not reach time 9");

         callbacks.order.clear();
         member.requestFederationRestore(saveLabel);
         await(member, () -> callbacks.restoreLabels.size() == 1,
            "pending-FQR restore initiation");
         JavaTckSupport.check(callbacks.restoreSuccesses.equals(List.of(saveLabel))
               && callbacks.restoreBegunCount == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "pending-FQR restore did not begin from the saved label");
         member.federateRestoreComplete();
         JavaTckSupport.check(initial.equals(member.queryLogicalTime()),
            "pending-FQR restore did not return to initial logical time");
         JavaTckSupport.expectFailure(() -> member.timeAdvanceRequest(invalidTime),
            "InTimeAdvancingState", "ordinary TAR while restored FQR is pending");

         member.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.restoredCount == 1
               && callbacks.flushGrants.size() == 1,
            "restored FQR grant arrived before Federation Restored");
         member.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.flushGrants.size() == 2
               && flushTime.equals(callbacks.flushGrants.get(1))
               && callbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun", "restore-initiate",
                  "restore-complete", "flush-grant"))
               && flushTime.equals(member.queryLogicalTime()),
            "restored pending FQR did not grant time 6 after Federation Restored");
         JavaTckSupport.check(callbacks.restoreFailures.isEmpty(),
            "pending-FQR federation restore failed");

         member.resignFederationExecution(ResignAction.NO_ACTION);
         joined = false;
         member.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (joined) {
            try { member.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { member.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (connected) {
            try { member.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final RTIambassador ambassador;
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeGrants = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures = new CopyOnWriteArrayList<>();
      private volatile boolean completeSaveInCallback;
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int restoreBegunCount;
      private volatile int restoredCount;

      private Recorder(RTIambassador ambassador) {
         this.ambassador = ambassador;
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) arguments[0]);
                  if (arguments.length == 2) saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  order.add("save-initiate");
                  if (completeSaveInCallback) {
                     ambassador.federateSaveBegun();
                     ambassador.federateSaveComplete();
                  }
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("flush-grant");
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("federationSaved".equals(name)) ++savedCount;
               else if ("federationNotSaved".equals(name)) {
                  saveFailures.add((SaveFailureReason) arguments[0]);
               } else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) arguments[0]);
                  order.add("restore-request-succeeded");
               } else if ("federationRestoreBegun".equals(name)) {
                  ++restoreBegunCount;
                  order.add("restore-begun");
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreLabels.add((String) arguments[0]);
                  order.add("restore-initiate");
               } else if ("federationRestored".equals(name)) {
                  ++restoredCount;
                  order.add("restore-complete");
               } else if ("federationNotRestored".equals(name)) {
                  restoreFailures.add((RestoreFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK pending-FQR restore callbacks";
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
