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

/** Portable timestamped attribute delivery through the three alternate advances. */
final class TimestampedAttributeAlternateAdvancesTck {
   private TimestampedAttributeAlternateAdvancesTck() {
   }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador flushReceiver = factory.getRtiAmbassador();
      RTIambassador taraReceiver = factory.getRtiAmbassador();
      RTIambassador nmraReceiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder flushRecorder = new Recorder();
      Recorder taraRecorder = new Recorder();
      Recorder nmraRecorder = new Recorder();
      String federation = "java-tck-timestamped-attribute-alternate-advances-"
         + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean flushConnected = false;
      boolean taraConnected = false;
      boolean nmraConnected = false;
      boolean publisherJoined = false;
      boolean flushJoined = false;
      boolean taraJoined = false;
      boolean nmraJoined = false;
      boolean created = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         flushReceiver.connect(flushRecorder.proxy(), callbackModel);
         flushConnected = true;
         taraReceiver.connect(taraRecorder.proxy(), callbackModel);
         taraConnected = true;
         nmraReceiver.connect(nmraRecorder.proxy(), callbackModel);
         nmraConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-attribute-producer", federation);
         publisherJoined = true;
         flushReceiver.joinFederationExecution("java-tck-attribute-fq", federation);
         flushJoined = true;
         taraReceiver.joinFederationExecution("java-tck-attribute-tara", federation);
         taraJoined = true;
         nmraReceiver.joinFederationExecution("java-tck-attribute-nmra", federation);
         nmraJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);

         List<RTIambassador> receivers = Arrays.asList(
            flushReceiver, taraReceiver, nmraReceiver);
         List<Recorder> receiverRecorders = Arrays.asList(
            flushRecorder, taraRecorder, nmraRecorder);
         List<ObjectClassHandle> receiverClasses = new ArrayList<>();
         List<AttributeHandle> receiverAttributes = new ArrayList<>();
         List<AttributeHandleSet> subscriptions = new ArrayList<>();
         for (RTIambassador receiver : receivers) {
            ObjectClassHandle objectClass = receiver.getObjectClassHandle(objectClassName);
            AttributeHandle attribute = receiver.getAttributeHandle(objectClass, attributeName);
            AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
            subscribed.add(attribute);
            receiver.subscribeObjectClassAttributes(objectClass, subscribed);
            receiverClasses.add(objectClass);
            receiverAttributes.add(attribute);
            subscriptions.add(subscribed);
         }

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         for (int i = 0; i < receivers.size(); ++i) {
            JavaTckSupport.drain(receivers.get(i));
            JavaTckSupport.check(object.equals(receiverRecorders.get(i).discoveredObject),
               "timestamped attribute receiver did not discover the registered object");
         }

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         String timeFactoryName = publisherTimeFactory.getName();
         LogicalTimeFactory<?, ?> flushTimeFactory = flushReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> taraTimeFactory = taraReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> nmraTimeFactory = nmraReceiver.getTimeFactory();
         JavaTckSupport.check(timeFactoryName.equals(flushTimeFactory.getName())
               && timeFactoryName.equals(taraTimeFactory.getName())
               && timeFactoryName.equals(nmraTimeFactory.getName()),
            "timestamped attribute members selected different logical-time factories");

         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         for (int i = 0; i < receivers.size(); ++i) {
            receivers.get(i).enableTimeConstrained();
            JavaTckSupport.drain(receivers.get(i));
         }
         publisher.enableTimeRegulation(epsilon);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabled
               && flushRecorder.timeConstrainedEnabled
               && taraRecorder.timeConstrainedEnabled
               && nmraRecorder.timeConstrainedEnabled,
            "time-role callbacks did not cross the standard callback interfaces");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime flushInitial = flushReceiver.queryLogicalTime();
         LogicalTime taraInitial = taraReceiver.queryLogicalTime();
         LogicalTime nmraInitial = nmraReceiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(flushInitial) && initial.equals(taraInitial)
               && initial.equals(nmraInitial),
            "timestamped attribute members began at different logical times");
         LogicalTime messageTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 4);
         LogicalTime requestBoundary = offset(initial, epsilon, 10);
         byte[] payload = new byte[] {0x41, 0x54, 0x54, 0x52, 0x2d, 0x41};
         byte[] tag = new byte[] {0x41, 0x4c, 0x54, 0x2d, 0x41};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, payload);
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag, messageTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "timestamped Update Attribute Values returned no valid retraction handle");
         for (int i = 0; i < receivers.size(); ++i) {
            JavaTckSupport.drain(receivers.get(i));
            JavaTckSupport.check(receiverRecorders.get(i).reflections.isEmpty(),
               "timestamped reflection arrived before its receiver requested an advance");
            receiverRecorders.get(i).callbackOrder.clear();
         }
         publisherRecorder.callbackOrder.clear();

         flushReceiver.flushQueueRequest(requestBoundary);
         taraReceiver.timeAdvanceRequestAvailable(messageTime);
         nmraReceiver.nextMessageRequestAvailable(requestBoundary);
         publisher.timeAdvanceRequest(publisherTarget);
         for (RTIambassador ambassador : Arrays.asList(
               publisher, flushReceiver, taraReceiver, nmraReceiver)) {
            JavaTckSupport.drain(ambassador);
         }

         checkReceiver(flushRecorder, receiverAttributes.get(0),
            object, producer, payload, tag, messageTime, retraction.handle,
            flushReceiver, "Flush Queue receiver");
         checkReceiver(taraRecorder, receiverAttributes.get(1),
            object, producer, payload, tag, messageTime, retraction.handle,
            taraReceiver, "TARA receiver");
         checkReceiver(nmraRecorder, receiverAttributes.get(2),
            object, producer, payload, tag, messageTime, retraction.handle,
            nmraReceiver, "NMRA receiver");

         JavaTckSupport.check(publisherRecorder.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRecorder.timeAdvanceGrantTimes.get(0)),
            "producer TAR did not grant its requested logical time");
         JavaTckSupport.check(flushRecorder.flushQueueGrantTimes.size() == 1
               && flushRecorder.timeAdvanceGrantTimes.isEmpty()
               && flushRecorder.callbackOrder.equals(
                  Arrays.asList("reflectAttributeValues", "flushQueueGrant")),
            "Flush Queue Request did not reflect the update before its sole grant");
         JavaTckSupport.check(taraRecorder.flushQueueGrantTimes.isEmpty()
               && taraRecorder.timeAdvanceGrantTimes.size() == 1
               && taraRecorder.callbackOrder.equals(
                  Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "TARA did not reflect the update before its sole grant");
         JavaTckSupport.check(nmraRecorder.flushQueueGrantTimes.isEmpty()
               && nmraRecorder.timeAdvanceGrantTimes.size() == 1
               && nmraRecorder.callbackOrder.equals(
                  Arrays.asList("reflectAttributeValues", "timeAdvanceGrant")),
            "NMRA did not reflect the update before its sole grant");
         JavaTckSupport.check(messageTime.equals(taraRecorder.timeAdvanceGrantTimes.get(0))
               && messageTime.equals(nmraRecorder.timeAdvanceGrantTimes.get(0)),
            "TARA or NMRA returned the wrong grant time");
         JavaTckSupport.check(flushRecorder.flushQueueGrantTime != null
               && flushRecorder.flushQueueOptimisticTime != null
               && messageTime.equals(flushRecorder.flushQueueOptimisticTime)
               && compareTime(flushRecorder.flushQueueGrantTime, initial) >= 0
               && compareTime(flushRecorder.flushQueueGrantTime, requestBoundary) <= 0
               && compareTime(flushRecorder.flushQueueOptimisticTime,
                  flushRecorder.flushQueueGrantTime) >= 0
               && flushRecorder.flushQueueGrantTime.equals(flushReceiver.queryLogicalTime()),
            "Flush Queue Grant/query returned inconsistent actual or optimistic time");
         JavaTckSupport.check(messageTime.equals(taraReceiver.queryLogicalTime())
               && messageTime.equals(nmraReceiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries disagreed with their respective grants");
         expectException(() -> publisher.retract(retraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting an update delivered through an alternate advance");

         for (int i = 0; i < receivers.size(); ++i) {
            receivers.get(i).disableTimeConstrained();
            JavaTckSupport.drain(receivers.get(i));
            receivers.get(i).unsubscribeObjectClassAttributes(
               receiverClasses.get(i), subscriptions.get(i));
            receivers.get(i).unsubscribeObjectClass(receiverClasses.get(i));
         }
         publisher.disableTimeRegulation();
         JavaTckSupport.drain(publisher);
         publisher.unpublishObjectClassAttributes(publisherClass, published);
         publisher.unpublishObjectClass(publisherClass);
      } finally {
         if (nmraJoined) {
            try { nmraReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (taraJoined) {
            try { taraReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (flushJoined) {
            try { flushReceiver.resignFederationExecution(ResignAction.NO_ACTION); }
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
         if (nmraConnected) {
            try { nmraReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (taraConnected) {
            try { taraReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (flushConnected) {
            try { flushReceiver.disconnect(); } catch (Exception ignored) { }
         }
         if (publisherConnected) {
            try { publisher.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void checkReceiver(Recorder recorder, AttributeHandle expectedAttribute,
         ObjectInstanceHandle expectedObject,
         FederateHandle expectedProducer, byte[] expectedPayload, byte[] expectedTag,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         RTIambassador ambassador, String description) throws Exception {
      JavaTckSupport.check(recorder.reflections.size() == 1,
         description + " received the wrong number of reflections");
      Reflection reflection = recorder.reflections.get(0);
      JavaTckSupport.check(expectedObject.equals(reflection.object)
            && reflection.values != null && reflection.values.size() == 1
            && Arrays.equals(expectedPayload, reflection.values.get(expectedAttribute)),
         description + " received the wrong class, object, or attribute payload");
      JavaTckSupport.check(Arrays.equals(expectedTag, reflection.tag),
         description + " did not preserve the user tag");
      JavaTckSupport.check(expectedProducer.equals(reflection.producer)
            && expectedTime.equals(reflection.time),
         description + " did not preserve the producer or timestamp");
      JavaTckSupport.check(reflection.sentOrder == OrderType.TIMESTAMP
            && reflection.receivedOrder == OrderType.TIMESTAMP,
         description + " did not report timestamp order at both boundaries");
      JavaTckSupport.check(expectedRetraction.equals(reflection.retraction)
            && reflection.transportation != null
            && !ambassador.getTransportationTypeName(reflection.transportation).isEmpty(),
         description + " returned an invalid retraction or transportation handle");
   }

   private static void expectException(CheckedOperation operation, String expected,
         String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) {
               return;
            }
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   @FunctionalInterface
   private interface CheckedOperation {
      void run() throws Exception;
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
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private ObjectInstanceHandle discoveredObject;
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Reflection> reflections = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueGrantTime;
      private LogicalTime<?, ?> flushQueueOptimisticTime;
      private final List<String> callbackOrder = new ArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
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
                  if ("toString".equals(name)) return "Java TCK alternate attribute callbacks";
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
