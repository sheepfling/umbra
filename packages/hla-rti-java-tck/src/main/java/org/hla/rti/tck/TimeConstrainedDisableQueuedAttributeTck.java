package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Public-API queued and subsequent attribute TSO-to-RO witness after disabling Time Constrained. */
final class TimeConstrainedDisableQueuedAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedDisableQueuedAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-disable-attribute-"
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
         publisher.joinFederationExecution("java-tck-disable-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-disable-attribute-receiver", federation);
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
            "Time Constrained attribute disable FOM did not resolve stable object handles");
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         publisher.changeDefaultAttributeOrderType(
            publisherClass, published, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "Time Constrained attribute disable registration returned no object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object),
            "Time Constrained attribute disable object discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained attribute disable members selected different time factories");
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

         byte[] value = {0x63, 0x64};
         byte[] queuedTag = {0x65};
         byte[] laterTag = {0x66};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         checkRetraction(publisher.updateAttributeValues(
            object, values, queuedTag.clone(), queuedTime),
            "queued timestamped attribute update");
         if (callbackModel == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverCallbacks.untimed.isEmpty()
               && receiverCallbacks.timed.isEmpty(),
            "future attribute reflection escaped before Time Constrained was disabled");

         receiver.disableTimeConstrained();
         await(receiver, callbackModel,
            () -> receiverCallbacks.untimed.size() == 1,
            "queued attribute reflection as receive order after disable");
         Reflection queued = receiverCallbacks.untimed.get(0);
         JavaTckSupport.check(receiverCallbacks.timed.isEmpty()
               && object.equals(queued.object)
               && queued.values.size() == 1
               && Arrays.equals(value, queued.values.get(receiverAttribute))
               && Arrays.equals(queuedTag, queued.tag),
            "queued attribute update did not retain its object, value, and tag in receive order");

         checkRetraction(publisher.updateAttributeValues(
            object, values, laterTag.clone(), laterTime),
            "subsequent timestamped attribute update");
         await(receiver, callbackModel,
            () -> receiverCallbacks.timed.size() == 1,
            "subsequent timestamped attribute reflection after disable");
         Reflection subsequent = receiverCallbacks.timed.get(0);
         JavaTckSupport.check(receiverCallbacks.untimed.size() == 1
               && object.equals(subsequent.object)
               && subsequent.values.size() == 1
               && Arrays.equals(value, subsequent.values.get(receiverAttribute))
               && Arrays.equals(laterTag, subsequent.tag)
               && laterTime.equals(subsequent.time)
               && subsequent.sentOrder == OrderType.TIMESTAMP
               && subsequent.receivedOrder == OrderType.RECEIVE,
            "subsequent timestamped attribute update did not retain receive-order delivery");

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

   private static final class Reflection {
      private final ObjectInstanceHandle object;
      private final AttributeHandleValueMap values;
      private final byte[] tag;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         if (arguments.length == 10) {
            time = (LogicalTime<?, ?>) arguments[6];
            sentOrder = (OrderType) arguments[7];
            receivedOrder = (OrderType) arguments[8];
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
      private final List<Reflection> untimed = new CopyOnWriteArrayList<>();
      private final List<Reflection> timed = new CopyOnWriteArrayList<>();

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
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 6) {
                  untimed.add(new Reflection(arguments));
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timed.add(new Reflection(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK Time Constrained attribute disable callbacks";
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
