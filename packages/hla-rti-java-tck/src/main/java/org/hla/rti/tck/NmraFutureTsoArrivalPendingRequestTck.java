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

/** Checks future and inclusive-boundary TSO arrivals while NMRA is pending. */
final class NmraFutureTsoArrivalPendingRequestTck {
   private static final long CALLBACK_TIMEOUT_NANOS = 10_000_000_000L;

   private NmraFutureTsoArrivalPendingRequestTck() { }

   static void run(RtiFactory factory, String fom, String timeImplementation,
         String interactionClassName, String parameterName, CallbackModel callbackModel)
         throws Exception {
      RTIambassador publisher = factory.getRtiAmbassador();
      RTIambassador requester = factory.getRtiAmbassador();
      Recorder publisherCallbacks = new Recorder();
      Recorder requesterCallbacks = new Recorder();
      String federation = "java-tck-nmra-future-tso-arrival-" + UUID.randomUUID();
      boolean publisherConnected = false;
      boolean requesterConnected = false;
      boolean federationCreated = false;
      boolean publisherJoined = false;
      boolean requesterJoined = false;
      boolean publisherPublished = false;
      boolean requesterSubscribed = false;
      boolean publisherRegulating = false;
      boolean requesterConstrained = false;
      InteractionClassHandle publisherInteraction = null;
      InteractionClassHandle requesterInteraction = null;
      try {
         publisher.connect(publisherCallbacks.proxy(), callbackModel);
         publisherConnected = true;
         requester.connect(requesterCallbacks.proxy(), callbackModel);
         requesterConnected = true;
         publisher.createFederationExecution(federation, fom, timeImplementation);
         federationCreated = true;
         FederateHandle publisherHandle = publisher.joinFederationExecution(
            "java-tck-nmra-future-publisher", federation);
         publisherJoined = true;
         requester.joinFederationExecution("java-tck-nmra-future-requester", federation);
         requesterJoined = true;

         publisherInteraction = publisher.getInteractionClassHandle(interactionClassName);
         requesterInteraction = requester.getInteractionClassHandle(interactionClassName);
         ParameterHandle publisherParameter =
            publisher.getParameterHandle(publisherInteraction, parameterName);
         ParameterHandle requesterParameter =
            requester.getParameterHandle(requesterInteraction, parameterName);
         JavaTckSupport.check(publisherInteraction != null && requesterInteraction != null
               && publisherParameter != null && requesterParameter != null
               && publisherInteraction.equals(requesterInteraction)
               && publisherParameter.equals(requesterParameter),
            "NMRA future-arrival FOM returned inconsistent interaction handles");

         requester.subscribeInteractionClass(requesterInteraction);
         requesterSubscribed = true;
         publisher.publishInteractionClass(publisherInteraction);
         publisherPublished = true;
         publisher.changeInteractionOrderType(publisherInteraction, OrderType.TIMESTAMP);

         LogicalTimeFactory<?, ?> publisherFactory = publisher.getTimeFactory();
         LogicalTimeFactory<?, ?> requesterFactory = requester.getTimeFactory();
         JavaTckSupport.check(publisherFactory.getName().equals(requesterFactory.getName()),
            "NMRA future-arrival members selected different logical-time factories");
         LogicalTime<?, ?> publisherInitial = publisher.queryLogicalTime();
         LogicalTime<?, ?> requesterInitial = requester.queryLogicalTime();
         JavaTckSupport.check(publisherInitial.equals(requesterInitial),
            "NMRA future-arrival members began at different logical times");
         LogicalTimeInterval publisherEpsilon =
            (LogicalTimeInterval) publisherFactory.makeEpsilon();
         LogicalTimeInterval requesterEpsilon =
            (LogicalTimeInterval) requesterFactory.makeEpsilon();

         requester.enableTimeConstrained();
         requesterConstrained = true;
         await(requester, callbackModel,
            () -> requesterCallbacks.timeConstrainedEnabled == 1,
            "NMRA future-arrival requester Time Constrained callback");
         publisher.enableTimeRegulation(publisherEpsilon);
         publisherRegulating = true;
         await(publisher, callbackModel,
            () -> publisherCallbacks.timeRegulationEnabled == 1,
            "NMRA future-arrival publisher Time Regulation callback");

         LogicalTime<?, ?> messageTime = offset(publisherInitial, publisherEpsilon, 5);
         LogicalTime<?, ?> requestBoundary = offset(requesterInitial, requesterEpsilon, 10);
         LogicalTime<?, ?> publisherTarget = offset(publisherInitial, publisherEpsilon, 4);
         byte[] payload = {0x4e, 0x4d, 0x52, 0x41};
         byte[] tag = {0x4e, 0x4d, 0x52, 0x41, 0x2d, 0x46};

         requester.nextMessageRequestAvailable(requestBoundary);
         pump(requester, callbackModel);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
               && requesterCallbacks.timestampedInteractions.isEmpty(),
            "NMRA future-arrival requester completed before a qualifying TSO arrived");

         MessageRetractionReturn sent = send(publisher, publisherInteraction,
            publisherParameter, payload, tag, messageTime);
         pump(requester, callbackModel);
         pump(publisher, callbackModel);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
               && requesterCallbacks.timestampedInteractions.isEmpty(),
            "NMRA future-arrival requester crossed the publisher's time boundary");

         publisher.timeAdvanceRequest(publisherTarget);
         awaitBoth(requester, publisher, callbackModel,
            () -> publisherCallbacks.grants.size() >= 1
               && requesterCallbacks.timestampedInteractions.size() >= 1
               && requesterCallbacks.grants.size() >= 1,
            "NMRA future-arrival delivery and grant");
         JavaTckSupport.check(publisherCallbacks.grants.size() == 1
               && publisherTarget.equals(publisherCallbacks.grants.get(0)),
            "NMRA future-arrival publisher did not reach its requested time");
         JavaTckSupport.check(requesterCallbacks.timestampedInteractions.size() == 1
               && requesterCallbacks.ordinaryInteractions.isEmpty()
               && requesterCallbacks.grants.size() == 1,
            "NMRA future-arrival requester received duplicate or incomplete callbacks");
         TimedInteraction first = requesterCallbacks.timestampedInteractions.get(0);
         checkInteraction(first, requesterInteraction, requesterParameter, publisherHandle,
            payload, tag, messageTime, sent.handle, "NMRA future-arrival callback");
         JavaTckSupport.check(requesterCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "grant")),
            "NMRA future-arrival grant did not follow its TSO delivery");
         JavaTckSupport.check(messageTime.equals(requesterCallbacks.grants.get(0))
               && messageTime.equals(requester.queryLogicalTime()),
            "NMRA future-arrival requester did not grant and advance at the message time");

         requesterCallbacks.grants.clear();
         publisherCallbacks.grants.clear();
         requesterCallbacks.callbackOrder.clear();
         int priorInteractions = requesterCallbacks.timestampedInteractions.size();
         LogicalTime<?, ?> boundaryMessageTime =
            offset(publisherInitial, publisherEpsilon, 10);
         LogicalTime<?, ?> boundaryPublisherTarget =
            offset(publisherInitial, publisherEpsilon, 11);
         byte[] boundaryPayload = {0x42, 0x4e, 0x44};
         byte[] boundaryTag = {0x42, 0x4e, 0x44, 0x2d, 0x46};

         requester.nextMessageRequestAvailable(requestBoundary);
         pump(requester, callbackModel);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
               && requesterCallbacks.timestampedInteractions.size() == priorInteractions,
            "NMRA boundary requester completed before the boundary TSO arrived");

         MessageRetractionReturn boundarySent = send(publisher, publisherInteraction,
            publisherParameter, boundaryPayload, boundaryTag, boundaryMessageTime);
         pump(requester, callbackModel);
         pump(publisher, callbackModel);
         JavaTckSupport.check(requesterCallbacks.grants.isEmpty()
               && requesterCallbacks.timestampedInteractions.size() == priorInteractions,
            "NMRA boundary requester crossed the publisher's time boundary");

         publisher.timeAdvanceRequest(boundaryPublisherTarget);
         awaitBoth(requester, publisher, callbackModel,
            () -> publisherCallbacks.grants.size() >= 1
               && requesterCallbacks.timestampedInteractions.size() >= priorInteractions + 1
               && requesterCallbacks.grants.size() >= 1,
            "NMRA inclusive-boundary future-arrival delivery and grant");
         JavaTckSupport.check(publisherCallbacks.grants.size() == 1
               && boundaryPublisherTarget.equals(publisherCallbacks.grants.get(0)),
            "NMRA boundary publisher did not reach its requested time");
         JavaTckSupport.check(requesterCallbacks.timestampedInteractions.size()
               == priorInteractions + 1 && requesterCallbacks.ordinaryInteractions.isEmpty()
               && requesterCallbacks.grants.size() == 1,
            "NMRA boundary requester received duplicate or incomplete callbacks");
         TimedInteraction boundary = requesterCallbacks.timestampedInteractions.get(
            priorInteractions);
         checkInteraction(boundary, requesterInteraction, requesterParameter, publisherHandle,
            boundaryPayload, boundaryTag, boundaryMessageTime, boundarySent.handle,
            "NMRA inclusive-boundary callback");
         JavaTckSupport.check(requesterCallbacks.callbackOrder.equals(
               Arrays.asList("interaction", "grant")),
            "NMRA did not deliver the TSO at the inclusive request boundary before its grant");
         JavaTckSupport.check(boundaryMessageTime.equals(requesterCallbacks.grants.get(0))
               && boundaryMessageTime.equals(requester.queryLogicalTime()),
            "NMRA inclusive-boundary requester did not grant at the message time");

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
         publisher.destroyFederationExecution(federation);
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
            try { publisher.destroyFederationExecution(federation); }
            catch (Exception ignored) { }
         }
         if (requesterConnected) disconnect(requester);
         if (publisherConnected) disconnect(publisher);
      }
   }

   private static MessageRetractionReturn send(RTIambassador publisher,
         InteractionClassHandle interaction, ParameterHandle parameter,
         byte[] payload, byte[] tag, LogicalTime<?, ?> time) throws Exception {
      ParameterHandleValueMap values =
         publisher.getParameterHandleValueMapFactory().create(1);
      values.put(parameter, payload.clone());
      MessageRetractionReturn result = publisher.sendInteraction(
         interaction, values, tag.clone(), time);
      JavaTckSupport.check(result != null && result.retractionHandleIsValid
            && result.handle != null,
         "timestamped future-arrival Send Interaction returned no valid retraction handle");
      return result;
   }

   private static void checkInteraction(TimedInteraction received,
         InteractionClassHandle expectedInteraction, ParameterHandle expectedParameter,
         FederateHandle expectedProducer, byte[] expectedPayload, byte[] expectedTag,
         LogicalTime<?, ?> expectedTime, MessageRetractionHandle expectedRetraction,
         String description) {
      JavaTckSupport.check(expectedInteraction.equals(received.interaction)
            && received.parameters.size() == 1
            && Arrays.equals(expectedPayload, received.parameters.get(expectedParameter))
            && Arrays.equals(expectedTag, received.tag)
            && expectedProducer.equals(received.producer)
            && expectedTime.equals(received.time)
            && received.sentOrder == OrderType.TIMESTAMP
            && received.receivedOrder == OrderType.TIMESTAMP
            && expectedRetraction.equals(received.retraction),
         description + " changed its interaction, payload, time, order, or retraction metadata");
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

   private static void pump(RTIambassador ambassador, CallbackModel callbackModel)
         throws Exception {
      if (callbackModel == CallbackModel.HLA_EVOKED) {
         JavaTckSupport.drain(ambassador);
      } else {
         Thread.yield();
      }
   }

   private static void await(RTIambassador ambassador, CallbackModel callbackModel,
         BooleanSupplier condition, String description) throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) ambassador.evokeCallback(0.0);
         else Thread.yield();
      }
      JavaTckSupport.check(condition.getAsBoolean(), "timed out waiting for " + description);
   }

   private static void awaitBoth(RTIambassador requester, RTIambassador publisher,
         CallbackModel callbackModel, BooleanSupplier condition, String description)
         throws Exception {
      long deadline = System.nanoTime() + CALLBACK_TIMEOUT_NANOS;
      while (!condition.getAsBoolean() && System.nanoTime() < deadline) {
         if (callbackModel == CallbackModel.HLA_EVOKED) {
            requester.evokeCallback(0.0);
            publisher.evokeCallback(0.0);
         } else {
            Thread.yield();
         }
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
      private final TransportationTypeHandle transportation;
      private final FederateHandle producer;
      private final LogicalTime<?, ?> time;
      private final OrderType sentOrder;
      private final OrderType receivedOrder;
      private final MessageRetractionHandle retraction;

      private TimedInteraction(Object[] arguments) {
         interaction = (InteractionClassHandle) arguments[0];
         parameters = ((ParameterHandleValueMap) arguments[1]).clone();
         tag = ((byte[]) arguments[2]).clone();
         transportation = (TransportationTypeHandle) arguments[3];
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
      private final List<TimedInteraction> timestampedInteractions =
         new CopyOnWriteArrayList<>();
      private final List<Object[]> ordinaryInteractions = new CopyOnWriteArrayList<>();
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
                  timestampedInteractions.add(new TimedInteraction(arguments));
                  callbackOrder.add("interaction");
               } else if ("receiveInteraction".equals(name) && arguments != null
                     && arguments.length == 6) {
                  ordinaryInteractions.add(arguments.clone());
                  callbackOrder.add("interaction");
               } else if ("timeAdvanceGrant".equals(name) && arguments != null
                     && arguments.length == 1) {
                  grants.add((LogicalTime<?, ?>) arguments[0]);
                  callbackOrder.add("grant");
               }
               if (method.getDeclaringClass() == Object.class) {
                  if ("toString".equals(name)) {
                     return "Java TCK NMRA future-arrival callbacks";
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
