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
import hla.rti1516_2025.TimeQueryReturn;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Exercises NMR and NMRA while a future timestamped object deletion arrives. */
final class TimestampedObjectDeletionFutureRequestArrivalTck {
   private TimestampedObjectDeletionFutureRequestArrivalTck() { }

   static void runNmr(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, false);
   }

   static void runNmra(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      run(factory, fom, timeImplementation, objectClassName, attributeName,
         callbackModel, true);
   }

   private static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel,
         boolean available) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String requestName = available ? "nmra" : "nmr";
      String federation = "java-tck-deletion-future-" + requestName + "-"
         + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false;
      boolean created = false, publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false;
      boolean regulating = false, constrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-deletion-future-" + requestName + "-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution(
            "java-tck-deletion-future-" + requestName + "-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "adapter FOM lookups returned inconsistent object or attribute handles");
         AttributeHandleSet publishedAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet subscribedAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         subscribed = true;

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "federates selected different logical-time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiverInitial),
            "federates began at different logical times");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);

         receiver.enableTimeConstrained();
         constrained = true;
         publisher.enableTimeRegulation(lookahead);
         regulating = true;
         await(() -> receiverRecorder.timeConstrainedEnabled
               && publisherRecorder.timeRegulationEnabled,
            publisher, receiver, "time-role callbacks");

         LogicalTime<?, ?> publisherCurrent = offset(initial, epsilon, 2);
         publisher.timeAdvanceRequest(publisherCurrent);
         await(() -> publisherRecorder.grants.size() == 1,
            publisher, receiver, "publisher's initial time advance");
         JavaTckSupport.check(publisherCurrent.equals(publisherRecorder.grants.get(0)),
            "publisher advanced to an unexpected logical time");

         LogicalTime<?, ?> deletionTime = offset(publisherCurrent, epsilon, 5);
         LogicalTime<?, ?> requestBoundary = offset(
            receiverInitial, (LogicalTimeInterval) receiverFactory.makeEpsilon(), 10);
         TimeQueryReturn galt = receiver.queryGALT();
         JavaTckSupport.check(galt.timeIsValid && deletionTime.equals(galt.time),
            "receiver did not observe the defined GALT at the future deletion time");

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         await(() -> receiverRecorder.discoveries.size() == 1,
            publisher, receiver, "object discovery");
         Discovery discovery = receiverRecorder.discoveries.get(0);
         JavaTckSupport.check(object.equals(discovery.object)
               && receiverClass.equals(discovery.objectClass)
               && discovery.name != null && !discovery.name.isEmpty()
               && producer.equals(discovery.producer),
            "object discovery returned the wrong instance, class, or producer");

         receiverRecorder.callbackOrder.clear();
         if (available) receiver.nextMessageRequestAvailable(requestBoundary);
         else receiver.nextMessageRequest(requestBoundary);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.removals.isEmpty()
               && receiverRecorder.grants.isEmpty(),
            requestName + " completed before its future timestamped deletion arrived");

         byte[] tag = available
            ? new byte[] {0x4e, 0x4d, 0x52, 0x41}
            : new byte[] {0x4e, 0x4d, 0x52};
         MessageRetractionReturn sent = publisher.deleteObjectInstance(
            object, tag.clone(), deletionTime);
         JavaTckSupport.check(sent != null && sent.retractionHandleIsValid
               && sent.handle != null,
            "timestamped deletion returned no valid retraction handle");

         await(() -> receiverRecorder.removals.size() >= 1
               && receiverRecorder.grants.size() >= 1,
            publisher, receiver, "future deletion and request-completion callbacks");
         JavaTckSupport.check(receiverRecorder.removals.size() == 1
               && receiverRecorder.grants.size() == 1,
            "future deletion produced duplicate or missing receiver callbacks");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(Arrays.asList(
               "removeObjectInstance", "timeAdvanceGrant")),
            "receiver grant arrived before the future timestamped deletion");
         Removal removal = receiverRecorder.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object)
               && producer.equals(removal.producer)
               && Arrays.equals(tag, removal.tag),
            "timestamped removal returned the wrong object, producer, or tag");
         JavaTckSupport.check(deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && sent.handle.equals(removal.retraction),
            "timestamped removal returned incorrect time, order, or retraction metadata");
         JavaTckSupport.check(deletionTime.equals(receiverRecorder.grants.get(0))
               && deletionTime.equals(receiver.queryLogicalTime()),
            "request did not grant and advance to the future deletion time");

         receiver.disableTimeConstrained();
         constrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         receiver.unsubscribeObjectClassAttributes(receiverClass, subscribedAttributes);
         subscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes);
         published = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (subscribed && receiverJoined) {
            try {
               ObjectClassHandle cls = receiver.getObjectClassHandle(objectClassName);
               receiver.unsubscribeObjectClassAttributes(cls,
                  JavaTckSupport.attributeSet(receiver,
                     receiver.getAttributeHandle(cls, attributeName)));
            } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try {
               ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
               publisher.unpublishObjectClassAttributes(cls,
                  JavaTckSupport.attributeSet(publisher,
                     publisher.getAttributeHandle(cls, attributeName)));
            } catch (Exception ignored) { }
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

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(interval);
      return result;
   }

   private static void await(Ready ready, RTIambassador publisher,
         RTIambassador receiver, String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(publisher);
         JavaTckSupport.drain(receiver);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   @FunctionalInterface
   private interface Ready { boolean isReady(); }

   private static final class Discovery {
      private ObjectInstanceHandle object;
      private ObjectClassHandle objectClass;
      private String name;
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
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;
      private final List<Discovery> discoveries = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();

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
                     && arguments != null && arguments.length == 4) {
                  Discovery event = new Discovery();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.objectClass = (ObjectClassHandle) arguments[1];
                  event.name = (String) arguments[2];
                  event.producer = (FederateHandle) arguments[3];
                  discoveries.add(event);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  Removal event = new Removal();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.tag = ((byte[]) arguments[1]).clone();
                  event.producer = (FederateHandle) arguments[2];
                  event.time = (LogicalTime<?, ?>) arguments[3];
                  event.sentOrder = (OrderType) arguments[4];
                  event.receivedOrder = (OrderType) arguments[5];
                  event.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(event);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK future timestamped deletion callbacks";
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
