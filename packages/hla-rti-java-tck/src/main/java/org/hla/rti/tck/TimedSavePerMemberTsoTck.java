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

/** Queued timestamped interaction precedes one member's timed-save initiation. */
final class TimedSavePerMemberTsoTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimedSavePerMemberTsoTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName,
         CallbackModel callbackModel) throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "per-member TSO timed save requires evoked callbacks");
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador ready = factory.getRtiAmbassador();
      RTIambassador delayed = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder readyCallbacks = new Recorder();
      Recorder delayedCallbacks = new Recorder();
      List<RTIambassador> connected = new ArrayList<>();
      List<RTIambassador> joined = new ArrayList<>();
      String federation = "java-tck-timed-save-per-member-tso-" + UUID.randomUUID();
      String saveLabel = "tck-timed-save-per-member-tso";
      boolean created = false;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         connected.add(owner);
         ready.connect(readyCallbacks.proxy(), callbackModel);
         connected.add(ready);
         delayed.connect(delayedCallbacks.proxy(), callbackModel);
         connected.add(delayed);
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         owner.joinFederationExecution("java-tck-per-member-tso-owner", federation);
         joined.add(owner);
         ready.joinFederationExecution("java-tck-per-member-tso-ready", federation);
         joined.add(ready);
         delayed.joinFederationExecution("java-tck-per-member-tso-delayed", federation);
         joined.add(delayed);

         InteractionClassHandle ownerInteraction =
            owner.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle delayedInteraction =
            delayed.getInteractionClassHandle(interactionClassName);
         ParameterHandle ownerParameter = owner.getParameterHandle(ownerInteraction, parameterName);
         ParameterHandle delayedParameter =
            delayed.getParameterHandle(delayedInteraction, parameterName);
         owner.publishInteractionClass(ownerInteraction);
         owner.changeInteractionOrderType(ownerInteraction, OrderType.TIMESTAMP);
         delayed.subscribeInteractionClass(delayedInteraction);

         LogicalTimeFactory<?, ?> timeFactory = owner.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(ready.getTimeFactory().getName())
               && timeFactory.getName().equals(delayed.getTimeFactory().getName()),
            "per-member TSO timed save members selected different time factories");
         LogicalTime<?, ?> initial = owner.queryLogicalTime();
         LogicalTimeInterval<?> epsilon = timeFactory.makeEpsilon();
         LogicalTime<?, ?> saveTime = offset(initial, epsilon, 5);
         ready.enableTimeConstrained();
         await(ready, () -> readyCallbacks.constrainedEnabled == 1,
            "ready per-member TSO constrained-role enablement");
         delayed.enableTimeConstrained();
         await(delayed, () -> delayedCallbacks.constrainedEnabled == 1,
            "delayed per-member TSO constrained-role enablement");
         owner.enableTimeRegulation(epsilon);
         await(owner, () -> ownerCallbacks.regulationEnabled == 1,
            "per-member TSO regulator enablement");

         byte[] payload = new byte[] {0x50, 0x45, 0x52};
         byte[] tag = new byte[] {0x54, 0x53, 0x4F};
         ParameterHandleValueMap parameters = owner.getParameterHandleValueMapFactory().create(1);
         parameters.put(ownerParameter, payload);
         MessageRetractionReturn sent = owner.sendInteraction(
            ownerInteraction, parameters, tag, saveTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "per-member TSO timed save send returned no valid retraction handle");
         owner.requestFederationSave(saveLabel, saveTime);
         ready.timeAdvanceRequest(saveTime);
         delayed.timeAdvanceRequest(saveTime);
         owner.timeAdvanceRequest(saveTime);

         await(ready, () -> readyCallbacks.labels.size() == 1
               && readyCallbacks.grants.size() == 1,
            "ready per-member TSO save initiation and grant");
         JavaTckSupport.check(readyCallbacks.labels.equals(List.of(saveLabel))
               && readyCallbacks.saveTimes.size() == 1
               && saveTime.equals(readyCallbacks.saveTimes.get(0))
               && saveTime.equals(readyCallbacks.grants.get(0))
               && readyCallbacks.order.equals(List.of("save-initiate", "grant")),
            "ready member did not initiate save before grant 5");
         JavaTckSupport.check(delayedCallbacks.interactions.isEmpty()
               && delayedCallbacks.labels.isEmpty()
               && delayedCallbacks.order.isEmpty()
               && ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.isEmpty(),
            "ready member callback service consumed delayed or regulator callbacks");

         await(delayed, () -> delayedCallbacks.interactions.size() == 1
               && delayedCallbacks.labels.size() == 1
               && delayedCallbacks.grants.size() == 1,
            "delayed per-member TSO delivery and save initiation");
         TimedInteraction delivered = delayedCallbacks.interactions.get(0);
         JavaTckSupport.check(delivered.interaction.equals(delayedInteraction)
               && saveTime.equals(delivered.time)
               && delivered.parameters.size() == 1
               && Arrays.equals(delivered.parameters.get(delayedParameter), payload)
               && Arrays.equals(delivered.tag, tag)
               && delivered.receivedOrder == OrderType.TIMESTAMP,
            "delayed member did not receive the timestamped interaction at save time");
         JavaTckSupport.check(delayedCallbacks.labels.equals(List.of(saveLabel))
               && delayedCallbacks.saveTimes.size() == 1
               && saveTime.equals(delayedCallbacks.saveTimes.get(0))
               && saveTime.equals(delayedCallbacks.grants.get(0))
               && delayedCallbacks.order.equals(List.of("interaction", "save-initiate", "grant")),
            "delayed member did not receive TSO before save initiation and grant");
         JavaTckSupport.check(ownerCallbacks.labels.isEmpty()
               && ownerCallbacks.order.isEmpty(),
            "regulator received save callbacks before its own service");

         await(owner, () -> ownerCallbacks.labels.size() == 1
               && ownerCallbacks.grants.size() == 1,
            "per-member TSO regulator save initiation and grant");
         JavaTckSupport.check(ownerCallbacks.labels.equals(List.of(saveLabel))
               && ownerCallbacks.saveTimes.size() == 1
               && saveTime.equals(ownerCallbacks.saveTimes.get(0))
               && saveTime.equals(ownerCallbacks.grants.get(0))
               && ownerCallbacks.order.equals(List.of("grant", "save-initiate")),
            "regulator did not grant and then initiate the timed save");

         for (RTIambassador member : List.of(owner, ready, delayed)) member.federateSaveBegun();
         for (RTIambassador member : List.of(owner, ready, delayed)) member.federateSaveComplete();
         awaitAll(List.of(owner, ready, delayed),
            () -> ownerCallbacks.savedCount == 1
               && readyCallbacks.savedCount == 1
               && delayedCallbacks.savedCount == 1,
            "three-member per-member TSO save completion");
         for (Recorder recorder : List.of(ownerCallbacks, readyCallbacks, delayedCallbacks)) {
            JavaTckSupport.check(recorder.failures.isEmpty(),
               "per-member TSO participant received Federation Not Saved");
         }

         delayed.disableTimeConstrained();
         ready.disableTimeConstrained();
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
                  if ("toString".equals(name)) return "Java TCK per-member TSO callbacks";
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
