package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RegionHandleSet;
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

/** Checks timestamped attribute order defaults supplied by the adapter's FOM. */
final class AttributeFddPreferredOrderDefaultTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private AttributeFddPreferredOrderDefaultTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String timestampPreferredAttributeName,
         String receivePreferredAttributeName, CallbackModel callbackModel) throws Exception {
      RTIambassador sender = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder senderCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-attribute-fdd-order-default-" + UUID.randomUUID();
      boolean senderConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean senderJoined = false;
      boolean receiverJoined = false;
      boolean regulating = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle senderClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet senderAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         sender.connect(senderCallbacks.proxy(), callbackModel);
         senderConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         sender.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = sender.joinFederationExecution(
            "java-tck-attribute-fdd-order-sender", federation);
         senderJoined = true;
         receiver.joinFederationExecution("java-tck-attribute-fdd-order-receiver", federation);
         receiverJoined = true;

         senderClass = sender.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle senderTimestampAttribute = sender.getAttributeHandle(
            senderClass, timestampPreferredAttributeName);
         AttributeHandle senderReceiveAttribute = sender.getAttributeHandle(
            senderClass, receivePreferredAttributeName);
         AttributeHandle receiverTimestampAttribute = receiver.getAttributeHandle(
            receiverClass, timestampPreferredAttributeName);
         AttributeHandle receiverReceiveAttribute = receiver.getAttributeHandle(
            receiverClass, receivePreferredAttributeName);
         JavaTckSupport.check(senderClass != null && receiverClass != null
               && senderClass.equals(receiverClass)
               && senderTimestampAttribute != null && senderReceiveAttribute != null
               && receiverTimestampAttribute != null && receiverReceiveAttribute != null
               && senderTimestampAttribute.equals(receiverTimestampAttribute)
               && senderReceiveAttribute.equals(receiverReceiveAttribute),
            "adapter FOM did not provide the configured object class and ordered attributes");

         senderAttributes = sender.getAttributeHandleSetFactory().create();
         senderAttributes.add(senderTimestampAttribute);
         senderAttributes.add(senderReceiveAttribute);
         receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverTimestampAttribute);
         receiverAttributes.add(receiverReceiveAttribute);
         sender.publishObjectClassAttributes(senderClass, senderAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         ObjectInstanceHandle object = sender.registerObjectInstance(senderClass);
         JavaTckSupport.check(object != null,
            "FDD-default attribute registration returned no object handle");
         await(receiver, callbackModel, () -> receiverCallbacks.discoveries.contains(object),
            "FDD-default attribute object discovery");

         LogicalTimeFactory<?, ?> senderFactory = sender.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(senderFactory.getName().equals(receiverFactory.getName()),
            "FDD-default attribute members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) senderFactory.makeEpsilon();
         LogicalTime<?, ?> initial = sender.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "FDD-default attribute members began at different logical times");
         LogicalTime<?, ?> timestampTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> receiveTime = offset(initial, epsilon, 6);
         LogicalTimeInterval lookahead = (LogicalTimeInterval) senderFactory.makeEpsilon();
         sender.enableTimeRegulation(lookahead);
         regulating = true;
         await(sender, callbackModel, () -> senderCallbacks.timeRegulationEnabled,
            "FDD-default attribute sender regulation callback");

         byte[] optionalTimestampValue = {0x4f, 0x50, 0x54};
         byte[] optionalTimestampTag = {0x52, 0x4f};
         AttributeHandleValueMap optionalTimestampValues =
            sender.getAttributeHandleValueMapFactory().create(1);
         optionalTimestampValues.put(
            senderTimestampAttribute, optionalTimestampValue.clone());
         // This attribute is TimeStamp-preferred in the adapter FOM and the
         // publisher is regulating, but the timestamp argument is optional.
         sender.updateAttributeValues(
            object, optionalTimestampValues, optionalTimestampTag.clone());
         await(receiver, callbackModel, () -> receiverCallbacks.receiveOrder.size() == 1,
            "no-time update for a TimeStamp-preferred FDD attribute");
         OrdinaryReflection ordinary = receiverCallbacks.receiveOrder.get(0);
         JavaTckSupport.check(receiverCallbacks.timestamped.isEmpty()
               && object.equals(ordinary.object) && ordinary.values.size() == 1
               && Arrays.equals(optionalTimestampValue,
                  ordinary.values.get(receiverTimestampAttribute))
               && Arrays.equals(optionalTimestampTag, ordinary.tag)
               && producer.equals(ordinary.producer) && ordinary.regionsEmpty
               && ordinary.transportation != null
               && !receiver.getTransportationTypeName(ordinary.transportation).isEmpty(),
            "no-time TimeStamp-preferred update did not produce an ordinary reflection");

         byte[] timestampValue = {0x46, 0x44, 0x44, 0x54};
         byte[] receiveValue = {0x46, 0x44, 0x44, 0x52};
         byte[] timestampTag = {0x54, 0x53};
         byte[] receiveTag = {0x52, 0x4f};
         AttributeHandleValueMap timestampValues =
            sender.getAttributeHandleValueMapFactory().create(1);
         timestampValues.put(senderTimestampAttribute, timestampValue.clone());
         AttributeHandleValueMap receiveValues =
            sender.getAttributeHandleValueMapFactory().create(1);
         receiveValues.put(senderReceiveAttribute, receiveValue.clone());
         MessageRetractionReturn timestampRetraction = sender.updateAttributeValues(
            object, timestampValues, timestampTag.clone(), timestampTime);
         MessageRetractionReturn receiveRetraction = sender.updateAttributeValues(
            object, receiveValues, receiveTag.clone(), receiveTime);
         JavaTckSupport.check(timestampRetraction != null
               && timestampRetraction.retractionHandleIsValid
               && timestampRetraction.handle != null,
            "FDD-preferred timestamp-order update returned no retraction designator");
         JavaTckSupport.check(receiveRetraction != null
               && !receiveRetraction.retractionHandleIsValid
               && receiveRetraction.handle == null,
            "FDD-preferred receive-order update unexpectedly returned a retraction designator");

         await(receiver, callbackModel, () -> receiverCallbacks.timestamped.size() == 2,
            "both FDD-preferred timestamp-bearing reflections");
         Reflection timestampDelivery = receiverCallbacks.forAttribute(receiverTimestampAttribute);
         Reflection receiveDelivery = receiverCallbacks.forAttribute(receiverReceiveAttribute);
         JavaTckSupport.check(receiverCallbacks.receiveOrder.size() == 1
               && receiverCallbacks.timestamped.size() == 2
               && timestampDelivery != null && receiveDelivery != null,
            "FDD-preferred updates did not preserve the ordinary/timestamped split");
         checkDelivery(receiver, timestampDelivery, object, receiverTimestampAttribute, producer,
            timestampTime, OrderType.TIMESTAMP, timestampValue, timestampTag,
            timestampRetraction.handle, "timestamp-preferred object attribute");
         checkDelivery(receiver, receiveDelivery, object, receiverReceiveAttribute, producer,
            receiveTime, OrderType.RECEIVE, receiveValue, receiveTag, null,
            "receive-preferred object attribute");
         JavaTckSupport.check(receiverCallbacks.sequence.size() == 3
               && receiverCallbacks.sequence.stream().allMatch("reflect"::equals),
            "FDD-preferred updates did not produce exactly three reflection callbacks");
      } finally {
         if (regulating && senderJoined) {
            try { sender.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined && receiverClass != null && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && senderJoined && senderClass != null && senderAttributes != null) {
            try { sender.unpublishObjectClassAttributes(senderClass, senderAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (senderJoined) {
            try { sender.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { sender.destroyFederationExecution(federation); } catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (senderConnected) {
            try { sender.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   static void runDefaultInitialization(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador initialFederate = factory.getRtiAmbassador();
      RTIambassador observer = factory.getRtiAmbassador();
      RTIambassador lateFederate = factory.getRtiAmbassador();
      Recorder initialCallbacks = new Recorder();
      Recorder observerCallbacks = new Recorder();
      Recorder lateCallbacks = new Recorder();
      String federation = "java-tck-fdd-attribute-order-initialization-" + UUID.randomUUID();
      boolean initialConnected = false;
      boolean observerConnected = false;
      boolean lateConnected = false;
      boolean created = false;
      boolean initialJoined = false;
      boolean observerJoined = false;
      boolean lateJoined = false;
      boolean initialPublished = false;
      boolean observerSubscribed = false;
      boolean latePublished = false;
      boolean initialRegulating = false;
      boolean lateRegulating = false;
      ObjectClassHandle initialClass = null;
      ObjectClassHandle observerClass = null;
      ObjectClassHandle lateClass = null;
      AttributeHandleSet initialAttributes = null;
      AttributeHandleSet observerAttributes = null;
      AttributeHandleSet lateAttributes = null;
      try {
         initialFederate.connect(initialCallbacks.proxy(), callbackModel);
         initialConnected = true;
         observer.connect(observerCallbacks.proxy(), callbackModel);
         observerConnected = true;
         lateFederate.connect(lateCallbacks.proxy(), callbackModel);
         lateConnected = true;
         initialFederate.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle initialProducer = initialFederate.joinFederationExecution(
            "java-tck-fdd-order-initial", federation);
         initialJoined = true;
         observer.joinFederationExecution("java-tck-fdd-order-observer", federation);
         observerJoined = true;

         initialClass = initialFederate.getObjectClassHandle(objectClassName);
         observerClass = observer.getObjectClassHandle(objectClassName);
         AttributeHandle initialAttribute = initialFederate.getAttributeHandle(
            initialClass, attributeName);
         AttributeHandle observerAttribute = observer.getAttributeHandle(
            observerClass, attributeName);
         JavaTckSupport.check(initialClass != null && observerClass != null
               && initialClass.equals(observerClass) && initialAttribute != null
               && initialAttribute.equals(observerAttribute),
            "FDD order-default fixture returned inconsistent class or attribute handles");
         initialAttributes = initialFederate.getAttributeHandleSetFactory().create();
         initialAttributes.add(initialAttribute);
         observerAttributes = observer.getAttributeHandleSetFactory().create();
         observerAttributes.add(observerAttribute);
         initialFederate.publishObjectClassAttributes(initialClass, initialAttributes);
         initialPublished = true;
         observer.subscribeObjectClassAttributes(observerClass, observerAttributes);
         observerSubscribed = true;

         LogicalTimeFactory<?, ?> initialFactory = initialFederate.getTimeFactory();
         LogicalTimeInterval initialEpsilon =
            (LogicalTimeInterval) initialFactory.makeEpsilon();
         LogicalTime<?, ?> initialTime = initialFederate.queryLogicalTime();
         initialFederate.enableTimeRegulation(initialEpsilon);
         initialRegulating = true;
         await(initialFederate, callbackModel, () -> initialCallbacks.timeRegulationEnabled,
            "initial federate FDD-default time-regulation callback");

         ObjectInstanceHandle initialObject =
            initialFederate.registerObjectInstance(initialClass);
         JavaTckSupport.check(initialObject != null,
            "initial federate could not register its FDD-default object");
         await(observer, callbackModel,
            () -> observerCallbacks.discoveries.contains(initialObject),
            "initial federate FDD-default object discovery");
         sendTimestampedAndObserve(initialFederate, initialProducer, initialTime,
            initialEpsilon, initialObject, initialAttribute, observerAttribute, observer,
            observerCallbacks, callbackModel, 5, 0x51, 0xA1, OrderType.RECEIVE,
            "initial federate FDD RECEIVE default");

         initialFederate.changeDefaultAttributeOrderType(
            initialClass, initialAttributes, OrderType.TIMESTAMP);
         ObjectInstanceHandle changedDefaultObject =
            initialFederate.registerObjectInstance(initialClass);
         JavaTckSupport.check(changedDefaultObject != null,
            "initial federate could not register after changing its class default");
         await(observer, callbackModel,
            () -> observerCallbacks.discoveries.contains(changedDefaultObject),
            "changed-default timestamp object discovery");
         sendTimestampedAndObserve(initialFederate, initialProducer, initialTime,
            initialEpsilon, changedDefaultObject, initialAttribute, observerAttribute, observer,
            observerCallbacks, callbackModel, 6, 0x52, 0xA2, OrderType.TIMESTAMP,
            "changed federate TIMESTAMP default positive control");

         FederateHandle lateProducer = lateFederate.joinFederationExecution(
            "java-tck-fdd-order-late-joiner", federation);
         lateJoined = true;
         lateClass = lateFederate.getObjectClassHandle(objectClassName);
         AttributeHandle lateAttribute = lateFederate.getAttributeHandle(lateClass, attributeName);
         JavaTckSupport.check(lateClass != null && lateClass.equals(initialClass)
               && lateAttribute != null && lateAttribute.equals(initialAttribute),
            "late federate resolved an inconsistent FDD class-attribute identity");
         lateAttributes = lateFederate.getAttributeHandleSetFactory().create();
         lateAttributes.add(lateAttribute);
         lateFederate.publishObjectClassAttributes(lateClass, lateAttributes);
         latePublished = true;
         LogicalTimeFactory<?, ?> lateFactory = lateFederate.getTimeFactory();
         JavaTckSupport.check(initialFactory.getName().equals(lateFactory.getName()),
            "late federate selected a different logical-time factory");
         LogicalTimeInterval lateEpsilon = (LogicalTimeInterval) lateFactory.makeEpsilon();
         LogicalTime<?, ?> lateInitialTime = lateFederate.queryLogicalTime();
         lateFederate.enableTimeRegulation(lateEpsilon);
         lateRegulating = true;
         await(lateFederate, callbackModel, () -> lateCallbacks.timeRegulationEnabled,
            "late federate FDD-default time-regulation callback");
         ObjectInstanceHandle lateObject = lateFederate.registerObjectInstance(lateClass);
         JavaTckSupport.check(lateObject != null,
            "late federate could not register its FDD-default object");
         await(observer, callbackModel, () -> observerCallbacks.discoveries.contains(lateObject),
            "late federate FDD-default object discovery");
         sendTimestampedAndObserve(lateFederate, lateProducer, lateInitialTime,
            lateEpsilon, lateObject, lateAttribute, observerAttribute, observer,
            observerCallbacks, callbackModel, 7, 0x53, 0xA3, OrderType.RECEIVE,
            "late federate retained the FDD RECEIVE default after another federate changed its default");
      } finally {
         if (initialRegulating && initialJoined) {
            try { initialFederate.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (lateRegulating && lateJoined) {
            try { lateFederate.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (observerSubscribed && observerJoined && observerClass != null
               && observerAttributes != null) {
            try { observer.unsubscribeObjectClassAttributes(observerClass, observerAttributes); }
            catch (Exception ignored) { }
         }
         if (latePublished && lateJoined && lateClass != null && lateAttributes != null) {
            try { lateFederate.unpublishObjectClassAttributes(lateClass, lateAttributes); }
            catch (Exception ignored) { }
         }
         if (initialPublished && initialJoined && initialClass != null
               && initialAttributes != null) {
            try { initialFederate.unpublishObjectClassAttributes(initialClass, initialAttributes); }
            catch (Exception ignored) { }
         }
         if (observerJoined) {
            try { observer.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (lateJoined) {
            try {
               lateFederate.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
         }
         if (initialJoined) {
            try {
               initialFederate.resignFederationExecution(
                  ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            } catch (Exception ignored) { }
         }
         if (created) {
            try { initialFederate.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (observerConnected) {
            try { observer.disconnect(); } catch (Exception ignored) { }
         }
         if (lateConnected) {
            try { lateFederate.disconnect(); } catch (Exception ignored) { }
         }
         if (initialConnected) {
            try { initialFederate.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void sendTimestampedAndObserve(RTIambassador producer,
         FederateHandle producerHandle, LogicalTime<?, ?> initialTime,
         LogicalTimeInterval epsilon, ObjectInstanceHandle object,
         AttributeHandle producerAttribute, AttributeHandle observerAttribute,
         RTIambassador observer, Recorder observerCallbacks, CallbackModel callbackModel,
         int timeStep, int value, int tagByte, OrderType expectedSentOrder, String description)
         throws Exception {
      LogicalTime<?, ?> updateTime = offset(initialTime, epsilon, timeStep);
      byte[] valueBytes = {(byte) value};
      byte[] tag = {(byte) tagByte};
      AttributeHandleValueMap values = producer.getAttributeHandleValueMapFactory().create(1);
      values.put(producerAttribute, valueBytes.clone());
      int ordinaryBefore = observerCallbacks.receiveOrder.size();
      int timestampedBefore = observerCallbacks.timestamped.size();
      MessageRetractionReturn retraction = producer.updateAttributeValues(
         object, values, tag.clone(), updateTime);
      boolean timestampOrder = expectedSentOrder == OrderType.TIMESTAMP;
      JavaTckSupport.check(retraction != null
            && retraction.retractionHandleIsValid == timestampOrder
            && (timestampOrder ? retraction.handle != null : retraction.handle == null),
         description + " returned retraction state inconsistent with the FDD/default order");
      await(observer, callbackModel,
         () -> observerCallbacks.receiveOrder.size() > ordinaryBefore
            || observerCallbacks.timestamped.size() > timestampedBefore,
         description + " reflection");

      Reflection delivery = observerCallbacks.timestamped.stream()
         .filter(item -> object.equals(item.object) && Arrays.equals(tag, item.tag))
         .findFirst().orElse(null);
      boolean ordinaryDelivery = observerCallbacks.receiveOrder.stream()
         .anyMatch(item -> object.equals(item.object) && Arrays.equals(tag, item.tag));
      JavaTckSupport.check(observerCallbacks.receiveOrder.size() == ordinaryBefore
            && observerCallbacks.timestamped.size() == timestampedBefore + 1
            && !ordinaryDelivery && delivery != null,
         description + " did not preserve the timestamped-update callback");
      checkDelivery(observer, delivery, object, observerAttribute, producerHandle, updateTime,
         expectedSentOrder, valueBytes, tag,
         timestampOrder ? retraction.handle : null, description);
   }

   private static void checkDelivery(RTIambassador receiver, Reflection delivery,
         ObjectInstanceHandle object, AttributeHandle attribute, FederateHandle producer,
         LogicalTime<?, ?> expectedTime, OrderType expectedSentOrder, byte[] value, byte[] tag,
         MessageRetractionHandle retraction, String description) throws Exception {
      JavaTckSupport.check(object.equals(delivery.object) && delivery.values.size() == 1
            && Arrays.equals(value, delivery.values.get(attribute))
            && Arrays.equals(tag, delivery.tag) && producer.equals(delivery.producer)
            && expectedTime.equals(delivery.time) && delivery.sentOrder == expectedSentOrder
            && delivery.receivedOrder == OrderType.RECEIVE
            && (retraction == null ? delivery.retraction == null
               : retraction.equals(delivery.retraction))
            && delivery.regionsEmpty && delivery.transportation != null
            && !receiver.getTransportationTypeName(delivery.transportation).isEmpty(),
         description + " changed its FDD-selected order or standard message data");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            ambassador.evokeMultipleCallbacks(0.0, 0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regionsEmpty = arguments[5] == null || ((RegionHandleSet) arguments[5]).isEmpty();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class OrdinaryReflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final boolean regionsEmpty;

      private OrdinaryReflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
         producer = (FederateHandle) arguments[4];
         regionsEmpty = arguments[5] == null || ((RegionHandleSet) arguments[5]).isEmpty();
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<OrdinaryReflection> receiveOrder = new CopyOnWriteArrayList<>();
      private final List<Reflection> timestamped = new CopyOnWriteArrayList<>();
      private final List<String> sequence = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;

      private Reflection forAttribute(AttributeHandle handle) {
         return timestamped.stream().filter(item -> item.values.containsKey(handle))
            .findFirst().orElse(null);
      }

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestamped.add(new Reflection(arguments));
                  sequence.add("reflect");
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 6) {
                  receiveOrder.add(new OrdinaryReflection(arguments));
                  sequence.add("reflect");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK attribute FDD preferred-order callbacks";
                  }
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
