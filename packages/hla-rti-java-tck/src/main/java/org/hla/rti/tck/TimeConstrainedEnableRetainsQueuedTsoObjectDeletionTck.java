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
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.BooleanSupplier;

/** Checks a timestamped object deletion queued across pending Time Constrained enablement. */
final class TimeConstrainedEnableRetainsQueuedTsoObjectDeletionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedEnableRetainsQueuedTsoObjectDeletionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-queued-object-deletion-" + UUID.randomUUID();
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
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-queued-delete-tso-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-queued-delete-tso-receiver", federation);
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
            "Time Constrained queued-object-deletion FOM did not resolve stable object handles");
         AttributeHandleSet published = publisher.getAttributeHandleSetFactory().create();
         published.add(publisherAttribute);
         AttributeHandleSet subscribed = receiver.getAttributeHandleSetFactory().create();
         subscribed.add(receiverAttribute);
         publisher.publishObjectClassAttributes(publisherClass, published);
         receiver.subscribeObjectClassAttributes(receiverClass, subscribed);
         ObjectInstanceHandle object = publisher.registerObjectInstance(publisherClass);
         JavaTckSupport.check(object != null,
            "Time Constrained queued-object-deletion registration returned no object handle");
         await(receiver, callbackModel,
            () -> receiverCallbacks.discoveries.contains(object),
            "Time Constrained queued-object-deletion discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "Time Constrained queued-object-deletion members selected different time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "Time Constrained queued-object-deletion members began at different times");
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
            "Time Constrained queued-object-deletion members use different time values");

         publisher.enableTimeRegulation(publisherEpsilon);
         await(publisher, callbackModel,
            () -> publisherCallbacks.regulationEnabled == 1,
            "Time Constrained queued-object-deletion publisher regulation");
         receiver.timeAdvanceRequest(receiverFive);
         await(receiver, callbackModel,
            () -> receiverCallbacks.grants.size() == 1,
            "Time Constrained queued-object-deletion unconstrained grant");
         JavaTckSupport.check(receiverFive.equals(receiverCallbacks.grants.get(0)),
            "Time Constrained queued-object-deletion receiver did not reach time five");

         receiver.enableTimeConstrained();
         serviceOnce(receiver, callbackModel);
         JavaTckSupport.check(receiverCallbacks.constrainedEnabled == 0,
            "Time Constrained enabled before the publisher advanced beyond receiver time");
         byte[] tag = {0x54, 0x43, 0x45, 0x2d, 0x58};
         MessageRetractionReturn retraction = publisher.deleteObjectInstance(
            object, tag.clone(), publisherSix);
         JavaTckSupport.check(retraction != null && retraction.retractionHandleIsValid
               && retraction.handle != null,
            "Time Constrained queued-object deletion returned no valid retraction handle");

         publisher.timeAdvanceRequest(publisherFour);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 1,
            "Time Constrained queued-object-deletion publisher grant to four");
         publisher.timeAdvanceRequest(publisherFive);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 2,
            "Time Constrained queued-object-deletion publisher grant to five");
         await(receiver, callbackModel,
            () -> receiverCallbacks.constrainedEnabled == 1,
            "Time Constrained queued-object deletion role transition");
         List<Removal> removalsAtTransition =
            new ArrayList<>(receiverCallbacks.timedRemovals);
         List<String> callbackOrderAtTransition =
            new ArrayList<>(receiverCallbacks.callbackOrder);
         int constrainedEnableOrder =
            callbackOrderAtTransition.indexOf("time-constrained-enabled");
         JavaTckSupport.check(constrainedEnableOrder >= 0,
            "callback recorder missed Time Constrained Enabled");
         JavaTckSupport.check(receiverCallbacks.simpleRemovals == 0,
            "the stored timestamped deletion used an unexpected non-timestamp callback form");
         if (!removalsAtTransition.isEmpty()) {
            JavaTckSupport.check(removalsAtTransition.size() == 1
                  && removalsAtTransition.get(0).receivedOrder == OrderType.RECEIVE
                  && callbackOrderAtTransition.indexOf("remove") < constrainedEnableOrder,
               "the deletion was not delivered as RECEIVE before Time Constrained Enabled");
         }
         boolean removalDeliveredAtTransition = !removalsAtTransition.isEmpty();
         List<String> callbackOrderBeforeTsoAdvance =
            new ArrayList<>(callbackOrderAtTransition);
         publisher.timeAdvanceRequest(publisherSix);
         await(publisher, callbackModel,
            () -> publisherCallbacks.grants.size() == 3,
            "Time Constrained queued-object-deletion publisher grant to six");
         JavaTckSupport.check(publisherSix.equals(publisherCallbacks.grants.get(2)),
            "Time Constrained queued-object-deletion publisher did not reach time six");
         receiver.timeAdvanceRequest(receiverSix);
         await(receiver, callbackModel,
            () -> receiverCallbacks.timedRemovals.size() == 1
               && receiverCallbacks.grants.size() == 2,
            "Time Constrained queued-object deletion before grant six");
         Removal event = receiverCallbacks.timedRemovals.get(0);
         JavaTckSupport.check(receiverCallbacks.simpleRemovals == 0
               && receiverCallbacks.timedRemovals.size() == 1
               && object.equals(event.object)
               && Arrays.equals(tag, event.tag)
               && producer.equals(event.producer)
               && receiverSix.equals(event.time)
               && event.sentOrder == OrderType.TIMESTAMP
               && event.receivedOrder == (removalDeliveredAtTransition
                  ? OrderType.RECEIVE : OrderType.TIMESTAMP)
               && retraction.handle.equals(event.retraction),
            "Queued object deletion lost its object, producer, tag, time, order, or retraction");
         JavaTckSupport.check(receiverSix.equals(receiverCallbacks.grants.get(1)),
            "time-constrained object-deletion receiver did not receive TAR(6) grant");
         if (removalDeliveredAtTransition) {
            JavaTckSupport.check(receiverCallbacks.callbackOrder.size()
                  == callbackOrderBeforeTsoAdvance.size() + 1
                  && "grant".equals(receiverCallbacks.callbackOrder.get(
                     receiverCallbacks.callbackOrder.size() - 1)),
               "the Time Constrained Enabled-time deletion was not followed only by TAR(6) grant");
         } else {
            JavaTckSupport.check(receiverCallbacks.callbackOrder.size()
                  == callbackOrderBeforeTsoAdvance.size() + 2
                  && "remove".equals(receiverCallbacks.callbackOrder.get(
                     callbackOrderBeforeTsoAdvance.size()))
                  && "grant".equals(receiverCallbacks.callbackOrder.get(
                     receiverCallbacks.callbackOrder.size() - 1)),
               "queued timestamped object removal did not precede its time-advance grant");
         }

         receiver.disableTimeConstrained();
         publisher.disableTimeRegulation();
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverJoined) resign(receiver, ResignAction.NO_ACTION);
         if (publisherJoined) {
            resign(publisher, ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         }
         if (created) {
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (receiverConnected) disconnect(receiver);
         if (publisherConnected) disconnect(publisher);
      }
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
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private volatile int simpleRemovals;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<Removal> timedRemovals = new CopyOnWriteArrayList<>();
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
                  callbackOrder.add("time-constrained-enabled");
               } else if ("timeAdvanceGrant".equals(name)
                     && arguments != null && arguments.length == 1) {
                  callbackOrder.add("grant");
                  grants.add((LogicalTime<?, ?>) arguments[0]);
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 3) {
                  simpleRemovals++;
                  callbackOrder.add("remove");
               } else if ("removeObjectInstance".equals(name)
                     && arguments != null && arguments.length == 7) {
                  timedRemovals.add(new Removal(arguments));
                  callbackOrder.add("remove");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued-object-deletion callbacks";
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
