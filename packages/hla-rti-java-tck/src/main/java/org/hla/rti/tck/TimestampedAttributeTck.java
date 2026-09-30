package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
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

/** Portable IEEE 1516.1-2025 timestamped attribute delivery checks. */
final class TimestampedAttributeTck {
   private TimestampedAttributeTck() {
   }

   static void deliver(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-" + UUID.randomUUID();
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
            "java-tck-tso-publisher", federation);
         publisherJoined = true;
         subscriber.joinFederationExecution("java-tck-tso-subscriber", federation);
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
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribed);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         drain(subscriber);
         check(subscriberRecorder.discoveredObject != null
               && object.equals(subscriberRecorder.discoveredObject),
            "subscriber did not discover the registered object before timestamped update");

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

         LogicalTime current = publisher.queryLogicalTime();
         LogicalTime subscriberInitialTime = subscriber.queryLogicalTime();
         LogicalTime timestamp = (LogicalTime) current.add(lookahead);
         byte[] payload = new byte[] {0x15, 0x16, 0x25, (byte) 0xff};
         byte[] tag = new byte[] {0x51, 0x52};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag, timestamp);
         check(retraction != null,
            "timestamped Update Attribute Values returned no public retraction designator");

         drain(subscriber);
         check(subscriberRecorder.timestampedReflections == 0,
            "timestamped reflection was delivered before the constrained subscriber requested an advance");
         check(subscriberInitialTime.equals(subscriber.queryLogicalTime()),
            "constrained subscriber logical time changed before its advance request");

         subscriber.timeAdvanceRequest(timestamp);
         drain(subscriber);
         check(subscriberRecorder.timestampedReflections == 1,
            "time-constrained subscriber did not receive exactly one timestamped reflection");
         check(object.equals(subscriberRecorder.reflectedObject),
            "timestamped reflection identified the wrong object instance");
         check(subscriberRecorder.reflectedValues != null
               && subscriberRecorder.reflectedValues.size() == 1
               && Arrays.equals(payload, subscriberRecorder.reflectedValues.get(subscriberAttribute)),
            "timestamped reflection did not preserve its attribute payload");
         check(Arrays.equals(tag, subscriberRecorder.reflectedTag),
            "timestamped reflection did not preserve its user tag");
         check(timestamp.equals(subscriberRecorder.reflectedTime),
            "timestamped reflection did not preserve its logical timestamp");
         check(subscriberRecorder.sentOrder == OrderType.TIMESTAMP
               && subscriberRecorder.receivedOrder == OrderType.TIMESTAMP,
            "time-constrained reflection did not report timestamp order at both boundaries");
         check(producer.equals(subscriberRecorder.producingFederate),
            "timestamped reflection identified the wrong producing federate");
         check(timestamp.equals(subscriberRecorder.timeAdvanceGrant),
            "timeAdvanceGrant did not report the requested logical time");
         check(timestamp.equals(subscriber.queryLogicalTime()),
            "subscriber logical time did not reach the granted timestamp");
         check(subscriberRecorder.callbackSequence.size() >= 2
               && "reflectAttributeValues".equals(subscriberRecorder.callbackSequence.get(0))
               && "timeAdvanceGrant".equals(subscriberRecorder.callbackSequence.get(1)),
            "queued timestamped reflection was not delivered before its matching grant");

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
      private ObjectInstanceHandle discoveredObject;
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private int timestampedReflections;
      private ObjectInstanceHandle reflectedObject;
      private AttributeHandleValueMap reflectedValues;
      private byte[] reflectedTag;
      private LogicalTime<?, ?> reflectedTime;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private FederateHandle producingFederate;
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
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestampedReflections++;
                  callbackSequence.add(name);
                  reflectedObject = (ObjectInstanceHandle) arguments[0];
                  reflectedValues = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflectedTag = ((byte[]) arguments[2]).clone();
                  producingFederate = (FederateHandle) arguments[4];
                  reflectedTime = (LogicalTime<?, ?>) arguments[6];
                  sentOrder = (OrderType) arguments[7];
                  receivedOrder = (OrderType) arguments[8];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped callbacks";
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
