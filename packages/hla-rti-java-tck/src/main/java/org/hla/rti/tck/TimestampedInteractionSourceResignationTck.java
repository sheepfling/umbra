package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Delivers an accepted timestamped interaction after its producer resigns. */
final class TimestampedInteractionSourceResignationTck {
   private TimestampedInteractionSourceResignationTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-interaction-source-resignation-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean clockConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean clockJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      boolean clockRegulating = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel);
         clockConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producerHandle = publisher.joinFederationExecution(
            "java-tck-interaction-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-interaction-receiver", federation);
         receiverJoined = true;
         clock.joinFederationExecution("java-tck-interaction-source-resignation-clock", federation);
         clockJoined = true;

         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);
         publisher.publishInteractionClass(publisherInteraction);
         published = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverInteraction);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> clockTimeFactory = clock.getTimeFactory();
         String factoryName = publisherTimeFactory.getName();
         JavaTckSupport.check(factoryName.equals(receiverTimeFactory.getName())
               && factoryName.equals(clockTimeFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval publisherLookahead =
            intervalOffset(publisherTimeFactory.makeEpsilon(), 5);
         LogicalTimeInterval clockLookahead = intervalOffset(clockTimeFactory.makeEpsilon(), 5);

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         awaitCallback(receiver, () -> receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver Time Constrained enable");
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report Time Constrained enable");
         publisher.enableTimeRegulation(publisherLookahead);
         publisherRegulating = true;
         awaitCallback(publisher, () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher Time Regulation enable");
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report Time Regulation enable");
         clock.enableTimeRegulation(clockLookahead);
         clockRegulating = true;
         awaitCallback(clock, () -> clockRecorder.timeRegulationEnabledCount == 1,
            "clock Time Regulation enable");
         JavaTckSupport.check(clockRecorder.timeRegulationEnabledCount == 1,
            "clock did not report Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime())
               && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = offset(initial, publisherTimeFactory.makeEpsilon(), 6);
         LogicalTime receiverTarget = offset(initial, receiverTimeFactory.makeEpsilon(), 6);
         LogicalTime clockTarget = offset(initial, clockTimeFactory.makeEpsilon(), 2);
         byte[] payload = new byte[] {0x52, 0x45, 0x53, 0x49, 0x47};
         byte[] tag = new byte[] {0x54, 0x53, 0x4f, 0x2d, 0x52};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, payload);
         hla.rti1516_2025.MessageRetractionReturn retractionResult =
            publisher.sendInteraction(publisherInteraction, parameters, tag, messageTime);
         JavaTckSupport.check(retractionResult != null
               && retractionResult.retractionHandleIsValid
               && retractionResult.handle != null,
            "timestamped Send Interaction returned no valid retraction handle");
         MessageRetractionHandle retraction = retractionResult.handle;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "timestamped interaction arrived before the receiver frontier");

         receiver.timeAdvanceRequest(receiverTarget);
         JavaTckSupport.check(receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "receiver advanced past the message before the producer frontier");
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         expectException(() -> publisher.retract(retraction), "FederateNotExecutionMember",
            "retracting through the resigned producer");
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty()
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "queued interaction was released before the clock advanced"
               + " (interactions=" + receiverRecorder.interactions.size()
               + ", grants=" + receiverRecorder.timeAdvanceGrantTimes.size() + ")");

         clock.timeAdvanceRequest(clockTarget);
         awaitCallback(clock, () -> clockRecorder.timeAdvanceGrantTimes.size() == 1,
            "clock time advance grant");
         JavaTckSupport.check(clockRecorder.timeAdvanceGrantTimes.size() == 1,
            "clock did not reach its requested frontier");
         awaitCallback(receiver, () -> receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1,
            "post-resignation interaction delivery and grant");
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "timeAdvanceGrant")),
            "queued interaction was not delivered exactly once before the receiver grant");

         Interaction actual = receiverRecorder.interactions.get(0);
         JavaTckSupport.check(actual.interaction.equals(receiverInteraction)
               && actual.parameters.size() == 1
               && Arrays.equals(payload, actual.parameters.get(receiverParameter)),
            "interaction callback returned the wrong class or parameter");
         JavaTckSupport.check(Arrays.equals(tag, actual.tag)
               && actual.producer.equals(producerHandle)
               && messageTime.equals(actual.time),
            "interaction callback did not preserve tag, producer, or timestamp");
         JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
               && actual.receivedOrder == OrderType.TIMESTAMP
               && retraction.equals(actual.retraction),
            "interaction callback returned incorrect order or retraction metadata");
         JavaTckSupport.check(actual.regions == null || actual.regions.isEmpty(),
            "ordinary timestamped interaction unexpectedly carried source regions");
         JavaTckSupport.check(actual.transportation != null
               && !receiver.getTransportationTypeName(actual.transportation).isEmpty(),
            "interaction callback returned an unresolvable transportation handle");
         JavaTckSupport.check(receiverTarget.equals(
               receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "receiver was granted an unexpected logical time");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         JavaTckSupport.drain(receiver);
         clock.disableTimeRegulation();
         clockRegulating = false;
         JavaTckSupport.drain(clock);
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (clockRegulating && clockJoined) {
            try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeInteractionClass(
               receiver.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishInteractionClass(
               publisher.getInteractionClassHandle(interactionClassName)); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (clockJoined) {
            try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (clockConnected) {
            try { clock.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void awaitCallback(RTIambassador ambassador, BooleanSupplier observed,
         String phase) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
         if (observed.getAsBoolean()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(phase + " was not observed within 10 seconds");
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @FunctionalInterface
   private interface CheckedOperation { void run() throws Exception; }

   private static final class Interaction {
      private InteractionClassHandle interaction;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private RegionHandleSet regions;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabledCount;
      private volatile int timeConstrainedEnabledCount;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interaction = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.regions = (RegionHandleSet) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK interaction resignation callbacks";
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
