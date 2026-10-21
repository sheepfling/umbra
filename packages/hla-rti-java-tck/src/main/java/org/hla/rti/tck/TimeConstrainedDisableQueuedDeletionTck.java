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
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Public-API queued and subsequent deletion TSO-to-RO witness after disabling Time Constrained. */
final class TimeConstrainedDisableQueuedDeletionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedDisableQueuedDeletionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-disable-deletion-"
         + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-disable-deletion-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-disable-deletion-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherClass.equals(receiverClass) && publisherAttribute != null
               && publisherAttribute.equals(receiverAttribute),
            "Time Constrained deletion disable FOM did not resolve stable object handles");
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         ObjectInstanceHandle queuedObject = publisher.registerObjectInstance(publisherClass);
         ObjectInstanceHandle laterObject = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(queuedObject != null && laterObject != null
               && !queuedObject.equals(laterObject),
            "Time Constrained deletion disable did not register two distinct objects");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(queuedObject)
               && receiverCallbacks.discoveries.contains(laterObject),
            "Time Constrained deletion disable discovery of both objects");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained deletion disable members selected different time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTime<?, ?> queuedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 3);
         publisher.enableTimeRegulation(epsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "publisher Time Regulation Enabled");
         receiver.enableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "receiver Time Constrained Enabled");

         byte[] queuedTag = {0x54, 0x43, 0x44, (byte) 0xd1};
         byte[] laterTag = {0x54, 0x43, 0x44, (byte) 0xd2};
         checkRetraction(publisher.deleteObjectInstance(
            queuedObject, queuedTag.clone(), queuedTime),
            "queued timestamped deletion");
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverCallbacks.untimed.isEmpty()
               && receiverCallbacks.timed.isEmpty(),
            "future object removal escaped before Time Constrained was disabled");

         receiver.disableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.untimed.size() == 1,
            "queued object removal as receive order after disable");
         Removal queued = receiverCallbacks.untimed.get(0);
         JavaTckSupport.check(receiverCallbacks.timed.isEmpty()
               && queuedObject.equals(queued.object)
               && Arrays.equals(queuedTag, queued.tag)
               && publisherHandle.equals(queued.producer),
            "queued deletion did not retain its object, tag, and producer in receive order");

         checkRetraction(publisher.deleteObjectInstance(
            laterObject, laterTag.clone(), laterTime),
            "subsequent timestamped deletion");
         await(receiver, callbackModel,
            () -> receiverCallbacks.timed.size() == 1,
            "subsequent timestamped object removal after disable");
         Removal subsequent = receiverCallbacks.timed.get(0);
         JavaTckSupport.check(receiverCallbacks.untimed.size() == 1
               && laterObject.equals(subsequent.object)
               && Arrays.equals(laterTag, subsequent.tag)
               && publisherHandle.equals(subsequent.producer)
               && laterTime.equals(subsequent.time)
               && subsequent.sentOrder == OrderType.TIMESTAMP
               && subsequent.receivedOrder == OrderType.RECEIVE,
            "subsequent timestamped deletion did not retain receive-order delivery");

         publisher.disableTimeRegulation();
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (publisherJoined) resign(publisher, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkRetraction(MessageRetractionReturn result, String phase) {
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null, phase + " returned no valid retraction handle");
   }

   @SuppressWarnings({"rawtypes", "unchecked"})
   private static LogicalTime offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(RTIambassador ambassador, CallbackModel model,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); }
      catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class Removal {
      private final ObjectInstanceHandle object;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Removal(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         tag = ((byte[]) arguments[1]).clone();
         producer = (FederateHandle) arguments[2];
         if (arguments.length == 7) {
            time = (LogicalTime<?, ?>) arguments[3];
            sentOrder = (OrderType) arguments[4];
            receivedOrder = (OrderType) arguments[5];
         } else {
            time = null;
            sentOrder = null;
            receivedOrder = OrderType.RECEIVE;
         }
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Removal> untimed = new CopyOnWriteArrayList<>();
      private final List<Removal> timed = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("discoverObjectInstance".equals(name)
                     && arguments != null && arguments.length >= 1) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("timeRegulationEnabled".equals(name)) {
                  regulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  constrainedEnabled++;
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 3) {
                  untimed.add(new Removal(arguments));
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  timed.add(new Removal(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Time Constrained deletion disable callbacks";
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
