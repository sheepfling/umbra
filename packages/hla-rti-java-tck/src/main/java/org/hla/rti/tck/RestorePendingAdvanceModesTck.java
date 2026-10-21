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

/** Restore saved pending TARA, NMR, and NMRA requests under both callback models. */
final class RestorePendingAdvanceModesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private enum Mode { TARA, NMR, NMRA }

   private RestorePendingAdvanceModesTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED
            || callbackModel == CallbackModel.HLA_IMMEDIATE,
         "pending-mode restore requires a standard callback model");
      for (Mode mode : Mode.values()) {
         runMode(factory, fom, timeImplementation, callbackModel, mode);
      }
   }

   private static void runMode(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel, Mode mode) throws Exception {
      boolean immediate = callbackModel == CallbackModel.HLA_IMMEDIATE;
      RTIambassador member = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder(member);
      String suffix = mode.name().toLowerCase() + (immediate ? "-immediate" : "-evoked");
      String federation = "java-tck-restore-pending-" + suffix + "-" + UUID.randomUUID();
      String saveLabel = "tck-pending-" + suffix;
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("java-tck-pending-" + suffix, federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> savedTarget = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTarget = offset(initial, epsilon, 9);
         LogicalTime<?, ?> nextTarget = offset(initial, epsilon, 6);

         member.enableTimeConstrained();
         if (immediate) {
            JavaTckSupport.check(callbacks.constrainedEnabled == 1,
               "immediate constrained enablement was not delivered directly");
         } else {
            await(member, () -> callbacks.constrainedEnabled == 1,
               "pending-mode constrained enablement");
         }

         callbacks.order.clear();
         callbacks.completeSaveInCallback = true;
         member.requestFederationSave(saveLabel);
         JavaTckSupport.check(callbacks.saveLabels.isEmpty(),
            "untimed save initiated before the pending-mode advance request");
         submitAdvance(member, mode, savedTarget);
         if (!immediate) {
            await(member, () -> callbacks.saveLabels.size() == 1
                  && callbacks.timeGrants.size() == 1,
               "pending-mode save initiation and first grant");
            await(member, () -> callbacks.savedCount == 1,
               "pending-mode federation save completion");
         }
         JavaTckSupport.check(callbacks.saveLabels.equals(List.of(saveLabel))
               && callbacks.savedCount == 1 && callbacks.saveFailures.isEmpty()
               && callbacks.timeGrants.equals(List.of(savedTarget)),
            "pending-mode save did not capture the request before its first grant");
         if (!immediate) while (member.evokeCallback(0.0)) { }

         callbacks.completeSaveInCallback = false;
         submitAdvance(member, mode, laterTarget);
         if (!immediate) {
            await(member, () -> callbacks.timeGrants.size() == 2,
               "pending-mode post-save grant");
         }
         JavaTckSupport.check(callbacks.timeGrants.size() == 2
               && laterTarget.equals(callbacks.timeGrants.get(1))
               && laterTarget.equals(member.queryLogicalTime()),
            "pending-mode post-save advance did not reach time 9");

         callbacks.order.clear();
         member.requestFederationRestore(saveLabel);
         if (!immediate) {
            await(member, () -> callbacks.restoreLabels.size() == 1,
               "pending-mode restore initiation");
         }
         JavaTckSupport.check(callbacks.restoreSuccesses.equals(List.of(saveLabel))
               && callbacks.restoreBegunCount == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "pending-mode restore control callbacks did not arrive");
         member.federateRestoreComplete();
         if (!immediate) {
            JavaTckSupport.check(initial.equals(member.queryLogicalTime()),
               "evoked restore did not return to saved initial logical time");
            JavaTckSupport.expectFailure(() -> member.timeAdvanceRequest(nextTarget),
               "InTimeAdvancingState",
               "ordinary TAR while a restored pending-mode request is active");
            member.evokeCallback(0.0);
            JavaTckSupport.check(callbacks.restoredCount == 1
                  && callbacks.timeGrants.size() == 2,
               "restored pending-mode grant arrived before Federation Restored");
            member.evokeCallback(0.0);
         }
         JavaTckSupport.check(callbacks.restoredCount == 1
               && callbacks.restoreFailures.isEmpty()
               && callbacks.timeGrants.size() == 3
               && savedTarget.equals(callbacks.timeGrants.get(2))
               && callbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun",
                  "restore-initiate", "restore-complete", "grant"))
               && savedTarget.equals(member.queryLogicalTime()),
            "restored pending-mode grant did not follow Federation Restored");

         submitAdvance(member, mode, nextTarget);
         if (!immediate) {
            await(member, () -> callbacks.timeGrants.size() == 4,
               "pending-mode post-restore grant");
         }
         JavaTckSupport.check(callbacks.timeGrants.size() == 4
               && nextTarget.equals(callbacks.timeGrants.get(3)),
            "advance mode was not usable after restored grant");

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

   private static void submitAdvance(RTIambassador member,
         Mode mode, LogicalTime<?, ?> target) throws Exception {
      switch (mode) {
         case TARA: member.timeAdvanceRequestAvailable(target); break;
         case NMR: member.nextMessageRequest(target); break;
         case NMRA: member.nextMessageRequestAvailable(target); break;
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
                  if ("toString".equals(name)) return "Java TCK pending-mode restore callbacks";
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
