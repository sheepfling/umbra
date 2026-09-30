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

/** Retraction after a former object owner resigns preserves only joined-member state. */
final class TimestampedObjectDeletionJoinedOwnersTck {
   private TimestampedObjectDeletionJoinedOwnersTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String firstAttributeName, String secondAttributeName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador formerOwner = factory.getRtiAmbassador();
      RTIambassador constrained = factory.getRtiAmbassador();
      Recorder publisherRecorder = new Recorder();
      Recorder formerRecorder = new Recorder();
      Recorder constrainedRecorder = new Recorder();
      String federation = "java-tck-deletion-joined-owners-" + UUID.randomUUID();
      boolean publisherConnected = false, formerConnected = false, constrainedConnected = false;
      boolean created = false, publisherJoined = false, formerJoined = false;
      boolean constrainedJoined = false, publisherPublished = false;
      boolean formerPublished = false, formerSubscribed = false, constrainedSubscribed = false;
      boolean publisherRegulating = false, constrainedEnabled = false;
      try {
         publisher.connect(publisherRecorder.proxy(), callbackModel); publisherConnected = true;
         formerOwner.connect(formerRecorder.proxy(), callbackModel); formerConnected = true;
         constrained.connect(constrainedRecorder.proxy(), callbackModel);
         constrainedConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-joined-deletion-publisher", federation);
         publisherJoined = true;
         formerOwner.joinFederationExecution("java-tck-joined-deletion-former", federation);
         formerJoined = true;
         constrained.joinFederationExecution("java-tck-joined-deletion-constrained", federation);
         constrainedJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle formerClass = formerOwner.getObjectClassHandle(objectClassName);
         ObjectClassHandle constrainedClass = constrained.getObjectClassHandle(objectClassName);
         AttributeHandle publisherFirst = publisher.getAttributeHandle(
            publisherClass, firstAttributeName);
         AttributeHandle publisherSecond = publisher.getAttributeHandle(
            publisherClass, secondAttributeName);
         AttributeHandle formerFirst = formerOwner.getAttributeHandle(formerClass,
            firstAttributeName);
         AttributeHandle formerSecond = formerOwner.getAttributeHandle(formerClass,
            secondAttributeName);
         AttributeHandle constrainedFirst = constrained.getAttributeHandle(constrainedClass,
            firstAttributeName);
         AttributeHandle constrainedSecond = constrained.getAttributeHandle(constrainedClass,
            secondAttributeName);
         JavaTckSupport.check(publisherClass != null && formerClass != null
               && constrainedClass != null && publisherFirst != null && publisherSecond != null
               && formerFirst != null && formerSecond != null && constrainedFirst != null
               && constrainedSecond != null && !publisherFirst.equals(publisherSecond),
            "multi-attribute object or distinct attributes were not available from the adapter FOM");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherFirst, publisherSecond);
         AttributeHandleSet formerAttributes =
            JavaTckSupport.attributeSet(formerOwner, formerFirst, formerSecond);
         AttributeHandleSet constrainedAttributes =
            JavaTckSupport.attributeSet(constrained, constrainedFirst, constrainedSecond);
         AttributeHandleSet publisherSecondOnly =
            JavaTckSupport.attributeSet(publisher, publisherSecond);
         AttributeHandleSet formerSecondOnly =
            JavaTckSupport.attributeSet(formerOwner, formerSecond);

         publisher.publishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherPublished = true;
         formerOwner.subscribeObjectClassAttributes(formerClass, formerAttributes);
         formerSubscribed = true;
         constrained.subscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
         constrainedSubscribed = true;
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.drain(formerOwner);
         JavaTckSupport.drain(constrained);
         JavaTckSupport.check(formerRecorder.discoveries.contains(object)
               && constrainedRecorder.discoveries.contains(object),
            "both joined recipients did not discover the registered object");
         String objectName = publisher.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty()
               && object.equals(formerOwner.getObjectInstanceHandle(objectName))
               && object.equals(constrained.getObjectInstanceHandle(objectName)),
            "object discovery did not preserve its shared identity");

         formerOwner.publishObjectClassAttributes(formerClass, formerSecondOnly);
         formerPublished = true;
         byte[] ownershipTag = new byte[] {0x4A, 0x4F, 0x49, 0x4E, 0x45, 0x44};
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            formerOwner.attributeOwnershipAcquisitionIfAvailable(
               object, formerSecondOnly, ownershipTag);
         } else {
            formerOwner.attributeOwnershipAcquisition(object, formerSecondOnly, ownershipTag);
         }
         AttributeHandleSet divested = publisher.attributeOwnershipDivestitureIfWanted(
            object, publisherSecondOnly, ownershipTag);
         JavaTckSupport.check(publisherSecondOnly.equals(divested),
            "exactly the requested second attribute was not divested");
         await(() -> !formerRecorder.acquisitions.isEmpty(), formerOwner, publisher,
            "joined former-owner acquisition notification");
         OwnershipEvent acquisition = formerRecorder.acquisitions.get(0);
         JavaTckSupport.check(object.equals(acquisition.object)
               && formerSecondOnly.equals(acquisition.attributes)
               && Arrays.equals(ownershipTag, acquisition.tag),
            "ownership callback returned the wrong object, attribute set, or tag");
         JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherFirst)
               && !publisher.isAttributeOwnedByFederate(object, publisherSecond)
               && formerOwner.isAttributeOwnedByFederate(object, formerSecond),
            "ownership transfer did not leave the expected split-owner state");

         LogicalTimeFactory<?, ?> timeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> constrainedTimeFactory = constrained.getTimeFactory();
         JavaTckSupport.check(timeFactory.getName().equals(constrainedTimeFactory.getName()),
            "publisher and constrained recipient selected different logical-time factories");
         LogicalTimeInterval epsilon = (LogicalTimeInterval) timeFactory.makeEpsilon();
         constrained.enableTimeConstrained(); constrainedEnabled = true;
         await(() -> constrainedRecorder.timeConstrainedEnabledCount == 1,
            constrained, "constrained recipient Time Constrained callback");
         LogicalTime publisherInitial = publisher.queryLogicalTime();
         LogicalTime constrainedInitial = constrained.queryLogicalTime();
         LogicalTimeInterval lookahead = intervalOffset(epsilon, 5);
         publisher.enableTimeRegulation(lookahead); publisherRegulating = true;
         await(() -> publisherRecorder.timeRegulationEnabledCount == 1,
            publisher, "publisher Time Regulation callback");

         LogicalTime deletionTime = offset(publisherInitial, epsilon, 6);
         byte[] deletionTag = new byte[] {0x44, 0x37, 0x2D, 0x4A, 0x4F, 0x49, 0x4E, 0x45, 0x44};
         MessageRetractionReturn deletion = publisher.deleteObjectInstance(
            object, deletionTag, deletionTime);
         JavaTckSupport.check(deletion != null && deletion.retractionHandleIsValid
               && deletion.handle != null,
            "timestamped object deletion returned no valid retraction handle");
         await(() -> !formerRecorder.removals.isEmpty(), formerOwner, constrained,
            "former owner timestamped removal callback");
         Removal removal = formerRecorder.removals.get(0);
         JavaTckSupport.check(formerRecorder.removals.size() == 1
               && object.equals(removal.object) && Arrays.equals(deletionTag, removal.tag)
               && producer.equals(removal.producer),
            "former-owner removal callback returned the wrong identity or tag");
         JavaTckSupport.check(deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.RECEIVE
               && deletion.handle.equals(removal.retraction),
            "former-owner removal lost its timestamp, receive-order, or retraction metadata");
         JavaTckSupport.check(constrainedRecorder.removals.isEmpty(),
            "constrained recipient received deletion before its grant");

         formerOwner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         formerJoined = false;
         JavaTckSupport.check(formerRecorder.retractionCallbacks == 0,
            "former owner received a retraction callback during resignation");
         LogicalTime constrainedTarget = offset(constrainedInitial, epsilon, 6);
         constrained.timeAdvanceRequest(constrainedTarget);
         publisher.retract(deletion.handle);
         JavaTckSupport.check(formerRecorder.retractionCallbacks == 0,
            "retraction was delivered to the departed former owner");
         JavaTckSupport.check(object.equals(publisher.getObjectInstanceHandle(objectName))
               && object.equals(constrained.getObjectInstanceHandle(objectName)),
            "retraction did not restore joined members' object identity");
         JavaTckSupport.check(publisher.isAttributeOwnedByFederate(object, publisherFirst)
               && !publisher.isAttributeOwnedByFederate(object, publisherSecond),
            "retraction incorrectly restored the departed owner's former attribute");

         publisherRecorder.ownershipNotOwned.clear();
         publisher.queryAttributeOwnership(object, publisherSecondOnly);
         await(() -> !publisherRecorder.ownershipNotOwned.isEmpty(), publisher,
            "query of the departed owner's unowned attribute");
         JavaTckSupport.check(publisherRecorder.ownershipNotOwned.size() == 1
               && object.equals(publisherRecorder.ownershipNotOwned.get(0).object)
               && publisherSecond.equals(publisherRecorder.ownershipNotOwned.get(0).attribute),
            "ownership query did not report the former owner's attribute as unowned");

         LogicalTime publisherTarget = offset(publisherInitial, epsilon, 2);
         publisher.timeAdvanceRequest(publisherTarget);
         await(() -> publisherRecorder.grants.size() == 1
               && constrainedRecorder.grants.size() == 1,
            publisher, constrained, "post-retraction publisher and receiver grants");
         JavaTckSupport.check(publisherTarget.equals(publisherRecorder.grants.get(0))
               && constrainedTarget.equals(constrainedRecorder.grants.get(0)),
            "post-retraction time advances granted the wrong requested times");
         JavaTckSupport.check(constrainedRecorder.removals.isEmpty()
               && constrainedRecorder.retractionCallbacks == 0,
            "retracted deletion produced stale removal or retraction callbacks");

         constrained.disableTimeConstrained(); constrainedEnabled = false;
         publisher.disableTimeRegulation(); publisherRegulating = false;
         constrained.unsubscribeObjectClassAttributes(constrainedClass, constrainedAttributes);
         constrainedSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherClass, publisherAttributes);
         publisherPublished = false;
         constrained.resignFederationExecution(ResignAction.NO_ACTION);
         constrainedJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (constrainedEnabled && constrainedJoined) try {
            constrained.disableTimeConstrained();
         } catch (Exception ignored) { }
         if (publisherRegulating && publisherJoined) try { publisher.disableTimeRegulation(); }
            catch (Exception ignored) { }
         if (formerPublished && formerJoined) try {
            ObjectClassHandle cls = formerOwner.getObjectClassHandle(objectClassName);
            formerOwner.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               formerOwner, formerOwner.getAttributeHandle(cls, secondAttributeName)));
         } catch (Exception ignored) { }
         if (formerSubscribed && formerJoined) try {
            ObjectClassHandle cls = formerOwner.getObjectClassHandle(objectClassName);
            formerOwner.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               formerOwner, formerOwner.getAttributeHandle(cls, firstAttributeName),
               formerOwner.getAttributeHandle(cls, secondAttributeName)));
         } catch (Exception ignored) { }
         if (constrainedSubscribed && constrainedJoined) try {
            ObjectClassHandle cls = constrained.getObjectClassHandle(objectClassName);
            constrained.unsubscribeObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               constrained, constrained.getAttributeHandle(cls, firstAttributeName),
               constrained.getAttributeHandle(cls, secondAttributeName)));
         } catch (Exception ignored) { }
         if (formerJoined) try {
            formerOwner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (constrainedJoined) try {
            constrained.resignFederationExecution(ResignAction.NO_ACTION);
         } catch (Exception ignored) { }
         if (publisherPublished && publisherJoined) try {
            ObjectClassHandle cls = publisher.getObjectClassHandle(objectClassName);
            publisher.unpublishObjectClassAttributes(cls, JavaTckSupport.attributeSet(
               publisher, publisher.getAttributeHandle(cls, firstAttributeName),
               publisher.getAttributeHandle(cls, secondAttributeName)));
         } catch (Exception ignored) { }
         if (publisherJoined) try {
            publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         } catch (Exception ignored) { }
         if (created) try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         if (constrainedConnected) try { constrained.disconnect(); } catch (Exception ignored) { }
         if (formerConnected) try { formerOwner.disconnect(); } catch (Exception ignored) { }
         if (publisherConnected) try { publisher.disconnect(); } catch (Exception ignored) { }
      }
   }

   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int i = 0; i < steps; ++i) result = (LogicalTime) result.add(epsilon);
      return result;
   }

   private static LogicalTimeInterval intervalOffset(LogicalTimeInterval epsilon, int steps)
         throws Exception {
      LogicalTimeInterval result = epsilon;
      for (int i = 1; i < steps; ++i) result = (LogicalTimeInterval) result.add(epsilon);
      return result;
   }

   private static void await(Ready ready, RTIambassador first, String description)
         throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(first);
         if (ready.test()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   private static void await(Ready ready, RTIambassador first, RTIambassador second,
         String description) throws Exception {
      long deadline = System.nanoTime() + 10_000_000_000L;
      while (System.nanoTime() < deadline) {
         JavaTckSupport.drain(first);
         JavaTckSupport.drain(second);
         if (ready.test()) return;
         Thread.sleep(1L);
      }
      throw new AssertionError(description + " timed out");
   }

   @FunctionalInterface
   private interface Ready { boolean test(); }

   private static final class OwnershipEvent {
      private ObjectInstanceHandle object;
      private AttributeHandleSet attributes;
      private byte[] tag;
   }

   private static final class UnownedAttribute {
      private ObjectInstanceHandle object;
      private AttributeHandle attribute;
   }

   private static final class Removal {
      private ObjectInstanceHandle object;
      private byte[] tag;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder, receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new ArrayList<>();
      private final List<OwnershipEvent> acquisitions = new ArrayList<>();
      private final List<UnownedAttribute> ownershipNotOwned = new ArrayList<>();
      private final List<Removal> removals = new ArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new ArrayList<>();
      private int timeConstrainedEnabledCount, timeRegulationEnabledCount;
      private int retractionCallbacks;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("attributeOwnershipAcquisitionNotification".equals(name)
                     && arguments != null && arguments.length >= 3) {
                  OwnershipEvent event = new OwnershipEvent();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.attributes = (AttributeHandleSet) arguments[1];
                  event.tag = ((byte[]) arguments[2]).clone();
                  acquisitions.add(event);
               } else if ("attributeIsNotOwned".equals(name) && arguments != null
                     && arguments.length >= 2) {
                  UnownedAttribute event = new UnownedAttribute();
                  event.object = (ObjectInstanceHandle) arguments[0];
                  event.attribute = (AttributeHandle) arguments[1];
                  ownershipNotOwned.add(event);
               } else if ("removeObjectInstance".equals(name) && arguments != null
                     && arguments.length == 7) {
                  Removal removal = new Removal();
                  removal.object = (ObjectInstanceHandle) arguments[0];
                  removal.tag = ((byte[]) arguments[1]).clone();
                  removal.producer = (FederateHandle) arguments[2];
                  removal.time = (LogicalTime<?, ?>) arguments[3];
                  removal.sentOrder = (OrderType) arguments[4];
                  removal.receivedOrder = (OrderType) arguments[5];
                  removal.retraction = (MessageRetractionHandle) arguments[6];
                  removals.add(removal);
               } else if ("requestRetraction".equals(name)) {
                  ++retractionCallbacks;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabledCount;
               } else if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabledCount;
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK joined-owner deletion callbacks";
                  if ("hashCode".equals(name)) return System.identityHashCode(proxy);
                  if ("equals".equals(name)) return proxy == (arguments == null ? null : arguments[0]);
               }
               return null;
            });
      }
   }
}
