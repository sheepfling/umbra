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

/** Restore a pending FQR with direct callbacks, then issue another FQR. */
final class RestorePendingFlushQueueImmediateTck {
   private RestorePendingFlushQueueImmediateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_IMMEDIATE,
         "pending-FQR immediate restore has native evidence only for immediate callbacks");
      RTIambassador member = factory.getRtiAmbassador();
      Recorder callbacks = new Recorder(member);
      String federation = "java-tck-restore-pending-fqr-immediate-" + UUID.randomUUID();
      String saveLabel = "tck-pending-flush-queue-immediate";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(callbacks.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("pending-fqr-immediate-member", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> flushTime = offset(initial, epsilon, 6);
         LogicalTime<?, ?> nextTime = offset(initial, epsilon, 7);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 9);

         member.enableTimeConstrained();
         JavaTckSupport.check(callbacks.constrainedEnabled == 1,
            "immediate constrained enablement was not delivered directly");
         member.enableTimeRegulation(epsilon);
         JavaTckSupport.check(callbacks.regulationEnabled == 1,
            "immediate regulation enablement was not delivered directly");

         callbacks.order.clear();
         callbacks.completeSaveInCallback = true;
         member.requestFederationSave(saveLabel, saveTime);
         member.flushQueueRequest(flushTime);
         JavaTckSupport.check(callbacks.saveLabels.equals(List.of(saveLabel))
               && callbacks.saveTimes.equals(List.of(saveTime))
               && callbacks.savedCount == 1 && callbacks.notSavedCount == 0
               && callbacks.flushGrants.equals(List.of(flushTime))
               && callbacks.order.equals(List.of(
                  "save-initiate", "save-complete", "flush-grant")),
            "immediate save did not complete re-entrantly before the first FQG");

         callbacks.completeSaveInCallback = false;
         member.timeAdvanceRequest(laterTime);
         JavaTckSupport.check(callbacks.timeGrants.equals(List.of(laterTime))
               && laterTime.equals(member.queryLogicalTime()),
            "post-save ordinary TAR did not reach the later time");

         callbacks.order.clear();
         member.requestFederationRestore(saveLabel);
         JavaTckSupport.check(callbacks.restoreSuccesses.equals(List.of(saveLabel))
               && callbacks.restoreBegunCount == 1
               && callbacks.restoreLabels.equals(List.of(saveLabel)),
            "immediate FQR restore control callbacks were not delivered directly");
         member.federateRestoreComplete();
         JavaTckSupport.check(callbacks.restoredCount == 1
               && callbacks.notRestoredCount == 0
               && callbacks.flushGrants.equals(List.of(flushTime, flushTime))
               && callbacks.order.equals(List.of(
                  "restore-request-succeeded", "restore-begun", "restore-initiate",
                  "restore-complete", "flush-grant"))
               && flushTime.equals(member.queryLogicalTime()),
            "immediate Federation Restored did not precede the reconstructed FQG");

         member.flushQueueRequest(nextTime);
         JavaTckSupport.check(callbacks.flushGrants.equals(
               List.of(flushTime, flushTime, nextTime)),
            "restored FQG did not release Time Advancing for the next FQR");

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
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static final class Recorder {
      private final RTIambassador member;
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeGrants = new CopyOnWriteArrayList<>();
      private final List<String> restoreSuccesses = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private volatile boolean completeSaveInCallback;
      private volatile int constrainedEnabled;
      private volatile int regulationEnabled;
      private volatile int savedCount;
      private volatile int notSavedCount;
      private volatile int restoreBegunCount;
      private volatile int restoredCount;
      private volatile int notRestoredCount;

      private Recorder(RTIambassador member) { this.member = member; }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("initiateFederateSave".equals(name)) {
                  saveLabels.add((String) args[0]);
                  if (args.length == 2) saveTimes.add((LogicalTime<?, ?>) args[1]);
                  order.add("save-initiate");
                  if (completeSaveInCallback) {
                     member.federateSaveBegun();
                     member.federateSaveComplete();
                  }
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) args[0]);
                  order.add("flush-grant");
               } else if ("timeAdvanceGrant".equals(name))
                  timeGrants.add((LogicalTime<?, ?>) args[0]);
               else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               } else if ("federationNotSaved".equals(name)) ++notSavedCount;
               else if ("requestFederationRestoreSucceeded".equals(name)) {
                  restoreSuccesses.add((String) args[0]);
                  order.add("restore-request-succeeded");
               } else if ("federationRestoreBegun".equals(name)) {
                  ++restoreBegunCount;
                  order.add("restore-begun");
               } else if ("initiateFederateRestore".equals(name)) {
                  restoreLabels.add((String) args[0]);
                  order.add("restore-initiate");
               } else if ("federationRestored".equals(name)) {
                  ++restoredCount;
                  order.add("restore-complete");
               } else if ("federationNotRestored".equals(name)) ++notRestoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK immediate pending-FQR callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
