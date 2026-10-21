package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.SaveFailureReason;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** An equal actual FQR grant leaves a timed save pending until a later grant. */
final class TimedSaveFlushQueueAdmissionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSaveFlushQueueAdmissionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "Flush Queue timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-flush-queue-admission-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-flush-queue-admission";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         connected.add(receiver);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-flush-queue-save-owner", federation);
         joined.add(owner);
         receiver.joinFederationExecution("java-tck-flush-queue-save-receiver", federation);
         joined.add(receiver);

         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         owner.publishInteractionClass(ownerInteraction);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "Flush Queue timed save members selected different time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> ownerFirstTime = offset(initial, epsilon, 4);
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 6);
         receiver.enableTimeConstrained();
         await(receiver, () -> receiverCallbacks.constrainedEnabled == 1,
            "Flush Queue constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "Flush Queue regulator enablement");

         byte[] payload = new byte[] {(byte) 0xF0, 0x51};
         byte[] tag = new byte[] {0x46, 0x51, 0x52};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, payload);
         MessageRetractionReturn sent = owner.sendInteraction(
            ownerInteraction, parameters, tag, saveTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "Flush Queue timed save send returned no valid retraction handle");
         owner.requestFederationSave(saveLabel, saveTime);
         owner.timeAdvanceRequest(ownerFirstTime);
         receiver.flushQueueRequest(saveTime);

         await(receiver, () -> receiverCallbacks.interactions.size() == 1
               && receiverCallbacks.flushGrants.size() == 1,
            "Flush Queue equal-boundary interaction and grant");
         TimedInteraction delivered = receiverCallbacks.interactions.get(0);
         JavaTckSupport.check(delivered.interaction.equals(receiverInteraction)
               && saveTime.equals(delivered.time)
               && delivered.parameters.size() == 1
               && Arrays.equals(delivered.parameters.get(receiverParameter), payload)
               && Arrays.equals(delivered.tag, tag)
               && delivered.receivedOrder == OrderType.TIMESTAMP,
            "Flush Queue recipient did not receive timestamped interaction at save time");
         JavaTckSupport.check(receiverCallbacks.labels.isEmpty()
               && saveTime.equals(receiverCallbacks.flushGrants.get(0))
               && receiverCallbacks.order.equals(List.of("interaction", "flush-grant"))
               && ownerCallbacks.order.isEmpty(),
            "equal actual Flush Queue grant incorrectly initiated the save");

         await(owner, () -> ownerCallbacks.grants.size() == 1,
            "Flush Queue regulator first grant");
         JavaTckSupport.check(ownerFirstTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.order.equals(List.of("grant")),
            "regulator did not receive its preliminary time-4 grant");
         owner.timeAdvanceRequest(saveTime);
         receiver.flushQueueRequest(laterTime);

         await(receiver, () -> receiverCallbacks.labels.size() == 1
               && receiverCallbacks.flushGrants.size() == 2,
            "Flush Queue strict-later save initiation and grant");
         JavaTckSupport.check(receiverCallbacks.labels.equals(List.of(saveLabel))
               && receiverCallbacks.saveTimes.size() == 1
               && saveTime.equals(receiverCallbacks.saveTimes.get(0))
               && laterTime.equals(receiverCallbacks.flushGrants.get(1))
               && receiverCallbacks.order.equals(List.of(
                  "interaction", "flush-grant", "save-initiate", "flush-grant"))
               && ownerCallbacks.order.equals(List.of("grant")),
            "strict-later actual Flush Queue grant did not initiate save first");

         await(owner, () -> ownerCallbacks.labels.size() == 1,
            "Flush Queue regulator save initiation");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0)),
            "regulator did not receive the matching timed save initiation");

         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveBegun();
         for (RTIambassador member : List.of(owner, receiver)) member.federateSaveComplete();
         awaitAll(List.of(owner, receiver),
            () -> ownerCallbacks.savedCount == 1 && receiverCallbacks.savedCount == 1,
            "two-member Flush Queue save completion");
         JavaTckSupport.check(ownerCallbacks.failures.isEmpty()
               && receiverCallbacks.failures.isEmpty(),
            "Flush Queue participant received Federation Not Saved");

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

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;
      private final OrderType receivedOrder;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
         receivedOrder = (OrderType) arguments[8];
      }
   }

   private static final class Recorder {
      private final List<String> labels = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> saveTimes = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> flushGrants = new CopyOnWriteArrayList<>();
      private final List<String> order = new CopyOnWriteArrayList<>();
      private final List<TimedInteraction> interactions = new CopyOnWriteArrayList<>();
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
               } else if ("receiveInteraction".equals(name) && arguments.length == 10) {
                  interactions.add(new TimedInteraction(arguments));
                  order.add("interaction");
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
                  if ("toString".equals(name)) return "Java TCK Flush Queue save callbacks";
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
