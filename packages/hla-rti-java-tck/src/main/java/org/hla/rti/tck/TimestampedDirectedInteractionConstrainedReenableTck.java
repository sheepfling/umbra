package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Preserves timestamped directed delivery across Time Constrained re-enable. */
final class TimestampedDirectedInteractionConstrainedReenableTck {
   private TimestampedDirectedInteractionConstrainedReenableTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-directed-reenable-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean publisherAttributesPublished = false;
      boolean receiverAttributesPublished = false;
      boolean publisherAttributesSubscribed = false;
      boolean receiverAttributesSubscribed = false;
      boolean directedPublished = false;
      boolean directedSubscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-directed-reenable-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-directed-reenable-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherObjectClass =
            publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherObjectClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverObjectClass, attributeName);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle receiverParameter =
            receiver.getParameterHandle(receiverInteraction, parameterName);

         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         publisherAttributesPublished = true;
         receiver.publishObjectClassAttributes(receiverObjectClass, receiverAttributes);
         receiverAttributesPublished = true;
         publisher.subscribeObjectClassAttributes(publisherObjectClass, publisherAttributes);
         publisherAttributesSubscribed = true;
         receiver.subscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherObjectClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         directedPublished = true;
         receiver.subscribeObjectClassDirectedInteractions(receiverObjectClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         directedSubscribed = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = receiver.registerObjectInstance(receiverObjectClass);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRecorder.discoveries.contains(target),
            "publisher did not discover the directed-interaction target");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiverTimeFactory.getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "publisher and receiver began at different logical times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report its initial Time Constrained callback");
         publisher.enableTimeRegulation(epsilon);
         publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report its Time Regulation callback");

         LogicalTime messageTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 4);
         byte[] payload = new byte[] {(byte) 0xD1, 0x52};
         byte[] tag = new byte[] {0x52, 0x45, 0x45, 0x4E, 0x2D, 0x44};
         ParameterHandleValueMap values = publisher.getParameterHandleValueMapFactory().create(1);
         values.put(publisherParameter, payload);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, values, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped directed send returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "directed interaction arrived before the receiver's grant");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "queued directed interaction arrived while Time Constrained was disabled");
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 2,
            "receiver did not report its second Time Constrained callback");
         JavaTckSupport.check(receiverRecorder.interactions.isEmpty(),
            "queued directed interaction arrived during Time Constrained re-enable");

         receiverRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(messageTime);
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.interactions.size() == 1,
            "queued directed interaction was not delivered exactly once");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("receiveDirectedInteraction", "timeAdvanceGrant")),
            "directed delivery did not precede the receiver's grant");
         JavaTckSupport.check(messageTime.equals(receiverRecorder.timeAdvanceGrant)
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrant),
            "time-advance grants did not report the requested times");
         JavaTckSupport.check(messageTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries did not match the grants");
         checkInteraction(receiverRecorder.interactions.get(0), receiverInteraction, target,
            receiverParameter, payload, tag, producer, messageTime, sent.handle, receiver);
         expectException(() -> publisher.retract(sent.handle), "MessageCanNoLongerBeRetracted",
            "retracting the directed interaction after delivery");
      } finally {
         if (receiverConstrained) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (directedSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass(
               receiver, objectClassName)); } catch (Exception ignored) { }
         }
         if (directedPublished && publisherJoined) {
            try { publisher.unpublishObjectClassDirectedInteractions(
               publisherObjectClass(publisher, objectClassName)); } catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(
               receiverObjectClass(receiver, objectClassName),
               JavaTckSupport.attributeSet(receiver,
                  receiver.getAttributeHandle(receiverObjectClass(receiver, objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (receiverAttributesPublished && receiverJoined) {
            try { receiver.unpublishObjectClassAttributes(
               receiverObjectClass(receiver, objectClassName),
               JavaTckSupport.attributeSet(receiver,
                  receiver.getAttributeHandle(receiverObjectClass(receiver, objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (publisherAttributesSubscribed && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(
               publisherObjectClass(publisher, objectClassName),
               JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(publisherObjectClass(publisher, objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
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
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static ObjectClassHandle publisherObjectClass(RTIambassador rti, String name)
         throws Exception {
      return rti.getObjectClassHandle(name);
   }

   private static ObjectClassHandle receiverObjectClass(RTIambassador rti, String name)
         throws Exception {
      return rti.getObjectClassHandle(name);
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void checkInteraction(Interaction actual, InteractionClassHandle expectedClass,
         ObjectInstanceHandle expectedObject, ParameterHandle expectedParameter,
         byte[] expectedPayload, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         RTIambassador inspector) throws Exception {
      JavaTckSupport.check(expectedClass.equals(actual.interactionClass)
            && expectedObject.equals(actual.object)
            && actual.parameters != null && actual.parameters.size() == 1
            && Arrays.equals(expectedPayload, actual.parameters.get(expectedParameter)),
         "directed callback returned the wrong target or parameter");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && expectedProducer.equals(actual.producer) && expectedTime.equals(actual.time),
         "directed callback did not preserve tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP,
         "directed callback returned the wrong sent or received order");
      JavaTckSupport.check(expectedRetraction.equals(actual.retraction)
            && actual.transportation != null
            && !inspector.getTransportationTypeName(actual.transportation).isEmpty(),
         "directed callback omitted the retraction or resolvable transport handle");
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
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle object;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private int timeRegulationEnabledCount;
      private int timeConstrainedEnabledCount;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Interaction> interactions = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Interaction interaction = new Interaction();
                  interaction.interactionClass = (InteractionClassHandle) arguments[0];
                  interaction.object = (ObjectInstanceHandle) arguments[1];
                  interaction.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                  interaction.tag = ((byte[]) arguments[3]).clone();
                  interaction.transportation = (TransportationTypeHandle) arguments[4];
                  interaction.producer = (FederateHandle) arguments[5];
                  interaction.time = (LogicalTime<?, ?>) arguments[6];
                  interaction.sentOrder = (OrderType) arguments[7];
                  interaction.receivedOrder = (OrderType) arguments[8];
                  interaction.retraction = (MessageRetractionHandle) arguments[9];
                  interactions.add(interaction);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped directed callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
