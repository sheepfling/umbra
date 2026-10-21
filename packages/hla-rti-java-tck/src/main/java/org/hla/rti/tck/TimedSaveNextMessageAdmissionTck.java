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

/** NMR admits at an equal save time; NMRA waits for a strictly later grant. */
final class TimedSaveNextMessageAdmissionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveNextMessageAdmissionTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "next-message timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-next-message-admission-" + UUID.randomUUID();
      String inclusiveLabel = "tck-next-message-save-inclusive";
      String exclusiveLabel = "tck-next-message-save-exclusive";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected.add(receiver);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-next-message-save-owner", federation);
         joined.add(owner);
         receiver.joinFederationExecution("java-tck-next-message-save-receiver", federation);
         joined.add(receiver);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "next-message members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> inclusiveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> ownerFirstTime = offset(initial, epsilon, 6);
         LogicalTime<?, ?> exclusiveTime = offset(initial, epsilon, 7);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 8);
         receiver.enableTimeConstrained();
         await(receiver, () -> receiverCallbacks.constrainedEnabled == 1,
            "next-message constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "next-message regulator enablement");

         owner.requestFederationSave(inclusiveLabel, inclusiveTime);
         receiver.nextMessageRequest(inclusiveTime);
         owner.timeAdvanceRequest(inclusiveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "inclusive NMR regulator grant");
         JavaTckSupport.check(inclusiveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.equals(List.of("grant")),
            "inclusive NMR regulator initiated save before recipient admission");
         await(receiver, () -> receiverCallbacks.labels.size() == 1
               && receiverCallbacks.grants.size() == 1,
            "inclusive NMR save initiation and grant");
         JavaTckSupport.check(receiverCallbacks.labels.equals(List.of(inclusiveLabel))
               && receiverCallbacks.saveTimes.size() == 1
               && inclusiveTime.equals(receiverCallbacks.saveTimes.get(0))
               && inclusiveTime.equals(receiverCallbacks.grants.get(0))
               && receiverCallbacks.order.equals(List.of("save-initiate", "grant"))
               && ownerCallbacks.order.equals(List.of("grant")),
            "inclusive NMR did not initiate save before equal grant 5");
         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "inclusive NMR regulator save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(inclusiveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && inclusiveTime.equals(ownerCallbacks.saveTimes.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "inclusive NMR regulator did not receive matching save initiation");

         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveBegun();
         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveComplete();
         awaitAll(List.of(owner, receiver),
            () -> ownerCallbacks.savedCount == 1 && receiverCallbacks.savedCount == 1,
            "inclusive NMR save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && receiverCallbacks.failures.isEmpty(),
            "inclusive NMR participant received Federation Not Saved");

         ownerCallbacks.order.clear();
         receiverCallbacks.order.clear();
         owner.requestFederationSave(exclusiveLabel, exclusiveTime);
         receiver.nextMessageRequestAvailable(exclusiveTime);
         owner.timeAdvanceRequest(ownerFirstTime);
         await(owner, () -> ownerCallbacks.grants.size() == 2,
            "exclusive NMRA regulator first grant");
         JavaTckSupport.check(ownerFirstTime.equals(ownerCallbacks.grants.get(1))
               && ownerCallbacks.labels.equals(List.of(inclusiveLabel))
               && ownerCallbacks.order.equals(List.of("grant")),
            "exclusive NMRA regulator initiated replacement save too early");
         await(receiver, () -> receiverCallbacks.grants.size() == 2,
            "exclusive NMRA equal-boundary grant");
         JavaTckSupport.check(exclusiveTime.equals(receiverCallbacks.grants.get(1))
               && receiverCallbacks.labels.equals(List.of(inclusiveLabel))
               && receiverCallbacks.order.equals(List.of("grant"))
               && ownerCallbacks.order.equals(List.of("grant")),
            "equal NMRA grant incorrectly initiated the replacement save");

         receiver.nextMessageRequestAvailable(laterTime);
         owner.timeAdvanceRequest(exclusiveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 3,
            "exclusive NMRA regulator second grant");
         JavaTckSupport.check(exclusiveTime.equals(ownerCallbacks.grants.get(2))
               && ownerCallbacks.labels.equals(List.of(inclusiveLabel))
               && ownerCallbacks.order.equals(List.of("grant", "grant")),
            "regulator received replacement save before qualifying NMRA admission");
         await(receiver, () -> receiverCallbacks.labels.size() == 2
               && receiverCallbacks.grants.size() == 3,
            "exclusive NMRA strict-later save initiation and grant");
         JavaTckSupport.check(receiverCallbacks.labels.equals(
                  List.of(inclusiveLabel, exclusiveLabel))
               && receiverCallbacks.saveTimes.size() == 2
               && exclusiveTime.equals(receiverCallbacks.saveTimes.get(1))
               && laterTime.equals(receiverCallbacks.grants.get(2))
               && receiverCallbacks.order.equals(List.of("grant", "save-initiate", "grant"))
               && ownerCallbacks.order.equals(List.of("grant", "grant")),
            "strict-later NMRA did not initiate replacement save before grant 8");
         await(owner, () -> ownerCallbacks.labels.size() == 2,
            "exclusive NMRA regulator save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(inclusiveLabel, exclusiveLabel))
               && ownerCallbacks.saveTimes.size() == 2
               && exclusiveTime.equals(ownerCallbacks.saveTimes.get(1))
               && ownerCallbacks.order.equals(List.of("grant", "grant", "save-initiate")),
            "regulator did not receive matching replacement save initiation");

         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveBegun();
         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveComplete();
         awaitAll(List.of(owner, receiver),
            () -> ownerCallbacks.savedCount == 2 && receiverCallbacks.savedCount == 2,
            "exclusive NMRA save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && receiverCallbacks.failures.isEmpty(),
            "next-message participant received Federation Not Saved");

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
                  if ("toString".equals(name)) return "Java TCK next-message callbacks";
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
