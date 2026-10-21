package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.ResignAction;
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

/** Public-API directed-interaction queued and subsequent TSO-to-RO witness. */
final class TimeConstrainedDisableQueuedDirectedTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedDisableQueuedDirectedTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionName,
         CallbackModel model) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-disable-directed-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      try {
         publisher.connect(publisherCallbacks.proxy(), model);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), model);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "java-tck-disable-directed-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("java-tck-disable-directed-receiver", federation);
         receiverJoined = true;

         ObjectClassHandle publishedObject = publisher.getObjectClassHandle(objectClassName);
         ObjectClassHandle subscribedObject = receiver.getObjectClassHandle(objectClassName);
         AttributeHandle publishedAttribute = publisher.getAttributeHandle(
            publishedObject, attributeName);
         AttributeHandle subscribedAttribute = receiver.getAttributeHandle(
            subscribedObject, attributeName);
         InteractionClassHandle publishedInteraction =
            publisher.getInteractionClassHandle(interactionName);
         InteractionClassHandle subscribedInteraction =
            receiver.getInteractionClassHandle(interactionName);
         JavaTckSupport.check(publishedObject != null && subscribedObject != null
               && publishedObject.equals(subscribedObject)
               && publishedAttribute != null
               && publishedAttribute.equals(subscribedAttribute)
               && publishedInteraction != null
               && publishedInteraction.equals(subscribedInteraction),
            "directed Time Constrained disable could not resolve adapter FOM handles");
         AttributeHandleSet publishedAttributes =
            JavaTckSupport.attributeSet(publisher, publishedAttribute);
         AttributeHandleSet subscribedAttributes =
            JavaTckSupport.attributeSet(receiver, subscribedAttribute);
         publisher.publishObjectClassAttributes(publishedObject, publishedAttributes);
         receiver.subscribeObjectClassAttributes(subscribedObject, subscribedAttributes);
         publisher.publishObjectClassDirectedInteractions(publishedObject,
            JavaTckSupport.interactionSet(publisher, publishedInteraction));
         receiver.subscribeObjectClassDirectedInteractions(subscribedObject,
            JavaTckSupport.interactionSet(receiver, subscribedInteraction));
         publisher.changeInteractionOrderType(publishedInteraction, OrderType.TIMESTAMP);
         ObjectInstanceHandle target = publisher.registerObjectInstance(publishedObject);
         JavaTckSupport.check(target != null, "directed target registration was invalid");
         await(receiver, model, () -> receiverCallbacks.discoveries.contains(target),
            "directed target discovery");

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(receiverFactory.getName()),
            "directed members selected different time factories");
         LogicalTime<?, ?> initial = publisher.queryLogicalTime();
         LogicalTimeInterval epsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTime<?, ?> queuedTime = offset(initial, epsilon, 5);
         LogicalTime<?, ?> laterTime = offset(initial, epsilon, 3);
         publisher.enableTimeRegulation(epsilon);
         await(publisher, model, () -> publisherCallbacks.regulationEnabled == 1,
            "publisher Time Regulation Enabled");
         receiver.enableTimeConstrained();
         await(receiver, model, () -> receiverCallbacks.constrainedEnabled == 1,
            "receiver Time Constrained Enabled");

         byte[] queuedTag = {0x72, (byte) 0xb1, 0x1c};
         byte[] laterTag = {0x72, (byte) 0xb1, 0x1d};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(0);
         checkRetraction(publisher.sendDirectedInteraction(publishedInteraction,
            target, parameters, queuedTag.clone(), queuedTime),
            "queued timestamped directed interaction");
         if (model == CallbackModel.HLA_EVOKED) receiver.evokeCallback(0.0);
         JavaTckSupport.check(receiverCallbacks.untimed.isEmpty()
               && receiverCallbacks.timed.isEmpty(),
            "future directed interaction escaped before Time Constrained was disabled");

         receiver.disableTimeConstrained();
         await(receiver, model, () -> receiverCallbacks.untimed.size() == 1,
            "queued directed interaction in receive order after disable");
         Delivery queued = receiverCallbacks.untimed.get(0);
         TransportationTypeHandle reliable =
            receiver.getTransportationTypeHandle("HLAreliable");
         JavaTckSupport.check(receiverCallbacks.timed.isEmpty()
               && subscribedInteraction.equals(queued.interaction)
               && target.equals(queued.object)
               && queued.parameters.isEmpty()
               && Arrays.equals(queuedTag, queued.tag)
               && producer.equals(queued.producer)
               && reliable.equals(queued.transportation),
            "queued directed interaction lost target, tag, producer, or transport in receive order");

         checkRetraction(publisher.sendDirectedInteraction(publishedInteraction,
            target, parameters, laterTag.clone(), laterTime),
            "subsequent timestamped directed interaction");
         await(receiver, model, () -> receiverCallbacks.timed.size() == 1,
            "subsequent timestamped directed interaction after disable");
         Delivery subsequent = receiverCallbacks.timed.get(0);
         JavaTckSupport.check(receiverCallbacks.untimed.size() == 1
               && subscribedInteraction.equals(subsequent.interaction)
               && target.equals(subsequent.object)
               && subsequent.parameters.isEmpty()
               && Arrays.equals(laterTag, subsequent.tag)
               && producer.equals(subsequent.producer)
               && laterTime.equals(subsequent.time)
               && subsequent.sentOrder == OrderType.TIMESTAMP
               && subsequent.receivedOrder == OrderType.RECEIVE,
            "subsequent directed interaction did not retain receive-order delivery");

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
      for (int index = 0; index < steps; ++index) result = (LogicalTime) result.add(interval);
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
      try { ambassador.disconnect(); }
      catch (Exception ignored) { }
   }

   private static final class Delivery {
      private final InteractionClassHandle interaction;
      private final ObjectInstanceHandle object;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;

      private Delivery(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         object = (ObjectInstanceHandle) arguments[1];
         parameters = ((ParameterHandleValueMap) arguments[2]).clone();
         tag = ((byte[]) arguments[3]).clone();
         transportation = (TransportationTypeHandle) arguments[4];
         producer = (FederateHandle) arguments[5];
         time = arguments.length == 10 ? (LogicalTime<?, ?>) arguments[6] : null;
         sentOrder = arguments.length == 10 ? (OrderType) arguments[7] : null;
         receivedOrder = arguments.length == 10 ? (OrderType) arguments[8] : null;
      }
   }

   private static final class Recorder {
      private volatile int regulationEnabled;
      private volatile int constrainedEnabled;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<Delivery> untimed = new CopyOnWriteArrayList<>();
      private final List<Delivery> timed = new CopyOnWriteArrayList<>();

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
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 6) {
                  untimed.add(new Delivery(arguments));
               } else if ("receiveDirectedInteraction".equals(name)
                     && arguments != null && arguments.length == 10) {
                  timed.add(new Delivery(arguments));
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) return "Java TCK queued directed callbacks";
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
