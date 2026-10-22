package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
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
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Portable timestamp ordering and federation-unique retraction handles across services. */
final class TimestampedInteractionCrossProducerOrderTck {
   private TimestampedInteractionCrossProducerOrderTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, String objectClassName,
         String attributeName, CallbackModel callbackModel)
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
         ObjectClassHandle earlyObjectClass = early.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle earlyAttribute = early.getAttributeHandle(
            earlyObjectClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverObjectClass, attributeName);
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
         receiverPeer.enableTimeConstrained();
         LogicalTimeInterval earlyEpsilon = (LogicalTimeInterval) earlyFactory.makeEpsilon();
         LogicalTimeInterval lateEpsilon = (LogicalTimeInterval) lateFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTimeInterval receiverPeerEpsilon =
            (LogicalTimeInterval) receiverPeerFactory.makeEpsilon();
         early.enableTimeRegulation(earlyEpsilon);
         late.enableTimeRegulation(lateEpsilon);
         awaitAll(early, late, receiver, receiverPeer,
            () -> receiverRecorder.timeConstrainedEnabled
               && receiverPeerRecorder.timeConstrainedEnabled
               && earlyRecorder.timeRegulationEnabled
               && lateRecorder.timeRegulationEnabled,
            "cross-producer time-role callbacks");
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
         LogicalTime timeEight = offset(initial, earlyEpsilon, 8);
         LogicalTime lateEight = offset(lateInitial, lateEpsilon, 8);
         LogicalTime receiverEight = offset(receiverInitial, receiverEpsilon, 8);
         LogicalTime timeNine = offset(initial, earlyEpsilon, 9);
         LogicalTime receiverNine = offset(receiverInitial, receiverEpsilon, 9);
         JavaTckSupport.check(timeFive.equals(offset(lateInitial, lateEpsilon, 5))
               && timeSeven.equals(offset(lateInitial, lateEpsilon, 7))
               && timeEight.equals(lateEight)
               && timeFive.equals(receiverFive) && timeSeven.equals(receiverSeven)
               && timeFive.equals(receiverPeerFive) && timeSeven.equals(receiverPeerSeven),
            "cross-producer members did not produce matching logical timestamps");

         JavaTckSupport.check(earlyObjectClass != null && earlyAttribute != null
               && receiverObjectClass != null && receiverAttribute != null,
            "cross-producer timestamped attribute lookup returned an invalid handle");
         AttributeHandleSet earlyAttributes = early.getAttributeHandleSetFactory().create();
         earlyAttributes.add(earlyAttribute);
         AttributeHandleSet receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         early.publishObjectClassAttributes(earlyObjectClass, earlyAttributes);
         early.changeDefaultAttributeOrderType(
            earlyObjectClass, earlyAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         ObjectInstanceHandle object = early.registerObjectInstance(earlyObjectClass);
         JavaTckSupport.check(object != null,
            "cross-producer timestamped attribute registration returned an invalid handle");
         awaitAll(early, late, receiver, receiverPeer,
            () -> object.equals(receiverRecorder.discoveredObject),
            "cross-producer attribute object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "cross-producer timestamped attribute receiver did not discover the object");
         ObjectInstanceHandle deletionObject = early.registerObjectInstance(earlyObjectClass);
         JavaTckSupport.check(deletionObject != null,
            "cross-producer timestamped deletion registration returned an invalid object");
         awaitAll(early, late, receiver, receiverPeer,
            () -> deletionObject.equals(receiverRecorder.discoveredObject),
            "cross-producer deletion object discovery");
         JavaTckSupport.check(deletionObject.equals(receiverRecorder.discoveredObject),
            "cross-producer timestamped deletion receiver did not discover the object");

         byte[] payload = new byte[] {0x53, 0x54, 0x4f};
         byte[] earlyTag = new byte[] {0x45, 0x41};
         byte[] lateTag = new byte[] {0x4c, 0x41};
         byte[] lateEqualTag = new byte[] {0x4c, 0x45};
         byte[] attributeTag = new byte[] {0x41, 0x54};
         byte[] deletionTag = new byte[] {0x44, 0x45};
         AttributeHandleValueMap attributeValues =
            early.getAttributeHandleValueMapFactory().create(1);
         attributeValues.put(earlyAttribute, new byte[] {0x41});
         MessageRetractionReturn lateMessage = send(
            late, lateClass, lateParameter, payload, lateTag, timeSeven);
         MessageRetractionReturn earlyMessage = send(
            early, earlyClass, earlyParameter, payload, earlyTag, timeFive);
         MessageRetractionReturn lateEqualMessage = send(
            late, lateClass, lateParameter, payload, lateEqualTag, timeFive);
         MessageRetractionReturn attributeMessage = early.updateAttributeValues(
            object, attributeValues, attributeTag, timeEight);
         MessageRetractionReturn deletionMessage = early.deleteObjectInstance(
            deletionObject, deletionTag, timeNine);
         JavaTckSupport.check(valid(lateMessage) && valid(earlyMessage) && valid(lateEqualMessage),
            "cross-producer timestamped send returned an invalid retraction handle");
         JavaTckSupport.check(valid(attributeMessage),
            "timestamped attribute update returned an invalid retraction handle");
         JavaTckSupport.check(valid(deletionMessage),
            "timestamped object deletion returned an invalid retraction handle");
         JavaTckSupport.check(!lateMessage.handle.equals(earlyMessage.handle)
               && !lateMessage.handle.equals(lateEqualMessage.handle)
               && !earlyMessage.handle.equals(lateEqualMessage.handle)
               && !lateMessage.handle.equals(attributeMessage.handle)
               && !earlyMessage.handle.equals(attributeMessage.handle)
               && !lateEqualMessage.handle.equals(attributeMessage.handle)
               && !lateMessage.handle.equals(deletionMessage.handle)
               && !earlyMessage.handle.equals(deletionMessage.handle)
               && !lateEqualMessage.handle.equals(deletionMessage.handle)
               && !attributeMessage.handle.equals(deletionMessage.handle),
            "timestamped interaction, attribute-update, and object-deletion sends returned non-unique federation retraction handles");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(receiverPeer);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty()
               && receiverPeerRecorder.interactions.isEmpty()
               && receiverRecorder.reflections.isEmpty()
               && receiverRecorder.removals.isEmpty()
               && earlyRecorder.removals.isEmpty(),
            "cross-producer interaction arrived before the receiver requested an advance");

         early.timeAdvanceRequest(timeSeven);
         late.timeAdvanceRequest(timeSeven);
         receiver.timeAdvanceRequest(receiverFive);
         receiverPeer.timeAdvanceRequest(receiverPeerFive);
         awaitAll(early, late, receiver, receiverPeer,
            () -> receiverRecorder.interactions.size() == 2
               && receiverPeerRecorder.interactions.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && receiverPeerRecorder.timeAdvanceGrantTimes.size() == 1,
            "cross-producer time-five interaction delivery");
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
         awaitAll(early, late, receiver, receiverPeer,
            () -> receiverRecorder.interactions.size() == 3
               && receiverPeerRecorder.interactions.size() == 3
               && receiverRecorder.timeAdvanceGrantTimes.size() == 2
               && receiverPeerRecorder.timeAdvanceGrantTimes.size() == 2
               && earlyRecorder.timeAdvanceGrantTimes.size() == 1
               && lateRecorder.timeAdvanceGrantTimes.size() == 1,
            "cross-producer time-seven interaction delivery");
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
         JavaTckSupport.check(earlyRecorder.timeAdvanceGrantTimes.size() == 1
               && timeSeven.equals(earlyRecorder.timeAdvanceGrantTimes.get(0)),
            "cross-producer early sender did not receive its time-seven grant before its next request");
         JavaTckSupport.check(lateRecorder.timeAdvanceGrantTimes.size() == 1
               && timeSeven.equals(lateRecorder.timeAdvanceGrantTimes.get(0)),
            "cross-producer late sender did not receive its time-seven grant before its next request");
         assertOrder(receiverRecorder, receiverClass, receiverParameter, earlyProducer,
            lateProducer, payload, earlyTag, lateEqualTag, lateTag, timeFive, timeSeven,
            receiver, "first receiver");
         assertOrder(receiverPeerRecorder, receiverPeerClass, receiverPeerParameter,
            earlyProducer, lateProducer, payload, earlyTag, lateEqualTag, lateTag,
            timeFive, timeSeven, receiverPeer, "peer receiver");
         JavaTckSupport.check(receiverSeven.equals(receiverRecorder.timeAdvanceGrantTimes.get(1))
               && receiverPeerSeven.equals(receiverPeerRecorder.timeAdvanceGrantTimes.get(1)),
            "cross-producer receivers received the wrong final grant time");

         // Establish a strict frontier: both regulators move to eight
         // before the constrained receiver requests its time-eight grant.
         early.timeAdvanceRequest(timeEight);
         late.timeAdvanceRequest(lateEight);
         receiver.timeAdvanceRequest(receiverEight);
         awaitAll(early, late, receiver, receiverPeer,
            () -> receiverRecorder.reflections.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 3,
            "cross-service timestamped attribute delivery at time eight");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 3,
            "cross-service timestamped attribute update did not arrive before the receiver grant");
         Reflection attributeReflection = receiverRecorder.reflections.get(0);
         JavaTckSupport.check(object.equals(attributeReflection.object)
               && attributeReflection.values.size() == 1
               && Arrays.equals(new byte[] {0x41}, attributeReflection.values.get(receiverAttribute))
               && Arrays.equals(attributeTag, attributeReflection.tag)
               && earlyProducer.equals(attributeReflection.producer)
               && timeEight.equals(attributeReflection.time)
               && attributeReflection.sentOrder == OrderType.TIMESTAMP
               && attributeReflection.receivedOrder == OrderType.TIMESTAMP
               && attributeMessage.handle.equals(attributeReflection.retraction),
            "cross-service timestamped attribute reflection returned incorrect metadata");
         JavaTckSupport.check(receiverEight.equals(receiverRecorder.timeAdvanceGrantTimes.get(2))
               && receiverRecorder.callbackOrder.size() == 7
               && "reflectAttributeValues".equals(receiverRecorder.callbackOrder.get(5))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(6)),
            "cross-service timestamped attribute reflection was not delivered before its grant");

         awaitAll(early, late, receiver, receiverPeer,
            () -> earlyRecorder.timeAdvanceGrantTimes.size() == 2
               && lateRecorder.timeAdvanceGrantTimes.size() == 2,
            "cross-service sender frontier at time eight");
         JavaTckSupport.check(earlyRecorder.timeAdvanceGrantTimes.size() == 2
               && timeEight.equals(earlyRecorder.timeAdvanceGrantTimes.get(1)),
            "cross-service timestamped sender did not advance its frontier to time eight");
         JavaTckSupport.check(lateRecorder.timeAdvanceGrantTimes.size() == 2
               && lateEight.equals(lateRecorder.timeAdvanceGrantTimes.get(1)),
            "cross-service second sender did not advance its frontier to time eight");

         LogicalTime lateNine = offset(lateInitial, lateEpsilon, 9);
         early.timeAdvanceRequest(timeNine);
         late.timeAdvanceRequest(lateNine);
         awaitAll(early, late, receiver, receiverPeer,
            () -> earlyRecorder.timeAdvanceGrantTimes.size() == 3
               && lateRecorder.timeAdvanceGrantTimes.size() == 3,
            "cross-service sender frontier at time nine");
         receiver.timeAdvanceRequest(receiverNine);
         awaitAll(early, late, receiver, receiverPeer,
            () -> receiverRecorder.removals.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 4,
            "cross-service timestamped object deletion at time nine");
         JavaTckSupport.check(receiverRecorder.removals.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.size() == 4,
            "cross-service timestamped object deletion did not arrive before the receiver grant");
         Removal deletion = receiverRecorder.removals.get(0);
         JavaTckSupport.check(deletionObject.equals(deletion.object)
               && Arrays.equals(deletionTag, deletion.tag)
               && earlyProducer.equals(deletion.producer)
               && timeNine.equals(deletion.time)
               && deletion.sentOrder == OrderType.TIMESTAMP
               && deletion.receivedOrder == OrderType.TIMESTAMP
               && deletionMessage.handle.equals(deletion.retraction),
            "cross-service timestamped object-removal callback returned incorrect metadata");
         JavaTckSupport.check(earlyRecorder.removals.isEmpty()
               && receiverPeerRecorder.removals.isEmpty(),
            "timestamped deletion looped back to its sender or reached an unsubscribed peer");
         JavaTckSupport.check(receiverNine.equals(receiverRecorder.timeAdvanceGrantTimes.get(3))
               && receiverRecorder.callbackOrder.size() == 9
               && "removeObjectInstance".equals(receiverRecorder.callbackOrder.get(7))
               && "timeAdvanceGrant".equals(receiverRecorder.callbackOrder.get(8)),
            "cross-service timestamped object removal was not delivered before its grant");

         receiver.disableTimeConstrained();
         JavaTckSupport.drain(receiver);
         receiverPeer.disableTimeConstrained();
         JavaTckSupport.drain(receiverPeer);
         early.disableTimeRegulation();
         JavaTckSupport.drain(early);
         late.disableTimeRegulation();
         JavaTckSupport.drain(late);
         receiver.unsubscribeInteractionClass(receiverClass);
         receiver.unsubscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         receiverPeer.unsubscribeInteractionClass(receiverPeerClass);
         early.unpublishInteractionClass(earlyClass);
         late.unpublishInteractionClass(lateClass);
         early.unpublishObjectClassAttributes(earlyObjectClass, earlyAttributes);
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

   private static void awaitAll(RTIambassador early, RTIambassador late,
         RTIambassador receiver, RTIambassador receiverPeer, BooleanSupplier observed,
         String phase) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         drainAll(early, late, receiver, receiverPeer);
         if (observed.getAsBoolean()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(phase + " did not complete within 10 seconds");
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

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private final Map<AttributeHandle, byte[]> values = new HashMap<>();
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Interaction> interactions = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

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
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
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
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  AttributeHandleValueMap values = (AttributeHandleValueMap) arguments[1];
                  for (AttributeHandle attribute : values.keySet()) {
                     reflection.values.put(attribute, values.get(attribute).clone());
                  }
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
                  callbackOrder.add(name);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(removal);
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
