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

/** Timed save reaches an ordinary TAR boundary before FQR callback service. */
final class TimedSaveOrdinaryBeforeFqrTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveOrdinaryBeforeFqrTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "ordinary-before-FQR timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador ordinary = factory.getRtiAmbassador();
      RTIambassador flushQueue = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder ordinaryCallbacks = new Recorder();
      Recorder flushCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-ordinary-before-fqr-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-ordinary-before-fqr";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         ordinary.connect(ordinaryCallbacks.proxy(), callbackModel);
         connected.add(ordinary);
         flushQueue.connect(flushCallbacks.proxy(), callbackModel);
         connected.add(flushQueue);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-ordinary-before-fqr-owner", federation);
         joined.add(owner);
         ordinary.joinFederationExecution("java-tck-ordinary-before-fqr-tar", federation);
         joined.add(ordinary);
         flushQueue.joinFederationExecution("java-tck-ordinary-before-fqr-fqr", federation);
         joined.add(flushQueue);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(ordinary.getTimeFactory().getName())
               && timeFactory.getName().equals(flushQueue.getTimeFactory().getName()),
            "ordinary-before-FQR members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> flushTime = offset(initial, epsilon, 6);

         ordinary.enableTimeConstrained();
         await(ordinary, () -> ordinaryCallbacks.constrainedEnabled == 1,
            "ordinary member constrained-role enablement");
         flushQueue.enableTimeConstrained();
         await(flushQueue, () -> flushCallbacks.constrainedEnabled == 1,
            "FQR member constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "ordinary-before-FQR regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         flushQueue.flushQueueRequest(flushTime);
         ordinary.timeAdvanceRequest(saveTime);
         owner.timeAdvanceRequest(saveTime);

         await(ordinary, () -> ordinaryCallbacks.labels.size() == 1
               && ordinaryCallbacks.grants.size() == 1,
            "ordinary boundary save initiation and grant");
         JavaTckSupport.check(ordinaryCallbacks.labels.equals(List.of(saveLabel))
               && ordinaryCallbacks.saveTimes.size() == 1
               && saveTime.equals(ordinaryCallbacks.saveTimes.get(0))
               && saveTime.equals(ordinaryCallbacks.grants.get(0))
               && ordinaryCallbacks.order.equals(List.of("save-initiate", "grant")),
            "ordinary TAR member did not initiate save before grant 5");
         JavaTckSupport.check(flushCallbacks.labels.isEmpty()
               && flushCallbacks.order.isEmpty()
               && ownerCallbacks.labels.isEmpty(),
            "FQR or owner received save initiation without callback service");

         await(flushQueue, () -> flushCallbacks.labels.size() == 1
               && flushCallbacks.flushGrants.size() == 1,
            "FQR save initiation and grant after ordinary boundary");
         JavaTckSupport.check(flushCallbacks.labels.equals(List.of(saveLabel))
               && flushCallbacks.saveTimes.size() == 1
               && saveTime.equals(flushCallbacks.saveTimes.get(0))
               && flushTime.equals(flushCallbacks.flushGrants.get(0))
               && flushCallbacks.order.equals(List.of("save-initiate", "flush-grant"))
               && ordinaryCallbacks.order.equals(List.of("save-initiate", "grant"))
               && ownerCallbacks.labels.isEmpty(),
            "FQR member did not initiate save before actual grant 6");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "owner save initiation after ordinary and FQR members");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0)),
            "owner did not receive matching timed save initiation");

         for (RTIambassador member : List.of(owner, ordinary, flushQueue)) {
            member.federateSaveBegun();
         }
         for (RTIambassador member : List.of(owner, ordinary, flushQueue)) {
            member.federateSaveComplete();
         }
         awaitAll(List.of(owner, ordinary, flushQueue),
            () -> ownerCallbacks.savedCount == 1
               && ordinaryCallbacks.savedCount == 1
               && flushCallbacks.savedCount == 1,
            "three-member ordinary-before-FQR save completion");
         for (Recorder recorder : List.of(ownerCallbacks, ordinaryCallbacks, flushCallbacks)) {
            JavaTckSupport.check(recorder.failures.isEmpty(),
               "ordinary-before-FQR member received Federation Not Saved");
         }

         flushQueue.disableTimeConstrained();
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
                  if ("toString".equals(name)) return "Java TCK ordinary-before-FQR callbacks";
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
