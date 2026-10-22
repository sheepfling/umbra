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

/** Preserves a queued timestamped attribute update across Time Regulation re-enable. */
final class TimestampedAttributeRegulationReenableTck {
   private TimestampedAttributeRegulationReenableTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-regulation-reenable-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      boolean published = false;
      boolean subscribed = false;
      ObjectClassHandle publisherClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet publisherAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-regulation-reenable-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-attribute-regulation-reenable-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         awaitPair(publisher, receiver,
            () -> object.equals(receiverRecorder.discoveredObject),
            "object discovery");
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "receiver did not discover the registered object");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped attribute members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         awaitPair(publisher, receiver,
            () -> receiverRecorder.timeConstrainedEnabledCount == 1,
            "initial Time Constrained callback");
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report its initial Time Constrained enable callback");
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         awaitPair(publisher, receiver,
            () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "initial Time Regulation callback");
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report its initial Time Regulation enable callback");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "publisher and receiver began at different logical times");
         LogicalTime updateTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         LogicalTimeInterval changedLookahead = offset(epsilon, 3);
         byte[] value = new byte[] {(byte) 0xb7, 0x2c, (byte) 0x91};
         byte[] tag = new byte[] {0x43, 0x48, 0x47, 0x41};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag, updateTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped Update Attribute Values returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "timestamped reflection arrived before its constrained grant");

         publisher.disableTimeRegulation();
         regulating = false;
         publisher.enableTimeRegulation(changedLookahead);
         regulating = true;
         awaitPair(publisher, receiver,
            () -> publisherRecorder.timeRegulationEnabledCount == 2,
            "second Time Regulation callback");
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 2,
            "publisher did not report its second Time Regulation enable callback");
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "queued reflection was delivered during Time Regulation re-enable");
         JavaTckSupport.check(changedLookahead.equals(publisher.queryLookahead()),
            "Query Lookahead did not return the changed lookahead");

         receiverRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(updateTime);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "queued reflection was delivered while the producer boundary was pending");
         publisher.timeAdvanceRequest(publisherTarget);
         try {
            awaitPair(publisher, receiver,
               () -> receiverRecorder.reflections.size() == 1
                  && receiverRecorder.timeAdvanceGrant != null
                  && publisherRecorder.timeAdvanceGrant != null,
               "queued reflection and grants");
         } catch (AssertionError timeout) {
            throw new AssertionError(timeout.getMessage()
               + " (reflections=" + receiverRecorder.reflections.size()
               + ", receiver grant=" + (receiverRecorder.timeAdvanceGrant != null)
               + ", publisher grant=" + (publisherRecorder.timeAdvanceGrant != null)
               + ")", timeout);
         }
         JavaTckSupport.check(receiverRecorder.reflections.size() == 1
               && publisherRecorder.timeRegulationEnabledCount == 2,
            "re-enabled producer did not deliver exactly one timestamped reflection");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "queued reflection was delivered after the receiver's grant");
         JavaTckSupport.check(updateTime.equals(receiverRecorder.timeAdvanceGrant)
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrant)
               && updateTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "time-advance grants or logical-time queries returned the wrong times");
         checkReflection(receiver, receiverRecorder.reflections.get(0), object,
            receiverAttribute, value, tag, producer, updateTime, sent.handle);
         JavaTckSupport.expectFailure(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the timestamped attribute update after delivery");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
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

   private static void checkReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
         byte[] expectedValue, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction)
         throws Exception {
      JavaTckSupport.check(expectedObject.equals(reflection.object)
            && reflection.values != null && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(expectedAttribute)),
         "timestamped regulation re-enable returned the wrong object or value");
      JavaTckSupport.check(Arrays.equals(expectedTag, reflection.tag)
            && expectedTime.equals(reflection.time),
         "timestamped regulation re-enable did not preserve tag or time");
      JavaTckSupport.check(expectedProducer.equals(reflection.producer)
            && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP,
         "timestamped regulation re-enable returned the wrong producer or order");
      JavaTckSupport.check(reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         "timestamped regulation re-enable returned an unresolvable transportation handle");
      JavaTckSupport.check(reflection.regions == null || reflection.regions.isEmpty(),
         "ordinary timestamped update unexpectedly included region metadata");
      JavaTckSupport.check(expectedRetraction.equals(reflection.retraction),
         "timestamped regulation re-enable returned the wrong retraction handle");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval offset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void awaitPair(RTIambassador publisher, RTIambassador receiver,
         BooleanSupplier observed, String phase) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         if (observed.getAsBoolean()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(phase + " did not complete within 10 seconds");
   }

   private static final class Reflection {
      private ObjectInstanceHandle object;
      private AttributeHandleValueMap values;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private RegionHandleSet regions;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabledCount;
      private volatile int timeConstrainedEnabledCount;
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabledCount++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabledCount++;
               } else if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
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
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK attribute regulation re-enable callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
