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
         JavaTckSupport.check(receiverCallbacks.receiveOrder.isEmpty()
               && receiverCallbacks.timestamped.size() == 2
               && timestampDelivery != null && receiveDelivery != null,
            "FDD-preferred updates did not produce exactly two timestamp-bearing reflections");
         checkDelivery(receiver, timestampDelivery, object, receiverTimestampAttribute, producer,
            timestampTime, OrderType.TIMESTAMP, timestampValue, timestampTag,
            timestampRetraction.handle, "timestamp-preferred object attribute");
         checkDelivery(receiver, receiveDelivery, object, receiverReceiveAttribute, producer,
            receiveTime, OrderType.RECEIVE, receiveValue, receiveTag, null,
            "receive-preferred object attribute");
         JavaTckSupport.check(receiverCallbacks.sequence.size() == 2
               && receiverCallbacks.sequence.stream().allMatch("reflect"::equals),
            "FDD-preferred updates did not produce exactly two reflection callbacks");
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
         regionsEmpty = ((RegionHandleSet) arguments[5]).isEmpty();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Boolean> receiveOrder = new CopyOnWriteArrayList<>();
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
                  receiveOrder.add(Boolean.TRUE);
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
