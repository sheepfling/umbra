package org.hla.rti.tck;

import hla.rti1516_2025.CallbackModel;
import hla.rti1516_2025.FederateAmbassador;
import hla.rti1516_2025.FederateHandle;
import hla.rti1516_2025.InteractionClassHandle;
import hla.rti1516_2025.MessageRetractionHandle;
import hla.rti1516_2025.MessageRetractionReturn;
import hla.rti1516_2025.OrderType;
import hla.rti1516_2025.ParameterHandle;
import hla.rti1516_2025.ParameterHandleValueMap;
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
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;

/** Checks that a pending NMR retains a same-time interaction cohort arriving later. */
final class NmrFutureTsoArrivalWhilePendingTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NmrFutureTsoArrivalWhilePendingTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      JavaTckSupport.check(callbackModel == CallbackModel.HLA_EVOKED,
         "NMR future-arrival callback-order assertions require HLA_EVOKED callbacks");
      RTIambassador requester = factory.getRtiAmbassador();
      RTIambassador publisher = factory.getRtiAmbassador();
      Recorder requesterCallbacks = new Recorder();
      Recorder publisherCallbacks = new Recorder();
      String federation = "java-tck-nmr-future-arrival-" + UUID.randomUUID();
      boolean requesterConnected = false;
      boolean publisherConnected = false;
      boolean federationCreated = false;
      boolean requesterJoined = false;
      boolean publisherJoined = false;
      boolean requesterSubscribed = false;
      boolean publisherPublished = false;
      boolean requesterConstrained = false;
      boolean publisherRegulating = false;
      InteractionClassHandle requesterInteraction = null;
      InteractionClassHandle publisherInteraction = null;
      try {
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         requester.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         requester.joinFederationExecution("java-tck-nmr-future-requester", federation);
         requesterJoined = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-nmr-future-publisher", federation);
         publisherJoined = true;

         requesterInteraction = requester.getInteractionClassHandle(interactionName);
         publisherInteraction = publisher.getInteractionClassHandle(interactionName);
         ParameterHandle requesterParameter = requester.getParameterHandle(
            requesterInteraction, parameterName);
         ParameterHandle publisherParameter = publisher.getParameterHandle(
            publisherInteraction, parameterName);
         JavaTckSupport.check(requesterInteraction != null && publisherInteraction != null
               && requesterParameter != null && publisherParameter != null
               && requesterInteraction.equals(publisherInteraction)
               && requesterParameter.equals(publisherParameter),
            "NMR future-arrival FOM did not resolve matching interaction handles");
         requester.subscribeInteractionClass(requesterInteraction);
         requesterSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publisherPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         JavaTckSupport.check(requesterFactory.getName().equals(publisherFactory.getName()),
            "NMR future-arrival members selected different logical-time factories");
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         JavaTckSupport.check(requesterInitial.equals(publisherInitial),
            "NMR future-arrival members began at different logical times");
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMR future-arrival requester Time Constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(publisher, () -> publisherCallbacks.timeRegulationEnabled == 1,
            "NMR future-arrival publisher Time Regulation callback");

         LogicalTime<?, ?> requestTime = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 5);
         byte[] firstPayload = {0x4e, 0x4d, 0x52};
         byte[] secondPayload = {0x4e, 0x4d, 0x52, 0x32};
         byte[] firstTag = {0x58};
         byte[] secondTag = {0x59};

         requester.nextMessageRequest(requestTime);
         verifyPendingWindow(requester, publisher, requesterCallbacks, publisherCallbacks,
            "NMR completed before a post-request TSO arrival existed");

         MessageRetractionHandle firstRetraction = sendTimestamped(
            publisher, publisherInteraction, publisherParameter, firstPayload, firstTag,
            messageTime);
         MessageRetractionHandle secondRetraction = sendTimestamped(
            publisher, publisherInteraction, publisherParameter, secondPayload, secondTag,
            messageTime);
         verifyPendingWindow(requester, publisher, requesterCallbacks, publisherCallbacks,
            "NMR delivered the late TSO cohort or granted before its timestamp became safe");

         int callbackOrderBeforeDelivery = requesterCallbacks.callbackOrder.size();
         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(requester, publisher,
            () -> requesterCallbacks.timestamped.size() >= 2
               && requesterCallbacks.grants.size() >= 1
               && publisherCallbacks.grants.size() >= 1,
            "NMR post-request timestamped interaction cohort and grant");

         JavaTckSupport.check(requesterCallbacks.timestamped.size() == 2
               && requesterCallbacks.grants.size() == 1
               && publisherCallbacks.grants.size() == 1,
            "NMR lost or duplicated a post-request TSO cohort member or grant");
         TimedInteraction first = requesterCallbacks.timestamped.get(0);
         TimedInteraction second = requesterCallbacks.timestamped.get(1);
         boolean firstThenSecond = matches(first, requesterInteraction, requesterParameter,
               publisherHandle, firstPayload, firstTag, messageTime, firstRetraction)
            && matches(second, requesterInteraction, requesterParameter, publisherHandle,
               secondPayload, secondTag, messageTime, secondRetraction);
         boolean secondThenFirst = matches(first, requesterInteraction, requesterParameter,
               publisherHandle, secondPayload, secondTag, messageTime, secondRetraction)
            && matches(second, requesterInteraction, requesterParameter, publisherHandle,
               firstPayload, firstTag, messageTime, firstRetraction);
         JavaTckSupport.check(firstThenSecond || secondThenFirst,
            "NMR lost, duplicated, or changed a same-time post-request TSO cohort member");
         JavaTckSupport.check(requesterCallbacks.grants.get(0).equals(messageTime)
               && publisherCallbacks.grants.get(0).equals(publisherTarget)
               && requester.queryLogicalTime().equals(messageTime),
            "NMR did not grant at the late TSO timestamp after the publisher advanced");
         List<String> callbackOrder = requesterCallbacks.callbackOrder;
         JavaTckSupport.check(callbackOrder.size() == callbackOrderBeforeDelivery + 3
               && callbackOrder.get(callbackOrderBeforeDelivery).equals("interaction")
               && callbackOrder.get(callbackOrderBeforeDelivery + 1).equals("interaction")
               && callbackOrder.get(callbackOrderBeforeDelivery + 2).equals("grant"),
            "NMR delivered the post-request TSO cohort after its grant");

         requester.disableTimeConstrained();
         requesterConstrained = false;
         publisher.disableTimeRegulation();
         publisherRegulating = false;
         requester.unsubscribeInteractionClass(requesterInteraction);
         requesterSubscribed = false;
         publisher.unpublishInteractionClass(publisherInteraction);
         publisherPublished = false;
         requester.resignFederationExecution(ResignAction.NO_ACTION);
         requesterJoined = false;
         publisher.resignFederationExecution(ResignAction.NO_ACTION);
         publisherJoined = false;
         requester.destroyFederationExecution(federation);
         federationCreated = false;
      } finally {
         if (requesterConstrained && requesterJoined) {
            try { requester.disableTimeConstrained(); } catch (Exception ignored) { }
         }
         if (publisherRegulating && publisherJoined) {
            try { publisher.disableTimeRegulation(); } catch (Exception ignored) { }
         }
         if (requesterSubscribed && requesterJoined && requesterInteraction != null) {
            try { requester.unsubscribeInteractionClass(requesterInteraction); }
            catch (Exception ignored) { }
         }
         if (publisherPublished && publisherJoined && publisherInteraction != null) {
            try { publisher.unpublishInteractionClass(publisherInteraction); }
            catch (Exception ignored) { }
         }
         if (requesterJoined) resign(requester);
         if (publisherJoined) resign(publisher);
         if (federationCreated) {
            try { requester.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static MessageRetractionHandle sendTimestamped(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter, byte[] payload,
         byte[] tag, LogicalTime<?, ?> time) throws Exception {
      ParameterHandleValueMap values =
         publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      MessageRetractionReturn result = publisher.sendInteraction(
         interaction, values, tag.clone(), time);
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null,
         "NMR future-arrival TSO send returned no valid retraction handle");
      return result.handle;
   }

   private static boolean matches(TimedInteraction received,
         InteractionClassHandle expectedInteraction, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedPayload, byte[] expectedTag,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction) {
      return expectedInteraction.equals(received.interaction)
         && received.parameters.size() == 1
         && Arrays.equals(expectedPayload, received.parameters.get(expectedParameter))
         && Arrays.equals(expectedTag, received.tag)
         && expectedProducer.equals(received.producer)
         && expectedTime.equals(received.time)
         && received.sentOrder == OrderType.TIMESTAMP
         && received.receivedOrder == OrderType.TIMESTAMP
         && expectedRetraction.equals(received.retraction);
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

   private static void await(RTIambassador ambassador, BooleanSupplier condition,
         String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         ambassador.evokeCallback(0.0);
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void verifyPendingWindow(RTIambassador requester,
         RTIambassador publisher, Recorder requesterCallbacks,
         Recorder publisherCallbacks, String description) throws Exception {
      long deadline = System.nanoTime() + TimeUnit.MILLISECONDS.toNanos(40);
      do {
         requester.evokeCallback(0.0);
         publisher.evokeCallback(0.0);
         JavaTckSupport.check(requesterCallbacks.timestamped.isEmpty()
               && requesterCallbacks.grants.isEmpty()
               && publisherCallbacks.grants.isEmpty()
               && requesterCallbacks.callbackOrder.isEmpty(), description);
         Thread.sleep(1);
      } while (System.nanoTime() < deadline);
   }

   private static void awaitBoth(RTIambassador requester, RTIambassador publisher,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         requester.evokeCallback(0.0);
         publisher.evokeCallback(0.0);
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

   private static final class TimedInteraction {
      private final InteractionClassHandle interaction;
      private final ParameterHandleValueMap parameters;
      private final byte[] tag;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         producer = (FederateHandle) arguments[4];
         time = (LogicalTime<?, ?>) arguments[6];
         sentOrder = (OrderType) arguments[7];
         receivedOrder = (OrderType) arguments[8];
         retraction = (MessageRetractionHandle) arguments[9];
      }
   }

   private static final class Recorder {
      private volatile int timeRegulationEnabled;
      private volatile int timeConstrainedEnabled;
      private final List<TimedInteraction> timestamped = new CopyOnWriteArrayList<>();
      private final List<LogicalTime<?, ?>> grants = new CopyOnWriteArrayList<>();
      private final List<String> callbackOrder = new CopyOnWriteArrayList<>();

      private FederateAmbassador proxy() {
         return (FederateAmbassador) Proxy.newProxyInstance(
            FederateAmbassador.class.getClassLoader(),
            new Class<?>[] {FederateAmbassador.class},
            (proxy, method, arguments) -> {
               String name = method.getName();
               if ("timeRegulationEnabled".equals(name)) {
                  timeRegulationEnabled++;
               } else if ("timeConstrainedEnabled".equals(name)) {
                  timeConstrainedEnabled++;
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 10) {
                  timestamped.add(new TimedInteraction(arguments));
                  callbackOrder.add("interaction");
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK NMR future-arrival callbacks";
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
