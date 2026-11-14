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
import hla.rti1516_2025.RegionHandleSet;
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
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks rate-based suppression of timestamped best-effort attribute passels. */
final class TimestampedAttributeUpdateRateReductionTck {
   private TimestampedAttributeUpdateRateReductionTck() { }

   static void run(RtiFactory factory, String rateFom, String timeImplementation,
         String objectClassName, String reliableAttributeName, String bestEffortAttributeName,
         String updateRateDesignator, CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-rate-reduction-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean published = false;
      boolean subscribed = false;
      boolean publisherRegulating = false;
      boolean receiverConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, rateFom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-rate-reduction-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-rate-reduction-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherReliable =
            publisher.getAttributeHandle(publisherClass, reliableAttributeName);
         AttributeHandle publisherBestEffort =
            publisher.getAttributeHandle(publisherClass, bestEffortAttributeName);
         AttributeHandle receiverReliable =
            receiver.getAttributeHandle(receiverClass, reliableAttributeName);
         AttributeHandle receiverBestEffort =
            receiver.getAttributeHandle(receiverClass, bestEffortAttributeName);
         JavaTckSupport.check(!publisherReliable.equals(publisherBestEffort)
               && !receiverReliable.equals(receiverBestEffort),
            "adapter FOM mapped reliable and best-effort attributes to one handle");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherReliable, publisherBestEffort);
         AttributeHandleSet receiverReliableSet =
            JavaTckSupport.attributeSet(receiver, receiverReliable);
         AttributeHandleSet receiverBestEffortSet =
            JavaTckSupport.attributeSet(receiver, receiverBestEffort);
         AttributeHandleSet receiverAttributes = JavaTckSupport.attributeSet(
            receiver, receiverReliable, receiverBestEffort);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverReliableSet, "");
         receiver.subscribeObjectClassAttributes(receiverClass, receiverBestEffortSet,
            updateRateDesignator);
         subscribed = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(callbackModel, receiver, () -> receiverRecorder.discoveredObjects.contains(object),
            "timestamped update-rate object discovery");

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(
               receiverTimeFactory.getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "publisher and receiver began at different logical times");
         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(callbackModel, receiver,
            () -> receiverRecorder.timeConstrainedEnabled == 1,
            "timeConstrainedEnabled callback");
         publisher.enableTimeRegulation(offset(epsilon, 5));
         publisherRegulating = true;
         await(callbackModel, publisher,
            () -> publisherRecorder.timeRegulationEnabled == 1,
            "timeRegulationEnabled callback");

         double namedRate = receiver.getUpdateRateValue(updateRateDesignator);
         JavaTckSupport.check(namedRate > 0.0
               && receiver.getUpdateRateValueForAttribute(object, receiverBestEffort) == namedRate,
            "active named subscription did not select its positive update rate");
         JavaTckSupport.check(receiver.getUpdateRateValueForAttribute(object, receiverReliable)
               == 0.0,
            "default-rate reliable subscription unexpectedly selected a named rate");

         byte[] reliableBytes = new byte[] {0x71, 0x72};
         byte[] bestEffortBytes = new byte[] {(byte) 0x81, (byte) 0x82};
         byte[] tag = new byte[] {(byte) 0x91};
         AttributeHandleValueMap bothValues =
            publisher.getAttributeHandleValueMapFactory().create(2);
         bothValues.put(publisherReliable, reliableBytes);
         bothValues.put(publisherBestEffort, bestEffortBytes);
         LogicalTime firstTime = add(publisherInitial, offset(epsilon, 6));
         LogicalTime secondTime = add(publisherInitial, offset(epsilon, 8));
         LogicalTime thirdTime = add(publisherInitial, offset(epsilon, 10));
         LogicalTime fourthTime = add(publisherInitial, offset(epsilon, 12));
         LogicalTime fifthTime = add(publisherInitial, offset(epsilon, 14));
         LogicalTime publisherTargetOne = add(publisherInitial, offset(epsilon, 2));
         LogicalTime publisherTargetTwo = add(publisherInitial, offset(epsilon, 4));
         LogicalTime publisherTargetThree = add(publisherInitial, offset(epsilon, 6));
         LogicalTime publisherTargetFour = add(publisherInitial, offset(epsilon, 8));
         LogicalTime publisherTargetFive = add(publisherInitial, offset(epsilon, 10));

         MessageRetractionHandle firstRetraction = update(
            publisher, object, bothValues, tag, firstTime,
            "first timestamped update-rate passel");
         receiver.timeAdvanceRequest(firstTime);
         publisher.timeAdvanceRequest(publisherTargetOne);
         await(callbackModel, publisher, receiver,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() >= 1
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 1
               && receiverRecorder.reflections.size() >= 2,
            "first pair of reflections and time grants");

         JavaTckSupport.check(receiverRecorder.reflections.size() == 2
               && countAttribute(receiverRecorder.reflections, receiverReliable) == 1
               && countAttribute(receiverRecorder.reflections, receiverBestEffort) == 1,
            "first passel did not deliver both attributes exactly once");
         for (Reflection reflection : receiverRecorder.reflections) {
            if (reflection.values.containsKey(receiverReliable)) {
               assertReflection(receiver, reflection, object, receiverReliable, reliableBytes,
                  tag, publisherHandle, firstTime, firstRetraction, "HLAreliable");
            } else {
               assertReflection(receiver, reflection, object, receiverBestEffort,
                  bestEffortBytes, tag, publisherHandle, firstTime, firstRetraction,
                  "HLAbestEffort");
            }
         }

         MessageRetractionHandle secondRetraction = update(
            publisher, object, bothValues, tag, secondTime,
            "second timestamped update-rate passel");
         receiver.timeAdvanceRequest(secondTime);
         publisher.timeAdvanceRequest(publisherTargetTwo);
         await(callbackModel, publisher, receiver,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() >= 2
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 2
               && receiverRecorder.reflections.size() >= 3,
            "second reliable reflection and time grants");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 3
               && countAttribute(receiverRecorder.reflections, receiverReliable) == 2
               && countAttribute(receiverRecorder.reflections, receiverBestEffort) == 1,
            "second passel did not suppress only the rate-limited best-effort attribute");
         Reflection secondReflection = receiverRecorder.reflections.get(2);
         assertReflection(receiver, secondReflection, object, receiverReliable, reliableBytes,
            tag, publisherHandle, secondTime, secondRetraction, "HLAreliable");

         AttributeHandleValueMap bestEffortOnly =
            publisher.getAttributeHandleValueMapFactory().create(1);
         bestEffortOnly.put(publisherBestEffort, bestEffortBytes);
         MessageRetractionHandle suppressedRetraction = update(
            publisher, object, bestEffortOnly, tag, thirdTime,
            "third rate-limited best-effort update");
         publisher.retract(suppressedRetraction);
         receiver.timeAdvanceRequest(thirdTime);
         publisher.timeAdvanceRequest(publisherTargetThree);
         await(callbackModel, publisher, receiver,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() >= 3
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 3,
            "third pair of time-advance grants");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 3,
            "all-best-effort passel was delivered despite the active update-rate limit");
         JavaTckSupport.check(thirdTime.equals(receiverRecorder.timeAdvanceGrantTimes.get(2)),
            "suppressed timestamped update returned the wrong receiver time-advance grant");
         JavaTckSupport.expectFailure(() -> publisher.retract(suppressedRetraction),
            "MessageCanNoLongerBeRetracted",
            "retracting the suppressed passel after its time-advance grant");

         TransportationTypeHandle reliableTransportation =
            publisher.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(reliableTransportation != null,
            "reliable transportation lookup returned no handle");
         AttributeHandleSet changedTransportationAttributes = JavaTckSupport.attributeSet(
            publisher, publisherBestEffort);
         publisher.requestAttributeTransportationTypeChange(
            object, changedTransportationAttributes, reliableTransportation);
         await(callbackModel, publisher,
            () -> publisherRecorder.transportConfirmations.size() >= 1,
            "reliable transportation change confirmation");
         JavaTckSupport.check(publisherRecorder.transportConfirmations.size() == 1,
            "transportation change returned more than one confirmation");
         AttributeTransportConfirmation confirmation =
            publisherRecorder.transportConfirmations.get(0);
         JavaTckSupport.check(object.equals(confirmation.object)
               && confirmation.attributes.size() == 1
               && confirmation.attributes.contains(publisherBestEffort)
               && "HLAreliable".equals(
                  publisher.getTransportationTypeName(confirmation.transportation)),
            "transportation change did not confirm reliable transport for the best-effort attribute");

         MessageRetractionHandle fourthRetraction = update(
            publisher, object, bestEffortOnly, tag, fourthTime,
            "first timestamped update after switching to reliable transportation");
         receiver.timeAdvanceRequest(fourthTime);
         publisher.timeAdvanceRequest(publisherTargetFour);
         await(callbackModel, publisher, receiver,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() >= 4
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 4
               && receiverRecorder.reflections.size() >= 4,
            "first post-change reflection and time grants");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 4
               && countAttribute(receiverRecorder.reflections, receiverReliable) == 2
               && countAttribute(receiverRecorder.reflections, receiverBestEffort) == 2,
            "first post-change update was not delivered exactly once with reliable transport");
         assertReflection(receiver, receiverRecorder.reflections.get(3), object,
            receiverBestEffort, bestEffortBytes, tag, publisherHandle, fourthTime,
            fourthRetraction, "HLAreliable");

         MessageRetractionHandle fifthRetraction = update(
            publisher, object, bestEffortOnly, tag, fifthTime,
            "second timestamped update after switching to reliable transportation");
         receiver.timeAdvanceRequest(fifthTime);
         publisher.timeAdvanceRequest(publisherTargetFive);
         await(callbackModel, publisher, receiver,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() >= 5
               && receiverRecorder.timeAdvanceGrantTimes.size() >= 5
               && receiverRecorder.reflections.size() >= 5,
            "second post-change reflection and time grants");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 5
               && countAttribute(receiverRecorder.reflections, receiverReliable) == 2
               && countAttribute(receiverRecorder.reflections, receiverBestEffort) == 3,
            "second post-change update was suppressed after switching to reliable transport");
         assertReflection(receiver, receiverRecorder.reflections.get(4), object,
            receiverBestEffort, bestEffortBytes, tag, publisherHandle, fifthTime,
            fifthRetraction, "HLAreliable");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
         receiver.disconnect();
         receiverConnected = false;
         publisher.disconnect();
         publisherConnected = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               ObjectClassHandle objectClass = receiver.getObjectClassHandle(objectClassName);
               AttributeHandle reliable = receiver.getAttributeHandle(
                  objectClass, reliableAttributeName);
               AttributeHandle bestEffort = receiver.getAttributeHandle(
                  objectClass, bestEffortAttributeName);
               receiver.unsubscribeObjectClassAttributes(
                  objectClass, JavaTckSupport.attributeSet(receiver, reliable, bestEffort));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               ObjectClassHandle objectClass = publisher.getObjectClassHandle(objectClassName);
               AttributeHandle reliable = publisher.getAttributeHandle(
                  objectClass, reliableAttributeName);
               AttributeHandle bestEffort = publisher.getAttributeHandle(
                  objectClass, bestEffortAttributeName);
               publisher.unpublishObjectClassAttributes(
                  objectClass, JavaTckSupport.attributeSet(publisher, reliable, bestEffort));
            } catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try {
               publisher.resignFederationExecution(
                  ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
            }
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

   private static MessageRetractionHandle update(RTIambassador publisher,
         ObjectInstanceHandle object, AttributeHandleValueMap values, byte[] tag,
         LogicalTime time, String description) throws Exception {
      MessageRetractionReturn result = publisher.updateAttributeValues(object, values, tag, time);
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned an invalid retraction handle");
      return result.handle;
   }

   private static int countAttribute(List<Reflection> reflections, AttributeHandle attribute) {
      int count = 0;
      for (Reflection reflection : reflections) {
         if (reflection.values.containsKey(attribute)) count++;
      }
      return count;
   }

   private static void assertReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] expectedValue,
         byte[] expectedTag, FederateHandle producer, LogicalTime expectedTime,
         MessageRetractionHandle retraction, String transportationName) throws Exception {
      JavaTckSupport.check(reflection.object.equals(object) && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(attribute)),
         "timestamped update-rate reflection returned the wrong object, attribute, or value");
      JavaTckSupport.check(reflection.regions == null || reflection.regions.isEmpty(),
         "timestamped update-rate reflection unexpectedly contained regions");
      JavaTckSupport.check(Arrays.equals(expectedTag, reflection.tag)
            && producer.equals(reflection.producer) && expectedTime.equals(reflection.time),
         "timestamped update-rate reflection changed tag, producer, or timestamp");
      JavaTckSupport.check(reflection.transportation != null
            && transportationName.equals(
               receiver.getTransportationTypeName(reflection.transportation)),
         "timestamped update-rate reflection returned the wrong transportation type");
      JavaTckSupport.check(reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction),
         "timestamped update-rate reflection returned incorrect order or retraction metadata");
   }

   private static LogicalTime add(LogicalTime start, LogicalTimeInterval interval)
         throws Exception {
      return (LogicalTime) start.add(interval);
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTimeInterval result = interval;
      for (int i = 1; i < steps; ++i) {
         result = (LogicalTimeInterval) result.add(interval);
      }
      return result;
   }

   private static void await(CallbackModel callbackModel, RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void await(CallbackModel callbackModel, RTIambassador publisher,
         RTIambassador receiver, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(publisher);
            JavaTckSupport.drain(receiver);
         } else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private RegionHandleSet regions;
      private MessageRetractionHandle retraction;
   }

   private static final class AttributeTransportConfirmation {
      private final ObjectInstanceHandle object;
      private final AttributeHandleSet attributes;
      private final TransportationTypeHandle transportation;

      private AttributeTransportConfirmation(ObjectInstanceHandle object,
            AttributeHandleSet attributes, TransportationTypeHandle transportation) {
         this.object = object;
         this.attributes = attributes.clone();
         this.transportation = transportation;
      }
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private final List<ObjectInstanceHandle> discoveredObjects = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();
      private final List<AttributeTransportConfirmation> transportConfirmations =
         new CopyOnWriteArrayList<>();

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
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regions = (RegionHandleSet) arguments[5];
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("confirmAttributeTransportationTypeChange".equals(name)
                     && arguments != null && arguments.length == 3) {
                  transportConfirmations.add(new AttributeTransportConfirmation(
                     (ObjectInstanceHandle) arguments[0], (AttributeHandleSet) arguments[1],
                     (TransportationTypeHandle) arguments[2]));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK rate-reduction callbacks";
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
