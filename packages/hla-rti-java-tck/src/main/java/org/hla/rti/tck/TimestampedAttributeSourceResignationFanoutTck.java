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
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Fans one queued timestamped attribute update out after publisher resignation. */
final class TimestampedAttributeSourceResignationFanoutTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedAttributeSourceResignationFanoutTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador first = factory.getRtiAmbassador();
      RTIambassador second = factory.getRtiAmbassador();
      RTIambassador clock = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder firstRecorder = new Recorder();
      Recorder secondRecorder = new Recorder();
      Recorder clockRecorder = new Recorder();
      String federation = "java-tck-attribute-resignation-fanout-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean firstConnected = false;
      boolean secondConnected = false;
      boolean clockConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean firstJoined = false;
      boolean secondJoined = false;
      boolean clockJoined = false;
      boolean firstSubscribed = false;
      boolean secondSubscribed = false;
      boolean publisherRegulating = false;
      boolean firstConstrained = false;
      boolean secondConstrained = false;
      boolean clockRegulating = false;
      ObjectClassHandle firstClass = null;
      ObjectClassHandle secondClass = null;
      AttributeHandleSet firstAttributes = null;
      AttributeHandleSet secondAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         first.connect(firstRecorder.proxy(), callbackModel);
         firstConnected = true;
         second.connect(secondRecorder.proxy(), callbackModel);
         secondConnected = true;
         clock.connect(clockRecorder.proxy(), callbackModel);
         clockConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-resignation-fanout-publisher", federation);
         publisherJoined = true;
         first.joinFederationExecution("java-tck-attribute-resignation-fanout-first", federation);
         firstJoined = true;
         second.joinFederationExecution("java-tck-attribute-resignation-fanout-second", federation);
         secondJoined = true;
         clock.joinFederationExecution("java-tck-attribute-resignation-fanout-clock", federation);
         clockJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         firstClass = first.getObjectClassHandle(objectClassName);
         secondClass = second.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle firstAttribute = first.getAttributeHandle(firstClass, attributeName);
         AttributeHandle secondAttribute = second.getAttributeHandle(secondClass, attributeName);
         JavaTckSupport.check(publisherClass != null && firstClass != null && secondClass != null
               && publisherAttribute != null && firstAttribute != null && secondAttribute != null,
            "timestamped attribute fan-out FOM lookup returned a null handle");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         firstAttributes = JavaTckSupport.attributeSet(first, firstAttribute);
         secondAttributes = JavaTckSupport.attributeSet(second, secondAttribute);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         first.subscribeObjectClassAttributes(firstClass, firstAttributes);
         firstSubscribed = true;
         second.subscribeObjectClassAttributes(secondClass, secondAttributes);
         secondSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(first, callbackModel, () -> firstRecorder.discovered.contains(object),
            "first recipient object discovery");
         await(second, callbackModel, () -> secondRecorder.discovered.contains(object),
            "second recipient object discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> firstFactory = first.getTimeFactory();
         LogicalTimeFactory<?, ?> secondFactory = second.getTimeFactory();
         LogicalTimeFactory<?, ?> clockFactory = clock.getTimeFactory();
         String factoryName = publisherFactory.getName();
         JavaTckSupport.check(factoryName.equals(firstFactory.getName())
               && factoryName.equals(secondFactory.getName())
               && factoryName.equals(clockFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTimeInterval publisherLookahead = intervalOffset(
            (LogicalTimeInterval) publisherFactory.makeEpsilon(), 5);
         LogicalTimeInterval clockLookahead = intervalOffset(
            (LogicalTimeInterval) clockFactory.makeEpsilon(), 5);

         first.enableTimeConstrained();
         firstConstrained = true;
         await(first, callbackModel, () -> firstRecorder.timeConstrainedEnabledCount == 1,
            "first recipient Time Constrained enable");
         second.enableTimeConstrained();
         secondConstrained = true;
         await(second, callbackModel, () -> secondRecorder.timeConstrainedEnabledCount == 1,
            "second recipient Time Constrained enable");
         publisher.enableTimeRegulation(publisherLookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher Time Regulation enable");
         clock.enableTimeRegulation(clockLookahead);
         clockRegulating = true;
         await(clock, callbackModel, () -> clockRecorder.timeRegulationEnabledCount == 1,
            "clock Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(first.queryLogicalTime())
               && initial.equals(second.queryLogicalTime())
               && initial.equals(clock.queryLogicalTime()),
            "federates began at different logical times");
         LogicalTime messageTime = offset(initial, publisherFactory.makeEpsilon(), 7);
         LogicalTime firstTarget = offset(initial, firstFactory.makeEpsilon(), 7);
         LogicalTime secondTarget = offset(initial, secondFactory.makeEpsilon(), 10);
         LogicalTime clockTarget = offset(initial, clockFactory.makeEpsilon(), 2);
         byte[] value = new byte[] {(byte) 0xa7, 0x71, 0x52};
         byte[] tag = new byte[] {0x52, 0x45, 0x53, 0x2d, 0x46, 0x41, 0x4e};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         MessageRetractionReturn sent = publisher.updateAttributeValues(
            object, values, tag, messageTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid && sent.handle != null,
            "timestamped Update Attribute Values returned no valid retraction handle");
         MessageRetractionHandle retraction = sent.handle;
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         JavaTckSupport.check(firstRecorder.reflections.isEmpty()
               && secondRecorder.reflections.isEmpty(),
            "timestamped update arrived before either recipient frontier");

         first.timeAdvanceRequest(firstTarget);
         second.nextMessageRequest(secondTarget);
         JavaTckSupport.check(firstRecorder.timeAdvanceGrantTimes.isEmpty()
               && secondRecorder.timeAdvanceGrantTimes.isEmpty(),
            "recipient advanced before the producer frontier");
         publisher.resignFederationExecution(ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES);
         publisherJoined = false;
         publisherRegulating = false;
         JavaTckSupport.expectFailure(() -> publisher.retract(retraction),
            "FederateNotExecutionMember", "retracting through the resigned publisher");
         JavaTckSupport.check(firstRecorder.reflections.isEmpty()
               && secondRecorder.reflections.isEmpty()
               && firstRecorder.timeAdvanceGrantTimes.isEmpty()
               && secondRecorder.timeAdvanceGrantTimes.isEmpty(),
            "queued reflections were released before recipients became safe to advance");

         firstRecorder.callbackOrder.clear();
         secondRecorder.callbackOrder.clear();
         clock.timeAdvanceRequest(clockTarget);
         await(clock, callbackModel, () -> clockRecorder.timeAdvanceGrantTimes.size() == 1,
            "regulating clock frontier grant");
         await(first, callbackModel, () -> firstRecorder.reflections.size() == 1
               && firstRecorder.timeAdvanceGrantTimes.size() == 1,
            "first recipient reflection and grant");
         await(second, callbackModel, () -> secondRecorder.reflections.size() == 1
               && secondRecorder.timeAdvanceGrantTimes.size() == 1,
            "second recipient reflection and grant");

         checkReflection(first, firstRecorder, object, firstAttribute, value, tag, producer,
            messageTime, firstTarget, retraction);
         checkReflection(second, secondRecorder, object, secondAttribute, value, tag, producer,
            messageTime, messageTime, retraction);

         first.disableTimeConstrained();
         firstConstrained = false;
         second.disableTimeConstrained();
         secondConstrained = false;
         clock.disableTimeRegulation();
         clockRegulating = false;
      } finally {
         if (firstConstrained && firstJoined) {
            try { first.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (secondConstrained && secondJoined) {
            try { second.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (clockRegulating && clockJoined) {
            try { clock.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (firstSubscribed && firstJoined) {
            try { first.unsubscribeObjectClassAttributes(firstClass, firstAttributes); }
            catch (Exception ignored) { }
         }
         if (secondSubscribed && secondJoined) {
            try { second.unsubscribeObjectClassAttributes(secondClass, secondAttributes); }
            catch (Exception ignored) { }
         }
         if (firstJoined) {
            try { first.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (secondJoined) {
            try { second.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (clockJoined) {
            try { clock.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(
               ResignAction.UNCONDITIONALLY_DIVEST_ATTRIBUTES); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (clockConnected) { try { clock.disconnect(); } catch (Exception ignored) { } }
         if (secondConnected) { try { second.disconnect(); } catch (Exception ignored) { } }
         if (firstConnected) { try { first.disconnect(); } catch (Exception ignored) { } }
         if (publisherConnected) { try { publisher.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkReflection(RTIambassador receiver, Recorder recorder,
         ObjectInstanceHandle expectedObject, AttributeHandle expectedAttribute,
         byte[] expectedValue, byte[] expectedTag, FederateHandle producer,
         LogicalTime<?, ?> messageTime, LogicalTime<?, ?> expectedGrant,
         MessageRetractionHandle retraction) throws Exception {
      JavaTckSupport.check(recorder.reflections.size() == 1,
         "recipient received a duplicate or missing timestamped reflection");
      Reflection actual = recorder.reflections.get(0);
      JavaTckSupport.check(expectedObject.equals(actual.object) && actual.values.size() == 1
            && Arrays.equals(expectedValue, actual.values.get(expectedAttribute)),
         "recipient received the wrong object, attribute, or value");
      JavaTckSupport.check(Arrays.equals(expectedTag, actual.tag)
            && producer.equals(actual.producer) && messageTime.equals(actual.time),
         "reflection did not preserve tag, producer, or timestamp");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == OrderType.TIMESTAMP
            && retraction.equals(actual.retraction),
         "reflection returned incorrect order or retraction metadata");
      JavaTckSupport.check(actual.transportation != null
            && !receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         "reflection returned an unresolvable transportation handle");
      JavaTckSupport.check(actual.regions == null || actual.regions.isEmpty(),
         "ordinary timestamped attribute update unexpectedly carried source regions");
      JavaTckSupport.check(recorder.timeAdvanceGrantTimes.size() == 1
            && expectedGrant.equals(recorder.timeAdvanceGrantTimes.get(0)),
         "recipient was granted an unexpected logical time");
      JavaTckSupport.check(recorder.callbackOrder.equals(
            Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
         "recipient received its reflection after the advance grant");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval start, int steps)
         throws Exception {
      LogicalTimeInterval result = start;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(start);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) JavaTckSupport.drain(ambassador);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
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
      private volatile LogicalTime<?, ?> timeAdvanceGrant;
      private volatile ObjectInstanceHandle discoveredObject;
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discovered = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new CopyOnWriteArrayList<>();

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
                  discovered.add(discoveredObject);
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
                  timeAdvanceGrantTimes.add(timeAdvanceGrant);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK attribute source-resignation fan-out callbacks";
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
