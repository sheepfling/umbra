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
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.RegionHandleSet;
import hla.rti1516_2025.TransportationTypeHandle;
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
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador subscriber = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      Recorder subscriberRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean immediateConnected = false;
      boolean subscriberConnected = false;
      boolean publisherJoined = false;
      boolean immediateJoined = false;
      boolean subscriberJoined = false;
      boolean created = false;
      boolean regulationEnabled = false;
      boolean constrainedEnabled = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         immediate.connect(immediateRecorder.proxy(), callbackModel);
         immediateConnected = true;
         subscriber.connect(subscriberRecorder.proxy(), callbackModel);
         subscriberConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-tso-publisher", federation);
         publisherJoined = true;
         immediate.joinFederationExecution("java-tck-tso-immediate", federation);
         immediateJoined = true;
         subscriber.joinFederationExecution("java-tck-tso-subscriber", federation);
         subscriberJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle immediateClass = immediate.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscriberClass = subscriber.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle immediateAttribute = immediate.getAttributeHandle(
            immediateClass, attributeName);
         AttributeHandle subscriberAttribute = subscriber.getAttributeHandle(
            subscriberClass, attributeName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet immediateSubscribed =
            immediate.getAttributeHandleSetFactory().create();
         immediateSubscribed.add(immediateAttribute);
         AttributeHandleSet subscribed = subscriber.getAttributeHandleSetFactory().create();
         subscribed.add(subscriberAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         immediate.subscribeObjectClassAttributes(immediateClass, immediateSubscribed);
         subscriber.subscribeObjectClassAttributes(subscriberClass, subscribed);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(callbackModel, () -> object.equals(immediateRecorder.discoveredObject),
            immediate, "immediate timestamped attribute discovery");
         await(callbackModel, () -> object.equals(subscriberRecorder.discoveredObject),
            subscriber, "constrained timestamped attribute discovery");
         check(subscriberRecorder.discoveredObject != null
               && object.equals(subscriberRecorder.discoveredObject),
            "subscriber did not discover the registered object before timestamped update");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         check(timeFactory.getName().equals(immediate.getTimeFactory().getName())
               && timeFactory.getName().equals(subscriber.getTimeFactory().getName()),
            "timestamped attribute members selected different logical-time factories");
         LogicalTimeInterval lookahead = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(lookahead);
         regulationEnabled = true;
         await(callbackModel, () -> publisherRecorder.timeRegulationEnabled,
            publisher, "publisher time-regulation callback");
         check(publisherRecorder.timeRegulationEnabled,
            "timeRegulationEnabled did not cross the standard callback interface");
         subscriber.enableTimeConstrained();
         constrainedEnabled = true;
         await(callbackModel, () -> subscriberRecorder.timeConstrainedEnabled,
            subscriber, "subscriber time-constrained callback");
         check(subscriberRecorder.timeConstrainedEnabled,
            "timeConstrainedEnabled did not cross the standard callback interface");

         LogicalTime current = publisher.queryLogicalTime();
         LogicalTime subscriberInitialTime = subscriber.queryLogicalTime();
         LogicalTime timestamp = addSteps(current, lookahead, 3);
         byte[] payload = new byte[] {0x15, 0x16, 0x25, (byte) 0xff};
         byte[] tag = new byte[] {0x51, 0x52};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag, timestamp);
         check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Update Attribute Values returned no public retraction designator");

         await(callbackModel, () -> immediateRecorder.timestampedReflections == 1,
            immediate, "immediate timestamped attribute reflection");
         drain(subscriber);
         check(object.equals(immediateRecorder.reflectedObject)
               && immediateRecorder.reflectedValues != null
               && immediateRecorder.reflectedValues.size() == 1
               && Arrays.equals(payload,
                  immediateRecorder.reflectedValues.get(immediateAttribute))
               && Arrays.equals(tag, immediateRecorder.reflectedTag)
               && producer.equals(immediateRecorder.producingFederate)
               && timestamp.equals(immediateRecorder.reflectedTime)
               && immediateRecorder.sentOrder == OrderType.TIMESTAMP
               && immediateRecorder.receivedOrder == OrderType.RECEIVE
               && retraction.handle.equals(immediateRecorder.reflectedRetraction),
            "immediate timestamped reflection returned incorrect standard callback metadata");
         check(immediateRecorder.transportation != null
               && !immediate.getTransportationTypeName(immediateRecorder.transportation).isEmpty()
               && immediateRecorder.regions != null && immediateRecorder.regions.isEmpty(),
            "immediate reflection returned invalid transportation or region metadata");
         check(subscriberRecorder.timestampedReflections == 0,
            "timestamped reflection was delivered before the constrained subscriber requested an advance");
         check(subscriberInitialTime.equals(subscriber.queryLogicalTime()),
            "constrained subscriber logical time changed before its advance request");

         publisher.retract(retraction.handle);
         await(callbackModel, () -> immediateRecorder.retractionCallbacks.size() == 1,
            immediate, "requestRetraction for delivered timestamped reflection");
         check(retraction.handle.equals(immediateRecorder.retractionCallbacks.get(0)),
            "requestRetraction returned the wrong message retraction handle");
         check(java.util.Arrays.asList("reflectAttributeValues", "requestRetraction")
               .equals(immediateRecorder.callbackSequence),
            "requestRetraction did not follow delivery of the timestamped reflection");
         drain(subscriber);
         check(subscriberRecorder.retractionCallbacks.isEmpty(),
            "retracting a queued timestamped reflection produced a retraction callback");
         JavaTckSupport.expectFailure(() -> publisher.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the already retracted timestamped attribute update");

         subscriber.timeAdvanceRequest(timestamp);
         publisher.timeAdvanceRequest(timestamp);
         drain(subscriber);
         drain(publisher);
         check(timestamp.equals(publisherRecorder.timeAdvanceGrant),
            "publisher did not receive the grant for the cancelled message timestamp");
         check(subscriberRecorder.timestampedReflections == 0,
            "retracted timestamped reflection was delivered at the constrained grant");
         check(subscriberRecorder.retractionCallbacks.isEmpty(),
            "retracted queued timestamped reflection produced a stale callback");
         check(timestamp.equals(subscriberRecorder.timeAdvanceGrant)
               && timestamp.equals(subscriber.queryLogicalTime()),
            "subscriber did not receive the grant for the cancelled message timestamp");
         subscriberRecorder.callbackSequence.clear();

         LogicalTime nextTimestamp = addSteps(timestamp, lookahead, 3);
         LogicalTime nextSubscriberTimestamp =
            addSteps(subscriber.queryLogicalTime(), lookahead, 3);
         check(nextTimestamp.equals(nextSubscriberTimestamp),
            "publisher and constrained subscriber derived different advance targets");
         values = publisher.getAttributeHandleValueMapFactory().create(1);
         byte[] nextPayload = new byte[] {0x21, 0x22, 0x23};
         byte[] nextTag = new byte[] {0x31, 0x32};
         values.put(publisherAttribute, nextPayload);
         MessageRetractionReturn deliveredRetraction = publisher.updateAttributeValues(
            object, values, nextTag, nextTimestamp);
         check(deliveredRetraction != null && deliveredRetraction.retractionHandleIsValid
               && deliveredRetraction.handle != null,
            "second timestamped update returned an invalid retraction designator");
         await(callbackModel, () -> immediateRecorder.timestampedReflections == 2,
            immediate, "second immediate timestamped attribute reflection");
         check(immediateRecorder.reflectedValues != null
               && Arrays.equals(nextPayload,
                  immediateRecorder.reflectedValues.get(immediateAttribute))
               && Arrays.equals(nextTag, immediateRecorder.reflectedTag)
               && nextTimestamp.equals(immediateRecorder.reflectedTime)
               && producer.equals(immediateRecorder.producingFederate)
               && immediateRecorder.sentOrder == OrderType.TIMESTAMP
               && immediateRecorder.receivedOrder == OrderType.RECEIVE
               && deliveredRetraction.handle.equals(immediateRecorder.reflectedRetraction),
            "second immediate timestamped reflection returned incorrect message data");
         check(immediateRecorder.transportation != null
               && !immediate.getTransportationTypeName(immediateRecorder.transportation).isEmpty()
               && immediateRecorder.regions != null && immediateRecorder.regions.isEmpty(),
            "second immediate reflection returned invalid transportation or region metadata");
         drain(subscriber);
         check(subscriberRecorder.timestampedReflections == 0,
            "constrained subscriber received the second timestamped reflection before advancing");
         subscriber.timeAdvanceRequest(nextSubscriberTimestamp);
         publisher.timeAdvanceRequest(nextTimestamp);
         drain(subscriber);
         drain(publisher);
         check(nextTimestamp.equals(publisherRecorder.timeAdvanceGrant),
            "publisher did not receive the grant for the delivered message timestamp");
         check(subscriberRecorder.timestampedReflections == 1,
            "time-constrained subscriber did not receive exactly one non-retracted reflection");
         check(object.equals(subscriberRecorder.reflectedObject),
            "timestamped reflection identified the wrong object instance");
         check(subscriberRecorder.reflectedValues != null
               && subscriberRecorder.reflectedValues.size() == 1
               && Arrays.equals(nextPayload,
                  subscriberRecorder.reflectedValues.get(subscriberAttribute)),
            "timestamped reflection did not preserve its attribute payload");
         check(Arrays.equals(nextTag, subscriberRecorder.reflectedTag),
            "timestamped reflection did not preserve its user tag");
         check(nextTimestamp.equals(subscriberRecorder.reflectedTime),
            "timestamped reflection did not preserve its logical timestamp");
         check(subscriberRecorder.sentOrder == OrderType.TIMESTAMP
               && subscriberRecorder.receivedOrder == OrderType.TIMESTAMP,
            "time-constrained reflection did not report timestamp order at both boundaries");
         check(deliveredRetraction.handle.equals(subscriberRecorder.reflectedRetraction),
            "timestamped reflection returned the wrong message retraction handle");
         check(subscriberRecorder.transportation != null
               && !subscriber.getTransportationTypeName(subscriberRecorder.transportation).isEmpty()
               && subscriberRecorder.regions != null && subscriberRecorder.regions.isEmpty()
               && subscriberRecorder.reflectedTime.toString() != null
               && !subscriberRecorder.reflectedTime.toString().isEmpty(),
            "constrained reflection returned invalid transportation, region, or time metadata");
         check(producer.equals(subscriberRecorder.producingFederate),
            "timestamped reflection identified the wrong producing federate");
         check(nextSubscriberTimestamp.equals(subscriberRecorder.timeAdvanceGrant),
            "timeAdvanceGrant did not report the requested logical time");
         check(nextSubscriberTimestamp.equals(subscriber.queryLogicalTime()),
            "subscriber logical time did not reach the granted timestamp");
         check(subscriberRecorder.callbackSequence.size() >= 2
               && "reflectAttributeValues".equals(subscriberRecorder.callbackSequence.get(0))
               && "timeAdvanceGrant".equals(subscriberRecorder.callbackSequence.get(1)),
            "queued timestamped reflection was not delivered before its matching grant");

         JavaTckSupport.expectFailure(() -> publisher.retract(deliveredRetraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a timestamped attribute update after its delivery grant");

         publisher.deleteObjectInstance(object, new byte[0]);
         await(callbackModel, () -> immediateRecorder.removedObjects.contains(object),
            immediate, "immediate object removal before invalid-object update boundary");
         await(callbackModel, () -> subscriberRecorder.removedObjects.contains(object),
            subscriber, "constrained object removal before invalid-object update boundary");
         AttributeHandleValueMap removedObjectValues =
            publisher.getAttributeHandleValueMapFactory().create(1);
         removedObjectValues.put(publisherAttribute, new byte[0]);
         LogicalTime afterRemovalTime = (LogicalTime) nextTimestamp.add(lookahead);
         JavaTckSupport.expectFailure(
            () -> publisher.updateAttributeValues(
               object, removedObjectValues, new byte[0], afterRemovalTime),
            "ObjectInstanceNotKnown", "timestamped update for a removed object instance");

         subscriber.disableTimeConstrained();
         constrainedEnabled = false;
         publisher.disableTimeRegulation();
         regulationEnabled = false;
         subscriber.unsubscribeObjectClassAttributes(subscriberClass, subscribed);
         subscriber.unsubscribeObjectClass(subscriberClass);
         immediate.unsubscribeObjectClassAttributes(immediateClass, immediateSubscribed);
         immediate.unsubscribeObjectClass(immediateClass);
         publisher.unpublishObjectClassAttributes(publisherClass, published);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (constrainedEnabled && subscriberJoined) {
            try { subscriber.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulationEnabled && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscriberJoined) {
            try { subscriber.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (immediateJoined) {
            try { immediate.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (immediateConnected) {
            try { immediate.disconnect(); } catch (Exception ignored) { }
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

   private static LogicalTime addSteps(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(CallbackModel model, java.util.function.BooleanSupplier condition,
         RTIambassador ambassador, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) drain(ambassador);
         else Thread.yield();
      }
      check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void check(boolean condition, String message) {
      if (!condition) {
         throw new AssertionError(message);
      }
   }

   private static final class Recorder {
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;
      private volatile int timestampedReflections;
      private volatile ObjectInstanceHandle reflectedObject;
      private volatile AttributeHandleValueMap reflectedValues;
      private volatile byte[] reflectedTag;
      private volatile LogicalTime<?, ?> reflectedTime;
      private volatile OrderType sentOrder;
      private volatile OrderType receivedOrder;
      private volatile MessageRetractionHandle reflectedRetraction;
      private volatile TransportationTypeHandle transportation;
      private volatile RegionHandleSet regions;
      private volatile FederateHandle producingFederate;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private final java.util.List<ObjectInstanceHandle> removedObjects =
         new java.util.concurrent.CopyOnWriteArrayList<>();
      private final java.util.List<MessageRetractionHandle> retractionCallbacks =
         new java.util.concurrent.CopyOnWriteArrayList<>();
      private final java.util.List<String> callbackSequence =
         new java.util.concurrent.CopyOnWriteArrayList<>();

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
                  transportation = (TransportationTypeHandle) arguments[3];
                  producingFederate = (FederateHandle) arguments[4];
                  regions = (RegionHandleSet) arguments[5];
                  reflectedTime = (LogicalTime<?, ?>) arguments[6];
                  sentOrder = (OrderType) arguments[7];
                  receivedOrder = (OrderType) arguments[8];
                  reflectedRetraction = (MessageRetractionHandle) arguments[9];
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackSequence.add(name);
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) {
                  retractionCallbacks.add((MessageRetractionHandle) arguments[0]);
                  callbackSequence.add(name);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  removedObjects.add((ObjectInstanceHandle) arguments[0]);
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
