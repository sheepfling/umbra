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
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
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

/** Tests prospective attribute-order defaults, instance overrides, and receive-order interaction delivery. */
final class OrderTypeControlsTck {
   private OrderTypeControlsTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         String parameterName, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-order-type-controls-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-order-controls-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-order-controls-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         InteractionClassHandle publisherInteraction =
            publisher.getInteractionClassHandle(interactionClassName);
         InteractionClassHandle receiverInteraction =
            receiver.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         ParameterHandle receiverParameter = receiver.getParameterHandle(
            receiverInteraction, parameterName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         publisher.publishInteractionClass(publisherInteraction);
         receiver.subscribeInteractionClass(receiverInteraction);

         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.RECEIVE);
         ObjectInstanceHandle receiveOrderObject = publisher.registerObjectInstance(publisherClass);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         ObjectInstanceHandle defaultTimestampObject = publisher.registerObjectInstance(publisherClass);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.RECEIVE);
         ObjectInstanceHandle explicitTimestampObject = publisher.registerObjectInstance(publisherClass);
         AttributeHandleSet instanceAttribute = publisher.getAttributeHandleSetFactory().create();
         instanceAttribute.add(publisherAttribute);
         publisher.changeAttributeOrderType(
            explicitTimestampObject, instanceAttribute, OrderType.TIMESTAMP);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.discoveries.contains(receiveOrderObject)
               && receiverRecorder.discoveries.contains(defaultTimestampObject)
               && receiverRecorder.discoveries.contains(explicitTimestampObject),
            "receiver did not discover all objects created under the successive order defaults");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         receiver.enableTimeConstrained();
         JavaTckSupport.drain(receiver);
         receiver.enableAsynchronousDelivery();
         publisher.enableTimeRegulation(lookahead);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled,
            "time-constrained or time-regulation callback was not delivered");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime messageTime = offset(publisherInitial, epsilon, 6);
         LogicalTime receiverTarget = offset(receiverInitial, epsilon, 6);
         JavaTckSupport.check(messageTime.equals(receiverTarget),
            "federates did not derive the same logical-time target");
         byte[] receiveValue = new byte[] {0x11};
         byte[] defaultTimestampValue = new byte[] {0x22};
         byte[] explicitTimestampValue = new byte[] {0x33};
         byte[] attributeTag = new byte[] {0x4f, 0x52, 0x44};
         publisher.updateAttributeValues(receiveOrderObject,
            oneAttribute(publisher, publisherAttribute, receiveValue), attributeTag);
         MessageRetractionReturn defaultRetraction = publisher.updateAttributeValues(
            defaultTimestampObject,
            oneAttribute(publisher, publisherAttribute, defaultTimestampValue),
            attributeTag, messageTime);
         MessageRetractionReturn explicitRetraction = publisher.updateAttributeValues(
            explicitTimestampObject,
            oneAttribute(publisher, publisherAttribute, explicitTimestampValue),
            attributeTag, messageTime);
         JavaTckSupport.check(valid(defaultRetraction) && valid(explicitRetraction),
            "timestamp-ordered attribute updates did not return valid retraction handles");

         receiver.timeAdvanceRequest(receiverTarget);
         publisher.timeAdvanceRequest(messageTime);
         drainAll(publisher, receiver);
         JavaTckSupport.check(receiverRecorder.receiveOrderReflections.size() == 1
               && receiverRecorder.timestampedReflections.size() == 2
               && receiverRecorder.timeAdvanceGrantTimes.size() == 1
               && messageTime.equals(receiverRecorder.timeAdvanceGrantTimes.get(0)),
            "attribute-order defaults did not produce one receive-order and two timestamped reflections");
         assertReceiveOrder(receiverRecorder.receiveOrderReflections.get(0),
            receiveOrderObject, receiverAttribute, receiveValue, attributeTag, producer,
            receiver, "receive-order default");
         assertTimestamped(receiverRecorder.timestampedReflections,
            defaultTimestampObject, explicitTimestampObject, receiverAttribute,
            defaultTimestampValue, explicitTimestampValue, attributeTag, producer,
            messageTime, defaultRetraction.handle, explicitRetraction.handle,
            receiver, "timestamp-order defaults and instance override");

         publisher.changeInteractionOrderType(publisherInteraction, OrderType.RECEIVE);
         byte[] parameterValue = new byte[] {0x49, 0x4e, 0x54};
         byte[] interactionTag = new byte[] {0x49, 0x4e, 0x54};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(1);
         parameters.put(publisherParameter, parameterValue);
         LogicalTime interactionTime = offset(publisherInitial, epsilon, 12);
         MessageRetractionReturn interactionResult = publisher.sendInteraction(
            publisherInteraction, parameters, interactionTag, interactionTime);
         JavaTckSupport.check(!valid(interactionResult),
            "receive-ordered interaction unexpectedly returned a retraction handle");
         drainAll(publisher, receiver);
         JavaTckSupport.check(receiverRecorder.timestampedInteractions.size() == 1
               && receiverRecorder.receiveOrderInteractions.isEmpty(),
            "receive-ordered timestamped send used the wrong callback overload");
         assertReceiveOrderInteraction(receiverRecorder.timestampedInteractions.get(0),
            receiverInteraction, receiverParameter, parameterValue, interactionTag,
            producer, interactionTime, receiver, "receive-order interaction control");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         drainAll(publisher, receiver);
         receiver.unsubscribeInteractionClass(receiverInteraction);
         publisher.unpublishInteractionClass(publisherInteraction);
         receiver.unsubscribeObjectClassAttributes(receiverClass, subscribed);
         publisher.unpublishObjectClassAttributes(publisherClass, published);
      } finally {
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
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

   private static AttributeHandleValueMap oneAttribute(RTIambassador ambassador,
         AttributeHandle attribute, byte[] value) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      return values;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static void drainAll(RTIambassador publisher, RTIambassador receiver)
         throws Exception {
      for (int i = 0; i < 64; ++i) {
         publisher.evokeCallback(0.0);
         receiver.evokeCallback(0.0);
      }
   }

   private static void assertReceiveOrder(Reflection event, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
            && Arrays.equals(value, event.values.get(attribute))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " did not preserve object, payload, or receive-order callback metadata");
   }

   private static void assertTimestamped(List<Reflection> events,
         ObjectInstanceHandle defaultObject, ObjectInstanceHandle explicitObject,
         AttributeHandle attribute, byte[] defaultValue, byte[] explicitValue, byte[] tag,
         FederateHandle producer, LogicalTime time,
         MessageRetractionHandle defaultRetraction,
         MessageRetractionHandle explicitRetraction, RTIambassador inspector,
         String description) throws Exception {
      JavaTckSupport.check(events.size() == 2 && !events.get(0).object.equals(events.get(1).object),
         description + " collapsed or duplicated the two timestamped instances");
      boolean defaultFirst = defaultObject.equals(events.get(0).object)
         && explicitObject.equals(events.get(1).object);
      boolean explicitFirst = explicitObject.equals(events.get(0).object)
         && defaultObject.equals(events.get(1).object);
      JavaTckSupport.check(defaultFirst || explicitFirst,
         description + " returned an unexpected object instance");
      for (Reflection event : events) {
         byte[] expectedValue = defaultObject.equals(event.object) ? defaultValue : explicitValue;
         MessageRetractionHandle expectedRetraction = defaultObject.equals(event.object)
            ? defaultRetraction : explicitRetraction;
         JavaTckSupport.check(event.values.size() == 1
               && Arrays.equals(expectedValue, event.values.get(attribute))
               && Arrays.equals(tag, event.tag) && time.equals(event.time)
               && producer.equals(event.producer)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && expectedRetraction.equals(event.retraction)
               && event.transportation != null
               && !inspector.getTransportationTypeName(event.transportation).isEmpty()
               && event.regionsEmpty,
            description + " lost value, tag, time, order, producer, transport, or retraction metadata");
      }
   }

   private static void assertReceiveOrderInteraction(Interaction event,
         InteractionClassHandle interactionClass, ParameterHandle parameter,
         byte[] value, byte[] tag, FederateHandle producer, LogicalTime timestamp,
         RTIambassador inspector, String description) throws Exception {
      JavaTckSupport.check(interactionClass.equals(event.interactionClass)
            && event.parameters.size() == 1 && Arrays.equals(value, event.parameters.get(parameter))
            && Arrays.equals(tag, event.tag) && producer.equals(event.producer)
            && timestamp.equals(event.time) && event.sentOrder == OrderType.RECEIVE
            && event.receivedOrder == OrderType.RECEIVE && event.retraction == null
            && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         description + " did not preserve receive-order delivery and timestamped callback metadata");
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Interaction {
      private InteractionClassHandle interactionClass;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private boolean regionsEmpty;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<Reflection> receiveOrderReflections = new ArrayList<>();
      private final List<Reflection> timestampedReflections = new ArrayList<>();
      private final List<Interaction> receiveOrderInteractions = new ArrayList<>();
      private final List<Interaction> timestampedInteractions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  Reflection event = new Reflection();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  if (arguments.length == 10) {
                     event.time = (LogicalTime<?, ?>) arguments[6];
                     event.sentOrder = (OrderType) arguments[7];
                     event.receivedOrder = (OrderType) arguments[8];
                     event.retraction = (MessageRetractionHandle) arguments[9];
                     timestampedReflections.add(event);
                  } else {
                     receiveOrderReflections.add(event);
                  }
               } else if ("receiveInteraction".equals(name)
                     && arguments != null && (arguments.length == 6 || arguments.length == 10)) {
                  Interaction event = new Interaction();
                  event.interactionClass = (InteractionClassHandle) arguments[0];
                  event.parameters = ((ParameterHandleValueMap) arguments[1]).clone();
                  event.tag = ((byte[]) arguments[2]).clone();
                  event.transportation = (TransportationTypeHandle) arguments[3];
                  event.producer = (FederateHandle) arguments[4];
                  event.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  if (arguments.length == 10) {
                     event.time = (LogicalTime<?, ?>) arguments[6];
                     event.sentOrder = (OrderType) arguments[7];
                     event.receivedOrder = (OrderType) arguments[8];
                     event.retraction = (MessageRetractionHandle) arguments[9];
                     timestampedInteractions.add(event);
                  } else {
                     receiveOrderInteractions.add(event);
                  }
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK order type callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
