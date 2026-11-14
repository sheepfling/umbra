package org.hla.rti.tck;

import hla.rti1516_2025.AttributeHandle;
import hla.rti1516_2025.AttributeHandleSet;
import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.ObjectClassHandle;
import hla.rti1516_2025.ObjectInstanceHandle;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandleValueMap;
import hla.rti1516_2025.ResignAction;
import hla.rti1516_2025.RTIambassador;
import hla.rti1516_2025.RtiFactory;
import hla.rti1516_2025.TransportationTypeHandle;
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

/** Checks that a directed TSO stays queued while Time Constrained enablement is pending. */
final class TimeConstrainedEnableRetainsQueuedTsoDirectedInteractionTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private TimeConstrainedEnableRetainsQueuedTsoDirectedInteractionTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String objectClassName, String attributeName, String interactionClassName,
         CallbackModel callbackModel) throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador receiver = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder receiverCallbacks = new Recorder();
      String federation = "java-tck-time-constrained-pending-directed-tso-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean receiverConnected = false;
      boolean created = false;
      boolean publisherJoined = false;
      boolean receiverJoined = false;
      boolean publisherAttributesPublished = false;
      boolean receiverAttributesSubscribed = false;
      boolean publisherDirectedPublished = false;
      boolean receiverDirectedSubscribed = false;
      boolean receiverConstrained = false;
      boolean publisherRegulating = false;
      ObjectClassHandle publisherObjectClass = null;
      ObjectClassHandle receiverObjectClass = null;
      AttributeHandle publisherAttribute = null;
      AttributeHandle receiverAttribute = null;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle receiverInteraction = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         receiver.connect(receiverCallbacks.proxy(), callbackModel);
         receiverConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         created = true;
         FederateHandle producer = publisher.joinFederationExecution(
            "queued-directed-tso-publisher", federation);
         publisherJoined = true;
         receiver.joinFederationExecution("queued-directed-tso-receiver", federation);
         receiverJoined = true;

         publisherObjectClass = publisher.getObjectClassHandle(objectClassName);
         receiverObjectClass = receiver.getObjectClassHandle(objectClassName);
         publisherAttribute = publisher.getAttributeHandle(publisherObjectClass, attributeName);
         receiverAttribute = receiver.getAttributeHandle(receiverObjectClass, attributeName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         receiverInteraction = receiver.getInteractionClassHandle(interactionClassName);
         JavaTckSupport.check(publisherObjectClass != null && receiverObjectClass != null
               && publisherAttribute != null && receiverAttribute != null
               && publisherInteraction != null && receiverInteraction != null,
            "time-constrained directed queued-TSO setup resolved a missing FOM handle");

         AttributeHandleSet publisherAttributes =
            JavaTckSupport.attributeSet(publisher, publisherAttribute);
         AttributeHandleSet receiverAttributes =
            JavaTckSupport.attributeSet(receiver, receiverAttribute);
         publisher.publishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         publisherAttributesPublished = true;
         receiver.subscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         receiverAttributesSubscribed = true;
         publisher.publishObjectClassDirectedInteractions(publisherObjectClass,
            JavaTckSupport.interactionSet(publisher, publisherInteraction));
         publisherDirectedPublished = true;
         receiver.subscribeObjectClassDirectedInteractionsUniversally(receiverObjectClass,
            JavaTckSupport.interactionSet(receiver, receiverInteraction));
         receiverDirectedSubscribed = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         ObjectInstanceHandle target = publisher.registerObjectInstance(publisherObjectClass);
         JavaTckSupport.check(target != null,
            "time-constrained directed queued-TSO registration returned no target handle");
         await(callbackModel, receiver,
            () -> receiverCallbacks.discoveries.contains(target),
            "time-constrained directed queued-TSO target discovery");

         LogicalTimeFactory<?, ?> publisherTimeFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> receiverTimeFactory = receiver.getTimeFactory();
         JavaTckSupport.check(publisherTimeFactory.getName().equals(receiverTimeFactory.getName()),
            "time-constrained directed queued-TSO members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> receiverInitial = receiver.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(receiverInitial),
            "time-constrained directed queued-TSO members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherTimeFactory.makeEpsilon();
         LogicalTimeInterval receiverEpsilon =
            (LogicalTimeInterval) receiverTimeFactory.makeEpsilon();

         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(callbackModel, publisher,
            () -> publisherCallbacks.timeRegulationEnabled == 1,
            "time-constrained directed queued-TSO publisher regulation");

         LogicalTime<?, ?> five = offset(receiverInitial, receiverEpsilon, 5);
         receiver.timeAdvanceRequest(five);
         await(callbackModel, receiver,
            () -> receiverCallbacks.grants.size() == 1,
            "unconstrained directed receiver grant to five");
         JavaTckSupport.check(five.equals(receiverCallbacks.grants.get(0)),
            "unconstrained directed receiver did not reach time five");

         receiver.enableTimeConstrained();
         receiverConstrained = true;
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverCallbacks.timeConstrainedEnabled == 0,
            "directed Time Constrained Enabled fired before the regulator advanced past five");

         LogicalTime<?, ?> six = offset(publisherInitial, publisherEpsilon, 6);
         byte[] tag = new byte[] {0x54, 0x43, 0x45, 0x2D, 0x44};
         ParameterHandleValueMap parameters =
            publisher.getParameterHandleValueMapFactory().create(0);
         MessageRetractionReturn sent = publisher.sendDirectedInteraction(
            publisherInteraction, target, parameters, tag, six);
         JavaTckSupport.check(sent != null,
            "time-constrained directed queued-TSO send returned no result");
         JavaTckSupport.check(sent.retractionHandleIsValid,
            "time-constrained directed queued-TSO send returned an invalid retraction handle");
         JavaTckSupport.check(sent.handle != null,
            "time-constrained directed queued-TSO send returned a null retraction handle");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverCallbacks.directedCallbackCount == 0
               && receiverCallbacks.timeConstrainedEnabled == 0,
            "a timestamped directed interaction escaped while Time Constrained enablement was pending");

         LogicalTime<?, ?> four = offset(publisherInitial, publisherEpsilon, 4);
         publisher.timeAdvanceRequest(four);
         await(callbackModel, publisher,
            () -> publisherCallbacks.grants.size() == 1,
            "directed queued-TSO publisher grant to four");
         JavaTckSupport.drain(receiver);
         JavaTckSupport.check(receiverCallbacks.timeConstrainedEnabled == 0
               && receiverCallbacks.directedCallbackCount == 0,
            "directed role transition or queued TSO completed while GALT equaled receiver time five");

         LogicalTime<?, ?> publisherFive = offset(publisherInitial, publisherEpsilon, 5);
         publisher.timeAdvanceRequest(publisherFive);
         await(callbackModel, publisher,
            () -> publisherCallbacks.grants.size() == 2,
            "directed queued-TSO publisher grant to five");
         await(callbackModel, receiver,
            () -> receiverCallbacks.timeConstrainedEnabled == 1,
            "directed queued-TSO Time Constrained transition");
         JavaTckSupport.check(receiverCallbacks.directedCallbackCount == 0,
            "the directed queued TSO was delivered before the constrained receiver advanced to six");
         List<String> callbackOrderBeforeTsoAdvance =
            new ArrayList<>(receiverCallbacks.callbackOrder);

         LogicalTime<?, ?> receiverSix = offset(receiverInitial, receiverEpsilon, 6);
         receiver.timeAdvanceRequest(receiverSix);
         await(callbackModel, receiver,
            () -> receiverCallbacks.timedDirectedInteractions.size() == 1,
            "directed queued-TSO delivery at six before publisher advance");
         JavaTckSupport.check(receiverCallbacks.grants.size() == 1,
            "directed queued-TSO receiver granted six before the publisher advanced to six");
         publisher.timeAdvanceRequest(six);
         awaitBoth(callbackModel, publisher, receiver,
            () -> publisherCallbacks.grants.size() >= 3
               && receiverCallbacks.grants.size() == 2,
            "directed queued-TSO safe grant at six");

         DirectedInteraction received = receiverCallbacks.timedDirectedInteractions.get(0);
         JavaTckSupport.check(receiverCallbacks.directedCallbackCount == 1
               && received.interactionClass.equals(receiverInteraction)
               && received.object.equals(target)
               && received.parameters.isEmpty()
               && Arrays.equals(received.tag, tag)
               && received.producer.equals(producer)
               && received.time.equals(six)
               && received.sentOrder == OrderType.TIMESTAMP
               && received.receivedOrder == OrderType.TIMESTAMP
               && received.retraction != null,
            "the queued timestamped directed interaction lost its target, producer, tag, time, or order");
         JavaTckSupport.check(receiverCallbacks.grants.size() == 2
               && six.equals(receiverCallbacks.grants.get(1)),
            "time-constrained directed receiver did not receive its grant at six");
         JavaTckSupport.check(receiverCallbacks.callbackOrder.size()
               == callbackOrderBeforeTsoAdvance.size() + 2
               && "directed".equals(
                  receiverCallbacks.callbackOrder.get(callbackOrderBeforeTsoAdvance.size()))
               && "grant".equals(receiverCallbacks.callbackOrder.get(
                  receiverCallbacks.callbackOrder.size() - 1)),
            "the queued timestamped directed interaction did not precede its time-advance grant");

         receiver.disableTimeConstrained();
         receiverConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass);
         receiverDirectedSubscribed = false;
         publisher.unpublishObjectClassDirectedInteractions(publisherObjectClass);
         publisherDirectedPublished = false;
         receiver.unsubscribeObjectClassAttributes(receiverObjectClass, receiverAttributes);
         receiverAttributesSubscribed = false;
         publisher.unpublishObjectClassAttributes(publisherObjectClass, publisherAttributes);
         publisherAttributesPublished = false;
         receiver.resignFederationExecution(ResignAction.NO_ACTION);
         receiverJoined = false;
         publisher.resignFederationExecution(ResignAction.CANCEL_THEN_DELETE_THEN_DIVEST);
         publisherJoined = false;
         publisher.destroyFederationExecution(federation);
         created = false;
      } finally {
         if (receiverConstrained && receiverJoined) {
            try { receiver.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (receiverDirectedSubscribed && receiverJoined && receiverObjectClass != null) {
            try { receiver.unsubscribeObjectClassDirectedInteractions(receiverObjectClass); }
            catch (Exception ignored) { }
         }
         if (publisherDirectedPublished && publisherJoined && publisherObjectClass != null) {
            try { publisher.unpublishObjectClassDirectedInteractions(publisherObjectClass); }
            catch (Exception ignored) { }
         }
         if (receiverAttributesSubscribed && receiverJoined && receiverObjectClass != null
               && receiverAttribute != null) {
            try { receiver.unsubscribeObjectClassAttributes(receiverObjectClass,
               JavaTckSupport.attributeSet(receiver, receiverAttribute)); }
            catch (Exception ignored) { }
         }
         if (publisherAttributesPublished && publisherJoined && publisherObjectClass != null
               && publisherAttribute != null) {
            try { publisher.unpublishObjectClassAttributes(publisherObjectClass,
               JavaTckSupport.attributeSet(publisher, publisherAttribute)); }
            catch (Exception ignored) { }
         }
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
   private static LogicalTime<?, ?> offset(LogicalTime start,
         LogicalTimeInterval interval, int steps) throws Exception {
      LogicalTime result = start;
      for (int index = 0; index < steps; ++index) {
         result = (LogicalTime) result.add(interval);
      }
      return result;
   }

   private static void await(CallbackModel model, RTIambassador ambassador,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(CallbackModel model, RTIambassador first,
         RTIambassador second, BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (model == CallbackModel.HLA_EVOKED) {
            first.evokeCallback(0.0);
            second.evokeCallback(0.0);
         }
         Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void resign(RTIambassador ambassador, ResignAction action) {
      try { ambassador.resignFederationExecution(action); } catch (Exception ignored) { }
   }

   private static void disconnect(RTIambassador ambassador) {
      try { ambassador.disconnect(); } catch (Exception ignored) { }
   }

   private static final class DirectedInteraction {
      private InteractionClassHandle interactionClass;
      private ObjectInstanceHandle object;
      private ParameterHandleValueMap parameters;
      private byte[] tag;
      private TransportationTypeHandle transportation;
      private FederateHandle producer;
      private LogicalTime<?, ?> time;
      private OrderType sentOrder;
      private OrderType receivedOrder;
      private MessageRetractionHandle retraction;
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private volatile int directedCallbackCount;
      private final List<ObjectInstanceHandle> discoveries = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<DirectedInteraction> timedDirectedInteractions =
         new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(), new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  ++timeRegulationEnabled;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  ++timeConstrainedEnabled;
                  callbackOrder.add("constrained");
               } else if ("discoverObjectInstance".equals(name) && arguments != null
                     && arguments.length >= 3) {
                  discoveries.add((ObjectInstanceHandle) arguments[0]);
               } else if ("receiveDirectedInteraction".equals(name) && arguments != null) {
                  ++directedCallbackCount;
                  callbackOrder.add("directed");
                  if (arguments.length == 10) {
                     DirectedInteraction interaction = new DirectedInteraction();
                     interaction.interactionClass = (InteractionClassHandle) arguments[0];
                     interaction.object = (ObjectInstanceHandle) arguments[1];
                     interaction.parameters = ((ParameterHandleValueMap) arguments[2]).clone();
                     interaction.tag = ((byte[]) arguments[3]).clone();
                     interaction.transportation = (TransportationTypeHandle) arguments[4];
                     interaction.producer = (FederateHandle) arguments[5];
                     interaction.time = (LogicalTime<?, ?>) arguments[6];
                     interaction.sentOrder = (OrderType) arguments[7];
                     interaction.receivedOrder = (OrderType) arguments[8];
                     interaction.retraction = (MessageRetractionHandle) arguments[9];
                     timedDirectedInteractions.add(interaction);
                  }
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK pending directed TSO callbacks";
                  }
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
