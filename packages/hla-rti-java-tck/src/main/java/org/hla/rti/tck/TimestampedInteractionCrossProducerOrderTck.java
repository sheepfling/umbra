package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Portable timestamp ordering and producer identity across two senders. */
final class TimestampedInteractionCrossProducerOrderTck {
   private TimestampedInteractionCrossProducerOrderTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador early = factory.getRtiAmbassador();
      RTIambassador late = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      RTIambassador receiverPeer = factory.getRtiAmbassador();
      Recorder earlyRecorder = new Recorder();
      Recorder lateRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      Recorder receiverPeerRecorder = new Recorder();
      String federation = "java-tck-timestamped-interaction-cross-producer-" + UUID.randomUUID();
      boolean earlyConnected = false;
      boolean lateConnected = false;
      boolean receiverConnected = false;
      boolean receiverPeerConnected = false;
      boolean created = false;
      boolean earlyJoined = false;
      boolean lateJoined = false;
      boolean receiverJoined = false;
      boolean receiverPeerJoined = false;
      try {
         early.connect(earlyRecorder.proxy(), callbackModel);
         earlyConnected = true;
         late.connect(lateRecorder.proxy(), callbackModel);
         lateConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         receiverPeer.connect(receiverPeerRecorder.proxy(), callbackModel);
         receiverPeerConnected = true;

         early.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle earlyProducer = early.joinFederationExecution(
            "java-tck-cross-producer-early", federation);
         earlyJoined = true;
         FederateHandle lateProducer = late.joinFederationExecution(
            "java-tck-cross-producer-late", federation);
         lateJoined = true;
         receiver.joinFederationExecution("java-tck-cross-producer-receiver", federation);
         receiverJoined = true;
         receiverPeer.joinFederationExecution("java-tck-cross-producer-peer", federation);
         receiverPeerJoined = true;

         InteractionClassHandle earlyClass = early.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle lateClass = late.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverClass =
            receiver.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverPeerClass =
            receiverPeer.getInteractionClassHandle(interactionClassName);
         ParameterHandle earlyParameter = early.getParameterHandle(earlyClass, parameterName);
         ParameterHandle lateParameter = late.getParameterHandle(lateClass, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverClass, parameterName);
         ParameterHandle receiverPeerParameter = receiverPeer.getParameterHandle(
            receiverPeerClass, parameterName);
         early.publishInteractionClass(earlyClass);
         late.publishInteractionClass(lateClass);
         early.changeInteractionOrderType(earlyClass, OrderType.TIMESTAMP);
         late.changeInteractionOrderType(lateClass, OrderType.TIMESTAMP);
         receiver.subscribeInteractionClass(receiverClass);
         receiverPeer.subscribeInteractionClass(receiverPeerClass);
         LogicalTimeFactory<?, ?> earlyFactory = early.getTimeFactory();
         LogicalTimeFactory<?, ?> lateFactory = late.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverPeerFactory = receiverPeer.getTimeFactory();
         String timeFactoryName = earlyFactory.getName();
         JavaTckSupport.check(timeFactoryName.equals(lateFactory.getName())
               && timeFactoryName.equals(receiverFactory.getName())
               && timeFactoryName.equals(receiverPeerFactory.getName()),
            "cross-producer members selected different logical-time factories");

         receiver.enableTimeConstrained();
         JavaTckSupport.drain(receiver);
         receiverPeer.enableTimeConstrained();
         JavaTckSupport.drain(receiverPeer);
         LogicalTimeInterval earlyEpsilon = (LogicalTimeInterval) earlyFactory.makeEpsilon();
         LogicalTimeInterval lateEpsilon = (LogicalTimeInterval) lateFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTimeInterval receiverPeerEpsilon =
            (LogicalTimeInterval) receiverPeerFactory.makeEpsilon();
         early.enableTimeRegulation(earlyEpsilon);
         JavaTckSupport.drain(early);
         late.enableTimeRegulation(lateEpsilon);
         JavaTckSupport.drain(late);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && receiverPeerRecorder.timeConstrainedEnabled
               && earlyRecorder.timeRegulationEnabled && lateRecorder.timeRegulationEnabled,
            "cross-producer time-role callbacks were not delivered");

         LogicalTime initial = early.queryLogicalTime();
         LogicalTime lateInitial = late.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime receiverPeerInitial = receiverPeer.queryLogicalTime();
         JavaTckSupport.check(initial.equals(lateInitial)
               && initial.equals(receiverInitial) && initial.equals(receiverPeerInitial),
            "cross-producer members began at different logical times");
         LogicalTime timeFive = offset(initial, earlyEpsilon, 5);
         LogicalTime timeSeven = offset(initial, earlyEpsilon, 7);
         LogicalTime receiverFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime receiverSeven = offset(receiverInitial, receiverEpsilon, 7);
         LogicalTime receiverPeerFive = offset(receiverPeerInitial, receiverPeerEpsilon, 5);
         LogicalTime receiverPeerSeven = offset(receiverPeerInitial, receiverPeerEpsilon, 7);
         JavaTckSupport.check(timeFive.equals(offset(lateInitial, lateEpsilon, 5))
               && timeSeven.equals(offset(lateInitial, lateEpsilon, 7))
               && timeFive.equals(receiverFive) && timeSeven.equals(receiverSeven)
               && timeFive.equals(receiverPeerFive) && timeSeven.equals(receiverPeerSeven),
            "cross-producer members did not produce matching logical timestamps");

         byte[] payload = new byte[] {0x53, 0x54, 0x4f};
         byte[] earlyTag = new byte[] {0x45, 0x41};
         byte[] lateTag = new byte[] {0x4c, 0x41};
         byte[] lateEqualTag = new byte[] {0x4c, 0x45};
         MessageRetractionReturn lateMessage = send(
            late, lateClass, lateParameter, payload, lateTag, timeSeven);
         MessageRetractionReturn earlyMessage = send(
            early, earlyClass, earlyParameter, payload, earlyTag, timeFive);
         MessageRetractionReturn lateEqualMessage = send(
            late, lateClass, lateParameter, payload, lateEqualTag, timeFive);
         JavaTckSupport.check(valid(lateMessage) && valid(earlyMessage) && valid(lateEqualMessage),
            "cross-producer timestamped send returned an invalid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(receiverPeer);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty()
               && receiverPeerRecorder.interactions.isEmpty(),
            "cross-producer interaction arrived before the receiver requested an advance");

         early.timeAdvanceRequest(timeSeven);
         late.timeAdvanceRequest(timeSeven);
         receiver.timeAdvanceRequest(receiverFive);
         receiverPeer.timeAdvanceRequest(receiverPeerFive);
         drainAll(early, late, receiver, receiverPeer);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "receiveInteraction", "timeAdvanceGrant")),
            "first receiver did not receive both time-five interactions before its grant");
         JavaTckSupport.check(receiverPeerRecorder.interactions.size() == 2
               && receiverPeerRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverPeerRecorder.callbackOrder.equals(
                  Arrays.asList("receiveInteraction", "receiveInteraction", "timeAdvanceGrant")),
            "peer receiver did not receive both time-five interactions before its grant");

         receiver.timeAdvanceRequest(receiverSeven);
         receiverPeer.timeAdvanceRequest(receiverPeerSeven);
         drainAll(early, late, receiver, receiverPeer);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 3
               && receiverRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "receiveInteraction", "timeAdvanceGrant",
                  "receiveInteraction", "timeAdvanceGrant")),
            "first receiver did not deliver the time-seven interaction before its grant");
         JavaTckSupport.check(receiverPeerRecorder.interactions.size() == 3
               && receiverPeerRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverPeerRecorder.callbackOrder.equals(Arrays.asList(
                  "receiveInteraction", "receiveInteraction", "timeAdvanceGrant",
                  "receiveInteraction", "timeAdvanceGrant")),
            "peer receiver did not deliver the time-seven interaction before its grant");

         assertOrder(receiverRecorder, receiverClass, receiverParameter, earlyProducer,
            lateProducer, payload, earlyTag, lateEqualTag, lateTag, timeFive, timeSeven,
            receiver, "first receiver");
         assertOrder(receiverPeerRecorder, receiverPeerClass, receiverPeerParameter,
            earlyProducer, lateProducer, payload, earlyTag, lateEqualTag, lateTag,
            timeFive, timeSeven, receiverPeer, "peer receiver");
         JavaTckSupport.check(receiverSeven.equals(receiverRecorder.timeAdvanceGrantTimes.get(1))
               && receiverPeerSeven.equals(receiverPeerRecorder.timeAdvanceGrantTimes.get(1)),
            "cross-producer receivers received the wrong final grant time");

         receiver.disableTimeConstrained();
         JavaTckSupport.drain(receiver);
         receiverPeer.disableTimeConstrained();
         JavaTckSupport.drain(receiverPeer);
         early.disableTimeRegulation();
         JavaTckSupport.drain(early);
         late.disableTimeRegulation();
         JavaTckSupport.drain(late);
         receiver.unsubscribeInteractionClass(receiverClass);
         receiverPeer.unsubscribeInteractionClass(receiverPeerClass);
         early.unpublishInteractionClass(earlyClass);
         late.unpublishInteractionClass(lateClass);
      } finally {
         if (receiverPeerJoined) {
            try { receiverPeer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (lateJoined) {
            try { late.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (earlyJoined) {
            try { early.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { early.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverPeerConnected) {
            try { receiverPeer.disconnect(); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (lateConnected) {
            try { late.disconnect(); } catch (Exception ignored) { }
         }
         if (earlyConnected) {
            try { early.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn send(RTIambassador ambassador,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] payload,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      ParameterHandleValueMap values = ambassador.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload);
      return ambassador.sendInteraction(interaction, values, tag, time);
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void drainAll(RTIambassador early, RTIambassador late,
         RTIambassador receiver, RTIambassador receiverPeer) throws Exception {
      for (RTIambassador ambassador : Arrays.asList(early, late, receiver, receiverPeer)) {
         JavaTckSupport.drain(ambassador);
      }
   }

   private static void assertOrder(Recorder recorder, InteractionClassHandle expectedClass,
         ParameterHandle expectedParameter, FederateHandle earlyProducer,
         FederateHandle lateProducer, byte[] payload, byte[] earlyTag, byte[] lateEqualTag,
         byte[] lateTag, LogicalTime<?, ?> timeFive, LogicalTime<?, ?> timeSeven,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(recorder.interactions.size() == 3,
         description + " received the wrong number of timestamped interactions");
      Interaction first = recorder.interactions.get(0);
      Interaction second = recorder.interactions.get(1);
      Interaction third = recorder.interactions.get(2);
      JavaTckSupport.check(timeFive.equals(first.time) && timeFive.equals(second.time)
            && timeSeven.equals(third.time),
         description + " did not deliver interactions in increasing timestamp order");
      boolean earlyFirst = earlyProducer.equals(first.producer)
         && Arrays.equals(earlyTag, first.tag)
         && lateProducer.equals(second.producer)
         && Arrays.equals(lateEqualTag, second.tag);
      boolean lateEqualFirst = lateProducer.equals(first.producer)
         && Arrays.equals(lateEqualTag, first.tag)
         && earlyProducer.equals(second.producer)
         && Arrays.equals(earlyTag, second.tag);
      JavaTckSupport.check((earlyFirst || lateEqualFirst)
            && lateProducer.equals(third.producer) && Arrays.equals(lateTag, third.tag),
         description + " changed producer/tag identity or constrained equal-time ordering");
      for (Interaction interaction : recorder.interactions) {
         JavaTckSupport.check(expectedClass.equals(interaction.interactionClass)
               && interaction.parameters != null && interaction.parameters.size() == 1
               && Arrays.equals(payload, interaction.parameters.get(expectedParameter)),
            description + " received a different interaction or parameter payload");
         JavaTckSupport.check(interaction.sentOrder == OrderType.TIMESTAMP
               && interaction.receivedOrder == OrderType.TIMESTAMP
               && interaction.retraction != null && interaction.transportation != null
               && !inspector.getTransportationTypeName(interaction.transportation).isEmpty(),
            description + " received incomplete order, retraction, or transport metadata");
      }
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private hla.rti1516_2025.MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  interaction.tag = ((byte[]) arguments[2]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[3];
                  interaction.producer = (FederateHandle) arguments[4];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (hla.rti1516_2025.MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK cross-producer callbacks";
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
