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

/** Each available-mode member is admitted before its own grant; the regulator waits. */
final class TimedSaveAvailableNextMessageMemberAdmissionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveAvailableNextMessageMemberAdmissionTck() { }

   static void run(RtiFactory factory, String fom,
         String timeImplementation, CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "available/next-message timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador available = factory.getRtiAmbassador();
      RTIambassador nextMessage = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder availableCallbacks = new Recorder();
      Recorder nextMessageCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-available-next-message-" + UUID.randomUUID();
      String saveLabel = "tck-available-next-message-save";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         available.connect(availableCallbacks.proxy(), callbackModel);
         connected.add(available);
         nextMessage.connect(nextMessageCallbacks.proxy(), callbackModel);
         connected.add(nextMessage);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-available-next-message-owner", federation);
         joined.add(owner);
         available.joinFederationExecution("java-tck-available-next-message-available", federation);
         joined.add(available);
         nextMessage.joinFederationExecution("java-tck-available-next-message-next-message", federation);
         joined.add(nextMessage);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(available.getTimeFactory().getName())
               && timeFactory.getName().equals(nextMessage.getTimeFactory().getName()),
            "available/next-message members selected different logical-time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 6);

         available.enableTimeConstrained();
         await(available, () -> availableCallbacks.constrainedEnabled == 1,
            "available constrained-role enablement");
         nextMessage.enableTimeConstrained();
         await(nextMessage, () -> nextMessageCallbacks.constrainedEnabled == 1,
            "next-message constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "available/next-message regulator enablement");

         owner.requestFederationSave(saveLabel, saveTime);
         available.timeAdvanceRequestAvailable(laterTime);
         nextMessage.nextMessageRequestAvailable(laterTime);
         owner.timeAdvanceRequest(saveTime);
         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "available/next-message regulator grant");
         JavaTckSupport.check(saveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.equals(List.of("grant")),
            "regulator initiated save before constrained-member admission");

         await(available, () -> availableCallbacks.labels.size() == 1
               && availableCallbacks.grants.size() == 1,
            "available member save initiation and grant");
         JavaTckSupport.check(availableCallbacks.labels.equals(List.of(saveLabel))
               && availableCallbacks.saveTimes.size() == 1
               && saveTime.equals(availableCallbacks.saveTimes.get(0))
               && laterTime.equals(availableCallbacks.grants.get(0))
               && availableCallbacks.order.equals(List.of("save-initiate", "grant"))
               && nextMessageCallbacks.labels.isEmpty()
               && nextMessageCallbacks.order.isEmpty()
               && ownerCallbacks.labels.isEmpty(),
            "available member was not admitted independently before its grant");

         await(nextMessage, () -> nextMessageCallbacks.labels.size() == 1
               && nextMessageCallbacks.grants.size() == 1,
            "next-message member save initiation and grant");
         JavaTckSupport.check(nextMessageCallbacks.labels.equals(List.of(saveLabel))
               && nextMessageCallbacks.saveTimes.size() == 1
               && saveTime.equals(nextMessageCallbacks.saveTimes.get(0))
               && laterTime.equals(nextMessageCallbacks.grants.get(0))
               && nextMessageCallbacks.order.equals(List.of("save-initiate", "grant"))
               && ownerCallbacks.labels.isEmpty(),
            "next-message member was not admitted independently before its grant");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "available/next-message regulator save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "regulator did not wait for both constrained members");

         for (RTIambassador member : List.of(owner, available, nextMessage)) {
            member.federateSaveBegun();
         }
         for (RTIambassador member : List.of(owner, available, nextMessage)) {
            member.federateSaveComplete();
         }
         awaitAll(List.of(owner, available, nextMessage),
            () -> ownerCallbacks.savedCount == 1 && availableCallbacks.savedCount == 1
                  && nextMessageCallbacks.savedCount == 1,
            "available/next-message save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && availableCallbacks.failures.isEmpty()
               && nextMessageCallbacks.failures.isEmpty(),
            "available/next-message participant received Federation Not Saved");

         available.disableTimeConstrained();
         nextMessage.disableTimeConstrained();
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
               } else if ("federationNotSaved".equals(name)) {
                  failures.add((SaveFailureReason) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK available/next-message callbacks";
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
