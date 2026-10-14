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

/** Suppresses only a locally deleted recipient's queued timestamped attribute reflection. */
final class TimestampedLocalDeleteAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedLocalDeleteAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador local = factory.getRtiAmbassador();
      RTIambassador surviving = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder localRecorder = new Recorder();
      Recorder survivingRecorder = new Recorder();
      String federation = "java-tck-timestamped-local-delete-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean localConnected = false;
      boolean survivingConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean localJoined = false;
      boolean survivingJoined = false;
      boolean publisherPublished = false;
      boolean localSubscribed = false;
      boolean survivingSubscribed = false;
      boolean publisherRegulating = false;
      boolean localConstrained = false;
      boolean survivingConstrained = false;
      ObjectClassHandle localClass = null;
      ObjectClassHandle survivingClass = null;
      AttributeHandleSet localAttributes = null;
      AttributeHandleSet survivingAttributes = null;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         local.connect(localRecorder.proxy(), callbackModel);
         localConnected = true;
         surviving.connect(survivingRecorder.proxy(), callbackModel);
         survivingConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-local-delete-attribute-publisher", federation);
         publisherJoined = true;
         local.joinFederationExecution("java-tck-local-delete-attribute-local", federation);
         localJoined = true;
         surviving.joinFederationExecution(
            "java-tck-local-delete-attribute-survivor", federation);
         survivingJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         localClass = local.getObjectClassHandle(objectClassName);
         survivingClass = surviving.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle localAttribute = local.getAttributeHandle(localClass, attributeName);
         AttributeHandle survivingAttribute = surviving.getAttributeHandle(
            survivingClass, attributeName);
         JavaTckSupport.check(publisherClass != null && localClass != null
               && survivingClass != null && publisherAttribute != null
               && localAttribute != null && survivingAttribute != null,
            "timestamped local-delete FOM lookup returned a null handle");
         AttributeHandleSet publisherAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         localAttributes = JavaTckSupport.attributeSet(local, localAttribute);
         survivingAttributes = JavaTckSupport.attributeSet(surviving, survivingAttribute);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherPublished = true;
         publisher.changeDefaultAttributeOrderType(
            publisherClass, publisherAttributes, OrderType.TIMESTAMP);
         local.subscribeObjectClassAttributes(localClass, localAttributes);
         localSubscribed = true;
         surviving.subscribeObjectClassAttributes(survivingClass, survivingAttributes);
         survivingSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "timestamped local-delete registration returned a null object handle");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "timestamped local-delete registration returned an empty object name");
         await(local, callbackModel, () -> localRecorder.discoveries.contains(object),
            "local recipient object discovery");
         await(surviving, callbackModel, () -> survivingRecorder.discoveries.contains(object),
            "surviving recipient object discovery");
         JavaTckSupport.check(object.equals(local.getObjectInstanceHandle(objectName))
               && object.equals(surviving.getObjectInstanceHandle(objectName)),
            "object discovery changed the registered object identity");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> localFactory = local.getTimeFactory();
         LogicalTimeFactory<?, ?> survivingFactory = surviving.getTimeFactory();
         String timeFactoryName = publisherFactory.getName();
         JavaTckSupport.check(timeFactoryName.equals(localFactory.getName())
               && timeFactoryName.equals(survivingFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime localInitial = local.queryLogicalTime();
         LogicalTime survivingInitial = surviving.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(localInitial)
               && publisherInitial.equals(survivingInitial),
            "federates began at different logical times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);

         local.enableTimeConstrained();
         localConstrained = true;
         await(local, callbackModel, () -> localRecorder.timeConstrainedEnabledCount == 1,
            "local recipient Time Constrained enable");
         surviving.enableTimeConstrained();
         survivingConstrained = true;
         await(surviving, callbackModel,
            () -> survivingRecorder.timeConstrainedEnabledCount == 1,
            "surviving recipient Time Constrained enable");
         publisher.enableTimeRegulation(lookahead);
         publisherRegulating = true;
         await(publisher, callbackModel, () -> publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher Time Regulation enable");

         LogicalTime firstTime = offset(publisherInitial, epsilon, 6);
         LogicalTime localFirstTarget = offset(localInitial, epsilon, 6);
         LogicalTime survivingFirstTarget = offset(survivingInitial, epsilon, 6);
         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         byte[] value = new byte[] {0x5a, 0x31};
         byte[] tag = new byte[] {(byte) 0xd2, 0x31};
         AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value);
         MessageRetractionReturn firstSent = publisher.updateAttributeValues(
            object, values, tag, firstTime);
         JavaTckSupport.check(firstSent != null && firstSent.retractionHandleIsValid
               && firstSent.handle != null,
            "first timestamped update returned no valid retraction handle");
         JavaTckSupport.check(localRecorder.reflections.isEmpty()
               && survivingRecorder.reflections.isEmpty(),
            "first timestamped update arrived before either recipient frontier");

         local.localDeleteObjectInstance(object);
         JavaTckSupport.expectFailure(() -> local.getKnownObjectClassHandle(object),
            "ObjectInstanceNotKnown", "known-object query after local deletion");
         localRecorder.callbackOrder.clear();
         survivingRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         local.timeAdvanceRequest(localFirstTarget);
         surviving.timeAdvanceRequest(survivingFirstTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitSessions(new RTIambassador[] {publisher, local, surviving}, callbackModel,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && localRecorder.timeAdvanceGrantTimes.size() == 1
               && survivingRecorder.reflections.size() == 1
               && survivingRecorder.timeAdvanceGrantTimes.size() == 1,
            "first timestamped delivery and grants");

         JavaTckSupport.check(localRecorder.reflections.isEmpty()
               && localRecorder.timeAdvanceGrantTimes.size() == 1
               && localFirstTarget.equals(localRecorder.timeAdvanceGrantTimes.get(0)),
            "local deletion did not suppress only the local queued reflection");
         JavaTckSupport.check(localRecorder.callbackOrder.equals(
               Arrays.asList("timeAdvanceGrant")),
            "local deletion delivered a stale local reflection callback");
         checkReflection(surviving, survivingRecorder, object, survivingAttribute, value, tag,
            producer, firstTime, OrderType.TIMESTAMP, firstSent.handle,
            survivingFirstTarget, 1, 1);
         JavaTckSupport.expectFailure(() -> local.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "object name query after local deletion");
         JavaTckSupport.check(object.equals(surviving.getObjectInstanceHandle(objectName)),
            "local deletion changed the surviving recipient's object identity");

         localRecorder.discoveries.clear();
         local.subscribeObjectClassAttributes(localClass, localAttributes);
         await(local, callbackModel, () -> localRecorder.discoveries.contains(object),
            "rediscovery after local deletion");
         JavaTckSupport.check(object.equals(local.getObjectInstanceHandle(objectName)),
            "rediscovery after local deletion changed object identity");

         LogicalTime secondTime = offset(publisherInitial, epsilon, 7);
         LogicalTime localSecondTarget = offset(localInitial, epsilon, 7);
         LogicalTime survivingSecondTarget = offset(survivingInitial, epsilon, 7);
         AttributeHandleValueMap secondValues = publisher.getAttributeHandleValueMapFactory()
            .create(1);
         secondValues.put(publisherAttribute, value);
         MessageRetractionReturn secondSent = publisher.updateAttributeValues(
            object, secondValues, tag, secondTime);
         JavaTckSupport.check(secondSent != null && secondSent.retractionHandleIsValid
               && secondSent.handle != null,
            "second timestamped update returned no valid retraction handle");

         localRecorder.callbackOrder.clear();
         survivingRecorder.callbackOrder.clear();
         publisherRecorder.callbackOrder.clear();
         local.timeAdvanceRequest(localSecondTarget);
         surviving.timeAdvanceRequest(survivingSecondTarget);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitSessions(new RTIambassador[] {publisher, local, surviving}, callbackModel,
            () -> publisherRecorder.timeAdvanceGrantTimes.size() == 2
               && localRecorder.reflections.size() == 1
               && localRecorder.timeAdvanceGrantTimes.size() == 2
               && survivingRecorder.reflections.size() == 2
               && survivingRecorder.timeAdvanceGrantTimes.size() == 2,
            "second timestamped delivery and grants");

         checkReflection(local, localRecorder, object, localAttribute, value, tag, producer,
            secondTime, OrderType.TIMESTAMP, secondSent.handle, localSecondTarget, 1, 2);
         checkReflection(surviving, survivingRecorder, object, survivingAttribute, value, tag,
            producer, secondTime, OrderType.TIMESTAMP, secondSent.handle,
            survivingSecondTarget, 2, 2);

         local.disableTimeConstrained();
         localConstrained = false;
         surviving.disableTimeConstrained();
         survivingConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
      } finally {
         if (localConstrained && localJoined) {
            try { local.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (survivingConstrained && survivingJoined) {
            try { surviving.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (localSubscribed && localJoined) {
            try { local.unsubscribeObjectClassAttributes(localClass, localAttributes); }
            catch (Exception ignored) { }
         }
         if (survivingSubscribed && survivingJoined) {
            try { surviving.unsubscribeObjectClassAttributes(survivingClass, survivingAttributes); }
            catch (Exception ignored) { }
         }
         if (publisherPublished && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisher.getObjectClassHandle(
               objectClassName), JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                     attributeName))); }
            catch (Exception ignored) { }
         }
         if (localJoined) {
            try { local.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (survivingJoined) {
            try { surviving.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (publisherJoined) {
            try { publisher.resignFederationExecution(
               ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (survivingConnected) {
            try { surviving.disconnect(); } catch (Exception ignored) { }
         }
         if (localConnected) { try { local.disconnect(); } catch (Exception ignored) { } }
         if (publisherConnected) { try { publisher.disconnect(); } catch (Exception ignored) { } }
      }
   }

   private static void checkReflection(RTIambassador receiver, Recorder recorder,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         FederateHandle producer, LogicalTime<?, ?> time, OrderType receivedOrder,
         MessageRetractionHandle retraction, LogicalTime<?, ?> expectedGrant,
         int expectedReflectionCount, int expectedGrantCount) throws Exception {
      JavaTckSupport.check(recorder.reflections.size() == expectedReflectionCount,
         "local-delete scenario delivered a duplicate or missing reflection");
      Reflection actual = recorder.reflections.get(recorder.reflections.size() - 1);
      JavaTckSupport.check(object.equals(actual.object) && actual.values.size() == 1
            && Arrays.equals(value, actual.values.get(attribute)),
         "timestamped reflection returned the wrong object, attribute, or value");
      JavaTckSupport.check(Arrays.equals(tag, actual.tag)
            && producer.equals(actual.producer) && time.equals(actual.time),
         "timestamped reflection changed its tag, producer, or logical time");
      JavaTckSupport.check(actual.sentOrder == OrderType.TIMESTAMP
            && actual.receivedOrder == receivedOrder && retraction.equals(actual.retraction),
         "timestamped reflection returned incorrect order or retraction metadata");
      JavaTckSupport.check(actual.transportation != null
            && !receiver.getTransportationTypeName(actual.transportation).isEmpty(),
         "timestamped reflection returned an unresolvable transportation handle");
      JavaTckSupport.check(actual.regions == null || actual.regions.isEmpty(),
         "ordinary timestamped attribute reflection unexpectedly carried regions");
      JavaTckSupport.check(recorder.timeAdvanceGrantTimes.size() == expectedGrantCount
            && expectedGrant.equals(recorder.timeAdvanceGrantTimes.get(expectedGrantCount - 1)),
         "recipient received the wrong time-advance grant");
      JavaTckSupport.check(recorder.callbackOrder.equals(Arrays.asList("reflect", "grant")),
         "timestamped reflection was delivered after its matching grant");
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

   private static void awaitSessions(RTIambassador[] ambassadors, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            for (RTIambassador ambassador : ambassadors) JavaTckSupport.drain(ambassador);
         } else {
            Thread.yield();
         }
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
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
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
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
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
                  callbackOrder.add("reflect");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK local-delete attribute callbacks";
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
