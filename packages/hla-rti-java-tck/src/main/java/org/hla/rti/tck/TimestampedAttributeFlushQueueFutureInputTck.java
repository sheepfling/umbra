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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Portable delivery of future timestamped attribute updates through Flush Queue. */
final class TimestampedAttributeFlushQueueFutureInputTck {
   private TimestampedAttributeFlushQueueFutureInputTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-flush-future-input-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean created = false;
      boolean regulating = false;
      boolean constrained = false;
      ObjectClassHandle publisherClass = null;
      AttributeHandleSet publisherAttributes = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-flush-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-attribute-flush-receiver", federation);
         receiverJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         publisherAttributes = publisher.getAttributeHandleSetFactory().create();
         publisherAttributes.add(publisherAttribute);
         receiverAttributes = receiver.getAttributeHandleSetFactory().create();
         receiverAttributes.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(object.equals(receiverRecorder.discoveredObject),
            "receiver did not discover the registered object");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiver.getTimeFactory().getName()),
            "timestamped attribute members selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained();
         constrained = true;
         JavaTckSupport.drain(receiver);
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled,
            "time-managed role callbacks were not delivered through the API");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime receiverInitial = receiver.queryLogicalTime();
         LogicalTime firstTime = offset(publisherInitial, epsilon, 9);
         LogicalTime secondTime = offset(publisherInitial, epsilon, 12);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 4);
         LogicalTime requestBoundary = offset(receiverInitial, epsilon, 10);
         byte[] firstValue = new byte[] {0x41, 0x54, 0x54, 0x52, 0x2d, 0x46, 0x51, 0x31};
         byte[] secondValue = new byte[] {0x41, 0x54, 0x54, 0x52, 0x2d, 0x46, 0x51, 0x32};
         byte[] firstTag = new byte[] {0x46, 0x51, 0x41, 0x31};
         byte[] secondTag = new byte[] {0x46, 0x51, 0x41, 0x32};
         MessageRetractionReturn first = update(publisher, object, publisherAttribute, firstValue,
            firstTag, firstTime);
         MessageRetractionReturn second = update(publisher, object, publisherAttribute, secondValue,
            secondTag, secondTime);
         checkRetraction(first, "first future attribute update");
         checkRetraction(second, "second future attribute update");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
            "future timestamped reflections arrived before Flush Queue Request");

         receiver.flushQueueRequest(requestBoundary);
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.check(receiverRecorder.reflections.isEmpty(),
               "future timestamped reflections arrived before callback servicing");
         }
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);

         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrantTimes.get(0)),
            "publisher received the wrong time-advance grant");
         JavaTckSupport.check(receiverRecorder.reflections.size() == 2
               && receiverRecorder.flushQueueGrantTimes.size() == 1
               && receiverRecorder.timeAdvanceGrantTimes.isEmpty(),
            "Flush Queue Request returned the wrong reflection or grant callback counts");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(Arrays.asList(
               "reflectAttributeValues", "reflectAttributeValues", "flushQueueGrant")),
            "Flush Queue Request granted before both queued reflections");
         checkReflection(receiver, receiverRecorder.reflections.get(0), object, receiverAttribute,
            firstValue, firstTag, producer, firstTime, first.handle, "first future reflection");
         checkReflection(receiver, receiverRecorder.reflections.get(1), object, receiverAttribute,
            secondValue, secondTag, producer, secondTime, second.handle, "second future reflection");

         LogicalTime flushGrant = receiverRecorder.flushQueueGrantTime;
         LogicalTime optimistic = receiverRecorder.flushQueueOptimisticTime;
         JavaTckSupport.check(flushGrant != null && optimistic != null
               && firstTime.equals(optimistic)
               && compareTime(flushGrant, receiverInitial) >= 0
               && compareTime(flushGrant, requestBoundary) <= 0
               && compareTime(optimistic, flushGrant) >= 0
               && flushGrant.equals(receiver.queryLogicalTime()),
            "Flush Queue Grant/query returned an inconsistent actual or optimistic frontier");
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverJoined && receiverClass != null && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherJoined && publisherClass != null && publisherAttributes != null) {
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

   private static MessageRetractionReturn update(RTIambassador publisher,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value);
      return publisher.updateAttributeValues(object, values, tag, time);
   }

   private static void checkRetraction(MessageRetractionReturn result, String description) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, description + " returned no valid retraction handle");
   }

   private static void checkReflection(RTIambassador receiver, Reflection reflection,
         ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
         byte[] expectedValue, byte[] expectedTag, FederateHandle expectedProducer,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         String description) throws Exception {
      JavaTckSupport.check(expectedObject.equals(reflection.object)
            && reflection.values != null && reflection.values.size() == 1
            && Arrays.equals(expectedValue, reflection.values.get(expectedAttribute)),
         description + " returned the wrong object or attribute payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, reflection.tag),
         description + " did not preserve its user tag");
      JavaTckSupport.check(expectedProducer.equals(reflection.producer)
            && expectedTime.equals(reflection.time),
         description + " returned the wrong producer or logical timestamp");
      JavaTckSupport.check(reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP,
         description + " returned the wrong order metadata");
      JavaTckSupport.check(reflection.regions == null || reflection.regions.isEmpty(),
         description + " unexpectedly included region metadata");
      JavaTckSupport.check(expectedRetraction.equals(reflection.retraction)
            && reflection.transportation != null
            && !receiver.getTransportationTypeName(reflection.transportation).isEmpty(),
         description + " did not preserve retraction or resolvable transportation metadata");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval,
         int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
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
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueGrantTime;
      private LogicalTime<?, ?> flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
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
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK future attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
