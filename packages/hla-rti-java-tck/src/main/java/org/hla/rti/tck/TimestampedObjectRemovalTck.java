package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.UUID;

/** Portable IEEE 1516.1-2025 timestamped object-removal checks. */
final class TimestampedObjectRemovalTck {
   private TimestampedObjectRemovalTck() {
   }

   static void deliver(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-timestamped-object-removal-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean subscriberConnected = false;
      boolean publisherJoined = false;
      boolean subscriberJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-removal-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-removal-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = subscriber.getAttributeHandleSetFactory().create();
         subscribed.add(subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribed);

         ObjectInstanceHandle receiveOrderObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle timestampedObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle timeAdvanceReleasedObject =
            publisher.registerObjectInstance(publisherClass);
         drain(subscriber);
         check(subscriberRecorder.discoveredObjects.contains(receiveOrderObject)
               && subscriberRecorder.discoveredObjects.contains(timestampedObject)
               && subscriberRecorder.discoveredObjects.contains(timeAdvanceReleasedObject),
            "subscriber did not discover all objects before deletion");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         drain(publisher);
         check(publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled did not cross the standard callback interface");
         subscriber.enableTimeConstrained();
         drain(subscriber);
         check(subscriberRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled did not cross the standard callback interface");

         byte[] receiveOrderTag = new byte[] {0x51, 0x52};
         publisher.deleteObjectInstance(receiveOrderObject, receiveOrderTag);
         drain(subscriber);
         check(subscriberRecorder.receiveOrderRemovals == 0
               && subscriberRecorder.timestampedRemovals == 0,
            "receive-order removal was delivered while asynchronous delivery was disabled");
         subscriber.enableAsynchronousDelivery();
         drain(subscriber);
         check(subscriberRecorder.receiveOrderRemovals == 1
               && subscriberRecorder.timestampedRemovals == 0,
            "enableAsynchronousDelivery did not release exactly the receive-order removal");
         check(receiveOrderObject.equals(subscriberRecorder.receiveOrderObject)
               && Arrays.equals(receiveOrderTag, subscriberRecorder.receiveOrderTag)
               && producer.equals(subscriberRecorder.receiveOrderProducer),
            "receive-order removal did not preserve object, tag, and producer metadata");

         subscriber.disableAsynchronousDelivery();
         byte[] timeAdvanceReleasedTag = new byte[] {0x71, 0x72};
         publisher.deleteObjectInstance(timeAdvanceReleasedObject, timeAdvanceReleasedTag);
         drain(subscriber);
         check(subscriberRecorder.receiveOrderRemovals == 1
               && subscriberRecorder.timestampedRemovals == 0,
            "disabling asynchronous delivery did not retain receive-order removal");

         LogicalTime current = publisher.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) current.add(lookahead);
         byte[] timestampedTag = new byte[] {0x61, 0x62};
         MessageRetractionReturn retraction = publisher.deleteObjectInstance(
            timestampedObject, timestampedTag, timestamp);
         check(retraction != null && retraction.retractionHandleIsValid,
            "timestamped Delete Object Instance returned no valid retraction designator");

         subscriberRecorder.callbackSequence.clear();
         subscriber.timeAdvanceRequest(timestamp);
         drain(subscriber);
         check(subscriberRecorder.receiveOrderRemovals == 2
               && subscriberRecorder.timestampedRemovals == 1,
            "time advance did not release the held receive-order and timestamped removals");
         check(timeAdvanceReleasedObject.equals(subscriberRecorder.receiveOrderObject)
               && Arrays.equals(timeAdvanceReleasedTag, subscriberRecorder.receiveOrderTag)
               && producer.equals(subscriberRecorder.receiveOrderProducer),
            "time-advance-released removal did not preserve object, tag, and producer metadata");
         check(timestampedObject.equals(subscriberRecorder.timestampedObject),
            "timestamped removal identified the wrong object instance");
         check(Arrays.equals(timestampedTag, subscriberRecorder.timestampedTag),
            "timestamped removal did not preserve its user tag");
         check(producer.equals(subscriberRecorder.timestampedProducer),
            "timestamped removal identified the wrong producing federate");
         check(timestamp.equals(subscriberRecorder.removalTime),
            "timestamped removal did not preserve its logical timestamp");
         check(subscriberRecorder.sentOrder == OrderType.TIMESTAMP
               && subscriberRecorder.receivedOrder == OrderType.TIMESTAMP,
            "timestamped removal did not report timestamp order at both boundaries");
         check(retraction.handle.equals(subscriberRecorder.retractionHandle),
            "timestamped removal did not preserve its retraction handle");
         check(timestamp.equals(subscriberRecorder.timeAdvanceGrant),
            "timeAdvanceGrant did not report the requested logical time");
         check(subscriberRecorder.callbackSequence.size() == 2
               && "removeObjectInstance".equals(subscriberRecorder.callbackSequence.get(0))
               && "timeAdvanceGrant".equals(subscriberRecorder.callbackSequence.get(1)),
            "timestamped removal was not delivered before its matching grant");

         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribed);
         subscriber.unsubscribeObjectClass(subscriberClass);
         publisher.unpublishObjectClassAttributes(publisherClass, published);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (subscriberConnected) {
            try { subscriber.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static void drain(RTIambassador ambassador) throws Exception {
      for (int i = 0; i < 64; ++i) {
         ambassador.evokeCallback(0.0);
      }
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static final class Recorder {
      private final java.util.Set<ObjectInstanceHandle> discoveredObjects =
         new java.util.HashSet<>();
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private int receiveOrderRemovals;
      private ObjectInstanceHandle receiveOrderObject;
      private byte[] receiveOrderTag;
      private FederateHandle receiveOrderProducer;
      private int timestampedRemovals;
      private ObjectInstanceHandle timestampedObject;
      private byte[] timestampedTag;
      private FederateHandle timestampedProducer;
      private LogicalTime<?, ?> removalTime;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private hla.rti1516_2025.MessageRetractionHandle retractionHandle;
      private LogicalTime<?, ?> timeAdvanceGrant;
      private final java.util.List<String> callbackSequence = new java.util.ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObjects.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 3) {
                  receiveOrderRemovals++;
                  receiveOrderObject = (ObjectInstanceHandle) arguments[0];
                  receiveOrderTag = ((byte[]) arguments[1]).clone();
                  receiveOrderProducer = (FederateHandle) arguments[2];
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  timestampedRemovals++;
                  callbackSequence.add(name);
                  timestampedObject = (ObjectInstanceHandle) arguments[0];
                  timestampedTag = ((byte[]) arguments[1]).clone();
                  timestampedProducer = (FederateHandle) arguments[2];
                  removalTime = (LogicalTime<?, ?>) arguments[3];
                  sentOrder = (OrderType) arguments[4];
                  receivedOrder = (OrderType) arguments[5];
                  retractionHandle = (hla.rti1516_2025.MessageRetractionHandle) arguments[6];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK object-removal callbacks";
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
