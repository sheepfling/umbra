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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Verifies a timestamped attribute update is delivered before a later object deletion. */
final class TimestampedUpdateBeforeDeleteTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimestampedUpdateBeforeDeleteTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         CallbackModel callbackModel) throws Exception {
      RTIambassador owner = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder ownerCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-timestamped-update-before-delete-" + UUID.randomUUID();
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
            "java-tck-update-before-delete-owner", federation);
         ownerJoined = true;
         receiver.joinFederationExecution(
            "java-tck-update-before-delete-receiver", federation);
         receiverJoined = true;

         String objectClassName = "HLAobjectRoot.TckObject";
         String attributeName = "Value";
         ownerClass = owner.getObjectClassHandle(objectClassName);
         receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle ownerAttribute = owner.getAttributeHandle(ownerClass, attributeName);
         AttributeHandle receiverAttribute =
            receiver.getAttributeHandle(receiverClass, attributeName);
         JavaTckSupport.check(ownerClass != null && receiverClass != null
               && ownerClass.equals(receiverClass) && ownerAttribute != null
               && ownerAttribute.equals(receiverAttribute),
            "timestamped update-before-delete FOM returned inconsistent object handles");
         ownerAttributes = JavaTckSupport.attributeSet(owner, ownerAttribute);
         receiverAttributes = JavaTckSupport.attributeSet(receiver, receiverAttribute);
         owner.publishObjectClassAttributes(ownerClass, ownerAttributes);
         ownerPublished = true;
         receiver.subscribeObjectClassAttributes(receiverClass, receiverAttributes);
         receiverSubscribed = true;

         ObjectInstanceHandle object = owner.registerObjectInstance(ownerClass);
         JavaTckSupport.check(object != null,
            "timestamped update-before-delete returned no object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object),
            "timestamped update-before-delete object discovery");

         LogicalTimeFactory<?, ?> ownerFactory = owner.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(ownerFactory.getName().equals(receiverFactory.getName()),
            "timestamped update-before-delete members selected different logical-time factories");
         LogicalTime<?, ?> ownerInitial = owner.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(ownerInitial.equals(receiverInitial),
            "timestamped update-before-delete members began at different logical times");
         LogicalTimeInterval ownerEpsilon = (LogicalTimeInterval) ownerFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTime<?, ?> updateTime = offset(ownerInitial, ownerEpsilon, 5);
         LogicalTime<?, ?> receiverUpdateTime = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime<?, ?> deletionTime = offset(ownerInitial, ownerEpsilon, 6);
         LogicalTime<?, ?> receiverDeletionTime = offset(receiverInitial, receiverEpsilon, 6);
         JavaTckSupport.check(updateTime.equals(receiverUpdateTime)
               && deletionTime.equals(receiverDeletionTime)
               && compare(updateTime, deletionTime) <= 0,
            "timestamped update-before-delete members derived inconsistent event times");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         await(receiver, callbackModel,
            () -> receiverCallbacks.timeConstrainedEnabled != null,
            "timestamped update-before-delete Time Constrained callback");
         owner.enableTimeRegulation(ownerEpsilon);
         ownerRegulating = true;
         await(owner, callbackModel,
            () -> ownerCallbacks.timeRegulationEnabled != null,
            "timestamped update-before-delete Time Regulation callback");

         byte[] value = {0x55, 0x50, 0x44};
         byte[] updateTag = {0x55, 0x50, 0x44};
         byte[] deletionTag = {0x44, 0x45, 0x4c};
         AttributeHandleValueMap values = owner.getAttributeHandleValueMapFactory().create(1);
         values.put(ownerAttribute, value.clone());
         MessageRetractionReturn updateRetraction = owner.updateAttributeValues(
            object, values, updateTag.clone(), updateTime);
         MessageRetractionReturn deletionRetraction = owner.deleteObjectInstance(
            object, deletionTag.clone(), deletionTime);
         JavaTckSupport.check(validRetraction(updateRetraction)
               && validRetraction(deletionRetraction),
            "timestamped update-before-delete returned an invalid retraction designator");
         JavaTckSupport.check(receiverCallbacks.reflections.isEmpty()
               && receiverCallbacks.removals.isEmpty(),
            "timestamped update-before-delete delivered a timestamped callback before FQR");

         receiver.flushQueueRequest(deletionTime);
         await(receiver, callbackModel,
            () -> receiverCallbacks.reflections.size() == 1
               && receiverCallbacks.removals.size() == 1
               && receiverCallbacks.flushGrants.size() == 1,
            "timestamped update-before-delete callbacks and Flush Queue Grant");
         Reflection reflection = receiverCallbacks.reflections.get(0);
         Removal removal = receiverCallbacks.removals.get(0);
         JavaTckSupport.check(receiverCallbacks.callbackOrder.equals(
               Arrays.asList("reflect", "remove", "flush-grant")),
            "timestamped update-before-delete did not deliver the update before object removal");
         JavaTckSupport.check(receiverCallbacks.ordinaryReflections.isEmpty()
               && reflection.object.equals(object) && reflection.values.size() == 1
               && Arrays.equals(value, reflection.values.get(receiverAttribute))
               && Arrays.equals(updateTag, reflection.tag) && producer.equals(reflection.producer)
               && updateTime.equals(reflection.time)
               && reflection.sentOrder == OrderType.TIMESTAMP
               && reflection.receivedOrder == OrderType.TIMESTAMP
               && updateRetraction.handle.equals(reflection.retraction),
            "timestamped update-before-delete changed the update payload or metadata");
         JavaTckSupport.check(receiverCallbacks.ordinaryRemovals.isEmpty()
               && removal.object.equals(object) && Arrays.equals(deletionTag, removal.tag)
               && producer.equals(removal.producer) && deletionTime.equals(removal.time)
               && removal.sentOrder == OrderType.TIMESTAMP
               && removal.receivedOrder == OrderType.TIMESTAMP
               && deletionRetraction.handle.equals(removal.retraction),
            "timestamped update-before-delete changed the deletion metadata");
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
            try { owner.resignFederationExecution(ResignAction.NO_ACTION); }
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

   private static boolean validRetraction(MessageRetractionReturn result) {
      return result != null && result.retractionHandleIsValid && result.handle != null;
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static int compare(LogicalTime first, LogicalTime second) {
      return first.compareTo(second);
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

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
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

   private static final class Recorder {
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Reflection> reflections = new CopyOnWriteArrayList<>();
      private final List<Object[]> ordinaryReflections = new CopyOnWriteArrayList<>();
      private final List<Removal> removals = new CopyOnWriteArrayList<>();
      private final List<Object[]> ordinaryRemovals = new CopyOnWriteArrayList<>();
      private final List<Object> flushGrants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();
      private volatile LogicalTime<?, ?> timeRegulationEnabled;
      private volatile LogicalTime<?, ?> timeConstrainedEnabled;

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length > 0) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeRegulationEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("timeConstrainedEnabled".equals(name)
                     && arguments != null && arguments.length == 1) {
                  timeConstrainedEnabled = (LogicalTime<?, ?>) arguments[0];
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  reflections.add(new Reflection(arguments));
                  callbackOrder.add("reflect");
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 6) {
                  ordinaryReflections.add(arguments.clone());
                  callbackOrder.add("reflect");
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  removals.add(new Removal(arguments));
                  callbackOrder.add("remove");
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 3) {
                  ordinaryRemovals.add(arguments.clone());
                  callbackOrder.add("remove");
               } else if ("flushQueueGrant".equals(name)
                     && arguments != null && arguments.length == 2) {
                  flushGrants.add(arguments[1]);
                  callbackOrder.add("flush-grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK update-before-delete callbacks";
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
