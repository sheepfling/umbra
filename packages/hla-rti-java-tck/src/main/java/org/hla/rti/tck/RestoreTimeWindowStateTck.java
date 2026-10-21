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

/** Restore the saved logical time and actual lookahead after post-save mutations. */
final class RestoreTimeWindowStateTck {
   private RestoreTimeWindowStateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "saved time-window restore has native evidence only for evoked callbacks");
      RTIambassador member = factory.getRtiAmbassador();
      Recorder events = new Recorder();
      String federation = "java-tck-restore-time-window-" + UUID.randomUUID();
      String saveLabel = "tck-time-window-baseline";
      boolean connected = false;
      boolean created = false;
      boolean joined = false;
      try {
         member.connect(events.proxy(), callbackModel);
         connected = true;
         member.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         member.joinFederationExecution("time-window-member", federation);
         joined = true;

         LogicalTimeFactory<?, ?> timeFactory = member.getTimeFactory();
         LogicalTime<?, ?> initial = member.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> time3 = offset(initial, epsilon, 3);
         LogicalTime<?, ?> time5 = offset(initial, epsilon, 5);
         LogicalTimeInterval<?> lookahead2 = interval(epsilon, 2);
         LogicalTimeInterval<?> lookahead5 = interval(epsilon, 5);
         member.enableTimeRegulation(lookahead2);
         await(member, () -> events.regulationEnabled == 1,
            "time-window regulation enablement");
         JavaTckSupport.check(initial.equals(member.queryLogicalTime())
               && lookahead2.equals(member.queryLookahead()),
            "initial regulated time window did not match zero and lookahead two");

         member.timeAdvanceRequest(time3);
         await(member, () -> events.grants == 1, "time-window grant at time three");
         JavaTckSupport.check(time3.equals(member.queryLogicalTime()),
            "logical time did not reach the pre-save time three");

         member.requestFederationSave(saveLabel);
         await(member, () -> events.saveLabels.equals(List.of(saveLabel)),
            "time-window save initiation");
         member.federateSaveBegun();
         member.federateSaveComplete();
         await(member, () -> events.savedCount == 1, "time-window federation saved");
         JavaTckSupport.check(events.notSavedCount == 0,
            "time-window federation save failed");

         member.modifyLookahead(lookahead5);
         JavaTckSupport.check(lookahead5.equals(member.queryLookahead()),
            "post-save lookahead did not change to five");
         member.timeAdvanceRequest(time5);
         await(member, () -> events.grants == 2,
            "post-save time-window grant at time five");
         JavaTckSupport.check(time5.equals(member.queryLogicalTime()),
            "post-save logical time did not advance to five");

         member.requestFederationRestore(saveLabel);
         await(member, () -> events.restoreRequestSucceeded == 1
               && events.restoreLabels.equals(List.of(saveLabel)),
            "time-window restore initiation");
         member.federateRestoreComplete();
         await(member, () -> events.restoredCount == 1,
            "time-window federation restored");
         JavaTckSupport.check(events.notRestoredCount == 0
               && time3.equals(member.queryLogicalTime())
               && lookahead2.equals(member.queryLookahead()),
            "restore did not reinstate saved logical time and actual lookahead");

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

   private static void await(RTIambassador member, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline)
         member.evokeCallback(0.0);
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTimeInterval interval(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int index = 1; index < steps; ++index)
         result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static final class Recorder {
      private final List<String> saveLabels = new CopyOnWriteArrayList<>();
      private final List<String> restoreLabels = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int grants;
      private volatile int savedCount;
      private volatile int notSavedCount;
      private volatile int restoreRequestSucceeded;
      private volatile int restoredCount;
      private volatile int notRestoredCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, args) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("timeAdvanceGrant".equals(name)) ++grants;
               else if ("initiateFederateSave".equals(name)) saveLabels.add((String) args[0]);
               else if ("federationSaved".equals(name)) ++savedCount;
               else if ("federationNotSaved".equals(name)) ++notSavedCount;
               else if ("requestFederationRestoreSucceeded".equals(name))
                  ++restoreRequestSucceeded;
               else if ("initiateFederateRestore".equals(name))
                  restoreLabels.add((String) args[0]);
               else if ("federationRestored".equals(name)) ++restoredCount;
               else if ("federationNotRestored".equals(name)) ++notRestoredCount;
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK time-window callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == args[0];
               }
               return null;
            });
      }
   }
}
