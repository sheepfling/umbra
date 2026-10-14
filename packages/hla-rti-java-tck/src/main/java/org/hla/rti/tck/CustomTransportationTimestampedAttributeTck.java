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

/** Verifies timestamped attribute retraction and delivery with an adapter-declared FOM transport. */
final class CustomTransportationTimestampedAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private CustomTransportationTimestampedAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String transportationName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder immediateCallbacks = new Recorder();
      Recorder constrainedCallbacks = new Recorder();
      String federation = "java-tck-custom-tso-attribute-" + UUID.randomUUID();
      boolean publisherConnected = false, immediateConnected = false, constrainedConnected = false;
      boolean created = false, publisherJoined = false, immediateJoined = false;
      boolean constrainedJoined = false, published = false;
      boolean immediateSubscribed = false, constrainedSubscribed = false;
      boolean regulationEnabled = false, constrainedEnabled = false;
      ObjectClassHandle publisherClass = null, immediateClass = null, constrainedClass = null;
      AttributeHandle publisherAttribute = null, immediateAttribute = null, constrainedAttribute = null;
      AttributeHandleSet publisherAttributes = null, immediateAttributes = null;
      AttributeHandleSet constrainedAttributes = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         immediate.connect(immediateCallbacks.proxy(), callbackModel);
         immediateConnected = true;
         constrained.connect(constrainedCallbacks.proxy(), callbackModel);
         constrainedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-custom-tso-attribute-publisher", federation);
         publisherJoined = true;
         immediate.joinFederationExecution("java-tck-custom-tso-attribute-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution(
            "java-tck-custom-tso-attribute-constrained", federation);
         constrainedJoined = true;

         publisherClass = publisher.getObjectClassHandle(objectClassName);
         immediateClass = immediate.getObjectClassHandle(objectClassName);
         constrainedClass = constrained.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherClass, attributeName);
         immediateAttribute = immediate.getAttributeHandle(immediateClass, attributeName);
         constrainedAttribute = constrained.getAttributeHandle(constrainedClass, attributeName);
         TransportationTypeHandle publisherTransportation =
            publisher.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle immediateTransportation =
            immediate.getTransportationTypeHandle(transportationName);
         TransportationTypeHandle constrainedTransportation =
            constrained.getTransportationTypeHandle(transportationName);
         JavaTckSupport.check(objectClassName.equals(publisher.getObjectClassName(publisherClass))
               && objectClassName.equals(immediate.getObjectClassName(immediateClass))
               && objectClassName.equals(constrained.getObjectClassName(constrainedClass))
               && attributeName.equals(publisher.getAttributeName(publisherClass, publisherAttribute))
               && attributeName.equals(immediate.getAttributeName(immediateClass, immediateAttribute))
               && attributeName.equals(constrained.getAttributeName(
                  constrainedClass, constrainedAttribute))
               && transportationName.equals(publisher.getTransportationTypeName(publisherTransportation))
               && transportationName.equals(immediate.getTransportationTypeName(immediateTransportation))
               && transportationName.equals(
                  constrained.getTransportationTypeName(constrainedTransportation)),
            "adapter FOM class, attribute, or custom transportation names did not round-trip");

         publisherAttributes = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         immediateAttributes = JavaTckSupport.attributeSet(immediate, immediateAttribute);
         constrainedAttributes = JavaTckSupport.attributeSet(constrained, constrainedAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         publisher.changeDefaultAttributeOrderType(publisherClass, publisherAttributes,
            OrderType.TIMESTAMP);
         immediate.subscribeObjectClassAttributes(immediateClass, immediateAttributes);
         immediateSubscribed = true;
         constrained.subscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
         constrainedSubscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null && object.encodedLength() > 0,
            "custom timestamped attribute registration returned an invalid object handle");
         awaitBoth(publisher, immediate, callbackModel,
            () -> immediateCallbacks.discoveries.contains(object),
            "immediate receiver object discovery");
         awaitBoth(publisher, constrained, callbackModel,
            () -> constrainedCallbacks.discoveries.contains(object),
            "constrained receiver object discovery");
         JavaTckSupport.check(immediateCallbacks.discoveries.size() == 1
               && constrainedCallbacks.discoveries.size() == 1
               && immediateClass.equals(immediateCallbacks.discoveries.get(0).objectClass)
               && constrainedClass.equals(constrainedCallbacks.discoveries.get(0).objectClass),
            "custom timestamped attribute discovery returned incorrect object metadata");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedFactory = constrained.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(constrainedFactory.getName()),
            "publisher and constrained receiver selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel, () -> publisherCallbacks.timeRegulationEnabled,
            "publisher time regulation enablement");
         constrained.enableTimeConstrained();
         await(constrained, callbackModel, () -> constrainedCallbacks.timeConstrainedEnabled,
            "constrained receiver time-constrained enablement");
         regulationEnabled = true;
         constrainedEnabled = true;

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTime immediateInitial = immediate.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(constrainedInitial)
               && publisherInitial.equals(immediateInitial),
            "custom timestamped attribute participants began at different logical times");
         LogicalTime firstTime = offset(publisherInitial, epsilon, 3);
         LogicalTime firstConstrainedTime = offset(constrainedInitial, epsilon, 3);
         JavaTckSupport.check(firstTime.equals(firstConstrainedTime),
            "publisher and constrained receiver derived different first timestamps");
         byte[] firstValue = new byte[] {0x41, 0x42, 0x43};
         byte[] firstTag = new byte[] {0x51, 0x52};
         MessageRetractionReturn firstRetraction = update(
            publisher, object, publisherAttribute, firstValue, firstTag, firstTime);
         requireValidRetraction(firstRetraction, "first timestamped update");
         JavaTckSupport.check(constrainedCallbacks.reflections.isEmpty(),
            "timestamped reflection reached the constrained receiver before its advance");
         await(immediate, callbackModel,
            () -> immediateCallbacks.reflections.size() >= 1,
            "first immediate timestamped reflection");
         JavaTckSupport.check(immediateCallbacks.reflections.size() == 1,
            "first timestamped update produced duplicate immediate reflections");
         checkReflection(immediateCallbacks.reflections.get(0), immediateAttribute, object,
            producer, firstValue, firstTag, firstTime, OrderType.RECEIVE,
            firstRetraction.handle, transportationName, immediate);

         publisher.retract(firstRetraction.handle);
         await(immediate, callbackModel,
            () -> immediateCallbacks.retractions.size() >= 1,
            "immediate receiver retraction callback");
         JavaTckSupport.check(immediateCallbacks.retractions.size() == 1
               && firstRetraction.handle.equals(immediateCallbacks.retractions.get(0)),
            "retraction callback returned the wrong handle");
         JavaTckSupport.check(constrainedCallbacks.reflections.isEmpty()
               && constrainedCallbacks.retractions.isEmpty(),
            "retraction or reflection leaked to the pending constrained receiver");

         constrained.timeAdvanceRequest(firstConstrainedTime);
         publisher.timeAdvanceRequest(firstTime);
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherCallbacks.grants.size() >= 1
               && constrainedCallbacks.grants.size() >= 1,
            "first producer and constrained time-advance grants");
         JavaTckSupport.check(constrainedCallbacks.reflections.isEmpty(),
            "retracted update was delivered when the constrained receiver advanced");

         LogicalTime secondTime = offset(firstTime, epsilon, 3);
         LogicalTime secondConstrainedTime = offset(firstConstrainedTime, epsilon, 3);
         JavaTckSupport.check(secondTime.equals(secondConstrainedTime),
            "publisher and constrained receiver derived different second timestamps");
         byte[] secondValue = new byte[] {0x61, 0x62, 0x63};
         byte[] secondTag = new byte[] {0x71, 0x72};
         MessageRetractionReturn secondRetraction = update(
            publisher, object, publisherAttribute, secondValue, secondTag, secondTime);
         requireValidRetraction(secondRetraction, "second timestamped update");
         await(immediate, callbackModel,
            () -> immediateCallbacks.reflections.size() >= 2,
            "second immediate timestamped reflection");
         JavaTckSupport.check(immediateCallbacks.reflections.size() == 2,
            "second timestamped update produced duplicate immediate reflections");
         checkReflection(immediateCallbacks.reflections.get(1), immediateAttribute, object,
            producer, secondValue, secondTag, secondTime, OrderType.RECEIVE,
            secondRetraction.handle, transportationName, immediate);

         constrained.timeAdvanceRequest(secondConstrainedTime);
         publisher.timeAdvanceRequest(secondTime);
         awaitBoth(publisher, constrained, callbackModel,
            () -> publisherCallbacks.grants.size() >= 2
               && constrainedCallbacks.grants.size() >= 2
               && constrainedCallbacks.reflections.size() >= 1,
            "second timestamped delivery and matching grants");
         JavaTckSupport.check(constrainedCallbacks.reflections.size() == 1,
            "constrained receiver got the wrong number of timestamped reflections");
         checkReflection(constrainedCallbacks.reflections.get(0), constrainedAttribute,
            object, producer, secondValue, secondTag, secondTime, OrderType.TIMESTAMP,
            secondRetraction.handle, transportationName, constrained);
         expectException(() -> publisher.retract(secondRetraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting the delivered custom-transportation update");

         constrained.disableTimeConstrained();
         constrainedEnabled = false;
         publisher.disableTimeRegulation();
         regulationEnabled = false;
         if (immediateSubscribed) {
            immediate.unsubscribeObjectClassAttributes(immediateClass, immediateAttributes);
            immediateSubscribed = false;
         }
         if (constrainedSubscribed) {
            constrained.unsubscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
            constrainedSubscribed = false;
         }
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         published = false;
      } finally {
         if (constrainedEnabled && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulationEnabled && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (immediateSubscribed && immediateJoined) {
            try { immediate.unsubscribeObjectClassAttributes(immediateClass, immediateAttributes); }
            catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeObjectClassAttributes(constrainedClass, constrainedAttributes); }
            catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes); }
            catch (Exception ignored) { }
         }
         if (constrainedJoined) resign(constrained);
         if (immediateJoined) resign(immediate);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (constrainedConnected) disconnect(constrained);
         if (immediateConnected) disconnect(immediate);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static MessageRetractionReturn update(RTIambassador publisher,
         ObjectInstanceHandle object, AttributeHandle attribute, byte[] value, byte[] tag,
         LogicalTime<?, ?> time) throws Exception {
      AttributeHandleValueMap values = publisher.getAttributeHandleValueMapFactory().create(1);
      values.put(attribute, value.clone());
      return publisher.updateAttributeValues(object, values, tag.clone(), time);
   }

   private static void requireValidRetraction(MessageRetractionReturn result, String what) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid && result.handle != null,
         what + " returned no valid message retraction handle");
   }

   private static void checkReflection(Reflection reflection, AttributeHandle expectedAttribute,
         ObjectInstanceHandle object, FederateHandle producer, byte[] value, byte[] tag,
         LogicalTime<?, ?> time, OrderType expectedReceivedOrder,
         MessageRetractionHandle retraction, String transportationName,
         RTIambassador inspector) throws Exception {
      JavaTckSupport.check(object.equals(reflection.object) && reflection.values.size() == 1
            && Arrays.equals(value, reflection.values.get(expectedAttribute))
            && Arrays.equals(tag, reflection.tag) && producer.equals(reflection.producer)
            && time.equals(reflection.time) && reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == expectedReceivedOrder
            && retraction.equals(reflection.retraction)
            && reflection.transportation != null
            && transportationName.equals(
               inspector.getTransportationTypeName(reflection.transportation)),
         "timestamped reflection changed its value, tag, time, order, retraction, or transport");
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      awaitBoth(ambassador, null, model, condition, description);
   }

   private static void awaitBoth(RTIambassador first, RTIambassador second,
         CallbackModel model, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (System.nanoTime() < deadline && !condition.getAsBoolean()) {
         if (model == CallbackModel.HLA_EVOKED) {
            JavaTckSupport.drain(first);
            if (second != null) JavaTckSupport.drain(second);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      JavaTckSupport.expectFailure(operation, expected, description);
   }

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Discovery {
      private final ObjectInstanceHandle object;
      private final ObjectClassHandle objectClass;

      private Discovery(ObjectInstanceHandle object, ObjectClassHandle objectClass) {
         this.object = object;
         this.objectClass = objectClass;
      }
   }

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(ObjectInstanceHandle object, AttributeHandleValueMap values,
            byte[] tag, TransportationTypeHandle transportation, FederateHandle producer,
            LogicalTime<?, ?> time, OrderType sentOrder, OrderType receivedOrder,
            MessageRetractionHandle retraction) {
         this.object = object;
         this.values = values.clone();
         this.tag = tag.clone();
         this.transportation = transportation;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<Discovery> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<MessageRetractionHandle> retractions = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private volatile boolean timeRegulationEnabled;
      private volatile boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  discoveries.add(new Discovery((ObjectInstanceHandle) arguments[0],
                     (ObjectClassHandle) arguments[1]));
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("reflectAttributeValues".equals(name) && arguments != null
                     && arguments.length == 10) {
                  reflections.add(new Reflection((ObjectInstanceHandle) arguments[0],
                     (AttributeHandleValueMap) arguments[1], (byte[]) arguments[2],
                     (TransportationTypeHandle) arguments[3], (FederateHandle) arguments[4],
                     (LogicalTime<?, ?>) arguments[6], (OrderType) arguments[7],
                     (OrderType) arguments[8], (MessageRetractionHandle) arguments[9]));
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK custom timestamped attribute callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
