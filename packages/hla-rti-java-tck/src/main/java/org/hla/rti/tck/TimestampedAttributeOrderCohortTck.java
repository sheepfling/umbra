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

/** Verifies timestamp ordering and complete equal-time cohorts for two recipients. */
final class TimestampedAttributeOrderCohortTck {
   private TimestampedAttributeOrderCohortTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador firstReceiver = factory.getRtiAmbassador();
      RTIambassador secondReceiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-order-cohort-" + UUID.randomUUID();
      boolean publisherConnected = false, firstConnected = false, secondConnected = false;
      boolean created = false, publisherJoined = false, firstJoined = false, secondJoined = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         firstReceiver.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         secondReceiver.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-order-cohort-publisher", federation);
         publisherJoined = true;
         firstReceiver.joinFederationExecution("java-tck-order-cohort-first", federation);
         firstJoined = true;
         secondReceiver.joinFederationExecution("java-tck-order-cohort-second", federation);
         secondJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle firstClass = firstReceiver.getObjectClassHandle(objectClassName);
         ObjectClassHandle secondClass = secondReceiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle firstAttribute = firstReceiver.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = secondReceiver.getAttributeHandle(secondClass, attributeName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet firstSubscribed = firstReceiver.getAttributeHandleSetFactory().create();
         firstSubscribed.add(firstAttribute);
         AttributeHandleSet secondSubscribed = secondReceiver.getAttributeHandleSetFactory().create();
         secondSubscribed.add(secondAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         firstReceiver.subscribeObjectClassAttributes(firstClass, firstSubscribed);
         secondReceiver.subscribeObjectClassAttributes(secondClass, secondSubscribed);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         drainAll(publisher, firstReceiver, secondReceiver);
         JavaTckSupport.check(object.equals(firstRecorder.discoveredObject)
               && object.equals(secondRecorder.discoveredObject),
            "both constrained recipients must discover the published object");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> firstFactory = firstReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = secondReceiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(firstFactory.getName())
               && publisherFactory.getName().equals(secondFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval publisherEpsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval firstEpsilon = (LogicalTimeInterval) firstFactory.makeEpsilon();
         LogicalTimeInterval secondEpsilon = (LogicalTimeInterval) secondFactory.makeEpsilon();
         firstReceiver.enableTimeConstrained();
         JavaTckSupport.drain(firstReceiver);
         secondReceiver.enableTimeConstrained();
         JavaTckSupport.drain(secondReceiver);
         publisher.enableTimeRegulation(publisherEpsilon);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(firstRecorder.timeConstrainedEnabled
               && secondRecorder.timeConstrainedEnabled && publisherRecorder.timeRegulationEnabled,
            "time constrained/regulation callbacks were not delivered");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime firstInitial = firstReceiver.queryLogicalTime();
         LogicalTime secondInitial = secondReceiver.queryLogicalTime();
         LogicalTime timeFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime timeSeven = offset(publisherInitial, publisherEpsilon, 7);
         LogicalTime firstFive = offset(firstInitial, firstEpsilon, 5);
         LogicalTime firstSeven = offset(firstInitial, firstEpsilon, 7);
         LogicalTime secondFive = offset(secondInitial, secondEpsilon, 5);
         LogicalTime secondSeven = offset(secondInitial, secondEpsilon, 7);
         JavaTckSupport.check(timeFive.equals(firstFive) && timeFive.equals(secondFive)
               && timeSeven.equals(firstSeven) && timeSeven.equals(secondSeven),
            "federates did not derive matching logical timestamps");

         byte[] earlyValue = new byte[] {0x41, 0x56, 0x2d, 0x35};
         byte[] equalValue = new byte[] {0x41, 0x56, 0x2d, 0x35, 0x45};
         byte[] lateValue = new byte[] {0x41, 0x56, 0x2d, 0x37};
         byte[] earlyTag = new byte[] {0x54, 0x41};
         byte[] equalTag = new byte[] {0x54, 0x45};
         byte[] lateTag = new byte[] {0x54, 0x37};
         MessageRetractionReturn late = update(
            publisher, object, publisherAttribute, lateValue, lateTag, timeSeven);
         MessageRetractionReturn early = update(
            publisher, object, publisherAttribute, earlyValue, earlyTag, timeFive);
         MessageRetractionReturn equal = update(
            publisher, object, publisherAttribute, equalValue, equalTag, timeFive);
         JavaTckSupport.check(valid(late) && valid(early) && valid(equal),
            "timestamped updates must return valid retraction handles");
         drainAll(publisher, firstReceiver, secondReceiver);
         JavaTckSupport.check(firstRecorder.reflections.isEmpty()
               && secondRecorder.reflections.isEmpty(),
            "timestamped reflections arrived before either recipient requested an advance");

         firstRecorder.callbackOrder.clear();
         secondRecorder.callbackOrder.clear();
         firstReceiver.timeAdvanceRequest(firstFive);
         secondReceiver.timeAdvanceRequest(secondFive);
         publisher.timeAdvanceRequest(offset(publisherInitial, publisherEpsilon, 4));
         drainAll(publisher, firstReceiver, secondReceiver);
         checkFirstBoundary(firstRecorder, firstFive, "first recipient");
         checkFirstBoundary(secondRecorder, secondFive, "second recipient");
         assertCohort(firstRecorder.reflections, firstAttribute, object, producer,
            timeFive, earlyValue, earlyTag, early.handle, equalValue, equalTag, equal.handle,
            firstReceiver, "first recipient");
         assertCohort(secondRecorder.reflections, secondAttribute, object, producer,
            timeFive, earlyValue, earlyTag, early.handle, equalValue, equalTag, equal.handle,
            secondReceiver, "second recipient");

         firstReceiver.timeAdvanceRequest(firstSeven);
         secondReceiver.timeAdvanceRequest(secondSeven);
         publisher.timeAdvanceRequest(offset(publisherInitial, publisherEpsilon, 6));
         drainAll(publisher, firstReceiver, secondReceiver);
         checkSecondBoundary(firstRecorder, firstSeven, "first recipient");
         checkSecondBoundary(secondRecorder, secondSeven, "second recipient");
         assertLate(firstRecorder.reflections.get(2), firstAttribute, object, producer,
            timeSeven, lateValue, lateTag, late.handle, firstReceiver, "first recipient");
         assertLate(secondRecorder.reflections.get(2), secondAttribute, object, producer,
            timeSeven, lateValue, lateTag, late.handle, secondReceiver, "second recipient");

         firstReceiver.disableTimeConstrained();
         secondReceiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         drainAll(publisher, firstReceiver, secondReceiver);
         firstReceiver.unsubscribeObjectClassAttributes(firstClass, firstSubscribed);
         secondReceiver.unsubscribeObjectClassAttributes(secondClass, secondSubscribed);
         publisher.unpublishObjectClassAttributes(publisherClass, published);
      } finally {
         if (secondJoined) {
            try { secondReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { firstReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (secondConnected) {
            try { secondReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (firstConnected) {
            try { firstReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static MessageRetractionReturn update(RTIambassador ambassador,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = ambassador.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      return ambassador.updateAttributeValues(object, values, tag, time);
   }

   private static boolean valid(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void drainAll(RTIambassador publisher, RTIambassador first,
         RTIambassador second) throws Exception {
      JavaTckSupport.drain(publisher);
      JavaTckSupport.drain(first);
      JavaTckSupport.drain(second);
   }

   private static void checkFirstBoundary(Recorder recorder, LogicalTime grant, String who) {
      JavaTckSupport.check(recorder.reflections.size() == 2
            && recorder.timeAdvanceGrantTimes.size() == 1
            && Arrays.asList("reflectAttributeValues", "reflectAttributeValues", "timeAdvanceGrant")
               .equals(recorder.callbackOrder)
            && grant.equals(recorder.timeAdvanceGrantTimes.get(0)),
         who + " did not receive the complete equal-time cohort before its matching grant");
   }

   private static void checkSecondBoundary(Recorder recorder, LogicalTime grant, String who) {
      JavaTckSupport.check(recorder.reflections.size() == 3
            && recorder.timeAdvanceGrantTimes.size() == 2
            && Arrays.asList("reflectAttributeValues", "reflectAttributeValues", "timeAdvanceGrant",
                  "reflectAttributeValues", "timeAdvanceGrant").equals(recorder.callbackOrder)
            && grant.equals(recorder.timeAdvanceGrantTimes.get(1)),
         who + " did not receive the later timestamped update before its matching grant");
   }

   private static void assertCohort(List<Reflection> reflections, AttributeHandle attribute,
         ObjectInstanceHandle object, FederateHandle producer, LogicalTime time,
         byte[] earlyValue, byte[] earlyTag, MessageRetractionHandle earlyRetraction,
         byte[] equalValue, byte[] equalTag, MessageRetractionHandle equalRetraction,
         RTIambassador inspector, String who) throws Exception {
      Reflection first = reflections.get(0);
      Reflection second = reflections.get(1);
      boolean earlyFirst = Arrays.equals(earlyValue, first.values.get(attribute))
         && Arrays.equals(earlyTag, first.tag)
         && Arrays.equals(equalValue, second.values.get(attribute))
         && Arrays.equals(equalTag, second.tag);
      boolean equalFirst = Arrays.equals(equalValue, first.values.get(attribute))
         && Arrays.equals(equalTag, first.tag)
         && Arrays.equals(earlyValue, second.values.get(attribute))
         && Arrays.equals(earlyTag, second.tag);
      JavaTckSupport.check(earlyFirst || equalFirst,
         who + " did not preserve both members of the equal-timestamp cohort");
      assertCommon(first, attribute, object, producer, time,
         earlyFirst ? earlyRetraction : equalRetraction, inspector, who);
      assertCommon(second, attribute, object, producer, time,
         earlyFirst ? equalRetraction : earlyRetraction, inspector, who);
   }

   private static void assertLate(Reflection reflection, AttributeHandle attribute,
         ObjectInstanceHandle object, FederateHandle producer, LogicalTime time,
         byte[] value, byte[] tag, MessageRetractionHandle retraction,
         RTIambassador inspector, String who) throws Exception {
      JavaTckSupport.check(Arrays.equals(value, reflection.values.get(attribute))
            && Arrays.equals(tag, reflection.tag), who + " received the wrong later update");
      assertCommon(reflection, attribute, object, producer, time, retraction, inspector, who);
   }

   private static void assertCommon(Reflection reflection, AttributeHandle attribute,
         ObjectInstanceHandle object, FederateHandle producer, LogicalTime time,
         MessageRetractionHandle retraction, RTIambassador inspector, String who)
         throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && reflection.values.containsKey(attribute) && time.equals(reflection.time),
         who + " received the wrong object, attribute, or timestamp");
      JavaTckSupport.check(producer.equals(reflection.producer)
            && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(reflection.retraction),
         who + " received incorrect producer, order, or retraction metadata");
      JavaTckSupport.check(reflection.transportation != null
            && !inspector.getTransportationTypeName(reflection.transportation).isEmpty()
            && reflection.regionsEmpty,
         who + " received unresolved transport metadata or unexpected regions");
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

   private static final class Recorder {
      private ObjectInstanceHandle discoveredObject;
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

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
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection reflection = new Reflection();
                  reflection.object = (ObjectInstanceHandle) arguments[0];
                  reflection.values = ((AttributeHandleValueMap) arguments[1]).clone();
                  reflection.tag = ((byte[]) arguments[2]).clone();
                  reflection.transportation = (TransportationTypeHandle) arguments[3];
                  reflection.producer = (FederateHandle) arguments[4];
                  reflection.regionsEmpty = arguments[5] == null
                     || ((RegionHandleSet) arguments[5]).isEmpty();
                  reflection.time = (LogicalTime<?, ?>) arguments[6];
                  reflection.sentOrder = (OrderType) arguments[7];
                  reflection.receivedOrder = (OrderType) arguments[8];
                  reflection.retraction = (MessageRetractionHandle) arguments[9];
                  reflections.add(reflection);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK timestamped attribute cohort callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
