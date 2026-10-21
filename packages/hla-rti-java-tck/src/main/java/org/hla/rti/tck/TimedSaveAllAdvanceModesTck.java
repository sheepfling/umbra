package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Timed save admits TAR, NMR, TARA, NMRA, and FQR members independently. */
final class TimedSaveAllAdvanceModesTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveAllAdvanceModesTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "all-advance-mode timed save requires evoked callbacks");
      List<RTIambassador> members = List.of(factory.getRtiAmbassador(),
         factory.getRtiAmbassador(), factory.getRtiAmbassador(),
         factory.getRtiAmbassador(), factory.getRtiAmbassador(),
         factory.getRtiAmbassador());
      List<Recorder> callbacks = List.of(new Recorder(), new Recorder(),
         new Recorder(), new Recorder(), new Recorder(), new Recorder());
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      RTIambassador owner = members.get(0);
      String federation = "java-tck-timed-save-all-advance-modes-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-all-advance-modes";
      boolean created = false;
      try {
         for (int index = 0; index < members.size(); ++index) {
            RTIambassador member = members.get(index);
            member.connect(callbacks.get(index).proxy(), callbackModel);
            connected.add(member);
         }
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         String[] names = {"owner", "tar", "nmr", "tara", "nmra", "fqr"};
         for (int index = 0; index < members.size(); ++index) {
            RTIambassador member = members.get(index);
            member.joinFederationExecution(
               "java-tck-all-advance-modes-" + names[index], federation);
            joined.add(member);
         }

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         for (RTIambassador member : members) {
            JavaTckSupport.check(timeFactory.getName().equals(member.getTimeFactory().getName()),
               "all-advance-mode members selected different logical-time factories");
         }
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 6);

         for (int index = 1; index < members.size(); ++index) {
            RTIambassador member = members.get(index);
            Recorder recorder = callbacks.get(index);
            member.enableTimeConstrained();
            await(member, () -> recorder.constrainedEnabled == 1,
               "all-advance-mode constrained-role enablement");
         }
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> callbacks.get(0).regulationEnabled == 1,
            "all-advance-mode regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         members.get(1).timeAdvanceRequest(saveTime);
         members.get(2).nextMessageRequest(saveTime);
         members.get(3).timeAdvanceRequestAvailable(laterTime);
         members.get(4).nextMessageRequestAvailable(laterTime);
         members.get(5).flushQueueRequest(laterTime);
         owner.timeAdvanceRequest(saveTime);

         verifyMember(members.get(1), callbacks.get(1), saveLabel,
            saveTime, saveTime, false);
         requireUnserviced(callbacks, 2);
         verifyMember(members.get(2), callbacks.get(2), saveLabel,
            saveTime, saveTime, false);
         requireUnserviced(callbacks, 3);
         verifyMember(members.get(3), callbacks.get(3), saveLabel,
            saveTime, laterTime, false);
         requireUnserviced(callbacks, 4);
         verifyMember(members.get(4), callbacks.get(4), saveLabel,
            saveTime, laterTime, false);
         requireUnserviced(callbacks, 5);
         verifyMember(members.get(5), callbacks.get(5), saveLabel,
            saveTime, laterTime, true);
         requireUnserviced(callbacks, 6);

         Recorder ownerCallbacks = callbacks.get(0);
         await(owner, () -> ownerCallbacks.labels.size() == 1
               && ownerCallbacks.grants.size() == 1,
            "all-advance-mode regulator save initiation and grant");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && saveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "regulator did not grant and then initiate the matching timed save");

         for (RTIambassador member : members) member.federateSaveBegun();
         for (RTIambassador member : members) member.federateSaveComplete();
         awaitAll(members, () -> {
            for (Recorder recorder : callbacks) {
               if (recorder.savedCount != 1) return false;
            }
            return true;
         }, "six-member all-advance-mode save completion");
         for (Recorder recorder : callbacks) {
            JavaTckSupport.check(recorder.failures.isEmpty(),
               "all-advance-mode member received Federation Not Saved");
         }

         for (int index = 1; index < members.size(); ++index) {
            members.get(index).disableTimeConstrained();
         }
         owner.disableTimeRegulation();
         for (int index = joined.size() - 1; index >= 0; --index) {
            joined.get(index).resignFederationExecution(ResignAction.NO_ACTION);
            joined.remove(index);
         }
         owner.destroyFederationExecution(federation);
         created = false;
      } finally {
         for (int index = joined.size() - 1; index >= 0; --index) {
            try { joined.get(index).resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         for (int index = connected.size() - 1; index >= 0; --index) {
            try { connected.get(index).disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void verifyMember(RTIambassador member, Recorder callbacks,
         String saveLabel, LogicalTime<?, ?> saveTime,
         LogicalTime<?, ?> grantTime, boolean flushGrant) throws Exception {
      await(member, () -> callbacks.labels.size() == 1
            && (flushGrant ? callbacks.flushGrants.size() == 1
                           : callbacks.grants.size() == 1),
         "all-advance-mode save initiation and grant");
      JavaTckSupport.check(callbacks.labels.equals(List.of(saveLabel))
            && callbacks.saveTimes.size() == 1
            && saveTime.equals(callbacks.saveTimes.get(0)),
         "constrained member did not receive timed save initiation");
      JavaTckSupport.check(grantTime.equals(flushGrant
               ? callbacks.flushGrants.get(0) : callbacks.grants.get(0))
            && callbacks.order.equals(flushGrant
               ? List.of("save-initiate", "flush-grant")
               : List.of("save-initiate", "grant")),
         "constrained member did not initiate save before its grant");
   }

   private static void requireUnserviced(List<Recorder> callbacks, int first) {
      for (int index = first; index < callbacks.size(); ++index) {
         JavaTckSupport.check(callbacks.get(index).labels.isEmpty()
               && callbacks.get(index).order.isEmpty(),
            "later constrained member received a callback before its own service");
      }
      JavaTckSupport.check(callbacks.get(0).labels.isEmpty()
            && callbacks.get(0).order.isEmpty(),
         "regulator received a callback before its own service");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
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

   private static void awaitAll(List<RTIambassador> members,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         for (RTIambassador member : members) member.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Recorder {
      private final List<String> labels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<SaveFailureReason> failures = new CopyOnWriteArrayList<>();
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int savedCount;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) ++regulationEnabled;
               else if ("timeConstrainedEnabled".equals(name)) ++constrainedEnabled;
               else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("flushQueueGrant".equals(name)) {
                  flushGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("flush-grant");
               } else if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  if (arguments.length == 2) {
                     saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  }
                  order.add("save-initiate");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK all-advance-mode callbacks";
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
