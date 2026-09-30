package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
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
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Exercises timestamped object removal through FQR, TARA, and NMRA. */
final class TimestampedObjectDeletionAlternateAdvancesTck {
   private TimestampedObjectDeletionAlternateAdvancesTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador flushReceiver = factory.getRtiAmbassador();
      RTIambassador taraReceiver = factory.getRtiAmbassador();
      RTIambassador nmraReceiver = factory.getRtiAmbassador();
      Recorder publisherRec = new Recorder();
      Recorder flushRec = new Recorder();
      Recorder taraRec = new Recorder();
      Recorder nmraRec = new Recorder();
      String federation = "java-tck-timestamped-deletion-alternates-" + UUID.randomUUID();
      boolean publisherConnected = false, flushConnected = false;
      boolean taraConnected = false, nmraConnected = false;
      boolean created = false, publisherJoined = false, flushJoined = false;
      boolean taraJoined = false, nmraJoined = false;
      boolean attributesPublished = false, flushSubscribed = false;
      boolean taraSubscribed = false, nmraSubscribed = false;
      boolean publisherRegulating = false, flushConstrained = false;
      boolean taraConstrained = false, nmraConstrained = false;
      try {
         publisher.connect(publisherRec.proxy(), callbackModel); publisherConnected = true;
         flushReceiver.connect(flushRec.proxy(), callbackModel); flushConnected = true;
         taraReceiver.connect(taraRec.proxy(), callbackModel); taraConnected = true;
         nmraReceiver.connect(nmraRec.proxy(), callbackModel); nmraConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-deletion-alternates-publisher", federation); publisherJoined = true;
         flushReceiver.joinFederationExecution("java-tck-deletion-fqr", federation);
         flushJoined = true;
         taraReceiver.joinFederationExecution("java-tck-deletion-tara", federation);
         taraJoined = true;
         nmraReceiver.joinFederationExecution("java-tck-deletion-nmra", federation);
         nmraJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute =
            publisher.getAttributeHandle(publisherClass, attributeName);
         AttributeHandleSet published = JavaTckSupport.attributeSet(publisher, publisherAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         attributesPublished = true;

         RTIambassador[] receivers = {flushReceiver, taraReceiver, nmraReceiver};
         Recorder[] receiverRecs = {flushRec, taraRec, nmraRec};
         ObjectClassHandle[] receiverClasses = new ObjectClassHandle[receivers.length];
         AttributeHandleSet[] subscriptions = new AttributeHandleSet[receivers.length];
         for (int i = 0; i < receivers.length; ++i) {
            receiverClasses[i] = receivers[i].getObjectClassHandle(objectClassName);
            AttributeHandle attribute = receivers[i].getAttributeHandle(
               receiverClasses[i], attributeName);
            subscriptions[i] = JavaTckSupport.attributeSet(receivers[i], attribute);
            receivers[i].subscribeObjectClassAttributes(receiverClasses[i], subscriptions[i]);
            if (i == 0) flushSubscribed = true;
            else if (i == 1) taraSubscribed = true;
            else nmraSubscribed = true;
            JavaTckSupport.check(publisherClass.equals(receiverClasses[i]),
               "object-class handles were not stable across deletion recipients");
         }

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         for (int i = 0; i < receivers.length; ++i) {
            checkDiscovery(receiverRecs[i], object, publisherClass, producer,
               "timestamped deletion receiver " + i);
         }

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> flushFactory = flushReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> taraFactory = taraReceiver.getTimeFactory();
         LogicalTimeFactory<?, ?> nmraFactory = nmraReceiver.getTimeFactory();
         String factoryName = publisherFactory.getName();
         JavaTckSupport.check(factoryName.equals(flushFactory.getName())
               && factoryName.equals(taraFactory.getName())
               && factoryName.equals(nmraFactory.getName()),
            "timestamped deletion recipients selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         flushReceiver.enableTimeConstrained(); flushConstrained = true;
         taraReceiver.enableTimeConstrained(); taraConstrained = true;
         nmraReceiver.enableTimeConstrained(); nmraConstrained = true;
         publisher.enableTimeRegulation(lookahead); publisherRegulating = true;
         JavaTckSupport.drain(publisher);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(publisherRec.timeRegulationEnabled
               && flushRec.timeConstrainedEnabled && taraRec.timeConstrainedEnabled
               && nmraRec.timeConstrainedEnabled,
            "a timestamped deletion time-role callback was not observed");

         LogicalTime initial = publisher.queryLogicalTime();
         LogicalTime flushInitial = flushReceiver.queryLogicalTime();
         LogicalTime taraInitial = taraReceiver.queryLogicalTime();
         LogicalTime nmraInitial = nmraReceiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(flushInitial) && initial.equals(taraInitial)
               && initial.equals(nmraInitial),
            "timestamped deletion recipients began at different logical times");
         LogicalTime deletionTime = offset(initial, epsilon, 7);
         LogicalTime flushBoundary = offset(flushInitial,
            (LogicalTimeInterval) flushFactory.makeEpsilon(), 10);
         LogicalTime taraTarget = offset(taraInitial,
            (LogicalTimeInterval) taraFactory.makeEpsilon(), 7);
         LogicalTime nmraBoundary = offset(nmraInitial,
            (LogicalTimeInterval) nmraFactory.makeEpsilon(), 10);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         byte[] tag = new byte[] {(byte) 0xD8, 0x61, 0x4E};
         MessageRetractionReturn sent = publisher.deleteObjectInstance(object, tag, deletionTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped Delete Object Instance returned no valid retraction handle");
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         JavaTckSupport.check(flushRec.removals.isEmpty() && taraRec.removals.isEmpty()
               && nmraRec.removals.isEmpty(),
            "timestamped object removal arrived before an advance request");
         flushRec.callbackOrder.clear(); taraRec.callbackOrder.clear();
         nmraRec.callbackOrder.clear(); publisherRec.callbackOrder.clear();

         flushReceiver.flushQueueRequest(flushBoundary);
         taraReceiver.timeAdvanceRequestAvailable(taraTarget);
         nmraReceiver.nextMessageRequestAvailable(nmraBoundary);
         publisher.timeAdvanceRequest(publisherTarget);
         awaitCompletion(publisher, receivers, publisherRec, receiverRecs);

         checkRemoval(flushRec, object, producer, tag, deletionTime, sent.handle,
            "Flush Queue Request");
         checkRemoval(taraRec, object, producer, tag, deletionTime, sent.handle,
            "Time Advance Request Available");
         checkRemoval(nmraRec, object, producer, tag, deletionTime, sent.handle,
            "Next Message Request Available");
         JavaTckSupport.check(publisherRec.timeAdvanceGrantTimes.size() == 1
               && publisherTarget.equals(publisherRec.timeAdvanceGrantTimes.get(0)),
            "publisher TAR did not grant its requested logical time");
         JavaTckSupport.check(flushRec.flushQueueGrantTimes.size() == 1
               && flushRec.timeAdvanceGrantTimes.isEmpty()
               && flushRec.callbackOrder.equals(Arrays.asList(
                  "removeObjectInstance", "flushQueueGrant")),
            "Flush Queue Request did not remove the object before its sole grant");
         JavaTckSupport.check(taraRec.flushQueueGrantTimes.isEmpty()
               && taraRec.timeAdvanceGrantTimes.size() == 1
               && taraRec.callbackOrder.equals(Arrays.asList(
                  "removeObjectInstance", "timeAdvanceGrant")),
            "TARA did not remove the object before its sole grant");
         JavaTckSupport.check(nmraRec.flushQueueGrantTimes.isEmpty()
               && nmraRec.timeAdvanceGrantTimes.size() == 1
               && nmraRec.callbackOrder.equals(Arrays.asList(
                  "removeObjectInstance", "timeAdvanceGrant")),
            "NMRA did not remove the object before its sole grant");
         JavaTckSupport.check(taraTarget.equals(taraRec.timeAdvanceGrantTimes.get(0))
               && deletionTime.equals(nmraRec.timeAdvanceGrantTimes.get(0)),
            "TARA or NMRA returned the wrong grant time");
         JavaTckSupport.check(flushRec.flushQueueGrantTime != null
               && flushRec.flushQueueOptimisticTime != null
               && deletionTime.equals(flushRec.flushQueueOptimisticTime)
               && compareTime(flushRec.flushQueueGrantTime, flushInitial) >= 0
               && compareTime(flushRec.flushQueueGrantTime, flushBoundary) <= 0
               && compareTime(flushRec.flushQueueOptimisticTime,
                  flushRec.flushQueueGrantTime) >= 0
               && flushRec.flushQueueGrantTime.equals(flushReceiver.queryLogicalTime()),
            "Flush Queue Grant/query returned inconsistent actual or optimistic time");
         JavaTckSupport.check(taraTarget.equals(taraReceiver.queryLogicalTime())
               && deletionTime.equals(nmraReceiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "logical-time queries disagreed with their advance grants");
         expectException(() -> publisher.retract(sent.handle),
            "MessageCanNoLongerBeRetracted", "retracting the delivered object deletion");

         flushReceiver.disableTimeConstrained(); flushConstrained = false;
         taraReceiver.disableTimeConstrained(); taraConstrained = false;
         nmraReceiver.disableTimeConstrained(); nmraConstrained = false;
         publisher.disableTimeRegulation(); publisherRegulating = false;
         for (int i = 0; i < receivers.length; ++i) {
            receivers[i].unsubscribeObjectClassAttributes(receiverClasses[i], subscriptions[i]);
            if (i == 0) flushSubscribed = false;
            else if (i == 1) taraSubscribed = false;
            else nmraSubscribed = false;
         }
         publisher.unpublishObjectClassAttributes(publisherClass, published);
         attributesPublished = false;
      } finally {
         if (flushConstrained && flushJoined) disableConstrained(flushReceiver);
         if (taraConstrained && taraJoined) disableConstrained(taraReceiver);
         if (nmraConstrained && nmraJoined) disableConstrained(nmraReceiver);
         if (publisherRegulating && publisherJoined) try {
            publisher.disableTimeRegulation();
         } catch (Exception ignored) { }
         if (flushSubscribed && flushJoined) unsubscribe(flushReceiver, objectClassName, attributeName);
         if (taraSubscribed && taraJoined) unsubscribe(taraReceiver, objectClassName, attributeName);
         if (nmraSubscribed && nmraJoined) unsubscribe(nmraReceiver, objectClassName, attributeName);
         if (attributesPublished && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               publisher, publisher.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (flushJoined) resign(flushReceiver);
         if (taraJoined) resign(taraReceiver);
         if (nmraJoined) resign(nmraReceiver);
         if (publisherJoined) resign(publisher);
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (nmraConnected) disconnect(nmraReceiver);
         if (taraConnected) disconnect(taraReceiver);
         if (flushConnected) disconnect(flushReceiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void awaitCompletion(RTIambassador publisher, RTIambassador[] receivers,
         Recorder publisherRec, Recorder[] receiverRecs) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(publisher);
         for (RTIambassador receiver : receivers) JavaTckSupport.drain(receiver);
         if (!publisherRec.timeAdvanceGrantTimes.isEmpty()
               && receiverRecs[0].removals.size() == 1
               && !receiverRecs[0].flushQueueGrantTimes.isEmpty()
               && receiverRecs[1].removals.size() == 1
               && !receiverRecs[1].timeAdvanceGrantTimes.isEmpty()
               && receiverRecs[2].removals.size() == 1
               && !receiverRecs[2].timeAdvanceGrantTimes.isEmpty()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError("timestamped object-deletion alternate advances timed out");
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compareTime(LogicalTime left, LogicalTime right) {
      return left.compareTo(right);
   }

   private static void checkDiscovery(Recorder recorder, ObjectInstanceHandle object,
         ObjectClassHandle objectClass, FederateHandle producer, String role) {
      JavaTckSupport.check(recorder.discoveries.size() == 1,
         role + " did not receive exactly one object discovery");
      Discovery discovery = recorder.discoveries.get(0);
      JavaTckSupport.check(object.equals(discovery.object)
            && objectClass.equals(discovery.objectClass) && producer.equals(discovery.producer),
         role + " received incorrect discovery metadata");
   }

   private static void checkRemoval(Recorder recorder, ObjectInstanceHandle expectedObject,
         FederateHandle expectedProducer, byte[] expectedTag, LogicalTime<?, ?> expectedTime,
         MessageRetractionHandle expectedRetraction, String role) {
      JavaTckSupport.check(recorder.removals.size() == 1,
         role + " did not receive exactly one object removal");
      Removal removal = recorder.removals.get(0);
      JavaTckSupport.check(expectedObject.equals(removal.object)
            && Arrays.equals(expectedTag, removal.tag)
            && expectedProducer.equals(removal.producer),
         role + " returned the wrong object-removal identity or metadata");
      JavaTckSupport.check(expectedTime.equals(removal.time)
            && removal.sentOrder == OrderType.TIMESTAMP
            && removal.receivedOrder == OrderType.TIMESTAMP,
         role + " returned the wrong timestamp or order metadata");
      JavaTckSupport.check(expectedRetraction.equals(removal.retraction),
         role + " did not preserve the message retraction handle");
   }

   private static void expectException(JavaTckSupport.CheckedOperation operation,
         String expected, String description) throws Exception {
      try {
         operation.run();
      } catch (Exception exception) {
         for (Class<?> type = exception.getClass(); type != null; type = type.getSuperclass()) {
            if (expected.equals(type.getSimpleName())) return;
         }
         throw new AssertionError(description + " threw " + exception.getClass().getName()
            + " instead of " + expected, exception);
      }
      throw new AssertionError(description + " did not throw " + expected);
   }

   private static void unsubscribe(RTIambassador rti, String className, String attributeName) {
      try {
         ObjectClassHandle cls = rti.getObjectClassHandle(className);
         AttributeHandle attribute = rti.getAttributeHandle(cls, attributeName);
         rti.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(rti, attribute));
      } catch (Exception ignored) { }
   }

   private static void disableConstrained(RTIambassador rti) {
      try { rti.disableTimeConstrained(); } catch (Exception ignored) { }
   }

   private static void resign(RTIambassador rti) {
      try { rti.resignFederationExecution(ResignAction.NO_ACTION); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador rti) {
      try { rti.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private FederateHandle producer;
   }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private boolean timeRegulationEnabled, timeConstrainedEnabled;
      private final List<Discovery> discoveries = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<LogicalTime<?, ?>> timeAdvanceGrantTimes = new ArrayList<>();
      private final List<LogicalTime<?, ?>> flushQueueGrantTimes = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private LogicalTime<?, ?> flushQueueGrantTime, flushQueueOptimisticTime;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) timeRegulationEnabled = true;
               else if ("timeConstrainedEnabled".equals(name)) timeConstrainedEnabled = true;
               else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.producer = (FederateHandle) arguments[2];
                  discoveries.add(event);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  Removal event = new Removal();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.tag = ((byte[]) arguments[1]).clone();
                  event.producer = (FederateHandle) arguments[2];
                  event.time = (LogicalTime<?, ?>) arguments[3];
                  event.sentOrder = (OrderType) arguments[4];
                  event.receivedOrder = (OrderType) arguments[5];
                  event.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(event); callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  timeAdvanceGrantTimes.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               } else if ("flushQueueGrant".equals(name) && arguments != null
                     && arguments.length == 2) {
                  flushQueueGrantTime = (LogicalTime<?, ?>) arguments[0];
                  flushQueueOptimisticTime = (LogicalTime<?, ?>) arguments[1];
                  flushQueueGrantTimes.add(flushQueueGrantTime); callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion alternate-advance callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
