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

/** Portable IEEE 1516.1-2025 timestamped object-deletion and retraction checks. */
final class TimestampedObjectDeletionRetractionTck {
   private TimestampedObjectDeletionRetractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador immediate = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder immediateRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-object-deletion-retraction-" + UUID.randomUUID();
      boolean publisherConnected = false, immediateConnected = false;
      boolean constrainedConnected = false, created = false;
      boolean publisherJoined = false, immediateJoined = false;
      boolean constrainedJoined = false, published = false;
      boolean immediateSubscribed = false, constrainedSubscribed = false;
      boolean regulating = false, timeConstrained = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel);
         publisherConnected = true;
         immediate.connect(immediateRecorder.proxy(), callbackModel);
         immediateConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;

         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-deletion-retraction-publisher", federation);
         publisherJoined = true;
         immediate.joinFederationExecution("java-tck-deletion-retraction-immediate", federation);
         immediateJoined = true;
         constrained.joinFederationExecution(
            "java-tck-deletion-retraction-constrained", federation);
         constrainedJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle immediateClass = immediate.getObjectClassHandle(objectClassName);
         ObjectClassHandle constrainedClass = constrained.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle immediateAttribute = immediate.getAttributeHandle(
            immediateClass, attributeName);
         AttributeHandle constrainedAttribute = constrained.getAttributeHandle(
            constrainedClass, attributeName);
         AttributeHandleSet publishedAttributes = JavaTckSupport.attributeSet(
            publisher, publisherAttribute);
         AttributeHandleSet immediateAttributes = JavaTckSupport.attributeSet(
            immediate, immediateAttribute);
         AttributeHandleSet constrainedAttributes = JavaTckSupport.attributeSet(
            constrained, constrainedAttribute);
         publisher.publishObjectClassAttributes(publisherClass, publishedAttributes);
         published = true;
         immediate.subscribeObjectClassAttributes(immediateClass, immediateAttributes);
         immediateSubscribed = true;
         constrained.subscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
         constrainedSubscribed = true;

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         publisher.enableTimeRegulation(epsilon);
         regulating = true;
         constrained.enableTimeConstrained();
         timeConstrained = true;
         await(() -> publisherRecorder.timeRegulationEnabled, publisher,
            "timeRegulationEnabled callback");
         await(() -> constrainedRecorder.timeConstrainedEnabled, constrained,
            "timeConstrainedEnabled callback");

         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         check(publisherInitial.equals(constrainedInitial),
            "publisher and constrained member began at different logical times");
         RegisteredObject first = registerAndDiscover(publisher, publisherClass,
            immediate, immediateRecorder, constrained, constrainedRecorder);
         LogicalTime firstTime = offset(publisherInitial, epsilon, 3);
         LogicalTime firstConstrainedTime = offset(constrainedInitial, epsilon, 3);
         byte[] firstTag = new byte[] {(byte) 0xD1, (byte) 0xE2, (byte) 0xF3};
         MessageRetractionReturn firstRetraction = publisher.deleteObjectInstance(
            first.handle, firstTag, firstTime);
         check(firstRetraction != null && firstRetraction.retractionHandleIsValid
               && firstRetraction.handle != null,
            "timestamped object deletion returned no valid retraction handle");
         await(() -> immediateRecorder.removals.size() == 1, immediate,
            "immediate timestamped deletion callback");
         assertRemoval(immediateRecorder.removals.get(0), first.handle, firstTag, producer,
            firstTime, OrderType.RECEIVE, firstRetraction.handle);
         check(constrainedRecorder.removals.isEmpty(),
            "constrained member received deletion before advancing time");
         expectException(() -> immediate.getObjectInstanceHandle(first.name),
            "ObjectInstanceNotKnown", "name lookup after provisional removal");
         check(first.handle.equals(constrained.getObjectInstanceHandle(first.name)),
            "pending timestamped deletion changed the constrained member's object identity");

         publisher.retract(firstRetraction.handle);
         await(() -> immediateRecorder.retractions.size() == 1, immediate,
            "requestRetraction callback for delivered removal");
         check(firstRetraction.handle.equals(immediateRecorder.retractions.get(0)),
            "requestRetraction callback returned the wrong handle");
         check(constrainedRecorder.retractions.isEmpty(),
            "pending constrained removal produced a retraction callback");
         check(first.handle.equals(immediate.getObjectInstanceHandle(first.name))
               && first.handle.equals(constrained.getObjectInstanceHandle(first.name)),
            "retraction did not restore object identity at both recipients");

         constrained.timeAdvanceRequest(firstConstrainedTime);
         publisher.timeAdvanceRequest(firstTime);
         await(() -> constrainedRecorder.grants.size() == 1, constrained,
            "constrained advance after retracting the first deletion");
         await(() -> publisherRecorder.grants.size() == 1, publisher,
            "publisher advance after retracting the first deletion");
         check(constrainedRecorder.removals.isEmpty(),
            "retracted deletion was delivered at the constrained member's grant");

         RegisteredObject second = registerAndDiscover(publisher, publisherClass,
            immediate, immediateRecorder, constrained, constrainedRecorder);
         LogicalTime secondTime = offset(firstTime, epsilon, 3);
         LogicalTime secondConstrainedTime = offset(firstConstrainedTime, epsilon, 3);
         byte[] secondTag = new byte[] {(byte) 0xA1, (byte) 0xB2, (byte) 0xC3};
         MessageRetractionReturn secondRetraction = publisher.deleteObjectInstance(
            second.handle, secondTag, secondTime);
         check(secondRetraction != null && secondRetraction.retractionHandleIsValid
               && secondRetraction.handle != null,
            "second timestamped deletion returned no valid retraction handle");
         await(() -> immediateRecorder.removals.size() == 2, immediate,
            "second immediate timestamped deletion callback");
         assertRemoval(immediateRecorder.removals.get(1), second.handle, secondTag, producer,
            secondTime, OrderType.RECEIVE, secondRetraction.handle);
         check(second.handle.equals(constrained.getObjectInstanceHandle(second.name))
               && constrainedRecorder.removals.isEmpty(),
            "constrained member observed the second deletion before its time advance");

         constrainedRecorder.callbackOrder.clear();
         constrained.timeAdvanceRequest(secondConstrainedTime);
         publisher.timeAdvanceRequest(secondTime);
         await(() -> constrainedRecorder.grants.size() == 2
               && constrainedRecorder.removals.size() == 1, constrained,
            "second timestamped deletion at the constrained member");
         await(() -> publisherRecorder.grants.size() == 2, publisher,
            "second timestamped deletion at the publisher");
         assertRemoval(constrainedRecorder.removals.get(0), second.handle, secondTag, producer,
            secondTime, OrderType.TIMESTAMP, secondRetraction.handle);
         check(constrainedRecorder.callbackOrder.size() == 2
               && "removeObjectInstance".equals(constrainedRecorder.callbackOrder.get(0))
               && "timeAdvanceGrant".equals(constrainedRecorder.callbackOrder.get(1)),
            "timestamped deletion was not delivered before its matching grant");
         check(secondTime.equals(constrainedRecorder.grants.get(1)),
            "constrained member received the wrong logical-time grant");
         expectException(() -> publisher.retract(secondRetraction.handle),
            "MessageCanNoLongerBeRetracted",
            "retracting a deletion after its producer boundary");
         LogicalTime afterRemovalTime = offset(secondTime, epsilon, 2);
         expectException(() -> publisher.deleteObjectInstance(second.handle, secondTag,
               afterRemovalTime), "ObjectInstanceNotKnown",
            "deleting an object instance after its terminal removal");
         expectException(() -> immediate.getObjectInstanceHandle(second.name),
            "ObjectInstanceNotKnown", "lookup after terminal immediate removal");
         expectException(() -> constrained.getObjectInstanceHandle(second.name),
            "ObjectInstanceNotKnown", "lookup after terminal constrained removal");

         constrained.disableTimeConstrained();
         timeConstrained = false;
         publisher.disableTimeRegulation();
         regulating = false;
         immediate.unsubscribeObjectClassAttributes(immediateClass, immediateAttributes);
         immediateSubscribed = false;
         constrained.unsubscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
         constrainedSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publishedAttributes);
         published = false;
      } finally {
         if (timeConstrained && constrainedJoined) {
            try { constrained.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (regulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (immediateSubscribed && immediateJoined) {
            try { immediate.unsubscribeObjectClassAttributes(
               immediate.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(immediate,
                  immediate.getAttributeHandle(immediate.getObjectClassHandle(objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (constrainedSubscribed && constrainedJoined) {
            try { constrained.unsubscribeObjectClassAttributes(
               constrained.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(constrained,
                  constrained.getAttributeHandle(constrained.getObjectClassHandle(objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (published && publisherJoined) {
            try { publisher.unpublishObjectClassAttributes(
               publisher.getObjectClassHandle(objectClassName),
               JavaTckSupport.attributeSet(publisher,
                  publisher.getAttributeHandle(publisher.getObjectClassHandle(objectClassName),
                     attributeName))); } catch (Exception ignored) { }
         }
         if (constrainedJoined) try {
            constrained.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (immediateJoined) try {
            immediate.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (constrainedConnected) try { constrained.disconnect(); }
            catch (Exception ignored) { }
         if (immediateConnected) try { immediate.disconnect(); }
            catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); }
            catch (Exception ignored) { }
      }
   }

   private static RegisteredObject registerAndDiscover(RTIambassador publisher,
         ObjectClassHandle objectClass, RTIambassador immediate, Recorder immediateRecorder,
         RTIambassador constrained, Recorder constrainedRecorder) throws Exception {
      ObjectInstanceHandle object = publisher.registerObjectInstance(objectClass);
      String name = publisher.getObjectInstanceName(object);
      await(() -> immediateRecorder.discovered.contains(object), immediate,
         "immediate object discovery");
      await(() -> constrainedRecorder.discovered.contains(object), constrained,
         "constrained object discovery");
      check(object.equals(immediate.getObjectInstanceHandle(name))
            && object.equals(constrained.getObjectInstanceHandle(name)),
         "object discovery did not preserve name-to-handle identity");
      return new RegisteredObject(object, name);
   }

   private static void assertRemoval(Removal removal, ObjectInstanceHandle object, byte[] tag,
         FederateHandle producer, LogicalTime<?, ?> time, OrderType receivedOrder,
         MessageRetractionHandle retraction) {
      check(object.equals(removal.object), "removal callback identified the wrong object");
      check(Arrays.equals(tag, removal.tag), "removal callback changed the user tag");
      check(producer.equals(removal.producer), "removal callback identified the wrong producer");
      check(time.equals(removal.time), "removal callback changed the logical time");
      check(removal.sentOrder == OrderType.TIMESTAMP,
         "timestamped deletion reported a non-timestamp sent order");
      check(removal.receivedOrder == receivedOrder,
         "removal callback reported the wrong received order");
      check(retraction.equals(removal.retraction),
         "removal callback returned the wrong retraction handle");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime<?, ?> offset(LogicalTime<?, ?> start,
         LogicalTimeInterval epsilon, int steps) throws Exception {
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

   private static void await(ReadyCheck ready, RTIambassador ambassador, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(ambassador);
         if (ready.isReady()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   private static void check(boolean condition, String message) {
      if (!condition) throw new AssertionError(message);
   }

   @FunctionalInterface
   private interface ReadyCheck { boolean isReady(); }

   private static final class RegisteredObject {
      private final ObjectInstanceHandle handle;
      private final String name;
      private RegisteredObject(ObjectInstanceHandle handle, String name) {
         this.handle = handle;
         this.name = name;
      }
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;
      private Removal(ObjectInstanceHandle object, byte[] tag, FederateHandle producer,
            LogicalTime<?, ?> time, OrderType sentOrder, OrderType receivedOrder,
            MessageRetractionHandle retraction) {
         this.object = object;
         this.tag = tag;
         this.producer = producer;
         this.time = time;
         this.sentOrder = sentOrder;
         this.receivedOrder = receivedOrder;
         this.retraction = retraction;
      }
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discovered = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<MessageRetractionHandle> retractions = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private final List<String> callbackOrder = new ArrayList<>();
      private boolean timeRegulationEnabled;
      private boolean timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discovered.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled = true;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled = true;
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  removals.add(new Removal((ObjectInstanceHandle) arguments[0],
                     ((byte[]) arguments[1]).clone(), (FederateHandle) arguments[2],
                     (LogicalTime<?, ?>) arguments[3], (OrderType) arguments[4],
                     (OrderType) arguments[5], (MessageRetractionHandle) arguments[6]));
                  callbackOrder.add(name);
               } else if ("requestRetraction".equals(name) && arguments != null
                     && arguments.length == 1) {
                  retractions.add((MessageRetractionHandle) arguments[0]);
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add(name);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK deletion-retraction callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
