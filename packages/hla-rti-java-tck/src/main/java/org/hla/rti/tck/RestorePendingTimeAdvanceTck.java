package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RestoreFailureReason;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Restore a saved, pending ordinary time advance under evoked callbacks. */
final class RestorePendingTimeAdvanceTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private RestorePendingTimeAdvanceTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "pending-TAR restore requires evoked callbacks");
      RTIambassador member = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder(member);
      String federation = "java-tck-restore-pending-tar-" + UUID.randomUUID();
      String saveLabel = "tck-pending-time-advance-baseline";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-pending-tar-member", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> savedTarget = offset(initial, epsilon, 5);
         LogicalTime<?, ?> invalidTarget = offset(initial, epsilon, 6);
         LogicalTime<?, ?> laterTarget = offset(initial, epsilon, 9);

         member.enableTimeConstrained();
         await(member, () -> callbacks.constrainedEnabled == 1,
            "pending-TAR constrained-role enablement");

         callbacks.order.clear();
         callbacks.completeSaveInCallback = true;
         member.requestFederationSave(saveLabel);
         JavaTckSupport.check(callbacks.saveLabels.isEmpty(),
            "evoked save callback arrived before callback servicing");
         member.timeAdvanceRequest(savedTarget);
         await(member, () -> callbacks.saveLabels.size() == 1
               && callbacks.timeGrants.size() == 1,
            "pending-TAR save initiation and first grant");
         JavaTckSupport.check(callbacks.saveLabels.equals(List.of(saveLabel))
               && savedTarget.equals(callbacks.timeGrants.get(0))
               && callbacks.order.equals(List.of("save-initiate", "grant")),
            "pending TAR did not initiate save before the first grant");
         await(member, () -> callbacks.savedCount == 1,
            "pending-TAR federation save completion");
         JavaTckSupport.check(callbacks.saveFailures.isEmpty(),
            "pending-TAR federation save failed");
         while (member.evokeCallback(0.0)) { }

         callbacks.completeSaveInCallback = false;
         member.timeAdvanceRequest(laterTarget);
         await(member, () -> callbacks.timeGrants.size() == 2,
            "post-save TAR grant");
         JavaTckSupport.check(laterTarget.equals(callbacks.timeGrants.get(1))
               && laterTarget.equals(member.queryLogicalTime()),
            "post-save TAR did not reach the later logical time");

         callbacks.order.clear();
         member.requestFederationRestore(saveLabel);
         await(member, () -> callbacks.restoreLabels.size() == 1,
            "pending-TAR restore initiation");
         JavaTckSupport.check(callbacks.restoreSuccesses.equals(List.of(saveLabel))
               && callbacks.restoreBegunCount == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "pending-TAR restore did not begin from the saved label");
         member.federateRestoreComplete();
         JavaTckSupport.check(initial.equals(member.queryLogicalTime()),
            "pending-TAR restore did not return to initial logical time");
         JavaTckSupport.expectFailure(() -> member.timeAdvanceRequest(invalidTarget),
            "InTimeAdvancingState", "ordinary TAR while restored TAR is pending");

         member.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.restoredCount == 1
               && callbacks.timeGrants.size() == 2,
            "restored TAR grant arrived before Federation Restored");
         member.evokeCallback(0.0);
         JavaTckSupport.check(callbacks.timeGrants.size() == 3
               && savedTarget.equals(callbacks.timeGrants.get(2))
               && callbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun",
                  "restore-initiate", "restore-complete", "grant"))
               && savedTarget.equals(member.queryLogicalTime()),
            "restored pending TAR did not grant its saved target after Federation Restored");
         JavaTckSupport.check(callbacks.restoreFailures.isEmpty(),
            "pending-TAR federation restore failed");

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

   static void runImmediate(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_IMMEDIATE,
         "immediate pending-TAR restore requires immediate callbacks");
      RTIambassador member = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder(member);
      String federation = "java-tck-restore-pending-tar-immediate-" + UUID.randomUUID();
      String saveLabel = "tck-pending-time-advance-immediate";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-pending-tar-immediate", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> savedTarget = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTarget = offset(initial, epsilon, 9);
         LogicalTime<?, ?> nextTarget = offset(initial, epsilon, 6);

         member.enableTimeConstrained();
         JavaTckSupport.check(callbacks.constrainedEnabled == 1,
            "immediate time-constrained enablement was not delivered directly");

         callbacks.order.clear();
         callbacks.completeSaveInCallback = true;
         callbacks.recordSavedInOrder = true;
         member.requestFederationSave(saveLabel);
         member.timeAdvanceRequest(savedTarget);
         JavaTckSupport.check(callbacks.saveLabels.equals(List.of(saveLabel))
               && callbacks.savedCount == 1 && callbacks.saveFailures.isEmpty()
               && callbacks.timeGrants.equals(List.of(savedTarget))
               && callbacks.order.equals(List.of(
                  "save-initiate", "save-complete", "grant")),
            "immediate save did not complete re-entrantly before its first grant");

         callbacks.completeSaveInCallback = false;
         member.timeAdvanceRequest(laterTarget);
         JavaTckSupport.check(callbacks.timeGrants.equals(
               List.of(savedTarget, laterTarget))
               && laterTarget.equals(member.queryLogicalTime()),
            "immediate post-save TAR did not reach the later time");

         callbacks.order.clear();
         member.requestFederationRestore(saveLabel);
         JavaTckSupport.check(callbacks.restoreSuccesses.equals(List.of(saveLabel))
               && callbacks.restoreBegunCount == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "immediate restore control callbacks were not delivered directly");
         member.federateRestoreComplete();
         JavaTckSupport.check(callbacks.restoredCount == 1
               && callbacks.restoreFailures.isEmpty()
               && callbacks.timeGrants.size() == 3
               && savedTarget.equals(callbacks.timeGrants.get(2))
               && callbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun",
                  "restore-initiate", "restore-complete", "grant"))
               && savedTarget.equals(member.queryLogicalTime()),
            "immediate restore completion did not precede the reconstructed grant");

         member.timeAdvanceRequest(nextTarget);
         JavaTckSupport.check(callbacks.timeGrants.size() == 4
               && nextTarget.equals(callbacks.timeGrants.get(3))
               && nextTarget.equals(member.queryLogicalTime()),
            "immediate restored grant did not release Time Advancing state");

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
      private final List<LogicalTime<?, ?>> timeGrants = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> saveFailures = new CopyOnWriteArrayList<>();
      private final List<RestoreFailureReason> restoreFailures =
         new CopyOnWriteArrayList<>();
      private volatile boolean completeSaveInCallback;
      private volatile boolean recordSavedInOrder;
      private volatile int constrainedEnabled;
      private volatile int savedCount;
      private volatile int restoreBegunCount;
      private volatile int restoredCount;

      private Recorder(RTIambassador ambassador) { this.ambassador = ambassador; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) arguments[0]);
                  order.add("save-initiate");
                  if (completeSaveInCallback) {
                     ambassador.federateSaveBegun();
                     ambassador.federateSaveComplete();
                  }
               } else if ("timeAdvanceGrant".equals(name)) {
                  timeGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  if (recordSavedInOrder) order.add("save-complete");
               }
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
                  if ("toString".equals(name)) return "Java TCK pending-TAR restore callbacks";
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
