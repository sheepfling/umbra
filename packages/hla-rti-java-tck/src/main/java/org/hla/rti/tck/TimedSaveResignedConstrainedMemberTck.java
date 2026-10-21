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

/** A resigned constrained member does not gate a pending timed save. */
final class TimedSaveResignedConstrainedMemberTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveResignedConstrainedMemberTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, callbackModel, false);
   }

   static void runPendingFqr(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      runInternal(factory, fom, timeImplementation, callbackModel, true);
   }

   private static void runInternal(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel,
         boolean pendingFqr) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "resigned constrained-member timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador ordinary = factory.getRtiAmbassador();
      RTIambassador departing = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder ordinaryCallbacks = new Recorder();
      Recorder departingCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-resigned-constrained-" + UUID.randomUUID();
      String saveLabel = "tck-resigned-constrained-save";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         ordinary.connect(ordinaryCallbacks.proxy(), callbackModel);
         connected.add(ordinary);
         departing.connect(departingCallbacks.proxy(), callbackModel);
         connected.add(departing);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-resigned-constrained-owner", federation);
         joined.add(owner);
         ordinary.joinFederationExecution("java-tck-resigned-constrained-ordinary", federation);
         joined.add(ordinary);
         departing.joinFederationExecution("java-tck-resigned-constrained-departing", federation);
         joined.add(departing);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(ordinary.getTimeFactory().getName())
               && timeFactory.getName().equals(departing.getTimeFactory().getName()),
            "timed-save members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> fqrTime = offset(initial, epsilon, 6);

         ordinary.enableTimeConstrained();
         await(ordinary, () -> ordinaryCallbacks.constrainedEnabled == 1,
            "remaining member constrained-role enablement");
         departing.enableTimeConstrained();
         await(departing, () -> departingCallbacks.constrainedEnabled == 1,
            "departing member constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         if (pendingFqr) departing.flushQueueRequest(fqrTime);
         departing.resignFederationExecution(ResignAction.NO_ACTION);
         joined.remove(departing);
         ordinary.timeAdvanceRequest(saveTime);
         owner.timeAdvanceRequest(saveTime);
         await(ordinary, () -> ordinaryCallbacks.labels.size() == 1
               && ordinaryCallbacks.grants.size() == 1,
            "remaining member save initiation and grant");
         JavaTckSupport.check(ordinaryCallbacks.labels.equals(List.of(saveLabel))
               && ordinaryCallbacks.saveTimes.size() == 1
               && saveTime.equals(ordinaryCallbacks.saveTimes.get(0))
               && saveTime.equals(ordinaryCallbacks.grants.get(0))
               && ordinaryCallbacks.order.equals(List.of("save-initiate", "grant"))
               && ownerCallbacks.labels.isEmpty()
               && departingCallbacks.labels.isEmpty()
               && departingCallbacks.fqrGrants.isEmpty()
               && departingCallbacks.order.isEmpty(),
            "resigned member still gated or received the pending save");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "regulator save initiation after member resignation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && departingCallbacks.labels.isEmpty(),
            "regulator did not receive the pending save after member resignation");

         owner.federateSaveBegun();
         ordinary.federateSaveBegun();
         owner.federateSaveComplete();
         ordinary.federateSaveComplete();
         awaitAll(List.of(owner, ordinary),
            () -> ownerCallbacks.savedCount == 1 && ordinaryCallbacks.savedCount == 1,
            "surviving-member save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && ordinaryCallbacks.failures.isEmpty()
               && departingCallbacks.savedCount == 0
               && departingCallbacks.failures.isEmpty(),
            "departed member received a save result or surviving save failed");

         ordinary.disableTimeConstrained();
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
      private final List<LogicalTime<?, ?>> fqrGrants = new CopyOnWriteArrayList<>();
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
               else if ("flushQueueGrant".equals(name)) {
                  fqrGrants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("fqr-grant");
               }
               else if ("timeAdvanceGrant".equals(name)) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  order.add("grant");
               } else if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  if (arguments.length == 2) saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  order.add("save-initiate");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK resigned constrained-member callbacks";
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
