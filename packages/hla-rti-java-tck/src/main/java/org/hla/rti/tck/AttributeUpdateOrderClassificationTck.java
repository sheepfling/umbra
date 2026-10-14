package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
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

/** Classifies receive-order and timestamp-order attribute updates at one receiver. */
final class AttributeUpdateOrderClassificationTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeUpdateOrderClassificationTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-attribute-order-classification-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      ObjectClassHandle publisherClass = null, receiverClass = null;
      AttributeHandle publisherAttribute = null, receiverAttribute = null;
      AttributeHandleSet publisherAttributes = null, receiverAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-order-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-attribute-order-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null
               && objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(receiver.getObjectClassName(receiverClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(receiver.getAttributeName(receiverClass, receiverAttribute)),
            "adapter FOM did not resolve and round-trip object/attribute handles");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle receiveObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(receiveObject != null && receiveObject.encodedLength() > 0,
            "receive-order attribute update registered an invalid object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(receiveObject),
            "receive-order object discovery");
         receiverCallbacks.receiveOrder.clear();
         publisherCallbacks.receiveOrder.clear();

         byte[] receiveValue = new byte[] {0x52, 0x4f};
         byte[] receiveTag = new byte[] {0x52, 0x4f};
         send(publisher, receiveObject, publisherAttribute, receiveValue, receiveTag);
         await(receiver, callbackModel, () -> receiverCallbacks.receiveOrder.size() == 1,
            "receive-order attribute reflection");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.isEmpty(),
            "ordinary Update Attribute Values did not use only the receive-order callback path");
         assertReflection(receiverCallbacks.receiveOrder.get(0), receiveObject,
            receiverAttribute, receiveValue, receiveTag, producer, receiver, false, null, null);

         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle timestampedObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(timestampedObject != null && timestampedObject.encodedLength() > 0,
            "timestamp-order attribute update registered an invalid object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(timestampedObject),
            "timestamp-order object discovery");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel, () -> publisherCallbacks.timeRegulationEnabled,
            "publisher time regulation enablement");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel, () -> receiverCallbacks.timeConstrainedEnabled,
            "receiver time constrained enablement");

         LogicalTime timestamp = offset(publisher.queryLogicalTime(), epsilon, 3);
         LogicalTime receiverTarget = offset(receiver.queryLogicalTime(), epsilon, 3);
         JavaTckSupport.check(timestamp.equals(receiverTarget),
            "publisher and receiver derived different timestamp-order advances");
         byte[] timestampValue = new byte[] {0x54, 0x53, 0x4f};
         byte[] timestampTag = new byte[] {0x54, 0x53};
         MessageRetractionReturn retraction = sendTimestamped(
            publisher, timestampedObject, publisherAttribute, timestampValue,
            timestampTag, timestamp);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Update Attribute Values returned no valid retraction handle");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.isEmpty(),
            "timestamped attribute update was delivered before receiver advance");

         receiverCallbacks.callbackOrder.clear();
         publisher.timeAdvanceRequest(timestamp);
         receiver.timeAdvanceRequest(receiverTarget);
         awaitBoth(publisher, receiver, callbackModel,
            () -> publisherCallbacks.timeAdvanceGrant != null
               && receiverCallbacks.timeAdvanceGrant != null
               && receiverCallbacks.timestampOrder.size() == 1,
            "timestamped reflection and matching time-advance grants");
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestampOrder.size() == 1,
            "same receiver did not keep receive-order and timestamp-order on distinct callback paths");
         assertReflection(receiverCallbacks.timestampOrder.get(0), timestampedObject,
            receiverAttribute, timestampValue, timestampTag, producer, receiver, true,
            timestamp, retraction.handle);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "timestamped reflection was not delivered before its matching grant");
         JavaTckSupport.check(timestamp.equals(publisherCallbacks.timeAdvanceGrant)
               && receiverTarget.equals(receiverCallbacks.timeAdvanceGrant),
            "time-advance grants returned different times from their requests");

         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
      } finally {
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) resign(receiver);
         if (publisherJoined) {
            try { publisher.resignFederationExecution(ResignAction.DELETE_OBJECTS); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void send(RTIambassador ambassador, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      ambassador.updateAttributeValues(object, values, tag.clone());
   }

   private static MessageRetractionReturn sendTimestamped(RTIambassador ambassador,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      return ambassador.updateAttributeValues(object, values, tag.clone(), time);
   }

   private static void assertReflection(Reflection event, ObjectInstanceHandle object,
         AttributeHandle attribute, byte[] value, byte[] tag, FederateHandle producer,
         RTIambassador inspector, boolean timestamped, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction) throws Exception {
      JavaTckSupport.check(object.equals(event.object) && event.values.size() == 1
            && Arrays.equals(value, event.values.get(attribute)) && Arrays.equals(tag, event.tag)
            && producer.equals(event.producer) && event.transportation != null
            && !inspector.getTransportationTypeName(event.transportation).isEmpty()
            && event.regionsEmpty,
         "attribute reflection returned incorrect object, value, tag, producer, or metadata");
      if (timestamped) {
         JavaTckSupport.check(expectedTime.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && expectedRetraction.equals(event.retraction),
            "timestamped reflection returned incorrect time, order, or retraction metadata");
      } else {
         JavaTckSupport.check(event.time == null && event.sentOrder == null,
            "ordinary reflection unexpectedly used the timestamped callback path");
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      awaitBoth(ambassador, null, model, condition, description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            if (second != null) JavaTckSupport.drain(second);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            boolean regionsEmpty, LogicalTime<?, ?> time, OrderType sentOrder,
            OrderType receivedOrder, MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.regionsEmpty = regionsEmpty;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Reflection> timestampOrder = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments[5] == null || ((hla.rti1516_2025.RegionHandleSet) arguments[5]).isEmpty(),
                     null, null, null, null));
                  callbackOrder.add(name);
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestampOrder.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     arguments[5] == null || ((hla.rti1516_2025.RegionHandleSet) arguments[5]).isEmpty(),
                     (LogicalTime<?, ?>) arguments[6], (OrderType) arguments[7],
                     (OrderType) arguments[8], (MessageRetractionHandle) arguments[9]));
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute-order callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
