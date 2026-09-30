package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.time.LogicalTime;
import hla.rti1516_2025.time.LogicalTimeFactory;
import hla.rti1516_2025.time.LogicalTimeInterval;
import java.lang.reflect.Proxy;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;

/** Checks timestamped removal delivery after a changed-lookahead regulation re-enable. */
final class TimestampedObjectDeletionRegulationReenableTck {
   private TimestampedObjectDeletionRegulationReenableTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder receiverRecorder = new Recorder();
      String federation = "java-tck-deletion-regulation-reenable-" + UUID.randomUUID();
      boolean publisherConnected = false, receiverConnected = false, created = false;
      boolean publisherJoined = false, receiverJoined = false;
      boolean published = false, subscribed = false, regulating = false, constrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel); publisherConnected = true;
         receiver.connect(receiverRecorder.proxy(), callbackModel); receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation); created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-deletion-regulation-producer", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-deletion-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         published = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         subscribed = true;

         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.discoveredObjects.contains(object),
            "receiver did not discover the object before timestamped deletion");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(receiver.getObjectInstanceHandle(objectName)),
            "discovery did not establish the shared object identity");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(receiverTimeFactory.getName()),
            "publisher and receiver selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         receiver.enableTimeConstrained(); constrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.timeConstrainedEnabledCount == 1,
            "receiver did not report Time Constrained enable");
         LogicalTimeInterval initialLookahead = intervalOffset(epsilon, 1);
         publisher.enableTimeRegulation(initialLookahead); regulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 1,
            "publisher did not report initial Time Regulation enable");

         LogicalTime initial = publisher.queryLogicalTime();
         JavaTckSupport.check(initial.equals(receiver.queryLogicalTime()),
            "publisher and receiver began at different logical times");
         LogicalTime deletionTime = offset(initial, epsilon, 5);
         LogicalTime publisherTarget = offset(initial, epsilon, 2);
         LogicalTimeInterval changedLookahead = intervalOffset(epsilon, 3);
         byte[] tag = new byte[] {0x43, 0x48, 0x47, 0x2D, 0x4C, 0x41};
         MessageRetractionReturn deletion = publisher.deleteObjectInstance(
            object, tag, deletionTime);
         JavaTckSupport.check(deletion != null && deletion.retractionHandleIsValid
               && deletion.handle != null,
            "timestamped deletion returned no valid retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.removals.isEmpty()
               && object.equals(receiver.getObjectInstanceHandle(objectName)),
            "removal arrived or object identity changed before its grant");

         publisher.disableTimeRegulation(); regulating = false;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationDisabledCount == 1,
            "publisher did not report Time Regulation disable");
         publisher.enableTimeRegulation(changedLookahead); regulating = true;
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(publisherRecorder.timeRegulationEnabledCount == 2,
            "publisher did not report the second Time Regulation enable");
         JavaTckSupport.check(changedLookahead.equals(publisher.queryLookahead()),
            "Query Lookahead did not return the changed lookahead");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverRecorder.removals.isEmpty(),
            "queued deletion was delivered during Time Regulation re-enable");

         receiverRecorder.callbackOrder.clear();
         receiver.timeAdvanceRequest(deletionTime);
         JavaTckSupport.check(receiverRecorder.removals.isEmpty(),
            "removal arrived before the producer crossed the changed boundary");
         publisher.timeAdvanceRequest(publisherTarget);
         JavaTckSupport.drain(receiver);
         JavaTckSupport.drain(publisher);
         JavaTckSupport.check(receiverRecorder.removals.size() == 1,
            "queued timestamped removal was not delivered exactly once");
         JavaTckSupport.check(receiverRecorder.callbackOrder.equals(
               Arrays.asList("removeObjectInstance", "timeAdvanceGrant")),
            "timestamped removal did not precede the constrained receiver grant");
         Removal removal = receiverRecorder.removals.get(0);
         JavaTckSupport.check(object.equals(removal.object) && Arrays.equals(tag, removal.tag)
               && producer.equals(removal.producer),
            "removal callback returned the wrong object, tag, or producer");
         JavaTckSupport.check(deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && deletion.handle.equals(removal.retraction),
            "removal callback did not preserve timestamp/order/retraction metadata");
         JavaTckSupport.check(deletionTime.equals(receiverRecorder.grant)
               && publisherTarget.equals(publisherRecorder.grant)
               && deletionTime.equals(receiver.queryLogicalTime())
               && publisherTarget.equals(publisher.queryLogicalTime()),
            "grants or logical-time queries did not match their requested times");
         expectException(() -> receiver.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown", "looking up the removed receiver object");
         expectException(() -> publisher.getObjectInstanceName(object),
            "ObjectInstanceNotKnown", "looking up the removed publisher object");
         expectException(() -> publisher.retract(deletion.handle),
            "MessageCanNoLongerBeRetracted", "retracting the delivered object deletion");

         receiver.disableTimeConstrained(); constrained = false;
         publisher.disableTimeRegulation(); regulating = false;
      } finally {
         if (constrained && receiverJoined) try { receiver.disableTimeConstrained(); }
            catch (Exception ignored) { }
         if (regulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (subscribed && receiverJoined) try {
            ObjectClassHandle cls = receiver.getObjectClassHandle(objectClassName);
            receiver.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               receiver, receiver.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (published && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               publisher, publisher.getAttributeHandle(cls, attributeName)));
         } catch (Exception ignored) { }
         if (receiverJoined) try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (receiverConnected) try { receiver.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
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

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder, receivedOrder;
      private hla.rti1516_2025.MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveredObjects = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private int timeConstrainedEnabledCount;
      private int timeRegulationEnabledCount, timeRegulationDisabledCount;
      private LogicalTime<?, ?> grant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveredObjects.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeRegulationDisabled".equals(name)) {
                  ++timeRegulationDisabledCount;
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (hla.rti1516_2025.MessageRetractionHandle) arguments[6];
                  removals.add(removal);
                  callbackOrder.add(name);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grant = (LogicalTime<?, ?>) arguments[0];
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion regulation callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
