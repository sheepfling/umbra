package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.AttributeHandleValueMap;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
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

/** Checks a timestamped attribute update queued across Time Constrained enablement. */
final class TimeConstrainedQueuedAttributeTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedQueuedAttributeTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-queued-attribute-" + UUID.randomUUID();
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
         publisher.joinFederationExecution("java-tck-queued-attribute-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-queued-attribute-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publisherClass = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle receiverClass = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publisherAttribute = publisher.getAttributeHandle(
            publisherClass, attributeName);
         AttributeHandle receiverAttribute = receiver.getAttributeHandle(
            receiverClass, attributeName);
         JavaTckSupport.check(publisherClass != null && receiverClass != null
               && publisherAttribute != null && receiverAttribute != null
               && publisherClass.equals(receiverClass)
               && publisherAttribute.equals(receiverAttribute),
            "Time Constrained queued-attribute FOM did not resolve stable object handles");
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         publisher.changeDefaultAttributeOrderType(publisherClass, published, OrderType.TIMESTAMP);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "Time Constrained queued-attribute registration returned no object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object),
            "Time Constrained queued-attribute discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained queued-attribute members selected different time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Time Constrained queued-attribute members began at different times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverFactory.makeEpsilon();
         LogicalTime<?, ?> receiverFive = offset(receiverInitial, receiverEpsilon, 5);
         LogicalTime<?, ?> publisherFour = offset(publisherInitial, publisherEpsilon, 4);
         LogicalTime<?, ?> publisherFive = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> publisherSix = offset(publisherInitial, publisherEpsilon, 6);
         LogicalTime<?, ?> receiverSix = offset(receiverInitial, receiverEpsilon, 6);
         JavaTckSupport.check(publisherFive.equals(receiverFive)
               && publisherSix.equals(receiverSix),
            "Time Constrained queued-attribute members use different time values");

         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "Time Constrained queued-attribute publisher regulation");
         receiver.timeAdvanceRequest(receiverFive);
         await(receiver, callbackModel,
            () -> receiverCallbacks.grants.size() == 1,
            "Time Constrained queued-attribute unconstrained grant");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.grants.get(0)),
            "Time Constrained queued-attribute receiver did not reach time five");

         receiver.enableTimeConstrained();
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained enabled before the publisher advanced beyond receiver time");
         byte[] value = {0x54, 0x53, 0x4f};
         byte[] tag = {0x54, 0x43, 0x45, 0x2d, 0x41};
         AttributeHandleValueMap values =
            publisher.getAttributeHandleValueMapFactory().create(1);
         values.put(publisherAttribute, value.clone());
         MessageRetractionReturn retraction = publisher.updateAttributeValues(
            object, values, tag.clone(), publisherSix);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Time Constrained queued-attribute update returned no valid retraction handle");
         serviceOnce(receiver, callbackModel);
         checkNoReflection(receiverCallbacks, "while enablement was pending");

         publisher.timeAdvanceRequest(publisherFour);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 1,
            "Time Constrained queued-attribute publisher grant to four");
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained enabled while publisher frontier was at receiver time");
         checkNoReflection(receiverCallbacks, "while GALT remained at receiver time");

         publisher.timeAdvanceRequest(publisherFive);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 2,
            "Time Constrained queued-attribute publisher grant to five");
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "Time Constrained queued-attribute role transition");
         checkNoReflection(receiverCallbacks, "after enablement but before TAR(6)");
         int orderBeforeAdvance = receiverCallbacks.callbackOrder.size();
         receiver.timeAdvanceRequest(receiverSix);
         await(receiver, callbackModel,
            () -> receiverCallbacks.timedReflections.size() == 1
               && receiverCallbacks.grants.size() == 2,
            "Time Constrained queued-attribute reflection before grant six");
         Reflection event = receiverCallbacks.timedReflections.get(0);
         JavaTckSupport.check(receiverCallbacks.receiveOrderReflections == 0
               && object.equals(event.object)
               && event.values.size() == 1
               && Arrays.equals(value, event.values.get(receiverAttribute))
               && Arrays.equals(tag, event.tag)
               && receiverSix.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == OrderType.TIMESTAMP
               && retraction.handle.equals(event.retraction),
            "Queued attribute update did not retain timestamp-order reflection metadata");
         JavaTckSupport.check(receiverSix.equals(receiverCallbacks.grants.get(1))
               && receiverCallbacks.callbackOrder.size() == orderBeforeAdvance + 2
               && "reflect".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance))
               && "grant".equals(receiverCallbacks.callbackOrder.get(orderBeforeAdvance + 1)),
            "Queued timestamped attribute reflection did not precede TAR(6)'s grant");

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver);
         if (publisherJoined) resign(publisher);
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static void checkNoReflection(Recorder recorder, String phase) {
      JavaTckSupport.check(recorder.receiveOrderReflections == 0
            && recorder.timedReflections.isEmpty(),
         "Time Constrained queued-attribute reflection escaped " + phase);
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

   private static void serviceOnce(RTIambassador ambassador, CallbackModel model)
         throws Exception {
      if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
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

   private static void resign(RTIambassador ambassador) {
      try { ambassador.resignFederationExecution(ResignAction.NO_ACTION); }
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
      private final MessageRetractionHandle retraction;

      private Reflection(Object[] arguments) {
         object = (ObjectInstanceHandle) arguments[0];
         values = ((AttributeHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int receiveOrderReflections;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<Reflection> timedReflections = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

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
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 6) {
                  receiveOrderReflections++;
               } else if ("reflectAttributeValues".equals(name)
                     && arguments != null && arguments.length == 10) {
                  Reflection event = new Reflection(arguments);
                  callbackOrder.add("reflect");
                  timedReflections.add(event);
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued-attribute callbacks";
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
