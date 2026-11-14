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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies object-known state after FQR delivers a timestamped removal. */
final class FqrTimestampedObjectDeletionKnownStateTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private FqrTimestampedObjectDeletionKnownStateTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-fqr-timestamped-object-deletion-" + UUID.randomUUID();
      boolean ownerConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean ownerJoined = false;
      boolean receiverJoined = false;
      boolean ownerPublished = false;
      boolean receiverSubscribed = false;
      boolean ownerRegulating = false;
      boolean receiverConstrained = false;
      ObjectClassHandle ownerClass = null;
      ObjectClassHandle receiverClass = null;
      AttributeHandleSet ownerAttributes = null;
      AttributeHandleSet receiverAttributes = null;
      try {
         owner.connect(ownerCallbacks.proxy(), callbackModel);
         ownerConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         owner.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = owner.joinFederationExecution(
            "java-tck-fqr-timestamped-deletion-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution(
            "java-tck-fqr-timestamped-deletion-receiver", federation);
         receiverJoined = true;

         ownerClass = owner.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(ownerClass != null && receiverClass != null
               && ownerClass.equals(receiverClass) && ownerAttribute != null
               && ownerAttribute.equals(receiverAttribute),
            "FQR timestamped deletion FOM returned inconsistent class or attribute handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "FQR timestamped deletion returned no object handle");
         String objectName = owner.getObjectInstanceName(object);
         JavaTckSupport.check(objectName != null && !objectName.isEmpty(),
            "FQR timestamped deletion returned an empty object name");
         await(receiver, callbackModel,
            () -> object.equals(receiverCallbacks.discoveredObject),
            "FQR timestamped deletion object discovery");
         JavaTckSupport.check(receiverClass.equals(receiver.getKnownObjectClassHandle(object)),
            "FQR timestamped deletion receiver did not initially know the object");

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(ownerFactory.getName().equals(receiverFactory.getName()),
            "FQR timestamped deletion members selected different logical-time factories");
         LogicalTime<?, ?> ownerInitial = owner.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(ownerInitial.equals(receiverInitial),
            "FQR timestamped deletion members began at different logical times");
         LogicalTimeInterval ownerEpsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled != null,
            "FQR timestamped deletion Time Constrained callback");
         owner.enableTimeRegulation(ownerEpsilon);
         ownerRegulating = true;
         await(owner, callbackModel,
            () -> ownerCallbacks.timeRegulationEnabled != null,
            "FQR timestamped deletion Time Regulation callback");

         LogicalTime<?, ?> deletionTime = offset(ownerInitial, ownerEpsilon, 4);
         LogicalTime<?, ?> receiverDeletionTime = offset(receiverInitial, receiverEpsilon, 4);
         LogicalTime<?, ?> afterDeletionTime = offset(ownerInitial, ownerEpsilon, 5);
         LogicalTime<?, ?> receiverAfterDeletionTime =
            offset(receiverInitial, receiverEpsilon, 5);
         JavaTckSupport.check(deletionTime.equals(receiverDeletionTime)
               && afterDeletionTime.equals(receiverAfterDeletionTime),
            "FQR timestamped deletion members derived different request times");

         byte[] tag = {0x46, 0x51, 0x52, 0x2d, 0x44};
         MessageRetractionReturn retraction =
            owner.deleteObjectInstance(object, tag.clone(), deletionTime);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "FQR timestamped deletion returned an invalid retraction handle");
         JavaTckSupport.check(receiverCallbacks.removals.isEmpty(),
            "FQR timestamped deletion was delivered before Flush Queue Request");

         receiver.flushQueueRequest(deletionTime);
         await(receiver, callbackModel,
            () -> receiverCallbacks.removals.size() == 1
               && receiverCallbacks.flushGrants.size() == 1,
            "FQR timestamped object deletion and Flush Queue Grant");
         Removal removal = receiverCallbacks.removals.get(0);
         FlushGrant flushGrant = receiverCallbacks.flushGrants.get(0);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("remove", "flush-grant")),
            "FQR granted before delivering the timestamped object removal");
         JavaTckSupport.check(object.equals(removal.object)
               && Arrays.equals(tag, removal.tag) && producer.equals(removal.producer)
               && deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(removal.retraction),
            "FQR returned incorrect timestamped object-removal metadata");
         JavaTckSupport.check(deletionTime.equals(flushGrant.optimisticTime),
            "FQR returned the wrong optimistic time for timestamped object deletion");

         owner.timeAdvanceRequest(afterDeletionTime);
         await(owner, callbackModel,
            () -> afterDeletionTime.equals(ownerCallbacks.timeAdvanceGrant),
            "FQR timestamped deletion producer advance");
         receiver.timeAdvanceRequest(receiverAfterDeletionTime);
         await(receiver, callbackModel,
            () -> receiverAfterDeletionTime.equals(receiverCallbacks.timeAdvanceGrant),
            "FQR timestamped deletion receiver advance");
         JavaTckSupport.check(receiverAfterDeletionTime.equals(receiver.queryLogicalTime()),
            "FQR receiver did not advance past the object-deletion timestamp");
         JavaTckSupport.expectFailure(
            () -> receiver.getObjectInstanceHandle(objectName),
            "ObjectInstanceNotKnown",
            "FQR timestamped object lookup after advancing past deletion time");
         JavaTckSupport.expectFailure(
            () -> receiver.getKnownObjectClassHandle(object),
            "ObjectInstanceNotKnown",
            "FQR timestamped known-object lookup after advancing past deletion time");
      } finally {
         if (ownerRegulating && ownerJoined) {
            try { owner.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (receiverSubscribed && receiverJoined && receiverClass != null
               && receiverAttributes != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverClass, receiverAttributes); }
            catch (Exception ignored) { }
         }
         if (ownerPublished && ownerJoined && ownerClass != null && ownerAttributes != null) {
            try { owner.unpublishObjectClassAttributes(ownerClass, ownerAttributes); }
            catch (Exception ignored) { }
         }
         if (receiverJoined) {
            try { receiver.resignFederationExecution(ResignAction.NO_ACTION); }
            catch (Exception ignored) { }
         }
         if (ownerJoined) {
            try { owner.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST); }
            catch (Exception ignored) { }
         }
         if (created) {
            try { owner.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) {
            try { receiver.disconnect(); } catch (Exception ignored) { }
         }
         if (ownerConnected) {
            try { owner.disconnect(); } catch (Exception ignored) { }
         }
      }
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start, LogicalTimeInterval interval, int steps)
         throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            ambassador.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
      }
      JavaTckSupport.check(condition.getAsBoolean(),
         "timed out waiting for " + description);
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Removal(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         tag = ((byte[]) arguments[1]).clone();
         producer = (FederateHandle) arguments[2];
         time = (LogicalTime<?, ?>) arguments[3];
         sentOrder = (OrderType) arguments[4];
         receivedOrder = (OrderType) arguments[5];
         retraction = (MessageRetractionHandle) arguments[6];
      }
   }

   private static final class FlushGrant {
      private final LogicalTime<?, ?> optimisticTime;

      private FlushGrant(Object[] arguments) {
         optimisticTime = (LogicalTime<?, ?>) arguments[1];
      }
   }

   private static final class Recorder {
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<FlushGrant> flushGrants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile ObjectInstanceHandle discoveredObject;
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeConstrainedEnabled;
      private volatile LogicalTime<?, ?> timeAdvanceGrant;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveredObject = (ObjectInstanceHandle) arguments[0];
               } else if ("timeRegulationEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  removals.add(new Removal(arguments));
                  callbackOrder.add("remove");
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushGrants.add(new FlushGrant(arguments));
                  callbackOrder.add("flush-grant");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeAdvanceGrant = (LogicalTime<?, ?>) arguments[0];
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK FQR object-deletion callbacks";
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
