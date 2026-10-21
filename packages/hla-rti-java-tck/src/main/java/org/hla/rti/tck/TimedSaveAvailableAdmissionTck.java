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

/** An equal TARA grant leaves a timed save pending until a later grant. */
final class TimedSaveAvailableAdmissionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveAvailableAdmissionTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "available-advance timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-available-admission-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-available-admission";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected.add(receiver);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-available-save-owner", federation);
         joined.add(owner);
         receiver.joinFederationExecution("java-tck-available-save-receiver", federation);
         joined.add(receiver);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "available-advance members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> firstOwnerTime = offset(initial, epsilon, 4);
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 6);
         receiver.enableTimeConstrained();
         await(receiver, () -> receiverCallbacks.constrainedEnabled == 1,
            "available-advance constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "available-advance regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         receiver.timeAdvanceRequestAvailable(saveTime);
         owner.timeAdvanceRequest(firstOwnerTime);
         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "available-advance regulator first grant");
         JavaTckSupport.check(firstOwnerTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.equals(List.of("grant")),
            "regulator's first grant incorrectly initiated the timed save");
         await(receiver, () -> receiverCallbacks.grants.size() == 1,
            "available-advance equal-boundary grant");
         JavaTckSupport.check(saveTime.equals(receiverCallbacks.grants.get(0))
               && receiverCallbacks.labels.isEmpty()
               && receiverCallbacks.order.equals(List.of("grant"))
               && ownerCallbacks.order.equals(List.of("grant")),
            "equal available-advance grant incorrectly initiated the save");

         receiver.timeAdvanceRequestAvailable(laterTime);
         owner.timeAdvanceRequest(saveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 2,
            "available-advance regulator second grant");
         JavaTckSupport.check(saveTime.equals(ownerCallbacks.grants.get(1))
               && ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.equals(List.of("grant", "grant")),
            "regulator received save initiation before qualifying recipient admission");
         await(receiver, () -> receiverCallbacks.labels.size() == 1
               && receiverCallbacks.grants.size() == 2,
            "available-advance strict-later save initiation and grant");
         JavaTckSupport.check(receiverCallbacks.labels.equals(List.of(saveLabel))
               && receiverCallbacks.saveTimes.size() == 1
               && saveTime.equals(receiverCallbacks.saveTimes.get(0))
               && laterTime.equals(receiverCallbacks.grants.get(1))
               && receiverCallbacks.order.equals(List.of("grant", "save-initiate", "grant"))
               && ownerCallbacks.order.equals(List.of("grant", "grant")),
            "strict-later available-advance grant did not initiate save first");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "available-advance regulator save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "grant", "save-initiate")),
            "regulator did not receive the matching timed save initiation");

         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveBegun();
         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveComplete();
         awaitAll(List.of(owner, receiver),
            () -> ownerCallbacks.savedCount == 1 && receiverCallbacks.savedCount == 1,
            "two-member available-advance save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && receiverCallbacks.failures.isEmpty(),
            "available-advance participant received Federation Not Saved");

         receiver.disableTimeConstrained();
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
               } else if ("initiateFederateSave".equals(name)) {
                  labels.add((String) arguments[0]);
                  if (arguments.length == 2) saveTimes.add((LogicalTime<?, ?>) arguments[1]);
                  order.add("save-initiate");
               } else if ("federationSaved".equals(name)) {
                  ++savedCount;
                  order.add("save-complete");
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK available-advance callbacks";
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
